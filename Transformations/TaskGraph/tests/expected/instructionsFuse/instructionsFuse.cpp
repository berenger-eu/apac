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
      int i, j;
#pragma omp task default(shared) depend(inout : i)
      { i = 4; }
#pragma omp task default(shared) depend(inout : j)
      { j = 4; }
      int c;
#pragma omp task default(shared) depend(inout : c)
      { c = 5; }
#pragma omp task default(shared) depend(inout : c) depend(in : i, j)
      {
        c = i + j;
        c = i - j;
        c = 2 * c;
        c = 2 * c + j - i;
      }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}