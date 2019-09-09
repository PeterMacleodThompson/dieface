/**
 * DOC: --  getdieface.c  -- get deface orientation from fxos8700 accelerometer
 *  Peter Thompson   -- Sept 2019
 *
 * Nova Scotia Solution.
 * z-axis = 1+,6-
 * y-axis = 2+,5-
 * x-axis = 3+,4-
 * cross-compile with:
 * arm-linux-gnueabihf-gcc -o ecompassARM  ecompass.c  fxosdriver.c average.c -L
 * /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm -li2c
 *
 *
 *
 */

#include "pmtfxos.h" /* for SRAWDATA */
#include <stdlib.h>  /* for abs() */

#define ONE 1
#define TWO 2
#define THREE 3
#define FOUR 4
#define FIVE 5
#define SIX 6

/* function prototypes */
int ReadAccelMagnData(int, SRAWDATA *, SRAWDATA *);

int getdieface(int fd) {
  SRAWDATA pAccelData;
  SRAWDATA pMagnData;
  int x, y, z;

  /* get accelerometer readings */
  ReadAccelMagnData(fd, &pAccelData, &pMagnData);
  x = pAccelData.x;
  y = pAccelData.y;
  z = pAccelData.z;

  /* compute and return dieface */
  if (abs(x) > abs(y) && abs(x) > abs(z)) {
    if (x > 0)
      return (FOUR);
    else
      return (THREE);
  }

  if (abs(y) > abs(x) && abs(y) > abs(z)) {
    if (y > 0)
      return (TWO);
    else
      return (FIVE);
  }

  if (abs(z) > abs(x) && abs(z) > abs(y)) {
    if (z > 0)
      return (ONE);
    else
      return (SIX);
  }

  /* should never reach here */
  return (-9);
}
