
int recFunc_apacSeq(int a, int b) {
  if (a == 0) {
    return b;
  }
  return recFunc_apacSeq(a - 1, b + 1);
}
int recFunc(int a, int b) {
  if (a == 0) {
    return b;
  }
  return recFunc(a - 1, b + 1);
};