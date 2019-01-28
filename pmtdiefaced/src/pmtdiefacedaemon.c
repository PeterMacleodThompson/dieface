/* pmtdiefacedaemon.c    - set up a daemon process called pmtdiefaced
compile with 
gcc -o ../pmtdiefaced diefacehandler.c diefacemain.c pmtdiefacedaemon.c	-I./include -lrt
OR
arm-linux-gnueabihf-gcc -o ../pmtdiefaced diefacehandler.c diefacemain.c pmtdiefacedaemon.c	 -I./include  -lrt


   copied from
	http://shahmirj.com/blog/beginners-guide-to-creating-a-daemon-in-linux
   with reference from  
   	http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html

daemon name = pmtdiefaced =  defined in S70pmtdiefaced script
sudo service S70pmtdiefaced start  
sudo service S70pmtdiefaced stop

cat /var/log/syslog | tail  ##has all daemon messages

*/

void diefacemain(void);

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


/* started by S70pmtdiefaced script */  
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

    //Change Directory
    //If we cant find the directory we exit with failure.
    if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

	/* pmt: update daemon pid which was incorrectly created by
		S70pmtdiefaced script. 
	see http://unix.stackexchange.com/questions/78056/start-stop-daemon-makes-cron-pidfile-with-wrong-pid*/
    sprintf(pmtsetpid, "echo %d > /run/pmtdiefaced.pid", getpid() );
    system(pmtsetpid);

	/* update /var/log/syslog */
    syslog(LOG_NOTICE, "Peter Thompson pmtdiefaced started");
    syslog(LOG_NOTICE, "%s", pmtsetpid );   

    /* the big loop */ 
    diefacemain();


}
