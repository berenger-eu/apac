#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

int f() { return 5; }

int &t(int &a) { return a; }
const int &tcons(const int &a) { return a; }
int main() {
  int a;
  std::reference_wrapper<const int> b = invalid_ref<int>();
  const int &__refT_b = 2;
  b = __refT_b;

  std::reference_wrapper<int> ref = invalid_ref<int>();
  ref = a;

  std::reference_wrapper<int> reffun = invalid_ref<int>();
  int &__refT_reffun = t(a);
  reffun = __refT_reffun;

  std::reference_wrapper<int> referef = invalid_ref<int>();
  referef = ref;

  std::reference_wrapper<const int> ref2 = invalid_ref<const int>();
  ref2 = b;

  std::reference_wrapper<const int> ref2f = invalid_ref<int>();
  const int &__refT_ref2f = f();
  ref2f = __refT_ref2f;

  std::reference_wrapper<const int> ref3 = invalid_ref<const int>();
  ref3 = a;

  std::reference_wrapper<const int> ref4 = invalid_ref<const int>();
  ref4 = ref;

  std::reference_wrapper<const int> ref5 = invalid_ref<int>();
  const int &__refT_ref5 = tcons(b);
  ref5 = __refT_ref5;

  std::reference_wrapper<const int> ref6 = invalid_ref<int>();
  const int &__refT_ref6 = 5;
  ref6 = __refT_ref6;

  return 0;
}