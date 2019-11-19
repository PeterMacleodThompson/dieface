/*  calibtest.c  -  generate circle of data in /usr/share/pmt/calibraw360
 *    based on calibration parameters in /usr/share/pmt/calibdata
 * compile with
 * arm-linux-gnueabihf-gcc -o bin/calibtest  src/calibtest.c  ../pmtfxosd/src/fxosdriver.c  -I ../include/ -L /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm -li2c
 *
 *
 */

#include "pmtfxos.h" /* for SRAWDATA */
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
double xrotate(int, double, double);
double yrotate(int, double, double);

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

/**
 * rotate single point (x,y) θ degrees to to (x',y')
 * x′=xcosθ−ysinθ
 * y′=ycosθ+xsinθ
 */
double xrotate(int theta, double x, double y) {
  return (x * cos(theta) - y * sin(theta));
}

double yrotate(int theta, double x, double y) {
  return (y * cos(theta) + x * sin(theta));
}


int main() {
  FILE *fpcalib;  /* /usr/share/pmt/calibdata record */
  FILE *fpsample; /* /usr/share/pmt/calibraw360 sample data */
  char inbuf[400];
  SRAWDATA pAccelData;
  SRAWDATA pMagnData;
  calibmagnet calib;
  int i;
  int x, y;
  int xavg, yavg;
  int xmax, ymax, xmin, ymin;
  int distance, distancemax, distancemin;
  int theta1, theta2;
  int samplesize;
  int err;

  initfxos();

  /* load calibration parameters from calibdata */
  fpcalib = fopen("/usr/share/pmt/calibdata", "r");
  if (fpcalib == NULL) {
    printf("/usr/share/pmt/calibdata file not found - must do hard iron calibration 1st \n");
    exit(1);
  }
  fgets(inbuf, sizeof(inbuf), fpcalib);
  sscanf(inbuf, "%d %d %d %lf", &calib.hardx, &calib.hardy,
         &calib.softdeg, &calib.softx);
  fclose(fpcalib);

  /* get sample size to read */
  printf("Welcome to Calibration Test of PeterMacleodThompson sensor-board\n");
  printf("Enter sample size to read from sensor-board-fxos8700 (default 1000):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%d", &samplesize);
  if (err == EOF)
    samplesize = 1000;
  printf("Now creating a 2 dimensional circle of magnetic datapoints\n");
  printf("Place device on flat level surface \n");
  printf("prepare to rotate device 360 degrees each half-minute\n");
  printf("keep device flat and level during rotation\n");
  printf("About 2 rotations expected during collection of 1000 datapoints\n");
  printf("Press ENTER when ready:");
  getchar();

  fpsample = fopen("/usr/share/pmt/calibraw360", "w");
  /* get & calibrate raw uTesla readings */
  for (i = 0; i < samplesize; i++) {
    ReadAccelMagnData(fd, &pAccelData, &pMagnData);
    /* hard iron calibration */
    x = pMagnData.x - calib.hardx;
    y = pMagnData.y - calib.hardy;
    /* soft iron calibration */
    x = xrotate(calib.softdeg, x, y);  /* align ellipse on x-axis */
    x = x * calib.softx;         /* scale x-axis to circle */
    x = xrotate(-calib.softdeg, x, y); /* de-rotate back*/

    /* save data */
    fprintf(fpsample, " %d  %d \n", x, y);
    printf("\r sampling %4d of %4d", i, samplesize);
    fflush(stdout);
    usleep(25000); /* =40Hz because 1/40 = 25000 micro-sec */
  }

  /* finished - close and print results */
  fclose(fpsample);

  printf("\n\nCalibration Test complete\n");
  printf("calibration parameters in /usr/share/pmt/calibdata: \n");
  printf("  hard-iron x,y = %d, %d\n", calib.hardx, calib.hardy);
  printf("  soft-iron degrees, scale-x = %d, %lf\n",
      calib.softdeg, calib.softx);
  printf("\nTo view results:  \n");
  printf("copy/paste data from /usr/share/pmt/calibraw360 to spreadsheet\n");
  printf(" to create an x-y scatter chart\n");
  printf("If chart is not a circle centered on (0,0)\n");
  printf("Adjust parameters in /usr/share/pmt/calibdata as follows:\n");
  printf("Change hard-iron (1st 2 parameters) to center ellipse on (0,0) \n");
  printf("Change soft-iron (last 2 parameters) to:  \n");
  printf("  rotate ellipse to align on x-axis\n");
  printf("  scale ellipse x axis to be a circle\n");
  printf("\nsee README or pmtfxos.h calibmagnet for details\n");

  exit(0);
}
