int main() {
  int *const apacMemeBloc__a_0 = new int(5);
  int &a = *(apacMemeBloc__a_0);
  int &b = a;

  const int *const apacMemeBloc__f_0 = new const int(5);
  const int &f = *(apacMemeBloc__f_0);
  delete apacMemeBloc__a_0;
  delete apacMemeBloc__f_0;
  return 1;
}