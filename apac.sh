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

unstackTransfo="$transformationBin/$unstackName"
constifyTransfo="$transformationBin/$constifyName"
declSplitTransfo="$transformationBin/$declSplitName"
gotoTransfo="$transformationBin/$gotoName"
stackHeapTransfo="$transformationBin/$stackHeapName"
taskGraphTransfo="$transformationBin/$taskGraphName"



inputFile=$1
# path/to/example.cpp
realInputFile=$(realpath "$inputFile")

# example.cpp
inputFileName=$(basename "$inputFile")
#example
inputFileNameNoExt="${inputFileName%.*}"
# path/to
inputFolder=$(dirname "$realInputFile")

curUsedFolder=$inputFolder
curUsedFile=$inputFileName

rm $inputFolder/$inputFileNameNoExt/* 
rmdir $inputFolder/$inputFileNameNoExt/



# Run the transformations
counter=0

$constifyTransfo "$inputFolder/$inputFileName" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Failed to run $constifyName"
    exit 1
fi

counter=$((counter+1))
curUsedFolder=$inputFolder/$inputFileNameNoExt
curUsedFile=$curUsedFolder/$inputFileName
outputFile="$curUsedFolder/$inputFileNameNoExt"_"$unstackName"_"$counter.cpp"

$unstackTransfo $curUsedFile > $outputFile 2> /dev/null
if [ $? -ne 0 ]; then
    echo "Failed to run $unstackName"
    exit 1
fi
counter=$((counter+1))



curUsedFile=$outputFile
outputFile="$curUsedFolder/$inputFileNameNoExt"_"$gotoName"_"$counter.cpp"

$gotoTransfo $curUsedFile > $outputFile    2> /dev/null
if [ $? -ne 0 ]; then
    echo "Failed to run $gotoName"
    exit 1
fi
counter=$((counter+1))

curUsedFile=$outputFile
outputFile="$curUsedFolder/$inputFileNameNoExt"_"$declSplitName"_"$counter.cpp"

$declSplitTransfo $curUsedFile > $outputFile 2> /dev/null
if [ $? -ne 0 ]; then
    echo "Failed to run $declSplitName"
    exit 1
fi
counter=$((counter+1))

curUsedFile=$outputFile
outputFile="$curUsedFolder/$inputFileNameNoExt"_"$stackHeapName"_"$counter.cpp"
$stackHeapTransfo $curUsedFile > $outputFile 2> /dev/null
if [ $? -ne 0 ]; then
    echo "Failed to run $stackHeapName"
    exit 1
fi
counter=$((counter+1))






curUsedFile=$outputFile
outputFile="$curUsedFolder/$inputFileNameNoExt"_"$taskGraphName"_"$counter.cpp"

$taskGraphTransfo $curUsedFile > $outputFile 2> /dev/null
if [ $? -ne 0 ]; then
    echo "Failed to run $taskGraphName"
    exit 1
fi
counter=$((counter+1))


