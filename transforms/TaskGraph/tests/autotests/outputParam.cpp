void writeResult(int *p, int val) { *p = val * 2; }

int main() {
  int a;
  int b;
  writeResult(&a, 3);
  writeResult(&b, 7);
  int result;
  result = a + b;
  return result;
}
