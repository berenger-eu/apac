# APAC

Source-to-source compiler for automatic parallelization of C++ code using OpenMP tasks.

## Build

### Using Docker

```bash
docker build -t apac .
docker run -v /path/to/project:/workspace -it apac /bin/bash
```

### Local build

Requirements: CMake 3.0+, LLVM 18, Clang 18

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Enable Clang-Tidy analysis:
```bash
cmake -DENABLE_CLANG_TIDY=ON ..
```

## Usage

### Complete transformation pipeline

```bash
./apac file.cpp [file2.cpp ...]
```

Output files are prefixed with `APAC`.

### Individual transformations

```bash
./transform_name file.cpp [options]
```

Options:
- `-main <name>`: Specify main function name (default: `main`)
- `-functions <f1,f2,...>`: Transform only specified functions
- `-ignore <f1,f2,...>`: Ignore specified functions

Example:
```bash
./taskGraph input.cpp -main custom_main -functions foo,bar -ignore helper
```

## Available transformations

- `apac`: Complete transformation pipeline
- `taskGraph`: Generate OpenMP task dependency graphs
- `declarationSplitter`: Split multiple variable declarations
- `duplicateFunctions`: Create sequential versions of functions
- `conditionUnstack`: Transform conditional statements
- `multipleDeclSplitter`: Split declaration lists
- `constify`: Constant propagation
- `gotoRet`: Transform goto and return statements
- `unstack`: Move stack variables to heap
- `stackheap`: Memory management transformation
- `mainParallel`: Parallelize main function
- `apacDepth`: Add recursion depth control

## Testing

```bash
cd build
make test
```
