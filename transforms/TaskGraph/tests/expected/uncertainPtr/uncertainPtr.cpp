#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int *minPtr(int *a, int *b) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return a;
  } else {
    return minPtr_apacSeq(a, b);
  }
}

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

      int *p;

// Issue when using address of a variable
#pragma omp task default(shared) depend(in : i, j)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        p = minPtr(&i, &j);
      }

#pragma omp task default(shared) depend(inout : j)
      { j++; }

#pragma omp task default(shared) depend(inout : i)
      { i++; }

#pragma omp task default(shared) depend(inout : i, j)
      { *p = 5; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}