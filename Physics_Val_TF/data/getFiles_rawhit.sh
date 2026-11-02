#!/bin/bash
#
# getFiles_rawhit.sh  --  raw-hit (streaming) DSTs.  Same as getFiles.sh with the raw-hit defaults.
#
#   ./getFiles_rawhit.sh 79516
#   ./getFiles_rawhit.sh -s run2pp -b new_nocdbtag_v001 53877
#
# Every detector stream of a segment must be read by the same job, so each sublist is one
# segment (63 streams for run3, one line per stream) whatever --per-job says.
# Any option of getFiles.sh can be given and overrides these defaults.

exec "$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)/getFiles.sh" \
  -t DST_STREAMING_EVENT -b new_nocdbtag_v001 "$@"
