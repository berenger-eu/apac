#include "_apac_header.hpp"
int main() {
  wrapper_t<int> __result;
  bool __apac_depth_ok = true;
  if (__apac_depth_ok) {
#pragma omp parallel num_threads(nb_cores)
#pragma omp master
    {
      if (true) {
        __result = build_wrapper<int>(0);
        goto __exit0;
      }
      __result = build_wrapper<int>(1);
      goto __exit0;
    }
  }
__exit0:
  return *__result;
}