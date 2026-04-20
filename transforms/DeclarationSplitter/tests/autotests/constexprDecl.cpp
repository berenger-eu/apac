constexpr int square(int x) { return x * x; }

int main() {
  constexpr int a = 5;
  constexpr int b = square(3);
  int c = a + b;
  return c;
}
