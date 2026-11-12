#include "_apac_header.hpp"
void f();
void f() {
  int a = 1;
  return;
}
void g() {
  {

    int b = 1;
    if (b)
      goto __exit0;
    else
      goto __exit0;
  }
__exit0:;
}

int main() {
  wrapper_t<int> __result;
  {

    f();
    g();
    __result = build_wrapper<int>(0);
    goto __exit1;
  }
__exit1:;
  return *__result;
}