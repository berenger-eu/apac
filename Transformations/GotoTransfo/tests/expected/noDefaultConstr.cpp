#include <memory>
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
  std::unique_ptr<point> __result;
  __result = std::make_unique<point>(point(1, 2));
  goto __exit1;
__exit1:
  return *__result;
}
int main() {
  std::unique_ptr<int> __result;
  __result = std::make_unique<int>(0);
  goto __exit2;
__exit2:
  return *__result;
}