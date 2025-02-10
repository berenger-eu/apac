#include "_apac_header.hpp"
class point {

public:
  point(int x, int y) : x(x), y(y) {
  __exit0:
    return;
  }
  int x;
  int y;
};

point createPoint() {
  wrapper_t<point> __result;
  __result = build_wrapper<point>(point(1, 2));
  goto __exit1;
__exit1:
  return *__result;
}
int main() {
  wrapper_t<int> __result;
  __result = build_wrapper<int>(0);
  goto __exit2;
__exit2:
  return *__result;
}