int main() {
  int a;
  a = 4;
  int *p;
  p = &a;
  (*p)++;
  a++;
  int j;
  j = 4;
  j = *p;

  j = a;
  return 0;
}