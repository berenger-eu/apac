int f(int a) { return a; }
int &g(int &a) { return a; }
bool h(bool a) { return a; }
int main() {

  int a = 1;
  int __tempVar_0 = f(a);
  int __tempVar_1 = f(__tempVar_0);
  a = __tempVar_1;
  int &__tempVar_2 = g(a);
  __tempVar_2++;
  int &__tempVar_3 = g(a);
  int __tempVar_4 = f(__tempVar_3);
  __tempVar_4;
  int __tempVar_5 = f(a++);
  int b = __tempVar_5;
  int &__tempVar_6 = g(a);
  int c = __tempVar_6++;
  bool __tempVar_7 = h(true);
  bool d = __tempVar_7;
  return 0;
}