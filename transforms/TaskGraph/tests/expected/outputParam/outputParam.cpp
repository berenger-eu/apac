#include <cstring>
#include <omp.h>
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

void writeResult(int *p, int val) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {
#pragma omp task default(shared)
      { *p = val * 2; }
    }
  } else {
    writeResult_apacSeq(p, val);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    int a;

    int b;

#pragma omp task default(shared) depend(inout : a)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      writeResult(&a, 3);
    }

#pragma omp task default(shared) depend(inout : b)                             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      writeResult(&b, 7);
    }

    int result;

#pragma omp task default(shared) depend(in : a, b)
    { result = a + b; }

#pragma omp taskwait
    return result;
  } else {
    return main_apacSeq();
  }
}
