/* $Id: main.c,v 1.14 2005/08/14 00:21:40 tom Exp $ */

#include <signal.h>
#include <unistd.h>		/* for _exit() */

#include "defs.h"

char dflag;
char gflag;
char lflag;
char rflag;
char tflag;
char vflag;

char *symbol_prefix;
static char *file_prefix = "y";
char *myname = "yacc";

int lineno;
int outline;

char *code_file_name;
char *defines_file_name;
char *input_file_name = "";
char *output_file_name = 0;
char *verbose_file_name;
char *graph_file_name;

FILE *action_file;	/*  a temp file, used to save actions associated    */
			/*  with rules until the parser is written          */
FILE *code_file;	/*  y.code.c (used when the -r option is specified) */
FILE *defines_file;	/*  y.tab.h                                         */
FILE *input_file;	/*  the input file                                  */
FILE *output_file;	/*  y.tab.c                                         */
FILE *text_file;	/*  a temp file, used to save text until all        */
			/*  symbols have been defined                       */
FILE *union_file;	/*  a temp file, used to save the union             */
			/*  definition until all symbol have been           */
			/*  defined                                         */
FILE *verbose_file;	/*  y.output                                        */
FILE *graph_file;	/*  y.dot                                           */

int nitems;
int nrules;
int nsyms;
int ntokens;
int nvars;

int start_symbol;
char **symbol_name;
char **symbol_pname;
short *symbol_value;
short *symbol_prec;
char *symbol_assoc;

short *ritem;
short *rlhs;
short *rrhs;
short *rprec;
char *rassoc;
short **derives;
char *nullable;

/*
 * Since fclose() is called via the signal handler, it might die.  Don't loop
 * if there is a problem closing a file.
 */
#define DO_CLOSE(fp) \
	if (fp != 0) { \
	    FILE *use = fp; \
	    fp = 0; \
	    fclose(use); \
	}

static int got_intr = 0;

void done(int k)
{
    DO_CLOSE(action_file);
    DO_CLOSE(text_file);
    DO_CLOSE(union_file);
    if (got_intr)
	_exit(EXIT_FAILURE);
    exit(k);
}

static void onintr(int sig GCC_UNUSED)
{
    got_intr = 1;
    done(EXIT_FAILURE);
}

static void set_signals(void)
{
#ifdef SIGINT
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, onintr);
#endif
#ifdef SIGTERM
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, onintr);
#endif
#ifdef SIGHUP
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, onintr);
#endif
}

static void usage(void)
{
    static const char *msg[] =
    {
	""
	,"Options:"
	,"  -b file_prefix        set filename prefix (default \"y.\")"
	,"  -d                    write definitions (y.tab.h)"
	,"  -g                    write a graphical description"
	,"  -l                    suppress #line directives"
	,"  -o output_file        (default \"y.tab.c\")"
	,"  -p symbol_prefix      set symbol prefix (default \"yy\")"
	,"  -r                    produce separate code and table files (y.code.c)"
	,"  -t                    add debugging support"
	,"  -v                    write description (y.output)"
	,"  -V                    show version information and exit"
    };
    unsigned n;

    fflush(stdout);
    fprintf(stderr, "Usage: %s [options] filename\n", myname);
    for (n = 0; n < sizeof(msg) / sizeof(msg[0]); ++n)
	fprintf(stderr, "%s\n", msg[n]);

    exit(1);
}

static void setflag(int ch)
{
    switch (ch)
    {
    case 'd':
	dflag = 1;
	break;

    case 'g':
	gflag = 1;
	break;

    case 'l':
	lflag = 1;
	break;

    case 'r':
	rflag = 1;
	break;

    case 't':
	tflag = 1;
	break;

    case 'v':
	vflag = 1;
	break;

    case 'V':
	printf("%s - %s\n", myname, VERSION);
	exit(EXIT_SUCCESS);

    default:
	usage();
    }
}

static void getargs(int argc, char *argv[])
{
    register int i;
    register char *s;
    int ch;

    if (argc > 0)
	myname = argv[0];

    for (i = 1; i < argc; ++i)
    {
	s = argv[i];
	if (*s != '-')
	    break;
	switch (ch = *++s)
	{
	case '\0':
	    input_file = stdin;
	    if (i + 1 < argc)
		usage();
	    return;

	case '-':
	    ++i;
	    goto no_more_options;

	case 'b':
	    if (*++s)
		file_prefix = s;
	    else if (++i < argc)
		file_prefix = argv[i];
	    else
		usage();
	    continue;

	case 'o':
	    if (*++s)
		output_file_name = s;
	    else if (++i < argc)
		output_file_name = argv[i];
	    else
		usage();
	    continue;

	case 'p':
	    if (*++s)
		symbol_prefix = s;
	    else if (++i < argc)
		symbol_prefix = argv[i];
	    else
		usage();
	    continue;

	default:
	    setflag(ch);
	    break;
	}

	for (;;)
	{
	    switch (ch = *++s)
	    {
	    case '\0':
		goto end_of_option;

	    default:
		setflag(ch);
		break;
	    }
	}
      end_of_option:;
    }

  no_more_options:;
    if (i + 1 != argc)
	usage();
    input_file_name = argv[i];
}

