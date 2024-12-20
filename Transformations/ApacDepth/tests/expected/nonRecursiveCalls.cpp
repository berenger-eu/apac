#include <cstring>
#include <omp.h>
int funcMock(int a) { return a + 1; }

const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int funcRec(int a) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    if (a == 0) {
      return 0;
    } else {
      __apac_depth = __apac_depth_local + 1;
      int result = funcRec(a - 1);
      result = result + funcMock(a - 1);
      return result;
    }
  } else {
    return funcRec_apacSeq(a);
  }
}