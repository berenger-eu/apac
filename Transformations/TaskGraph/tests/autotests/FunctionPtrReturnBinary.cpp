
int *inPtrs(int *a, int *b, int c, int **d) { return a; }

int main() {

  int *a, *b, *c;
  int d;
  int *f, **e;
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
  return 0;
}