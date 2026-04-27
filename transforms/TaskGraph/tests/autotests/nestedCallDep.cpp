int double_val(int x) { return x * 2; }
int add(int a, int b) { return a + b; }

int main() {
  int a;
  a = 3;
  int b;
  b = 4;
  int result;
  result = add(double_val(a), double_val(b));
  return result;
}
