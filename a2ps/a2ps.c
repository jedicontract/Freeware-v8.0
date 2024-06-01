
/************************************************************************/
/*									*/
/* Description: Ascii to PostScript printer program.			*/
/* File: imag:/users/local/src/a2ps/a2ps.c					*/
/* Created: Mon Nov 13 14:51:15 1990 by miguel@imag (Miguel Santana)	*/
/* Version: 4.0								*/
/*									*/
/* Edit history:							*/
/* 1) Derived of shell program written by evan@csli (Evan Kirshenbaum).	*/
/*    Written in C for improve speed execution and portability. Many	*/
/*    improvements have been added.					*/
/* Fixes by Oscar Nierstrasz @ cui.uucp:				*/
/* 2) Fixed incorrect handling of stdin (removed error if no file names)*/
/* 3) Added start_page variable to eliminate blank pages printed for	*/
/*	files that are exactly multiples of 132 lines (e.g., man pages)	*/
/* Modified by miguel@imag:						*/
/* 4) Added new options at installation : sheet format (height/width in	*/
/*    inches), page format (number of columns per line and of lines per	*/
/*    page).								*/
/* Modified by miguel@imag:						*/
/* 5) Added new option to print n copies of a same document.		*/
/* 6) Cut long filenames if don't fit in the page header.		*/
/* Modified by Tim Clark (T.Clark@warwick.ac.uk):			*/
/* 7) Two additional modes of printing (portrait and wide format modes)	*/
/* 8) Fixed to cope with filenames which contain a character which must	*/
/*    be escaped in a PostScript string.				*/
/* Modified by miguel@imag.fr to					*/
/* 9) Added new option to suppress heading printing.			*/
/* 10) Added new option to suppress page surrounding border printing.	*/
/* 11) Added new option to change font size. Lines and columns are	*/
/*     automatically adjusted, depending on font size and printing mode	*/
/* 12) Minor changes (best layout, usage message, etc).			*/
/* Modified by tullemans@apolloway.prl.philips.nl			*/
/* 13) Backspaces (^H) are now handled correctly.			*/
/* Modified by Johan Vromans (jv@mh.nl) to				*/
/* 14) Added new option to give a header title that replaces use of	*/
/*     filename.							*/
/* Modified by craig.r.stevenson@att.com to				*/
/* 15) Print last modification date/time in header                      */
/* 16) Printing current date/time on left side of footer (optional)	*/
/* Modified by erikt@cs.umu.se:						*/
/* 17) Added lpr support for the BSD version				*/
/* 18) Added som output of pages printed.				*/
/* Modified by wstahw@lso.win.tue.nl:					*/
/* 19) Added option to allowing the printing of 2 files in one sheet	*/
/* Modified by mai@wolfen.cc.uow.oz					*/
/* 20) Added an option to set the lines per page to a specified value.	*/
/* 21) Added support for printing nroff manuals				*/
/* Modified by miguel@imag.fr						*/
/* 22) Integration of changes.						*/
/* 23) No more standard header file (printed directly by a2ps).		*/
/* 24) New format for command options.					*/
/* 25) Other minor changes.						*/
/* 26) ML. 6-MAR-1996 19:09:09.24  Fixed case NULL and void main.       */
/* 27) ML. 11-SEP-1996 14:29:31.91 Avoid div by 0 in is_binaryfile.     */
/* Modified by duffy@process.com                                        */
/* 28) 28-oct-2004, fixed accvio in fprintf(stderr) upon bad -o value   */ 
/* 29) 28-OCT-2004, Compile on VMS/IA64 (worked w/o any source changes) */
/*									*/
/************************************************************************/

/*
 * Copyright (c) 1990, Miguel Santana, miguel@imag.imag.fr
 *
 * Permission is granted to copy and distribute this file in modified
 * or unmodified form, for noncommercial use, provided (a) this copyright
 * notice is preserved, (b) no attempt is made to restrict redistribution
 * of this file, and (c) this file is not distributed as part of any
 * collection whose redistribution is restricted by a compilation copyright.
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <types.h>
#include <stat.h>

#ifdef VMS
#include <string.h>
#include <time.h>
#endif

#ifdef ANSIC
#include <time.h>
#include <string.h>
#else
#ifdef BSD
#include <sys/time.h>
#include <strings.h>
#else
#ifdef SYSV
#include <sys/timeb.h>
#include <time.h>
/*#else
error !       */
#endif
#endif
#endif

#define	FALSE	0
#define	TRUE	1
    
/*
 * The following values seem to work best for US "letter", while still
 * leaving a bit of a margin....   (HG)
 */
#ifndef WIDTH
#define	WIDTH	8.5
#endif
    
#ifndef HEIGHT
#define	HEIGHT	10.75
#endif
    
#ifndef MARGIN
#define	MARGIN	.5
#endif
    
#ifndef DIR_SEP
#define	DIR_SEP	'/'
#endif
    
#ifndef LPR_PRINT
#ifdef VMS
#define LPR_PRINT	FALSE
#else
#define LPR_PRINT	TRUE
#endif
#endif

#if LPR_PRINT
#ifndef LPR_COMMAND
#define LPR_COMMAND	"lprint"
#endif
#ifndef LPR_OPT
#define	LPR_OPT	" "
#endif
#endif
    
#define	PORTRAIT_HEADER		0.29
#define	LANDSCAPE_HEADER	0.22
#define	PIXELS_INCH		72
#define MAXFILENAME		20
#define MAX_LINES               160		/* max. lines per page */
#define MAN_LINES               66		/* no lines for a man */
#define IS_ROMAN		0		/* normal font */
#define IS_BOLD			1		/* bold sequence flag */

   
/*
 * Forward declarations for service functions.
 */
