/* suplane - makes a data set with up to 3 plane reflectors
 *
 * Credits:
 *	CWP: Chris
 *
 *----------------------------------------------------------------------
 * Permission to use the SU programs is granted.  Courteous users will
 * retain this acknowlegment of origins at the Stanford Exploration
 * Project, Stanford University and the Center for Wave Phenomena, 
 * Colorado School of Mines.   Inquiries/bug reports should be sent to:
 * Jack K. Cohen, Center for Wave Phenomena, Colorado School of Mines,
 * Golden, CO 80014
 *----------------------------------------------------------------------
 *
 * $Author: jkc $
 * $Source: /src/su/src/RCS/suplane.c,v $
 * $Revision: 1.6 $ ; $Date: 88/05/24 23:22:22 $
*/

/*********************** self documentation **********************/
char *sdoc = "\
SUPLANE - create common offset data file with up to 3 planes	\n\
								\n\
suplane [optional parameters] > out_data_file  			\n\
								\n\
Optional parameters:						\n\
	npl = 3		number of planes			\n\
	nt = 64 	number of time samples			\n\
	ntr = 32	number of traces			\n\
	taper = 0	no end-of-plane taper			\n\
			1 = taper planes to zero at the end	\n\
 	offset = 400 	offset					\n\
 	dt = 0.004 	time sample rate in seconds		\n\
 plane 1 ...							\n\
	dip1 = 0	dip of plane #1 (ms/trace)		\n\
	len1 = 3*ntr/4	HORIZONTAL extent of plane (traces)	\n\
	ct1 = nt/2	time sample for center pivot	 	\n\
	cx1 = ntr/2	trace for center pivot			\n\
 plane 2 ...							\n\
	dip2 = 2	dip of plane #1 (ms/trace)		\n\
	len2 = 3*ntr/4	HORIZONTAL extent of plane (traces)	\n\
	ct2 = nt/2	time sample for center pivot 		\n\
	cx2 = ntr/2	trace for center pivot			\n\
 plane 3 ...							\n\
	dip3 = 4	dip of plane #1 (ms/trace)		\n\
	len3 = 3*ntr/4	HORIZONTAL extent of plane (traces)	\n\
	ct3 = nt/2	time sample for center pivot		\n\
	cx3 = ntr/2	trace for center pivot			\n\
								\n\
	liner = 0	use parameters 				\n\
			= 1 parameters set for 64x64 data set   \n\
			with separated dipping planes.		\n\
";
/*****************************************************************/

/* Embed Revision Control System identifier strings */
static char progid[] =
	"   $Source: /src/su/src/RCS/suplane.c,v $";
static char revid[] =
	"   $Revision: 1.6 $ ; $Date: 88/05/24 23:22:22 $";

#include "../include/cwp.h"
#include "../include/segy.h"

segy tr;

