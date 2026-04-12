int main() {
  const int a = 1, b = 2, c = 3;
  volatile int x = 0, y = 0;
  return a + b + c + x + y;
}
