int square(int x) { return x * x; }
int cube(int x) { return x * x * x; }
int double_val(int x) { return x + x; }
int negate_val(int x) { return -x; }

int main() {
  int a;
  a = square(3);
  int b;
  b = cube(4);
  int c;
  c = double_val(5);
  int d;
  d = negate_val(6);
  int total;
  total = a + b + c + d;
  return total;
}
