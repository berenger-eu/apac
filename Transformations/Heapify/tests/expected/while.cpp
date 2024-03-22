int main() {
  int *const apacMemeBloc__i_0 = new int(0);
  int &i = *(apacMemeBloc__i_0);

  while (i < 10) {
    i++;
    int *const apacMemeBloc__m_0 = new int();
    int &m = *(apacMemeBloc__m_0);

    delete apacMemeBloc__m_0;
  }
  while ((i++) < 10) {
    int *const apacMemeBloc__m_1 = new int();
    int &m = *(apacMemeBloc__m_1);
    delete apacMemeBloc__m_1;
  }
  {
    int *const apacMemeBloc__i_1 = new int(-1);
    int &i = *(apacMemeBloc__i_1);
    while (i) {
      i++;
    };
    delete apacMemeBloc__i_1;
  }
  {
    int *const apacMemeBloc__i_2 = new int(-1);
    int &i = *(apacMemeBloc__i_2);
    while (i)
      i++;
    delete apacMemeBloc__i_2;
  };
  delete apacMemeBloc__i_0;
}