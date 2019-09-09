/* diefacemain.c
  PeterThompson March 2016 - revised Dec 2018

/****************  main for testing ***********************/

// for shared memory
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>
#include <syslog.h>

#include "dieface.h"
#include "pmtfxos.h"

// shared memory for dieface events
#define INNAME "/pmtfxos"
#define INSIZE (sizeof(struct PMTfxos))
#define OUTNAME "/pmtdieface"
#define OUTSIZE (sizeof(struct PMTdieEvent))

// function prototypes
void diefaceinit(struct PMTdieEvent *);
int diefacehandler(struct PMTdieEvent *, int);
/* returns 0=False=no new steady state
   OR 1=True=new steady state */
int getdieface();

volatile sig_atomic_t stopd = 0;

/* daemon stop = SIGTERM routine */
void terminate(int signum) {
  stopd = 1;
  syslog(LOG_NOTICE, "SIGTERM received for pmtdiefaced\n");
}

void diefacemain() {
  struct PMTdieEvent *dfnow; /* df=dieface shared memory out */
  struct PMTfxos *fxosnow;   /* shared memory in */
  struct sigaction action;   /* SIGTERM for daemon stop */

  int fd1, fd2; /* file descriptor */
  char err[100];
  int diefaceNew;

  /******** open /dev/shm/pmtfxos for reading (consumer)************/
  fd2 = shm_open(INNAME, O_RDONLY, 0666);
  if (fd2 < 0) {
    perror("shm_open()");
    return;
  }

  fxosnow = mmap(0, INSIZE, PROT_READ, MAP_SHARED, fd2, 0);

  /******* open /dev/shm/pmtdieface for writing (producer) *******/
  /* setup for SIGTERM */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = terminate;
  if (sigaction(SIGTERM, &action, NULL) < 0) {
    sprintf(err, "sigaction error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  }

  /* setup for shared memory /dev/shm/pmtdieface */
  fd1 = shm_open(OUTNAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd1 < 0) {
    sprintf(err, "shm_open error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return;
  }

  ftruncate(fd1, OUTSIZE);
  dfnow = mmap(0, OUTSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);

  diefaceinit(dfnow);
  /* THE BIG LOOP */
  while (!stopd) {
    diefaceNew = fxosnow->diefaceup;

    /* state machine - uses 1st digit (up) only */
    diefacehandler(dfnow, diefaceNew);

    //		usleep(100000);		/* 100,000 = 10 Hz = fxos8700.c */
    usleep(500000); /* sleep 1/2 sec */
  }

  /* SIGTERM received */
  munmap(dfnow, OUTSIZE);
  close(fd2);
  shm_unlink(OUTNAME);

  munmap(fxosnow, INSIZE);
  close(fd1);

  syslog(LOG_NOTICE, "stopping pmtdiefaced %d ", getpid());
  return;
}
