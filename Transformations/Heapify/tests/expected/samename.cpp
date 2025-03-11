int main() {
  int *const apacMemeBloc__n_0 = new int(5);
  int &n = *(apacMemeBloc__n_0);
  ;
  {
    int *const apacMemeBloc__n_1 = new int(1);
    int &n = *(apacMemeBloc__n_1);
    ;
    delete apacMemeBloc__n_1;
  }
  delete apacMemeBloc__n_0;
}