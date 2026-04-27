int heavy(int x) { return x * x; }

int main() {
  int a;
  a = 5;
  int b;
  b = heavy(3);
  int c;
  c = heavy(7);
  int result;
  result = a > 4 ? b : c;
  return result;
}
