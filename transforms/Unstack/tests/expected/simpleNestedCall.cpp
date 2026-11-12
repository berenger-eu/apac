int f(int a) { return 5; }
int g(int a) { return 4; }
int main() {
  auto __tempVar_0 = g(5);
  int a = f(static_cast<decltype(__tempVar_0)>(__tempVar_0));
  return 1;
}