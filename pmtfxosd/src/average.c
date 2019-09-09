/*  stack for moving average calculation
  compile with gcc -o stack stack.c
*/

/*  #define MAINFORTESTING  */

#include <stdio.h>
#define STACKSIZE 40

static int stackx[STACKSIZE]; /* static initialize to zero by C */
static int stacky[STACKSIZE]; /* static initialize to zero by C */

void stackpushx(int value) {
  int i;
  for (i = STACKSIZE - 1; i > 0; i--)
    stackx[i] = stackx[i - 1];
  stackx[0] = value;
  return;
}

void stackpushy(int value) {
  int i;
  for (i = STACKSIZE - 1; i > 0; i--)
    stacky[i] = stacky[i - 1];
  stacky[0] = value;
  return;
}

int stackavgx() {
  double sum = 0.0;
  int err = 0;
  int i;

  for (i = 0; i < STACKSIZE; i++) {
    sum += stackx[i];
    if (stackx[i] == 0.0)
      err++;
  }
  return ((int)(sum / (double)(STACKSIZE - err) + 0.5));
}

int stackavgy() {
  double sum = 0.0;
  int err = 0;
  int i;

  for (i = 0; i < STACKSIZE; i++) {
    sum += stacky[i];
    if (stacky[i] == 0.0)
      err++;
  }
  return ((int)(sum / (double)(STACKSIZE - err) + 0.5));
}

#ifdef MAINFORTESTING
int main() {
  int i;
  for (i = 0; i < 30; i++) {
    stackpushx((double)(i));
    printf("stack= %d %d %d %d %d \n", stackx[0], stackx[1], stackx[2],
           stackx[3], stackx[4]);
    printf("i=%d, avg = %d \n", i, stackavgx());
  }
  return (0);
}
#endif
