int main() {

  int a, b;

#pragma omp task depend(inout : a)
  { a = 4; }

#pragma omp task depend(inout : b)
  { b = 5; }

  int *pa, *pb;

  int *p;

#pragma omp task depend(inout : pb)
  { pb = &b; }

#pragma omp task depend(inout : p)
  {
    pa = &a;
    p = pa;
  }

#pragma omp task depend(inout : a) depend(in : p)
  { *p = 5; }

#pragma omp task depend(inout : p) depend(in : pb)
  { p = pb; }

#pragma omp task depend(in : b, p)
  { *p = 6; }

#pragma omp task depend(in : a)
  {
    a++;
    (*pa)++;
  }

  return 0;
}