#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int work(int x) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return x * 3;
  } else {
    return work_apacSeq(x);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {
      int sum;
      sum = 0;
      int i;
      i = 0;
      for (; i < 5; i++) {
        int tmp;
        tmp = work(i);
        sum = sum + tmp;
      }
    }
    return sum;
  } else {
    return main_apacSeq();
  }
}
