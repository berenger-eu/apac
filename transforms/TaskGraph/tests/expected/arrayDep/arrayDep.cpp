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
void fdummy(int *tab) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
#pragma omp task default(shared)
      { tab[4]++; }
    }
  } else {
    fdummy_apacSeq(tab);
  }
}
void fdummyRef(int &ref) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
#pragma omp task default(shared)
      { ref++; }
    }
  } else {
    fdummyRef_apacSeq(ref);
  }
}
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
      int *apacMemeBloc__tab_0;
      std::reference_wrapper<int *> tab = invalid_ref<int *>();
      int x;
#pragma omp task default(shared) depend(inout : x)
      { x = 4; }
#pragma omp task default(shared) depend(inout : tab[5])                        \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        apacMemeBloc__tab_0 = new int[10];
        tab = apacMemeBloc__tab_0;
        fdummy(tab);
        fdummyRef(tab[5]);
      }
#pragma omp taskwait depend(in : x)
#pragma omp task default(shared) depend(inout : tab[x])                        \
    depend(in : apacMemeBloc__tab_0, tab, x)                                   \
    firstprivate(__apac_depth_local, x)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummyRef(tab[x]);
      }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}