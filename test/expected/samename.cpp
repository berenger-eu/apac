int main() {
  int *const apacMemeBloc__n_0 = new int(5);
  int &n = *(apacMemeBloc__n_0);

  if (true) {
    int *const apacMemeBloc__n_1 = new int(1);
    int &n = *(apacMemeBloc__n_1);

    delete apacMemeBloc__n_0;
    delete apacMemeBloc__n_1;
    return 1;
  }
}