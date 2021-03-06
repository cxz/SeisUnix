/* SUTWIG4: $Revision: 1.25 $ ; $Date: 89/09/23 19:04:55 $		*/

/*----------------------------------------------------------------------
 * Copyright (c) Colorado School of Mines, 1989.
 * All rights reserved.
 *
 * This code is part of SU.  SU stands for Seismic Unix, a processing line
 * developed at the Colorado School of Mines, partially based on Stanford
 * Exploration Project (SEP) software.  Inquiries should be addressed to:
 *
 *  Jack K. Cohen, Center for Wave Phenomena, Colorado School of Mines,
 *  Golden, CO 80401  (isis!csm9a!jkcohen)
 *----------------------------------------------------------------------
 */

#include "cwp.h"
#include "segy.h"
#include "fconst.h"

/************************ self documentation *****************************/
string sdoc = "\
									\n\
SUTWIG4 - wiggle trace program for data plotting of four data files 	\n\
									\n\
sutwig4 data1 data2 data3 data4 [optional parameters] | tube		\n\
									\n\
NOTE1:  The data files MUST have equal nt.	 			\n\
									\n\
Optional Parameters:							\n\
....LABELING ...							\n\
	title1	= File1		title for plot #1 (upper left)		\n\
	title2	= File2		title for plot #2 (upper right)		\n\
	title3	= File3		title for plot #3 (lower left)		\n\
	title4	= File4		title for plot #4 (lower right)		\n\
	titlsz = 3		title print size			\n\
....MISCELLANEOUS ...							\n\
	hgap	= 0.2		horizontal gap between plots (inches)	\n\
	vgap	= 0.7		vertical gap between plots (inches)	\n\
	ntict   = 5		number of tics on time axis		\n\
	nticx   = 4		number of tics on trace axis		\n\
	tlines	= 1		flag for timing lines (=0 for none)	\n\
	fill	= 1		flag for positive fill 			\n\
				(=0 for no fill)			\n\
	ltic    = 0.05		length of tic mark (inches)		\n\
	plotfat	= 0		line thickness of traces		\n\
	axisfat	= 0		line thickness of box & tics		\n\
....SIZE & LOCATION ... (defaults OK for hardcopy)			\n\
	sizet	= 2.7		length of t-axis (inches)		\n\
	sizex	= 2.7		length of x-axis (...)			\n\
	margint	= 0.05		top/bot gap between box and 		\n\
				traces (...)				\n\
	marginx	= 0.05		side gap between box and traces (...)	\n\
	zerot	= 0.8		base of plot to bottom of screen (...)	\n\
	zerox	= 0.2		left of plot to left of screen (...)	\n\
....GAINING ...	(same for all plots)					\n\
	rel 	= 0		plots scaled independently 		\n\
				=1 for relative scaling (max required)	\n\
	max	= none		global max for relative scaling		\n\
				required if rel =1			\n\
	overlap = 2.0		max deflection (in traces) is 		\n\
				overlap*scale				\n\
									\n\
gain defaults (see sugain):						\n\
	tpow=0.0 epow=0.0 gpow=1.0 agc=0 wagc=20			\n\
	trap=0.0 clip=0.0 qclip=1.0 qbal=1 pbal=0 scale=1.0		\n\
									\n\
									\n\
NOTE2: Axis/tic labels sacrificed to maximize plot size on hardcopy	\n\
";
/*************************************************************************/

/* Credits:
 *	CWP: Chris
 *
 *
 */


/* Embed Revision Control System identifier strings */
static string progid =
	"   $Source: /src/su/src/RCS/sutwig4.c,v $";
static string revid =
	"   $Revision: 1.25 $ ; $Date: 89/09/23 19:04:55 $";




/* Set gain defaults (balance by maximum magnitude) */
#define TPOW	0.0
#define EPOW	0.0
#define GPOW	1.0
#define AGC 	0
#define WAGC	20
#define TRAP	0.0
#define CLIP	0.0
#define QCLIP	1.0
#define QBAL	1	/* default is balance by maximum magnitude 	*/
#define PBAL	0
#define SCALE	1.0

