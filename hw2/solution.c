#include <stdio.h>

int main(int argc, char *argv[]) {
  int cs;
  long long a, b;
  int i;
  scanf("%d", &cs);
  for(i = 0; i < cs; i++){
    scanf("%lld%lld", &a, &b);
    printf("%lld\n", a+b);
  }
  return 0;
}
