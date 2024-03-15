int main() {
  int *const apacMemeBloc__m_0 = new int();
  int &m = *(apacMemeBloc__m_0);
  int *const apacMemeBloc__k_0 = new int(5);
  int &k = *(apacMemeBloc__k_0);

  delete apacMemeBloc__m_0;
  delete apacMemeBloc__k_0;
  return 1;
}