int main() {
  int i = 0;
  while (true) {
    bool evalI = (i % 2 == 1);
    if (!(evalI))
      break;
    { i++; }
  }

  return 0;
}