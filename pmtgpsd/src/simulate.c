/**
 * DOC: -- gpsfix.c -- places fixed gps location on shared memory
 * Peter Thompson -- April 2019
 *
 * places predefined fixed gps data on shared memory
 * used for
 *   - accessing specific maps at known locations
 *   - defaults when gps device is not operating
 *   - simulations, testing, demonstrations of hikiingGPS software
 *
 * write-your-own, pick-your-own, compile-your-own simulation
 * via #define, #ifdef #endif  statements
 *
 ****** some pmt favourite locations  *****
 *  Percy Lake longlat 78 22 11.0, 45 13 9.0
 *          on map 031e01 ==> NW corner at 78 30 00W  45 15 00
 *  Panorama longlat 116 14 26.0, 50 27 33.9
 *  Foster Lake longlat 105 30 0.0, 56 45 0.0
 *  Forgetmenot longlat 114 48 50.0, 50 47 43.6
 *  Nahanni longlat 123 49 55.0, 61 06 55.0
 *  Bonavista longlat 114 03 12.84, 50 56 41.58
 */
#include "pmtgps.h"

#define PercyLake
/* #define Panorama */

#ifdef PercyLake
void simulate(struct PMTgps *gpsnow) {
  gpsnow->longitude = 782211; /* 78.36972 */
  gpsnow->longitudeEW = 'W';
  gpsnow->latitude = 451309; /* 45.21917 */
  gpsnow->latitudeNS = 'N';
  gpsnow->altitude = 1450; /* 1450 ft 440 meters */
  gpsnow->declination = -11.567;
  gpsnow->speed = 4;
  gpsnow->track = 90;
  gpsnow->date = 20181031;
  gpsnow->gmt = 103005;
  gpsnow->solartime = gpsnow->longitude / 150000 + gpsnow->gmt;
  gpsnow->meridiantime = 0;
  gpsnow->meridianlong = 0;
  return;
}
#endif

#ifdef Panorama
void simulate(struct linxdata *gpsnow) {
  gpsnow->longitude = 1161426; /* 116.2406  */
  gpsnow->longitudeEW = 'W';
  gpsnow->latitude = 502734; /* 50.45944  */
  gpsnow->latitudeNS = 'N';
  gpsnow->altitude = 3773; /* 3773ft = 1150meters */
  gpsnow->declination = 14.65;
  gpsnow->speed = 2;
  gpsnow->track = 270;
  gpsnow->date = 20190115;
  gpsnow->gmt = 112535;
  gpsnow->solartime = gpsnow->longitude / 150000 + gpsnow->gmt;
  gpsnow->meridiantime = 0;
  gpsnow->meridianlong = 0;
  return;
}
#endif
