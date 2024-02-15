#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
BOLD='\033[1m'
difference=false
# Iterate over all .cpp files in the current directory
for file in *.cpp; do
    # Check if the file exists and is a regular file
    if [ -f "$file" ]; then
        echo "Processing file: $file"
        ../../build/apac $file
        foldername=$(basename "$file" .cpp)
        difference=false
        for file2 in "$foldername"/*; do
            if [ -f "$file2" ]; then
                #echo "Created file : $file2"
                filename=$(basename "$file2" /)
                diff "$file2" "$foldername"_expected/"$filename" > /dev/null
                if [ $? -ne 0 ]; then
                    difference=true
                    echo -e "${RED}Different : $filename${NC}"
                fi
            fi
        done 
        if $difference; then
                echo -e "${RED}${BOLD}Test failed : $foldername${NC}"
            else 
                echo -e "${GREEN}${BOLD}Test succeeded : $foldername${NC}"
        fi
        rm -rf "$foldername"/
    fi
done