#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int a;
#pragma omp taskgroup
  {
#pragma omp task default(shared) depend(inout : a)
    { a = 4; }
    int *p;
#pragma omp task default(shared) depend(inout : p)
    { p = &a; }
#pragma omp task default(shared) depend(inout : a) depend(in : p)
    {
      (*p)++;
      a++;
    }
    int j;
#pragma omp task default(shared) depend(inout : j)
    { j = 4; }
#pragma omp task default(shared) depend(in : a, p, j)
    {
      j = *p;
      j = a;
    }
    ;
  }
  return 0;
}