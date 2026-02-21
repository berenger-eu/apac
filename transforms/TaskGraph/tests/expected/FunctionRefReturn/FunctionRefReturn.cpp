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
int &inRef(int &a, int &b, char d) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {}
    return a;
  } else {
    return inRef_apacSeq(a, b, d);
  }
}
int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {
#pragma omp taskgroup
    {
      int a, b, j;
#pragma omp task default(shared) depend(inout : a)
      { a = 4; }
#pragma omp task default(shared) depend(inout : b)
      { b = 5; }
      // int d;
      // int e;
      // e = 7;
      // d = 4;
      // int *pd;
      // pd = &d;
      char ca;
#pragma omp task default(shared) depend(inout : ca)
      { ca = 'a'; }
      std::reference_wrapper<int> ref = invalid_ref<int>();
#pragma omp task default(shared) depend(inout : a, b, ref) depend(in : ca)     \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        ref = inRef(a, b, ca);
      }
#pragma omp task default(shared) depend(inout : a, b, ref)                     \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        a++;
        b++;
        ref++;
      }
//*pd = 5;
// e = 8;
#pragma omp task default(shared) depend(inout : ca)
      { ca = 'b'; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}