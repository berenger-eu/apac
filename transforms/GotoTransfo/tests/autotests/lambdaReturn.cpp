#include <functional>

int applyAndAdd(int a, int b) {
  auto doubler = [](int x) { return x * 2; };
  if (a > 0)
    return doubler(b) + a;
  return doubler(a) + b;
}

int main() {
  return applyAndAdd(3, 4);
}
