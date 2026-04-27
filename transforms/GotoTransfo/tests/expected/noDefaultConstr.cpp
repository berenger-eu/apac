#include "_apac_header.hpp"
class point {

public:
  point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

point createPoint() {
  wrapper_t<point> __result;
  {

    if (true) {
      __result = build_wrapper<point>(point(1, 2));
      goto __exit0;
    } else {
      __result = build_wrapper<point>(point(3, 4));
      goto __exit0;
    }
  }
__exit0:;
  return *__result;
}
int main() { return 0; }