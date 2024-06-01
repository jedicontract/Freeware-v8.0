Article 2409 of vmsnet.internals:
Path: jac.zko.dec.com!pa.dec.com!decwrl!ames!tgv.com!adelman
From: adelman@tgv.com (Kenneth Adelman)
Newsgroups: vmsnet.internals
Subject: Re: CDROM software distribution format
Date: 8 Sep 1994 16:13 PDT
Organization: TGV, Incorporated (Santa Cruz, CA, USA)
Lines: 431
Distribution: world
Message-ID: <8SEP199416130498@tgv.com>
References: <1994Sep8.002012.11255@nls.com>
NNTP-Posting-Host: hq.tgv.com
X-Lunar-Date: 0 days, 15 hours, 4 minutes since the new moon
News-Software: VAX/VMS VNEWS 1.41    

In article <1994Sep8.002012.11255@nls.com>, davek@nls.com (David Kellerman) writes...
>I'm making arrangements for CD distribution of one of our software
>products that up to now has been distributed in VMSINSTAL format
>on TK50 and magtape.  I know Digital and some other CD duplicators
>are set up to simply take a tape master and do all the magic to
>produce the CD.  But I've got convenient access to a facility that
>can produce gold masters, and would like to take advantage of it.
>Of course, they know nothing about VMS.
>
>Their general idea seems to be "give us a 1G SCSI disk with the
>data on it in the format you want, and we'll copy it to CD."  This
>sounds plausible, although it would be more convenient for me to
>clean off one of the disks on my system, build the distribution on
>it, and then make a block-by-block copy onto a transfer tape.
>
>Has anybody out there done this?  Recommendations, pitfalls, disk
>size and geometry issues, sources of more information, etc?

    We have a CD-R writer so we've done the full process in-house, but
never to an 8mm transfer tape.	I think I understand the process you
need, but have never done it using a third party for the second half.

    (Btw, I've also used DEC's service and was very happy with it.
800-DEC-MRDS. You send them a BACKUP/IMAGE of your filesystem.)

    Basically,

    1) Create a VMS volume in-house.
    2) INIT it and put your files in it.
    3) dismount it
    4) MOUNT it /FOREIGN.
    5) Copy the raw blocks from the filesystem to an 8mm tape.

    The problem with sending a disk directly to them is that
there is no such thing as a 1GB CD. You'd need to build a filesystem
on a disk which is less than the maximum CD size. You can do this
yourself using a VDDRIVER partition of a real disk to create
your filesystem, and the following program to copy the raw blocks
to an 8mm.

						    Ken



/*
 *	Program to copy a disk partition mounted /FOREIGN to a tape drive.
 */
#define IOSIZE (64) /* In sectors */
#include <stdio.h>
#ifdef __GNUC__
#include <vms/descrip.h>
#include <vms/dvidef.h>
#include <vms/iodef.h>
#include <sys/time.h>
#include <vms/ssdef.h>
#else
#include <descrip.h>
#include <dvidef.h>
#include <iodef.h>
#include "multinet_root:[multinet.include.sys]time.h"
#endif
#define sysfail(sts)  (((sts)&1)==0)
#define syswork(sts)  (((sts)&1)!=0)

unsigned short DiskChannel;	     /* VMS Channel to disk */
unsigned short TapeChannel;
unsigned long DiskSize;		     /* Number of disk sectors after assign */
static struct	timeval time0;	     /* Time at which timeing started */

