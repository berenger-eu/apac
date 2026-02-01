int main() {
  int v1;
  int v1Init = 5;
  char v2 = 'c';
  int *v3;
  int *v3Init = &v1;
  int &v4 = v1;
  const int v5 = 4;
  unsigned long v6 = 5;
  volatile int v7 = 7;
  int **const *v8;
  int ***v9;
  int ***&v10 = v9;
}