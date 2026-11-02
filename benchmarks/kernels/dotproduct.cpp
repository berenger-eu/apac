#include "support.hpp"
double dot_product(const double *a, const double *b, int n, int grain) {
  if (n <= grain)
    return dot_leaf_apacSeq(a, b, n);
  int m;
  m = n / 2;
  double left;
  left = dot_product(&a[0], &b[0], m, grain);
  double right;
  right = dot_product(&a[m], &b[m], n - m, grain);
  return left + right;
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  z[0] = dot_product(x, y, n, grain);
  return 0;
}
