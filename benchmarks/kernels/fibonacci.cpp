#include "support.hpp"
unsigned long long fibonacci(int n, int grain) {
  if (n <= grain || n < 2)
    return fib_leaf_apacSeq(n);
  unsigned long long left;
  left = fibonacci(n - 1, grain);
  unsigned long long right;
  right = fibonacci(n - 2, grain);
  return left + right;
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  return fibonacci(n, grain);
}