int is_binaryfile();
void print_standard_prologue();
void startpage(), endpage();

/*
 * A2ps flags related to options.
 */
int numbering = FALSE;		/* Line numbering option */
int folding = TRUE;		/* Line folding option */
int restart = FALSE;		/* Don't restart page number after each file */
int only_printable = FALSE;	/* Replace non printable char by space */
int interpret = TRUE;		/* Interpret TAB, FF and BS chars option */
int print_binaries = FALSE;	/* Force printing for binary files */ 
int landscape = TRUE;		/* Otherwise portrait format sheets */
int new_landscape = TRUE;	/* To scrute changes of landscape option */
int wide_pages = FALSE;         /* TRUE implies landscape, not twinpage */
int new_wide_pages = FALSE;	/* To scrute changes of wide option */
int twinpage = TRUE;		/* 2 pages per sheet if true, 1 otherwise */
int twinfiles = FALSE;		/* Allow 2 files per sheet */
int no_header = FALSE;		/* TRUE if user doesn't want the header */
int no_border = FALSE;		/* Don't print the surrounding border ? */
int printdate = FALSE;		/* Print current date as footnote */
#if LPR_PRINT
int lpr_print = LPR_PRINT;	/* Fork a lpr process to do the printing */
#endif

/*
 * Counters of different kinds.
 */
int column = 0;			/* Column number (in current line) */
int line = 0;			/* Line number (in current page) */
int line_number = 0;		/* Source line number */
int pages = 0;			/* Number of logical pages printed */
int sheets = 0;			/* Number of physical pages printed */
int old_pages, old_sheets;	/* Value before printing current file */
int sheetside = 0;		/* Side of the sheet currently printing */
int linesperpage;		/* Lines per page */
int lines_requested = 0;	/* Lines per page requested by the user */
int new_linesrequest = 0;	/* To scrute new values for lines_requested */
int columnsperline;		/* Characters per output line */
int nonprinting_chars, chars;	/* Number of nonprinting and total chars */
int copies_number = 1;		/* Number of copies to print */
int column_width = 8;	        /* Default column tab width (8) */

/*
 * Other global variables.
 */
int first_page;			/* First page for a file */
int no_files = TRUE;		/* No file until now */
int prefix_width;		/* Width in characters for line prefix */
float fontsize = 0.0;		/* Size of a char for body font */
float new_fontsize = 0.0;	/* To scrute new values for fontsize */
char *command;			/* Name of a2ps program */
char *lpr_opt = NULL;		/* Options to lpr */
char *header_text = NULL;	/* Allow for different header text */
char *prologue = NULL;		/* postscript header file */

/*
 * Sheet dimensions
*/
double page_height = HEIGHT;	/* Paper height */
double page_width = WIDTH;	/* Paper width */

/*
 * Print a usage message.
 */
void
usage()
{
/*fprintf(stderr,"Usage: %s [options] [ f1 [ [H_opt] f2 ...] ]\n", command);*/
/*Ge"andert f"ur VAX, 24.11.94 EW */
    fprintf(stderr,"Usage: a2ps [options] [ f1 [ [H_opt] f2 ...] ]\n");
    fprintf(stderr,"options = -#num\t\tnumber of copies to print\n");
    fprintf(stderr,"          -?\t\tprint this information\n");
    fprintf(stderr,"          -b\t-nb\tforce (DON'T FORCE) binary printing\n");
    fprintf(stderr,"          -c\t-nc\tallow (DON'T ALLOW) two files on the same sheet\n");
    fprintf(stderr,"          -d\t-nd\tPRINT (don't print) current date at the bottom\n");
    fprintf(stderr,"          -Fnum\t\tfont size, num is a float number\n");
    fprintf(stderr,"          -f\t-nf\tFOLD (don't fold) lines\n");
    fprintf(stderr,"          -Hstr\t\tuse str like header title for subsequent files\n");
    fprintf(stderr,"          \t-nH\tdon't print any header\n");
    fprintf(stderr,"          -h\t\tprint this information\n");
    fprintf(stderr,"          -Ifile\t\tinclude this file as a2ps prologue\n");
    fprintf(stderr,"          -i\t-ni\tINTERPRET (don't interpret) tab, bs and ff chars\n");
    fprintf(stderr,"          -lnum\t\tuse num lines per page\n");
    fprintf(stderr,"          -m\t\tprocess the file as a man\n");
    fprintf(stderr,"          -n\t-nn\tNUMBER (don't number) line files\n");
#if LPR_PRINT
    fprintf(stderr,"          -Pprinter\t-nP\tSEND (don't send) directly to the printer");
#ifdef LPR_OPT
    if (LPR_OPT != NULL && sizeof(LPR_OPT) > 0)
	fprintf(stderr,"\n             (with options '%s' and -Pprinter)", LPR_OPT);
#endif
    fprintf(stderr, "\n");
#endif
#ifdef VMS
    fprintf(stderr,"          -ofile\t\toutput file\n");
#endif
    fprintf(stderr,"          -p\t-np\tprint in portrait (LANDSCAPE) mode\n");
    fprintf(stderr,"          -r\t-nr\tRESTART (don't restart) page number after each file\n");
    fprintf(stderr,"          -s\t-ns\tPRINT (don't print) surrounding borders\n");
    fprintf(stderr,"          -tnum\t\tset tab size to n\n");
    fprintf(stderr,"          -v\t-nv\tVISIBLE (blank) display of unprintable chars\n");
    fprintf(stderr,"          -w\t-nw\twide (NARROW, 2-up if landscape)\n");
    exit(1);
}

/*
 * Set an option only if it's global.
 */
