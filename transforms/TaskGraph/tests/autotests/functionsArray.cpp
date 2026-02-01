#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

void fdummy(int &ref) { ref++; }

int main() {
  int(*apacMemeBloc__a_0)[2][2];
  apacMemeBloc__a_0 = new int[2][2][2];
  std::reference_wrapper<int(*)[2][2]> a = invalid_ref<int(*)[2][2]>();
  a = apacMemeBloc__a_0;

  int x;
  x = 4;
  fdummy(a[0][0][0]);
  fdummy(a[0][0][1]);
  fdummy(a[0][1][0]);
  fdummy(a[0][1][x]);
  return 0;
}