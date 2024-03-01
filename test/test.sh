#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'
difference=false
countPassed=0
countTotal=0

curPath="$(realpath $(dirname "$0"))"
stackHeap="$curPath/../build/stackheap"
expectedPath="$curPath/expected"
resultPath="$curPath/results"
testsPath="$curPath/autotests"

rm -rf "$resultPath"/*

# Iterate over all .cpp files in the current directory
for file in $testsPath/*.cpp; do
    # Check if the file exists and is a regular file
    if [ -f "$file" ]; then
        ((countTotal++))
        echo "Processing $file"
        differenceInText=false
        differenceInAST=false
        fileName=$(basename "$file" /)
        folderName=$(basename "$fileName" .cpp)
        mkdir $resultPath/$folderName
        folderResultPath="$resultPath/$folderName"
        $stackHeap $file $arguments > "$folderResultPath/$fileName" 2> /dev/null
        if [ $? -ne 0 ]; then
            echo -e "${RED}${BOLD}Failed to process file : $file"
            differenceInAST=true
            differenceInText=true
        else
            for file2 in "$folderResultPath"/*.cpp; do
                if [ -f "$file2" ] && [[ "$file2" != *"astdiff"* ]]; then
                    #echo "Created file : $file2"
                    expectedResult="$expectedPath"/$fileName
                    diff "$file2" "$expectedResult" > /dev/null
                    if [ $? -ne 0 ]; then
                        differenceInText=true
                        echo -e "${RED}Different Text : $filename${NC}"
                    fi
                    clang-check -ast-print "$expectedResult" > "$curPath/ast_result" 2> /dev/null
                    clang-check -ast-print $file2 > "$curPath/ast_expected" 2> /dev/null
                    diff ast_expected ast_result > "$folderResultPath"/astdiff
                    if [ $? -ne 0 ]; then
                        differenceInAST=true
                        echo -e "${RED}Different AST : $filename${NC}"
                    fi
                fi
            done
            rm "$curPath/ast_expected"
            rm "$curPath/ast_result"
        fi 
        if $differenceInAST; then
            echo -e "${RED}${BOLD}Test failed, AST : $foldername${NC}"
        else 
            ((countPassed++))
            echo -e "${GREEN}${BOLD}Test succeeded : $foldername${NC}"
        fi
        if $differenceInText; then
            echo -e "${RED}Test failed, TEXT : $foldername${NC}"
        else 
            echo -e "${GREEN}Test succeeded, TEXT : $foldername${NC}"
        fi
        if [ $differenceInAST == false ]; then #&& [ $differenceInText == false ]
            rm -rf "$folderResultPath"
        fi
    fi
done
echo -e "${BLUE}${BOLD}Tests passed : $countPassed/$countTotal ${NC}"
if [ $countPassed != $countTotal ]; then
    exit 0
fi
rm -rf "$resultsPath/*"
exit 0
done