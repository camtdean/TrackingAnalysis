#!/bin/bash
#
# submitCondorJobs.sh  --  queue one condor job per sublist in a top list made by getFiles.sh.
#
#   ./submitCondorJobs.sh fileLists/<set>/inputDSTlists.txt
#
# It writes a complete submit file next to the top list (submit_<date>.job: myCondor.job plus
# the values for this submission, so you can see exactly what was run), then calls condor_submit
# on it. myCondor.job itself is never edited.

set -euo pipefail

DATA_DIR=$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)
JOB_TEMPLATE=${DATA_DIR}/myCondor.job
MACRO_DIR=${DATA_DIR}/macro

usage() {
  cat <<EOF
Usage: $(basename "$0") [options] <top list>

  <top list>            inputDSTlists.txt written by getFiles.sh (one sublist path per line)

Options:
  -e, --events N        events per job over its whole list, 0 = all           (default 0)
      --skip N          events to skip at the start of each list              (default 0)
  -o, --out-dir D       where the nTuples go   (default ${DATA_DIR}/root/<list set name>)
  -m, --memory MB       first memory request                                  (from myCondor.job)
      --memory-step MB  added each time condor retries a job that ran out     (from myCondor.job)
      --max-memory MB   ceiling for the retries                               (from myCondor.job)
      --limit N         submit only the first N sublists (a quick test before the full set)
  -y, --yes             do not ask for confirmation
      --dry-run         write the submit file and show what would be submitted, do not submit
  -h, --help

Examples:
  $(basename "$0") --limit 2 -e 2000 fileLists/run3pp_ana573_2026p003_v001_DST_TRKR_TRACKS/inputDSTlists.txt
  $(basename "$0") fileLists/run3pp_ana573_2026p003_v001_DST_TRKR_TRACKS/inputDSTlists.txt
EOF
}

die() { echo "ERROR: $*" >&2; exit 1; }

NEVENTS=0; NSKIP=0; OUT_DIR=""; MEMORY=""; MEMORY_STEP=""; MAX_MEMORY=""; LIMIT=""
ASSUME_YES=false; DRY_RUN=false; TOP=""

num() { [[ "$2" =~ ^[0-9]+$ ]] || die "$1 needs a whole number (got '$2')"; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    -e|--events)      [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; NEVENTS=$2; shift 2 ;;
    --skip)           [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; NSKIP=$2; shift 2 ;;
    -o|--out-dir)     [[ $# -ge 2 ]] || die "$1 needs a value"; OUT_DIR=$2; shift 2 ;;
    -m|--memory)      [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; MEMORY=${2}MB; shift 2 ;;
    --memory-step)    [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; MEMORY_STEP=$2; shift 2 ;;
    --max-memory)     [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; MAX_MEMORY=${2}MB; shift 2 ;;
    --limit)          [[ $# -ge 2 ]] || die "$1 needs a value"; num "$1" "$2"; LIMIT=$2; shift 2 ;;
    -y|--yes)         ASSUME_YES=true; shift ;;
    --dry-run)        DRY_RUN=true; shift ;;
    -h|--help)        usage; exit 0 ;;
    -*)               usage >&2; die "unknown option: $1" ;;
    *)                [[ -z "${TOP}" ]] || die "only one top list at a time (got '${TOP}' and '$1')"; TOP=$1; shift ;;
  esac
done

[[ -n "${TOP}" ]] || { usage >&2; die "give the top list (inputDSTlists.txt) written by getFiles.sh"; }
[[ -r "${TOP}" ]] || die "cannot read ${TOP}"
[[ -r "${JOB_TEMPLATE}" ]] || die "cannot read ${JOB_TEMPLATE}"
[[ -f "${MACRO_DIR}/runHFreco.sh" && -f "${MACRO_DIR}/Fun4All_HF.C" ]] || die "${MACRO_DIR} must hold runHFreco.sh and Fun4All_HF.C"
[[ -x "${MACRO_DIR}/runHFreco.sh" ]] || die "${MACRO_DIR}/runHFreco.sh is not executable (chmod +x)"
${DRY_RUN} || command -v condor_submit >/dev/null 2>&1 || die "condor_submit not found (run on a machine that can submit, or use --dry-run)"

