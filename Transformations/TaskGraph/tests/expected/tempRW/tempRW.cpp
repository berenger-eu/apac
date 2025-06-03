#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
      int i;
      int x;
#pragma omp task default(shared) depend(inout : i)
      { i = 1; }
#pragma omp task default(shared) depend(inout : x)
      { x = 2; }
#pragma omp task default(shared) depend(inout : i) depend(in : x)
      { i = x; }
#pragma omp task default(shared) depend(inout : i)
      { i = 1; }
#pragma omp task default(shared) depend(inout : x)
      { x = 2; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}