int f(int a) { return a + 1; }
int main() {
  int __tempVar_2 = f(4);
  int __tempVar_1 = f(__tempVar_2);
  int a = f(__tempVar_1), b = f(4), c;
  return 0;
}