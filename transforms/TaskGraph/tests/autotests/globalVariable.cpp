int counter = 0;

int increment(int delta) {
  counter += delta;
  return counter;
}

int main() {
  int a;
  a = increment(1);

  int b;
  b = increment(2);

  return a + b + counter;
}
