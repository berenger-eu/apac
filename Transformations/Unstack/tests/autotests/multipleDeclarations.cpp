int f(int a) { return a + 1; }
int main() {
  int a = f(f(f(4))), b = f(4), c;
  return 0;
}