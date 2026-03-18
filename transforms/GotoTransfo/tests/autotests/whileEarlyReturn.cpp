int firstNegative(int *arr, int size) {
  int i = 0;
  while (i < size) {
    if (arr[i] < 0) {
      return arr[i];
    }
    i++;
  }
  return 0;
}
