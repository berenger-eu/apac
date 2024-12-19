#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

/////////////////////////////////////////////////////////////////////////
// Version 1: Sequential Fibonacci
/////////////////////////////////////////////////////////////////////////

long long fib_seq(long long int n) {
  if (n < 2)
    return n;

  long long int x = fib_seq(n - 1);
  long long int y = fib_seq(n - 2);

  return x + y;
}