main(argc,argv)
int argc;
char *argv[];
{
	int s, code, realt;
	int lbn, readsize, Status, log_lbn=0;
	register int i;
	unsigned short int IOSB1[4], IOSB2[4];
	char Buffer1[IOSIZE*512], Buffer2[IOSIZE*512], *cp, *BPTape, *BPDisk;
	struct dsc$descriptor devdsc={0,0,0};
	struct timeval timedol;
	struct timeval td;

	if (argc < 3) {
	    fprintf(stderr,"Usage: %s diskname tapename\n",argv[0]);
	    exit(1);
	}

	devdsc.dsc$w_length=strlen(devdsc.dsc$a_pointer=argv[1]);
	if (sysfail(s=SYS$ASSIGN(&devdsc,&DiskChannel,0,0))) {
		fprintf(stderr,"Unable to $ASSIGN to device %s\n",argv[1]);
		(void) SYS$EXIT(s);
	}

	devdsc.dsc$w_length=strlen(devdsc.dsc$a_pointer=argv[2]);
	if (sysfail(s=SYS$ASSIGN(&devdsc,&TapeChannel,0,0))) {
		fprintf(stderr,"Unable to $ASSIGN to device %s\n",argv[2]);
		(void) SYS$EXIT(s);
	}

	if (argc >= 4) {
	    i = atoi(argv[3]);
	    if (i < 0) {
		code = DVI$_MAXBLOCK;
		if (sysfail(s=LIB$GETDVI(&code,&DiskChannel,NULL,&DiskSize))) {
		    fprintf(stderr,"$GETDVI failed on %s\n",argv[1]);
		    (void) SYS$EXIT(s);
		}
		DiskSize -= (-i);
	    } else {
		DiskSize = i;
	    }
	} else {
	    code = DVI$_MAXBLOCK;
	    if (sysfail(s=LIB$GETDVI(&code,&DiskChannel,NULL,&DiskSize))) {
		fprintf(stderr,"$GETDVI failed on %s\n",argv[1]);
		(void) SYS$EXIT(s);
	    }
	}
	/*
	 *  PACKACK and REWIND the CD-ROM
	 */

	printf("\"Rewinding\" %s\n",argv[2]);
	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_PACKACK,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("PAKACK failed\n");
		exit(Status);
	}

	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_DSE,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("REWIND failed\n");
		exit(Status);
	}

	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_REWIND,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("REWIND failed\n");
		exit(Status);
	}

	printf("Copying %u blocks from %s->%s\n",DiskSize,argv[1],argv[2]);

	gettimeofday(&time0, (struct timezone *)0);

	/*
	 *  Read from disk...
	 */
	lbn = 0;

	readsize = IOSIZE;
	if (lbn+readsize > DiskSize) readsize = DiskSize - lbn;
	Status = SYS$QIOW(0,
			  DiskChannel,
			  IO$_READLBLK,
			  IOSB1,
			  0,
			  0,
			  Buffer1,
			  readsize * 512,
			  lbn, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("Status error = 0x%x at lbn %d\n",Status,lbn);
		exit(Status);
	}
	BPTape = Buffer1;
	BPDisk = Buffer2;
	lbn += readsize;

	for (; lbn<DiskSize; lbn+=readsize) {
	    /*
	     *	Start the write to tape...
	     */
	    Status = SYS$QIO(2,
			     TapeChannel,
			     IO$_WRITEPBLK,
			     IOSB2,
			     0,
			     0,
			     BPTape,
			     IOSB1[1],
			     0, 0, 0, 0);
	    if (!(Status&1)) {
		    printf("Writing error = 0x%x at lbn %d\n",Status,lbn);
		    exit(Status);
	    }
	    /*
	     *	Read from disk...
	     */
	    readsize = IOSIZE;
	    if (lbn+readsize > DiskSize) readsize = DiskSize - lbn;
	    Status = SYS$QIO(3,
			     DiskChannel,
			     IO$_READLBLK,
			     IOSB1,
			     0,
			     0,
			     BPDisk,
			     readsize * 512,
			     lbn, 0, 0, 0);
	    if (!(Status&1)) {
		    printf("Status error = 0x%x at lbn %d\n",Status,lbn);
		    exit(Status);
	    }

	    /*
	     *	Wait for each...
	     */
	    SYS$WFLAND(2,(1<<2)|(1<<3));
	    if (!(IOSB1[0]&1)) {
		    printf("Reading error = 0x%x at lbn %d\n",IOSB1[0],lbn);
		    exit(IOSB1[0]);
	    }
	    if (!(IOSB2[0]&1)) {
		    printf("Writing error = 0x%x at lbn %d\n",IOSB2[0],lbn);
		    exit(IOSB2[0]);
	    }
	    /*
	     *	Swap buffers...
	     */
	    cp = BPTape;
	    BPTape = BPDisk;
	    BPDisk = cp;


	    if (lbn > log_lbn + (DiskSize/20)) {
		log_lbn = lbn;
		gettimeofday(&timedol, (struct timezone *)0);
		tvsub( &td, &timedol, &time0 );
		realt = td.tv_sec + ((double)td.tv_usec) / 1000000;
		if (realt == 0) realt = 1;
		printf("%d%% done; estimate %d:%02d left, %.2f MB/sec\n",
		  (int)( (double) lbn * 100.0 / (double) DiskSize),
	   (int)(((double) realt * (double) DiskSize / (double) lbn) - (double) realt) / 60,
	   (int)((((double) realt * (double) DiskSize / (double) lbn) - (double) realt)) % 60,
		       (double) lbn / 2048.0 / (double) realt);
	    }

	}

	/*
	 *  Write the last buffer to tape...
	 */
	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_WRITEPBLK,
			  IOSB2,
			  0,
			  0,
			  BPTape,
			  IOSB1[1],
			  0, 0, 0, 0);
	if (Status&1) Status = IOSB1[0];
	if (!(Status&1)) {
		printf("Writing error = 0x%x at lbn %d\n",Status,lbn);
		exit(Status);
	}

	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_WRITEOF,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("WRITEOF failed\n");
		exit(Status);
	}

	gettimeofday(&timedol, (struct timezone *)0);
	tvsub( &td, &timedol, &time0 );
	realt = td.tv_sec + ((double)td.tv_usec) / 1000000;
	if (realt == 0) realt = 1;
	printf("Wrote %dMB, %d seconds, %0.2f MB/sec\n",
	    DiskSize / 2048, realt, (double) DiskSize / 2048.0 / (double) realt);

