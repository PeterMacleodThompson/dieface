/**
 * DOC: --  testdaemons.c  -- test pmtfxosd, pmtgpsd, pmtdiefaced daemons
 *  Peter Thompson   -- May 2019
 *
 *  reads/prints shared memory files: /dev/shm/pmtfxos, pmtgps, pmtdieface
 *
 * cross-compile with:
 *  arm-linux-gnueabihf-gcc -o testfxosdaemon  testfxosdaemon.c  -I../include -L
 * /home/peter/bbb2018/buildroot/output/target/usr/lib  -lrt
 */

#include <fcntl.h> /* for shared memory access */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> /* for shared memory access */
#include <unistd.h>   /* for shared memory access */

#include "dieface.h"
#include "pmtfxos.h" // for  PMT structures
#include "pmtgps.h"  // NMEA sentences, PMT structures

#define NAME1 "/pmtfxos"
#define SIZE1 (sizeof(struct PMTfxos))
#define NAME2 "/pmtgps" //  for shared memory /dev/shm/pmtgps
#define SIZE2 (sizeof(struct PMTgps))
#define NAME3 "/pmtdieface"
#define SIZE3 (sizeof(struct PMTdieEvent))

void main() {

  struct PMTgps *gpsnow;   // shared memory structure
  struct PMTfxos *fxosnow; // shared memory structure
  struct PMTdieEvent *dienow;

  int fd1, fd2, fd3; /*/dev/pmtgps /dev/pmtfxos /dev/pmtdieEvent */

  /* initialize daemon access fxosd */
  fd1 = shm_open(NAME1, O_RDONLY, 0666);
  if (fd1 < 0) {
    perror("/dev/shm/pmtfxos");
    return;
  }
  fxosnow = mmap(0, SIZE1, PROT_READ, MAP_SHARED, fd1, 0);

  /* initialize daemon access - pmtgpsd */
  fd2 = shm_open(NAME2, O_RDONLY, 0666);
  if (fd2 < 0) {
    perror("/dev/shm/pmtgps");
    return;
  }
  gpsnow = mmap(0, SIZE2, PROT_READ, MAP_SHARED, fd2, 0);

  /* initialize daemon access dieface */
  fd3 = shm_open(NAME3, O_RDONLY, 0666);
  if (fd3 < 0) {
    perror("/dev/shm/pmtdieface");
    return;
  }
  dienow = mmap(0, SIZE3, PROT_READ, MAP_SHARED, fd3, 0);

  /* loop forever printing shared memory */
  for (;;) {
    printf("\r diefaceup,heading= %d, %5d; action id,state,last=%d, %d, %lld",
           fxosnow->diefaceup, fxosnow->heading, dienow->id, dienow->diefaceSS,
           dienow->dieaction);
    fflush(stdout);
    usleep(25000); /* = 40Hz because 1/40=25000 ms */
  }
  return;
}
