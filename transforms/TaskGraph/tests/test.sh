#!/bin/bash
set -eu
repoRoot="$(cd "$(dirname "$0")/../../.." && pwd)"
exec python3 "$repoRoot/scripts/snapshot-tests.py" TaskGraph "$@"
