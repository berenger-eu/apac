#include "_apac_header.hpp"
#include <functional>
int applyAndAdd(int a, int b) {
  wrapper_t<int> __result;
  {
    auto doubler = [](int x) { return x * 2; };
    if (a > 0) {
      __result = build_wrapper<int>(doubler(b) + a);
      goto __exit0;
    }
    __result = build_wrapper<int>(doubler(a) + b);
    goto __exit0;
  }
__exit0:;
  return *__result;
}

int main() {
  wrapper_t<int> __result;
  {
    __result = build_wrapper<int>(applyAndAdd(3, 4));
    goto __exit0;
  }
__exit0:;
  return *__result;
}
