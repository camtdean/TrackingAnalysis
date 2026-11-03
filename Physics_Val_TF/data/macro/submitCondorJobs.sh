#!/bin/bash
# Forwarder: the submit script lives one directory up (../submitCondorJobs.sh).
exec "$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)/../submitCondorJobs.sh" "$@"
