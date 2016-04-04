/* TAPEREAD: $Revision: 1.1 $ ; $Date: 89/11/06 21:07:31 $	*/


/* Exercise the tape drive	*/

#include "cwp.h"

#define BSIZE	10240			/* Buffer size		*/
#define TSIZE	1024			/* Trace size		*/
#define NTIMES	50			/* Number of reads	*/
#define DATA	"traces.out"		/* Name of trace file	*/
#define TAPE	"/dev/rmt1"		/* Name of tape file	*/


main()
{
	char buf[BSIZE];/* buffer to hold data		*/
	int d_fd;	/* file descriptor for data	*/
	int t_fd;	/* file descriptor for tape	*/
	int itr;	/* current trace number		*/


	/* Open tape file */
	if (-1 == (t_fd = open(TAPE, O_RDONLY))) {
		perror("can't open tape read_only");
		exit(1);
	}


	/* Open data file */
	if (-1 == (d_fd = open(DATA, O_WRONLY | O_CREAT | O_TRUNC, 0666))) {
		perror("can't open data file write_only");
		exit(2);
	}


	/* Read traces from tape to datafile */
	for (itr = 0; itr < NTIMES; ++itr) {

		/* read the trace and check for errors */
		if (-1 == read(t_fd, buf, TSIZE)) {
			perror("read failed"); 
			exit(3);
		}

		/* write the trace and look for errors */
		if (-1 == write(d_fd, buf, TSIZE)) {
			perror("write failed");
			exit(5);
		}

		if ((itr + 1) % 20 == 0) {
			(void) fprintf(stderr,
				" %d traces from tape\n", itr + 1);
		}
	}


	/* Clean up */
	if (-1 == close(t_fd)) {
		perror("close failed on tape");
		exit(7);
	}
	if (-1 == close(d_fd)) {
		perror("close failed on data");
		exit(8);
	}

	return 0;
}
