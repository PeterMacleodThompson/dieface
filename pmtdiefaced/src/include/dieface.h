/* Peter Thompson - March 2016 - revised Dec 2018 */

/**************** valid actions when rolling the die *****************
	internal numbers identify dieface facing up for less than 3 seconds 
    first and last numbers show same steady state of die before/after roll
	   where die is facing up for more than 3 seconds 
	action must be <= 19 digits */

	/* https://embeddedgurus.com/stack-overflow/2010/01/a-tutorial-on-lookup-tables-in-c/ */

/* for actions, dieface up only counts. Dieface direction (yaw) is ignored */
#define    NOACTION  0
#define    UNDOCUMENTED  99
#define    FRONTBOW   151
#define    BACKBOW   121
#define    LEFTBANK   141
#define    RIGHTBANK   131
#define    FRONTFLIP   15621
#define    BACKFLIP   12651
#define    LEFTROLL   14631
#define    RIGHTROLL   13641
#define    FRONTSEMIFLIP   15651
#define    BACKSEMIFLIP   12621
#define    LEFTSEMIROLL   14641
#define    RIGHTSEMIROLL   13631
#define    FRONT2FLIP   156215621
#define    BACK2FLIP   126512651
#define    LEFT2ROLL   146314631
#define    RIGHT2ROLL   136413641

#define		VSS			1	/* Valid Start State */
#define		VA			17	/* Valid Action */

#define NUM(validAction) (sizeof(validAction)/sizeof(validAction[0]))

/********* shared memory results of rolling the die ********/

struct PMTdieEvent {
	short 		id;			/* avoid duplicate events */					
	int 		diefaceSS;	/* Steady State - see fxos8700.h for diefaceUSS */ 
	long long 	dieaction;	/* max 19 digits - describes roll */
};
    

/******** valid action ***********************************/
	static const long long validAction[] = {
	NOACTION, UNDOCUMENTED, 
	FRONTBOW, BACKBOW, LEFTBANK, RIGHTBANK,
	FRONTFLIP, BACKFLIP, LEFTROLL, RIGHTROLL, 
	FRONTSEMIFLIP, BACKSEMIFLIP, LEFTSEMIROLL, RIGHTSEMIROLL,
	FRONT2FLIP, BACK2FLIP, LEFT2ROLL, RIGHT2ROLL
	}; 


