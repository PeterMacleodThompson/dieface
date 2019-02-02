/*  fxos8700stub.c  replaces fxos8700.c ... X86  TEST ONLY
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

#include <errno.h>
#include <syslog.h>

#include "fxos8700.h" // for  PMT structures
#define SIZE (sizeof(struct PMTfxos8700))
#define NAME "/pmtfxos"

volatile sig_atomic_t stopd = 0;

/* daemon stop = SIGTERM routine */
void terminate(int signum) {
  stopd = 1;
  syslog(LOG_NOTICE, "SIGTERM received for pmtfxosd\n");
}

void pmtfxos8700(void) {
  struct PMTfxos8700 *fxosnow; // shared memory structure
  struct sigaction action;     // SIGTERM for daemon stop
  int fd;
  float x, y, z;
  char err[100];
  int dieface;
  int j;

  /* setup for SIGTERM */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = terminate;
  if (sigaction(SIGTERM, &action, NULL) < 0) {
    sprintf(err, "sigaction error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  }

  /* setup for shared memory /dev/shm/pmtfxos */
  fd = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd < 0) {
    sprintf(err, "shm_open error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return;
  }

  ftruncate(fd, SIZE);
  fxosnow = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /*  THE BIG LOOP   */
  x = 0.0;
  y = -90.0;
  z = -180.0;
  j = 0;
  while (!stopd) {
    x = x + 20.0;
    if (x > 360.0) {
      x = 0.0;
      y = y + 10.0;
      if (y > 90) {
        y = -90;
        z = z + 10;
        if (z > 180.0) {
          z = -180;
        }
      }
    }

    /* now write to the mmap file=fxosnow as if it were memory */
    /* overwrites single fxosnow record in /dev/shm/pmtfxos
            TODO filter all fxos values via average of 5-10 raw values
            TODO apply NOAA declination to ecompass */
    fxosnow->yaw = x;         /* 0 -> 360 */
    fxosnow->pitch = y;       /* -90 -> +90 */
    fxosnow->roll = z;        /* -180 -> +180 */
    fxosnow->ecompassmag = x; /* 0 -> 360 */
    dieface = getdieface(fxosnow);
    fxosnow->diefaceUSS = dieface;
    //		usleep(25000);		/* 40 hz = 40 times/sec */
    //		usleep(100000);		/* 100,000 = 10 Hz = fxos8700.c */
    usleep(500000); /* sleep 1/2 sec = 2 Hz */

    if (j++ >= 40) {
      j = 0;
      sleep(5);
    }
  }

  /* stopd = stop daemon signal received */
  munmap(fxosnow, SIZE);
  close(fd);
  shm_unlink(NAME);
  syslog(LOG_NOTICE, "stopping pmtfxosd %d ", getpid());
  return;
}
