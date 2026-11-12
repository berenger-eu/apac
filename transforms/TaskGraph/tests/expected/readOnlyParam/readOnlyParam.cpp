#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <omp.h>

inline int __apac_default_depth(unsigned int threads) {
  int depth = 1;
  for (unsigned int remaining = threads > 1 ? threads - 1 : 0; remaining;
       remaining >>= 1)
    ++depth;
  return depth;
}
const int nb_cores = omp_get_max_threads();
const int parallel_depth = __apac_default_depth(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = [] {
  const char *value = std::getenv("APAC_DEPTH_MAX");
  if (!value)
    return parallel_depth;
  char *end = nullptr;
  errno = 0;
  long depth = std::strtol(value, &end, 10);
  if (errno == ERANGE || end == value || *end || depth < 0 || depth > INT_MAX) {
    std::fprintf(stderr, "APAC_DEPTH_MAX must be an integer between 0 and %d\n",
                 INT_MAX);
    std::exit(2);
  }
  return static_cast<int>(depth);
}();
struct __apac_depth_guard {
  int previous;
  explicit __apac_depth_guard(int depth) : previous(__apac_depth) {
    __apac_depth = depth;
  }
  ~__apac_depth_guard() { __apac_depth = previous; }
  __apac_depth_guard(const __apac_depth_guard &) = delete;
  __apac_depth_guard &operator=(const __apac_depth_guard &) = delete;
};
long sum_reduce(int *data, int start, int end) {
  char __apac_array_phase_0;
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    if (end - start <= 1) {
    }

    int mid;

#pragma omp task default(shared) depend(inout : mid)
    { mid = start + (end - start) / 2; }

    long left;

#pragma omp taskwait depend(in : data)
#pragma omp task default(shared) depend(in : start, mid, left, data)           \
    depend(in : __apac_array_phase_0) firstprivate(__apac_depth_local, data)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      left = sum_reduce(data, start, mid);
    }

    long right;

#pragma omp taskwait depend(in : data)
#pragma omp task default(shared) depend(in : end, mid, data, right)            \
    depend(in : __apac_array_phase_0) firstprivate(__apac_depth_local, data)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      right = sum_reduce(data, mid, end);
    }

#pragma omp taskwait
    return left + right;
  } else {
    return sum_reduce_apacSeq(data, start, end);
  }
}

int main() {
  char __apac_array_phase_1;
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int *data;

#pragma omp taskwait
#pragma omp taskwait
      data = new int[100];

      long r1;

#pragma omp taskwait depend(in : data)
#pragma omp task default(shared) depend(inout : r1) depend(in : data)          \
    depend(in : __apac_array_phase_1) firstprivate(__apac_depth_local, data)
      {
        __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
        r1 = sum_reduce(data, 0, 50);
      }

      long r2;

#pragma omp taskwait depend(in : data)
#pragma omp task default(shared) depend(inout : r2) depend(in : data)          \
    depend(in : __apac_array_phase_1) firstprivate(__apac_depth_local, data)
      {
        __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
        r2 = sum_reduce(data, 50, 100);
      }

      long total;

#pragma omp task default(shared) depend(in : r1, r2)
      { total = r1 + r2; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}
