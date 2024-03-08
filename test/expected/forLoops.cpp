int main() {
  int *const apacMemeBloc__i_0 = new int(0);
  int &i = *(apacMemeBloc__i_0);

  for (; i < 10; i++) {
    int *const apacMemeBloc__m_0 = new int(i);
    int &m = *(apacMemeBloc__m_0);

    delete apacMemeBloc__m_0;
  }
  for (; i < 20; i++) {
    int *const apacMemeBloc__m_1 = new int(i);
    int &m = *(apacMemeBloc__m_1);
    delete apacMemeBloc__m_1;
  }
  for (int j = 5; j < 10; j++) {
    int *const apacMemeBloc__m_2 = new int();
    int &m = *(apacMemeBloc__m_2);

    delete apacMemeBloc__m_2;
  }
  for (int j = 10; j < 20; j++) {
    int *const apacMemeBloc__m_3 = new int();
    int &m = *(apacMemeBloc__m_3);
    delete apacMemeBloc__m_3;
  }
  delete apacMemeBloc__i_0;
}