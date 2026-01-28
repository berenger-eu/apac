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
  f(f(5));
  return 0;
}