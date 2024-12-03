#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}
int main() {
  int a, j;
  std::reference_wrapper<int> ref = invalid_ref<int>();
#pragma omp task
  {
    j = 4;
    j = 1;
  }
#pragma omp task
  {
    a = 4;
    ref = a;
    a++;
    ref++;
  }
}