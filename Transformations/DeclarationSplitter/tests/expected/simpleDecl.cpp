#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int main() {
  int a;
  a = 5;

  int b;
  return 0;
}