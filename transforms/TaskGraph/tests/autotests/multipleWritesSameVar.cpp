int f(int x) { return x + 1; }
int g(int x) { return x * 3; }

int main() {
  int x;
  x = f(5);
  x = g(x);
  int result;
  result = x + 1;
  return result;
}
