/* dieface.c
  PeterThompson April 2015 
  extracts dieface activity from getdieface() and returns action + state   

	hacked from "Writing efficient state machines in C"
    If you don't understand state machines, read this first
  http://johnsantic.com/comp/state.html 

---------------------------------------------------------
  dieface.c  is a two-state machine for fxos8700 motion
     it simulates rolling a single die. 

Standard Position (SP):  
	1 up
	2 North- true, not magnetic
	3 West
	4 East
	5 South
	6 Down
There are 5 other (total 6) die positions depending on what is facing up
 after action (the die is rolled).   The Standard Position MUST have
 dieface 2 facing North to be valid.  

The following STATE MACHINE  is based on starting in Standard Position SP:
states (2 of them)  SS,USS
    steady state: SS:   (can be any die position that hasn't moved for 3 seconds)
    unsteady state:  USS: (the die is rolling, no dieface the "winner")


events (2 of them)
    new dieface appears: onroll:  eg dieface1 to dieface6
    clock reaches 3 seconds: offroll: eg current (new) dieface is steady state

    
outputs
    new dieface 
    some action (TBD)
	frontBow, backBow, leftBank, rightBank, frontFlip, backFlip, leftRoll, rightRoll,  frontsemiFlip, backsemiFlip, rightsemiRoll, leftsemiRoll, front2Flip, back2Flip, right2Roll, left2Roll, rotate -60 to 60 (+ is clockwise, - is counterclockwise)  ==> 17 actions.
--------------------------------------------------------------------
*/
#define DEBUG	/* print to /var/log/messages */


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <syslog.h>   	/* for syslog */
#include "dieface.h"

/* note the 5+2=7 external variables */
long long	action;		/* stores all newdieface activity as number */
time_t 		clockbegin;	
int 		diefaceNow, diefaceNext;
int			resetDF;	/* reset dieface flag */


/***********************  the state machine defined *********************/

/* there are 2 states */
enum states { 	SS, 		/* Steady State, die is stationary */
				USS			/* Unsteady State, die is rolling */
	     } statenow;		

/* there are 2 events */
enum events { 	onroll, 	/* die rolling; a new dieface popped up */
				offroll		/* die stopped rolling for >= 3 sec */
	    } newevent;



/* the 4 function prototypes: 2 states x 2 events */
void SSonroll(void);		/* Steady State + die starts rolling */
void SSoffroll(void);		/* Steady State + die not rolling*/
void USSonroll(void);		/* Unsteady State + continue rolling */
void USSoffroll(void);		/* Unsteady State + stop rolling */	



/* the state/event function lookup table */
void (*const statetable[2][2]) (void) = {
		{SSonroll,  SSoffroll},	/* Steady State functions */
		{USSonroll, USSoffroll}	/* Unsteady State functions */
		};






/**********  the 4 functions: 2 states x 2 events ****************/

/* function[1,1] = Steady State + die started rolling */
void SSonroll(void) 	
{
    action = diefaceNow/10; 		/* action 1st digit = steady state digit */
    action = action*10 + diefaceNext/10; 	/* action 2nd digit */
	if( action > 999999999)  
		action = 99;
    statenow = USS;
    clockbegin = time(NULL);
    return;
}

/* function[1,2] = Steady State + die not rolling,  */
void SSoffroll(void)
{
    return;
}

/* function[2,1] = Unsteady State + die continues rolling */
void USSonroll(void)
{
    action = action*10 + diefaceNext/10; 	/* diefaceNew digit added to action */
    clockbegin = time(NULL);
    return;
}

/* function[2,2] = Unsteady State + die stops rolling */
void USSoffroll(void)	
{
	char debuglog[500];		// printer string sent to syslog	
	int i;
	int actionfinal;

    /* state machine reset */
    statenow = SS;
    clockbegin = time(NULL);
	
	/* validate action */
	actionfinal = UNDOCUMENTED;
	for( i=0; i<NUM(validAction); i++ )
		if(action == validAction[i])   
			actionfinal = action;
		
	if(actionfinal == UNDOCUMENTED)	{
		sprintf(debuglog,"Action NOT Found = %lld\n", action );	
		syslog(LOG_INFO, "%s", debuglog );	
	}
	/* set flags to reset shared memory */ 
	resetDF = 1;  /* TRUE */
	action = actionfinal;

	diefaceNow = diefaceNext;
    return;
}









/*****************   diefaceinit ****************************/
void diefaceinit(struct PMTdieEvent *df)
{
	/* initialize the 6 external variables */
    clockbegin = time(NULL);
    statenow = SS;
    newevent = offroll;
    diefaceNow = 12;
	diefaceNext = 12;
    action = 99;   	/* default UNDOCUMENTED */
 
	/* set dieface shared memory */
	df->id = 1;						
	df->diefaceSS = 12;  	
	df->dieaction = 99;	
}


/*****************   diefacehandler ****************************/
void diefacehandler(struct PMTdieEvent *df, int diefaceNew )
{
	/* NOTE diefaceNew is 2 digits (up+North), and must be 
	converted to 1 digit (up) via /10 before saving as action */

    double elapsed;
	char debuglog[500];		// printer string sent to syslog	

    /* compute elapsed time since last diefaceNew event */
    elapsed = ((double) (time(NULL) - clockbegin) );
	diefaceNext = diefaceNew;

    /* parse events */
    if( diefaceNew == diefaceNow && elapsed <= 3.0) 
		return;
    else if( diefaceNew == diefaceNow && elapsed > 3.0 ) 
		newevent = offroll;
    else if( diefaceNew != diefaceNow)
		newevent = onroll;	

	/* state machine */
	resetDF = 0;  /* FALSE */
    statetable[statenow][newevent] ();


	if( resetDF == 1 ) {
		/* reset dieface = new steady state + final action*/
		df->id++;						/* ensures uniqueness */
		df->diefaceSS = diefaceNext;  	
		df->dieaction = action;	
	}
 
	diefaceNow = diefaceNext; 
    return;

}







