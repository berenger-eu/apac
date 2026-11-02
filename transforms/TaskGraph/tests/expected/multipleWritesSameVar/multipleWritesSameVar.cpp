#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int f(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x + 1;
  } else {
    return f_apacSeq(x);
  }
}
int g(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * 3;
  } else {
    return g_apacSeq(x);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    int x;

    int result;

#pragma omp task default(shared) firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      x = f(5);
      x = g(x);
      result = x + 1;
    }

#pragma omp taskwait
    return result;
  } else {
    return main_apacSeq();
  }
}
