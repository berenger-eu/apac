#include "support.hpp"
void gemm_tile(const double *a, const double *b, double *c, int stride,
               int rows, int cols, int grain) {
  if (rows > grain) {
    int m;
    m = rows / 2;
    gemm_tile(&a[0], &b[0], &c[0], stride, m, cols, grain);
    gemm_tile(&a[m * stride], &b[0], &c[m * stride], stride, rows - m, cols,
              grain);
  } else if (cols > grain) {
    int m;
    m = cols / 2;
    gemm_tile(&a[0], &b[0], &c[0], stride, rows, m, grain);
    gemm_tile(&a[0], &b[m], &c[m], stride, rows, cols - m, grain);
  } else {
    gemm_leaf_apacSeq(a, b, c, stride, rows, cols);
  }
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  gemm_tile(x, y, z, n, n, n, grain);
  return 0;
}
