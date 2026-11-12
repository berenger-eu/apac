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
int square(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * x;
  } else {
    return square_apacSeq(x);
  }
}
int cube(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * x * x;
  } else {
    return cube_apacSeq(x);
  }
}
int double_val(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x + x;
  } else {
    return double_val_apacSeq(x);
  }
}
int negate_val(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return -x;
  } else {
    return negate_val_apacSeq(x);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    int a;

#pragma omp task default(shared) depend(inout : a)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      a = square(3);
    }

    int b;

#pragma omp task default(shared) depend(inout : b)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      b = cube(4);
    }

    int c;

#pragma omp task default(shared) depend(inout : c)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      c = double_val(5);
    }

    int d;

#pragma omp task default(shared) depend(inout : d)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
      d = negate_val(6);
    }

    int total;

#pragma omp task default(shared) depend(in : a, b, c, d)
    { total = a + b + c + d; }

#pragma omp taskwait
    return total;
  } else {
    return main_apacSeq();
  }
}
