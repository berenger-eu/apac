#include <cstring>
#include <functional>
#include <omp.h>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int a, j;
#pragma omp taskgroup
  {
    std::reference_wrapper<int> ref = invalid_ref<int>();
    std::reference_wrapper<int> ref2 = invalid_ref<int>();
#pragma omp task default(shared) depend(inout : ref2, j)                       \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      ref2 = j;
    }
#pragma omp task default(shared) depend(inout : a, ref)                        \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      a = 4;
      ref = a;
    }
#pragma omp task default(shared) depend(inout : a, ref) depend(in : ref2, j)   \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      ref = ref2;
    }
#pragma omp task default(shared) depend(in : ref2, j)                          \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      j = 4;
      ref2++;
    }
#pragma omp task default(shared) depend(in : a, ref)                           \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      a++;
      ref++;
    }
    ;
  }
  return 0;
}