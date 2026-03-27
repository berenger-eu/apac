int sign(int x) {
  if (x > 0)
    return 1;
  if (x < 0)
    return -1;
  return 0;
}

double absVal(double x) {
  if (x < 0.0)
    return -x;
  return x;
}

int main() {
  return sign(-3) + (int)absVal(-2.5);
}
