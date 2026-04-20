#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int compute() { return 42; }

int main() {
  int a;
  a = 5;
  int b;
  b = compute();
  int c;
  c = a + b;
  return c;
}
