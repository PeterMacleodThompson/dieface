/* gcc -o pmtfxosd pmtfxosdaemon.c fxos8700stub.c getdieface.c -lrt    OR

   export PATH=$PATH:$HOME/bbb2018/buildroot/output/host/bin ## for compiler
   arm-linux-gnueabihf-gcc -o pmtfxosd pmtfxosdaemon.c fxos8700stub.c getdieface.c -lrt

	daemon code copied from ...
   	http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html

daemon name = pmtfxosd =  defined in S60pmtfxosd script
sudo service S60pmtfxosd start  <==>  /etc/init.d/S60pmtfxosd
sudo service S60pmtfxosd stop

cat /var/log/syslog | tail  ##has all daemon messages
*/

void pmtfxos8700(void);	/* the big loop process */




/* for daemon setup */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>


/* started by S60pmtfxosd script */  
int main(int argc, char *argv[]) {

    char pmtsetpid[100];	/* for pid correction */

	/* process id and session id */
    pid_t pid, sid;

   //Fork the Parent Process
    pid = fork();

    if (pid < 0) { exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
    if (pid > 0) { exit(EXIT_SUCCESS); }

    //Change File Mask (permissions) to enable daemon access
    umask(0);

    //Create a new Signature Id for our child
    sid = setsid();
    if (sid < 0) { exit(EXIT_FAILURE); }

    //Change working directory to / = pmtroot
    if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

	/* pmt: update daemon pid which was incorrectly created by
		S60pmtfxosd script. 
	see http://unix.stackexchange.com/questions/78056/start-stop-daemon-makes-cron-pidfile-with-wrong-pid*/
    sprintf(pmtsetpid, "echo %d > /run/pmtfxosd.pid", getpid() );
    system(pmtsetpid);

	/* update /var/log/syslog */
    syslog(LOG_NOTICE, "Peter Thompson pmtfxosd connecting with fxos8700");
    syslog(LOG_NOTICE, "%s", pmtsetpid );   


  /* the big loop */
  pmtfxos8700();


}
