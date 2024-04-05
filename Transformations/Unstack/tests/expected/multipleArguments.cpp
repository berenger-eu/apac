int f(int a, int b) { return a + b; }

int main() {
  int __tempVar_0 = f(1, 2);
  int __tempVar_1 = f(4, 5);
  int __tempVar_2 = f(__tempVar_0, __tempVar_1);
  int a = __tempVar_2;
  return 1;
}