main(argc,argv)
int argc; char **argv;
{
	float dip1;		/* time-dip of plane 1 (ms/trace)	*/
	float dip2;		/* time-dip of plane 2 (ms/trace)	*/
	float dip3;		/* time-dip of plane 3 (ms/trace)	*/
	float dt;		/* time sample rate in seconds 		*/
	float eps;		/* fit - itless (for linear interpol)	*/
	float fit;		/* fractional time sample		*/
	float offset;		/* constant offset for header 		*/
	int ct1,cx1;		/* center of plane 1 (sample and trace)	*/
	int ct2,cx2;		/* center of plane 2 (sample and trace)	*/
	int ct3,cx3;		/* center of plane 3 (sample and trace)	*/
	int itless;		/* sample above plane intersection	*/
	int itmore;		/* sample below plane intersection 	*/
	int itr;		/* trace counter			*/
	int len1;		/* HORIZ extent of plane 1		*/
	int len2;		/* HORIZ extent of plane 2		*/
	int len3;		/* HORIZ extent of plane 3		*/
	int liner;		/* flag for special output section	*/
	int msps;		/* milliseconds per sample 		*/
	int npl;		/* number of planes 			*/
	int nt;			/* time samples in outdata		*/
	int ntr;		/* traces in outdata			*/
	int tfe1;		/* traces-from-end of plane 1 (for taper) */
	int tfe2;		/* traces-from-end of plane 2 (for taper) */
	int tfe3;		/* traces-from-end of plane 3 (for taper) */
	int taper;		/* flag to taper plane ends to zero	*/


	/* Stdin not used and no parameter arguments required */
	initgetpar(argc, argv); askdoc(0);


	/* set parameters and fill header fields */
	nt = 64;	igetpar("nt", &nt);		tr.ns = nt;
	ntr = 32;	igetpar("ntr", &ntr);		tr.ntr = ntr;
	dt = 0.004;	fgetpar("dt", &dt);		tr.dt = dt*1000000;
	offset = 400;	fgetpar("offset", &offset);	tr.offset = offset;
	npl = 3;	igetpar("npl", &npl);
	taper = 0;	igetpar("taper", &taper);

	/* set defaults and/or get parameters for plane 1 */
	dip1 = 0;	fgetpar("dip1", &dip1);
	len1 = 3*ntr/4;	igetpar("len1", &len1);
	ct1 = nt/2;	igetpar("ct1", &ct1);		ct1 -= 1;		
	cx1 = ntr/2;	igetpar("cx1", &cx1);		cx1 -= 1;

	/* set defaults and/or get parameters for plane 2 */
	dip2 = 2;	fgetpar("dip2", &dip2);
	len2 = 3*ntr/4;	igetpar("len2", &len2);
	ct2 = nt/2;	igetpar("ct2", &ct2);		ct2 -= 1;
	cx2 = ntr/2;	igetpar("cx2", &cx2);		cx2 -= 1;	

	/* set defaults and/or get parameters for plane 3 */
	dip3 = 4;	fgetpar("dip3", &dip3);
	len3 = 3*ntr/4;	igetpar("len3", &len3);
	ct3 = nt/2;	igetpar("ct3", &ct3);		ct3 -= 1;
	cx3 = ntr/2;	igetpar("cx3", &cx3);		cx3 -= 1;	

	/* check if user wants the special output specified */
        /* by liner=1; if so, set parameters accordingly    */
	liner = 0;	igetpar("liner", &liner);
	if (liner == 1) {
		nt = 64;	tr.ns = nt;
		ntr = 64;	tr.ntr = ntr;
		npl = 3;	

		dip1 = 0;
		len1 = ntr/4;	
		ct1 = nt/2;	ct1 -= 1;		
		cx1 = 3*ntr/4;	cx1 -= 1;

		dip2 = 2;
		len2 = ntr/4;	
		ct2 = nt/2;	ct2 -= 1;
		cx2 = ntr/2;	cx2 -= 1;	

		dip3 = 4;
		len3 = ntr/4;	
		ct3 = nt/2;	ct3 -= 1;
		cx3 = ntr/4;	cx3 -= 1;	
	}

	/* calculate milliseconds per sample */
	msps = dt*1000.0;	

	tfe1 = 0; tfe2 = 0; tfe3 = 0;
	for (itr = 0; itr < ntr; itr++) {
		bzero(tr.data, nt * sizeof(float));

		/* plane 1 */
		if (itr >= cx1-len1/2 && itr <= cx1+len1/2) {
		    ++tfe1;

		    /* fit is fractional sample of plane intersection */
		    fit = ct1 - ( cx1 - itr ) * dip1 / msps; 
		    if (fit >= 0 && fit <= (float) nt) {

			/* linear interpolation */
			itless = fit;
			eps = fit - itless;
			itmore = fit + 1;
			tr.data[itless] += 1.0 - eps;	 
			tr.data[itmore] += eps;	 

			/* taper option */
			if (taper == 1) {
			  /* last point */
			  if (tfe1 == 1 || tfe1 == len1 + 1) {
				tr.data[itless] /= 6.0;	 
				tr.data[itmore] /= 6.0;	 
			  } 
			  /*  next-to-last point */
			  if (tfe1 == 2 || tfe1 == len1) {
				tr.data[itless] /= 3.0;	 
				tr.data[itmore] /= 3.0;	 
			  } 
		    }
		  }
		}

		/*  plane 2  */
		if (npl > 1) {
		  if (itr >= cx2-len2/2 && itr <= cx2+len2/2) {
		    ++tfe2;

		    /* fit is fractional sample of plane intersection */
		    fit = ct2 - ( cx2 - itr ) * dip2 / msps; 
		    if (fit >= 0 && fit <= (float) nt) {

			/* linear interpolation */
			itless = fit;
			eps = fit - itless;
			itmore = fit + 1;
			tr.data[itless] += 1.0 - eps;	 
			tr.data[itmore] += eps;	 

			/* taper option */
			if (taper == 1) {
			  /* last point */
			  if (tfe2 == 1 || tfe2 == len2 + 1) {
				tr.data[itless] /= 6.0;	 
				tr.data[itmore] /= 6.0;	 
			  } 
			  /*  next-to-last point */
			  if (tfe2 == 2 || tfe2 == len2) {
				tr.data[itless] /= 3.0;	 
				tr.data[itmore] /= 3.0;	 
			  } 
		        }
		    }
		  }
		}

		/* plane 3  */
		if (npl > 2) {
		  if (itr >= cx3-len3/2 && itr <= cx3+len3/2) {
		    ++tfe3;

		    /* fit is fractional sample of plane intersection */
		    fit = ct3 - ( cx3 - itr ) * dip3 / msps; 
		    if (fit >= 0 && fit <= (float) nt) {

			/* linear interpolation */
			itless = fit;
			eps = fit - itless;
			itmore = fit + 1;
			tr.data[itless] += 1.0 - eps;	 
			tr.data[itmore] += eps;	 

			/* taper option */
			if (taper == 1) {
			  /* last point */
			  if (tfe3 == 1 || tfe3 == len3 + 1) {
				tr.data[itless] /= 6.0;	 
				tr.data[itmore] /= 6.0;	 
			  } 
			  /* next-to-last point */
			  if (tfe3 == 2 || tfe3 == len3) {
				tr.data[itless] /= 3.0;	 
				tr.data[itmore] /= 3.0;	 
			  } 
		        }
		    }
		  }
		}

		/* fill tracl header and put trace out */
		tr.tracl = itr + 1;
		puttr(&tr);
	}
	exit(0);
}