segy tr;


main(argc, argv)
int argc; char **argv;
{
	float absmax;		/* absolute max sample on trace 	*/
	int absmaxloc;		/* ... its location (not used) 		*/
	float *dataptr;		/* mega-vector of data from the segys	*/
	float givenmax;		/* maximum in data panel for absol scaling*/
	float dt;		/* time sample rate (and tic increment)	*/
	float dx;		/* tic label increment for horiz axis	*/
	float scale;		/* scale for data gaining		*/
	float tmin;		/* minimum time (and tic label) for dat2*/
	float xmin;		/* minimum tic label for horiz axis	*/
	int fd1,fd2,fd3,fd4;	/* file descriptors of input files	*/
	int ffile;		/* file flag for plotting sub		*/
	int n;			/* nt*ntr				*/
	int nt;			/* time samples per trace (from tr.ns)	*/
	int ntsize;		/* number of data bytes on a trace	*/
	int ntr;		/* traces to be plotted			*/
	int tracl2;		/* tracl of 2nd trace (for dx)		*/
	int ndata;		/* allocation for mega-vector		*/
	int relamp;		/* relative amplitude plotting flag	*/
	string label1;		/* vertical axis label (default Time)	*/
	string label2;		/* horiz axis label (default Trace)	*/
	string title1;		/* title on plot1			*/
	string title2;		/* title on plot2			*/
	string title3;		/* title on plot3			*/
	string title4;		/* title on plot4			*/
	void gain();		/* see su/lib/gainpkge.c		*/
	void wigplot();		/* isolate cplot commands		*/
	void vertwig();		/* draw vertical wiggle traces		*/


	/* Initialize SU */
	initargs(argc, argv);
	if (!igetpar("ID", &ID))	ID = 0;
	if (ID) {
		(void) fprintf(stderr, "%s\n", progid);
		(void) fprintf(stderr, "%s\n", revid);
	}
	askdoc(4);	/* four file args required */


	/* Prevent bytes from spilling onto screen */
	if (isatty(STDOUT)) {
		err("must redirect or pipe byte code output");
	}


	/* Open four files given as arguments for reading */
	if (-1 == (fd1 = open(argv[1], 0))) {
		syserr("can't open %s\n", argv[1]);
	}
	if (-1 == (fd2 = open(argv[2], 0))) {
		syserr("can't open %s\n", argv[2]);
	}
	if (-1 == (fd3 = open(argv[3], 0))) {
		syserr("can't open %s\n", argv[3]);
	}
	if (-1 == (fd4 = open(argv[4], 0))) {
		syserr("can't open %s\n", argv[4]);
	}


	/* Read in and plot first (upper left) input file */

	/* Read first trace */ 
	if (!fgettr(fd1,&tr)) err("can't get first trace");
	ffile =1;		/* working on file 1 */
	nt = tr.ns;
	xmin = tr.tracl;	fgetpar("xmin", &xmin);
	ntsize = nt * FSIZE;

	tmin = tr.delrt/1000.0;	fgetpar("tmin", &tmin);
				/* tr.delrt is in millisecs */

	/* Set or get dt for tic labelling */
	if (!fgetpar("dt", &dt)) {
	    if (tr.dt) {  /* is dt field set? */
		    dt = tr.dt / 1000000.0;
	    } else {		/* dt not set, assume 4 ms */
		    dt = 0.004;
		    warn("tr.dt not set, for labeling assume dt=%g", dt);
	    }
	}
	

	/* Allocate block of memory for data float mega-vector */
	ndata = MAX(NFALLOC, nt); /* alloc at least one trace */
	dataptr = vec(ndata);


	/* Loop over input traces & put them into data mega-vector */
	ntr = 0;
	do {
		++ntr;
		if (ntr == 2) tracl2 = tr.tracl;/* needed for dx    */
		if (ntr*nt > ndata) {		/* need more memory */
			ndata <<= 1;		/* ask for double   */
			dataptr = re_vec(dataptr, ndata);
		}
		bcopy(tr.data, dataptr + (ntr - 1)*nt, ntsize); 
	} while(fgettr(fd1, &tr));

	dx = tracl2 - xmin;	fgetpar("dx", &dx);


	/* See if user wants relative scaling between plots */
	relamp = 0;		igetpar("rel",&relamp);
	if (relamp == 1) {
		MUSTFGETPAR("max",&givenmax);
	}

	/* For rel amp plots find max value in data for scaling */
	scale = 1.0;
	if (relamp == 1) {
		n = nt*ntr;
		maxmgv_(dataptr, ONE, &absmax, &absmaxloc, &n);
		scale = absmax/givenmax;
	}

	/* Gain */
	gain(dataptr, TPOW, EPOW, GPOW, AGC, TRAP, CLIP, QCLIP,
			QBAL, PBAL, scale, tmin, dt, WAGC, nt, ntr);

	/* Plot getpars */
	title1 = "File 1";		sgetpar("title1",  &title1);
	label1 = "";		sgetpar("label1", &label1);
	label2 = "";		sgetpar("label2", &label2);


	/* Plot */
	wigplot(dataptr,nt, ntr, title1, label1, label2, 
			tmin, dt, xmin, dx, ffile);


	/* Read in and plot second (upper right) input file */

	/* Read first trace */ 
	if (!fgettr(fd2,&tr)) err("can't get first trace");
	ffile = 2;		/* working on file 2 */
	nt = tr.ns;
	xmin = tr.tracl;	fgetpar("xmin", &xmin);
	ntsize = nt * FSIZE;


	/* Allocate block of memory for data float mega-vector */
	ndata = MAX(NFALLOC, nt); /* alloc at least one trace */
	dataptr = vec(ndata);


	/* Loop over input traces & put them into data mega-vector */
	ntr = 0;
	do {
		++ntr;
		if (ntr == 1) tracl2 = tr.tracl;/* needed for dx    */
		if (ntr*nt > ndata) {		/* need more memory */
			ndata <<= 1;		/* ask for double   */
			dataptr = re_vec(dataptr, ndata);
		}
		bcopy(tr.data, dataptr + (ntr - 1)*nt, ntsize); 
	} while(fgettr(fd2, &tr));

	dx = tracl2 - xmin;	fgetpar("dx", &dx);


	/* For rel amp plots find max value in data for scaling */
	if (relamp == 1) {
		n = nt*ntr;
		maxmgv_(dataptr, ONE, &absmax, &absmaxloc, &n);
		scale = absmax/givenmax;
	}

	/* Gain */
	gain(dataptr, TPOW, EPOW, GPOW, AGC, TRAP, CLIP, QCLIP,
			QBAL, PBAL, scale, tmin, dt, WAGC, nt, ntr);

	/* Plot getpars */
	title2 = "File 2";		sgetpar("title2",  &title2);


	/* Plot */
	wigplot(dataptr,nt, ntr, title2, label1, label2, 
			tmin, dt, xmin, dx, ffile);



	/* Read in and plot third (lower left) input file */

	/* Read first trace */ 
	if (!fgettr(fd3,&tr)) err("can't get first trace");
	ffile = 3;		/* working on file 3 */
	nt = tr.ns;
	xmin = tr.tracl;	fgetpar("xmin", &xmin);
	ntsize = nt * FSIZE;


	/* Allocate block of memory for data float mega-vector */
	ndata = MAX(NFALLOC, nt); /* alloc at least one trace */
	dataptr = vec(ndata);


	/* Loop over input traces & put them into data mega-vector */
	ntr = 0;
	do {
		++ntr;
		if (ntr == 1) tracl2 = tr.tracl;/* needed for dx    */
		if (ntr*nt > ndata) {		/* need more memory */
			ndata <<= 1;		/* ask for double   */
			dataptr = re_vec(dataptr, ndata);
		}
		bcopy(tr.data, dataptr + (ntr - 1)*nt, ntsize); 
	} while(fgettr(fd3, &tr));

	dx = tracl2 - xmin;	fgetpar("dx", &dx);


	/* For rel amp plots find max value in data for scaling */
	if ( relamp == 1 ) {
		n = nt*ntr;
		maxmgv_(dataptr, ONE, &absmax, &absmaxloc, &n);
		scale = absmax/givenmax;
	}

	/* Gain */
	gain(dataptr, TPOW, EPOW, GPOW, AGC, TRAP, CLIP, QCLIP,
			QBAL, PBAL, scale, tmin, dt, WAGC, nt, ntr);

	/* Plot getpars */
	title3 = "File 3";		sgetpar("title3",  &title3);


	/* Plot */
	wigplot(dataptr,nt, ntr, title3, label1, label2, 
			tmin, dt, xmin, dx, ffile);



	/* Read in and plot fourth (lower right) input file */

	/* Read first trace */ 
	if (!fgettr(fd4,&tr)) err("can't get first trace");
	ffile = 4;		/* working on file 4 */
	nt = tr.ns;
	xmin = tr.tracl;	fgetpar("xmin", &xmin);
	ntsize = nt * FSIZE;


	/* Allocate block of memory for data float mega-vector */
	ndata = MAX(NFALLOC, nt); /* alloc at least one trace */
	dataptr = vec(ndata);


	/* Loop over input traces & put them into data mega-vector */
	ntr = 0;
	do {
		++ntr;
		if (ntr == 1) tracl2 = tr.tracl;/* needed for dx    */
		if (ntr*nt > ndata) {		/* need more memory */
			ndata <<= 1;		/* ask for double   */
			dataptr = re_vec(dataptr, ndata);
		}
		bcopy(tr.data, dataptr + (ntr - 1)*nt, ntsize); 
	} while(fgettr(fd4, &tr));

	dx = tracl2 - xmin;	fgetpar("dx", &dx);


	/* For rel amp plots find max value in data for scaling */
	if (relamp == 1) {
		n = nt*ntr;
		maxmgv_(dataptr, ONE, &absmax, &absmaxloc, &n);
		scale = absmax/givenmax;
	}

	/* Gain */
	gain(dataptr, TPOW, EPOW, GPOW, AGC, TRAP, CLIP, QCLIP,
			QBAL, PBAL, scale, tmin, dt, WAGC, nt, ntr);

	/* Plot getpars */
	title4 = "File 4";		sgetpar("title4",  &title4);


	/* Plot */
	wigplot(dataptr,nt, ntr, title4, label1, label2, 
			tmin, dt, xmin, dx, ffile);

	endplot();


	return SUCCEED;
}



