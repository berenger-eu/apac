#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int heavy(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * x;
  } else {
    return heavy_apacSeq(x);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int a;

#pragma omp task default(shared) depend(inout : a)
      { a = 5; }

      int b;

#pragma omp task default(shared) depend(inout : b)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        b = heavy(3);
      }

      int c;

#pragma omp task default(shared) depend(inout : c)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        c = heavy(7);
      }

      int result;

#pragma omp task default(shared) depend(in : a, b, c)
      { result = a > 4 ? b : c; }
    }
    return result;
  } else {
    return main_apacSeq();
  }
}
