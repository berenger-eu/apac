#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}

class Point {
public:
  Point() = delete;
  Point(int x, int y) : x(x), y(y) {}

private:
  int x;
  int y;
};

int main() {
  int a;
  a = 5;

  int b;
  int c;
  int d;
  b = 5;
  c = 5;
  d = 4;

  Point p(4, 5);
  Point p2(4, 3);

  // Point p2;
  return 0;
}