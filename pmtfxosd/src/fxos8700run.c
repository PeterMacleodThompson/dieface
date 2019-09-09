/**
 * DOC: -- fxos8700run.c -- places fxos8700 data on shared memory
 * Peter Thompson -- Jan 2019  extracted from ~/Documents/../daemons2016/
 *
 * There are 3 parts to this program
 *  1- shared memory code copied from
 *      http://logan.tw/posts/2018/01/07/posix-shared-memory/
 *
 *  2- SIGTERM code copied from:
 *      https://airtower.wordpress.com/2010/06/16/catch-sigterm-exit-gracefully/
 *        and explained in:
 *      http://www.alexonlinux.com/signal-handling-in-linux
 *
 *  3- calls to ecompass() for data to place on shared memory
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* for mmap */
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pmtfxos.h" // for  PMT structures
#include <errno.h>
#include <syslog.h>

#define SIZE (sizeof(struct PMTfxos))
#define NAME "/pmtfxos"
#define I2C_ERROR -1
#define I2C_OK 0
#define SUCCESS 1
#define FAILURE 0

/* function prototypes */
double stackavg();
void stackpush(double);
int i2cOpen();
void i2cClose();
int initFXOS8700(int);
int getdieface(int);
int ecompass(int);
int ecompassinit(void);

volatile sig_atomic_t stopd = 0;

/**
 * terminate()   SIGTERM routine to stop daemon
 * stops pmtgps() infinite loop by setting stopd=1
 * Return: nothing
 */
void terminate(int signum) {
  stopd = 1;
  syslog(LOG_NOTICE, "SIGTERM received for pmtfxosd\n");
}

/**
 * pmtfxos8700() -- continuously posts fxos8700 data to shared memory
 * Return: nothing
 */
void pmtfxos8700(void) {
  struct PMTfxos *fxosnow; /* shared memory structure */
  struct sigaction action; /* SIGTERM for daemon stop */
  int fdshm;               /* shared memory file descriptor */
  int fdi2c;               /* i2c file descriptor */
  float x, y, z;
  char err[100];
  int dieface;
  int j;
  int fxok;

  /* setup for SIGTERM */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = terminate;
  if (sigaction(SIGTERM, &action, NULL) < 0) {
    sprintf(err, "sigaction error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  }

  /* setup for shared memory /dev/shm/pmtfxos */
  fdshm = shm_open(NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fdshm < 0) {
    sprintf(err, "shm_open error = %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return;
  }

  ftruncate(fdshm, SIZE);
  fxosnow = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fdshm, 0);

  /* initiate i2c bus, calibdata  */
  fxok = SUCCESS;
  sprintf(err, " initiate i2c bus=2 and device=0x1e   INIT \n");
  syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  fdi2c = i2cOpen();
  if (initFXOS8700(fdi2c) == I2C_ERROR) {
    sprintf(err, "i2c error, FXOS8700 chip unavailable\n");
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    sprintf(err, "Using defaults: diefaceup=1, heading=0\n");
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    fxok = FAILURE;
  }
  if (ecompassinit() == FAILURE) {
    sprintf(err, "no calib data, using zeros\n");
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
  }
  sprintf(err, "i2c FXOS8700 opened succesfully\n");
  syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */

  /**** START THE BIG LOOP ****/
  while (!stopd) {
    if (fxok) {
      fxosnow->diefaceup = getdieface(fdi2c);
      fxosnow->heading = ecompass(fdi2c);
    } else {
      fxosnow->diefaceup = 1;
      fxosnow->heading = 0;
    }
  }
  /**** END THE BIG LOOP ****/
  /* stopd = stop daemon signal received */
  munmap(fxosnow, SIZE);
  close(fdshm);
  i2cClose(fdi2c);
  shm_unlink(NAME);
  syslog(LOG_NOTICE, "stopping pmtfxosd %d ", getpid());
  return;
}
