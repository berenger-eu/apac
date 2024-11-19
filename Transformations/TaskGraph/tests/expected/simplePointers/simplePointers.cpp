int main() {

  int a;

#pragma omp task depend(inout : a)
  { a = 4; }

  int *p;

#pragma omp task depend(inout : p)
  { p = &a; }

#pragma omp task depend(inout : a) depend(in : p)
  {
    (*p)++;
    a++;
  }

  int j;

#pragma omp task depend(inout : j)
  { j = 4; }

#pragma omp task depend(in : a, p, j)
  {
    j = *p;
    j = a;
  }

  return 0;
}