#!/bin/bash
#
# getFiles.sh  --  find the DSTs of your runs, split them into sublists, write the top list for condor.
#
#   1. edit the block "WHAT TO GET" below (run array, species, tag, DST type, DSTs per job)
#   2. ./getFiles.sh
#   3. ./submitCondorJobs.sh
#
# Output, under fileLists/ next to this script:
#   fileLists/inputDSTlists.txt                   the top list: one line per run (what submitCondorJobs.sh reads)
#   fileLists/<set>/jobs/run<RUN>.txt             that run's sublists (one condor submission per run)
#   fileLists/<set>/sublists/run<RUN>_NNNN.list   <= dstsPerJob DSTs each (one condor job each)
#   fileLists/<set>/runs/run<RUN>.list            every DST found for the run
# <set> is <species>_<tag>_<dsttype>. The top list holds the runs of this call only, and the lists of
# those runs are rebuilt each time, so rerunning picks up new files.
#
# Where the DSTs come from:
#   default       the FileCatalog: runs from psql (--all-runs), files from CreateDstList.pl
#   --from-dirs   search the production directories on lustre (always used for raw-hit DSTs)
#   --from-list   a list of DST names or paths you made yourself
#
# Any DST type works (DST_TRKR_TRACKS, DST_TRKR_SEED, DST_TRKR_CLUSTER, DST_COMBINED, DST_CALOFITTING, ...).
# Raw-hit DSTs (DST_STREAMING_EVENT) need every detector stream of one segment in one job, so they
# are searched for in the directories and are always one segment per sublist.
#
# Options (all optional, they override the block): ./getFiles.sh --help

# run under bash even if started as "sh getFiles.sh"
[ -n "${BASH_VERSION:-}" ] || exec bash "$0" "$@"

set -euo pipefail

# =============================== WHAT TO GET ===============================
runs=(79516 79515)                    # run numbers
species=run3pp                        # run3pp, run3auau, run3oo, run2pp
buildTag=ana581_2026p005_v001         # <anabuild>_<cdbtag>_<version>, i.e. the FileCatalog tag
dstType=DST_TRKR_TRACKS               # any DST type: DST_TRKR_TRACKS, DST_TRKR_SEED, DST_TRKR_CLUSTER, ...
dstsPerJob=100                        # DSTs in each condor job
# ===========================================================================

SPECIES=${species}; BUILD_TAG=${buildTag}; DST_TYPE=${dstType}; PER_JOB=${dstsPerJob}
RUN_TYPE=${RUN_TYPE:-physics}
PRODUCTION_DIR=${PRODUCTION_DIR:-/sphenix/lustre01/sphnxpro/production}

usage() {
  cat <<EOF
Usage: $(basename "$0") [options] [RUN ...]

With no arguments the settings at the top of this script ("WHAT TO GET") are used. RUN numbers on the
command line replace the run array.

Options:
  -s, --species S        run3pp | run3auau | run3oo | run2pp          (default: ${SPECIES})
  -b, --build-tag T      <anabuild>_<cdbtag>_<version>                (default: ${BUILD_TAG})
  -t, --dst-type T       DST_TRKR_TRACKS, DST_TRKR_SEED, DST_TRKR_CLUSTER, DST_COMBINED,
                         DST_CALOFITTING, DST_STREAMING_EVENT, ...    (default: ${DST_TYPE})
      --run-type R       production run type                           (default: ${RUN_TYPE})
  -n, --per-job N        DSTs per sublist = per condor job             (default: ${PER_JOB})
  -r, --runs-file F      file of run numbers (whitespace separated, '#' starts a comment)
      --all-runs         every run the FileCatalog has for this build tag and DST type
                         (psql FileCatalog) instead of the run array
      --from-dirs        look in the production directories instead of the FileCatalog
  -f, --from-list F      build the lists from file F, which holds DST names or paths, one per line.
                         Run numbers are read from the file names; RUN arguments, if given, filter.
      --full-paths       write full paths into the lists instead of bare file names
                         (bare names are resolved by Fun4All through the file catalogue;
                          full paths need --from-dirs or --from-list)
      --name TAG         name of the list set       (default: <species>_<build-tag>_<dst-type>)
  -o, --out-dir D        directory holding the list sets                (default: <this directory>/fileLists)
      --production-dir D top of the production area                     (default: ${PRODUCTION_DIR})
  -h, --help             this text

Examples:
  $(basename "$0")
  $(basename "$0") 79516
  $(basename "$0") --all-runs -b ana581_2026p005_v001
  $(basename "$0") -n 50 -t DST_TRKR_SEED -b ana571_2026p002_v001 79514 79515 79516
  $(basename "$0") -t DST_STREAMING_EVENT -b new_nocdbtag_v001 79516
  $(basename "$0") --from-dirs 79516
  $(basename "$0") -f my_files.txt
EOF
}

