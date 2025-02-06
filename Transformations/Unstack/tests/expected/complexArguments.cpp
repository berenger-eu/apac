int f(int a, int b) { return a + b; }
int g(int a) { return a; }
int main() {
  int b = 4;
  int __tempVar_5 = g(4);
  int __tempVar_3 = g(b);
  int __tempVar_4 = f(7, 8);
  int __tempVar_2 = f(b, 2);
  int __tempVar_1 = f(__tempVar_2, __tempVar_5 + __tempVar_4 - __tempVar_3);
  int a = g(__tempVar_1 * 4);
  return 1;
}