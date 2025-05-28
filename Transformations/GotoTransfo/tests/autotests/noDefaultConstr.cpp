class point {

public:
  point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

point createPoint() {

  if (true) {
    return point(1, 2);
  } else {
    return point(3, 4);
  }
}
int main() { return 0; }