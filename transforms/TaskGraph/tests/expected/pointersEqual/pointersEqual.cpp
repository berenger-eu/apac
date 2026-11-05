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
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int a, b;

#pragma omp task default(shared) depend(inout : a)
      { a = 4; }

#pragma omp task default(shared) depend(inout : b)
      { b = 5; }

      int *pa, *pb;

      int *p;

#pragma omp task default(shared) depend(inout : pb)
      { pb = &b; }

#pragma omp task default(shared) depend(inout : p)
      {
        pa = &a;
        p = pa;
      }

#pragma omp task default(shared) depend(inout : a) depend(in : p)
      { *p = 5; }

#pragma omp task default(shared) depend(inout : p) depend(in : pb)
      { p = pb; }

#pragma omp task default(shared) depend(inout : b) depend(in : p)
      { *p = 6; }

#pragma omp task default(shared) depend(inout : a)
      {
        a++;
        (*pa)++;
      }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}