# APAC classic benchmarks

Eight original implementations are provided in `kernels/`. No benchmark source
is copied from BOTS. Every measured execution must pass an untimed result check.
In a prepared bundle the originals are in `<name>/kernel.cpp` and the generated
sources in `<name>/APACkernel.cpp`; paths such as `kernels/` and `scripts/` below
refer to the development checkout.

The previous `readOnlyParam`, `dotProduct` and `callChain` examples are historical
inputs and are not run by this suite.

| Name | Workload | Size | Meaning of `grain` | Oracle |
| --- | --- | --- | --- | --- |
| `fibonacci` | Naive recursive Fibonacci | index, 0–46 | switch to the sequential recursive leaf at `n <= grain` | iterative Fibonacci |
| `quicksort` | Three-way quicksort, middle pivot | number of integers | maximum insertion-sort leaf length | full comparison with `std::sort` |
| `mergesort` | Recursive merge sort with a separate scratch buffer | number of integers | maximum insertion-sort leaf length | full comparison with `std::sort` |
| `nqueens` | Bitmask search with recursive enumeration of branches | board dimension, 0–15 | search depth before switching to a sequential subtree | known counts of all solutions |
| `dotproduct` | Recursive reduction of two initialized vectors | vector length | maximum sequential leaf length | long-double linear reduction |
| `matmul` | Recursive rectangular tiles, sequential `i,k,j` leaves | square matrix dimension, up to 4096 | maximum leaf rows and columns | full `i,j,k` long-double reference |
| `nbody` | Direct all-pairs 3D gravity with softening and semi-implicit Euler | particles, 1–32768 | targets per leaf | separate long-double AoS simulation, reversed interaction order |
| `jacobi` | Five-point 2D stencil, fixed boundaries and two buffers | square grid dimension, 2–8192 | output rows per leaf | long-double stencil, different loop order |

N-body uses equal masses `1/N`, softening squared `0.001`, and timestep `0.001`.
`steps` controls N-body and Jacobi timesteps; other kernels ignore it. All arrays
are initialized deterministically from a seed. Quicksort runs four distributions
by default: random, sorted, reverse, and many duplicates. Buffer sentinels detect
writes past the logical allocation. Floating values use the tolerance
`abs(actual-reference) <= 2e-9 * (1+abs(reference))` and must be finite.

## Prepare on the development machine

From the repository root, after building APAC:

```sh
python3 benchmarks/benchmark.py prepare \
  --build-dir build --output-dir /tmp/apac-benchmarks
```

The output directory must be empty. Preparation runs the APAC pipeline on each
kernel, preserves the original, and retains the generated source, required
headers, DOT graphs and transformation log. The manifest records hashes of
sources and APAC, plus the Git revision/dirty status. A failed preparation is
marked incomplete and cannot be used for measurements.

Transfer the entire directory to the compute machine. It contains `run.py` and
all sources needed there; **APAC and LLVM are not required on the compute node**.
Compilation with the target machine's GCC/OpenMP happens during `run`.

## Run on the compute machine

Inside the transferred directory:

```sh
# First check that compilation, OpenMP and validation work.
python3 run.py run --output-dir . --profile smoke \
  --threads 1 2 4 --repeat 1 --warmups 0

# Then measure on an allocated compute node; choose its thread counts.
python3 run.py run --output-dir . --profile standard \
  --threads 1 2 4 8 16 --repeat 7 --warmups 1
```

Python 3 and a GCC installation with C++17/OpenMP support are needed. `small` is
the default profile, `smoke` is only for correctness, and `standard` supplies
larger initial workloads. A short-measurement marker is emitted when a median is
below 10 ms; increase sizes for the actual machine. Profiles are starting points,
not promises of suitable durations or speedups.

A single case can be adjusted independently:

```sh
python3 run.py run --output-dir . --cases matmul \
  --size 1024 --grain 32 --threads 1 8 16 --repeat 7
python3 run.py run --output-dir . --cases nbody \
  --size 4096 --grain 64 --steps 16 --threads 1 8 16 --repeat 7
python3 run.py run --output-dir . --cases quicksort \
  --size 4000000 --grain 32 --pattern duplicates --seed 42
```

