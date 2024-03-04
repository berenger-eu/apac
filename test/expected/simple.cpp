int main() {
  int *const apacMemeBloc__n_0 = new int(5);
  int &n = *(apacMemeBloc__n_0);
  delete apacMemeBloc__n_0;
  return 2;
}