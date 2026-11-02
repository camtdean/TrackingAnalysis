#!/bin/bash
#
# runHFreco.sh  --  run Fun4All_HF.C on one list of DSTs (one condor job).
#
#   runHFreco.sh <inputList | file.root> [nEvents=0] [outDir=<data>/root/] [nSkip=0]
#
#   inputList  a sublist made by getFiles.sh (one DST per line)
#   nEvents    events to process in total over the whole list, 0 = all of them
#   outDir     top directory for the nTuples (one sub-directory per channel)
#   nSkip      events to skip at the start of the list
#
# Inputs are read where they are (Fun4All resolves bare names through the file catalogue):
# nothing is copied to /tmp or to the worker's scratch disk.

# --- environment (same as the twoTrack scripts) -----------------------------
source /opt/sphenix/core/bin/sphenix_setup.sh -n new

export USER="$(id -u -n)"
export LOGNAME=${USER}
export HOME=/sphenix/u/${LOGNAME}

# Use the local install (your KFParticle changes live there).
# Turn off with:  USE_LOCAL_INSTALL=false  in the environment.
if [[ "${USE_LOCAL_INSTALL:-true}" == true ]]; then
  export SPHENIX=${HOME}/sPHENIX
  export MYINSTALL=$SPHENIX/install
  if [[ -d "${MYINSTALL}" ]]; then
    export LD_LIBRARY_PATH=$MYINSTALL/lib:$LD_LIBRARY_PATH
    export ROOT_INCLUDE_PATH=$MYINSTALL/include:$ROOT_INCLUDE_PATH
    export PATH="$HOME/.local/bin:$PATH"
    source /opt/sphenix/core/bin/setup_local.sh $MYINSTALL
  else
    echo "WARNING: USE_LOCAL_INSTALL=true but ${MYINSTALL} does not exist; using the release libraries"
  fi
fi

# From here on a typo should stop the job, not run with an empty value.
set -u

# --- where the macro lives: not the current directory ------------------------
# condor passes MACRO_DIR; otherwise use the directory of this script.
if [[ -n "${MACRO_DIR:-}" ]]; then
  macroDir=${MACRO_DIR}
else
  macroDir=$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)
fi
[[ -f "${macroDir}/Fun4All_HF.C" ]] || { echo "ERROR: ${macroDir}/Fun4All_HF.C not found"; exit 2; }
dataDir=$(cd "${macroDir}/.." && pwd)

# --- arguments ---------------------------------------------------------------
if [[ $# -lt 1 ]]; then
  echo "usage: runHFreco.sh <inputList | file.root> [nEvents=0] [outDir] [nSkip=0]"
  exit 2
fi
inDst=$1
nEvents=${2:-0}
outDir=${3:-${dataDir}/root}
nSkip=${4:-0}

[[ "${nEvents}" =~ ^[0-9]+$ ]] || { echo "ERROR: nEvents '${nEvents}' is not a number"; exit 2; }
[[ "${nSkip}"   =~ ^[0-9]+$ ]] || { echo "ERROR: nSkip '${nSkip}' is not a number"; exit 2; }

# relative paths are relative to where the job was started, before we change directory
[[ "${inDst}" == /* ]] || inDst=${PWD}/${inDst}
if [[ "${inDst}" == *.list || "${inDst}" == *.txt ]]; then
  [[ -s "${inDst}" ]] || { echo "ERROR: input list ${inDst} is missing or empty"; exit 2; }
  nDst=$(grep -c . "${inDst}")
else
  nDst=1
fi

[[ "${outDir}" == /* ]] || outDir=${PWD}/${outDir}
mkdir -p "${outDir}" || { echo "ERROR: cannot create ${outDir}"; exit 2; }
outDir=$(cd "${outDir}" && pwd)/      # the macro appends channel names, so keep the trailing slash

echo "runHFreco.sh on $(hostname) at $(date)"
echo "  input   : ${inDst} (${nDst} DST file(s))"
echo "  events  : ${nEvents} (0 = all)   skip: ${nSkip}"
echo "  output  : ${outDir}"
echo "  macro   : ${macroDir}/Fun4All_HF.C"

# --- run ---------------------------------------------------------------------
cd "${macroDir}" || exit 2

rootLog=$(mktemp "${_CONDOR_SCRATCH_DIR:-${TMPDIR:-/tmp}}/runHFreco.XXXXXX.log")
trap 'rm -f "${rootLog}"' EXIT

set -o pipefail
root.exe -q -b "Fun4All_HF.C(${nEvents},\"${inDst}\",\"${outDir}\",${nSkip})" 2>&1 | tee "${rootLog}"
rc=${PIPESTATUS[0]}
set +o pipefail

# The macro prints "Finished" as its last line. root.exe can return 0 after a failed load, so look for it.
if [[ ${rc} -eq 0 ]] && ! grep -q '^Finished' "${rootLog}"; then
  echo "ERROR: root exited 0 but Fun4All_HF.C never printed 'Finished' (macro failed to load or crashed)"
  rc=1
fi

echo "runHFreco.sh exit code ${rc} at $(date)"
exit ${rc}
