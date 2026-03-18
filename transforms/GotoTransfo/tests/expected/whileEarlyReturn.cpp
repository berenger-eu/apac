#include "_apac_header.hpp"
int firstNegative(int *arr, int size) {
  wrapper_t<int> __result;
  {

    int i = 0;
    while (i < size) {
      if (arr[i] < 0) {
        __result = build_wrapper<int>(arr[i]);
        goto __exit0;
      }
      i++;
    }
    __result = build_wrapper<int>(0);
    goto __exit0;
  }
__exit0:;
  return *__result;
}
