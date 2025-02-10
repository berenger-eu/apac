#include "_apac_header.hpp"
char fun() {
  wrapper_t<char> __result;
  __result = build_wrapper<char>('c');
  goto __exit0;
__exit0:
  return *__result;
}
int main() {
  wrapper_t<int> __result;
  __result = build_wrapper<int>(4);
  goto __exit1;
__exit1:
  return *__result;
}