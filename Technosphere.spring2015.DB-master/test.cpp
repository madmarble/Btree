#include <stdio.h>
#include <string.h>

int main ()
{
  char buffer1 = 156;
  char buffer2 = 200;

  int n;
  void *a = &buffer1;
  void *b = &buffer2;
  n=memcmp ( a, b, 1);

  printf("%d\n", n);
  return 0;
}