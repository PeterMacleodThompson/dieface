/**
 * DOC: -- gpsrun.c -- places gps data on shared memory
 * Peter Thompson -- Jan 2019
 *
 * There are 3 parts to this program
 *  1- shared memory code copied from
 *      http://logan.tw/posts/2018/01/07/posix-shared-memory/
 *
 *  2- SIGTERM code copied from:
 *      https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/
 *        and explained in:
 *      http://www.alexonlinux.com/signal-handling-in-linux
 *
 *  3- calls to linxdriver.c to obtain gps data from Linx R4 gps device
 */
/* #define MAINFORTESTING /* enable/disable testing with main() below */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* for mmap */
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>
#include <syslog.h>

#include "pmtgps.h"    /* for NMEA sentences and PMT,Linx structures */
#define NAME "/pmtgps" /*  for shared memory /dev/shm/pmtgps */
#define SIZE (sizeof(struct PMTgps))
#define LONG 0
#define LAT 1
#define TRUE 1
#define FALSE 0

/* function prototypes */
int linxinit(void);
struct linxdata *linxread(void);
void linxclose(void);
void simulate(struct PMTgps *, int);

/**
 *DDtoDMS - convert decimal degree longitude/latitude to degree minute
 *second
 * @dd decimal degrees
 * @longlat flags whether dd is longitude or latitude
 *
 * algorithm: integral part of (fractional part of dd * 60.0) = minutes.
 *  repeat for seconds.
 *
 * Return: pointer to dms structure
 */
struct dms *DDtoDMS(double dd, int longlat) {
  static struct dms ddmmss;
  double minutes;

  if (dd < 0.0 && longlat == LONG)
    ddmmss.nsew = 'W';
  else if (dd >= 0.0 && longlat == LONG)
    ddmmss.nsew = 'E';
  else if (dd < 0.0 && longlat == LAT)
    ddmmss.nsew = 'S';
  else if (dd >= 0.0 && longlat == LAT)
    ddmmss.nsew = 'N';

  dd = fabs(dd);
  ddmmss.degrees = (int)dd;
  minutes = (dd - (double)ddmmss.degrees) * 60.0;
  ddmmss.minutes = (int)minutes;
  ddmmss.seconds = (int)((minutes - (double)ddmmss.minutes) * 60.0);

  return &ddmmss;
}

volatile sig_atomic_t stopd = 0;

/**
 * terminate()   SIGTERM routine to stop daemon
 * stops pmtgps() infinite loop by setting stopd=1
 * Return: nothing
 */
void terminate(int signum) {
  stopd = 1;
  syslog(LOG_NOTICE, "SIGTERM received for pmtgpsd\n");
}

/**
 * pmtgps() -- continuously posts gps data to shared memory
 * Return: nothing
 */
void pmtgps(void) {
  struct PMTgps *gpsnow;   /* shared memory structure */
  struct linxdata *linx;   /* gps device Linx R4 data */
  struct sigaction action; /*SIGTERM for daemon stop */
  struct dms *ddmmss;      /* declared in DDtoDMS()  */
  int fd;
  char err[100];
  int gpsworks; /* flag: simulation vs Linx R4 device */

  /* setup for SIGTERM */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = terminate;
  if (sigaction(SIGTERM, &action, NULL) < 0) {
    sprintf(err, "sigaction error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  }

  /* setup for shared memory /dev/shm/pmtgps */
  fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd < 0) {
    sprintf(err, "shm_open error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return;
  }

  ftruncate(fd, SIZE);
  gpsnow = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /* initialize shared-memory to NULL ISLAND, Linx chip is slow */
  simulate(gpsnow, NULL_ISLAND);

  /* initialize Linx R4 device */
  if (linxinit())
    gpsworks = TRUE;
  else
    gpsworks = FALSE;

  /**** START THE BIG LOOP ****/
  while (!stopd) {
    if (!gpsworks)
      simulate(gpsnow, PERCY_LAKE);
    else {
      linx = linxread(); /* gps device data */
      /* reformat linx data to gpsnow data */
      ddmmss = DDtoDMS(linx->longitude, LONG);
      gpsnow->longitude =
          ddmmss->degrees * 10000 + ddmmss->minutes * 100 + ddmmss->seconds;
      gpsnow->longitudeEW = ddmmss->nsew;
      ddmmss = DDtoDMS(linx->latitude, LAT);
      gpsnow->latitude =
          ddmmss->degrees * 10000 + ddmmss->minutes * 100 + ddmmss->seconds;
      gpsnow->latitudeNS = ddmmss->nsew;
      gpsnow->altitude = linx->altitude * 3.28084; /* convert meters to feet */
      gpsnow->declination = linx->declination;
      gpsnow->speed = linx->speed;
      gpsnow->track = linx->track;
      gpsnow->date = linx->date;
      gpsnow->gmt = linx->gmt;
      gpsnow->solartime = gpsnow->longitude / 150000 + gpsnow->gmt;
      gpsnow->meridianlong = (int)linx->longitude;
      gpsnow->meridiantime = gpsnow->meridianlong + gpsnow->gmt % 10000;
    }
#ifdef MAINFORTESTING
    printf(" hello world \n");
    printf(
        "long=%d %c, lat=%d %c, alt=%dft, decl=%f, speed=%dknots, track=%d \n",
        gpsnow->longitude, gpsnow->longitudeEW, gpsnow->latitude,
        gpsnow->latitudeNS, gpsnow->altitude, gpsnow->declination,
        gpsnow->speed, gpsnow->track);
    printf("date=%d, gmt=%d, solartime=%d, meridianlong=%d, meridiantime=%d \n",
           gpsnow->date, gpsnow->gmt, gpsnow->solartime, gpsnow->meridianlong,
           gpsnow->meridiantime);
#endif
  }
  /**** END THE BIG LOOP ****/

  munmap(gpsnow, SIZE);
  close(fd);
  shm_unlink(NAME);
  syslog(LOG_NOTICE, "stopping pmtgpsd %d ", getpid());
  return;
}

#ifdef MAINFORTESTING
int main() {
  printf("calling pmtgps\n");
  pmtgps();
  printf("exiting pmtgps\n");
  return 0;
}
/* compile instructions see ../../Cross Compile Instructions */

#endif
