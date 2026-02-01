int f(int a, int b) { return a + b; }

int main() {
  int __tempVar_3 = f(5, 6);
  int __tempVar_2 = f(4, __tempVar_3);
  int __tempVar_1 = f(1, 2);
  int a = f(__tempVar_1, __tempVar_2);
  return 1;
}