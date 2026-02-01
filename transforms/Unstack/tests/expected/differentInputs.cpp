int f(int a) { return a; }
int &g(int &a) { return a; }
bool h(bool a) { return a; }
int main() {

  int a = 1;
  int __tempVar_1 = f(a);
  a = f(__tempVar_1);
  g(a)++;
  int &__tempVar_4 = g(a);
  f(__tempVar_4);
  int b = f(a++);
  int c = g(a)++;
  bool d = h(true);
  return 0;
}