#include "_apac_header.hpp"
int classify(int n) {
  wrapper_t<int> __result;
  {

    switch (n) {
    case 0: {
      __result = build_wrapper<int>(0);
      goto __exit0;
    }
    case 1: {
      __result = build_wrapper<int>(1);
      goto __exit0;
    }
    case 2: {
      __result = build_wrapper<int>(4);
      goto __exit0;
    }
    default: {
      __result = build_wrapper<int>(-1);
      goto __exit0;
    }
    }
  }
__exit0:;
  return *__result;
}

int main() {
  wrapper_t<int> __result;
  {

    __result = build_wrapper<int>(classify(2));
    goto __exit1;
  }
__exit1:;
  return *__result;
}
