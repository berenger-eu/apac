#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int main() {
  int a, j;
  a = 4;
  std::reference_wrapper<int> ref = invalid_ref<int>();
  ref = a;
  j = 4;
  a++;
  j = 1;
  ref++;
}