void
set_global_option(arg)
char *arg;
{
    switch (arg[1]) {
    case '?':				/* help */
    case 'h':
	usage();
    case 'b':				/* print binary files */
	if (arg[2] != (char)NULL)
	    usage();
	print_binaries = TRUE;
	break;
    case 'c':				/* allow two files per sheet */
	if (arg[2] != (char)NULL)
	    usage();
	twinfiles = TRUE;
	break;
    case 'f':				/* fold lines too large */
	if (arg[2] != (char)NULL)
	    usage();
	folding = TRUE;
	break;
    case 'I':				/* include this file as a2ps prologue */
	if (arg[2] == (char)NULL)
	    usage();
	    prologue = arg+2;
	break;
    case 'i':				/* interpret control chars */
	if (arg[2] != (char)NULL)
	    usage();
	interpret = TRUE;
	break;
    case 'n':
	if (arg[2] == (char)NULL)
	    return;
	if (arg[3] != (char)NULL)
	    usage();
	switch (arg[2]) {
	case 'b':			/* don't print binaries */
	    print_binaries = FALSE;
	    break;
	case 'c':			/* don't allow 2 files/sheet */
	    twinfiles = FALSE;
	    break;
	case 'f':			/* cut lines too long */
	    folding = FALSE;
	    break;
	case 'H':			/* don't print header */
	    no_header = TRUE;
	    break;
	case 'i':			/* don't interpret ctrl chars */
	    interpret = FALSE;
	    break;
#if LPR_PRINT
	case 'P':			/* don't lpr */
	    lpr_print = FALSE;
	    break;
#endif
	case 'r':			/* don't restart sheet number */
	    restart = FALSE;
	    break;
	case 'v':			/* only printable chars */
	    only_printable = TRUE;
	    break;
	case 'd':
	case 'm':
	case 'n':
	case 'p':
	case 's':
	case 'w':
	    if (arg[3] != (char)NULL)
		usage();
	    return;
	default:
	    usage();
	}
	break;
#ifdef VMS
    case 'o':				/* output file */
        if( arg[2] == (char)NULL ) 
            usage();
	if (freopen(&arg[2], "w", stdout) == NULL) {
	    fprintf(stderr, "Error opening output file %s\n", arg+2);/*VMS-friendly*/
	    /*fprintf(stderr, "Error opening output file %s\n", arg[2]);*//*ORIGINAL*/
	    exit(1);
	}
        break;
#endif
#if LPR_PRINT
    case 'P':				/* fork a process to print */ 
	if (arg[2] != (char)NULL) {
	    lpr_opt = (char *)malloc(strlen(arg)+1);
	    strcpy(lpr_opt, arg);
	}
	lpr_print = TRUE;
	break;
#endif
    case 'r':				/* restart sheet number */
	if (arg[2] != (char)NULL)
	    usage();
	restart = TRUE;
	break;
    case 't':				/* set tab size */
	if (arg[2] == (char)NULL || (column_width = atoi(arg+2)) <= 0)
	    usage();
	break;
    case 'v':				/* print control chars */
	if (arg[2] != (char)NULL)
	    usage();
	only_printable = FALSE;
	break;
    case 'd':
    case 'm':
    case 'p':
    case 's':
    case 'w':
	if (arg[2] != (char)NULL)
	    usage();
    case '#':
    case 'F':
    case 'H':
    case 'l':
	return;
    default:
	usage();
    }
    arg[0] = (char)NULL;
}

/*
 * Set an option of the command line. This option will be applied
 * to all files that will be found in the rest of the command line.
 * The -H option is the only exception: it is applied only to the
 * file.
 */
void
set_positional_option(arg)
char *arg;
{
    int copies;
    int lines;
    float size;

    switch (arg[1]) {
    case '\0':				/* global option */
	break;
    case '#':				/* n copies */
	if (sscanf(&arg[2], "%d", &copies) != 1 || copies <= 0)
	    fprintf(stderr, "Bad number of copies: '%s'. Ignored\n", &arg[2]);
	else
	    copies_number = copies;
	printf("/#copies %d def\n", copies_number);
	break;
    case 'd':				/* print current date/time */
	printdate = TRUE;
	break;
    case 'F':				/* change font size */
	if (arg[2] == (char)NULL || sscanf(&arg[2], "%f", &size) != 1 || size == 0.0)
	{
	    fprintf(stderr, "Wrong value for option -F: '%s'. Ignored\n",
		    &arg[2]);
	    break;
	}
	new_fontsize = size;
	break;
    case 'H':				/* header text */
	header_text = arg+2;
	break;
    case 'l':				/* set lines per page */
	/* Useful with preformatted files. Scaling is automatically	*/
	/* done when necessary.						*/
	if (arg[2] == (char)NULL || sscanf(&arg[2], "%d", &lines) != 1 ||
	    lines < 0 || lines > MAX_LINES)
	{
	    fprintf(stderr, "Wrong value for option -l: '%s'. Ignored\n",
		    &arg[2]);
	    break;
	}
	new_linesrequest = lines;
	break;
    case 'm':				/* Process file as a man */
	new_linesrequest = MAN_LINES;
	numbering = FALSE;
	break;
    case 'n':				/* number file lines */
	if (arg[2] == (char)NULL) {
	    numbering = TRUE;
	    break;
	}
	switch (arg[2]) {
	case 'd':			/* don't print date/time */
	    printdate = FALSE;
	    break;
	case 'm':			/* stop processing as a man */
	    new_linesrequest = 0;
	    break;
	case 'n':			/* don't number lines */
	    numbering = FALSE;
	    break;
	case 'p':			/* landscape format */
	    new_landscape = TRUE;
	    break;
	case 's':			/* no surrounding border */
	    no_border = TRUE;
	    break;
	case 'w':			/* twin pages */
	    new_wide_pages = FALSE;
	    break;
	default:
	    usage();
	}
	break;
    case 'p':				/* portrait format */
	if (arg[2] != (char)NULL)
	    usage();
	new_landscape = FALSE;
	break;
    case 's':				/* surrounding border */
	if (arg[2] != (char)NULL)
	    usage();
	no_border = FALSE;
	break;
    case 'w':				/* wide format */
	if (arg[2] != (char)NULL)
	    usage();
	new_wide_pages = TRUE;
	break;
    default:
	usage();
    }
}


