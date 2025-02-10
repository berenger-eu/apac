#include "_apac_header.hpp"
bool h() {
  wrapper_t<bool> __result;
  __result = build_wrapper<bool>(true);
  goto __exit0;
__exit0:
  return *__result;
}