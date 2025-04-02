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
    ;
  }
  return 0;
}