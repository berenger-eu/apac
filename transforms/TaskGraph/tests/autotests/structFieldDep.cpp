struct Point {
  int x;
  int y;
};
int computeX(int v) { return v * 2; }
int computeY(int v) { return v + 1; }

int main() {
  Point s;
  s.x = computeX(3);
  s.y = computeY(7);
  int result;
  result = s.x + s.y;
  return result;
}
