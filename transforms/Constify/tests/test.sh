#!/bin/bash
# Compare source and AST snapshots, then check observable behavior and qualifier levels.
set -u
curPath="$(cd "$(dirname "$0")" && pwd)"
repoRoot="$(cd "$curPath/../../.." && pwd)"
buildDir="${APAC_BUILD_DIR:-$repoRoot/build}"
constify="$buildDir/constify"
testsPath="$curPath/autotests"
expectedPath="$curPath/expected"
resultPath="$curPath/results"
countPassed=0
countTotal=0
mkdir -p "$resultPath"

for input in "$testsPath"/*.cpp; do
    [ -f "$input" ] || continue
    ((countTotal+=1))
    name="$(basename "$input" .cpp)"
    folder="$resultPath/$name"
    output="$folder/$name.cpp"
    expected="$expectedPath/$name/$name.cpp"
    mkdir -p "$folder"
    # Every failed command counts as a failure; neither empty ASTs nor text-only
    # differences can accidentally turn a broken transformation into a success.
    if cp "$input" "$output" &&
       "$constify" --in-place "$output" -- -std=c++17 -I"$testsPath" >"$folder/stdout" 2>"$folder/diagnostics" &&
       clang-format-18 -i "$output" &&
       clang-check-18 -ast-print "$expected" -- -std=c++17 -I"$testsPath" >"$folder/expected.ast" 2>"$folder/expected.errors" &&
       clang-check-18 -ast-print "$output" -- -std=c++17 -I"$testsPath" >"$folder/result.ast" 2>"$folder/result.errors" &&
       diff -u "$expected" "$output" >"$folder/source.diff" &&
       diff -u "$folder/expected.ast" "$folder/result.ast" >"$folder/ast.diff"; then
        ((countPassed+=1))
        echo "PASS: Constify $name"
        rm -rf "$folder"
    else
        echo "FAIL: Constify $name (diagnostics in $folder)"
    fi
done

echo "Constify snapshots passed: $countPassed/$countTotal"
semanticStatus=0
python3 "$repoRoot/scripts/constify-regression.py" --build-dir "$buildDir" || semanticStatus=$?
[ "$countTotal" -gt 0 ] && [ "$countPassed" -eq "$countTotal" ] && [ "$semanticStatus" -eq 0 ]
