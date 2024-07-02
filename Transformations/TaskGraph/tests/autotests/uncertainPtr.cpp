int *minPtr(int *a, int *b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}

int main() {
  int i, j;
  i = 4;
  j = 4;
  int *p;
  p = minPtr(&i, &j);
  return 0;
}