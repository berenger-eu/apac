#include "support.hpp"
void merge_sort(int *p, int *aux, int n, int grain) {
  if (n > grain) {
    int m;
    m = n / 2;
    merge_sort(&p[0], &aux[0], m, grain);
    merge_sort(&p[m], &aux[m], n - m, grain);
    merge_apacSeq(p, aux, n, m);
  } else {
    insertion_apacSeq(p, n);
  }
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  merge_sort(keys, aux, n, grain);
  return 0;
}
