
int main() {
  int a;
  int a1;
  int *b;
  int *b1;
  int **c;
  c = &b;

  b = &a;

  a = 1;

  *b = 2;
  **c = 3;

  b1 = &a1;
  c = &b1;
  **c = 4;
  *b = 5;
  a1 = 4;
  a = 5;
  return 1;
}