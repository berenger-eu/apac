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
constify=$(command -v constify 2>/dev/null || echo "$curPath/../../../build/constify")
expectedPath="$curPath/expected"
testsPath="$curPath/autotests"
resultPath=$testsPath



# Iterate over all .cpp files in the current directory
for file in $testsPath/*.cpp; do
    # Check if the file exists and is a regular file
    if [ -f "$file" ]; then
        ((countTotal++))
        differenceInText=false
        differenceInAST=false
        folderName=$(basename "$file" .cpp)
        $constify $file > /dev/null 2> /dev/null
        if [ $? -ne 0 ]; then
            echo -e "${RED}${BOLD}Failed to process file : $file"
            differenceInAST=true
            differenceInText=true
        else      
            for file2 in "$resultPath/$folderName"/*; do
                if [ -f "$file2" ] && [[ "$file2" != *"astdiff"* ]]; then
                    fileName=$(basename "$file2" /)
                    clang-format-18 -i "$file2"
                    diff "$file2" "$expectedPath/$folderName"/"$fileName" > /dev/null
                    if [ $? -ne 0 ]; then
                        differenceInText=true
                        echo -e "${RED}Different Text : $fileName${NC}"
                    fi
                    clang-check-18 -ast-print "$expectedPath/$folderName/$fileName" > $curPath/ast_expected 2> /dev/null
                    clang-check-18 -ast-print $file2 > $curPath/ast_result 2> /dev/null
                    diff $curPath/ast_expected $curPath/ast_result > "$resultPath/$folderName/astdiff"
                    if [ $? -ne 0 ]; then
                        differenceInAST=true
                        echo -e "${RED}Different AST : $fileName${NC}"
                    fi
                    
                fi
            done
            rm $curPath/ast_expected
            rm $curPath/ast_result
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
        
        if [ $differenceInAST == false ] && [ $differenceInText == false ]; then
            rm -rf "$resultPath/$folderName/"
        fi
        
    fi
done
echo -e "${BLUE}${BOLD}Tests passed : $countPassed/$countTotal ${NC}"
if [ $countPassed != $countTotal ]; then
    exit 1
fi
exit 0