#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int &inRef(int &a, int &b, char d) { return a; }

int main() {
  int a, b, j;
  a = 4;
  b = 5;
  // int d;
  // int e;
  // e = 7;
  // d = 4;
  // int *pd;
  // pd = &d;
  char ca;
  ca = 'a';
  std::reference_wrapper<int> ref = invalid_ref<int>();
  ref = inRef(a, b, ca);

  a++;
  b++;
  ref++;

  //*pd = 5;
  // e = 8;
  ca = 'b';

  return 0;
}