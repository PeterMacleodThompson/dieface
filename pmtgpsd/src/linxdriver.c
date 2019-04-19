/**
 * DOC:  -- linxdriver.c -- for linx R4 gps receiver device --
 *
 * Linx R4 gps device broadcasts NMEA sentences
 * on a UART (aka Serial Port)
 *   9600 baud,
 *   8 data bits,
 *   no parity,
 *   no hardware flow control,
 *   character buffering.
 */

#include "pmtgps.h"  /* for NMEA sentences and GPS structures */
#include <errno.h>   /* for error messages via errno */
#include <fcntl.h>   /* File control definitions */
#include <math.h>    /* fabs() */
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <syslog.h>  /* for syslog */
#include <termios.h> /* POSIX terminal control definitions */
#include <unistd.h>  /* UNIX standard function definitions */

#define SUCCESS 1
#define TRUE 1
#define FAILURE 0
#define FALSE 0
#define MAXCHAR 10000 /* max char to read looking for right NMEA sentence */
#define MOTIONLESS 0.0002778 /* 0.0002778 degrees = 1 second = 101 feet */

/* Function Prototypes */
void wmminit(void);
void wmmclose(void);
int ddmmyytoyyyymmdd(int);
double dmtodd(double);
double wmmdeclination(double, double, double, int, int, int);

static char err[100];
static struct linxdata gpslinx;
int fser;    /* File descriptor for serial port */
FILE *fpgps; /* File pointer for /var/log/pmtgpsd-rejects.log */

/**
 * linxinit() -- initialize UART, Null Island for linx R4 gps device --
 * Return: SUCCESS if initialize ok,  or FAILURE otherwise
 */
