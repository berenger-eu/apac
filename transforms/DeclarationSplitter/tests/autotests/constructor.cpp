
class Point {
public:
  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}

private:
  int x;
  int y;
};

int main() {
  Point p(4, 5);
  return 0;
}