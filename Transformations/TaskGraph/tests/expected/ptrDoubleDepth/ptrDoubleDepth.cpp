#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int a, b;
#pragma omp taskgroup
  {
    int *pa, *pb;
    int **ppa;
#pragma omp task default(shared) depend(inout : pa)
    { pa = &a; }
#pragma omp task default(shared) depend(inout : pb)
    { pb = &b; }
#pragma omp task default(shared) depend(inout : ppa)
    { ppa = &pa; }
#pragma omp task default(shared) depend(inout : a) depend(in : pa, ppa)
    { **ppa = 10; }
#pragma omp task default(shared) depend(inout : a)
    { a = 1; }
#pragma omp task default(shared) depend(inout : pa) depend(in : pb, ppa)
    {
      *ppa = pb;
      **ppa = 12;
      b = 2;
    }
    ;
  }
  return 0;
}