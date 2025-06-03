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
      int a;
      int a1;
      int *b;
      int *b1;
      int **c;
#pragma omp task default(shared) depend(inout : c)
      { c = &b; }
#pragma omp task default(shared) depend(inout : b)
      { b = &a; }
#pragma omp task default(shared) depend(inout : a)
      { a = 1; }
#pragma omp task default(shared) depend(inout : a) depend(in : b)
      { *b = 2; }
#pragma omp task default(shared) depend(inout : a) depend(in : c, b)
      { **c = 3; }
#pragma omp task default(shared) depend(inout : b1)
      { b1 = &a1; }
#pragma omp task default(shared) depend(inout : c)
      { c = &b1; }
#pragma omp task default(shared) depend(in : c, b1)
      {
        **c = 4;
        a1 = 4;
      }
#pragma omp task default(shared) depend(inout : a) depend(in : b)
      {
        *b = 5;
        a = 5;
      }
    }
    return 1;
  } else {
    return main_apacSeq();
  }
}