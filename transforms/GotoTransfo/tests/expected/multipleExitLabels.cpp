#include "_apac_header.hpp"
int sign(int x) {
  wrapper_t<int> __result;
  {

    if (x > 0) {
      __result = build_wrapper<int>(1);
      goto __exit0;
    }
    if (x < 0) {
      __result = build_wrapper<int>(-1);
      goto __exit0;
    }
    __result = build_wrapper<int>(0);
    goto __exit0;
  }
__exit0:;
  return *__result;
}

double absVal(double x) {
  wrapper_t<double> __result;
  {

    if (x < 0.0) {
      __result = build_wrapper<double>(-x);
      goto __exit1;
    }
    __result = build_wrapper<double>(x);
    goto __exit1;
  }
__exit1:;
  return *__result;
}

int main() { return sign(-3) + (int)absVal(-2.5); }
