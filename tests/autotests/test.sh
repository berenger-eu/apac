#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'
difference=false
countPassed=0
countTotal=0
# Iterate over all .cpp files in the current directory
for file in *.cpp; do
    # Check if the file exists and is a regular file
    if [ -f "$file" ]; then
        echo "Processing file: $file"
        ((countTotal++))
        differenceInText=false
        differenceInAST=false
        foldername=$(basename "$file" .cpp)
        ../../build/apac $file > /dev/null 2> /dev/null
        if [ $? -ne 0 ]; then
            echo -e "${RED}${BOLD}Failed to process file : $file"
            differenceInAST=true
            differenceInText=true
        else
            for file2 in "$foldername"/*; do
                if [ -f "$file2" ]; then
                    #echo "Created file : $file2"
                    filename=$(basename "$file2" /)
                    diff "$file2" "$foldername"_expected/"$filename" > /dev/null
                    if [ $? -ne 0 ]; then
                        differenceInText=true
                        echo -e "${RED}Different Text : $filename${NC}"
                    fi
                    clang-check -ast-print "$foldername"_expected/"$filename" > ./ast_result 2> /dev/null
                    clang-check -ast-print $file2 > ./ast_expected 2> /dev/null
                    diff ast_expected ast_result > /dev/null
                    if [ $? -ne 0 ]; then
                        differenceInAST=true
                        echo -e "${RED}Different AST : $filename${NC}"
                    fi
                    
                fi
            done
            rm ./ast_expected
            rm ./ast_result
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
        if [ $differenceInAST == false ] && [ $differenceInText == false ]; then
            rm -rf "$foldername"/
        fi
    fi
done
echo -e "${BLUE}${BOLD}Tests passed : $countPassed/$countTotal ${NC}"
echo -e "${BLUE}${BOLD}Tests passed : $countPassed/$countTotal ${NC}"