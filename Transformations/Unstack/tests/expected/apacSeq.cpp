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
  int __tempVar_1 = f(5);
  f(__tempVar_1);
  return 0;
}