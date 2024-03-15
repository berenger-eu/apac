#include <stdio.h>
//To see if stderr breaks the program or not
int main()
{
  const int  v=5;
  fprintf(stderr, "Error: array is not sorted\n");
}