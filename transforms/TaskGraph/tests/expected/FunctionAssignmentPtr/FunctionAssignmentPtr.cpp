#include <cstring>
#include <omp.h>

const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int *inPtrs(int *a, int *b, int c, int **d) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return a;
  } else {
    return inPtrs_apacSeq(a, b, c, d);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      int valA, valB, valC, valF;

      int *a, *b, *c;

      int d;

      int *f, **e;

#pragma omp task default(shared) depend(inout : b)
      { b = &valB; }

#pragma omp task default(shared) depend(inout : c)
      { c = &valC; }

#pragma omp task default(shared) depend(inout : e)
      { e = &f; }

#pragma omp task default(shared) depend(inout : d)
      { d = 4; }

#pragma omp task default(shared) depend(inout : a)
      {
        a = &valA;
        *a = 1;
      }

#pragma omp task default(shared) depend(inout : valB) depend(in : b)
      { *b = 2; }

#pragma omp task default(shared) depend(inout : valC) depend(in : c)
      { *c = 3; }

#pragma omp task default(shared) depend(inout : f, valF)
      {
        f = &valF;
        *f = 10;
      }

#pragma omp task default(shared) depend(inout : f) depend(in : e)
      { *e = f; }

#pragma omp task default(shared) depend(inout : a) depend(in : b, c, e, d)     \
    firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        a = inPtrs(b, c, d, e);
      }

#pragma omp task default(shared) depend(inout : valB, valC, valF) depend(in : a)
      { *a = 4; }

#pragma omp task default(shared) depend(inout : d)
      { d = 5; }

#pragma omp task default(shared) depend(inout : valB)
      { *b = 5; }

#pragma omp task default(shared) depend(inout : valC)
      { *c = 6; }

#pragma omp task default(shared) depend(inout : valF) depend(in : f, e)
      {
        *f = 11;
        **e = 12;
      }

#pragma omp task default(shared) depend(inout : d) depend(in : valB, valC, valF)
      { d = *a; }
    }
    return 0;
  } else {
    return main_apacSeq();
  }
}