int main() {
  int i = 0;
  {
    bool evalI = (i % 2 == 1);
    while (evalI) {
      i++;
    }
  }

  return 0;
}