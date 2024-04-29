int f(int a) { return a + 1; }
int main() {
  int __tempVar_1 = f(4);
  int __tempVar_2 = f(__tempVar_1);
  int __tempVar_3 = f(__tempVar_2);
  int __tempVar_0 = f(4);
  int a = __tempVar_3, b = __tempVar_0, c;
  return 0;
}