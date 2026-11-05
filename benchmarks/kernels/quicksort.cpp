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
void partition(int *p, int n, int &lo, int &hi) {
  int pivot = p[n / 2];
  lo = 0;
  hi = n;
  int i = 0;
  while (i < hi) {
    if (p[i] < pivot) {
      int t = p[lo];
      p[lo++] = p[i];
      p[i++] = t;
    } else if (p[i] > pivot) {
      int t = p[--hi];
      p[hi] = p[i];
      p[i] = t;
    } else
      ++i;
  }
}

void quick_sort(int *values, int count, int cutoff) {
  if (count <= cutoff) {
    insertion_sort(values, count);
    return;
  }
  int equal_begin, equal_end;
  partition(values, count, equal_begin, equal_end);
  quick_sort(&values[0], equal_begin, cutoff);
  quick_sort(&values[equal_end], count - equal_end, cutoff);
}
