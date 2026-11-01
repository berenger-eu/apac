#include "_apac_header.hpp"
int func_apacSeq(long long n) {
  int a;
  a = 4;
  if (n > 0) {
    return 1;
  } else {
    return 0;
  }
}

int main() {
  wrapper_t<int> __result;
  {

    long n;
    n = 654;
    __result = build_wrapper<int>(0);
    goto __exit0;
  }
__exit0:;
  return *__result;
}