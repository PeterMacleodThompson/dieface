/**
 * DOC: --  calibrate.c  -- Calibrate Magnetometer
 * Peter Thompson   -- May 2019, Nov 2019
 *
 * Nova Scotia Solution
 *
 https://www.sensorsmag.com/components/compensating-for-tilt-hard-iron-and-soft-iron-effects
 *
 * FXOS8700 magnetic data is output as 999.9 uTesla(uT) = 9.999 Gauss
 * Earth's magnetic field is 25-65 uTesla or .2-.6 Gauss
 * cross-compile with:
 *  arm-linux-gnueabihf-gcc -o bin/calibrate  src/calibrate.c ../pmtfxosd/src/fxosdriver.c -I ../include/ -L /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm -li2c

 *
 */

#include "pmtfxos.h" /* for SRAWDATA */
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
static calibmagnet calib;
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

void printmean(void) {
  printf("\nMean of each Dataset 999.9 uTesla\n");
  printf("--------------------------------------\n");
  printf("calibrawN NORTH ==> zero-X max-Y \n");
  printf("calibrawE EAST  ==> max-X  zero-Y\n");
  printf("calibrawS SOUTH ==> zero-X min-Y \n");
  printf("calibrawW WEST  ==> min-X  zero-Y\n");
  printf("------------------------\n");
  printf("calibrawN NORTH %5d %5d\n", mean[0][0], mean[0][1]);
  printf("calibrawE EAST  %5d %5d\n", mean[1][0], mean[1][1]);
  printf("calibrawS SOUTH %5d %5d\n", mean[2][0], mean[2][1]);
  printf("calibrawW WEST  %5d %5d\n", mean[3][0], mean[3][1]);
  printf("zero-X should match, zero-Y should match.\n");
  printf("zero-X, zero-Y == hard iron calibration\n");
  return;
}

void printstdev(int avgstdev) {
  printf("\nStandard Deviation of each Dataset 999.9 uTesla\n");
  printf("--------------------------------------------------\n");
  printf("calibrawN NORTH %5d %5d\n", stdev[0][0], stdev[0][1]);
  printf("calibrawE EAST  %5d %5d\n", stdev[1][0], stdev[1][1]);
  printf("calibrawS SOUTH %5d %5d\n", stdev[2][0], stdev[2][1]);
  printf("calibrawW WEST  %5d %5d\n", stdev[3][0], stdev[3][1]);
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
  char yesno;
  int rejects;
  int err;
  double stdevfactor;

  initfxos();

  printf("Welcome to Calibration for PeterMacleodThompson sensor-board\n");
  printf("Enter sample size to get from sensor fxos8700 (default 1000):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%d", &samplesize);
  if (err == EOF)
    samplesize = 1000;
  printf("samplesize = %d\n\n", samplesize);
  printf("4 files will be replaced: /usr/share/pmt/calibrawN..E..S..W  - ok? (y/n):");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%c", &yesno);
  if(yesno != 'y')
    exit(1);

  /* get samples, calc and save their means */
  getsample("/usr/share/pmt/calibrawN", "facing NORTH", 0);
  getsample("/usr/share/pmt/calibrawE", "facing EAST", 1);
  getsample("/usr/share/pmt/calibrawS", "facing SOUTH", 2);
  getsample("/usr/share/pmt/calibrawW", "facing WEST", 3);

  /* extract standard deviation from samples & save */
  if (getstdev("/usr/share/pmt/calibrawN", 0) == FAILURE)
    exit(1);
  if (getstdev("/usr/share/pmt/calibrawE", 1) == FAILURE)
    exit(1);
  if (getstdev("/usr/share/pmt/calibrawS", 2) == FAILURE)
    exit(1);
  if (getstdev("/usr/share/pmt/calibrawW", 3) == FAILURE)
    exit(1);

  /* raw data printout */
  printf("\n\nRAW DATA\n");
  printf("------------\n");
  printmean();


  /*  hard iron calibration */
  printf("calibration file: /usr/share/pmt/calibdata will be replaced!\n");
  printf("current calibration data will be lost. ok? (y/n)");
  fgets(inbuf, sizeof(inbuf), stdin);
  err = sscanf(inbuf, "%c", &yesno);
  if(yesno != 'y')
    exit(1);
  calib.hardx = hardcalc(0);
  calib.hardy = hardcalc(1);
  calib.softdeg = 0;  /* default no soft iron calibration */
  calib.softx = 1.0;
  fp = fopen("/usr/share/pmt/calibdata", "w");
  fprintf(fp, "%d %d %d %lf", calib.hardx, calib.hardy,
          calib.softdeg, calib.softx);
  printf("\n\n\nCalibration Complete, new record written\n");
  printf("   into... /usr/share/pmt/calibdata\n");
  printf("Your new calibration data is below:\n");
  printf("NEW: hard-iron x,y = %d, %d\n", calib.hardx, calib.hardy);
  printf("DEFAULT: soft-iron degrees, scale-x = %d, %lf\n",
      calib.softdeg, calib.softx);
  printf("NOTE hard-iron calibration (above) may be good enough - try it\n"); 
  printf("     soft-iron calibration defaults to no calibration: 0, 1.0\n"); 
  printf("\n\nFurther calibration is manual using calibtest\n");
  printf("calibtest creates /usr/share/pmt/calibraw360\n");
  printf("cut/paste calibraw360 to spreadsheet for x-y scatter chart\n");
  printf("adjust /usr/share/pmt/calibdata until circle on (0,0) is charted\n");
  fclose(fp);
  exit(0);
}
