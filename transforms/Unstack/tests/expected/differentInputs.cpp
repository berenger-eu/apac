int f(int a) { return a; }
int &g(int &a) { return a; }
bool h(bool a) { return a; }
int main() {

  int a = 1;
  auto __tempVar_0 = f(a);
  a = f(static_cast<decltype(__tempVar_0)>(__tempVar_0));
  g(a)++;
  f(g(a));
  int b = f(a++);
  int c = g(a)++;
  bool d = h(true);
  return 0;
}