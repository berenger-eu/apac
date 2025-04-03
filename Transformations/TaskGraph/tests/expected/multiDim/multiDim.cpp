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
int main() {
  int __apac_depth_local = __apac_depth;
  int(*apacMemeBloc__a_0)[2][2];
#pragma omp taskgroup
  {
    std::reference_wrapper<int(*)[2][2]> a = invalid_ref<int(*)[2][2]>();
#pragma omp task default(shared) depend(inout : apacMemeBloc__a_0, a)          \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      apacMemeBloc__a_0 = new int[2][2][2];
      a = apacMemeBloc__a_0;
    }
    int b;
#pragma omp task default(shared)
    { b = 4; }
    const int c = 4;
    int d, e;
#pragma omp task default(shared) depend(inout : d)
    { d = 4; }
#pragma omp task default(shared) depend(inout : e)
    { e = 7; }
#pragma omp task default(shared) depend(in : apacMemeBloc__a_0, a)             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      a[0][0][0] = 1;
      a[0][0][0] = 4;
    }
#pragma omp task default(shared) depend(in : apacMemeBloc__a_0, a)             \
    firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      a[1][1][1] = 2;
    }
#pragma omp task default(shared) depend(inout : a[0][0][1])                    \
    depend(in : apacMemeBloc__a_0, a, d, e) firstprivate(__apac_depth_local)
    {
      __apac_depth = __apac_depth_local + 1;
      a[0][0][1] = d + e;
    }
#pragma omp taskwait depend(in : e, d)
#pragma omp task default(shared) depend(inout : a[d][0][1], a[e][0][1])        \
    depend(in : apacMemeBloc__a_0, a, d, e)                                    \
    firstprivate(__apac_depth_local, e, d)
    {
      __apac_depth = __apac_depth_local + 1;
      a[d][0][1] = e;
      a[d][0][1] = 7;
      a[e][0][1] = a[d][0][1];
      a[e][0][1] = 9;
    }
#pragma omp taskwait depend(in : e)
#pragma omp task default(shared) depend(inout : a[0][0][1], a[e + 1][0][1])    \
    depend(in : apacMemeBloc__a_0, a, d, e)                                    \
    firstprivate(__apac_depth_local, e)
    {
      __apac_depth = __apac_depth_local + 1;
      a[e + 1][0][1] = a[0][0][1];
      a[0][0][1] = d + e;
    }
#pragma omp taskwait depend(inout : apacMemeBloc__a_0)
    delete[] apacMemeBloc__a_0;
    ;
  }
  return 0;
}