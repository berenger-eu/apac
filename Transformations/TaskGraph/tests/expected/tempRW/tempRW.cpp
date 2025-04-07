#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int i;
#pragma omp taskgroup
  {
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
    {
      x = 2;
    }
    ;
  }
  return 0;
}