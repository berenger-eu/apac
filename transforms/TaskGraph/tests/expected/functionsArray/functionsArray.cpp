#include <cstring>
#include <functional>
#include <omp.h>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
void fdummy(int &ref) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
#pragma omp task default(shared)
      { ref++; }
    }
  } else {
    fdummy_apacSeq(ref);
  }
}
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
      int(*apacMemeBloc__a_0)[2][2];
      std::reference_wrapper<int(*)[2][2]> a = invalid_ref<int(*)[2][2]>();
#pragma omp task default(shared) depend(inout : apacMemeBloc__a_0, a)          \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        apacMemeBloc__a_0 = new int[2][2][2];
        a = apacMemeBloc__a_0;
      }
      int x;
#pragma omp task default(shared) depend(inout : x)
      { x = 4; }
#pragma omp task default(shared) depend(in : apacMemeBloc__a_0, a)             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummy(a[0][0][0]);
      }
#pragma omp task default(shared) depend(in : apacMemeBloc__a_0, a)             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummy(a[0][0][1]);
      }
#pragma omp task default(shared) depend(inout : a[0][1][0])                    \
    depend(in : apacMemeBloc__a_0, a) firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummy(a[0][1][0]);
      }
#pragma omp taskwait depend(in : x)
#pragma omp task default(shared) depend(inout : a[0][1][x])                    \
    depend(in : apacMemeBloc__a_0, a, x) firstprivate(__apac_depth_local, x)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummy(a[0][1][x]);
      }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}