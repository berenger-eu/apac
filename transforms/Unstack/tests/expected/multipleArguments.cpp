int f(int a, int b) { return a + b; }

int main() {
  auto __tempVar_0 = f(1, 2);
  auto __tempVar_1 = f(5, 6);
  auto __tempVar_2 = f(4, static_cast<decltype(__tempVar_1)>(__tempVar_1));
  int a = f(static_cast<decltype(__tempVar_0)>(__tempVar_0),
            static_cast<decltype(__tempVar_2)>(__tempVar_2));
  return 1;
}