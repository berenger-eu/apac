int f(int x) { return x * 2; }

int main() {
  int a = 1;
  {
    int b = f(a);
    {
      int c = b + a;
    }
  }
  int d = a + 10;
  return d;
}
