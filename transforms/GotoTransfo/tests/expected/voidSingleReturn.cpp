#include "_apac_header.hpp"
void f() {
  {

    if (true) {
      goto __exit0;
    }
    int x = 0;
    x = 1;
  }
__exit0:;
}
int main() {
  wrapper_t<int> __result;
  {
    __result = build_wrapper<int>(0);
    goto __exit1;
  }
__exit1:;
  return *__result;
}