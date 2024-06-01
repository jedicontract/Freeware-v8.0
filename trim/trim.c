#pragma module TRIM "V1.36"

/* TRIM.C

  A program which is intended to do what the TRIM utility (supplied with
  ALL-IN-1 but not documented or supported) apparently does.

  B. Z. Lederman	14-Feb-1989
			15-Feb-1989 RMS and parsing in place, /TRIM,
				    /FIRST_LINE and /LAST_LINE appear to be
				    working properly.
			06-Mar-1989 put test for printing line in function.
			 5-Dec-1989 work on stripping non-printing chars.
			25-Jun-1991 add /PREFIX, /SUFFIX
			26-Jun-1991 add /TRAIL, add /START, /FINAL, /REMOVE,
					/REPLACE, /STATISTICS, wildcard
					file name.
			27-Jun-1991 add more counters,
				    /REMOVE one or all instances
			28-Jun-1991 Case sensitivity on remove, match.
			 2-Jul-1991 separate first line /PREFIX, /SUFFIX
				    add /APPEND for a line on the end
				    add /PREPEND for a line at the front
			23-Aug-1991 Larger record buffer
			16-Jan-1992 Fix problem with VFC files.
			18-May-1992 Remove redundant "&" on in_record
			11-Aug-1998 DECC and Alpha
*/

#include <STDIO>
#include <STDLIB>
#include <RMS>
#include <SSDEF>
#include <DESCRIP>
#include <CLIMSGDEF>
#include <CTYPE>		/* for isprint				*/
#include <LIMITS>		/* for SHRT_MAX				*/
#include <LIB$ROUTINES>
#include <STR$ROUTINES>
#include <CLI$ROUTINES>
#include <STARLET>

/* declare internal function prototypes					*/

int test_include_line ();


static int status, secondary_status, timer = 0;

/* flags and values for the command qualifiers				*/

int first_line_n, last_line_n, include_line;

int file_open, file_end;	/* file processing flags		*/

struct				/* flags for processing options		*/
{
    int starting;
    int final;
    int remove;
    int prefix;
    int prefirst;
    int suffix;
    int suffirst;
    int trim;
    int strip;		/* new flag; strip non-printing characters	*/
    int trail;		/* new flag; strip trailing blanks and tabs	*/
    int replace;	/* new flag; replace REMOVED string		*/
    int statistics;	/* new flag; write processing statistics	*/
    int single;		/* new flag; replace only first instance	*/
    int case_sens;	/* new flag; select case sensitivity on search	*/
    int append;		/* new flag; append a line at the end		*/
    int prepend;	/* new flag; prepend a line at the beginning	*/
} flag;

struct				/* counters				*/
{
    int lines;			/* count the line we're on		*/
    int removes;		/* number of times a string is removed	*/
    int includes;		/* number of lines included in output	*/
} counters;

struct				/* totals counters			*/
{
    int lines;			/* total number of lines in all files	*/
    int files;			/* number of files processed		*/
    int removes;		/* total times a string is removed	*/
    int includes;		/* total lines in all outputs		*/
} totals = {0, 0, 0, 0};

struct FAB in_fab;		/* RMS file structures (FAB and RAB)	*/
struct RAB in_rab;

struct FAB out_fab;
struct RAB out_rab;

char in_record[SHRT_MAX];	/* record buffer			*/

short int rsize;		/* retrieved record size is a word	*/

/* Strings to hold the file specifications.				*/

