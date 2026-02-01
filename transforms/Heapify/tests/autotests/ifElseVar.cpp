int main() {
  {
    int i = 1;
    if (i) {

      i++;
    } else {

      i++;
    }
  }

  if (true) {
    ;
  } else {
    int i = 1;
    if (i) {
      ;
    } else {
      i++;
    }
  }
  {
    int h = 1;
    int j = h + 1;
    if (j) {
      ;
    } else {
      int h = 1;
      int j = 4 + h;
      if (j) {
        h++;
      } else {
        j = 5;
      }
    }
  }
}
