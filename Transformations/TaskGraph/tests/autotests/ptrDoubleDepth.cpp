int main() {
  int a, b;
  int *pa, *pb;
  int **ppa;
  pa = &a;
  pb = &b;
  ppa = &pa;
  **ppa = 10;
  *ppa = pb;
  **ppa = 12;
  a = 1;
  b = 2;
  return 0;
}