#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int main() {
  int(*apacMemeBloc__a_0)[2][2];
  apacMemeBloc__a_0 = new int[2][2][2];
  std::reference_wrapper<int(*)[2][2]> a = invalid_ref<int(*)[2][2]>();
  a = apacMemeBloc__a_0;

  int b;
  b = 4;
  const int c = 4;
  int d, e;
  d = 4;
  e = 7;

  a[0][0][0] = 1;
  a[0][0][0] = 4;
  a[1][1][1] = 2;
  a[0][0][1] = d + e;

  a[d][0][1] = e;
  a[d][0][1] = 7;
  a[e][0][1] = a[d][0][1];
  a[e][0][1] = 9;
  a[e + 1][0][1] = a[0][0][1];
  a[0][0][1] = d + e;

  delete[] apacMemeBloc__a_0;
  return 0;
}