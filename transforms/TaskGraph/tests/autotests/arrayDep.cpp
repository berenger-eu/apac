#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

void fdummy(int *tab) { tab[4]++; }
void fdummyRef(int &ref) { ref++; }
int main() {
  int *apacMemeBloc__tab_0;
  apacMemeBloc__tab_0 = new int[10];
  std::reference_wrapper<int *> tab = invalid_ref<int *>();
  tab = apacMemeBloc__tab_0;
  int x;
  x = 4;
  fdummy(tab);
  fdummyRef(tab[5]);
  fdummyRef(tab[x]);
  return 0;
}