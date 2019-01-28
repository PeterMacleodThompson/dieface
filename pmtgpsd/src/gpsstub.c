/*  gpsstub.c  replaces pmtgps.c ... X86  TEST ONLY
	shared memory code copied from 
 http://logan.tw/posts/2018/01/07/posix-shared-memory/

SIGTERM code copied from:
https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/
and explained in:
http://www.alexonlinux.com/signal-handling-in-linux
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* for mmap */
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <syslog.h>
#include <errno.h>


#include "pmtgps.h" 	// for NMEA sentences and PMT structures
#define NAME "/pmtgps"	//  for shared memory /dev/shm/pmtgps
#define SIZE (sizeof(struct PMTgps))

/* convert degree minute second to degree decimal */
#define DMStoDD(ddmmss)   ( (double)(ddmmss/10000) + \
  ((double)((ddmmss%10000)/100))/60.0 + ((double)(ddmmss%100)/3600.0) ) 

volatile sig_atomic_t stopd = 0;

/* daemon stop = SIGTERM routine */
void terminate(int signum)
{
    stopd = 1;
	syslog(LOG_NOTICE, "SIGTERM received for pmtgpsd\n" );
}




void pmtgps(void)	
{
    struct PMTgps 	*gpsnow;   // shared memory structure
    struct sigaction action;		// SIGTERM for daemon stop
    int 		fd;
    int 		i;
	char		err[100];
	double 		d;


	/* setup for SIGTERM */
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate;
    if( sigaction(SIGTERM, &action, NULL) < 0)   {
		sprintf( err, "sigaction error = %s\n", strerror(errno) ); 
    	syslog(LOG_NOTICE, "%s", err ); /*to /var/log/syslog */
	}

	/* setup for shared memory /dev/shm/pmtgps */
  	fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  	if (fd < 0) {
		sprintf( err, "shm_open error = %s\n", strerror(errno) ); 
    	syslog(LOG_NOTICE, "%s", err ); /*to /var/log/syslog */
    	return;
 	 }

  	ftruncate(fd, SIZE);
  	gpsnow = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			 fd, 0);



	wmminit();  /* world magetic model init */


	/*  THE BIG LOOP   */
    while(!stopd)
    {
 /****** some pmt defaults  *****/
/*
Percy Lake longlat 78 22 11.0, 45 13 9.0
	on map 031e01 ==> NW corner at 78 30 00W  45 15 00
	return(782211451309);	// return Percy Lake for now 
Panorama longlat 116 14 26.0, 50 27 33.9
Foster Lake longlat 105 30 0.0, 56 45 0.0  
Forgetmenot longlat 114 48 50.0, 50 47 43.6
Nahanni longlat 123 49 55.0, 61 06 55.0	NOT FOUND
Bonavista longlat 114 03 12.84, 50 56 41.58
*/
		/* now write to the mmap file=gpsnow as if it were memory */

		/* percy lake */
 		gpsnow->longitude = 782211;  /* 78.36972 */
 		gpsnow->longitudeEW = 'W';
 		gpsnow->latitude = 451309;   /* 45.21917 */
 		gpsnow->latitudeNS = 'N';
 		gpsnow->altitude = 1450;	/* 1450 ft 440 meters */
 		gpsnow->declination = wmmdeclination(-78.36972, 45.21917, 0.440, 2018, 12, 18); 
		/* declination = -11 degrees 39 minutes =  */
 		gpsnow->speed = 4;
 		gpsnow->track = 90;
 		gpsnow->date = 20181031;
 		gpsnow->gmt = 103005;	
 		gpsnow->solartime = gpsnow->longitude / 150000 + gpsnow->gmt;
 		gpsnow->meridiantime = 0;
 		gpsnow->meridianlong = 0;

		sleep(3);

		/* panorama */
 		gpsnow->longitude = 1161426;	/* 116.2406  */
 		gpsnow->longitudeEW = 'W';
 		gpsnow->latitude = 502734;		/* 50.45944  */
 		gpsnow->latitudeNS = 'N';
 		gpsnow->altitude = 3773;		/* 3773ft = 1150meters */
 		gpsnow->declination = wmmdeclination(-116.2406, 50.45944, 1.150, 2018, 12, 18);
			/* declination = 14 degrees, 39 minutes = 14.65 degrees */
 		gpsnow->speed = 2;
 		gpsnow->track = 270;
 		gpsnow->date = 20190115;
 		gpsnow->gmt = 112535;	
 		gpsnow->solartime = gpsnow->longitude / 150000 + gpsnow->gmt;
 		gpsnow->meridiantime = 0;
 		gpsnow->meridianlong = 0;

		sleep(3);
		 
    }

	munmap(gpsnow, SIZE); 
	close(fd);
	shm_unlink(NAME);
	wmmclose();  /* world magetic model close */
	syslog(LOG_NOTICE, "stopping pmtgpsd %d ", getpid() );

    return;

}
