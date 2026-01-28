#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <omp.h>

/////////////////////////////////////////////////////////////////////////
// Version 1: Sequential Fibonacci
/////////////////////////////////////////////////////////////////////////

const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

long long fib_seq(long long int n) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

    if (n < 2)
      return n;

    __apac_depth = __apac_depth_local + 1;
    long long int x = fib_seq(n - 1);
    __apac_depth = __apac_depth_local + 1;
    long long int y = fib_seq(n - 2);

    return x + y;
  } else {
    return fib_seq_apacSeq(n);
  }
}