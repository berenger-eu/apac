int main() {
  const int a = 1;
  const int b = 2;
  const int c = 3;
  volatile int x = 0;
  volatile int y = 0;
  return a + b + c + x + y;
}
