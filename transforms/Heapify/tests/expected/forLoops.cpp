int main() {
  int *const apacMemeBloc__i_0 = new int(0);
  int &i = *(apacMemeBloc__i_0);
  ;
  for (; i < 10; i++) {
    int *const apacMemeBloc__m_1 = new int(i);
    int &m = *(apacMemeBloc__m_1);
    ;
    delete apacMemeBloc__m_1;
  }
  {
    int *const apacMemeBloc__j_1 = new int(5);
    int &j = *(apacMemeBloc__j_1);
    ;
    for (; j < 10; j++) {
      int *const apacMemeBloc__m_2 = new int();
      int &m = *(apacMemeBloc__m_2);
      ;
      delete apacMemeBloc__m_2;
    }
    delete apacMemeBloc__j_1;
  }
  delete apacMemeBloc__i_0;
}