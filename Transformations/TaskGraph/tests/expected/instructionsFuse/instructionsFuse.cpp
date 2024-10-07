int main() {

  int i, j;

#pragma omp task depend(inout : i)
  { i = 4; }

#pragma omp task depend(inout : j)
  { j = 4; }

  int c;

#pragma omp task depend(inout : c)
  { c = 5; }

#pragma omp task depend(in : j, i, c)
  {
    c = i + j;
    c = i - j;
    c = 2 * c;
    c = 2 * c + j - i;
  }

  return 0;
}