int *minPtr(int *a, int *b) { return a; }

int main() {
  int i, j;
  i = 4;
  j = 4;
  int *p;
  // Issue when using address of a variable
  p = minPtr(&i, &j);
  j++;
  i++;
  *p = 5;
  return 0;
}