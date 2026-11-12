int f(int a, int b) { return a + b; }
int g(int a) { return a; }
int main() {
  int b = 4;
  auto __tempVar_0 = f(b, 2);
  auto __tempVar_1 = g(4);
  auto __tempVar_2 = f(7, 8);
  auto __tempVar_3 = g(b);
  auto __tempVar_4 = f(static_cast<decltype(__tempVar_0)>(__tempVar_0),
                       static_cast<decltype(__tempVar_1)>(__tempVar_1) +
                           static_cast<decltype(__tempVar_2)>(__tempVar_2) -
                           static_cast<decltype(__tempVar_3)>(__tempVar_3));
  int a = g(static_cast<decltype(__tempVar_4)>(__tempVar_4) * 4);
  return 1;
}