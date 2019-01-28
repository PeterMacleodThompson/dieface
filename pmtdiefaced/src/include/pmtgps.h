/*****************   gps.h  NMEA sentences   **********************/
/* Peter Thompson structures for NMEA sentences */
/* LINX-EVM-GPS-R4 streams via UART (RS232) the following 8 NMEA sentences:
1 GPGSA - satellites active - ignored
2 GPGSV x3 - satellites in view data - ignored
5 GPRMC - see below
6 GPVTG - ground speed - ignored
7 GPGGA - see below
8 GPGLL - longitude, latitude - ignored
*/
    struct GPGGA {
	float time;		// 999999.999 = UTC time hh:mm:ss.sss
	float latitude;		// 9999.9999 = 99 degrees 99.9999 minutes
	char north;		// N=North or S=South
	float longitude;	// 99999.9999 = 999 degrees 99.9999 minutes
	char west;		// W=west or E = east	
	int quality;		// 0=invalid, 1-8=valid
	int satellites;		// 99 = number of satellites being followed
	float dilution;		// 9.9 = horizontal dilution of position
	float altitude;		// 9999.9 = altitude above mean sea level
	char meters;		// M=meters 	  	
	float geoid;		// height of geoid (mean sea level) above WGS84 ellipsoid
	char metric;		// M=meters
    };

    struct GPRMC {
	float time;		// 999999.999 = UTC hh:mm:ss.sss
	char status;		// A=active, V=void
	float latitude;		// 9999.9999 = 99 degrees 99.9999 minutes
	char north;		// N=North or S=South
	float longitude;	// 99999.9999 = 999 degrees 99.9999 minutes
	char west;		// W=west or E = east	
	float speed;		// 999.99 = knots per hour
	float track;		// 999.99 = track angle in degrees True
	int date;		// 999999 = ddmmyy
	float declination;	// 999.9 
	char east;		// W=west, E=east
    };

struct PMTgps {			//FIXME check PMTgps format consistent with oz4
    int		spinlock;	// assume atomic like sig_atomic_t. 0=free, 1=locked
    int 	status;		// -1=lost satellites, 0=stationary, 1=moved
    int 	latitude;	// format = ddmmss 
    char	latitudeNS;	// N=north, S=South (default N)
    int 	longitude;	// format = dddmmss
    char	longitudeEW;	// E=east, W=west (default W)
    int 	altitude; 	// in feet
    double 	declination;	// to West = negative (Toronto), to East = positive (Calgary) 
				// http://www.ngdc.noaa.gov/geomag/WMM/ 
				// magnetic-declination.com
    int		speed;		// in km per hour
    int 	track;		// in degrees 
    int 	date;		// based on GMT format = yyyymmdd
    int 	gmt;		// Greenwich Mean Time = Coordinated Universal Time(UTC) 	
				// = solar time at Greenwich England.  format = hhmmss
    int 	solartime;	// mean local solar time = LongitudeDegrees/15 + gmt
    int 	meridiantime; 	// local standard time based on 15 degree meridians: hhmmss 
    int 	meridianlong;	// START longitude of current timezone. END = START + 15 degrees
}; 









