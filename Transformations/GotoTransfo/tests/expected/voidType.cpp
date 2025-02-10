#include "_apac_header.hpp"
void f();
void f() {
  {

    int a = 1;
    goto __exit0;
  }
__exit0:
  return;
}
void g() {
  { int b = 1; }
__exit1:
  return;
}

int main() {
  wrapper_t<int> __result;
  {

    f();
    g();
    __result = build_wrapper<int>(0);
    goto __exit2;
  }
__exit2:
  return *__result;
}