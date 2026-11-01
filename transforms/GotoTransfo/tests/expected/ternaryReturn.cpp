#include "_apac_header.hpp"
int max(int a, int b) {
  wrapper_t<int> __result;
  {

    __result = build_wrapper<int>((a > b) ? a : b);
    goto __exit0;
  }
__exit0:;
  return *__result;
}

int clamp(int v, int lo, int hi) {
  wrapper_t<int> __result;
  {

    __result = build_wrapper<int>((v < lo) ? lo : (v > hi) ? hi : v);
    goto __exit1;
  }
__exit1:;
  return *__result;
}
