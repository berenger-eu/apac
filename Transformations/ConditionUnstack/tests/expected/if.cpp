int main() {
  {
    int j = 0;
    if (j == 0) {
      ;
    }
  }

  {
    int j = 0;
    int i = j + 1;
    if (i) {
      ;
    }
  }
}