TOP=$(cd "$(dirname "${TOP}")" && pwd)/$(basename "${TOP}")
SET_DIR=$(dirname "${TOP}")
SET_NAME=$(basename "${SET_DIR}")

# the top list must hold absolute paths of lists that exist and are not empty
queue_list=$(mktemp "${SET_DIR}/.queue.XXXXXX")
trap 'rm -f "${queue_list}"' EXIT
nbad=0; njobs=0
while IFS= read -r line || [[ -n "${line}" ]]; do
  line=${line//[$'\r']/}
  [[ -z "${line}" || "${line}" == \#* ]] && continue
  if [[ "${line}" != /* ]]; then echo "ERROR: not an absolute path: ${line}" >&2; nbad=$((nbad+1)); continue; fi
  if [[ ! -s "${line}" ]]; then echo "ERROR: missing or empty list: ${line}" >&2; nbad=$((nbad+1)); continue; fi
  echo "${line}" >> "${queue_list}"; njobs=$((njobs+1))
done < "${TOP}"
[[ ${nbad} -eq 0 ]] || die "${nbad} problem(s) in ${TOP}; fix them or rerun getFiles.sh"
[[ ${njobs} -gt 0 ]] || die "${TOP} lists no jobs"

if [[ -n "${LIMIT}" && ${LIMIT} -lt ${njobs} ]]; then
  head -n "${LIMIT}" "${queue_list}" > "${queue_list}.head" && mv "${queue_list}.head" "${queue_list}"
  njobs=${LIMIT}
fi

[[ -n "${OUT_DIR}" ]] || OUT_DIR=${DATA_DIR}/root/${SET_NAME}
[[ "${OUT_DIR}" == /* ]] || OUT_DIR=${PWD}/${OUT_DIR}
OUT_DIR=${OUT_DIR%/}/
LOG_DIR=${DATA_DIR}/log/${SET_NAME}
if ! ${DRY_RUN}; then
  mkdir -p "${OUT_DIR}" || die "cannot create ${OUT_DIR}"
  mkdir -p "${LOG_DIR}" || die "cannot create ${LOG_DIR}"
fi

stamp=$(date +%Y%m%d_%H%M%S)_$$
JOB_FILE=${SET_DIR}/submit_${stamp}.job
QUEUE_FILE=${SET_DIR}/queue_${stamp}.txt
cp "${queue_list}" "${QUEUE_FILE}"

{
  cat "${JOB_TEMPLATE}"
  echo ""
  echo "# ---- values for this submission (${stamp}) ----"
  echo "submitDir  = ${DATA_DIR}"
  echo "macroDir   = ${MACRO_DIR}"
  echo "outDir     = ${OUT_DIR}"
  echo "logDir     = ${LOG_DIR}"
  echo "nEvents    = ${NEVENTS}"
  echo "nSkip      = ${NSKIP}"
  [[ -n "${MEMORY}"      ]] && echo "memory     = ${MEMORY}"
  [[ -n "${MEMORY_STEP}" ]] && echo "memoryStep = ${MEMORY_STEP}"
  [[ -n "${MAX_MEMORY}"  ]] && echo "maxMemory  = ${MAX_MEMORY}"
  echo "Queue dstList from ${QUEUE_FILE}"
} > "${JOB_FILE}"

last_value() { grep -E "^$1[[:space:]]*=" "${JOB_FILE}" | tail -1 | sed -E 's/^[^=]*=[[:space:]]*//'; }
mem_line="$(last_value memory) first request, +$(last_value memoryStep) MB per retry, up to $(last_value maxMemory)"
echo "List set    : ${SET_NAME}"
echo "Jobs        : ${njobs} (one per sublist, queue list ${QUEUE_FILE})"
echo "Events/job  : ${NEVENTS} (0 = all in the list)"
echo "Output      : ${OUT_DIR}"
echo "Logs        : ${LOG_DIR}"
echo "Submit file : ${JOB_FILE}"
echo "Memory      : ${mem_line}"

if ${DRY_RUN}; then
  echo "(dry run) would run: condor_submit ${JOB_FILE}"
  exit 0
fi

if ! ${ASSUME_YES} && [[ -t 0 ]]; then
  read -r -p "Submit ${njobs} job(s)? [y/N] " ans
  [[ "${ans}" == [yY]* ]] || { echo "Not submitted."; exit 1; }
fi

condor_submit "${JOB_FILE}"
