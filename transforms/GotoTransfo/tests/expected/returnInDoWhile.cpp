#include "_apac_header.hpp"
int findFirst(int *arr, int n, int target) {
  wrapper_t<int> __result;
  {
    int i = 0;
    do {
      if (arr[i] == target) {
        __result = build_wrapper<int>(i);
        goto __exit0;
      }
      i++;
    } while (i < n);
    __result = build_wrapper<int>(-1);
    goto __exit0;
  }
__exit0:;
  return *__result;
}

int main() {
  int arr[5] = {3, 1, 4, 1, 5};
  wrapper_t<int> __result;
  {
    __result = build_wrapper<int>(findFirst(arr, 5, 4));
    goto __exit0;
  }
__exit0:;
  return *__result;
}
