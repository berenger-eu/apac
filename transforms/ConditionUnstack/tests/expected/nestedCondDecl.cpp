int getValue() { return 42; }

int main() {
  {
    int x = getValue();
    if (x > 0) {
      {
        int y = x * 2;
        if (y > 50) {
          return y;
        }
      }
      return x;
    }
  }
  return 0;
}
