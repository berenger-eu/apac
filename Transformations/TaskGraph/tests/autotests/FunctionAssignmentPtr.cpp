
int *inPtrs(int *a, int *b, int c, int **d) { return a; }

int main() {
  int valA, valB, valC, valF;
  int *a, *b, *c;
  int d;
  int *f, **e;
  a = &valA;
  b = &valB;
  c = &valC;
  f = &valF;
  e = &f;
  d = 4;
  *a = 1;
  *b = 2;
  *c = 3;
  *f = 10;
  *e = f;
  a = inPtrs(b, c, d, e);
  *a = 4;
  d = 5;
  *b = 5;
  *c = 6;
  *f = 11;
  **e = 12;
  d = *a;
  return 0;
}