int main() {
  int *const apacMemeBloc_n_0 = new int();
  int &n = *(apacMemeBloc_n_0);

  if (true) {
    int *const apacMemeBloc__m_0 = new int();
    int &m = *(apacMemeBloc__m_0);

    delete apacMemeBloc_n_0;
    delete apacMemeBloc__m_0;
  } else if (true) {
    int *const apacMemeBloc__m_1 = new int();
    int &m = *(apacMemeBloc__m_1);

    delete apacMemeBloc_n_0;
    delete apacMemeBloc__m_1;
  }

  else {
    int *const apacMemeBloc__m_2 = new int();
    int &m = *(apacMemeBloc__m_2);

    delete apacMemeBloc_n_0;
    delete apacMemeBloc__m_2;
  }
  delete apacMemeBloc_n_0;
}