void wigplot(dataptr, nt, ntr, title, label1, label2, 
			tmin, dt, xmin, dx, ffile)
float *dataptr;
int nt, ntr;
string title, label1, label2;
float tmin, dt, xmin, dx;
int ffile;
{
	float hgap;		/* vertical gap between plots		*/
	float vgap;		/* horizontal gap between plots		*/
	float ltic;		/* length of tic marks (inches)		*/
	float margint;		/* top/bot gap between box and traces	*/
	float marginx;		/* side gap between box and traces 	*/
	float mt;		/* margint/scalet			*/
	float mx;		/* marginx/scalex			*/
	float overlap;		/* maximum trace overlap		*/
	float scalet;		/* time axis scale			*/
	float scalex;		/* trace axis scale			*/
	float sizet;		/* length of t-axis (inches)		*/
	float sizex;		/* length of x-axis (inches)		*/
	float tpos;		/* temp for time position		*/
	float xpos;		/* temp for trace position		*/
	float zerot;		/* base of plot to bot. of screen	*/
	float zerox;		/* left of plot to left of screen	*/
	int axisfat;		/* line thickness of box & tics		*/
	int dtict;		/* distance between time tics		*/
	int dticx;		/* distance between trace tics		*/
	int fill;		/* fill flag				*/
	int lablsz;		/* label print size			*/
	int ntict;		/* number of tics on time axis		*/
	int nticx;		/* number of tics on trace axis		*/
	int plotfat;		/* line thickness of traces		*/
	int ticsz;		/* tic labeling print size		*/
	int titlsz;		/* title print size			*/
	int tlines;		/* 1=timing lines (0=no timing lines)	*/
	register int i;		/* counter				*/


	hgap = .2;		fgetpar("hgap",&hgap);
	vgap = .7;		fgetpar("vgap",&vgap);

	fill = 1;		igetpar("fill", &fill);

	overlap = 2.0;		fgetpar("overlap", &overlap);

	zerot = .8;		fgetpar("zerot", &zerot);
	zerox = .2;		fgetpar("zerox", &zerox);

	sizet = 2.7;		fgetpar("sizet", &sizet);
	sizex = 2.7;		fgetpar("sizex", &sizex);

	scalet = -sizet/nt;
	scalex = sizex/MAX(ntr, 8);

	margint = 0.05;		fgetpar("margint", &margint);
	marginx = 0.05;		fgetpar("marginx", &marginx);

	ltic = 0.05;		fgetpar("ltic", &ltic);

	plotfat = 0;		igetpar("plotfat", &plotfat);
	axisfat = 0;		igetpar("axisfat", &axisfat);

	titlsz = 3;		igetpar("titlsz", &titlsz);
	lablsz = 4;		igetpar("lablsz", &lablsz);
	ticsz = 3;		igetpar("ticsz", &ticsz);

	tlines = 1;		igetpar("tlines", &tlines);

	ntict = 5;		igetpar("ntict", &ntict);
	dtict = nt/ntict;

	nticx = 4;		igetpar("nticx", &nticx);
	dticx = MAX(ntr/nticx, 1);
	
	/* Plots 1 and 2 are high */ 
	if ( ffile <= 2 ) {
		zerot = zerot + sizet + vgap;
	}
	
	/* Move plots 2 and 4 to the right */ 
	if ( ffile == 2  || ffile==4 ) {
		zerox = zerox + sizex + hgap;
	}

	setscl(scalex, scalet);
	set0(zerox, zerot + sizet);
	setu0(0,0);
	setfat(axisfat);

	mx = marginx/scalex;  mt = margint/scalet; 

	/* Title */
	setcol(1);  
	uText(0.5*ntr, mt + 0.25/scalet, titlsz, 0, title);

	/* Axis box */
	setcol(3);  
	umove( -mx                  ,                  mt ); 
	udraw( -mx                  , (float) (nt-1) - mt );
	udraw( (float) (ntr-1) + mx , (float) (nt-1) - mt ); 
	udraw( (float) (ntr-1) + mx ,                  mt );
	udraw( -mx                  ,                  mt );

	/* Vertical axis tic marks */
	for (i = 0; i <= nt; i += dtict) {
		umove( -mx, (float) i );
		where( &xpos , &tpos );
		draw( xpos-ltic, tpos );
		if (tlines == 1) {
			umove( -mx , (float) i );
			udraw( (float) (ntr-1) + mx , (float) i); 
		}	
	        umove( mx + (ntr-1) , (float) i );
	        where( &xpos , &tpos);
	        draw( xpos + ltic , tpos );
	}

	/* Horizontal axis tic marks */
	for (i = 0; i < ntr; i++) {
	        umove( (float) i , mt );
	        where( &xpos , &tpos );
	        draw( xpos , tpos + ltic );
	        if (!(i % dticx)) {		/* top tics */
		    where(&xpos, &tpos);
	    	    draw(xpos, tpos + ltic);
	        }
	        umove( (float) i , - mt + (nt-1) );
	        where(&xpos, &tpos);
	        draw(xpos, tpos - ltic);
	        if (!(i % dticx)) {		/* bottom tics */
		    where(&xpos, &tpos);
	    	    draw(xpos, tpos - ltic);
		}
	}

	/* Draw wiggle traces */
	setcol(2);  
	setfat(plotfat);
	setscl(scalex*overlap, scalet);
	for (i = 0; i < ntr; i++) {
		setu0(-(float) i / overlap, 0.0); 
		vertwig(dataptr + nt*i, nt, fill);
	}
	return;
}
