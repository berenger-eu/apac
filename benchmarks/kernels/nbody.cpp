#include "support.hpp"
void nbody_range(const double *p, const double *v, double *out, double *outv,
                 int n, int first, int count, int grain) {
  if (count > grain) {
    int m;
    m = count / 2;
    nbody_range(&p[0], &v[0], &out[0], &outv[0], n, first, m, grain);
    nbody_range(&p[0], &v[0], &out[3 * m], &outv[3 * m], n, first + m,
                count - m, grain);
  } else {
    nbody_leaf_apacSeq(p, v, out, outv, n, first, count);
  }
}
unsigned long long benchmark_kernel(int n, int grain, int steps, int *keys,
                                    int *aux, double *x, double *y, double *z,
                                    double *tmp) {
  for (int step = 0; step < steps; ++step) {
    if (step % 2 == 0)
      nbody_range(x, y, z, tmp, n, 0, n, grain);
    else
      nbody_range(z, tmp, x, y, n, 0, n, grain);
  }
  return 0;
}
