#!/bin/bash
# Always use this checkout's tools and resolve paths from the repository root.
repoRoot="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repoRoot" || exit 1
buildDir="${APAC_BUILD_DIR:-$repoRoot/build}"
export PATH="$buildDir:$PATH"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'

countPassed=0
countTotal=0
transformationsPath=transforms

# Iterate over all .cpp files in the current directory
for folder in $transformationsPath/*; do
    if [ -d "$folder" ]; then
        folderName=$(basename "$folder" $transformationsPath)
        bash $folder/tests/test.sh
        if [ $? -ne 0 ]; then
            echo -e "${RED}${BOLD}FAILURE: Test suite for $folderName transformation${NC}"
        else
            ((countPassed++))
            echo -e "${GREEN}${BOLD}SUCCESS: Test suite for $folderName transformation${NC}"
        fi
        ((countTotal++))
    fi
done
((countTotal++))
if python3 "$repoRoot/scripts/regression.py" --build-dir "$buildDir"; then
    ((countPassed++))
fi

for regression in taskgraph-regression.py depth-regression.py call-loop-regression.py array-phase-regression.py validation-regression.py benchmark-regression.py snapshot-runner-regression.py; do
    ((countTotal++))
    if python3 "$repoRoot/scripts/$regression"; then
        ((countPassed++))
    fi
done

echo -e "${BLUE}${BOLD}Test suite passed : $countPassed/$countTotal ${NC}"
if [ $countPassed != $countTotal ]; then
    exit 1
fi
exit 0
