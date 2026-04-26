int getValue() { return 42; }

int main() {
  if (int x = getValue(); x > 0) {
    if (int y = x * 2; y > 50) {
      return y;
    }
    return x;
  }
  return 0;
}