struct dsc$descriptor_s search_d =
    {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

struct dsc$descriptor_d filespec_d =		/* open this when found	*/
    {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

/* Descriptor for string processing.					*/

struct dsc$descriptor_d out_d =
    {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

struct dsc$descriptor_d case_d =
    {0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

/* Dynamic strings to hold the returned values for the qualifiers	*/

struct dsc$descriptor_d first_d    =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d last_d     =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d start_d    =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d final_d    =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d remove_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d replace_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d prefix_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d prefirst_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d suffix_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d suffirst_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d append_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
struct dsc$descriptor_d prepend_d   =
				{0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};

/* Null descriptor.							*/

$DESCRIPTOR (null_d, "");

void get_commands (), open_files (), close_files ();   /* called	*/
void get_record (), put_record (), do_file ();	       /* routines	*/

main ()
{
    int file_status = 1, find_context = 0;	/* used for file search	*/

    status = LIB$INIT_TIMER (&timer);

    if ((status & 1) != 1)		/* check for errors		*/
    {
	LIB$SIGNAL (status);
    };

    get_commands ();		/* get the command line			*/

    if (!flag.replace)		/* if removing but not replacing	*/
    {
	status = STR$COPY_DX (&replace_d, &null_d); /* set null string	*/

	if ((status & 1) != 1)		/* check for errors		*/
	{
	    LIB$SIGNAL (status);
	};
    }

    while ((file_status & 1) == 1)
    {
        file_status = LIB$FIND_FILE (&search_d, &filespec_d, &find_context, 
					0, 0, &secondary_status, 0);

	if (file_status == RMS$_NMF)	/* if no more files, exit loop	*/
	{				/* and go on to output part	*/
	    break;
	}
	else if (file_status == RMS$_FNF) /* if no files found at all,	*/
	{				/* quit program.		*/
	    exit (file_status);		/* normal exit is enough	*/
	}
	else if ((file_status & 1) != 1)	/* if some other error,	*/
	{					/* signal to user	*/
	    LIB$SIGNAL (file_status, secondary_status);
	};

	if (flag.statistics)
	{
	    status = LIB$PUT_OUTPUT (&filespec_d);  /* output file name	*/
	};

	do_file ();		/* process a file			*/

	if (flag.statistics)	/* add up statistics			*/
	{
	    totals.files = totals.files + 1;
	    totals.lines = totals.lines + counters.lines;
	    totals.removes = totals.removes + counters.removes;
	    totals.includes = totals.includes + counters.includes;
	};
    };				/* end of processing loop		*/

    if (flag.statistics)
    {
	printf ("\n files = %d, lines = %d", totals.files, totals.lines);

	printf ("\n removes = %d, includes = %d\n",
					totals.removes, totals.includes);

	status = LIB$SHOW_TIMER (&timer, 0, 0, 0);
    }

    exit (status);		/* time to leave			*/
}

void do_file ()
{
    int i;			/* loop counter				*/
    int tsize;			/* temporary size			*/


    open_files ();		/* open files				*/

    file_end = FALSE;		/* not end of file yet			*/

    counters.lines = 0;		/* re-set counters			*/
    counters.removes = 0;
    counters.includes = 0;

    if (flag.prepend)		/* if we are supposed to prepend a line	*/
    {
	status = STR$COPY_DX (&out_d, &prepend_d);

	if ((status & 1) != 1)			/* check for errors	*/
	{
	    LIB$SIGNAL (status);
	};

	counters.includes = counters.includes + 1;	 /* keep count	*/
	put_record ();		/* write out the line			*/
    };

    while (!file_end)		/* processing loop			*/
    {
	get_record ();		/* get a record				*/

	if (file_end) break;	/* be sure to quit here on end of file	*/

	if (rsize > 0)		/* if the record isn't null		*/
	{
	    if (flag.strip)			/* if stripping		*/
	    {
		tsize = 0;

		for (i = 0;  i < rsize;  i++)
		{
		    if (((isprint (in_record[i]) != 0) ||
			 (isspace (in_record[i]) != 0)) &&
			( isascii (in_record[i]) != 0) )
		    {
			in_record[tsize] = in_record[i];
			tsize = tsize + 1;
		    };
		};
		rsize = tsize;
	    };		    /* End of stripping non-printing characters	*/
/*
   Put the record into a descriptor for STR$xxx routines.
*/
	    status = STR$COPY_R (&out_d, &rsize, in_record);

	    if ((status & 1) != 1)		/* check for errors	*/
	    {
		LIB$SIGNAL (status);
	    };
/*
   Trim trailing blanks and tabs before checking for blank line inclusion.
*/
	    if (flag.trail)
	    {
		status = STR$TRIM (&out_d, &out_d, 0);

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    };
	}		/* End of record size not zero.			*/
	else		/* If input record was zero, null output record */
	{
	    status = STR$COPY_DX (&out_d, &null_d);

	    if ((status & 1) != 1)		/* check for errors	*/
	    {
		LIB$SIGNAL (status);
	    };
	};			/* End of initial record processing.	*/

	if (flag.starting)
	{
	    if (flag.case_sens)		/* if preserving case		*/
	    {
		i = STR$POSITION (&out_d, &start_d, 0);
	    }
	    else			/* not preserving case		*/
	    {
		status = STR$UPCASE (&case_d, &out_d);

		if ((status & 1) != 1)
		{
		    LIB$SIGNAL (status);
		};

		i = STR$POSITION (&case_d, &start_d, 0);
	    };

	    if (i > 0)
	    {
		status = STR$RIGHT (&out_d, &out_d, &i);

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    }
	    else		/* starting character not found, null	*/
	    {			/* output				*/
		status = STR$COPY_DX (&out_d, &null_d);

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    };
	};			/* end of starting strip		*/

	if (flag.final)
	{
	    if (flag.case_sens)		/* if preserving case		*/
	    {
		i = STR$POSITION (&out_d, &final_d, 0);
	    }
	    else			/* not preserving case		*/
	    {
		status = STR$UPCASE (&case_d, &out_d);

		if ((status & 1) != 1)
		{
		    LIB$SIGNAL (status);
		};

		i = STR$POSITION (&case_d, &final_d, 0);
	    };

	    if (i > 1)
	    {
		i = i - 1;			/* don't keep final 	*/

		status = STR$LEFT (&out_d, &out_d, &i);

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    };
	};			/* end of final strip			*/

	if (flag.remove)
	{
	    i = 1;			/* always test once		*/

	    while (i > 0)		/* remove as often as necessary	*/
	    {
		if (flag.case_sens)	/* if preserving case		*/
		{
		    i = STR$POSITION (&out_d, &remove_d, 0);
		}
		else			/* not preserving case		*/
		{
		    status = STR$UPCASE (&case_d, &out_d);

		    if ((status & 1) != 1)
		    {
			LIB$SIGNAL (status);
		    };

		    i = STR$POSITION (&case_d, &remove_d, 0);
		};

		if (i > 0)
		{
		    tsize = i + remove_d.dsc$w_length - 1;

		    status = STR$REPLACE (&out_d, &out_d, &i, &tsize,
					&replace_d);

		    if ((status & 1) != 1)	/* check for errors	*/
		    {
			LIB$SIGNAL (status);
		    };

		    counters.removes = counters.removes + 1; /* keep count */
		};

		if (flag.single) i = 0;	/* don't repeat			*/
	    };				/* end of remove loop		*/
	};				/* end of remove string		*/

	include_line = test_include_line ();	/* do we include this	*/
						/* line?		*/
	if (include_line)			/* if yes, process it	*/
	{
	    counters.includes = counters.includes + 1;	 /* keep count	*/

	    if (flag.prefix)
	    {
		if (flag.prefirst && (counters.includes == 1))
		{
		    status = STR$PREFIX (&out_d, &prefirst_d);
		}
		else
		{
		    status = STR$PREFIX (&out_d, &prefix_d);
		};

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    };

	    if (flag.suffix)
	    {
		if (flag.suffirst && (counters.includes == 1))
		{
		    status = STR$APPEND (&out_d, &suffirst_d);
		}
		else
		{
		    status = STR$APPEND (&out_d, &suffix_d);
		};

		if ((status & 1) != 1)		/* check for errors	*/
		{
		    LIB$SIGNAL (status);
		};
	    };

	    put_record ();	/* write out the line			*/
	};			/* end of if including this line	*/
    };				/* end of processing loop		*/

    if (flag.append)		/* if we are supposed to append a line	*/
    {
	status = STR$COPY_DX (&out_d, &append_d);

	if ((status & 1) != 1)			/* check for errors	*/
	{
	    LIB$SIGNAL (status);
	};

	counters.includes = counters.includes + 1;	 /* keep count	*/
	put_record ();		/* write out the line			*/
    };

    close_files ();		/* close files				*/

    return;
}

int test_include_line ()

{
    int include = FALSE;	/* default is don't include this record */
/*
   When counting lines for inclusion, all of the following
   characteristics must be met:
*/
/*
   If the current line is >= the desired first line (which defaults to
   1) the record is acceptable.
*/
    if (counters.lines >= first_line_n)
    {
/*
   If we are not counting the last line (last_line_n = 0) OR if we are
   counting and we haven't exceeded the line count, the record is
   acceptable.
*/
	if ((last_line_n == 0) || (counters.lines <= last_line_n))
	{
/*
   If the trim (blank lines) flag isn't set; or if it is and the line
   isn't blank, the record is acceptable.
*/
	    if ( (flag.trim == FALSE) ||
		( (flag.trim == TRUE) && (out_d.dsc$w_length > 0) ) )
	    {
		include = TRUE;		/* include this record		*/
	    };				/* end if trimming blanks	*/
	};				/* end if last line count	*/
    };					/* end if first line count	*/

    return (include);
}

void get_record ()

{
    status = SYS$GET (&in_rab, 0, 0);		/* retrieve record	*/

    if (status == RMS$_NORMAL)			/* if successful	*/
    {
	rsize = in_rab.rab$w_rsz;		/* get length		*/
	counters.lines = counters.lines + 1;	/* count records read	*/
    }
    else if (status == RMS$_EOF)		/* if end-of-file	*/
    {
	file_end = TRUE;
    }
    else					/* error on read	*/
    {
	LIB$SIGNAL (in_rab.rab$l_sts, in_rab.rab$l_stv);
    };

    return;
}

void put_record ()

{
/*
   Point the output buffers to the current descriptor location and
   size.  Because we use a dynamic descriptor, even the address can
   change.
*/
    out_rab.rab$l_rbf = out_d.dsc$a_pointer;
    out_rab.rab$w_rsz = out_d.dsc$w_length;

    status = SYS$PUT (&out_rab, 0, 0);		/* write a record	*/

    if (status != RMS$_NORMAL)			/* if not successful	*/
    {
	LIB$SIGNAL (out_rab.rab$l_sts, out_rab.rab$l_stv);
    };

    return;
}

void open_files ()

{

/* initialize the FABs and RABs						*/

    in_fab = cc$rms_fab;
    in_rab = cc$rms_rab;

    in_fab.fab$l_fna = filespec_d.dsc$a_pointer; /* file name		*/
    in_fab.fab$b_fns = filespec_d.dsc$w_length;	/* size of file name	*/
    in_fab.fab$b_org = FAB$C_SEQ;	/* sequential file		*/
    in_fab.fab$b_fac = FAB$M_GET;	/* get only			*/
    in_fab.fab$b_shr = FAB$M_SHRGET;	/* shared read (may need share	*/
					/* write for other users)	*/
    in_rab.rab$l_fab = &in_fab;		/* specify fab			*/
    in_rab.rab$l_ubf = in_record;	/* specify buffer		*/
    in_rab.rab$w_usz = sizeof (in_record);  /* size of buffer		*/
    in_rab.rab$b_rac = RAB$C_SEQ;	/* specify sequential access	*/
    in_rab.rab$l_rop = RAB$M_RRL |	/* read regardless of lock	*/
		       RAB$M_NLK |	/* don't lock record		*/
		       RAB$M_RAH |	/* read ahead			*/
		       RAB$M_WAT;	/* wait if locked (?)		*/

    status = SYS$OPEN (&in_fab);	/* open the file		*/

    if (status == RMS$_FNF)		/* if "File Not Found"		*/
    {
	exit (status);			/* system emits message		*/
    }
    else if (status != RMS$_NORMAL)	/* for any other error		*/
    {
	LIB$STOP (status);		/* stop with trace		*/
    };

    status = SYS$CONNECT (&in_rab);	/* connect to the file stream	*/

    if (status != RMS$_NORMAL)		/* quit on any error		*/
    {
	LIB$STOP (status);
    };

    out_fab = cc$rms_fab;
    out_rab = cc$rms_rab;

/* Copy file attributes from the input file.				*/

    out_fab.fab$l_fna = filespec_d.dsc$a_pointer;   /* file name	*/
    out_fab.fab$b_fns = filespec_d.dsc$w_length;    /* size of file name */

/* Some output file attributes must be set to specific values.		*/

    out_fab.fab$b_org = FAB$C_SEQ;	    /* sequential file		*/
    out_fab.fab$b_fac = FAB$M_PUT;	    /* put only			*/
    out_fab.fab$l_fop = FAB$M_TEF |	    /* truncate at EOF		*/
			FAB$M_OFP |	    /* "stick" name only	*/
			FAB$M_MXV;	    /* use next version number	*/
    out_fab.fab$b_rfm = FAB$C_VAR;	    /* output records variable	*/
    out_fab.fab$w_mrs = sizeof (in_record); /* set maximum record size	*/

/* check to see if copying attributes is really appropriate		*/

    if ((in_fab.fab$b_rat & FAB$M_CR) == 1)	/* if 'list' cc		*/
    {
	out_fab.fab$b_rat = in_fab.fab$b_rat;	/* copy value		*/
    }
    else
    {
	out_fab.fab$b_rat = FAB$M_CR;		/* set carriage control	*/
    };

    out_fab.fab$l_alq = in_fab.fab$l_alq;	/* copy allocated space	*/
    out_fab.fab$w_bls = in_fab.fab$w_bls;	/* "  block size	*/

    out_rab.rab$l_fab = &out_fab;	/* specify fab			*/
    out_rab.rab$l_rbf = in_record;	/* specify (dummy) buffer	*/
    out_rab.rab$b_rac = RAB$C_SEQ;	/* specify sequential access	*/
    out_rab.rab$l_rop = RAB$M_WBH;	/* write behind			*/

    status = SYS$CREATE (&out_fab);	/* create a new file		*/

    if (status != RMS$_NORMAL)		/* exit on any error		*/
    {
	LIB$STOP (status);
    };

    status = SYS$CONNECT (&out_rab);	/* connect to the file		*/

    if (status != RMS$_NORMAL)		/* exit on any error		*/
    {
	LIB$STOP (status);
    };

    file_open = TRUE;			/* indicate success		*/

    return;
}

void close_files ()

{
    if (file_open)
    {
	status = SYS$DISCONNECT (&in_rab);
	if (status != RMS$_NORMAL)
	{
	    LIB$STOP (status);
	};

	status = SYS$CLOSE (&in_fab);
	if (status != RMS$_NORMAL)
	{
	    LIB$STOP (status);
	};

	status = SYS$DISCONNECT (&out_rab);
	if (status != RMS$_NORMAL)
	{
	    LIB$STOP (status);
	};

	status = SYS$CLOSE (&out_fab); 
	if (status != RMS$_NORMAL)
	{
	    LIB$STOP (status);
	};

	file_open = FALSE;
    };

    return;
}

void get_commands ()
{
/*
   Get the commands from TRIM.CLD

   Because the CLD file has values required for qualifiers if they are
   present, the absense of a value for a present qualifier is an error
   which causes the program to terminate.
*/
    static int pstatus, gstatus, tstatus;
/*
   Descriptors for the command qualifiers: in alphabetical order by
   command, easier to check for conflicts that way.
*/
    $DESCRIPTOR (append_q,   "APPEND");
    $DESCRIPTOR (case_q,     "CASE_SENSITIVE");
    $DESCRIPTOR (filespec_q, "FILE_SPEC");
    $DESCRIPTOR (final_q,    "FINAL_CHAR");
    $DESCRIPTOR (first_q,    "FIRST_LINE");
    $DESCRIPTOR (last_q,     "LAST_LINE");
    $DESCRIPTOR (remove_q,   "REMOVE");
    $DESCRIPTOR (replace_q,  "REPLACE");
    $DESCRIPTOR (prepend_q,  "PREPEND");
    $DESCRIPTOR (prefix_q,   "PREFIX");
    $DESCRIPTOR (single_q,   "SINGLE");
    $DESCRIPTOR (starting_q, "STARTING_CHAR");
    $DESCRIPTOR (statist_q,  "STATISTICS");
    $DESCRIPTOR (strip_q,    "STRIP");
    $DESCRIPTOR (suffix_q,   "SUFFIX");
    $DESCRIPTOR (trail_q,    "TRAIL");
    $DESCRIPTOR (trim_q,     "TRIM");

    $DESCRIPTOR (firstline_q,"FIRST");		/* might not be used	*/

    short int val_len;		/* used in parsing qualifiers		*/

/* retrieve file specification						*/

    pstatus = CLI$PRESENT (&filespec_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&filespec_q, &search_d, &val_len);

	if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n file spec. value absent");
	    LIB$STOP (gstatus);
	}
	else if (gstatus != SS$_NORMAL)
	{
	    printf ("\n something wrong with file spec. value");
	    LIB$STOP (gstatus);
	};
    }
    else
    {
	printf ("\n file spec. absent");
	LIB$STOP (pstatus);
    };

/* first line processing						*/

    pstatus = CLI$PRESENT (&first_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&first_q, &first_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    status = LIB$CVT_DTB (val_len, first_d.dsc$a_pointer,
				    &first_line_n);
	    if (status != SS$_NORMAL)
	    {
		printf ("\n invalid number for first line");
		LIB$STOP (status);
	    };
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    first_line_n = 1;
	}
	else
	{
	    printf ("\n something wrong with first line qualifier");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus == CLI$_DEFAULTED)
    {
	first_line_n = 1;
    }
    else if (pstatus == CLI$_ABSENT)
    {
	printf ("\n first line qualifier absent");
	first_line_n = 1;
    }
    else
    {
	printf ("\n something wrong with first line qualifier");
	LIB$STOP (pstatus);
    };

/* last line processing							*/

    pstatus = CLI$PRESENT (&last_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&last_q, &last_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    status = LIB$CVT_DTB (val_len, last_d.dsc$a_pointer,
				    &last_line_n);
	    if (status != SS$_NORMAL)
	    {
		printf ("\n invalid number for last line");
		LIB$STOP (status);
	    };
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    last_line_n = 0;
	}
	else
	{
	    printf ("\n something wrong with last line qual.");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus == CLI$_DEFAULTED)
    {
	last_line_n = 0;
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n last line qualifier absent");
	LIB$STOP (pstatus);
    };

/* CASE_SENSITIVE qualifier (preserve case sensitivity on searches)	*/

    pstatus = CLI$PRESENT (&case_q);

    if (pstatus == CLI$_PRESENT)
    {
	flag.case_sens = TRUE;
    }
    else if (pstatus == CLI$_DEFAULTED)
    {
	flag.case_sens = TRUE;		/* default is preserve case	*/
    }
    else if (pstatus == CLI$_ABSENT)
    {
	flag.case_sens = TRUE;		/* default is preserve case	*/
    }
    else if (pstatus == CLI$_NEGATED)
    {
	flag.case_sens = FALSE;
    }
    else
    {
	printf ("\n something wrong with CASE_SENSITIVE qualifier");
	LIB$STOP (pstatus);
    };

/* starting_char processing						*/

    pstatus = CLI$PRESENT (&starting_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&starting_q, &start_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.starting = TRUE;

	    if (start_d.dsc$w_length > 1)	/* if more than 1 char	*/
	    {					/* trim to 1 char.	*/
		tstatus = STR$LEN_EXTR (&start_d, &start_d, &1, &1);

		if ((tstatus & 1) != 1)
		{
		    LIB$SIGNAL (tstatus);
		};
	    };

	    if (!flag.case_sens)	/* if not preserving case	*/
	    {
		tstatus = STR$UPCASE (&start_d, &start_d);

		if ((tstatus & 1) != 1)
		{
		    LIB$SIGNAL (tstatus);
		};
	    };
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.starting = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n starting char value absent");
	    LIB$STOP (gstatus);
	}
	else
	{
	    printf ("\n something wrong with starting char value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n problem with starting char qualifier");
	LIB$STOP (pstatus);
    };

/* final_char processing						*/

    pstatus = CLI$PRESENT (&final_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&final_q, &final_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.final = TRUE;

	    if (final_d.dsc$w_length > 1)	/* if more than 1 char	*/
	    {					/* trim to 1 char.	*/
		tstatus = STR$LEN_EXTR (&final_d, &final_d, &1, &1);

		if ((tstatus & 1) != 1)
		{
		    LIB$SIGNAL (tstatus);
		};
	    };

	    if (!flag.case_sens)	/* if not preserving case	*/
	    {
		tstatus = STR$UPCASE (&final_d, &final_d);

		if ((tstatus & 1) != 1)
		{
		    LIB$SIGNAL (tstatus);
		};
	    };
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.final = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n final char value absent");
	    LIB$STOP (gstatus);
	}
	else
	{
	    printf ("\n something wrong with final char value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with final char qualifier");
	LIB$STOP (pstatus);
    };

/* remove processing							*/

    pstatus = CLI$PRESENT (&remove_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&remove_q, &remove_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.remove = TRUE;

	    if (!flag.case_sens)	/* if not preserving case	*/
	    {
		tstatus = STR$UPCASE (&remove_d, &remove_d);

		if ((tstatus & 1) != 1)
		{
		    LIB$SIGNAL (tstatus);
		};
	    };
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.remove = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n remove value absent");
	    LIB$STOP (gstatus);
	}
	else
	{
	    printf ("\n something wrong with remove value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with remove qualifier");
	LIB$STOP (pstatus);
    };

/* replace and single processing					*/

    if (flag.remove)		/* only done if /REMOVE is being done	*/
    {
	pstatus = CLI$PRESENT (&replace_q);

	if (pstatus == CLI$_PRESENT)
	{
	    gstatus = CLI$GET_VALUE (&replace_q, &replace_d, &val_len);

	    if (gstatus == SS$_NORMAL)
	    {
/*
   If the user is removing something and replacing it with exactly what
   is being removed, don't bother doing either operation.
*/
		if (STR$COMPARE_EQL (&replace_d, &remove_d) == 0)
		{
		    flag.remove  = FALSE;	/* strings are same,	*/
		    flag.replace = FALSE;	/* don't replace	*/
		}
		else
		{
		    flag.replace = TRUE;	/* Do the replacement	*/
		};
	    }
	    else if (gstatus == CLI$_DEFAULTED)
	    {
		flag.replace = FALSE;
	    }
	    else if (gstatus == CLI$_ABSENT)
	    {
		printf ("\n replace value absent");
		LIB$STOP (gstatus);
	    }
	    else
	    {
		printf ("\n something wrong with replace value");
		LIB$STOP (gstatus);
	    };
	}
	else if (pstatus != CLI$_ABSENT)
	{
	    printf ("\n something wrong with replace qualifier");
	    LIB$STOP (pstatus);
	};			/* end of replace processing		*/

	pstatus = CLI$PRESENT (&single_q);

	if (pstatus == CLI$_PRESENT)
	{
	    flag.single = TRUE;		/* Do first replacement only.	*/
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.single = FALSE;
	}
	else if (pstatus != CLI$_ABSENT)
	{
	    printf ("\n something wrong with single qualifier");
	    LIB$STOP (pstatus);
	};				/* end of single processing	*/
    };			/* end of if remove qualifier present		*/

/* prefix processing							*/

    flag.prefirst = FALSE;

    pstatus = CLI$PRESENT (&prefix_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&prefix_q, &prefix_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.prefix = TRUE;
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.prefix = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n prefix value absent");
	    LIB$STOP (gstatus);
	}
	else if (gstatus == CLI$_COMMA)	/* if more than one prefix	*/
	{
	    flag.prefix = TRUE;		/* prefix flag is true		*/

	    status = STR$COPY_DX (&prefirst_d, &prefix_d);  /* store first */

	    tstatus = CLI$GET_VALUE (&prefix_q, &prefix_d, &val_len);

	    if (tstatus != SS$_NORMAL)	/* if we didn't get the second	*/
	    {				/* one correctly		*/
		printf ("\n something wrong with second prefix value");
		LIB$STOP (tstatus);
	    };
	    flag.prefirst = TRUE;	/* prefix first line is true	*/
	}
	else
	{
	    printf ("\n something wrong with prefix value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with prefix qualifier");
	LIB$STOP (pstatus);
    };
/*
   May need to check to see if the first prefix string had FIRST: or
   FIRST= at the beginning and remove it
*/

/* suffix processing							*/

    flag.suffirst = FALSE;		/* assume no 1st line suffix	*/

    pstatus = CLI$PRESENT (&suffix_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&suffix_q, &suffix_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.suffix = TRUE;
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.suffix = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n suffix value absent");
	    LIB$STOP (gstatus);
	}
	else if (gstatus == CLI$_COMMA)	/* if more than one suffix	*/
	{
	    flag.suffix = TRUE;		/* suffix flag is true		*/

	    status = STR$COPY_DX (&suffirst_d, &suffix_d);  /* store first */

	    tstatus = CLI$GET_VALUE (&suffix_q, &suffix_d, &val_len);

	    if (tstatus != SS$_NORMAL)	/* if we didn't get the second	*/
	    {				/* one correctly		*/
		printf ("\n something wrong with second suffix value");
		LIB$STOP (tstatus);
	    };

	    flag.suffirst = TRUE;	/* suffix first line is true	*/
	}
	else
	{
	    printf ("\n something wrong with suffix value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with suffix qualifier");
	LIB$STOP (pstatus);
    };

/* TRIM qualifier (trim blank lines)					*/

    pstatus = CLI$PRESENT (&trim_q);

    if (pstatus == CLI$_PRESENT)
    {
	flag.trim = TRUE;
    }
    else if (pstatus == CLI$_ABSENT)
    {
	flag.trim = FALSE;
    }
    else if (pstatus == CLI$_NEGATED)
    {
	flag.trim = FALSE;
    }
    else
    {
	printf ("\n something wrong with TRIM qual");
	LIB$STOP (pstatus);
    };

/* STRIP qualifier (strip non-printing characters)			*/

    pstatus = CLI$PRESENT (&strip_q);

    if (pstatus == CLI$_PRESENT)
    {
	flag.strip = TRUE;
    }
    else if (pstatus == CLI$_ABSENT)
    {
	flag.strip = FALSE;
    }
    else if (pstatus == CLI$_NEGATED)
    {
	flag.strip = FALSE;
    }
    else
    {
	printf ("\n something wrong with STRIP qual");
	LIB$STOP (pstatus);
    };

/* TRAIL qualifier (strip trailing blanks and tabs)			*/

    pstatus = CLI$PRESENT (&trail_q);

    if ((pstatus == CLI$_PRESENT) || (pstatus == CLI$_DEFAULTED))
    {
	flag.trail = TRUE;
    }
    else if ((pstatus == CLI$_ABSENT) || (pstatus == CLI$_NEGATED))
    {
	flag.trail = FALSE;
    }
    else
    {
	printf ("\n something wrong with TRAIL qual");
	LIB$STOP (pstatus);
    };

/* STATISTICS qualifier (print out statistics)				*/

    pstatus = CLI$PRESENT (&statist_q);

    if (pstatus == CLI$_PRESENT)
    {
	flag.statistics = TRUE;
    }
    else if (pstatus == CLI$_ABSENT)
    {
	flag.statistics = FALSE;
    }
    else if (pstatus == CLI$_NEGATED)
    {
	flag.statistics = FALSE;
    }
    else
    {
	printf ("\n something wrong with STATISTICS qual");
	LIB$STOP (pstatus);
    };

/* append processing							*/

    pstatus = CLI$PRESENT (&append_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&append_q, &append_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.append = TRUE;
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.append = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n append value absent");
	    LIB$STOP (gstatus);
	}
	else
	{
	    printf ("\n something wrong with append value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with append qualifier");
	LIB$STOP (pstatus);
    };

/* prepend processing							*/

    pstatus = CLI$PRESENT (&prepend_q);

    if (pstatus == CLI$_PRESENT)
    {
	gstatus = CLI$GET_VALUE (&prepend_q, &prepend_d, &val_len);

	if (gstatus == SS$_NORMAL)
	{
	    flag.prepend = TRUE;
	}
	else if (gstatus == CLI$_DEFAULTED)
	{
	    flag.prepend = FALSE;
	}
	else if (gstatus == CLI$_ABSENT)
	{
	    printf ("\n prepend value absent");
	    LIB$STOP (gstatus);
	}
	else
	{
	    printf ("\n something wrong with prepend value");
	    LIB$STOP (gstatus);
	};
    }
    else if (pstatus != CLI$_ABSENT)
    {
	printf ("\n something wrong with prepend qualifier");
	LIB$STOP (pstatus);
    };
/*
   All done. No status returned, because we won't get here if there is
   a serious error.
*/

    return;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      