
int recFunc(int a, int b) {
  if (a == 0) {
    return b;
  }
  return recFunc(a - 1, b + 1);
}