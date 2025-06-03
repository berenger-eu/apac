#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
      int *apacMemeBloc__tab_0;
#pragma omp task default(shared) depend(inout : apacMemeBloc__tab_0)
      { apacMemeBloc__tab_0 = new int[10]; }
#pragma omp taskwait depend(inout : apacMemeBloc__tab_0)
      delete[] apacMemeBloc__tab_0;
    }
    return 1;
  } else {
    return main_apacSeq();
  }
}