#ifdef VERIFY
	/*
	 *  Rewind and make a verification pass...
	 */

	printf("\"Rewinding\" %s\n",argv[2]);
	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_REWIND,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("REWIND failed\n");
		exit(Status);
	}

	printf("Verifying %u blocks from %s->%s\n",DiskSize,argv[1],argv[2]);
	gettimeofday(&time0, (struct timezone *)0);

	for (log_lbn=lbn=0; lbn<DiskSize; lbn+=readsize) {
	    /*
	     *	Read from disk...
	     */
	    readsize = IOSIZE;
	    if (lbn+readsize > DiskSize) readsize = DiskSize - lbn;
	    Status = SYS$QIO(2,
			     DiskChannel,
			     IO$_READLBLK,
			     IOSB1,
			     0,
			     0,
			     Buffer1,
			     readsize * 512,
			     lbn, 0, 0, 0);
	    if (!(Status&1)) {
		    printf("Status error = 0x%x at lbn %d\n",Status,lbn);
		    exit(Status);
	    }
	    /*
	     *	Read from tape...
	     */
	    Status = SYS$QIO(3,
			     TapeChannel,
			     IO$_READPBLK,
			     IOSB2,
			     0,
			     0,
			     Buffer2,
			     readsize * 512,
			     0, 0, 0, 0);
	    if (!(Status&1)) {
		    printf("Reading error = 0x%x at lbn %d\n",Status,lbn);
		    exit(Status);
	    }
	    SYS$WFLAND(2,(1<<2)|(1<<3));
	    if (!(IOSB1[0]&1)) {
		    printf("Reading error = 0x%x at lbn %d\n",IOSB1[0],lbn);
		    exit(IOSB1[0]);
	    }
	    if (!(IOSB2[0]&1)) {
		    printf("Reading error = 0x%x at lbn %d\n",IOSB2[0],lbn);
		    exit(IOSB2[0]);
	    }

	    if (IOSB2[1] != readsize * 512) {
		printf("Bad size read from tape; got %u, expected %u\n",
		    IOSB2[1],readsize*512);
	    }

	    if (bcmp(Buffer1, Buffer2, readsize*512) != 0) {
		printf("Verification error at LBN %d\n",lbn);
		exit(0x10000004);
	    }

	    if (lbn > log_lbn + (DiskSize/20)) {
		log_lbn = lbn;
		gettimeofday(&timedol, (struct timezone *)0);
		tvsub( &td, &timedol, &time0 );
		realt = td.tv_sec + ((double)td.tv_usec) / 1000000;
		if (realt == 0) realt = 1;
		printf("%d%% done; estimate %d:%02d left, %.2f MB/sec\n",
		  (int)( (double) lbn * 100.0 / (double) DiskSize),
	   (int)(((double) realt * (double) DiskSize / (double) lbn) - (double) realt) / 60,
	   (int)((((double) realt * (double) DiskSize / (double) lbn) - (double) realt)) % 60,
		       (double) lbn / 2048.0 / (double) realt);
	    }
	}

	/*
	 *  One more read; make sure we get EOF...
	 */
	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_READPBLK,
			  IOSB2,
			  0,
			  0,
			  Buffer2,
			  readsize * 512,
			  0, 0, 0, 0);
	if (Status&1) Status = IOSB2[0];
	if (Status != SS$_ENDOFFILE) {
	    printf("EOF expected and not received! status=0x%x\n",Status);
		exit((Status & 1) ? 0x10000004 : Status);
	}
#endif

	/*
	 *  Rewind for posterity....
	 */

	Status = SYS$QIOW(0,
			  TapeChannel,
			  IO$_REWIND,
			  IOSB1,
			  0, 0, 0, 0, 0, 0, 0, 0);
	if (Status&1) Status=IOSB1[0];
	if (!(Status&1)) {
		printf("REWIND failed\n");
		exit(Status);
	}

	(void) SYS$DASSGN(TapeChannel);
	(void) SYS$DASSGN(DiskChannel);
	exit(0);
}


tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}



                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    