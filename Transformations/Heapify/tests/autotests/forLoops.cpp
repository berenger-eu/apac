int main() {
  int i = 0;
  for (; i < 10; i++) {
    int m = i;
  }
  {
    int j = 5;
    for (; j < 10; j++) {
      int m;
    }
  }
}