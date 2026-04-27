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
    return x * 2;
  } else {
    return g_apacSeq(x);
  }
}
int h(int x, int y) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x - y;
  } else {
    return h_apacSeq(x, y);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int a;

#pragma omp task default(shared) depend(inout : a)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        a = f(1);
      }

      int b;

#pragma omp task default(shared) depend(inout : b)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        b = g(2);
      }

      int c;

      int d;

#pragma omp task default(shared) depend(in : a, b)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        c = h(a, b);
        d = c + a;
      }
    }
    return d;
  } else {
    return main_apacSeq();
  }
}
