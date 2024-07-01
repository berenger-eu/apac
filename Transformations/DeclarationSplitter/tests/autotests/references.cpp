int f() { return 5; }

int &t(int &a) { return a; }
const int &tcons(const int &a) { return a; }
int main() {
  int a;
  const int b = 2;

  int &ref = a;
  int &reffun = t(a);
  int &referef = ref;

  const int &ref2 = b;
  const int &ref2f = f();
  const int &ref3 = a;
  const int &ref4 = ref;
  const int &ref5 = tcons(b);

  const int &ref6 = 5;

  return 0;
}