#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <omp.h>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

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
int &inRef(int &a, int &b, char d) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return a;
  } else {
    return inRef_apacSeq(a, b, d);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int a, b, j;

#pragma omp task default(shared) depend(inout : a)
      { a = 4; }

#pragma omp task default(shared) depend(inout : b)
      { b = 5; }

      // int d;
      // int e;
      // e = 7;
      // d = 4;
      // int *pd;
      // pd = &d;
      char ca;

#pragma omp task default(shared) depend(inout : ca)
      { ca = 'a'; }

      std::reference_wrapper<int> ref = invalid_ref<int>();

#pragma omp task default(shared) depend(inout : a, b, ref) depend(in : ca)     \
    firstprivate(__apac_depth_local)
      {
        __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
        ref = inRef(a, b, ca);
      }

#pragma omp task default(shared) depend(inout : a, b, ref)                     \
    firstprivate(__apac_depth_local)
      {
        __apac_depth_guard __apac_task_depth(__apac_depth_local + 1);
        a++;
        b++;
        ref++;
      }

//*pd = 5;
// e = 8;
#pragma omp task default(shared) depend(inout : ca)
      { ca = 'b'; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}