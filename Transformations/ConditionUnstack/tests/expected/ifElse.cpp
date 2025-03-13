int main() {
  {
    int i = 1;
    if (i) {
      i++;
    } else {
      int j = 2;
      int k = 3;
      if (k) {
        i++;
      } else {
        j++;
      }
    }
  }
}