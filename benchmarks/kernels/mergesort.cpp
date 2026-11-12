void insertion_sort(int *p, int n) {
  for (int i = 1; i < n; ++i) {
    int value = p[i], j = i;
    while (j > 0 && p[j - 1] > value) {
      p[j] = p[j - 1];
      --j;
    }
    p[j] = value;
  }
}
void merge(int *p, int *aux, int n, int m) {
  int i = 0, j = m, k = 0;
  while (i < m && j < n)
    aux[k++] = p[i] <= p[j] ? p[i++] : p[j++];
  while (i < m)
    aux[k++] = p[i++];
  while (j < n)
    aux[k++] = p[j++];
  for (i = 0; i < n; ++i)
    p[i] = aux[i];
}

void merge_sort(int *values, int *scratch, int count, int cutoff) {
  if (count <= cutoff) {
    insertion_sort(values, count);
    return;
  }
  int middle = count / 2;
  merge_sort(&values[0], &scratch[0], middle, cutoff);
  merge_sort(&values[middle], &scratch[middle], count - middle, cutoff);
  merge(values, scratch, count, middle);
}
