/**
 * DOC: -- pmtgpsdaemon.c  -- initialize daemon for gps --
 * Peter Thompson Jan 2019
 *
 *      daemon code copied from ...
 *      http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
 *
 * daemon name = pmtgpsd =  defined in S50pmtgpd script
 * sudo service S50pmtgpsd start  <==> sudo /etc/init.d/S50pmtgpsd
 * sudo service S50pmtgpsd stop
 * cat /var/log/syslog | tail  ## has all daemon messages
 *
 * X86 compile with
 * gcc -o ../bin/pmtgpsd gpsstub.c  pmtgpsdaemon.c peterpoint.c
 *      GeomagnetismLibrary.c -I./include  -lrt -lm   OR
 * ARM compile with
 *    export PATH=$PATH:$HOME/bbb2018/buildroot/output/host/bin ## for compiler
 *    arm-linux-gnueabihf-gcc -o ../bin/pmtgpsd gpsstub.c  pmtgpsdaemon.c
 * peterpoint.c GeomagnetismLibrary.c -I./include -lrt -lm
 */

void pmtgps(void); /* the big loop process */

/* for daemon setup */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/**
 * main()   daemon
 * @argc    not used in this daemon
 * @argv    not used in this daemon
 *
 * daemon started by S50pmtgpsd script
 */
int main(int argc, char *argv[]) {

  char pmtsetpid[100]; /* for pid correction */

  /* process id and session id */
  pid_t pid, sid;

  /* Fork the Parent Process */
  pid = fork();

  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  /* We got a good pid, Close the Parent Process */
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Change File Mask (permissions) to enable daemon access */
  umask(0);

  /* Create a new Signature Id for our child */
  sid = setsid();
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  /* Change working directory to / = pmtroot */
  if ((chdir("/")) < 0) {
    exit(EXIT_FAILURE);
  }

  /* Close Standard File Descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  /**
   * pmt: update daemon pid which was incorrectly created by
   *      S50pmtgpsd script.
   * see
   * http://unix.stackexchange.com/questions/78056/start-stop-daemon-makes-cron-pidfile-with-wrong-pid*/
  sprintf(pmtsetpid, "echo %d > /run/pmtgpsd.pid", getpid());
  system(pmtsetpid);

  /* update /var/log/syslog */
  syslog(LOG_NOTICE, "Peter Thompson pmtgpsd connecting with gps");
  syslog(LOG_NOTICE, "%s", pmtsetpid);

  /* the big loop */
  pmtgps();
}
