int work(int x) { return x * 3; }

int main() {
  int sum;
  sum = 0;
  int i;
  i = 0;
  while (i < 4) {
    int tmp;
    tmp = work(i);
    sum = sum + tmp;
    i = i + 1;
  }
  return sum;
}
