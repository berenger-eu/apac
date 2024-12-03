#include <functional>
#include <optional>
template <class T> T &invalid_ref() {
  T *ptr = nullptr;
  return (*ptr);
}
int main() {
  int(*apacMemeBloc__a_0)[2][2];
  std::reference_wrapper<int(*)[2][2]> a = invalid_ref<int(*)[2][2]>();
#pragma omp task depend(inout : apacMemeBloc__a_0, a)
  {
    apacMemeBloc__a_0 = new int[2][2][2];
    a = apacMemeBloc__a_0;
  }
  int b;
#pragma omp task
  { b = 4; }
  const int c = 4;
  int d, e;
#pragma omp task depend(inout : d)
  { d = 4; }
#pragma omp task depend(inout : e)
  { e = 7; }
#pragma omp task depend(in : apacMemeBloc__a_0, a)
  {
    a[0][0][0] = 1;
    a[0][0][0] = 4;
  }
#pragma omp task depend(in : apacMemeBloc__a_0, a)
  { a[1][1][1] = 2; }
#pragma omp task depend(inout : a[0][0][1])                                    \
    depend(in : apacMemeBloc__a_0, a, d, e)
  { a[0][0][1] = d + e; }
#pragma omp task depend(inout : a[d][0][1], a[e][0][1])                        \
    depend(in : apacMemeBloc__a_0, a, d, e)
  {
    a[d][0][1] = e;
    a[d][0][1] = 7;
    a[e][0][1] = a[d][0][1];
    a[e][0][1] = 9;
  }
#pragma omp task depend(in : apacMemeBloc__a_0, a, d, e, a[0][0][1],           \
                            a[e + 1][0][1])
  {
    a[e + 1][0][1] = a[0][0][1];
    a[0][0][1] = d + e;
  }
#pragma omp taskwait apacMemeBloc__a_0
  delete[] apacMemeBloc__a_0;
  return 0;
}