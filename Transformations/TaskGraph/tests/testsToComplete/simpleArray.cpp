
#include <vector>

int main() {
  /*
  int a;
  a=4;
  int *p;
  p=new int[10];
  p[0]=4;
  p [0]=7;
  p[1]=a;
  int b;
  b=p[0];
  */
  int a;
  a = 5;
  std::vector<int> vec;
  vec.push_back(4);
  int b;
  b = vec[0];
  vec[1] = a;
  vec[0] = 5;
  vec[0] = 4;
}