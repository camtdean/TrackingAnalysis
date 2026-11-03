#!/bin/bash

cd "$(dirname "$(readlink -f "$0")")"       # work from this directory, wherever it is called from

submissionFile=myCondor.job
runLists=fileLists/inputDSTlists.txt        # one line per run, written by getFiles.sh

[ -r ${runLists} ] || { echo "${runLists} not found, run ./getFiles.sh first"; exit 1; }
mkdir -p log root

for runList in $(cat ${runLists})
do
  echo "Submitting ${runList}"
  sed -i -e "s|^Queue.*|Queue dstList from ${runList}|" ${submissionFile}
  condor_submit ${submissionFile}
done
