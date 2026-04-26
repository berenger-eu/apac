int compute() { return 42; }

int main() {
  auto a = 5;
  auto b = compute();
  auto c = a + b;
  return c;
}
