int main() {
  int *apacMemeBloc__tab_0;
#pragma omp task depend(inout : apacMemeBloc__tab_0)
  { apacMemeBloc__tab_0 = new int[10]; }
#pragma omp taskwait apacMemeBloc__tab_0
  delete[] apacMemeBloc__tab_0;
  return 1;
}