/****************************************************************/
/*			Service routines			*/
/****************************************************************/

/*
 * This routine buffers a line of input, release one character at a time
 * or a whole sequence of characters with some meaning like bold sequences
 * produced by nroff (no others sequences are recognized by the moment):
 *        <c><\b><c><\b><c><\b><c>
 */
int
mygetc(statusp)
int *statusp;
{
    static int curr = 0;
    static int size = 0;
    static unsigned char buffer[512];
    int c;

    *statusp = IS_ROMAN;

    /* Read a new line, if necessary */
    if (curr >= size) {
	if (gets((char *)buffer) == NULL)
	    return  EOF;
	size = strlen((const char *)buffer);
	buffer[size] = '\n';
	buffer[++size] = '\0';
	curr = 0;
    }
    if (buffer[curr+1] != '\b')		/* this is not a special sequence */
	return  buffer[curr++];

    /* Check if it is a bold sequence */
    if ((c = buffer[curr++]) == buffer[curr+1] &&
	buffer[curr]	== buffer[curr+2] &&
	c		== buffer[curr+3] &&
	buffer[curr]	== buffer[curr+4] &&
	c		== buffer[curr+5])
    {
	*statusp = IS_BOLD;
	curr += 6;
    }

    /* Return the first character of the sequence */
    return  c;
}

/*
 * Test if we have a binary file.
 */
