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
declarationSplitter="$curPath/../../../build/declarationSplitter"
expectedPath="$curPath/expected"
resultPath="$curPath/results"
testsPath="$curPath/autotests"

mkdir -p $resultPath
# Iterate over all .cpp files in the current directory
for file in $testsPath/*.cpp; do
    # Check if the file exists and is a regular file
    if [ -f "$file" ]; then
        ((countTotal++))
        differenceInText=false
        differenceInAST=false
        fileName=$(basename "$file" /)
        folderName=$(basename "$fileName" .cpp)
        mkdir $resultPath/$folderName
        folderResultPath="$resultPath/$folderName"
        $declarationSplitter $file $arguments > "$folderResultPath/$fileName" 2> /dev/null
                         
        if [ $? -ne 0 ]; then
            echo -e "${RED}${BOLD}Failed to process file : $file"
            differenceInAST=true
            differenceInText=true
        else
            clang-format-18 -i "$folderResultPath/$fileName" 
            for file2 in "$folderResultPath"/*.cpp; do
                if [ -f "$file2" ] && [[ "$file2" != *"astdiff"* ]]; then
                    #echo "Created file : $file2"
                    expectedResult="$expectedPath"/$fileName
                    diff "$file2" "$expectedResult" > /dev/null
                    if [ $? -ne 0 ]; then
                        differenceInText=true
                        echo -e "${RED}Different Text : $fileName${NC}"
                    fi
                    clang-check-18 -ast-print "$expectedResult" > "$curPath/ast_result" 2> /dev/null
                    clang-check-18 -ast-print $file2 > "$curPath/ast_expected" 2> /dev/null
                    diff "$curPath/ast_expected" "$curPath/ast_result" > "$folderResultPath"/astdiff
                    if [ $? -ne 0 ]; then
                        differenceInAST=true
                        echo -e "${RED}Different AST : $fileName${NC}"
                    fi
                fi
            done
            rm "$curPath/ast_expected"
            rm "$curPath/ast_result"
        fi 
        if $differenceInAST; then
            echo -e "${RED}${BOLD}Test failed, AST : $folderName${NC}"
        else 
            ((countPassed++))
            echo -e "${GREEN}${BOLD}Test succeeded : $folderName${NC}"
        fi
        if $differenceInText; then
            echo -e "${RED}Test failed, TEXT : $folderName${NC}"
        else 
            echo -e "${GREEN}Test succeeded, TEXT : $folderName${NC}"
        fi
        if [ $differenceInAST == false ]; then #&& [ $differenceInText == false ]
            rm -rf "$folderResultPath"
        fi
    fi
done
echo -e "${BLUE}${BOLD}Tests passed : $countPassed/$countTotal ${NC}"
if [ $countPassed != $countTotal ]; then
    exit 1
fi
rm -rf "$resultPath/"
exit 0
done