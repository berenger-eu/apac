int funcMock(int a) { return a + 1; }

int funcRec(int a) {
  if (a == 0) {
    return 0;
  } else {
    int result = funcRec(a - 1);
    result = result + funcMock(a - 1);
    return result;
  }
}