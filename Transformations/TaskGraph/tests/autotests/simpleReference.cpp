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
  std::reference_wrapper<int> ref2 = invalid_ref<int>();
  ref2 = j;
  ref = a;
  ref = ref2;
  j = 4;
  a++;
  j = 1;
  ref++;
}