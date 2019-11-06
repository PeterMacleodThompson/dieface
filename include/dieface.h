/* Peter Thompson - March 2016 - revised Dec 2018 */

/**
 * DOC: -- dieface.h -- Peter Thompson March 2016
 * revised Dec 2018, Oct 2019
 *
 * diefaceSS = dieface Steady State
 *           = diefaceup does not change for >= 3 seconds
 * dieaction = list of diefaceup's between 2 steady states
 *           = diefaceup's lasting < 3 sec
 *
 * FIXME 3 sec too slow - use 1 sec?   fxos too slow, misses fast hand actions.
 */

/********* shared memory results of rolling the die ********/

struct PMTdieEvent {
  short id;            /* identify repeated events */
  int diefaceSS;       /* Steady State - see fxos8700.h for diefaceUSS */
  long long dieaction; /* max 19 digits - describes roll */
};
