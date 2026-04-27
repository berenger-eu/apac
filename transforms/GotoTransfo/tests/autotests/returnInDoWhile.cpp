int findFirst(int *arr, int n, int target) {
  int i = 0;
  do {
    if (arr[i] == target)
      return i;
    i++;
  } while (i < n);
  return -1;
}

int main() {
  int arr[5] = {3, 1, 4, 1, 5};
  return findFirst(arr, 5, 4);
}
