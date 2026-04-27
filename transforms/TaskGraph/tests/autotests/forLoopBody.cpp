int work(int x) { return x * 3; }

int main() {
  int sum;
  sum = 0;
  int i;
  i = 0;
  for (; i < 5; i++) {
    int tmp;
    tmp = work(i);
    sum = sum + tmp;
  }
  return sum;
}