`--size`, `--grain` and `--steps` require a single selected case. `--cases` can
otherwise select several names. `--compiler` selects GCC; `--cxxflags` defaults
to `-O2` and is split into arguments without invoking a shell. Use the same flags
for both versions, as the runner does. Avoid LTO or fast-math for the initial
study. The runner always includes APAC with one thread, even if it is omitted
from `--threads`. `--timeout` bounds each compilation/transformation/execution.

The repository also supports a combined local invocation:

```sh
python3 benchmarks/benchmark.py all --output-dir /tmp/apac-local-bench \
  --profile small --threads 1 2 4 --repeat 5
```

## What is measured

Each sample starts a fresh process and reconstructs the same input, so a sorting
warmup cannot accidentally turn later random-input measurements into sorted ones.
Timing covers the kernel and completion of its tasks. Initialization, allocation,
verification, reporting, and OpenMP team creation are outside the timed interval.
The sequential executable uses the original algorithm and the same source leaf
cutoffs. APAC is measured with one thread and the selected larger thread counts.

The timing harness is not transformed. APAC transforms the kernel through its
pipeline; its MainParallel pass has no `main` to rewrite. The harness explicitly
creates the OpenMP team, enters one task-generating thread, and waits for all
kernel tasks. Kernel and harness are separate translation units with an opaque
entry call (GCC `noipa`) to keep computation inside the measured interval.

Sequential loops in leaves named `*_apacSeq` are intentionally kept sequential.
The recursive drivers are transformed. This gives both versions the same
algorithmic leaf work and a controllable source granularity. APAC's existing
runtime depth cutoff still applies; changing the thread count may change where
its sequential fallback starts. These are measurements of the current compiler,
not an optimized hand-written OpenMP implementation.

Buffer arguments use the documented APAC partition convention. For example,
`&positions[0]` can represent the entire shared read-only position buffer while
output starts represent disjoint target groups. Shared representatives may have
multiple readers. All conflicting accesses through distinct representatives
respect disjoint regions. No dependency is bypassed through a hidden global.

## Results and reproducibility

Each invocation creates a fresh `results/<UTC timestamp>/` directory containing:

- `samples.csv`: every measured sample, parameters, verification status, kernel
  seconds, complete process wall time (including untimed work), checksum, actual
  team size and maximum numerical error;
- `report.json`: the manifest, platform/CPU/affinity information, compiler and
  flags, linked libraries, OpenMP/Slurm settings, raw samples and summaries;
- compiled binaries and wrapper sources, plus compilation/execution logs.

Summaries include median, standard deviation, minimum/maximum, speedup relative
to the original sequential version, and scaling relative to APAC with one thread.
APAC-one-thread time divided by sequential time gives the transformation overhead.
Configuration order rotates between repetitions to reduce a fixed execution-order
bias. Warmups are validated and excluded from statistics.

`OMP_DYNAMIC=FALSE` is enforced. Existing `OMP_PROC_BIND` and `OMP_PLACES` settings
are respected; otherwise they default to `close` and `cores`. Other allocation
constraints are respected, including `OMP_THREAD_LIMIT`. A runtime team smaller
than requested fails the run instead of being mislabeled. Use only thread counts
covered by the compute allocation. Initialization/first touch is serial in both
versions; NUMA placement and machine load still matter when interpreting results.

Oracle failures, sanitizer failures, timeouts, changed sources or incorrect team
sizes stop the run. A partial JSON report is retained with `complete: false`;
only complete reports should be used for the study. Do not edit a prepared source
to tune parameters: use the CLI options, or prepare a fresh bundle after changing
the code.

## Correctness regression

```sh
python3 scripts/benchmark-regression.py
```

This is included in `scripts/run-tests.sh`. It prepares all kernels, copies the
bundle, compiles/runs the transferred version without APAC using UBSan, exercises
empty/singleton inputs, odd splits and both buffer parities, and checks that a
corrupted result and a changed/incomplete bundle are rejected. It has no timing
or speedup assertion.
