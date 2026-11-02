#pragma once
#include <cmath>
#include <cstdint>

// Original sequential leaves. The same leaves and cutoffs are used by the
// original and APAC versions; APAC transforms the recursive drivers.
inline void insertion_apacSeq(int *p, int n) {
  for (int i = 1; i < n; ++i) {
    int value = p[i], j = i;
    while (j > 0 && p[j - 1] > value) {
      p[j] = p[j - 1];
      --j;
    }
    p[j] = value;
  }
}
inline void partition_apacSeq(int *p, int n, int &lo, int &hi) {
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
inline void merge_apacSeq(int *p, int *aux, int n, int m) {
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
inline unsigned long long fib_leaf_apacSeq(int n) {
  if (n < 2)
    return n;
  return fib_leaf_apacSeq(n - 1) + fib_leaf_apacSeq(n - 2);
}
inline unsigned long long queens_leaf_apacSeq(unsigned long long mask,
                                              unsigned long long columns,
                                              unsigned long long left,
                                              unsigned long long right) {
  if (columns == mask)
    return 1;
  unsigned long long available = mask & ~(columns | left | right), total = 0;
  while (available) {
    unsigned long long bit = available & (~available + 1);
    available ^= bit;
    total += queens_leaf_apacSeq(mask, columns | bit, (left | bit) << 1,
                                 (right | bit) >> 1);
  }
  return total;
}
inline double dot_leaf_apacSeq(const double *a, const double *b, int n) {
  double sum = 0;
  for (int i = 0; i < n; ++i)
    sum += a[i] * b[i];
  return sum;
}
inline void gemm_leaf_apacSeq(const double *a, const double *b, double *c,
                              int stride, int rows, int cols) {
  for (int i = 0; i < rows; ++i)
    for (int k = 0; k < stride; ++k)
      for (int j = 0; j < cols; ++j)
        c[i * stride + j] += a[i * stride + k] * b[k * stride + j];
}
inline void nbody_leaf_apacSeq(const double *p, const double *v, double *out,
                               double *outv, int n, int first, int count) {
  const double dt = 0.001, mass = 1.0 / n;
  for (int local = 0; local < count; ++local) {
    int i = first + local;
    double ax = 0, ay = 0, az = 0;
    for (int j = 0; j < n; ++j) {
      double dx = p[3 * j] - p[3 * i], dy = p[3 * j + 1] - p[3 * i + 1],
             dz = p[3 * j + 2] - p[3 * i + 2];
      double r2 = dx * dx + dy * dy + dz * dz + 0.001;
      double weight = mass / (r2 * std::sqrt(r2));
      ax += dx * weight;
      ay += dy * weight;
      az += dz * weight;
    }
    outv[3 * local] = v[3 * i] + dt * ax;
    outv[3 * local + 1] = v[3 * i + 1] + dt * ay;
    outv[3 * local + 2] = v[3 * i + 2] + dt * az;
    for (int d = 0; d < 3; ++d)
      out[3 * local + d] = p[3 * i + d] + dt * outv[3 * local + d];
  }
}
inline void jacobi_leaf_apacSeq(const double *source, double *out, int n,
                                int first, int count) {
  for (int row = 0; row < count; ++row) {
    int i = first + row;
    for (int j = 1; j < n - 1; ++j)
      out[row * n + j] =
          0.25 * (source[(i - 1) * n + j] + source[(i + 1) * n + j] +
                  source[i * n + j - 1] + source[i * n + j + 1]);
  }
}
