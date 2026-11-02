#include "support.hpp"
void quick_sort(int *p, int n, int grain) {
  if (n > grain) {
    int lo;
    int hi;
    partition_apacSeq(p, n, lo, hi);
    quick_sort(&p[0], lo, grain);
    quick_sort(&p[hi], n - hi, grain);
  } else {
    insertion_apacSeq(p, n);
  }
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  quick_sort(keys, n, grain);
  return 0;
}
