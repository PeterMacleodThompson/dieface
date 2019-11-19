/**
 * DOC: --  ecompass.c  -- Electronic Compass from FXOS8700 magnetometer
 * readings Peter Thompson   -- May 2019
 *
 * Nova Scotia Solution.
 * fxos8700 magnetometer is noisy. Noise is redeced via taking the average
 * of the last several readings.
 * FXOS8700 magnetic data is output as 999.9 uTesla(uT) = 9.999 Gauss
 * Earth's magnetic field is 25-65 uTesla or .2-.6 Gauss
 * cross-compile with:
 * arm-linux-gnueabihf-gcc -o ecompassARM  ecompass.c  fxosdriver.c average.c -L
 * /home/peter/bbb2018/buildroot/output/target/usr/lib  -lm -li2c
 */

#include "pmtfxos.h" /* for SRAWDATA, calibmagnet */
#include <math.h>    /* for atan2(), sqrt() */
#include <stdio.h>
#include <stdlib.h>   /* for FILE ops */
#include <sys/time.h> /* for struct timeval */
#include <syslog.h>
#include <unistd.h> /* for usleep() */

#define CALIB_ERROR -1

/*   #define MAINFORTESTING */

#ifdef MAINFORTESTING
#define I2C_ERROR -1
#define I2C_OK 0
int i2cOpen();
void i2cClose();
int initFXOS8700(int);
#endif

#define REJECT -999
#define FAILURE 0
#define SUCCESS 1

/* function prototypes */
int ReadAccelMagnData(int, SRAWDATA *, SRAWDATA *);
double xrotate(int, double, double);
double yrotate(int, double, double);
void stackpushx(int);
void stackpushy(int);
int stackavgx(void);
int stackavgy(void);

static calibmagnet calib;

/**
 * ecompass() - read fxos8700 magnetometer uTesla and convert to degrees
 *   using hard iron (hi)  and soft iron (si) calibration numbers
 * @fd file descriptor for i2c read/write
 *
 * Return: integer degrees 0-360
 */

int ecompassinit(void) {
  FILE *fp;
  char inbuf[400];
  /* get calibration numbers */
  fp = fopen("/usr/share/pmt/calibdata", "r");
  if (fp == NULL) {
    /* return with error */
    syslog(LOG_NOTICE, "/usr/share/pmt/calibdata file not found \n");
    return (FAILURE);
  }
  fgets(inbuf, 400, fp);
  sscanf(inbuf, "%d %d %d %lf %d %d %lf %lf", &calib.hardx, &calib.hardy,
         &calib.softdeg, &calib.softx);
  return (SUCCESS);
}

int ecompass(int fd) {
  SRAWDATA pAccelData;
  SRAWDATA pMagnData;
  double degrees;
  int vari;
  struct point {
    int x;
    int y;
  } tesla;
  int avgx, avgy;

  /* get & calibrate raw uTesla readings */
  ReadAccelMagnData(fd, &pAccelData, &pMagnData);
  /* hard iron calibration */
  tesla.x = pMagnData.x - calib.hardx;
  tesla.y = pMagnData.y - calib.hardy;
  if (calib.softdeg != 0 || calib.softx != 1.0) {
    /* soft iron calibration */
    tesla.x = xrotate(calib.softdeg, tesla.x,
                      tesla.y);      /* rotate x to align ellipse on x-axis */
    tesla.x = tesla.x * calib.softx; /* scale x-axis */
    tesla.x = xrotate(-calib.softdeg, tesla.x,
                      tesla.y); /* de-rotate x back to original position */
  }

  /* noise reduction via averages */
  stackpushx(tesla.x);
  stackpushy(tesla.y);
  avgx = stackavgx() + 0.5;
  avgy = stackavgy() + 0.5;

  /* convert to degrees & return */
  degrees = atan2(avgy, avgx) * 57.2957795; /* 180/pi = 57.295...) */
  if (degrees < 0)
    degrees = degrees + 360.0;
  return ((int)(degrees + 0.5));

  /* not used deleteme   if( abs(degrees - avgdeg) > calib.sddegree ) */
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

#ifdef MAINFORTESTING
/* main for testing */
void main(void) {
  struct timeval start, now;
  long elapsed;
  int playtime; /* seconds */
  char input[20];
  int fd;
  int degrees;
  int rejects, cnt;

  printf(" initiate i2c-1 bus and device=0x1e   \n");
  if ((fd = i2cOpen()) == FAILURE) {
    printf("i2c failed to open - check syslog \n");
    exit(0);
  }

  if ((initFXOS8700(fd)) == I2C_ERROR) {
    printf("FXOS8700 not found - check syslog \n");
    exit(0);
  }
  if (ecompassinit() == FAILURE) {
    printf("FXOS8700 not found - check syslog \n");
    exit(0);
  }
  printf("Calibration numbers - see Nova Scotia Solution for explanation\n");
  printf(" hardx=%d\n hardy=%d\n softdeg=%d\n softx=%lf\n", calib.hardx,
         calib.hardy, calib.softdeg, calib.softx);

  while (1) {
    rejects = cnt = 0;
    /* get playtime from user */
    printf("Enter playtime in seconds 600=10min (0 to exit):");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &playtime);
    printf("playtime = %d \n", playtime);
    if (playtime == 0)
      break;
    gettimeofday(&start, NULL); /* start the clock */

    while (1) {
      degrees = ecompass(fd);
      cnt++;
      if (degrees == REJECT) {
        rejects++;
        continue;
      }
      printf("\r degrees = %d", degrees);
      fflush(stdout);
      usleep(25000); /* =40Hz because 1/40 = 25000 micro-sec */

      /* time to quit? */
      gettimeofday(&now, NULL);
      elapsed = now.tv_sec - start.tv_sec;
      if ((int)elapsed > playtime)
        break;
    }
    printf("\n\n\nfxos8700 readings %d\n", cnt);
    printf("rejected %d which is %f percent\n", rejects,
           ((double)rejects / (double)cnt) * 100.0);
  }
  printf("\nexiting play()\n");
  i2cClose(fd);
  exit(0);
}

#endif
