#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

constexpr int square(int x) { return x * x; }

int main() {
  constexpr int a = 5;
  constexpr int b = square(3);
  int c;
  c = a + b;
  return c;
}
