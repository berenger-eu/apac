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
  char __apac_array_phase_0;
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

#pragma omp taskwait depend(in : tab)
#pragma omp task default(shared) depend(in : tab)                              \
    depend(in : __apac_array_phase_0) depend(inout : tab[4]) firstprivate(tab)
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
  char __apac_array_phase_2;
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int *apacMemeBloc__tab_0;

#pragma omp taskwait
#pragma omp taskwait
      apacMemeBloc__tab_0 = new int[10];

      std::reference_wrapper<int *> tab = invalid_ref<int *>();

#pragma omp taskwait
      tab = apacMemeBloc__tab_0;

      int x;

#pragma omp task default(shared) depend(inout : x)
      { x = 4; }

#pragma omp taskwait depend(in : apacMemeBloc__tab_0, tab)
#pragma omp task default(shared) depend(in : apacMemeBloc__tab_0, tab)         \
    depend(inout : __apac_array_phase_2) firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummy(tab);
      }

#pragma omp taskwait depend(in : apacMemeBloc__tab_0, tab)

#pragma omp task depend(inout : __apac_array_phase_2)
      {}
#pragma omp task default(shared) depend(in : apacMemeBloc__tab_0, tab)         \
    depend(in : __apac_array_phase_2) depend(inout : tab.get()[5])             \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        fdummyRef(tab[5]);
      }

#pragma omp taskwait depend(in : apacMemeBloc__tab_0, tab, x)
#pragma omp task default(shared) depend(in : apacMemeBloc__tab_0, tab, x)      \
    depend(in : __apac_array_phase_2) depend(inout : tab.get()[x])             \
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