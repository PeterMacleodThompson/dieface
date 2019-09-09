WMM = World Magnetic Model
Given longitude, latitude (gps), WMM calculates magnetic declination
which is added to magnetometer (fxos8700) ==> true north
 
downloaded from http://www.ngdc.noaa.gov/geomag/WMM/
Last downloaded Dec 2018.  
Must be re-downloaded on occasion to 
update earth's changing magnetic fields.

In original downloaded sources:
-------------------------------
WMM2015_Linux/src/
wmm_point.c = WMM supplied example program
  compiled into point & tested
peterpoint.c = hacked version of wmm_point.c
  compiled into peterpoint & tested (not in originals)
WMM.COF is data file required for execution of wmm_point or peterpoint
 
WMM2015_Linux/bin/ is not used  
original downloaded sources not included in github

In pmtgpsd
----------
WMM.COF is stored in github/../pmtgpsd/data
 but it MUST BE MOVED to /usr/share/pmt/WMM.COF 
EGM9615.h & GeomagnetismHeader.h  are kept in pmtgpsd/src/include/
GeomagnetismLibrary.c peterpoint.c are kept in pmtgpsd/src/



