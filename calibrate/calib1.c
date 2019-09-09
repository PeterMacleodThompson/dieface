/**
 * DOC: --  calibv2.c  -- Calibrate Magnetometer
 * Peter Thompson   -- May 2019
 *
 * Nova Scotia Solution
 *
 https://www.sensorsmag.com/components/compensating-for-tilt-hard-iron-and-soft-iron-effects
 *
 * FXOS8700 magnetic data is output as 999.9 uTesla(uT) = 9.999 Gauss
 * Earth's magnetic field is 25-65 uTesla or .2-.6 Gauss
 * cross-compile with:
 *  arm-linux-gnueabihf-gcc -o calibrate1  calib1.c ../pmtfxosd/src/fxosdriver.c
 -I../pmtfxosd/src/ -L /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm
 -li2c

 *
 */

#include "include/pmtfxos.h" /* for SRAWDATA */
#include <math.h>            /* for pow() */
#include <stdint.h>          /* for uint_8 */
#include <stdio.h>
#include <stdlib.h> /* for exit() */
#include <string.h>
#include <sys/time.h> /* for struct timeval */
#include <unistd.h>   /* for usleep() */

#define FAILURE 0
#define SUCCESS 1
#define I2C_ERROR -1
#define I2C_OK 0

/* function prototypes */
int ReadAccelMagnData(int, SRAWDATA *, SRAWDATA *);
int i2cOpen();
void i2cClose();
int initFXOS8700(int);

static int fd;
static int mean[4][2];  /* mean of 0, 90, 180, 270 datasets for x,y */
static int stdev[4][2]; /* std deviation of 0, 90, 180, 270 datasets for x,y */
static CALIBMAGNET calib;
static int xradius, yradius;
static int samplesize;

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

void getsample(char *filename, char *instructions, int index) {
  FILE *fp;
  SRAWDATA pAccelData;
  SRAWDATA pMagnData;
  int i;
  int xsum, ysum;

  xsum = ysum = 0;

  fp = fopen(filename, "w");
  printf("\nPlace device on flat level surface %s\n", instructions);
  printf("Press ENTER when ready:");
  getchar();

  for (i = 0; i < samplesize; i++) {
    ReadAccelMagnData(fd, &pAccelData, &pMagnData);
    fprintf(fp, " %d  %d \n", pMagnData.x, pMagnData.y);
    xsum += pMagnData.x;
    ysum += pMagnData.y;
    printf("\r sampling %4d of %4d", i, samplesize);
    fflush(stdout);
    usleep(25000); /* =40Hz because 1/40 = 25000 micro-sec */
  }
  mean[index][0] = xsum / samplesize; /* x */
  mean[index][1] = ysum / samplesize; /* y */
  fclose(fp);
  return;
}

int getstdev(char *filename, int index) {
  FILE *fp;
  char inbuf[400];
  int x, y;
  double xsd, ysd;
  int i;

  xsd = ysd = 0.0;
  fp = fopen(filename, "r");
  if (fp == NULL) {
    /* return with error */
    printf("calibration file %s not created\n", filename);
    printf("Calibration Failed, exiting\n");
    return (FAILURE);
  }
  for (i = 0; i < samplesize; i++) {
    fgets(inbuf, sizeof(inbuf), fp);
    sscanf(inbuf, "%d %d ", &x, &y);
    /* sum squares of (x-mean) for std dev */
    xsd += pow((x - mean[index][0]), 2);
    ysd += pow((y - mean[index][1]), 2);
  }
  stdev[index][0] = (int)sqrt(xsd / (double)samplesize);
  stdev[index][1] = (int)sqrt(ysd / (double)samplesize);
  fclose(fp);
  return (SUCCESS);
}

