/**
 * DOC: -- pmtgps.h -- Headers for gps daemons
 *  Peter Thompson Jan 2019
 *
 * structures for selected NMEA sentences and
 * a structure for gps data.
 * function prototypes.
 *
 *****************   gps.h  NMEA sentences   *********************
 *  structures for NMEA sentences
 * LINX-EVM-GPS-R4 streams via UART (RS232) the following 8 NMEA sentences:
 *   1 GPGSA - satellites active - ignored
 *   2 GPGSV x3 - satellites in view data - ignored
 *   5 GPRMC - see below
 *   6 GPVTG - ground speed - ignored
 *   7 GPGGA - see below
 *   8 GPGLL - longitude, latitude - ignored
 */

/**
 * struct GPGGA --  NMEA sentence for current fix information
 *   we use 4 items
 *      UTC time,
 *      longitude,NS,
 *      latitude, EW,
 *      Altitude, M,
 */
struct GPGGA {
  float time;      /* 999999.999 = UTC time hh:mm:ss.sss */
  float latitude;  /* 9999.9999 = 99 degrees 99.9999 minutes */
  char north;      /* N=North or S=South */
  float longitude; /* 99999.9999 = 999 degrees 99.9999 minutes */
  char west;       /* W=west or E = east */
  int quality;     /* 0=invalid, 1-8=valid */
  int satellites;  /* 99 = number of satellites being followed */
  float dilution;  /* 9.9 = horizontal dilution of position */
  float altitude;  /* 9999.9 = altitude above mean sea level */
  char meters;     /* M=meters */
  float geoid;     /* height of geoid (mean sea level) above WGS84 ellipsoid */
  char metric;     /* M=meters */
};

/**
 * struct GPRMC --  NMEA sentence containing minimum data for gps
 *   we use 3 items
 *      date,
 *      speed,
 *      track,
 */
struct GPRMC {
  float time;        /* 999999.999 = UTC hh:mm:ss.sss */
  char status;       /* A=active, V=void */
  float latitude;    /* 9999.9999 = 99 degrees 99.9999 minutes */
  char north;        /* N=North or S=South */
  float longitude;   /* 99999.9999 = 999 degrees 99.9999 minutes */
  char west;         /* W=west or E = east */
  float speed;       /* 999.99 = knots per hour */
  float track;       /* 999.99 = track angle in degrees True */
  int date;          /* 999999 = ddmmyy */
  float declination; /* 999.9 */
  char east;         /* W=west, E=east */
};

/**
 * struct PMTgps --  gps data in display format
 *   longitude, latitude is degrees, minutes seconds (not Decimal degrees)
 *   1 second of latitude is approx 100 ft (33 meters)
 *   longitude format is dddmmss plus an East/West indicator
 *   latitude format is ddmmss plus a North/South indicator
 */
struct PMTgps {
  int spinlock;       /* assume atomic like sig_atomic_t. 0=free, 1=locked */
  int status;         /* -1=lost satellites, 0=stationary, 1=moved */
  int latitude;       /* format = ddmmss */
  char latitudeNS;    /* N=north, S=South (default N) */
  int longitude;      /* format = dddmmss */
  char longitudeEW;   /* E=east, W=west (default W) */
  int altitude;       /* in feet  NO NEED KM FOR DECLINATION CALCULATION */
  double declination; /* to West = negative (Toronto), to East = positive */
                      /* (Calgary) http://www.ngdc.noaa.gov/geomag/WMM/ */
                      /* magnetic-declination.com */
  int speed;          /* in km per hour */
  int track;          /* in degrees */
  int date;           /* based on GMT format = yyyymmdd */
  int gmt;          /* Greenwich Mean Time = Coordinated Universal Time(UTC) */
                    /* = solar time at Greenwich England.  format = hhmmss */
  int solartime;    /* mean local solar time = LongitudeDegrees/15 + gmt */
  int meridiantime; /* local standard time based on 15 degree meridians: hhmmss
                     */
  int meridianlong; /* START longitude of current timezone. END = START + 15 */
                    /* degrees */
};

/* Function Prototypes */
double wmmdeclination(double, double, double, int, int, int);
void wmminit(void);
void wmmclose(void);
