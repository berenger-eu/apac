#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int f(int x) { return x * 2; }

int main() {
  int a;
  a = 1;

  {
    int b;
    b = f(a);

    {
      int c;
      c = b + a;
    }
  }
  int d;
  d = a + 10;

  return d;
}