int scrubdata(char *filename, int index, double sdfactor) {
  FILE *fpin, *fpout;
  char newfile[200];
  char inbuf[400];
  int x, y;
  int xsum, ysum;
  double distance;
  int rejects;
  int i;

  xsum = ysum = rejects = 0;
  fpin = fopen(filename, "r");

  strcpy(newfile, filename);
  strcat(newfile, "scrub");
  fpout = fopen(newfile, "w");
  for (i = 0; i < samplesize; i++) {
    fgets(inbuf, sizeof(inbuf), fpin);
    sscanf(inbuf, "%d %d ", &x, &y);
    /* distance between 2 points */
    distance =
        sqrt(pow((x - mean[index][0]), 2) + pow((y - mean[index][1]), 2));
    if (distance > calib.sdtesla * calib.sdf) {
      rejects++;
      continue;
    }
    fprintf(fpout, " %d  %d \n", x, y);
    xsum += x;
    ysum += y;
  }
  mean[index][0] = xsum / (samplesize - rejects);
  mean[index][1] = ysum / (samplesize - rejects);
  return (rejects);
}

void printmean(void) {
  printf("\nMean of each Dataset 999.9 uTesla\n");
  printf("--------------------------------------\n");
  printf("fxmag0   NORTH ==> zero-X max-Y \n");
  printf("fxmag90  EAST  ==> max-X  zero-Y\n");
  printf("fxmag180 SOUTH ==> zero-X min-Y \n");
  printf("fxmag270 WEST  ==> min-X  zero-Y\n");
  printf("------------------------\n");
  printf("fxmag0   NORTH %5d %5d\n", mean[0][0], mean[0][1]);
  printf("fxmag90  EAST  %5d %5d\n", mean[1][0], mean[1][1]);
  printf("fxmag180 SOUTH %5d %5d\n", mean[2][0], mean[2][1]);
  printf("fxmag270 WEST  %5d %5d\n", mean[3][0], mean[3][1]);
  printf("zero-X should match, zero-Y should match.\n");
  printf("zero-X, zero-Y == hard iron calibration\n");
  return;
}

void printstdev(int avgstdev) {
  printf("\nStandard Deviation of each Dataset 999.9 uTesla\n");
  printf("--------------------------------------------------\n");
  printf("fxmag0   NORTH %5d %5d\n", stdev[0][0], stdev[0][1]);
  printf("fxmag90  EAST  %5d %5d\n", stdev[1][0], stdev[1][1]);
  printf("fxmag180 SOUTH %5d %5d\n", stdev[2][0], stdev[2][1]);
  printf("fxmag270 WEST  %5d %5d\n", stdev[3][0], stdev[3][1]);
  printf("Average Standard Deviation - all data ==> %d\n\n", avgstdev);
  return;
}

int stdevcalc(void) {
  int i, j, sum;

  sum = 0;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 2; j++)
      sum += stdev[i][j];
  return (sum / 8 + 0.5);
}

int hardcalc(int index) {
  int min, max, mid;
  int imin, imax, imid1, imid2;
  int radius;
  int i;

  max = -99999;
  min = 99999;
  /* get min and max */
  for (i = 0; i < 4; i++) {
    if (mean[i][index] < min) {
      min = mean[i][index];
      imin = i;
    }
    if (mean[i][index] > max) {
      max = mean[i][index];
      imax = i;
    }
  }
  /* get mid1 = 1st number between min and max */
  for (i = 0; i < 4; i++) {
    if (i == imax || i == imin)
      continue;
    imid1 = i;
  }
  /* get mid2 = 2nd number between min and max */
  for (i = 0; i < 4; i++) {
    if (i == imax || i == imin || i == imid1)
      continue;
    imid2 = i;
  }

  /* calculate average midpoint */
  radius = (mean[imax][index] - mean[imin][index]) / 2;
  if (index == 0)
    xradius = radius;
  else
    yradius = radius;

  /* calculate average midpoint */
  mid = ((mean[imax][index] - mean[imin][index]) / 2) + mean[imin][index];
  if (index == 0)
    printf("hardx calculation = (max-min)/2 +min = %d averaged with %d, %d\n",
           mid, mean[imid1][index], mean[imid2][index]);
  else
    printf("hardy calculation = (max-min)/2 +min = %d averaged with %d, %d\n",
           mid, mean[imid1][index], mean[imid2][index]);

  return ((mid + mean[imid1][index] + mean[imid2][index]) / 3);
}

