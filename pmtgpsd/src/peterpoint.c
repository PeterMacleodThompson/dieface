/*from  https://ngdc.noaa.gov/geomag/WMM/soft.shtml 
 World Magnetic Model by NOAA   
 - hacked from wmm_point.c into 3 parts:
	- wmminit()
	- wmmdecl() + hack GeomagnetismLibrary.c 
		+ replace MAG_GetUserInput()
		+ remove MAG_PrintUserDataWithUncertainty()
	- wmmclose()
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <syslog.h>

#include "GeomagnetismHeader.h"
/*#include "GeomagnetismLibrary.c"*/
#include "EGM9615.h"

/*---------------------------------------------------------------------------*/

/* 
The program expects the files GeomagnetismLibrary.c, GeomagnetismHeader.h,
WMM.COF and EGM9615.h to be in the same directory. 

Manoj.C.Nair@Noaa.Gov
April 21, 2011

 *  Revision Number: $Revision: 1270 $
 *  Last changed by: $Author: awoods $
 *  Last changed on: $Date: 2014-11-21 10:40:43 -0700 (Fri, 21 Nov 2014) $
*/


/*
**** following comments from GeomagnetismLibrary.c MAG_GetUserInput() ******
This takes the MagneticModel and Geoid as input and outputs the Geographic coordinates and Date as objects.
INPUT :  
MagneticModel  : Data structure with the following elements used here
  double epoch;       Base time of Geomagnetic model epoch (yrs)
               : Geoid Pointer to data structure MAGtype_Geoid (used for converting HeightAboveGeoid to HeightABoveEllipsoid

OUTPUT: 
CoordGeodetic : Pointer to data structure. Following elements are updated
  double lambda; (longitude)
  double phi; ( geodetic latitude)
  double HeightAboveEllipsoid; (height above the ellipsoid (HaE) )
  double HeightAboveGeoid;(height above the Geoid )

MagneticDate : Pointer to data structure MAGtype_Date with the following elements updated
  int	Year; (If user directly enters decimal year this field is not populated)
  int	Month;(If user directly enters decimal year this field is not populated)
  int	Day; (If user directly enters decimal year this field is not populated)
  double DecimalYear;      decimal years  THIS IS REQUIRED

CALLS: 	MAG_DMSstringToDegree(buffer, &CoordGeodetic->lambda); (The program uses this to convert the string into a decimal longitude.)
                MAG_ValidateDMSstringlong(buffer, Error_Message)
                MAG_ValidateDMSstringlat(buffer, Error_Message)
                MAG_Warnings
                MAG_ConvertGeoidToEllipsoidHeight
                MAG_DateToYear

 */

    /* Memory allocation */
    static MAGtype_MagneticModel * MagneticModels[1], *TimedMagneticModel;
    static MAGtype_Ellipsoid Ellip;
    static MAGtype_CoordSpherical CoordSpherical;
    static MAGtype_CoordGeodetic CoordGeodetic;
    static MAGtype_Date UserDate;
    static MAGtype_GeoMagneticElements GeoMagneticElements, Errors;
    static MAGtype_Geoid Geoid;
    static char filename[] = "/usr/share/pmtgeomagnet/WMM.COF";
//	static char filename[] = "WMM.COF";
    static char VersionDate_Large[] = "$Date: 2014-11-21 10:40:43 -0700 (Fri, 21 Nov 2014) $";
    static char VersionDate[12];
    static int NumTerms, nMax = 0;
    static int epochs = 1;


	static int wmmstop = 0;  /* TRUE = 1, FALSE = 0 */



/* WMM initialize */
void wmminit()
{
	char		err[100];

    strncpy(VersionDate, VersionDate_Large + 39, 11);
    VersionDate[11] = '\0';
    if(!MAG_robustReadMagModels(filename, &MagneticModels, epochs)) {
        sprintf(err, "/usr/share/pmtgeomagnet/WMM.COF not found.  declination = 0.0 \n ");
    	syslog(LOG_NOTICE, "%s", err ); /*to /var/log/syslog */  
		wmmstop = 1;   /* TRUE */      
        return;
    }
    if(nMax < MagneticModels[0]->nMax) nMax = MagneticModels[0]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */
    if(MagneticModels[0] == NULL || TimedMagneticModel == NULL)
    {
        MAG_Error(2);
    }
    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */
    /* Check for Geographic Poles */



    /* Set EGM96 Geoid parameters */
    Geoid.GeoidHeightBuffer = GeoidHeightBuffer;
    Geoid.Geoid_Initialized = 1;
    /* Set EGM96 Geoid parameters END */
    /* WMM initialize END */

	return;
}


/* declination calculation */
double wmmdeclination( double longitude, double latitude, double altitudekm, int year, int month, int day )
{
	char		err[100];

	if(wmmstop)
		return(0.0);

	/*Get User Input - peter's hack  */
	CoordGeodetic.phi = latitude;
	CoordGeodetic.lambda = longitude;
	CoordGeodetic.HeightAboveGeoid = altitudekm;
	Geoid.UseGeoid = 1;
 	MAG_ConvertGeoidToEllipsoidHeight(&CoordGeodetic, &Geoid); 
	UserDate.Month = month;
	UserDate.Day  = day;
	UserDate.Year = year; 
	if( !MAG_DateToYear(&UserDate, err) )  {
    	syslog(LOG_NOTICE, "%s", err ); 	/*to /var/log/syslog */
		return(0.0);
	}  		

	/* do wmm magic  - copied from wmm_point.c */       
    MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/
    MAG_TimelyModifyMagneticModel(UserDate, MagneticModels[0], TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
    MAG_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements); /* Computes the geoMagnetic field elements and their time change*/
    MAG_CalculateGridVariation(CoordGeodetic, &GeoMagneticElements);
    MAG_WMMErrorCalc(GeoMagneticElements.H, &Errors); 		/* I dont think I use this  */


	return(GeoMagneticElements.Decl);

}



    
void wmmclose()
{
	/* WMM close  */
    MAG_FreeMagneticModelMemory(TimedMagneticModel);
    MAG_FreeMagneticModelMemory(MagneticModels[0]);
	/* WMM close END */

    return;
}


/* main for testing only
calculates  declination = 14 degrees, 59 minutes */ 
/* uncomment main() and compile with 
gcc -o peterpoint peterpoint.c GeomagnetismLibrary.c -I./include -lm   */

/*
main()
{
	double longitude = -116.2406;
	double latitude = 50.45944;
	double altitudekm  = 1.150;
	int month = 12;
	int day = 18;	
	int year = 2018;
	double d, e;
	
	wmminit(); 
	
	while(1) {
	d = wmmdeclination( longitude,latitude, altitudekm, year, month, day);
 	e = wmmdeclination(-78.36972, 45.21917, 0.440, 2018, 12, 18); 
	printf(" declination = %7.4f  %7.4f \n", d, e);
	}

	wmmclose();  
}

*/

