#include "support.hpp"
void jacobi_rows(const double *source, double *out, int n, int first, int count,
                 int grain) {
  if (count > grain) {
    int m;
    m = count / 2;
    jacobi_rows(&source[0], &out[0], n, first, m, grain);
    jacobi_rows(&source[0], &out[m * n], n, first + m, count - m, grain);
  } else {
    jacobi_leaf_apacSeq(source, out, n, first, count);
  }
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  for (int step = 0; step < steps; ++step) {
    if (step % 2 == 0)
      jacobi_rows(x, &y[n], n, 1, n - 2, grain);
    else
      jacobi_rows(y, &x[n], n, 1, n - 2, grain);
  }
  return 0;
}
