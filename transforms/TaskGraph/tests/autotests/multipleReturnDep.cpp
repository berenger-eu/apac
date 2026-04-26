int f(int x) { return x + 1; }
int g(int x) { return x * 2; }
int h(int x, int y) { return x - y; }

int main() {
  int a;
  a = f(1);
  int b;
  b = g(2);
  int c;
  c = h(a, b);
  int d;
  d = c + a;
  return d;
}
