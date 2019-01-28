/*   getdieface(struct dieface *df)  
     extracts pitch, roll from daemons pmtfxosd (fxos8700.h) 
	   and returns which dieface is facing UP.

Computation is based on:
http://personal.maths.surrey.ac.uk/st/T.Bridges/SLOSH/3-2-1-Eulerangles.pdf
AN4685, AN4696  by Mark Pedley in ~/Documents/daemons2016/pmtfxosd/Documentation
Euler order of computation is 1st yaw (ignored here), 2nd pitch, 3rd roll

Dieface computed from accelerometer using NED=North,East,Down standard:
 - yaw 		0  < psi  < 360 (as per normal compass)
 - pitch 	-90 < theta < 90  pitching nose up from pilot seat
 - roll 	-180 < phi < 180  rolling clockwise from pilot seat 

 - ecompassmag
 - ecompasstrue .... may be used in future
THUS for dieface calculation, 
	Yaw = 0, 90, 180, 270   (360=0)
	Pitch = 0, +90, -90
 	Roll  = 0, +90, -90, +-180
	  making for 48 possible dieface Euler Angles (see table below)
	Euler order of computation is 1st yaw , 2nd pitch, 3rd roll
	There are 6 dieface's with 4 yaw positions each
		 = 24 unique dieface  positions

	hikingGPS reference orientation:
		device = face up on flat surface pointing true North
	  	dieface = 1 Up and 2 North(true, not magetic) = 12
	1 up
	2 North- true, not magnetic
	3 West
	4 East
	5 South
	6 Down
 
------- Dieface Orientation Table--(derived manually)---------------------
                   0=North    90=East   180=South    270=West       
             -------------------------------------------------------------
PITCH .. ROLL
 0 .. 0             12          13          15           14
 0 .. +90           32          53          45           24
 0 .. -90           42          23          35           54
 0 .. +-180         62          63          65           64

+90 .. 0            26          36          56           46
+90 .. +90          36          56          46           26
+90 .. -90          46          26          36           56
+90 .. +-180        56          46          26           36

-90 .. 0			51          41          21           31
-90 .. +90          31          51          41           21
-90 .. -90          41          21          31           51
-90 .. +-180        21          31          51           41 


--------------------------------------------------------------------------
which converts to 3 dimensional table T(pitch, roll, yaw) where
pitch 0, +90, -90 <==> 0,1,2
roll 0, +90, -90, +-180 <==> 0,1,2,3
yaw 0, 90, 180, 270 <==> 0,1,2,3 





  
   mmap read from PMTfxos8700 
copied from www.linuxquestions.org - mmap tutorial
http://www.linuxquestions.org/questions/programming-9/mmap-tutorial-c-c-511265/
search "mmap command"




*/


#include <stdio.h>
#include <stdlib.h>
#include <syslog.h> 
#include <stdint.h>  	

#include "fxos8700.h" 	// for raw accelerometer data


#define FILEPATH "/tmp/fxos8700.bin"
#define FILESIZE (sizeof(struct PMTfxos8700))
#define YAW 4
#define PITCH 3
#define ROLL 4

int getdieface(struct PMTfxos8700 *fxosnow)
{
    int 		fd;
	int			dieface; 
	char 		debuglog[500];		// printer string sent to syslog	

	int 		pitch, roll, yaw;
	/* https://embeddedgurus.com/stack-overflow/2010/01/a-tutorial-on-lookup-tables-in-c/ */
	static const int   dietable[PITCH] [ROLL] [YAW] =  
	{     
	  {
		{12, 13, 15, 14},	
		{32, 53, 45, 24},
		{42, 23, 35, 54},
		{62, 63, 65, 64}
	  },
	  {
		{26, 36, 56, 46},
		{36, 56, 46, 26},
		{46, 26, 36, 56},
		{56, 46, 26, 36}
	  },
	  {
		{51, 41, 21, 31},
		{31, 51, 41, 21},
		{41, 21, 31, 51},
		{21, 31, 51, 41} 
	  }	
	};


	/* convert  fxos8700 input to yaw, pitch roll dieface digits */
	/* pitch 0, +90, -90 <==> 0,1,2 */
	if( fxosnow->pitch < -45.0 )
		pitch = 2;
	else if( fxosnow->pitch > 45.0 )
		pitch = 1;
	else 
		pitch = 0;

	/* roll 0, +90, -90, +-180 <==> 0,1,2,3 */
	if( fxosnow->roll < -135.0  || 
		fxosnow->roll  >  +135.0 )
		roll = 3;
	else if( fxosnow->roll < -45 && fxosnow->roll > -135.0 ) 
		roll = 2;
	else if( fxosnow->roll  > 45  && fxosnow->roll < +135.0 ) 		
		roll = 1;
	else 
		roll = 0;

	/* yaw 0, 90, 180, 270 <==> 0,1,2,3  */
	if( fxosnow->yaw < 45 )
		yaw = 0;
	else if( fxosnow->yaw < 135 )
		yaw = 1;
	else if( fxosnow->yaw < 225 )
		yaw = 2;
	else if( fxosnow->yaw < 315 )
		yaw = 3;
	else 
		yaw = 0;

	dieface = dietable[pitch][roll][yaw];

	return( dieface );
}


