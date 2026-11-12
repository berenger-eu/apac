int f(int a) {
  int c;
  c = a + 1;
  return c;
}
void func_apacSeq(long long n) {
  int a;
  a = f(f(a));
}
int main() {
  long n;
  auto __tempVar_0 = f(5);
  f(static_cast<decltype(__tempVar_0)>(__tempVar_0));
  return 0;
}