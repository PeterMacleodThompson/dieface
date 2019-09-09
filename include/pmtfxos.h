/**
 * DOC: -- pmtfxos.h -- Headers for pmtfxosd daemon
 *  Peter Thompson May 2019
 *   rewrite of fxos8700.h of daemons2016 written Jan 2016
 *
 * This is NOT tilt compensated e-compass
 * Freescale Mark Pedley routines are NOT used.
 *
 * The algorithm is much simpler in that there are
 * only 6 positions (1-6 on a die) based on
 * which dieface is facing up.  Pitch and roll
 * are always computed to 0, 90, 180, or 270 degrees
 *
 * Yaw for the ecompass is computed using arctan(y/x)
 * if diefaceup = 1.   Otherwise, -1 error is returned
 */

/**
 * struct PMTfxos -- shows device dieface postion
 * diefaceup shows dieface facing up - 6 possibilities.
 * degmag is degrees magnetic based on arctan(y/x) if dieface=1
 * degmag = -1 if dieface = 2-6.
 */

#include <stdint.h> /* for uint_8 */

struct PMTfxos {
  int diefaceup; /* value must be 1-6 */
  int heading;   /* 0-360 if diefaceup=1.  otherwise -1 error */
};

/*FXOS8700 structure to read accelerometer, magnetometer data */
typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} SRAWDATA;

/**
 * structure for magnetometer calibration
 * kept in file   ./pmtcalib  FIXME=move to correct location
 */
typedef struct {
  int hardx;       /* hard iron calibration adjustment for x */
  int hardy;       /* hard iron calibration adjustment for y */
  int theta;       /* degrees to rotate ellipse for soft iron adjustment */
  double softx;    /* soft iron x adjustment to convert ellipse to circle */
  int rperfect;    /* radius of the perfect circle for this device uTesla */
  int sdtesla;     /* standard deviation in uTesla (device noise) */
  double sddegree; /* standard deviation for degrees */
  double sdf;      /* standard deviation factor 1=68%, 2=95%, 3=99.7%  */
} CALIBMAGNET;

/*FIXME change to non-cap letters like calibmagnet ?? */