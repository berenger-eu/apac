int main() {
  {
    int *const apacMemeBloc__i_0 = new int(1);
    int &i = *(apacMemeBloc__i_0);
    if (i)
      i++;
    else
      i++;
    delete apacMemeBloc__i_0;
  };

  if (true)
    ;
  else {
    int *const apacMemeBloc__i_1 = new int(1);
    int &i = *(apacMemeBloc__i_1);
    if (i)
      ;
    else
      i++;
    delete apacMemeBloc__i_1;
  };

  {
    int *const apacMemeBloc__h_0 = new int(1);
    int &h = *(apacMemeBloc__h_0);
    int *const apacMemeBloc__j_0 = new int(h + 1);
    int &j = *(apacMemeBloc__j_0);
    if (j) {
      ;
    } else {
      int *const apacMemeBloc__h_1 = new int(1);
      int &h = *(apacMemeBloc__h_1);
      int *const apacMemeBloc__j_1 = new int(4 + h);
      int &j = *(apacMemeBloc__j_1);
      if (j)
        h++;
      else
        j = 5;
      delete apacMemeBloc__h_1;
      delete apacMemeBloc__j_1;
    };
    delete apacMemeBloc__h_0;
    delete apacMemeBloc__j_0;
  };
}