/* MAIN FOR CALIBRATION */
int main(void) {
  FILE *fp;
  char inbuf[20];
  char action;
  int rejects;
  int err;
  double stdevfactor;

  initfxos();

  printf("Welcome to Calibration for PeterMacleodThompson ecompass board\n");
  printf("Enter sample size to get from fxos8700 (default 1000):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%d", &samplesize);
  if (err == EOF)
    samplesize = 1000;
  printf("samplesize = %d\n\n", samplesize);
  printf("Standard Deviation Factor follows 68-95-99.7 rule of statistics\n");
  printf("1=68%% 2=95%% 3=99.7%% inclusion of datapoints\n");
  printf("1 => more accurate, less responsive. 3 => less accurate\n");
  printf("Enter desired factor (1.0 is default):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%lf", &calib.sdf);
  if (err == EOF)
    calib.sdf = 1.0;
  printf("standard deviation factor for calibration = %f\n\n", calib.sdf);

  /* get samples, calc and save their means */
  getsample("fxmag0", "facing NORTH", 0);
  getsample("fxmag90", "facing EAST", 1);
  getsample("fxmag180", "facing SOUTH", 2);
  getsample("fxmag270", "facing WEST", 3);

  /* extract standard deviation from samples & save */
  if (getstdev("fxmag0", 0) == FAILURE)
    exit(1);
  if (getstdev("fxmag90", 1) == FAILURE)
    exit(1);
  if (getstdev("fxmag180", 2) == FAILURE)
    exit(1);
  if (getstdev("fxmag270", 3) == FAILURE)
    exit(1);

  /* raw data printout */
  calib.sdtesla = stdevcalc();
  printf("\n\nRAW DATA\n");
  printf("------------\n");
  printmean();
  printstdev(calib.sdtesla);

  /* remove outlier datapoints & redo */
  stdevfactor = calib.sdtesla * calib.sdf;
  rejects = scrubdata("fxmag0", 0, stdevfactor);
  printf("%d datapoints scrubbed =%f%%\n", rejects,
         ((double)rejects / samplesize) * 100.0);
  rejects = scrubdata("fxmag90", 1, stdevfactor);
  printf("%d datapoints scrubbed =%f%%\n", rejects,
         ((double)rejects / samplesize) * 100.0);
  rejects = scrubdata("fxmag180", 2, stdevfactor);
  printf("%d datapoints scrubbed =%f%%\n", rejects,
         ((double)rejects / samplesize) * 100.0);
  rejects = scrubdata("fxmag270", 3, stdevfactor);
  printf("%d datapoints scrubbed =%f%%\n", rejects,
         ((double)rejects / samplesize) * 100.0);
  /* scrubbed data printout */
  calib.sdtesla = stdevcalc();
  printf("\n\nSCRUBBED DATA\n");
  printf("------------\n");
  printmean();
  printstdev(calib.sdtesla);

  /*  hard iron calibration 1st draft */
  calib.hardx = hardcalc(0);
  calib.hardy = hardcalc(1);
  calib.theta = 0;
  calib.softx = 1.0;
  calib.rperfect = (xradius + yradius) / 2;
  calib.sddegree = atan2(calib.sdtesla, calib.rperfect);
  fp = fopen("calibdata", "w");
  fprintf(fp, "%d %d %d %lf %d %d %lf %lf", calib.hardx, calib.hardy,
          calib.theta, calib.softx, calib.rperfect, calib.sdtesla,
          calib.sddegree, calib.sdf);
  printf("\nHard Iron Calibration Complete\n");
  printf("hard x,y = %d, %d\n", calib.hardx, calib.hardy);
  printf("soft theta, x = %d, %lf\n", calib.theta, calib.softx);
  printf("radius of circle = %d\n", calib.rperfect);
  printf("standard deviation:  %d uTesla,  %f degrees,  %f factor\n",
         calib.sdtesla, calib.sddegree, calib.sdf);

  printf(" \ncircle radius x = %d, y = %d, rperfect = %d\n", xradius, yradius,
         (xradius + yradius) / 2);
  if (abs(xradius - yradius) > calib.sdf * calib.sdtesla) {
    printf("You have calibrated an ellipse, not a circle\n");
    printf("You must do step2 soft iron calibration to get a circle\n");
  }
  fclose(fp);
  exit(0);
}
