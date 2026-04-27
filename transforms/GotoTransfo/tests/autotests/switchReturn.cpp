int classify(int n) {
  switch (n) {
  case 0:
    return 0;
  case 1:
    return 1;
  case 2:
    return 4;
  default:
    return -1;
  }
}

int main() {
  return classify(2);
}
