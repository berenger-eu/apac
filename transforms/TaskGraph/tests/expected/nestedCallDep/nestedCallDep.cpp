#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int double_val(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * 2;
  } else {
    return double_val_apacSeq(x);
  }
}
int add(int a, int b) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return a + b;
  } else {
    return add_apacSeq(a, b);
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
      { a = 3; }

      int b;

#pragma omp task default(shared) depend(inout : b)
      { b = 4; }

      int result;

#pragma omp task default(shared) depend(in : a, b)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        result = add(double_val(a), double_val(b));
      }
    }
    return result;
  } else {
    return main_apacSeq();
  }
}
