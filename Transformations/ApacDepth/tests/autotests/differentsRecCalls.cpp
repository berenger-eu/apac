

int recFunct(int a) {
  if (a == 0) {
    return 0;
  } else {
    int result = recFunct(a - 1);
    result = recFunct(a - 1);
    recFunct(a - 1);
    return result;
  }
}