die() { echo "ERROR: $*" >&2; exit 1; }
note() { echo "$*"; }

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
OUT_ROOT=$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)/fileLists   # next to this script, wherever you run it from
NAME=""
RUNS_FILE=""
FROM_LIST=""
ALL_RUNS=false
FROM_DIRS=false
FULL_PATHS=false
RUNS=()
PER_JOB_SET=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    -s|--species)        [[ $# -ge 2 ]] || die "$1 needs a value"; SPECIES=$2; shift 2 ;;
    -b|--build-tag)      [[ $# -ge 2 ]] || die "$1 needs a value"; BUILD_TAG=$2; shift 2 ;;
    -t|--dst-type)       [[ $# -ge 2 ]] || die "$1 needs a value"; DST_TYPE=$2; shift 2 ;;
    --run-type)          [[ $# -ge 2 ]] || die "$1 needs a value"; RUN_TYPE=$2; shift 2 ;;
    -n|--per-job)        [[ $# -ge 2 ]] || die "$1 needs a value"; PER_JOB=$2; PER_JOB_SET=true; shift 2 ;;
    -r|--runs-file)      [[ $# -ge 2 ]] || die "$1 needs a value"; RUNS_FILE=$2; shift 2 ;;
    -f|--from-list)      [[ $# -ge 2 ]] || die "$1 needs a value"; FROM_LIST=$2; shift 2 ;;
    --all-runs)          ALL_RUNS=true; shift ;;
    --from-dirs)         FROM_DIRS=true; shift ;;
    --full-paths)        FULL_PATHS=true; shift ;;
    --name)              [[ $# -ge 2 ]] || die "$1 needs a value"; NAME=$2; shift 2 ;;
    -o|--out-dir)        [[ $# -ge 2 ]] || die "$1 needs a value"; OUT_ROOT=$2; shift 2 ;;
    --production-dir)    [[ $# -ge 2 ]] || die "$1 needs a value"; PRODUCTION_DIR=$2; shift 2 ;;
    --force)             shift ;;   # lists are always rebuilt; accepted for old command lines
    -h|--help)           usage; exit 0 ;;
    --)                  shift; while [[ $# -gt 0 ]]; do RUNS+=("$1"); shift; done ;;
    -*)                  usage >&2; die "unknown option: $1" ;;
    *)                   RUNS+=("$1"); shift ;;
  esac
done

if [[ -n "${RUNS_FILE}" ]]; then
  [[ -r "${RUNS_FILE}" ]] || die "cannot read runs file ${RUNS_FILE}"
  while read -r tok; do
    [[ -n "${tok}" ]] && RUNS+=("${tok}")
  done < <(sed 's/#.*//' "${RUNS_FILE}" | tr -s '[:space:]' '\n')
fi

if [[ ${#RUNS[@]} -eq 0 ]] && ! ${ALL_RUNS} && [[ -z "${FROM_LIST}" ]]; then
  RUNS=(${runs[@]+"${runs[@]}"})      # nothing asked for on the command line: the array at the top
fi

[[ "${PER_JOB}" =~ ^[1-9][0-9]*$ ]] || die "--per-job must be a positive integer (got '${PER_JOB}')"
for r in ${RUNS[@]+"${RUNS[@]}"}; do
  [[ "$r" =~ ^[0-9]+$ ]] || die "'$r' is not a run number"
done
if [[ -z "${FROM_LIST}" && ${#RUNS[@]} -eq 0 ]] && ! ${ALL_RUNS}; then
  die "no runs: fill in the runs array at the top of $(basename "$0"), or give run numbers / --all-runs / --runs-file / --from-list"
fi
if ${ALL_RUNS} && [[ ${#RUNS[@]} -gt 0 ]]; then die "--all-runs cannot be combined with run numbers"; fi
if [[ -n "${FROM_LIST}" ]]; then
  [[ -r "${FROM_LIST}" ]] || die "cannot read ${FROM_LIST}"
  ! ${ALL_RUNS} || die "--all-runs does not apply to --from-list"
  ! ${FROM_DIRS} || die "use either --from-list or --from-dirs"
fi

STREAMING=false
if [[ -z "${FROM_LIST}" && "${DST_TYPE}" == DST_STREAMING_EVENT* ]]; then STREAMING=true; fi

# where the file names come from
if [[ -n "${FROM_LIST}" ]]; then SOURCE=list
elif ${STREAMING}; then
  SOURCE=dirs
  ! ${ALL_RUNS} || die "--all-runs uses the FileCatalog, which is not used for raw-hit DSTs: give run numbers"
  ${FROM_DIRS} || note "Raw-hit DSTs: searching the production directories (the FileCatalog is not used for them)."
elif ${FROM_DIRS}; then SOURCE=dirs
  ! ${ALL_RUNS} || die "--all-runs needs the FileCatalog; drop --from-dirs"
else SOURCE=catalogue; fi
if [[ "${SOURCE}" == catalogue ]]; then
  ! ${FULL_PATHS} || die "--full-paths needs --from-dirs (the catalogue lists give file names only)"
  [[ "${BUILD_TAG}" =~ ^[A-Za-z0-9_.-]+$ ]] || die "odd characters in the build tag '${BUILD_TAG}'"
  [[ "${DST_TYPE}"  =~ ^[A-Za-z0-9_.-]+$ ]] || die "odd characters in the DST type '${DST_TYPE}'"
  command -v CreateDstList.pl >/dev/null 2>&1 || die "CreateDstList.pl not found (log in to an sPHENIX node / source the environment), or use --from-dirs"
  if ${ALL_RUNS}; then command -v psql >/dev/null 2>&1 || die "psql not found (log in to an sPHENIX node / source the environment), or give run numbers"; fi
fi
if ${STREAMING} && [[ "${PER_JOB}" != "1" ]]; then
  if ${PER_JOB_SET}; then
    note "Raw-hit DSTs: every job needs all detector streams of one segment, so --per-job is forced to 1 (you asked for ${PER_JOB})."
  fi
  PER_JOB=1
fi

if [[ -z "${NAME}" ]]; then
  if [[ -n "${FROM_LIST}" ]]; then NAME=fromlist; else NAME="${SPECIES}_${BUILD_TAG}_${DST_TYPE}"; fi
fi
OUT_DIR=${OUT_ROOT}/${NAME}
RUNS_DIR=${OUT_DIR}/runs
SUB_DIR=${OUT_DIR}/sublists
JOBS_DIR=${OUT_DIR}/jobs
mkdir -p "${RUNS_DIR}" "${SUB_DIR}" "${JOBS_DIR}"
OUT_DIR_ABS=$(cd "${OUT_DIR}" && pwd)
OUT_ROOT_ABS=$(cd "${OUT_ROOT}" && pwd)
JOBS_ABS=${OUT_DIR_ABS}/jobs

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
run8() { printf '%08d' "$((10#$1))"; }

# directory holding the files of a run for one stream/type
type_dir() {   # $1 = dst type or stream directory name, $2 = run
  local lo hi d
  lo=$(( 10#$2 / 100 * 100 )); hi=$(( lo + 100 ))
  d=${PRODUCTION_DIR}/${SPECIES}/${RUN_TYPE}/${BUILD_TAG}/$1/$(printf 'run_%08d_%08d' "${lo}" "${hi}")
  [[ "${SPECIES}" == "run2pp" ]] && d=${d}/dst
  echo "${d}"
}

# names (one per line) of the files matching a pattern in a directory
list_dir() {   # $1 = dir, $2 = glob
  [[ -d "$1" ]] || return 0
  find "$1" -maxdepth 1 -type f -name "$2" | LC_ALL=C sort
}

emit() {   # filter full paths down to what goes into the lists
  if ${FULL_PATHS}; then cat; else awk -F/ '{print $NF}'; fi
}

# raw-hit streams for this species
streams() {
  local i ebdc
  case "${SPECIES}" in
    run2pp)
      for i in 0 1 2 3 4 5 6 7; do echo "INTT${i}"; done
      for i in 0 1 2 3 4 5; do echo "MVTX${i}"; done
      for i in $(seq 0 23); do echo "TPC$(printf '%02d' "$i")"; done
      echo "TPOT" ;;
    run3pp|run3auau|run3oo)
      for i in 0 1 2 3 4 5 6 7; do echo "intt${i}"; done
      for i in 0 1 2 3 4 5; do echo "mvtx${i}"; done
      for i in $(seq 0 23); do ebdc=$(printf '%02d' "$i"); echo "ebdc${ebdc}_0"; echo "ebdc${ebdc}_1"; done
      echo "ebdc39" ;;
    *) die "do not know the raw-hit streams of ${SPECIES}" ;;
  esac
}

# remember what was written
N_RUNS_DONE=0
DONE_RUNS=()     # runs written by this call: they, and only they, go in the top list
N_JOBS_TOTAL=0

# write the sublists of one run from its full list ($1 = run, $2 = list of names/paths)
make_sublists() {
  local run=$1 list=$2 r8 n
  r8=$(run8 "${run}"); rn=$((10#${run}))
  n=$(wc -l < "${list}" | tr -d ' ')
  find "${SUB_DIR}" -maxdepth 1 -name "run${rn}_*.list" -exec rm -f {} +      # rebuilt from scratch
  awk -v n="${PER_JOB}" -v pre="${SUB_DIR}/run${rn}_" '
    (NR-1)%n==0 { if (f) close(f); f=sprintf("%s%04d.list", pre, c++) }
    { print > f }' "${list}"
  local nsub
  nsub=$(find "${SUB_DIR}" -maxdepth 1 -name "run${rn}_*.list" | wc -l | tr -d ' ')
  note "run ${run}: ${n} DST(s) -> ${nsub} sublist(s) of up to ${PER_JOB}"
  N_RUNS_DONE=$((N_RUNS_DONE + 1)); DONE_RUNS+=("${rn}")
  N_JOBS_TOTAL=$((N_JOBS_TOTAL + nsub))
}

# ---------------------------------------------------------------------------
# Discovery
# ---------------------------------------------------------------------------
discover_streaming() {   # $1 = run  ->  writes one sublist per segment, returns 0 if any
  local run=$1 r8 s dir seg tmp common nseg segdir f nmiss
  r8=$(run8 "${run}"); rn=$((10#${run}))
  tmp=$(mktemp -d)
  : > "${tmp}/common"
  local first=true
  for s in $(streams); do
    dir=$(type_dir "DST_STREAMING_EVENT_${s}" "${run}")
    list_dir "${dir}" "DST_STREAMING_EVENT_${s}_${SPECIES}_${BUILD_TAG}-${r8}-*.root" \
      | sed -E 's/.*-([0-9]+)\.root$/\1/' | LC_ALL=C sort -u > "${tmp}/seg_${s}"
    nseg=$(wc -l < "${tmp}/seg_${s}" | tr -d ' ')
    note "run ${run} ${s}: ${nseg} segment(s)"
    if ${first}; then cp "${tmp}/seg_${s}" "${tmp}/common"; first=false
    else LC_ALL=C comm -12 "${tmp}/common" "${tmp}/seg_${s}" > "${tmp}/common.new"; mv "${tmp}/common.new" "${tmp}/common"; fi
  done
  common=$(wc -l < "${tmp}/common" | tr -d ' ')
  if [[ "${common}" == "0" ]]; then
    rm -rf "${tmp}"; echo "WARNING: run ${run}: no segment is present in every stream, skipping" >&2; return 1
  fi
  # a segment missing from some stream is dropped, not guessed
  cat "${tmp}"/seg_* | LC_ALL=C sort -u > "${tmp}/union"
  for s in $(streams); do
    nmiss=$(LC_ALL=C comm -13 "${tmp}/seg_${s}" "${tmp}/union" | wc -l | tr -d ' ')
    if [[ "${nmiss}" != "0" ]]; then
      echo "WARNING: run ${run}: ${s} is missing ${nmiss} segment(s) that other streams have; those segments are dropped" >&2
    fi
  done
  : > "${RUNS_DIR}/run${rn}.list.tmp"
  find "${SUB_DIR}" -maxdepth 1 -name "run${rn}_seg*.list" -exec rm -f {} +  # rebuilt from scratch
  while read -r seg; do
    segdir=${SUB_DIR}/run${rn}_seg${seg}.list
    : > "${segdir}.tmp"
    for s in $(streams); do
      dir=$(type_dir "DST_STREAMING_EVENT_${s}" "${run}")
      f=DST_STREAMING_EVENT_${s}_${SPECIES}_${BUILD_TAG}-${r8}-${seg}.root
      if ${FULL_PATHS}; then echo "${dir}/${f}" >> "${segdir}.tmp"; else echo "${f}" >> "${segdir}.tmp"; fi
    done
    mv "${segdir}.tmp" "${segdir}"
    echo "run${rn}_seg${seg}" >> "${RUNS_DIR}/run${rn}.list.tmp"
  done < "${tmp}/common"
  mv "${RUNS_DIR}/run${rn}.list.tmp" "${RUNS_DIR}/run${rn}.list"
  note "run ${run}: ${common} segment(s) with all $(streams | wc -l | tr -d ' ') streams -> ${common} sublist(s), one segment each"
  N_RUNS_DONE=$((N_RUNS_DONE + 1)); DONE_RUNS+=("${rn}"); N_JOBS_TOTAL=$((N_JOBS_TOTAL + common))
  rm -rf "${tmp}"
  return 0
}

discover_dir() {   # $1 = run
  local run=$1 r8 dir tmp
  r8=$(run8 "${run}"); rn=$((10#${run}))
  dir=$(type_dir "${DST_TYPE}" "${run}")
  [[ -d "${dir}" ]] || { echo "WARNING: run ${run}: ${dir} does not exist, skipping" >&2; return 1; }
  tmp=${RUNS_DIR}/run${rn}.list.tmp
  list_dir "${dir}" "${DST_TYPE}_${SPECIES}_${BUILD_TAG}-${r8}-*.root" | emit > "${tmp}"
  if [[ ! -s "${tmp}" ]]; then
    rm -f "${tmp}"; echo "WARNING: run ${run}: no ${DST_TYPE} files for ${SPECIES}/${BUILD_TAG} in ${dir}" >&2; return 1
  fi
  mv "${tmp}" "${RUNS_DIR}/run${rn}.list"
  make_sublists "${run}" "${RUNS_DIR}/run${rn}.list"
}

catalogue_runs() {   # runs the FileCatalog has for this tag and type, ascending
  psql FileCatalog -X -A -t -c "select distinct(runnumber) from datasets where tag='${BUILD_TAG}' and dsttype='${DST_TYPE}' order by runnumber" \
    | tr -d '[:blank:]\r' | grep -E '^[0-9]+$' || true
}

discover_catalogue() {   # $1 = run
  local run=$1 r8 tmp f raw
  r8=$(run8 "${run}"); rn=$((10#${run}))
  tmp=$(mktemp -d)
  # CreateDstList.pl writes <dsttype>-<run8>.list into the current directory
  if ! ( cd "${tmp}" && CreateDstList.pl --run "${rn}" --tag "${BUILD_TAG}" "${DST_TYPE}" ) > "${tmp}/log" 2>&1; then
    echo "WARNING: run ${run}: CreateDstList.pl failed:" >&2; sed 's/^/    /' "${tmp}/log" >&2; rm -rf "${tmp}"; return 1
  fi
  f=$(find "${tmp}" -maxdepth 1 -name "*-${r8}.list" | head -n 1)
  if [[ -z "${f}" || ! -s "${f}" ]]; then
    echo "WARNING: run ${run}: the catalogue has no ${DST_TYPE} files for tag ${BUILD_TAG}" >&2; rm -rf "${tmp}"; return 1
  fi
  tr -d '\r' < "${f}" | awk 'NF {print $1}' | awk -F/ '{print $NF}' | LC_ALL=C sort -u > "${tmp}/raw"
  # keep this species only (the names carry it: <type>_<species>_<tag>-<run>-<seg>.root)
  grep -F -- "_${SPECIES}_${BUILD_TAG}-" "${tmp}/raw" > "${tmp}/sel" || true
  if [[ ! -s "${tmp}/sel" ]]; then
    echo "WARNING: run ${run}: catalogue files found, but none for species ${SPECIES} (first one: $(head -n 1 "${tmp}/raw")); check -s" >&2
    rm -rf "${tmp}"; return 1
  fi
  if [[ "$(wc -l < "${tmp}/sel")" != "$(wc -l < "${tmp}/raw")" ]]; then
    echo "WARNING: run ${run}: $(( $(wc -l < "${tmp}/raw") - $(wc -l < "${tmp}/sel") )) catalogue file(s) of other species ignored" >&2
  fi
  mv "${tmp}/sel" "${RUNS_DIR}/run${rn}.list"
  rm -rf "${tmp}"
  make_sublists "${run}" "${RUNS_DIR}/run${rn}.list"
}

discover_from_list() {
  local tmp runs_in r
  tmp=$(mktemp -d)
  # one "run name" pair per line, duplicates dropped
  while IFS= read -r line; do
    line=${line%%#*}; line=${line//[$'\r\t ']/}
    [[ -z "${line}" ]] && continue
    base=${line##*/}
    if [[ "${base}" =~ -([0-9]{8})-[0-9]+\.root$ ]]; then
      echo "$((10#${BASH_REMATCH[1]})) ${line}"
    else
      echo "WARNING: cannot read a run number from '${line}', skipping" >&2
    fi
  done < "${FROM_LIST}" | LC_ALL=C sort -u > "${tmp}/pairs"
  if [[ ${#RUNS[@]} -gt 0 ]]; then
    : > "${tmp}/sel"
    for r in "${RUNS[@]}"; do awk -v r="$((10#${r}))" '$1==r' "${tmp}/pairs" >> "${tmp}/sel"; done
    mv "${tmp}/sel" "${tmp}/pairs"
  fi
  [[ -s "${tmp}/pairs" ]] || { rm -rf "${tmp}"; die "no usable DST names in ${FROM_LIST}"; }
  runs_in=$(awk '{print $1}' "${tmp}/pairs" | LC_ALL=C sort -un)
  for r in ${runs_in}; do
    r8=$(run8 "${r}"); rn=$((10#${r}))
    awk -v r="${r}" '$1==r {print $2}' "${tmp}/pairs" \
      | awk '{n=split($0,a,"/"); print a[n] "\t" $0}' | LC_ALL=C sort | cut -f2- \
      | { if ${FULL_PATHS}; then cat; else awk -F/ '{print $NF}'; fi; } > "${RUNS_DIR}/run${rn}.list.tmp"
    mv "${RUNS_DIR}/run${rn}.list.tmp" "${RUNS_DIR}/run${rn}.list"
    make_sublists "${r}" "${RUNS_DIR}/run${rn}.list"
  done
  rm -rf "${tmp}"
}

# ---------------------------------------------------------------------------
# Go
# ---------------------------------------------------------------------------
note "List set ${OUT_DIR}  (${PER_JOB} DST(s) per job)"
if [[ "${SOURCE}" == catalogue ]] && ${ALL_RUNS}; then
  note "Asking the FileCatalog for the runs with tag ${BUILD_TAG}, type ${DST_TYPE} ..."
  mapfile -t RUNS < <(catalogue_runs)
  [[ ${#RUNS[@]} -gt 0 ]] || die "the FileCatalog lists no runs for tag ${BUILD_TAG}, type ${DST_TYPE}"
  note "  ${#RUNS[@]} run(s): ${RUNS[0]} ... ${RUNS[${#RUNS[@]}-1]}"
fi
case "${SOURCE}" in
  list) discover_from_list ;;
  dirs)
    note "Searching ${PRODUCTION_DIR}/${SPECIES}/${RUN_TYPE}/${BUILD_TAG}/${DST_TYPE}*"
    for run in "${RUNS[@]}"; do
      if ${STREAMING}; then discover_streaming "${run}" || true
      else discover_dir "${run}" || true; fi
    done ;;
  catalogue)
    for run in "${RUNS[@]}"; do discover_catalogue "${run}" || true; done ;;
esac

if [[ "${N_RUNS_DONE}" == "0" ]]; then
  die "nothing was written: no files found (see the warnings above)"
fi

# per-run job lists (jobs/run<N>.txt: the sublists of one run) and the top list (one jobs/run<N>.txt per
# run of THIS call), each written in one step. The top list sits in fileLists/ so there is one place to look.
top=${OUT_ROOT_ABS}/inputDSTlists.txt
: > "${top}.tmp"
nruns=0; njobs=0
for r in $(printf '%s\n' "${DONE_RUNS[@]}" | LC_ALL=C sort -un); do
  find "${OUT_DIR_ABS}/sublists" -maxdepth 1 -name "run${r}_*.list" | LC_ALL=C sort > "${JOBS_ABS}/run${r}.txt.tmp"
  mv "${JOBS_ABS}/run${r}.txt.tmp" "${JOBS_ABS}/run${r}.txt"
  echo "${JOBS_ABS}/run${r}.txt" >> "${top}.tmp"
  nruns=$((nruns + 1)); njobs=$((njobs + $(wc -l < "${JOBS_ABS}/run${r}.txt")))
done
mv "${top}.tmp" "${top}"

note ""
note "Top list: ${top}"
note "  ${nruns} run(s) = ${nruns} condor submission(s), ${njobs} job(s) in total"
if [[ "${top}" == "$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)/fileLists/inputDSTlists.txt" ]]; then
  note "Submit with:  ./submitCondorJobs.sh"
else
  note "Submit with:  ./submitCondorJobs.sh ${top}"
fi
