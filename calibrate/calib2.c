/*  calib2.c  -  soft iron calibration
 * compile with
 * arm-linux-gnueabihf-gcc -o calibrate2  calib2.c  ../pmtfxosd/src/fxosdriver.c
 * ../pmtfxosd/src/average.c  -I../pmtfxosd/src/ -L
 * /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm -li2c
 *
 *
 */

#include "include/pmtfxos.h" /* for SRAWDATA */
#include <math.h>            /* for sqrt */
#include <stdio.h>
#include <stdlib.h> /* for exit() */
#include <unistd.h> /* for usleep() */

#define FAILURE 0
#define SUCCESS 1
#define I2C_ERROR -1
#define I2C_OK 0

/* function prototypes */
int ReadAccelMagnData(int, SRAWDATA *, SRAWDATA *);
int initFXOS8700(int);
void stackpushx(int);
void stackpushy(int);
int stackavgx(void);
int stackavgy(void);
int i2cOpen();
void i2cClose();

static int fd;

void initfxos(void) {
  printf(" initiate i2c-1 bus and device=0x1e   \n");
  if ((fd = i2cOpen()) == FAILURE) {
    printf("i2c failed to open - check syslog \n");
    exit(0);
  }

  if ((initFXOS8700(fd)) == I2C_ERROR) {
    printf("FXOS8700 not found - check syslog \n");
    exit(0);
  }
  return;
}

int main() {
  FILE *fpcalib;  /* calibdata record */
  FILE *fpsample; /* fxmag360 sample data */
  char inbuf[400];
  SRAWDATA pAccelData;
  SRAWDATA pMagnData;
  CALIBMAGNET calib;
  int i;
  int x, y;
  int xavg, yavg;
  int xmax, ymax, xmin, ymin;
  int distance, distancemax, distancemin;
  int theta1, theta2;
  int samplesize;
  int err;

  initfxos();

  distancemax = -99999;
  distancemin = 99999;
  fpcalib = fopen("calibdata", "r");
  if (fpcalib == NULL) {
    printf("calibdata file not found - must do hard iron calibration 1st \n");
    exit(1);
  }
  fgets(inbuf, sizeof(inbuf), fpcalib);
  sscanf(inbuf, "%d %d %d %lf %d %d %lf %lf", &calib.hardx, &calib.hardy,
         &calib.theta, &calib.softx, &calib.rperfect, &calib.sdtesla,
         &calib.sddegree, &calib.sdf);
  fclose(fpcalib);

  /* get & calibrate raw uTesla readings */
  printf("Welcome to Soft Calibration of PeterMacleodThompson device\n");
  printf("Enter sample size to get from fxos8700 (default 1000):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%d", &samplesize);
  if (err == EOF)
    samplesize = 1000;
  printf("Now creating a 2 dimensional circle of magnetic datapoints\n");
  printf("Place device on flat level surface \n");
  printf("prepare to rotate device 360 degrees each minute\n");
  printf("keep device flat and level during rotation\n");
  printf("About 3 rotations expected during collection of 5000 datapoints\n");
  printf("Press ENTER when ready:");
  getchar();

  fpsample = fopen("fxmag360", "w");
  for (i = 0; i < samplesize; i++) {
    ReadAccelMagnData(fd, &pAccelData, &pMagnData);
    /* hard iron calibration */
    x = pMagnData.x - calib.hardx;
    y = pMagnData.y - calib.hardy;

    /* save data */
    fprintf(fpsample, " %d  %d \n", x, y);
    printf("\r sampling %4d of %4d", i, samplesize);
    fflush(stdout);
    usleep(25000); /* =40Hz because 1/40 = 25000 micro-sec */

    /* noise reduction via averages */
    stackpushx(x);
    stackpushy(y);
    xavg = stackavgx() + 0.5;
    yavg = stackavgy() + 0.5;

    /* check for ellipse */
    distance = sqrt(xavg * xavg + yavg * yavg);
    if (abs(distance - calib.rperfect) > calib.sdf * calib.sdtesla) {
      if (distance > distancemax) {
        xmax = xavg;
        ymax = yavg;
        distancemax = distance;
      } else if (distance < distancemin) {
        xmin = xavg;
        ymin = yavg;
        distancemin = distance;
      }
    }
  }
  fclose(fpsample);

  printf("\n\nradius = %d, max = %d, min = %d\n", calib.rperfect, distancemax,
         distancemin);
  printf("equates to max(x,y) = (%d, %d) min(x,y) = (%d, %d) \n", xmax, ymax,
         xmin, ymin);

  if (distancemax == -99999 && distancemin == 99999) {
    printf("We have a circle - no soft iron calibration\n");
    exit(0);
  } else {
    printf("We have an ellipse - soft iron calibration required\n");
    theta1 = atan2(ymax, xmax);
    theta2 = atan2(ymin, xmin);
    if (abs(theta1 - theta2) > 1.0 * calib.sdf) {
      printf("ERROR in soft calibration rotation %d degrees vs %d degrees\n",
             theta1, theta2);
      printf(" max = (%d, %d) min = (%d, %d) do not align\n", xmax, ymax, xmin,
             ymin);
      printf(" Exiting system, please calibrate manually or ignore\n");
    }
    calib.theta = theta1;
    calib.softx = xmin / xmax;
    fpcalib = fopen("calibdata", "w");
    fprintf(fpcalib, "%d %d %d %lf %d %d %lf %lf", calib.hardx, calib.hardy,
            calib.theta, calib.softx, calib.rperfect, calib.sdtesla,
            calib.sddegree, calib.sdf);
    printf("Hard &Soft Iron Calibration Complete\n");
    printf("hard x,y = %d, %d\n", calib.hardx, calib.hardy);
    printf("soft theta, x = %d, %lf\n", calib.theta, calib.softx);
    printf("radius of circle = %d\n", calib.rperfect);
    printf("standard deviation:  %d uTesla,  %f degrees,  %f factor\n",
           calib.sdtesla, calib.sddegree, calib.sdf);
    fclose(fpcalib);
    exit(0);
  }
}
