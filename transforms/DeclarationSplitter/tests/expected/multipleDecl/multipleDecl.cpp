#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int main() {
  int a;
  int b;
  int d;
  int e;
  a = 5;
  d = 4;

  return 0;
}