char *allocate(unsigned n)
{
    register char *p;

    p = NULL;
    if (n)
    {
	p = CALLOC(1, n);
	if (!p)
	    no_space();
    }
    return (p);
}

#define CREATE_FILE_NAME(dest, suffix) \
	dest = MALLOC(len + strlen(suffix) + 1); \
	if (dest == 0) \
	    no_space(); \
	strcpy(dest, file_prefix); \
	strcpy(dest + len, suffix)

static void create_file_names(void)
{
    register int len;
    register char *defines_suffix;
    register char *prefix;

    prefix = NULL;
    defines_suffix = DEFINES_SUFFIX;

    /* compute the file_prefix from the user provided output_file_name */
    if (output_file_name != 0)
    {
	if (!(prefix = strstr(output_file_name, ".tab.c"))
	    && (prefix = strstr(output_file_name, ".c")))
	    defines_suffix = ".h";
    }

    if (prefix != NULL)
    {
	len = prefix - output_file_name;
	file_prefix = (char *)MALLOC(len);
	if (file_prefix == 0)
	    no_space();
	strncpy(file_prefix, output_file_name, len);
    }
    else
	len = strlen(file_prefix);

    /* if "-o filename" was not given */
    if (output_file_name == 0)
    {
	CREATE_FILE_NAME(output_file_name, OUTPUT_SUFFIX);
    }

    if (rflag)
    {
	CREATE_FILE_NAME(code_file_name, CODE_SUFFIX);
    }
    else
	code_file_name = output_file_name;

    if (dflag)
    {
	CREATE_FILE_NAME(defines_file_name, defines_suffix);
    }

    if (vflag)
    {
	CREATE_FILE_NAME(verbose_file_name, VERBOSE_SUFFIX);
    }

    if (gflag)
    {
	CREATE_FILE_NAME(graph_file_name, GRAPH_SUFFIX);
    }

    if (prefix != NULL)
    {
	FREE(file_prefix);
    }
}

static void open_files(void)
{
    create_file_names();

    if (input_file == 0)
    {
	input_file = fopen(input_file_name, "r");
	if (input_file == 0)
	    open_error(input_file_name);
    }

    action_file = tmpfile();
    if (action_file == 0)
	open_error("action_file");

    text_file = tmpfile();
    if (text_file == 0)
	open_error("text_file");

    if (vflag)
    {
	verbose_file = fopen(verbose_file_name, "w");
	if (verbose_file == 0)
	    open_error(verbose_file_name);
    }

    if (gflag)
    {
	graph_file = fopen(graph_file_name, "w");
	if (graph_file == 0)
	    open_error(graph_file_name);
	fprintf(graph_file, "digraph %s {\n", file_prefix);
	fprintf(graph_file, "\tedge [fontsize=10];\n");
	fprintf(graph_file, "\tnode [shape=box,fontsize=10];\n");
	fprintf(graph_file, "\torientation=landscape;\n");
	fprintf(graph_file, "\trankdir=LR;\n");
	fprintf(graph_file, "\t/*\n");
	fprintf(graph_file, "\tmargin=0.2;\n");
	fprintf(graph_file, "\tpage=\"8.27,11.69\"; // for A4 printing\n");
	fprintf(graph_file, "\tratio=auto;\n");
	fprintf(graph_file, "\t*/\n");
    }

    if (dflag)
    {
	defines_file = fopen(defines_file_name, "w");
	if (defines_file == 0)
	    open_error(defines_file_name);
	union_file = tmpfile();
	if (union_file == 0)
	    open_error("union_file");
    }

    output_file = fopen(output_file_name, "w");
    if (output_file == 0)
	open_error(output_file_name);

    if (rflag)
    {
	code_file = fopen(code_file_name, "w");
	if (code_file == 0)
	    open_error(code_file_name);
    }
    else
	code_file = output_file;
}

int main(int argc, char *argv[])
{
    set_signals();
    getargs(argc, argv);
    open_files();
    reader();
    lr0();
    lalr();
    make_parser();
    graph();
    finalize_closure();
    verbose();
    output();
    done(0);
    /*NOTREACHED */
}
	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            