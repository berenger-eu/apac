int main() {
  {
    int(*apacMemeBloc__a_0)[2][2] = new int[2][2][2];
    int(*&a)[2][2] = (apacMemeBloc__a_0);
    ;
    int(*apacMemeBloc__b_1)[2][2] = new int[2][2][2]{1, 2, 3, 4, 5, 6, 7, 8};
    int(*&b)[2][2] = (apacMemeBloc__b_1);
    ;
    delete[] apacMemeBloc__a_0;
    delete[] apacMemeBloc__b_1;
  }
  return 1;
}