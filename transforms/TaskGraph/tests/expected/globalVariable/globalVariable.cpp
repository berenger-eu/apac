#include <cstring>
#include <omp.h>
int counter = 0;

const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int increment(int delta) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

#pragma omp task default(shared)
      { counter += delta; }
    }
    return counter;
  } else {
    return increment_apacSeq(delta);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int a;

#pragma omp task default(shared) firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        a = increment(1);
      }

      int b;

#pragma omp task default(shared) firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        b = increment(2);
      }
    }
    return a + b + counter;
  } else {
    return main_apacSeq();
  }
}
