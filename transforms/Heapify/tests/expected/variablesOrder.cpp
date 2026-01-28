int main() {
  int *const apacMemeBloc__n_0 = new int(4);
  int &n = *(apacMemeBloc__n_0);
  ;
  {
    int n = 1;
    delete apacMemeBloc__n_0;
    return 1;
  }
  int *const apacMemeBloc__m_1 = new int(5);
  int &m = *(apacMemeBloc__m_1);
  ;
  {
    int n = 1;
    delete apacMemeBloc__n_0;
    delete apacMemeBloc__m_1;
    return 1;
  }
  delete apacMemeBloc__n_0;
  delete apacMemeBloc__m_1;
}