int linxinit(void) {
  struct termios options;

  /*   open serial port 1  */
  fser = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
  if (fser == -1) {
    sprintf(err, "linxinit(): Unable to open /dev/ttyS1 = %s\n",
            strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return (FAILURE);
  }

  /* get/set the  options for the port */
  tcgetattr(fser, &options);
  cfsetispeed(&options, B9600); /* baud rate 9600 */
  cfsetospeed(&options, B9600);
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB; /* Mask the character size to 8 bits, no parity */
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;      /* Select 8 data bits */
  options.c_cflag &= ~CRTSCTS; /* Disable hardware flow control */
  options.c_lflag &= ~(ICANON | ECHO | ISIG);
  /* Set the new options for the port */
  tcsetattr(fser, TCSANOW, &options);

  /* initiate gpslinx to Null Island */
  gpslinx.date = 0;        /* 999999 = ddmmyy */
  gpslinx.gmt = 0.0;       /* 999999.999 = UTC hh:mm:ss.sss */
  gpslinx.longitude = 0.0; /*  + => East,  - => West */
  gpslinx.latitude = 0.0;  /*  + => North, - => South */
  gpslinx.altitude = 0.0;  /* 9999.9 = altitude meters */
  gpslinx.declination =
      0.0;             /* to West = negative (Toronto), to East = positive */
  gpslinx.speed = 0.0; /* 999.99 = knots per hour */
  gpslinx.track = 0.0; /* 999.99 = track angle in degrees True */

  /* log file for rejected NMEA statements */
  fpgps = fopen("/var/log/pmtgpsd-rejects.log", "w");
  if (!fpgps) {
    sprintf(err, " %s\n", strerror(errno));
    syslog(LOG_NOTICE, "Unable to open /var/log/pmtgpsd-rejects.log = %s", err);
    return (FAILURE);
  }
  syslog(LOG_INFO, "Serial port /dev/ttyS1 successfully opened");
  return (SUCCESS);
}

/**
 * linxread() -- read data from linx R4 gps device --
 * Return: linxdata record or NULL if MAXCHAR read with no success
 */
struct linxdata *linxread(void) {

  static struct linxdata gpslinxsave;
  struct GPGGA gpgga;
  struct GPRMC gprmc;
  /* Flags showing new gps records received TRUE=1,FALSE=0 */
  int gpggaF, gprmcF;
  char chout;
  char buf[100];
  int i, j, num;

  i = 0;
  gpggaF = gprmcF = FALSE;

  /*
   *    Read characters until NMEA sentence found or MAXCHAR char read
   *    algorithm: look for $ (beginning of new NMEA sentence)
   *    then examine previous sentence (all char back to last $)
   *    for $GPGGA or $GPRMC
   *    if MAXCHAR read without finding NMEA sentence, return NULL pointer
   */
  for (j = 0; j < MAXCHAR; j++) {
    /* Read char off serial port */
    num = read(fser, &chout, sizeof(chout));
    if (num < 0) {
      sprintf(err, " %s\n", strerror(errno));
      syslog(LOG_NOTICE, " error reading serial port = %s", err);
      /* probably nothing available yet -wait 0.5sec = 500,000 */
      usleep(500000); /*FIXME Linx is slow, can sleep 1 sec? */
      continue;
    }
    if (num == 0) {
      sprintf(err, " %s\n", strerror(errno));
      syslog(LOG_NOTICE, "EOF on serial port = %s", err);
      continue;
    }

    /* character found on serial port? */
    if (chout == '$') {
      /* found NMEA sentence */
      buf[i] = '\0';
      if (strncmp(buf, "$GPGGA", 6) == 0) {
        sscanf(buf + 7, "%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c", &gpgga.time,
               &gpgga.latitude, &gpgga.north, &gpgga.longitude, &gpgga.west,
               &gpgga.quality, &gpgga.satellites, &gpgga.dilution,
               &gpgga.altitude, &gpgga.meters, &gpgga.geoid, &gpgga.metric);
        gpggaF = TRUE;
      } else if (strncmp(buf, "$GPRMC", 6) == 0) {
        sscanf(buf + 7, "%f,%c,%f,%c,%f,%c,%f,%f,%d,%f,%c", &gprmc.time,
               &gprmc.status, &gprmc.latitude, &gprmc.north, &gprmc.longitude,
               &gprmc.west, &gprmc.speed, &gprmc.track, &gprmc.date,
               &gprmc.declination, &gprmc.east);
        gprmcF = TRUE;
      }

      /* if both NMEA records received, and both are valid... */
      if (gpggaF && gprmcF && gpgga.quality >= 1 && gpgga.quality <= 5 &&
          gprmc.status == 'V') {
        /* create a new gpslinx record */
        gpslinx.date = ddmmyytoyyyymmdd(gprmc.date);
        gpslinx.gmt = (int)gpgga.time;
        gpslinx.latitude = dmtodd(gpgga.latitude);
        if (gpgga.north == 'S')
          gpslinx.latitude = -gpslinx.latitude;
        gpslinx.longitude = dmtodd(gpgga.longitude);
        if (gpgga.west == 'W')
          gpslinx.longitude = -gpslinx.longitude;
        gpslinx.altitude = gpgga.altitude;
        gpslinx.speed = gprmc.speed * 1.852; /* convert knots/hr to km/hr */
        gpslinx.track = gprmc.track;
        if (fabs(gpslinx.longitude - gpslinxsave.longitude) +
                fabs(gpslinx.latitude - gpslinxsave.latitude) >
            MOTIONLESS * 100)
          gpslinx.declination =
              wmmdeclination(gpslinx.longitude, gpslinx.latitude,
                             gpslinx.altitude / 1000.0, gpslinx.date / 10000,
                             (gpslinx.date % 10000) / 100, gpslinx.date % 100);
        /* copy, then return a new gpslinx record */
        memcpy(&gpslinxsave, &gpslinx, sizeof(struct linxdata));
        return &gpslinx;
      }
      /*
       * ...a NMEA sentence found (chout = $), but it is
       * not the 2 sentences we want, so
       * first log the rejected NMEA sentence, then
       * look for next NMEA by starting a new NMEA sentence
       */
      if ((strncmp(buf, "$GPGGA", 6) != 0) && (strncmp(buf, "$GPRMC", 6) != 0))
        fprintf(fpgps, "%s", buf); /* log unwanted NMEA sentence */

      else if ((strncmp(buf, "$GPGGA", 6) == 0) &&
               (gpgga.quality < 1 || gpgga.quality > 5)) {
        fprintf(fpgps, "Next GPGGA rejected due to  poor quality");
        fprintf(fpgps, "%s", buf); /* log rejected GPGGA sentence */
      } else if ((strncmp(buf, "$GPRMC", 6) == 0) && (gprmc.status == 'V')) {
        fprintf(fpgps, "Next GPRMC rejected due Void status");
        fprintf(fpgps, "%s", buf); /* log rejected GPRMC sentence */
      }
      /* reset search for new NMEA */
      buf[0] = chout;
      i = 1;
      continue;
    }
    /*
     * chout != $
     * continue building NMEA sentence 1 character at atime
     */
    buf[i] = chout;
    i++;
  }
  syslog(LOG_INFO, "GPGGA, GPRMC sentences not found in %d characters", j);
  return NULL;
}

/**
 * linxclose() -- initialize UART, Null Island for linx R4 gps device --
 * Return: nothing
 */
void linxclose(void) {
  syslog(LOG_INFO, "Exiting pmtgpsd  \n");
  close(fser);   /* Close the serial port */
  fclose(fpgps); /* Close the log file */
  return;
}

/**
 * ddmmyytoyyyymmdd() - convert time to standard format for pmt
 * @date format ddmmyy
 * Return: date in format yyyymmdd
 */
int ddmmyytoyyyymmdd(int date) {
  int yy;
  int mm;
  int dd;
  yy = date % 100;
  mm = (date % 10000) / 100;
  dd = date / 10000;
  return ((yy + 2000) * 10000 + mm * 100 + dd);
}

/**
 * dmtodd() - convert degree-minutes to decimal-degrees
 * @dm dddmm.mmmm d=degrees, m=minutes aka degree-minutes
 * Return: ddd.dddddd decimal degrees equivalent
 */
double dmtodd(double dm) {
  int degrees;
  double minutes;
  double dd;

  degrees = (int)(dm / 100.0);
  minutes = dm - (double)degrees;
  dd = (double)degrees + minutes / 60.0;
  return dd;
}
