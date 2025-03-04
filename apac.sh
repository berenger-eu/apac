#!/bin/bash


# Temporary, only to run all of the transformations on a file
# until we have a proper pipeline

# Run the transformations
dirName=$(dirname "$(realpath "$0")")
transformationBin="$dirName/build"

#Order of transformations

unstackName="unstack"
constifyName="constify"
declSplitName="declarationSplitter"
gotoName="gotoRet"
stackHeapName="stackheap"
taskGraphName="taskGraph"
duplicateFunctionsName="duplicateFunctions"
$mainParallelName="mainParallel"


unstackTransfo="$transformationBin/$unstackName"
constifyTransfo="$transformationBin/$constifyName"
declSplitTransfo="$transformationBin/$declSplitName"
gotoTransfo="$transformationBin/$gotoName"
stackHeapTransfo="$transformationBin/$stackHeapName"
taskGraphTransfo="$transformationBin/$taskGraphName"
duplicateFunctions="$transformationBin/$duplicateFunctionsName"
mainParallel="$transformationBin/$mainParallelName"


inputFile=$1
# path/to/example.cpp
realInputFile=$(realpath "$inputFile")

# example.cpp
inputFileName=$(basename "$inputFile")
#example
inputFileNameNoExt="${inputFileName%.*}"
# path/to
inputFolder=$(dirname "$realInputFile")



function run_transformation {
    local transformation=$1
    local transformationName=$2
    local inputFile=$3
    
local    outputFile="$curUsedFolder/$inputFileNameNoExt"_"$transformationName"_"$counter.cpp"

    $transformation $inputFile > $outputFile 2> /dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to run $transformationName"
        exit 1
    fi
    counter=$((counter+1))

    curUsedFile=$outputFile
}



function run_const {
    local inputFile=$1
    $constifyTransfo $inputFile > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Failed to run $constifyName"
        exit 1
    fi

    local folderName=$(basename "$curUsedFile" .cpp)
    local fileName=$(basename "$curUsedFile")
    cp $curUsedFolder/$folderName/$fileName $curUsedFolder/$inputFileNameNoExt"_"$constifyName"_"$counter.cpp
    curUsedFile=$curUsedFolder/$inputFileNameNoExt"_"$constifyName"_"$counter.cpp
        counter=$((counter+1))
}

rm -rf $inputFolder/$inputFileNameNoExt/* 
rmdir  $inputFolder/$inputFileNameNoExt/

mkdir $inputFolder/$inputFileNameNoExt
cp $realInputFile $inputFolder/$inputFileNameNoExt/$inputFileName
curUsedFolder=$inputFolder/$inputFileNameNoExt
curUsedFile=$curUsedFolder/$inputFileName
counter=0


# Run the transformations
run_transformation $duplicateFunctions $duplicateFunctionsName $curUsedFile
#run_const $curUsedFile
#run_transformation $unstackTransfo $unstackName $curUsedFile $curUsedFile
run_transformation $declSplitTransfo $declSplitName $curUsedFile
run_transformation $gotoTransfo $gotoName $curUsedFile
#run_transformation $stackHeapTransfo $stackHeapName $curUsedFile
run_transformation $taskGraphTransfo $taskGraphName $curUsedFile
run_transformation $mainParallel $mainParallelName $curUsedFile

echo "All transformations ran successfully"