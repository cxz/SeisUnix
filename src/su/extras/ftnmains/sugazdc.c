/* SUGAZDC: $Revision: 1.3 $ ; $Date: 90/05/23 14:32:41 $	*/

/*----------------------------------------------------------------------
 * Copyright (c) Colorado School of Mines, 1990.
 * All rights reserved.
 *
 * This code is part of SU.  SU stands for Seismic Unix, a processing line
 * developed at the Colorado School of Mines, partially based on Stanford
 * Exploration Project (SEP) software.  Inquiries should be addressed to:
 *
 *  Jack K. Cohen, Center for Wave Phenomena, Colorado School of Mines,
 *  Golden, CO 80401  (jkc@dix.mines.colorado)
 *----------------------------------------------------------------------
 */

#include "su.h"
#include "segy.h"

/*********************** self documentation **********************/
string sdoc = "\
								\n\
SUGAZDC - const. vel. downward continuation of zero-offset data	\n\
								\n\
sugazmig dx= v= z= [optional parameters] < stdin > stdout 	\n\
								\n\
Required parameters						\n\
	dx= 		trace spacing on input data		\n\
	v= 		migration velocity (constant)		\n\
	z= 		depth level of output section		\n\
			(orig data was at z=0)			\n\
								\n\
Optional parameters						\n\
	dt= (from header) 	time sample rate (in seconds)	\n\
								\n\
";
/**************** end self doc ***********************************/

/* Credits:
 *	CWP: Chris
 *
 */


segy tr;

main(int argc, char **argv)
{
	float *data;		/* mega-vector of input data		*/
	float dt;		/* time sample rate			*/
	float dx;		/* trace spacing on input data  	*/
	float v;		/* migration velocity		  	*/
	float *wrk1;		/* work area vector 			*/
	float *wrk2;		/* work area vector 			*/
	float *wrk5;		/* work area vector 			*/
	float *wrk6;		/* work area vector 			*/
	float z;		/* downward continuation depth		*/
	int itatz;		/* time sample at the given z level 	*/
	int ix;			/* counter			 	*/
	int nalloc;		/* allocation parameter			*/
	int nt;			/* time samples per trace in input data	*/
	int ntbytes;		/* ... in bytes				*/
	int ntleft;		/* time samples below the given z level	*/
	int ntpad;		/* time samples to gazmigsub (pow of 2) */
	int ntpadbytes;		/* ... in bytes				*/
	int nx;			/* traces in input data			*/
	int nxpad;		/* traces in output data (power of 2) 	*/


	/* Initialize */
	initargs(argc, argv);
	askdoc(1);


	/*  read first trace & check that indata is time-domain */ 
	if (!gettr(&tr)) err("can't get first trace\n");
	if (tr.trid != TREAL && tr.trid != FPACK && tr.trid != 0) {  
		err("input is not x-domain data, trid=%d",tr.trid);
	}
	if (tr.trid == FPACK) {
		err("not ready for fpack, trid=%d",tr.trid);
	}

	/*  get nt from header, pad to a power of 2 */
	nt = tr.ns;	
	ntbytes = nt * FSIZE;
	for (ntpad = 1; ntpad < nt; ntpad *= 2);
	ntpadbytes = ntpad * FSIZE;

	/* get dt from header or user */
	if (!fgetpar("dt", &dt))	dt = tr.dt/1000000.;

	/*  dx, v and zmust be given by user */ 
	MUSTFGETPAR("dx", &dx);
	if (dx <= 0) err("dx=%d :should be > 0", dx);
	MUSTFGETPAR("v", &v);
	if (v < 0) err("v=%d :should be positive or zero", v);
	MUSTFGETPAR("z", &z);
	if (z < 0) err("z=%d :should be positive", z);

	/*  calc time sample at this z and number of samples left*/ 
	itatz = 2*z / ( v*dt ) + 1; 
	ntleft = nt - itatz;

	/* Alloc block of memory for data 	*/
	nalloc = MAX(NFALLOC, ntpad); 
	data = ealloc1float(nalloc);


	/* Loop over input traces & put them into data mega-vector */
	nx = 0;
	do {
		++nx;
		if (nx*ntpad > nalloc) { /* need more memory */	
			nalloc += NFALLOC;
			data = erealloc1float(data, nalloc);
		}
		bcopy(tr.data, data + (nx - 1)*ntpad, ntpadbytes); 
	} while (gettr(&tr));


	/* FFTs to come later, so pad number of traces 
	   to power of 2 and allocate appropriate space  */
	for (nxpad = 1; nxpad < nx; nxpad *= 2);
	if (nxpad*ntpad > nalloc) { 
	    data = erealloc1float(data, ntpad*nxpad); 
	}

	/* Print some info for the user */
	warn(" for SUGAZDC: 	dx = %g	    v  = %g     z = %g", dx, v, z);
	warn("input/output: 	nt = %d     nx = %d", nt, nx);
	warn("   work area: 	nt = %d     nx = %d",  ntpad, nxpad);

	/* Alloc memory for outdata and work areas 	*/
	wrk1 	= ealloc1float(ntpad*nxpad);
	wrk2 	= ealloc1float(2*ntpad*nxpad);
	wrk5 	= ealloc1float(nxpad);
	wrk6 	= ealloc1float(ntpad);

	/* Zero-out the padded traces */
	if (nxpad > nx) bzero(data + nx*ntpad, (nxpad-nx)*ntpadbytes); 

	/* Gazdag downward continuation (const. vel) */
	gazdcsub_(data, &ntpad, &dt, &nxpad, &dx, &v, &z,      
		   wrk1, wrk2, wrk5, wrk6);      

	/* Output the result by pulling traces off data mega-vector */
	for (ix = 0; ix < nx; ix++) {
		/* Zero-out data below original max time */
		bzero(data + ix*ntpad + ntleft, itatz*FSIZE); 
		bcopy(data + ix*ntpad, tr.data, ntbytes); 
		tr.trid = TREAL;
		tr.ntr = nx;
		tr.ns = nt;
		tr.dt = dt*1000000.0;
		tr.tracl = ix;
		tr.cdp = ix;
		puttr(&tr);
	}

	
	return EXIT_SUCCESS;
}
