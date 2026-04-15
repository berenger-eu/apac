#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int square(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * x;
  } else {
    return square_apacSeq(x);
  }
}
int cube(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * x * x;
  } else {
    return cube_apacSeq(x);
  }
}
int double_val(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x + x;
  } else {
    return double_val_apacSeq(x);
  }
}
int negate_val(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return -x;
  } else {
    return negate_val_apacSeq(x);
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
        a = square(3);
      }

      int b;

#pragma omp task default(shared) depend(inout : b)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        b = cube(4);
      }

      int c;

#pragma omp task default(shared) depend(inout : c)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        c = double_val(5);
      }

      int d;

#pragma omp task default(shared) depend(inout : d)                             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        d = negate_val(6);
      }

      int total;

#pragma omp task default(shared) depend(in : a, b, c, d)
      { total = a + b + c + d; }
    }
    return total;
  } else {
    return main_apacSeq();
  }
}