int
is_binaryfile(name)
char *name;
{
    if (chars > 120 || pages > 1) {
	first_page = FALSE;
	if (chars > 0 && !print_binaries && (nonprinting_chars*100 / chars) >= 60)
	{
	    fprintf(stderr, "%s is a binary file: printing aborted\n", name);
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * Cut long filenames.
 */
void
cut_filename(old_name, new_name)
char *old_name, *new_name;
{
    register char *p;
    register int i;
    
    p = old_name + (strlen(old_name)-1);
    while (p >= old_name && *p != DIR_SEP) p--;
    
    for (i = 0, p++; *p != (char)NULL && i < MAXFILENAME; i++)
	*new_name++ = *p++;
    *new_name = (char)NULL;
}

/*
 * Print a char in a form accept by postscript printers.
 */
int
printchar(c)
unsigned char c;
{
    if (c >= ' ' && c < 0177)
    {
	if (c == '(' || c == ')' || c == '\\')
	    putchar('\\');
	putchar(c);
	return 0;
    }
    
    if (only_printable)
    {
	putchar(' ');
	return 1;
    }
    
    if (c > 0177)
    {
	printf("M-");
	c &= 0177;
    }
    if (c < ' ')
    {
	putchar('^');
	if ((c = c + '@') == '(' || c == ')' || c == '\\')
	    putchar('\\');
	putchar(c);
    }
    else if (c == 0177)
	printf("^?");
    else
	putchar(c);
    
    return 1;
}

/*
 * Begins a new physical page.
 */
void
skip_page()
{
    if (twinpage == FALSE || sheetside == 1)
	printf("%%%%Page: %d %d\n", sheets+1, sheets+1);
    startpage();
}

/*
 * Fold a line too long.
 */
int
fold_line(name)
char *name;
{
    column = 0;
    printf(") s\n");
    if (++line >= linesperpage)
    {
	endpage();
	skip_page();
	if (first_page && is_binaryfile(name))
	    return FALSE;
	line = 0;
    }
    if (numbering)
	printf("(     +");
    else
	printf("( ");
    
    return TRUE;
}

/*
 * Cut a textline too long to the size of a page line.
 */
char
cut_line()
{
    int c;
    int status;

    while ((c = mygetc(&status)) != EOF && c != '\n' && c != '\f');
    return c;
}


/****************************************************************/
/*			"Postscript" routines.			*/
/****************************************************************/

/*
 * Print a physical page.
 */
void
printpage()
{
    sheetside = 0;
    sheets++;
    if (no_border == FALSE && (twinpage || no_header))
	printf("%d sheetnumber\n", sheets - (restart ? old_sheets : 0));
    if (printdate)
	printf("currentdate\n");
    printf("showpage\n");
}

/*
 * Prints page header and page border and
 * initializes printing of the file lines.
 */
void
startpage()
{
    if (sheetside == 0 && landscape){
	printf("sheetwidth 0 inch translate	%% new origin\n");
	printf("90 rotate			%% landscape format\n");
    }
    pages++;
    if (no_header == FALSE)
	printf("%d %d header\n", pages - old_pages, sheetside);
    if (no_border == FALSE) {
	printf("%d border\n", sheetside);
	if (no_header == FALSE)
	    printf("hborder\n");
    }
    printf("/x0 upperx %d get bodymargin add def\n", sheetside);
    printf("/y0 uppery bodymargin bodyfontsize add %s add sub def\n",
	   no_header ? "0" : "headersize");
    printf("x0 y0 moveto\n");
    printf("bodyfont setfont\n");
}

/*
 * Terminates printing, flushing last page.
 */
void
cleanup()
{
    if (twinpage && sheetside == 1)
	printpage();
}

/*
 * Adds a sheet number to the page (footnote) and prints the formatted
 * page (physical impression). Activated at the end of each source page.
*/
void
endpage()
{
    if (twinpage && sheetside == 0)
	sheetside = 1;
    else
	printpage();
}


/****************************************************************/
/*		Printing a file					*/
/****************************************************************/

/*
 * Print the file prologue.
 */
void
print_file_prologue(name, title)
char *name, *title;
{
    int new_format, new_font;
    char *p, *string;
    int lines;
    float char_width, header_size;
    struct stat statbuf;
    char new_title[MAXFILENAME+1];

    /* Print last page of previous file, if necessary */
    if (!twinfiles)
	cleanup();

    /* Initialize variables related to the format */
    new_format = FALSE;
    if (new_landscape != landscape || new_wide_pages != wide_pages) {
	landscape = new_landscape;
	wide_pages = new_wide_pages;
	twinpage = landscape && !wide_pages;
	new_format = TRUE;
	printf("/landscape %s def\n", landscape ? "true" : "false");
	printf("/twinpage %s def\n", twinpage ? "true" : "false");
	printf("%% Character size for fonts.\n");
	if (landscape)
	    printf("/filenmfontsize 11 def\n");
	else
	    printf("/filenmfontsize 15 def\n");
	printf("/datefontsize filenmfontsize 0.8 mul def\n");
	printf("/datefont /Helvetica datefontsize getfont def\n");
	printf("/datewidth datefont setfont currdate stringwidth pop def\n");
	printf("/stdfilenmfont filenmfontname filenmfontsize getfont def\n");
	printf("/headermargin filenmfontsize 0.25 mul def\n");
    }

    /* Initialize variables related to the header */
    if (no_header && name == title) {
	header_size = 0.0;
	printf("/headersize 0.0 def\n");
    }
    else {
	header_size = (landscape ? LANDSCAPE_HEADER : PORTRAIT_HEADER)*PIXELS_INCH;
	if (strlen(title) > MAXFILENAME) {
	    cut_filename(title, new_title);
	    title = new_title;
	}
	printf("/headersize %g inch def\n",
	       landscape ? LANDSCAPE_HEADER : PORTRAIT_HEADER);
    }

    /* Initialize variables related to the font size */
    new_font = FALSE;
    if (fontsize != new_fontsize || new_format ||
	lines_requested != new_linesrequest)
    {
	if (new_fontsize == 0.0 || (fontsize == new_fontsize && new_format))
	    new_fontsize = landscape ? 6.8 : 9.0;
	if (lines_requested != new_linesrequest) {
	    if ((lines_requested = new_linesrequest) != 0) {
		/* Scale fontsize */
		if (landscape)
		    lines = ((page_width - header_size) / new_fontsize) - 1;
		else
		    lines = ((page_height - header_size) / new_fontsize) - 1;
		if (lines_requested > lines || lines_requested <= 3*lines/4)
		    new_fontsize *= (float)lines / (float)lines_requested;
	    }
	}
	fontsize = new_fontsize;
	new_font = TRUE;
	printf("/bodyfontsize %g def\n", fontsize);
	printf("/bodyfont /CourierBack bodyfontsize getfont def\n");
	printf("/boldfont /Courier-Bold bodyfontsize getfont def\n");
	printf("/bodymargin bodyfontsize 0.7 mul def\n");
    }

    /* Initialize file printing, if there is any change */
    if (new_format || new_font) {
	char_width = 0.6 * fontsize;
	if (landscape) {
	    linesperpage = ((page_width - header_size) / fontsize) - 1;
	    if (wide_pages)
		columnsperline = (page_height / char_width) - 1;
	    else
		columnsperline = ((page_height / 2) / char_width) - 1;
	}
	else {
	    linesperpage = ((page_height - header_size) / fontsize) - 1;
	    columnsperline = (page_width / char_width) - 1;
	}
	if (lines_requested > 0)
	    linesperpage = lines_requested;
	if (linesperpage <= 0 || columnsperline <= 0) {
	    fprintf(stderr, "Font %g too big !!\n", fontsize);
	    exit(1);
	}
	printf("/lines %d def\n", linesperpage);
	printf("/columns %d def\n", columnsperline);
	printf("\n%% Logical page attributs (a half of a sheet).\n");
	printf("/pagewidth\n");
	printf("   bodyfont setfont (0) stringwidth pop columns mul bodymargin dup add add\n");
	printf("   def\n");
	printf("/pageheight\n");
	printf("   bodyfontsize lines mul bodymargin dup add add headersize add\n");
	printf("   def\n");
	printf("/filenmroom\n");
	printf("      pagewidth\n");
	printf("      filenmfontsize 4 mul datewidth add (Page 999) stringwidth pop add\n");
	printf("    sub\n");
	printf("  def\n");
	printf("\n%% Coordinates for upper corner of a logical page and for\n");
	printf("%% sheet number. Coordinates depend on format mode used.\n");
	printf("%% In twinpage mode, coordinate x of upper corner is not\n");
	printf("%% the same for left and right pages: upperx is an array of\n");
	printf("%% two elements, indexed by the side of the sheet.\n");
	printf("/topmargin margin twinpage {3} {2} ifelse div def\n");
	if (landscape) {
	    printf("%% Landscape format\n");
	    printf("/uppery rightmargin pageheight add bodymargin add def\n");
	    printf("/sheetnumbery datefontsize datefontsize add def\n");
	    printf("/datey sheetnumbery def\n");
	    if (twinpage) {
		printf("\n%% Two logical pages\n");
		printf("/upperx [ topmargin			%% left page\n");
		printf("          dup 2 mul pagewidth add	%% right page\n");
		printf("        ] def\n");
		printf("/sheetnumberx sheetheight topmargin sub def\n");
		printf("/datex topmargin def\n");
	    }
	    else {
		printf("\n%% Only one logical page\n");
		printf("/upperx [ topmargin dup ] def\n");
		printf("/sheetnumberx sheetheight topmargin sub datefontsize sub def\n");
		printf("/datex topmargin datefontsize add def\n");
	    }
	}
	else {
	    printf("\n%% Portrait format\n");
	    printf("/uppery topmargin pageheight add def\n");
	    printf("/upperx [ leftmargin dup ] def\n");
	    printf("/sheetnumberx sheetwidth rightmargin sub datefontsize sub def\n");
	    printf("/sheetnumbery\n");
	    printf("  sheetheight\n");
	    printf("  topmargin pageheight add datefontsize add headermargin add\n");
	    printf("    sub\n");
	    printf("  def\n");
	    printf("/datey sheetnumbery def\n");
	    printf("/datex leftmargin def\n");
	}
    }

    /* Retrieve file modification date and hour */
    if (strcmp(name, "stdin") != 0) {
	if (fstat(fileno(stdin), &statbuf) == -1) {
	    fprintf(stderr, "Error getting file modification timeOB\n");
	    exit(1);
	}
	string = ctime((const time_t *)&statbuf.st_mtime);
	printf("/date (%.6s %.4s %.8s) def\n", string+4, string+20, string+11);
    /* ge"ander f"ur VAX, EW */
	/*printf("/currdate () def\n");	/* curr time in header already */
    }
    else
	printf("/currdate () def\n");	/* curr time in header already */

    /* Start file impression */
    putchar('(');
    for (p = title; *p != (char)NULL;)
	printchar(*p++);
    printf(") newfile\n");
}

/*
 * Print one file.
 */
void
print_file(name, header_opt, header)
char *name, *header;
int header_opt;
{
    register int c;
    int nchars;
    int start_line, start_page;
    int continue_exit;
    int status, new_status;

    /* Reinitialize postscript variables depending on positional options */
    print_file_prologue(name, header == NULL ? name : header);
    
    /*
     * Boolean to indicates that previous char is \n (or interpreted \f)
     * and a new page would be started, if more text follows
     */
    start_page = FALSE;
    
    /*
     * Printing binary files is not very useful. We stop printing
     * if we detect one of these files. Our heuristic to detect them:
     * if 75% characters of first page are non-printing characters,
     * the file is a binary file.
     * Option -b force binary files impression.
     */
    nonprinting_chars = chars = 0;
    
    /* Initialize printing variables */
    column = 0;
    line = line_number = 0;
    first_page = TRUE;
    start_line = TRUE;
    prefix_width = numbering ? 6 : 1;

    /* Start printing */
    skip_page();

    /* Process each character of the file */
    status = IS_ROMAN;
    c = mygetc(&new_status);
    while (c != EOF) {
	/*
	 * Preprocessing (before printing):
	 * - TABs expansion (see interpret option)
	 * - FF and BS interpretation
	 * - replace non printable characters by a space or a char sequence
	 *   like:
	 *     ^X for ascii codes < 0x20 (X = [@, A, B, ...])
	 *     ^? for del char
	 *     M-c for ascii codes > 0x3f
	 * - prefix parents and backslash ['(', ')', '\'] by backslash
	 *   (escape character in postcript)
	 */
	/* Form feed */
	if (c == '\f' && interpret) {
	    /* Close current line */
	    if (!start_line) {
		printf(") s\n");
		start_line = TRUE;
	    }
	    /* start a new page ? */
	    if (start_page)
		skip_page();
	    /* Close current page and begin another */
	    endpage();
	    start_page = TRUE;
	    /* Verification for binary files */
	    if (first_page && is_binaryfile(name))
		return;
	    line = 0;
	    if ((c = mygetc(&new_status)) == EOF)
		break;
	}
	
	/* Start a new line ? */
	if (start_line)	{
	    if (start_page) {
		/* only if there is something to print! */
		skip_page();
		start_page = FALSE ;
	    }
	    if (numbering)
		printf("(%-5d|", ++line_number);
	    else
		printf("( ");
	    start_line = FALSE;
	}
	
	/* Is a new font ? This feature is used only to detect bold	*/
	/* sequences produced by nroff (man pages), in connexion with	*/
	/* mygetc.							*/
	if (status != new_status) {
	    printf(")\n");
	    printf("%s", status == IS_ROMAN ? "b" : "st");
	    printf(" (");
	    status = new_status;
	}

	/* Interpret each character */
	switch (c) {
	case '\b':
	    if (!interpret)
		goto print;
	    /* A backspace is converted to 2 chars ('\b'). These chars	*/
	    /* with the Courier backspace font produce correct under-	*/
	    /* lined strings.	*/
	    if (column)
		column--;
	    putchar('\\');
	    putchar('b');
	    break;
	case '\n':
	    column = 0;
	    start_line = TRUE;
	    printf(") s\n");
	    if (++line >= linesperpage) {
		endpage();
		start_page = TRUE ;
		if (first_page && is_binaryfile(name))
		    return;
		line = 0;
	    }
	    break;
	case '\t':
	    if (interpret) {
		continue_exit = FALSE;
		do {
		    if (++column + prefix_width > columnsperline) {
			if (folding) {
			    if (fold_line(name) == FALSE)
				return;
			}
			else {
			    c = cut_line();
			    continue_exit = TRUE;
			    break;
			}
		    }
		    putchar(' ');
		} while (column % column_width);
		if (continue_exit)
		    continue;
		break;
	    }
	default:
	print:
	    if (only_printable)
		nchars = 1;
	    else {
		nchars = c > 0177 ? 2 : 0;
		nchars += (c&0177) < ' ' || (c&0177) == 0177 ? 2 : 1;
	    }
	    if (prefix_width + (column += nchars) > columnsperline)
		if (folding) {
		    if (fold_line(name) == FALSE)
			return;
		}
		else {
		    c = cut_line();
		    new_status = IS_ROMAN;
		    continue;
		}
	    nonprinting_chars += printchar(c);
	    chars++;
	    break;
	}
	c = mygetc(&new_status);
    }
    
    if (!start_line)
	printf(") s\n");
    if (!start_page)
	endpage();
}


/****************************************************************/
/*		Print a postscript prologue for a2ps.		*/
/****************************************************************/

/*
 * Print the a2ps prologue.
 */
void
print_prologue()
{
    register int c;
    FILE *f;
    char *string;
#ifdef ANSIC
    time_t date;
#else
#ifdef BSD
    struct timeval date;
    struct tm *p;
#else
#ifdef SYSV
    struct timeb date;
#endif
#endif
#endif
    
    if (prologue == NULL)
	print_standard_prologue();
    else if ((f = fopen(prologue, "r")) != NULL) {
	/* Header file printing */
	while ((c = getc(f)) != EOF)
	    putchar(c);
    }
    else {
	fprintf(stderr, "Postscript header missing: %s\n", prologue);
	exit(1);
    }

    /* Completes the prologue with a2ps static variables */
    printf("\n%% Initialize page description variables.\n");
    printf("/x0 0 def\n");
    printf("/y0 0 def\n");
    printf("/sheetheight %g inch def\n", HEIGHT);
    printf("/sheetwidth %g inch def\n", WIDTH);
    printf("/margin %g inch def\n", MARGIN);
    printf("/rightmargin margin 3 div def\n");
    printf("/leftmargin margin 2 mul 3 div def\n");
    printf("/twinfiles %s def\n", twinfiles ? "true" : "false");
    printf("/date () def\n");

    /* Retrieve date and hour */
#ifdef ANSIC
    if (time(&date) == -1) {
	fprintf(stderr, "Error calculating time\n");
	exit(1);
    }
    string = ctime(&date);
#else
#ifdef BSD
    (void) gettimeofday(&date, (struct timezone *)0);
    p = localtime(&date.tv_sec);
    string = asctime(p);
#else
#ifdef SYSV
    (void)ftime(&date);
    string = ctime(&date.time);
#endif
#endif
#endif

    /* And print them */
    printf("/currdate (%.6s %.4s %.8s) def\n", string+4, string+20, string+11);

    /* Close prolog */
    printf("%%%%EndProlog\n\n");
    
    /* Go on */
    printf("/docsave save def\n");
}

/*
 * Print the standard prologue.
 */
void
print_standard_prologue()
{
    printf("%%!PS  PostScript Source Code\n");
    printf("%%\n");
    printf("%%%% Description: PostScript prolog for a2ps program.\n");
    printf("%%%% Copyright (c) 1990, Miguel Santana, miguel@imag.imag.fr\n");
    printf("%%%% a2ps 4.0\n");
    printf("%%%%EndComments\n");
    printf("\n/$a2psdict 100 dict def\n");
    printf("$a2psdict begin\n");
    printf("\n%% General macros.\n");
    printf("/xdef {exch def} bind def\n");
    printf("/getfont {exch findfont exch scalefont} bind def\n");
    printf("\n%% Create Courier backspace font\n");
    printf("/backspacefont {\n");
    printf("    /Courier findfont dup length dict begin\n");
    printf("	{ %%forall\n");
    printf("	    1 index /FID eq { pop pop } { def } ifelse\n");
    printf("	} forall\n");
    printf("	currentdict /UniqueID known { %%if\n");
    printf("	    /UniqueID UniqueID 16#800000 xor def\n");
    printf("	} if\n");
    printf("	CharStrings length 1 add dict begin\n");
    printf("	    CharStrings { def } forall\n");
    printf("	    /backspace { -600 0 0 0 0 0 setcachedevice } bind def\n");
    printf("	    currentdict\n");
    printf("	end\n");
    printf("	/CharStrings exch def\n");
    printf("	/Encoding Encoding 256 array copy def\n");
    printf("	Encoding 8 /backspace put\n");
    printf("	currentdict\n");
    printf("    end\n");
    printf("    definefont pop\n");
    printf("} bind def\n");
    printf("\n%% FUNCTIONS\n");
    printf("\n%% Function newfile: Initialize file printing.\n");
    printf("/newfile\n");
    printf("{ /filenm xdef\n");
    printf("  /filenmwidth filenm stringwidth pop def\n");
    printf("  /filenmfont\n");
    printf("       filenmwidth filenmroom gt\n");
    printf("       {\n");
    printf("	       filenmfontname\n");
    printf("	       filenmfontsize filenmroom mul filenmwidth div\n");
    printf("	     getfont\n");
    printf("       }\n");
    printf("       { stdfilenmfont }\n");
    printf("     ifelse\n");
    printf("  def\n");
    printf("} bind def\n");
    printf("\n%% Function header: prints page header. no page and\n");
    printf("%% sheetside are passed as arguments.\n");
    printf("/header\n");
    printf("  { upperx 1 index get  uppery headersize sub 1 add  moveto\n");
    printf("    datefont setfont\n");
    printf("    gsave\n");
    printf("      upperx 1 index get uppery moveto pop\n");
    printf("      0 headersize 2 div neg rmoveto \n");
    printf("      headersize setlinewidth\n");
    printf("      0.85 setgray\n");
    printf("      pagewidth 0 rlineto stroke\n");
    printf("    grestore\n");
    printf("    gsave\n");
    printf("      datefontsize headermargin rmoveto\n");
    printf("      date show				%% date/hour\n");
    printf("    grestore\n");
    printf("    gsave\n");
    printf("      pnum cvs pop				%% page pop up\n");
    printf("        pagewidth (Page 999) stringwidth pop sub\n");
    printf("        headermargin\n");
    printf("	  rmoveto\n");
    printf("      (Page ) show pnum show		%% page number\n");
    printf("    grestore\n");
    printf("    empty pnum copy pop\n");
    printf("    gsave\n");
    printf("      filenmfont setfont\n");
    printf("         filenmroom filenm stringwidth pop sub 2 div datewidth add\n");
    printf("          bodymargin 2 mul \n");
    printf("        add \n");
    printf("        headermargin\n");
    printf("      rmoveto\n");
    printf("        filenm show			%% file name\n");
    printf("      grestore\n");
    printf("    } bind def\n");
    printf("\n%% Function border: prints border page. Use sheetside as arg\n");
    printf("/border \n");
    printf("{ upperx 1 index get uppery moveto pop\n");
    printf("  gsave				%% print four sides\n");
    printf("    0.7 setlinewidth		%% of the square\n");
    printf("    pagewidth 0 rlineto\n");
    printf("    0 pageheight neg rlineto\n");
    printf("    pagewidth neg 0 rlineto\n");
    printf("    closepath stroke\n");
    printf("  grestore\n");
    printf("} bind def\n");
    printf("\n%% Function hborder: completes border of the header.\n");
    printf("/hborder \n");
    printf("{ gsave\n");
    printf("	0.7 setlinewidth\n");
    printf("	0 headersize neg rmoveto\n");
    printf("	pagewidth 0 rlineto\n");
    printf("	stroke\n");
    printf("  grestore\n");
    printf("} bind def\n");
    printf("\n%% Function sheetnumber: prints the sheet number.\n");
    printf("/sheetnumber\n");
    printf("    { sheetnumberx sheetnumbery moveto\n");
    printf("      datefont setfont\n");
    printf("      pnum cvs\n");
    printf("	  dup stringwidth pop (0) stringwidth pop sub neg 0 rmoveto show\n");
    printf("      empty pnum copy pop\n");
    printf("    } bind def\n");
    printf("\n%% Function currentdate: prints the current date.\n");
    printf("/currentdate\n");
    printf("    { datex datey moveto\n");
    printf("      bodyfont setfont\n");
    printf("      (Date: ) show\n");
    printf("      currdate show\n");
    printf("    } bind def\n");
    printf("\n%% Function s: print a source line\n");
    printf("/s  { show\n");
    printf("      /y0 y0 bodyfontsize sub def\n");
    printf("      x0 y0 moveto\n");
    printf("    } bind def\n");
    printf("\n%% Functions b and st: change to bold or standard font\n");
    printf("/b  { show\n");
    printf("      boldfont setfont\n");
    printf("    } bind def\n");
    printf("/st { show\n");
    printf("      bodyfont setfont\n");
    printf("    } bind def\n");
    printf("\n%% Strings used to make easy printing numbers\n");
    printf("/pnum 12 string def\n");
    printf("/empty 12 string def\n");
    printf("\n%% Global initializations\n");
    printf("\n/CourierBack backspacefont\n");
    printf("/filenmfontname /Helvetica-Bold def\n");
    printf("/inch {72 mul} bind def\n");
}


/*
 * Main routine for a2ps.
 */
int
main(argc, argv)
int argc;
char *argv[];
{
    register int narg;
    register char *arg;
    int number;
#if LPR_PRINT
    int fd[2], lpr_pid;
#endif
    
    /* Process global options  */
    command = argv[0];
    arg = argv[narg = 1];
    while (narg < argc) {
	if (arg[0] == '-')
	    set_global_option(arg);
	arg = argv[++narg];
    }


    /* Initialize variables not depending of positional options */
    landscape = wide_pages = -1;	/* To force format switching */
    fontsize = -1.0;			/* To force fontsize switching */
    page_height = (HEIGHT - MARGIN) * PIXELS_INCH;
    page_width = (WIDTH - MARGIN) * PIXELS_INCH;
    
    /* Postcript prologue printing */
    print_prologue();
    
    /* Print files designated or standard input */
    arg = argv[narg = 1];
    while (narg < argc) {
	if (arg[0] != (char)NULL) {
	    if (arg[0] == '-')
		set_positional_option(arg);
	    else {
		if (freopen(arg, "r", stdin) == NULL) {
		    fprintf(stderr, "Error opening %s\n", arg);
		    cleanup();
		    printf("\n%%%%Trailer\ndocsave restore end\n\4");
		    exit(1);
		}
		no_files = FALSE;

		/* Save counters values */
		old_pages = pages;
		if (twinfiles && twinpage)
		    old_sheets = sheets;
		else
		    old_sheets = sheets + sheetside;

		/* Print the file */
		print_file(arg, no_header, header_text);

		/* Print the number of pages and sheets printed */
		number = pages - old_pages;
		fprintf(stderr, "[%s: %d page%s on ", arg,
			number, number == 1 ? "" : "s");
		number = sheets - old_sheets + sheetside;
		fprintf(stderr, "%d sheet%s]\n", number, number == 1 ? "" : "s");

		/* Reinitialize header title */
		header_text = NULL;
	    }
	}
	arg = argv[++narg];
    }
    if (no_files)
	print_file("stdin", no_header, header_text);

    /* Print the total number of lines printed */
    if (pages != old_pages) {
	fprintf(stderr, "[Total: %d page%s on ", pages, pages == 1 ? "" : "s");
	number = sheets + sheetside;
	fprintf(stderr, "%d sheet%s]\n", number, number == 1 ? "" : "s");
    }

    /* And stop */
    cleanup();
    printf("\n%%%%Trailer\ndocsave restore end\n");
    exit(0);
}
