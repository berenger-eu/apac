#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int f() { return 5; }
int main() {
  int a;
  a(2);

  int b;
  b(f());

  int c;
  int d;
  int e;
  int l;
  d(f());
  e = 5;

  return 0;
}