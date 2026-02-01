int main() {
  int a, b;
  a = 4;
  b = 5;
  int *pa, *pb;
  int *p;
  pa = &a;
  pb = &b;
  p = pa;
  *p = 5;
  p = pb;
  *p = 6;
  a++;
  (*pa)++;
  return 0;
}