#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

long sum_reduce(int *data, int start, int end) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      if (end - start <= 1) {
      }

      int mid;

#pragma omp task default(shared) depend(inout : mid)
      { mid = start + (end - start) / 2; }

      long left;

#pragma omp task default(shared) depend(in : start, mid)                       \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        left = sum_reduce(data, start, mid);
      }

      long right;

#pragma omp task default(shared) depend(in : end, mid)                         \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        right = sum_reduce(data, mid, end);
      }
    }
    return left + right;
  } else {
    return sum_reduce_apacSeq(data, start, end);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int *data;

#pragma omp task default(shared) depend(inout : data)
      { data = new int[100]; }

      long r1;

#pragma omp task default(shared) depend(inout : r1) depend(in : data)          \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        r1 = sum_reduce(data, 0, 50);
      }

      long r2;

#pragma omp task default(shared) depend(inout : r2) depend(in : data)          \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        r2 = sum_reduce(data, 50, 100);
      }

      long total;

#pragma omp task default(shared) depend(in : r1, r2)
      { total = r1 + r2; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}
