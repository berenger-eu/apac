# Array dependencies and phases

TaskGraph recognizes three access modes for one-dimensional buffers of arithmetic
or enumeration elements:

| Expression used as a call argument | Mode | Dependency representative |
| --- | --- | --- |
| `p` | Whole buffer | A synthetic gate |
| `p[i]` | Element | `p[i]` |
| `&p[i]`, including `&p[0]` | Partition | `p[i]` |

Ordinary indexed reads/writes also use element mode. Dereferencing a buffer
pointer uses its first element. Known pointers to scalar variables retain their
ordinary alias dependencies. Reference wrappers use `.get()` in OpenMP locators.
Parameter effect analysis determines whether an argument is read or written;
unknown indirect effects are treated as writes.

For example:

```cpp
fill(p);                 // whole-buffer write
read(p);                 // whole-buffer read
update(p[5]);            // element write
update(p[6]);            // independent element write
partition(&p[0], m);     // partition beginning at 0
partition(&p[m], n - m); // independent partition beginning at m
```

This produces `fill -> read -> {update(5), update(6)} -> {partition(0), partition(m)}`.
The pairs can execute concurrently when their ordinary scalar dependencies allow it.
Two whole-buffer readers can also execute concurrently.

## Partition contract

A partition start is a representative, not an inferred memory range. During a
partition phase, accesses using different representatives must not overlap if
one of them writes. This is an APAC input precondition: violating it can introduce
a race in the transformed program even when the sequential input is valid C++.
APAC does not infer endpoints from helper bodies or runtime sizes. Calls using
identical start addresses are ordered by OpenMP, including through different
pointer aliases.

Use an explicit `#pragma omp taskwait` before changing the partition layout while
remaining in partition mode. For example, dividing the same buffer into four
pieces after processing two halves needs this boundary. Pointer rebinding and
preserved synchronous control-flow boundaries also wait and reset the phase.

## Generated synchronization

The gate is separate storage from the buffer, declared in the function frame and
kept alive until its tasks complete. Whole-buffer readers use `depend(in: gate)`;
writers use `depend(inout: gate)`. Element/partition tasks use `depend(in: gate)`
plus a read or write dependence on their representative.

Every change of mode inserts an empty sibling task with `depend(inout: gate)`.
That task joins the previous phase and gates the next one. This includes the
whole-buffer **read** to element **write** transition. No dependency list covering
every element is generated. The runtime still has to account for the tasks it
joins; synchronization is not constant-time in the number of outstanding tasks.

The current implementation uses one conservative gate for all eligible buffers
in a function, since arbitrary pointer aliases can designate the same memory.
Consequently, whole-buffer writes and mode changes can synchronize otherwise
independent buffers. Within a fine-grained phase, dependencies use actual
representative addresses. Different element types also trigger a phase change.
A task mixing modes falls back to whole-buffer mode.

Pointer and index producers finish before OpenMP evaluates their locators. Local
plain pointer/index values are captured for the task. Complex address expressions
can require a full wait; indices read through other arrays or scalar reference
wrappers, side-effecting indices, and indices also written by the instruction are executed synchronously to avoid evaluating them twice or losing
an output through `firstprivate`.

Multidimensional buffers and other unsupported forms retain the previous analysis;
when mixed with recognized phases, ambiguous accesses are synchronized
conservatively. Hidden global accesses inside callees are still outside caller
dependency analysis. The programmer remains responsible for their protection.

DOT output labels phases and representatives and displays phase transitions in
purple. Textually identical representatives have visible edges; equality through
other pointer aliases is resolved by OpenMP at runtime.

Run `python3 scripts/array-phase-regression.py` to compare sequential and transformed
execution at `-O0`/`-O2` with 1, 2 and 4 OpenMP workers. The suite also checks the
whole-read transition, structural independence of element tasks, aliases,
rebinding, partition epochs, wrapper locators and the full compilation pipeline.
