int *inPtrs(int *a, int *b) { return a; }

int main() {
  int *a;
  int *b;
  int *c;
  *b = 5;
  *c = 6;
  a = inPtrs(b, c);
  *a = 5;
  *b = 6;
  *c = 7;
  return 0;
}