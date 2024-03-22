#!/bin/bash
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'

countPassed=0
countTotal=0

# Iterate over all .cpp files in the current directory
bash Constify/tests/autotests/test.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}${BOLD}FAILURE: Test suite for Constify transformation${NC}"
else
    ((countPassed++))
    echo -e "${GREEN}${BOLD}SUCCESS: Test suite for Constify transformation${NC}"
fi
((countTotal++))
bash Heapify/tests/test.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}${BOLD}FAILURE: Test suite for Heapify transformation${NC}"
else
    ((countPassed++))
    echo -e "${GREEN}${BOLD}SUCCESS: Test suite for Heapify transformation${NC}"
fi
((countTotal++))
bash GotoTransfo/tests/test.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}${BOLD}FAILURE: Test suite for Goto transformation${NC}"
else
    ((countPassed++))
    echo -e "${GREEN}${BOLD}SUCCESS: Test suite for Goto transformation${NC}"
fi
((countTotal++))
echo -e "${BLUE}${BOLD}Test suite passed : $countPassed/$countTotal ${NC}"
if [ $countPassed != $countTotal ]; then
    exit 1
fi
exit 0
done