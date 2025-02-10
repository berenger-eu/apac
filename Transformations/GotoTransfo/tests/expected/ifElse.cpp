#include "_apac_header.hpp"
int main() {
  wrapper_t<int> __result;

  if (1) {
    __result = build_wrapper<int>(4);
    goto __exit0;
  } else if (1) {
    __result = build_wrapper<int>(5);
    goto __exit0;
  } else {
    __result = build_wrapper<int>(1);
    goto __exit0;
  }
__exit0:
  return *__result;
}