void f();
void f() {
  int a = 1;
  goto __exit0;
__exit0:
  return;
}
void g() {
  int b = 1;
__exit1:
  return;
}

int main() {
  int __result;

  f();
  g();
  __result = 0;
  goto __exit2;
__exit2:
  return __result;
}