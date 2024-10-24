int main() {
  int(*apacMemeBloc__a_0)[2][2];
  apacMemeBloc__a_0 = new int[2][2][2];
  int(*&a)[2][2] = (apacMemeBloc__a_0);
  int b;
  b = 4;
  const int c = 4;
  int d;
  d = 4;
  a[0][0][0] = 1;
  a[1][1][1] = 2;
  a[0][0][1] = 3;
  a[d][0][1] = 4;
  delete[] apacMemeBloc__a_0;
  return 0;
}