int f(int a, int b) { return a + b; }
int g(int a) { return a; }
int main() {
  int b = 4;
  int __tempVar_0 = f(b, 2);
  int __tempVar_1 = g(4);
  int __tempVar_2 = f(7, 8);
  int __tempVar_3 = g(b);
  int __tempVar_4 = f(__tempVar_0, __tempVar_1 + __tempVar_2 - __tempVar_3);
  int __tempVar_5 = g(__tempVar_4 * 4);
  int a = __tempVar_5;
  return 1;
}