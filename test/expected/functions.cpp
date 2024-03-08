void functionj() {
  int *const apacMemeBloc__nm_0 = new int();
  int &nm = *(apacMemeBloc__nm_0);

  int *const apacMemeBloc__m_0 = new int();
  int &m = *(apacMemeBloc__m_0);

  delete apacMemeBloc__nm_0;
  delete apacMemeBloc__m_0;
  return;
}

int main() {
  int *const apacMemeBloc__m_1 = new int ();
  int &m = *(apacMemeBloc__m_1);

  delete apacMemeBloc__m_1;
  return 1;
}