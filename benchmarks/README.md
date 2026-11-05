# APAC classic benchmarks

Six benchmarks run by default: Fibonacci, quicksort, mergesort, GEMM, N-body and
Jacobi. N-Queens remains available explicitly as a sequential-search control.
Every measured execution must pass an untimed independent result check.

Each `kernels/*.cpp` is an ordinary sequential implementation with descriptive
parameters and its complete computational helpers. It contains neither OpenMP
pragmas, APAC-specific names, nor timing-harness arguments. The runner generates
the opaque timing adapter and records the `--ignore` list selecting sequential
computational leaves in the manifest. Those choices are outside the algorithm.

The modified Marek/BOTS SparseLU and Moldyn sources and old generated OpenMP
versions are preserved under `reference/apac-marek/`, with their original license,
provenance and review findings. They are reference/porting inputs, not measured
kernels of the current suite. See [the review](reference/apac-marek/README.md).

In a prepared bundle the originals are in `<name>/kernel.cpp` and the generated
sources in `<name>/APACkernel.cpp`; paths such as `kernels/`, `reference/` and `scripts/` below
refer to the development checkout.

The previous `readOnlyParam`, `dotProduct` and `callChain` examples are historical
inputs and are not run by this suite.

| Name | Workload | Size | Meaning of `grain` | Oracle |
| --- | --- | --- | --- | --- |
| `fibonacci` | Naive recursive Fibonacci | index, 0–46 | unused; APAC runtime depth controls tasking | iterative Fibonacci |
| `quicksort` | Three-way quicksort, middle pivot | number of integers | maximum insertion-sort leaf length | full comparison with `std::sort` |
| `mergesort` | Recursive merge sort with a separate scratch buffer | number of integers | maximum insertion-sort leaf length | full comparison with `std::sort` |
| `nqueens` | Natural backtracking with a loop over candidate columns (explicit control case) | board dimension, 0–15 | unused | known counts of all solutions |
| `matmul` | Three nested tile loops, each call accumulates one `(i,j,k)` tile | square matrix dimension, up to 4096 | tile size in all three dimensions | full `i,j,k` long-double reference |
| `nbody` | Direct all-pairs 3D gravity with velocity-Verlet integration | particles, 1–32768 | unused; one acceleration call per body | long-double AoS Verlet simulation, momentum and center-of-mass invariants |
| `jacobi` | Five-point 2D stencil, fixed boundaries and two buffers | square grid dimension, 2–8192 | output rows per block | long-double stencil, different loop order |

N-body uses equal masses `1/N`, softening squared `0.001`, and timestep `0.001`.
The N-body input computes accelerations, advances positions and half-step
velocities, recomputes accelerations and completes the velocity update. Positions
and velocities remain in their own arrays. No artificial subdivision of the
particle vector is introduced to create parallelism.

N-Queens now uses its natural candidate loop instead of recursive enumeration
invented to spawn tasks. APAC currently cannot parallelize that scalar recursive
reduction; it is a single-search-task control, excluded from default measurements.
Use `--cases nqueens` explicitly to include it. Dot product was removed because
its former subdivision was imposed solely to manufacture independent tasks.

`steps` controls N-body and Jacobi timesteps; other kernels ignore it. All arrays
are initialized deterministically from a seed. Quicksort runs four distributions
by default: random, sorted, reverse, and many duplicates. Buffer sentinels detect
writes past the logical allocation. Floating values use the tolerance
`abs(actual-reference) <= 2e-9 * (1+abs(reference))` and must be finite.

## Prepare on the development machine

From the repository root, after building APAC:

```sh
python3 benchmarks/benchmark.py prepare \
  --build-dir build --output-dir /tmp/apac-benchmarks \
  --archive /tmp/apac-benchmarks.tar.gz
```

The output directory must be empty. Preparation runs the APAC pipeline on each
kernel, preserves the original, and retains the generated source, required
headers, DOT graphs and transformation log. The manifest records hashes of
sources and APAC, plus the Git revision/dirty status. A failed preparation is
marked incomplete and cannot be used for measurements.

The optional `--archive` creates a gzip-compressed tar archive containing the
bundle directory, including both sequential and generated sources. The archive
must be outside the output directory and must not already exist. It is created
only after all selected kernels have been prepared successfully. Neither
preparation nor archiving runs the performance measurements.

Transfer the archive to the compute machine and extract it:

```sh
tar -xzf apac-benchmarks.tar.gz
cd apac-benchmarks
```

Alternatively, transfer the entire prepared directory. It contains `run.py` and
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
  --size 4096 --steps 16 --threads 1 8 16 --repeat 7
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

The sequential inputs contain ordinary function calls. GEMM enumerates `(i,j,k)`
tile updates: updates of one output tile have the same representative and must
execute in k order; different output tiles can execute concurrently. N-body
computes each body's acceleration and performs drift/kick passes on particle
vectors. Jacobi updates row blocks with a completion between timesteps.
Fibonacci, quicksort and mergesort retain their naturally recursive algorithms.

The runner selects computational leaves with APAC's `--ignore` option, preserving
the same leaf implementation in both executables. The adapter is generated after
transformation, so it consumes no artificial recursive task-depth level.
APAC's runtime depth cutoff still applies to transformed function calls.
Its default is `1 + ceil(log2(omp_get_max_threads()))`, computed with integer
arithmetic. It increases monotonically, including at non-power-of-two thread
counts (18 gives 6; 36 and 48 give 7). Task-local guards restore the executing
thread's depth when a task completes.

For a fixed-depth experiment, set the environment before starting the runner:

```sh
APAC_DEPTH_MAX=8 python3 run.py run --output-dir . --profile standard \
  --threads 1 2 4 8 16 18 24 32 36 --repeat 7
```

`APAC_DEPTH_MAX=0` forces the sequential fallback; positive values override the
automatic depth. Invalid values fail explicitly. The runner records `APAC_*`
settings in `report.json`, alongside the OpenMP and Slurm environment.
These are measurements of the current compiler,
not an optimized hand-written OpenMP implementation.

The first supported call-loop subset accepts integer induction/control locals,
void direct calls with scalar arithmetic values and explicit `&buffer[index]` partition
arguments, and nested loops/blocks. Indices, pointer values and scalar arguments
are captured at task creation. There is one join at the end of the entire nest,
not a wait after every iteration. Loop boundaries also wait for preceding tasks.
Unsupported control flow, references, rebindings, mixed array modes and reads of
task outputs in loop control retain the synchronous fallback. This is a current
compiler limitation, not a recommendation to replace loops with recursion.

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

This is included in `scripts/run-tests.sh`. It prepares all seven kernels, extracts the
archive, compiles/runs the transferred version without APAC using UBSan, exercises
empty/singleton inputs, odd splits and both buffer parities, and checks that a
corrupted result and a changed/incomplete bundle are rejected. It has no timing
or speedup assertion.
