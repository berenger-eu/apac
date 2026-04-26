int main() {
  volatile int flag = 0, counter = 0;
  flag = 1;
  counter = flag + 1;
  return counter;
}
