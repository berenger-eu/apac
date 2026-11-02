#include "support.hpp"
unsigned long long queens(unsigned long long mask, unsigned long long columns,
                          unsigned long long left, unsigned long long right,
                          int depth, int cutoff);
unsigned long long choices(unsigned long long available,
                           unsigned long long mask, unsigned long long columns,
                           unsigned long long left, unsigned long long right,
                           int depth, int cutoff) {
  if (available == 0)
    return 0;
  unsigned long long bit;
  bit = available & (~available + 1);
  unsigned long long a;
  a = queens(mask, columns | bit, (left | bit) << 1, (right | bit) >> 1,
             depth + 1, cutoff);
  unsigned long long b;
  b = choices(available ^ bit, mask, columns, left, right, depth, cutoff);
  return a + b;
}
unsigned long long queens(unsigned long long mask, unsigned long long columns,
                          unsigned long long left, unsigned long long right,
                          int depth, int cutoff) {
  if (columns == mask)
    return 1;
  if (depth >= cutoff)
    return queens_leaf_apacSeq(mask, columns, left, right);
  unsigned long long available;
  available = mask & ~(columns | left | right);
  return choices(available, mask, columns, left, right, depth, cutoff);
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  return queens((1ULL << n) - 1, 0, 0, 0, 0, grain);
}
