int main() {
  int *const apacMemeBloc__a_0 = new int(1);
  int &a = *(apacMemeBloc__a_0);
  char *const apacMemeBloc__b_0 = new char();
  char &b = *(apacMemeBloc__b_0);
  delete apacMemeBloc__a_0;
  delete apacMemeBloc__b_0;
  return 1;
}