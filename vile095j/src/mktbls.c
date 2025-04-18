/* This standalone utility program constructs the function, key and command
 *	binding tables for vile.  The input is a data file containing the
 *	desired default relationships among the three entities.  Output
 *	is nebind.h, neproto.h, nefunc.h, and nename.h, all of which are then
 *	included in main.c
 *
 *	Copyright (c) 1990 by Paul Fox
 *	Copyright (c) 1995-2004 by Paul Fox and Thomas Dickey
 *
 *	See the file "cmdtbl" for input data formats, and "estruct.h" for
 *	the output structures.
 *
 * Heavily modified/enhanced to also generate the table of mode and variable
 * names and their #define "bindings", based on input from the file modetbl,
 * by Tom Dickey, 1993.    -pgf
 *
 *
 * $Header: /usr/build/vile/vile/RCS/mktbls.c,v 1.135 2006/05/21 20:53:14 tom Exp $
 *
 */

/*
 * gcc (at least through 4.04) allows offsets, but g++ does not.
 *
 * Visual C++ warns about cases for 64-bit compiles that do not work with
 * the default USE_OFFSETS=1, and that works with modern C compilers, but
 * that does not work with some legacy C compilers, e.g., Solaris.  Since
 * the Unix compilers mostly follow LP64 model, it is not (yet) necessary
 * to make a configure script check to distinguish between the two - just
 * support that USE_OFFSET=2 for Visual C++.
 */
#if defined(__GNUG__) || (defined(__OS2__) && defined(__IBMC__) && (__IBMC__ >= 200))
#define USE_OFFSETS 0
#elif defined(_MSC_VER)
#define USE_OFFSETS 2
#else
#define USE_OFFSETS 1
#endif

/* stuff borrowed/adapted from estruct.h */

#ifdef HAVE_CONFIG_H
#include <config.h>
#undef DOALLOC			/* since we're not linking with trace.c */
#else /* !defined(HAVE_CONFIG_H) */

/* Note: VAX-C doesn't recognize continuation-line in ifdef lines */
# ifdef vms
#  define HAVE_STDLIB_H 1
# endif

	/* pc-stuff */
# if defined(__TURBOC__) || defined(__WATCOMC__) || defined(__GO32__) || defined(__IBMC__) || defined(_WIN32)
#  define HAVE_STDLIB_H 1
# endif

#endif /* !defined(HAVE_CONFIG_H) */

#ifndef SYS_VMS
#define SYS_VMS 0
#endif

#ifndef HAVE_STDLIB_H
# define HAVE_STDLIB_H 0
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#else
# if !defined(HAVE_CONFIG_H) || defined(MISSING_EXTERN_MALLOC)
extern char *malloc(unsigned int len);
# endif
# if !defined(HAVE_CONFIG_H) || defined(MISSING_EXTERN_FREE)
extern void free(char *ptr);
# endif
#endif

/*----------------------------------------------------------------------------*/
#ifndef DOALLOC
#define DOALLOC 0
#endif

#if DOALLOC
#include "trace.h"
#endif

#ifndef NO_LEAKS
#define NO_LEAKS 0
#endif

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

/* argument for 'exit()' or '_exit()' */
#if	SYS_VMS
#include	<stsdef.h>
#define GOODEXIT	(STS$M_INHIB_MSG | STS$K_SUCCESS)
#define BADEXIT		(STS$M_INHIB_MSG | STS$K_ERROR)
#else
#define GOODEXIT	0
#define BADEXIT		1
#endif

#define	TABLESIZE(v)	(sizeof(v)/sizeof(v[0]))

#ifndef OPT_EXEC_MACROS
#define OPT_EXEC_MACROS 40
#endif

/*--------------------------------------------------------------------------*/

#define MAX_BIND        4	/* maximum # of key-binding types */
#define	MAX_PARSE	5	/* maximum # of tokens on line */
#define	LEN_BUFFER	60	/* nominal buffer-length */
#define	MAX_BUFFER	(LEN_BUFFER*10)
#define	LEN_CHRSET	256	/* total # of chars in set (ascii) */

	/* FIXME: why not use <ctype.h> ? */
#define	DIFCNTRL	0x40
#define tocntrl(c)	((c)^DIFCNTRL)
#define toalpha(c)	((c)^DIFCNTRL)
#define	DIFCASE		0x20
#define	isUpper(c)	((c) >= 'A' && (c) <= 'Z')
#define	isLower(c)	((c) >= 'a' && (c) <= 'z')
#define toUpper(c)	((c)^DIFCASE)
#define toLower(c)	((c)^DIFCASE)
#define isboolean(c)	((c) == 'b' || (c) == 'M')

#ifndef	TRUE
#define	TRUE	(1)
#define	FALSE	(0)
#endif

#define EOS     '\0'

#define	L_CURL	'{'
#define	R_CURL	'}'

#define	Fprintf	(void)fprintf
#define	Sprintf	(void)sprintf

#ifdef MISSING_EXTERN_FPRINTF
extern int fprintf(FILE *fp, const char *fmt,...);
#endif

#define	SaveEndif(head)	InsertOnEnd(&head, "#endif")

#define	FreeIfNeeded(p) if (p != 0) { free(p); p = 0; }

/*--------------------------------------------------------------------------*/

typedef struct stringl {
    char *Name;			/* stores primary-data */
    char *Func;			/* stores secondary-data */
    char *Data;			/* associated data, if any */
    char *Cond;			/* stores ifdef-flags */
    char *Note;			/* stores comment, if any */
    struct stringl *nst;
} LIST;

static char *Blank = "";

static LIST *all_names;
static LIST *all_kbind;		/* data for kbindtbl[] */
static LIST *all_w32bind;	/* w32 data for kbindtbl[] */
static LIST *all_funcs;		/* data for extern-lines in neproto.h */
static LIST *all__FUNCs;	/* data for {}-lines in nefunc.h */
static LIST *all__CMDFs;	/* data for extern-lines in nefunc.h */
static LIST *all_statevars;
static LIST *all_ufuncs;
static LIST *all_fsms;		/* FSM tables */
static LIST *all_majors;	/* list of predefined major modes */
static LIST *all_modes;		/* data for name-completion of modes */
static LIST *all_submodes;	/* data for name-completion of submodes */
static LIST *all_gmodes;	/* data for GLOBAL modes */
static LIST *all_mmodes;	/* data for MAJOR modes */
static LIST *all_qmodes;	/* data for QUALIFIER modes */
static LIST *all_bmodes;	/* data for BUFFER modes */
static LIST *all_wmodes;	/* data for WINDOW modes */

static void save_all_modes(const char *type, char *normal, const char
			   *abbrev, char *cond);
static void save_all_submodes(const char *type, char *normal, const char
			      *abbrev, char *cond);

	/* definitions for sections of cmdtbl */
typedef enum {
    SECT_CMDS = 0
    ,SECT_FUNC
    ,SECT_VARS
    ,SECT_GBLS
    ,SECT_MAJR
    ,SECT_QUAL
    ,SECT_BUFF
    ,SECT_WIND
    ,SECT_FSMS
} SECTIONS;

	/* definitions for indices to 'asciitbl[]' vs 'kbindtbl[]' */
#define ASCIIBIND 0
#define CTLXBIND 1
#define CTLABIND 2
#define SPECBIND 3

static char *bindings[LEN_CHRSET];
static char *conditions[LEN_CHRSET];

static const char *tblname[MAX_BIND] =
{
    "asciitbl",
    "ctlxtbl",
    "metatbl",
    "spectbl"
};

static const char *prefname[MAX_BIND] =
{
    "",
    "CTLX|",
    "CTLA|",
    "SPEC|"
};

static char *fsm_name;
static char *inputfile;
static int l = 0;
static FILE *cmdtbl;
static FILE *nebind;
static FILE *neexec;
static FILE *neprot;
static FILE *nefunc;
static FILE *nename;
static FILE *nevars;
static FILE *nemode;
static FILE *nefkeys;
static FILE *nefsms;
static jmp_buf my_top;

/******************************************************************************/
static int
isSpace(int c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

static int
isPrint(int c)
{
    return c >= ' ' && c < 0x7f;
}

/******************************************************************************/
static void
badfmt(const char *s)
{
    Fprintf(stderr, "\"%s\", line %d: bad format:", inputfile, l);
    Fprintf(stderr, "\t%s\n", s);
    longjmp(my_top, 1);
}

static void
badfmt2(const char *s, int col)
{
    char temp[MAX_BUFFER];
    Sprintf(temp, "%s (column %d)", s, col);
    badfmt(temp);
}

/******************************************************************************/
static char *
Alloc(unsigned len)
{
    char *pointer = (char *) malloc(len);
    if (pointer == 0)
	badfmt("bug: not enough memory");
    return pointer;
}

static char *
StrAlloc(const char *s)
{
    return strcpy(Alloc((unsigned) strlen(s) + 1), s);
}

static LIST *
ListAlloc(void)
{
    return (LIST *) Alloc(sizeof(LIST));
}

static void
free_LIST(LIST ** p)
{
    LIST *q;

    while ((q = *p) != 0) {
	*p = q->nst;
	if (q->Name != Blank)
	    FreeIfNeeded(q->Name);
	if (q->Func != Blank)
	    FreeIfNeeded(q->Func);
	if (q->Data != Blank)
	    FreeIfNeeded(q->Data);
	if (q->Cond != Blank)
	    FreeIfNeeded(q->Cond);
	if (q->Note != Blank)
	    FreeIfNeeded(q->Note);
	free((char *) q);
    }
}

/******************************************************************************/
static void
WriteLines(FILE *fp, const char *const *list, int count)
{
    while (count-- > 0)
	Fprintf(fp, "%s\n", *list++);
}
#define	write_lines(fp,list) WriteLines(fp, list, (int)TABLESIZE(list))

/******************************************************************************/
static FILE *
OpenHeader(const char *name, char **argv)
{
    FILE *fp;
    static const char *progcreat =
    "/* %s: this header file was produced automatically by\n\
 * the %s program, based on input from the file %s\n */\n";

    if ((fp = fopen(name, "w")) == 0) {
	Fprintf(stderr, "mktbls: couldn't open header file %s\n", name);
	longjmp(my_top, 1);
    }
    Fprintf(fp, progcreat, name, argv[0], argv[1]);
    return fp;
}

/******************************************************************************/
static void
InsertSorted(
		LIST ** headp,
		const char *name,
		const char *func,
		const char *data,
		const char *cond,
		const char *note)
{
    LIST *n, *p, *q;
    int r;

    n = ListAlloc();
    n->Name = StrAlloc(name);
    n->Func = StrAlloc(func);
    n->Data = StrAlloc(data);
    n->Cond = StrAlloc(cond);
    n->Note = StrAlloc(note);

    for (p = *headp, q = 0; p != 0; q = p, p = p->nst) {
	if ((r = strcmp(n->Name, p->Name)) < 0)
	    break;
	else if (r == 0 && !strcmp(n->Cond, p->Cond)) {
	    printf("name=%s\n", n->Name);	/* FIXME */
	    badfmt("duplicate name");
	}
    }
    n->nst = p;
    if (q == 0)
	*headp = n;
    else
	q->nst = n;
}

static void
InsertOnEnd(LIST ** headp, const char *name)
{
    LIST *n, *p, *q;

    n = ListAlloc();
    n->Name = StrAlloc(name);
    n->Func = Blank;
    n->Data = Blank;
    n->Cond = Blank;
    n->Note = Blank;

    for (p = *headp, q = 0; p != 0; q = p, p = p->nst) ;

    n->nst = 0;
    if (q == 0)
	*headp = n;
    else
	q->nst = n;
}

/******************************************************************************/
static char *
append(char *dst, const char *src)
{
    (void) strcat(dst, src);
    return (dst + strlen(dst));
}

static char *
formcond(const char *c1, const char *c2)
{
    static char cond[MAX_BUFFER];

    if (c1[0] && c2[0])
	Sprintf(cond, "(%s) && (%s)", c1, c2);
    else if (c1[0] || c2[0])
	Sprintf(cond, "(%s%s)", c1, c2);
    else
	cond[0] = EOS;
    return cond;
}

static int
LastCol(char *buffer)
{
    int col = 0, c;

    while ((c = *buffer++) != 0) {
	if (isPrint(c))
	    col++;
	else if (c == '\t')
	    col = (col | 7) + 1;
    }
    return col;
}

static char *
PadTo(int col, char *buffer)
{
    int any = 0;
    int len = (int) strlen(buffer);
    int now;
    char with;

    for (;;) {
	if ((now = LastCol(buffer)) >= col) {
	    if (any)
		break;
	    else
		with = ' ';
	} else if (col - now > 2)
	    with = '\t';
	else
	    with = ' ';

	buffer[len++] = with;
	buffer[len] = EOS;
	any++;
    }
    return buffer;
}

static int
two_conds(int c, char *cond)
{
    /* return true if both bindings have different
       conditions associated with them */
    return (cond[0] != '\0' &&
	    conditions[c] != NULL &&
	    strcmp(cond, conditions[c]) != '\0');
}

static void
set_binding(int btype, int c, char *cond, char *func)
{
    char name[MAX_BUFFER];

    if (btype != ASCIIBIND) {
	if (c < ' ') {
	    Sprintf(name, "%stocntrl('%c')",
		    prefname[btype],
		    toalpha(c));
	} else if (c >= 0x80) {
	    Sprintf(name, "%s0x%x",
		    prefname[btype], c);
	} else {
	    Sprintf(name, "%s'%s%c'",
		    prefname[btype],
		    (c == '\'' || c == '\\') ? "\\" : "",
		    c);
	}
	InsertSorted(&all_kbind, name, func, "", cond, "");
    } else {
	if (bindings[c] != NULL) {
	    if (!two_conds(c, cond))
		badfmt("duplicate key binding");
	    free(bindings[c]);
	}
	bindings[c] = StrAlloc(func);
	if (cond[0]) {
	    FreeIfNeeded(conditions[c]);
	    conditions[c] = StrAlloc(cond);
	} else {
	    conditions[c] = NULL;
	}
    }
}

/******************************************************************************/
	/* returns the number of non-comment tokens parsed, with a list of
	 * tokens (0=comment) as a side-effect.  Note that quotes are removed
	 * from the token, so we have to have them only in the first token! */
static int
Parse(char *input, char **vec)
{
    int expecting = TRUE, count = 0, quote = 0, n, c;

    for (c = 0; c < MAX_PARSE; c++)
	vec[c] = "";
    for (c = (int) strlen(input); c > 0 && isSpace(input[c - 1]); c--)
	input[c - 1] = EOS;

    for (n = 0; (c = input[n++]) != EOS;) {
	if (quote) {
	    if (c == quote) {
		quote = 0;
		if (input[n] && !isSpace(input[n]))
		    badfmt2("expected blank", n);
		input[n - 1] = EOS;
	    }
	} else {
	    if ((c == '"') || (c == '\'')) {
		quote = c;
	    } else if (c == '<') {
		c = quote = '>';
	    } else if (isSpace(c)) {
		input[n - 1] = EOS;
		expecting = TRUE;
	    } else if (c == '#') {
		while (isSpace(input[n]))
		    n++;
		vec[0] = input + n;
		break;
	    }
	    if (expecting && !isSpace(c)) {
		if (count + 1 >= MAX_PARSE)
		    break;
		vec[++count] = input + n - ((c != quote) ? 1 : 0);
		expecting = FALSE;
	    }
	}
    }
    return count;
}

/******************************************************************************/
static const char *lastIfdef;

static void
BeginIf(void)
{
    lastIfdef = 0;
}

static void
FlushIf(FILE *fp)
{
    if (lastIfdef != 0) {
	Fprintf(fp, "#endif\n");
	lastIfdef = 0;
    }
}

static void
WriteIf(FILE *fp, const char *cond)
{
    if (cond == 0)
	cond = "";
    if (cond[0] != EOS) {
	if (lastIfdef != 0) {
	    if (!strcmp(lastIfdef, cond))
		return;
	    FlushIf(fp);
	}
	Fprintf(fp, "#if %s\n", lastIfdef = cond);
    } else
	FlushIf(fp);
}

/******************************************************************************/
/* get abbreviation by taking the uppercase chars only */
static char *
AbbrevMode(char *src)
{
    char *dst = StrAlloc(src);
    char *s = src, *d = dst;
    while (*s) {
	if (isUpper(*s))
	    *d++ = (char) toLower(*s);
	s++;
    }
    *d = EOS;
    return dst;
}

/* get name, converted to lowercase */
static char *
NormalMode(char *src)
{
    char *dst = StrAlloc(src);
    char *s = dst;

    while (*s) {
	if (isUpper(*s))
	    *s = (char) toLower(*s);
	s++;
    }
    return dst;
}

/* given single-char type-key (cf: Mode2Key), return define-string */
static const char *
c2TYPE(int c)
{
    const char *value;

    switch (c) {
    case 'b':
	value = "BOOL";
	break;
    case 'e':
	value = "ENUM";
	break;
    case 'i':
	value = "INT";
	break;
    case 's':
	value = "STRING";
	break;
    case 'x':
	value = "REGEX";
	break;
    default:
	value = "?";
    }
    return value;
}

static int
is_majormode(const char *name)
{
    LIST *p;

    for (p = all_mmodes; p != 0; p = p->nst) {
	if (!strcmp(name, p->Name))
	    return TRUE;
    }
    return FALSE;
}

/* check that the mode-name won't be illegal */
static void
CheckModes(char *name)
{
    if (!strncmp(name, "no", 2))
	badfmt("illegal mode-name");
}

/* make a sort-key for mode-name */
static char *
Mode2Key(char *type, char *name, char *cond, int submode)
{
    int c;
    char *abbrev = AbbrevMode(name), *normal = NormalMode(name), *tmp =
    Alloc((unsigned) (4 + strlen(normal) + strlen(abbrev)));

    CheckModes(normal);
    CheckModes(abbrev);

    switch (c = *type) {
    case 'b':
    case 'e':
    case 'i':
    case 's':
	break;
    case 'r':
	c = 'x';		/* make this sort after strings */
    }

    save_all_modes(type, normal, abbrev, cond);
    if (submode)
	save_all_submodes(type, normal, abbrev, cond);

    Sprintf(tmp, "%s\n%c\n%s", normal, c, abbrev);
#if NO_LEAKS
    free(normal);
    free(abbrev);
#endif
    return tmp;
}

/* converts a mode-name to a legal (hopefully unique!) symbol */
static char *
Name2Symbol(char *name)
{
    char *base, *dst;
    char c;

    /* allocate enough for adjustment in 'Name2Address()' */
    /*   "+ 10" for comfort */
    base = dst = Alloc((unsigned) (strlen(name) + 10));

    *dst++ = 's';
    *dst++ = '_';
    while ((c = *name++) != EOS) {
	if (c == '-')
	    c = '_';
	*dst++ = c;
    }
    *dst++ = '_';
    *dst++ = '_';
    *dst = EOS;
    return base;
}

/* converts a mode-name & type to a reference to string-value */
static char *
Name2Address(char *name, char *type)
{
    /*  "+ 10" for comfort */
    unsigned len = (unsigned) strlen(name) + 10;
    char *base = Alloc(len);
    char *temp;

    temp = Name2Symbol(name);
    if (strlen(temp) + 1 + (isboolean(*type) ? 4 : 0) > len)
	badfmt("bug: buffer overflow in Name2Address");

    (void) strcpy(base, temp);
    if (isboolean(*type))
	(void) strcat(strcat(strcpy(base + 2, "no"), temp + 2), "+2");
    free(temp);
    return base;
}

/* define Member_Offset macro, used in index-definitions */
static void
DefineOffset(FILE *fp)
{
    Fprintf(fp, "#ifndef\tMember_Offset\n");
#if USE_OFFSETS == 2
    Fprintf(fp, "\
#define\tMember_Offset(T,M) ((int)(((long)(&(((T*)0)->M) - (char *)0))/\\\n\
\t\t\t\t ((long)(&(((T*)0)->Q1) - &(((T*)0)->s_MAX)))))\n");
#elif USE_OFFSETS == 1
    Fprintf(fp, "\
#define\tMember_Offset(T,M) ((int)(((long)&(((T*)0)->M))/\\\n\
\t\t\t\t ((long)&(((T*)0)->Q1) - (long)&(((T*)0)->s_MAX))))\n");
#else
    Fprintf(fp, "\
#define\tMember_Offset(T,M) ((vile_ ## M)-1)\n");
#endif
    Fprintf(fp, "#endif\n");
}

/* generate the index-struct (used for deriving ifdef-able index definitions) */
static void
WriteIndexStruct(FILE *fp, LIST * p, const char *ppref)
{
#if USE_OFFSETS
    char *s, temp[MAX_BUFFER], line[MAX_BUFFER], *vec[MAX_PARSE];

    BeginIf();
    Fprintf(fp, "typedef\tstruct\t%c\n", L_CURL);
    while (p != 0) {
	WriteIf(fp, p->Cond);
	(void) Parse(strcpy(line, p->Name), vec);
	Sprintf(temp, "\tchar\t%s;", s = Name2Symbol(vec[1]));
	free(s);
	if (p->Note[0]) {
	    (void) PadTo(32, temp);
	    Sprintf(temp + strlen(temp), "/* %s */", p->Note);
	}
	Fprintf(fp, "%s\n", temp);
	p = p->nst;
    }

    FlushIf(fp);
    Fprintf(fp, "\tchar\ts_MAX;\n");
    Fprintf(fp, "\tchar\tQ1;\n");
    Fprintf(fp, "\t%c Index%s;\n\n", R_CURL, ppref);
#else
    char *s, temp[MAX_BUFFER], line[MAX_BUFFER], *vec[MAX_PARSE];
    int count = 0;

    BeginIf();
    Fprintf(fp, "typedef\tenum\t%c\n", L_CURL);
    Fprintf(fp, "\tMIN_%c_VALUES = 0\n", *ppref);
    for (; p != 0; p = p->nst, count++) {
	WriteIf(fp, p->Cond);
	(void) Parse(strcpy(line, p->Name), vec);
	Sprintf(temp, "\t,vile_%s", s = Name2Symbol(vec[1]));
	free(s);
	if (p->Note[0]) {
	    (void) PadTo(32, temp);
	    Sprintf(temp + strlen(temp), "/* %s */", p->Note);
	}
	Fprintf(fp, "%s\n", temp);
    }
    FlushIf(fp);

    Sprintf(temp, "\t,NUM_%c_VALUES", *ppref);
    (void) PadTo(32, temp);
    Sprintf(temp + strlen(temp), "/* TABLESIZE(%c_valnames) -- %s */\n",
	    toLower(*ppref), ppref);
    Fprintf(nemode, "%s", temp);

    Fprintf(fp, "\t%c Index%s;\n\n", R_CURL, ppref);
#endif
}

/* generate the index-definitions */
static void
WriteModeDefines(LIST * p, const char *ppref)
{
    char temp[MAX_BUFFER], line[MAX_BUFFER], *vec[MAX_PARSE];
    int count = 0;
    char *s;
    BeginIf();

    for (; p != 0; p = p->nst, count++) {
	(void) Parse(strcpy(line, p->Name), vec);
	Sprintf(temp, "#define %.1s%s%s",
		(*ppref == 'B') ? "" : ppref,
		(*vec[2] == 'b') ? "MD" : "VAL_",
		p->Func);
	(void) PadTo(24, temp);
	WriteIf(nemode, p->Cond);
	Sprintf(temp + strlen(temp), "Member_Offset(Index%s, %s)",
		ppref, s = Name2Symbol(vec[1]));
	free(s);
	Fprintf(nemode, "%s\n", temp);
    }

    Fprintf(nemode, "\n");
    FlushIf(nemode);

#if USE_OFFSETS
    Sprintf(temp, "#define NUM_%c_VALUES\tMember_Offset(Index%s, s_MAX)",
	    *ppref, ppref);
    (void) PadTo(32, temp);
    Sprintf(temp + strlen(temp), "/* TABLESIZE(%c_valnames) -- %s */\n",
	    toLower(*ppref), ppref);
    Fprintf(nemode, "%s", temp);
#endif

    Fprintf(nemode, "#define MAX_%c_VALUES\t%d\n\n", *ppref, count);
}

static void
WriteModeSymbols(LIST * p)
{
    char temp[MAX_BUFFER], line[MAX_BUFFER];
    char *vec[MAX_PARSE], *s;

    /* generate the symbol-table */
    BeginIf();
    while (p != 0) {
	WriteIf(nemode, p->Cond);
	(void) Parse(strcpy(line, p->Name), vec);
	Sprintf(temp, "\t%c %s,",
		L_CURL, s = Name2Address(vec[1], vec[2]));
	(void) PadTo(32, temp);
	free(s);
	s = 0;

	Sprintf(temp + strlen(temp), "%s,",
		*vec[3] ? (s = Name2Address(vec[3], vec[2])) : "\"X\"");
	(void) PadTo(48, temp);
	if (s != 0)
	    free(s);

	Sprintf(temp + strlen(temp), "VALTYPE_%s,", c2TYPE(*vec[2]));
	(void) PadTo(64, temp);
	if (!strcmp(p->Data, "0"))
	    (void) strcat(temp, "(ChgdFunc)0 },");
	else
	    Sprintf(temp + strlen(temp), "%s },", p->Data);
	Fprintf(nemode, "%s\n", temp);
	p = p->nst;
    }
    FlushIf(nemode);

}

/******************************************************************************/
static void
save_all_modes(
		  const char *type,
		  char *normal,
		  const char *abbrev,
		  char *cond)
{
    if (isboolean(*type)) {
	char t_normal[LEN_BUFFER], t_abbrev[LEN_BUFFER];
	save_all_modes("Bool",
		       strcat(strcpy(t_normal, "no"), normal),
		       *abbrev
		       ? strcat(strcpy(t_abbrev, "no"), abbrev)
		       : "",
		       cond);
    }
    InsertSorted(&all_modes, normal, type, "", cond, "");
    if (*abbrev)
	InsertSorted(&all_modes, abbrev, type, "", cond, "");
}

static void
dump_all_modes(void)
{
    static const char *const top[] =
    {
	"",
	"#ifdef realdef",
	"/*",
	" * List of strings shared between all_modes, b_valnames and w_valnames",
	" */",
	"static const char",
    };
    static const char *const middle[] =
    {
	"\ts_NULL[] = \"\";",
	"#endif /* realdef */",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(char *const all_modes[]) = {",
    };
    static const char *const bottom[] =
    {
	"\tNULL\t/* ends table */",
	"};",
	"#else",
	"extern const char *const all_modes[];",
	"#endif /* realdef */",
    };
    char temp[MAX_BUFFER], *s;
    LIST *p, *q;

    InsertSorted(&all_modes, "all", "?", "", "", "");
    write_lines(nemode, top);
    BeginIf();
    for (p = all_modes; p; p = p->nst) {
	if (!isboolean(p->Func[0])) {
	    for (q = p->nst; q != 0; q = q->nst)
		if (!isboolean(q->Func[0]))
		    break;
	    WriteIf(nemode, p->Cond);
	    Sprintf(temp, "\t%s[]", s = Name2Symbol(p->Name));
	    (void) PadTo(32, temp);
	    free(s);
	    Sprintf(temp + strlen(temp), "= \"%s\",", p->Name);
	    (void) PadTo(64, temp);
	    Fprintf(nemode, "%s/* %s */\n", temp, p->Func);
	}
    }
    FlushIf(nemode);

    write_lines(nemode, middle);
    for (p = all_modes; p; p = p->nst) {
	if (is_majormode(p->Name))
	    continue;
	WriteIf(nemode, p->Cond);
	Fprintf(nemode, "\t%s,\n", s = Name2Address(p->Name, p->Func));
	free(s);
    }
    FlushIf(nemode);

    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
save_bindings(char *s, char *func, char *cond)
{
    int btype, c, highbit;

    btype = ASCIIBIND;

    if (*s == '^' && *(s + 1) == 'A' && *(s + 2) == '-') {
	btype = CTLABIND;
	s += 3;
    } else if (*s == 'F' && *(s + 1) == 'N' && *(s + 2) == '-') {
	btype = SPECBIND;
	s += 3;
    } else if (*s == '^' && *(s + 1) == 'X' && *(s + 2) == '-') {
	btype = CTLXBIND;
	s += 3;
    }
    if (*s == 'M' && *(s + 1) == '-') {
	highbit = 0x80;
	s += 2;
    } else {
	highbit = 0;
    }

    if (*s == '\\') {		/* try for an octal value */
	c = 0;
	while (*++s < '8' && *s >= '0')
	    c = (c * 8) + *s - '0';
	if (c >= LEN_CHRSET)
	    badfmt("octal character too big");
	c |= highbit;
	set_binding(btype, c, cond, func);
    } else if (*s == '^' && (c = *(s + 1)) != EOS) {	/* a control char? */
	if (c > 'a' && c < 'z')
	    c = toUpper(c);
	c = tocntrl(c);
	c |= highbit;
	set_binding(btype, c, cond, func);
	s += 2;
    } else if ((c = *s) != 0) {
	c |= highbit;
	set_binding(btype, c, cond, func);
	s++;
    } else {
	badfmt("getting binding");
    }

    if (*s != EOS)
	badfmt("got extra characters");
}

static void
dump_bindings(void)
{
    char temp[MAX_BUFFER];
    const char *sctl, *meta;
    int i, c, btype;
    LIST *p;

    btype = ASCIIBIND;

    Fprintf(nebind, "\nconst CMDFUNC *%s[%d] = %c\n",
	    tblname[btype], LEN_CHRSET, L_CURL);

    BeginIf();
    for (i = 0; i < LEN_CHRSET; i++) {
	WriteIf(nebind, conditions[i]);

	sctl = "";
	if (i & 0x80)
	    meta = "meta-";
	else
	    meta = "";
	c = i & 0x7f;
	if (c < ' ' || c > '~') {
	    sctl = "ctrl-";
	    c = toalpha(c);
	}

	if (bindings[i])
	    Sprintf(temp, "\t&f_%s,", bindings[i]);
	else
	    Sprintf(temp, "\tNULL,");

	Fprintf(nebind, "%s/* %s%s%c */\n", PadTo(32, temp),
		meta, sctl, c);
	if (conditions[i] != 0)
	    Fprintf(nebind, "#else\n\tNULL,\n");
	FlushIf(nebind);

    }
    Fprintf(nebind, "%c;\n", R_CURL);

    Fprintf(nebind, "\nKBIND kbindtbl[] = %c\n", L_CURL);
    BeginIf();
    for (p = all_kbind; p; p = p->nst) {
	WriteIf(nebind, p->Cond);
	Sprintf(temp, "\t%c %s,", L_CURL, p->Name);
	PadTo(32, temp);
	Sprintf(temp + strlen(temp), "&f_%s", p->Func);
	PadTo(56, temp);
	Fprintf(nebind, "%sKBIND_LINK(NULL) %c,\n", temp, R_CURL);
    }
    FlushIf(nebind);
    if (all_w32bind) {
	BeginIf();
	for (p = all_w32bind; p; p = p->nst) {
	    WriteIf(nebind, p->Cond);
	    Sprintf(temp, "\t%c %s,", L_CURL, p->Name);
	    Fprintf(nebind, "%s&f_%s KBIND_LINK(NULL) %c,\n",
		    PadTo(32, temp), p->Func, R_CURL);
	}
	FlushIf(nebind);
    }

    Fprintf(nebind, "\t{ 0, NULL KBIND_LINK(NULL) }\n");
    Fprintf(nebind, "%c;\n", R_CURL);

}

/******************************************************************************/
/*
 * Construct submode names for the given predefined majormodes
 */
static void
dump_majors(void)
{
    LIST *p, *q;
    char norm[LEN_BUFFER], abbr[LEN_BUFFER], type[LEN_BUFFER];
    char normal[LEN_BUFFER], abbrev[LEN_BUFFER];
    char *my_cond = "OPT_MAJORMODE";

    for (q = all_majors; q; q = q->nst) {
	Sprintf(normal, "%smode", q->Name);	/* FIXME */
	save_all_modes("Major", normal, "", my_cond);
    }
    for (p = all_mmodes; p; p = p->nst) {
	if (sscanf(p->Name, "%s\n%s\n%s", norm, type, abbr) != 3)
	    continue;
	for (q = all_majors; q; q = q->nst) {
	    Sprintf(normal, "%s-%s", q->Name, norm);
	    Sprintf(abbrev, "%s%s", q->Name, abbr);
	    save_all_modes(c2TYPE(*type), normal, abbrev, my_cond);
	}
    }
}

/******************************************************************************/
static void
save_mmodes(char *type, char **vec)
{
    char *key = Mode2Key(type, vec[1], vec[4], TRUE);

    InsertSorted(&all_mmodes, key, vec[2], vec[3], vec[4], vec[0]);
#if NO_LEAKS
    free(key);
#endif
}

static void
dump_mmodes(void)
{
    static const char *const top[] =
    {
	"",
	"#if OPT_MAJORMODE",
	"/* major mode flags\t*/",
	"/* the indices of M_VALUES.v[] */",
    };
    static const char *const middle[] =
    {
	"",
	"typedef struct M_VALUES {",
	"\t/* each entry is a val, and a ptr to a val */",
	"\tstruct VAL mv[MAX_M_VALUES+1];",
	"} M_VALUES;",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(struct VALNAMES m_valnames[MAX_M_VALUES+1]) = {",
    };
    static const char *const bottom[] =
    {
	"",
	"\t{ NULL,\tNULL,\tVALTYPE_INT, 0 }",
	"};",
	"#else",
	"extern const struct VALNAMES m_valnames[MAX_M_VALUES+1];",
	"#endif",
	"#endif /* OPT_MAJORMODE */",
    };

    write_lines(nemode, top);
    WriteIndexStruct(nemode, all_mmodes, "Major");
    WriteModeDefines(all_mmodes, "Major");
    write_lines(nemode, middle);
    WriteModeSymbols(all_mmodes);
    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
save_all_submodes(
		     const char *type,
		     char *normal,
		     const char *abbrev,
		     char *cond)
{
    if (isboolean(*type)) {
	char t_normal[LEN_BUFFER], t_abbrev[LEN_BUFFER];
	save_all_submodes("Bool",
			  strcat(strcpy(t_normal, "no"), normal),
			  *abbrev
			  ? strcat(strcpy(t_abbrev, "no"), abbrev)
			  : "",
			  cond);
    }
    InsertSorted(&all_submodes, normal, type, "", cond, "");
    if (*abbrev)
	InsertSorted(&all_submodes, abbrev, type, "", cond, "");
}

static void
dump_all_submodes(void)
{
    static const char *const top[] =
    {
	"",
	"#if OPT_MAJORMODE",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(char *const all_submodes[]) = {",
    };
    static const char *const bottom[] =
    {
	"\tNULL\t/* ends table */",
	"};",
	"#else",
	"extern const char *const all_submodes[];",
	"#endif /* realdef */",
	"#endif /* OPT_MAJORMODE */",
    };
    char *s;
    LIST *p;

    write_lines(nemode, top);
    for (p = all_submodes; p; p = p->nst) {
	if (is_majormode(p->Name))
	    continue;
	WriteIf(nemode, p->Cond);
	Fprintf(nemode, "\t%s,\n", s = Name2Address(p->Name, p->Func));
	free(s);
    }
    FlushIf(nemode);

    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
predefine_submodes(char **vec, int len)
{
    LIST *p;
    int found;
    char norm[LEN_BUFFER], type[LEN_BUFFER], abbr[LEN_BUFFER], temp[LEN_BUFFER];

    if (len > 1) {
	for (p = all_majors, found = FALSE; p; p = p->nst) {
	    if (!strcmp(p->Name, vec[2])) {
		found = TRUE;
		break;
	    }
	}
	if (!found)
	    InsertSorted(&all_majors, vec[2], "", "", "", "");
	if (len > 2) {
	    for (p = all_bmodes, found = FALSE; p; p = p->nst) {
		if (sscanf(p->Name, "%s\n%s\n%s",
			   norm, type, abbr) == 3
		    && (!strcmp(norm, vec[3])
			|| !strcmp(abbr, vec[3]))) {
		    found = TRUE;
		    break;
		}
	    }
	    if (found) {
		Sprintf(temp, "%s-%s", vec[2], norm);
		strcpy(norm, temp);
		Sprintf(temp, "%s%s", vec[2], abbr);
		strcpy(abbr, temp);
		save_all_modes(c2TYPE(*type), norm, abbr,
			       formcond(p->Cond, "OPT_MAJORMODE"));
	    }
	}
    }
}

/******************************************************************************/
static void
save_qmodes(char *type, char **vec)
{
    char *key = Mode2Key(type, vec[1], vec[4], TRUE);
    InsertSorted(&all_qmodes, key, vec[2], vec[3], vec[4], vec[0]);
#if NO_LEAKS
    free(key);
#endif
}

static void
dump_qmodes(void)
{
    static const char *const top[] =
    {
	"",
	"/* submode qualifier flags\t*/",
	"/* the indices of Q_VALUES.v[] */",
    };
    static const char *const middle[] =
    {
	"",
	"typedef struct Q_VALUES {",
	"\t/* each entry is a val, and a ptr to a val */",
	"\tstruct VAL qv[MAX_Q_VALUES+1];",
	"} Q_VALUES;",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(struct VALNAMES q_valnames[MAX_Q_VALUES+1]) = {",
    };
    static const char *const bottom[] =
    {
	"",
	"\t{ NULL,\tNULL,\tVALTYPE_INT, 0 }",
	"};",
	"#else",
	"extern const struct VALNAMES q_valnames[MAX_Q_VALUES+1];",
	"#endif",
    };

    write_lines(nemode, top);
    WriteIndexStruct(nemode, all_qmodes, "Qualifiers");
    WriteModeDefines(all_qmodes, "Qualifiers");
    write_lines(nemode, middle);
    WriteModeSymbols(all_qmodes);
    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
save_bmodes(char *type, char **vec)
{
    char *key = Mode2Key(type, vec[1], vec[4], TRUE);
    InsertSorted(&all_bmodes, key, vec[2], vec[3], vec[4], vec[0]);
#if NO_LEAKS
    free(key);
#endif
}

static void
dump_bmodes(void)
{
    static const char *const top[] =
    {
	"",
	"/* buffer mode flags\t*/",
	"/* the indices of B_VALUES.v[] */",
    };
    static const char *const middle[] =
    {
	"",
	"typedef struct B_VALUES {",
	"\t/* each entry is a val, and a ptr to a val */",
	"\tstruct VAL bv[MAX_B_VALUES+1];",
	"} B_VALUES;",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(struct VALNAMES b_valnames[]) = {",
    };
    static const char *const bottom[] =
    {
	"",
	"\t{ NULL,\tNULL,\tVALTYPE_INT, 0 }",
	"};",
	"#else",
	"extern const struct VALNAMES b_valnames[];",
	"#endif",
    };

    write_lines(nemode, top);
    WriteIndexStruct(nemode, all_bmodes, "Buffers");
    WriteModeDefines(all_bmodes, "Buffers");
    write_lines(nemode, middle);
    WriteModeSymbols(all_bmodes);
    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
start_vars_h(char **argv)
{
    static const char *const head[] =
    {
	"",
	"#if OPT_EVAL",
	"",
	"/*\tstructure to hold temp variables and their definitions\t*/",
	"",
	"typedef struct UVAR {",
	"\tstruct UVAR *next;",
	"\tchar *u_name;\t\t/* name of temp variable */",
	"\tchar *u_value;\t\t/* value (string) */",
	"} UVAR;",
	"",
	"decl_uninit( UVAR *temp_vars );\t/* temporary variables */",
	"",
    };

    if (!nevars) {
	nevars = OpenHeader("nevars.h", argv);
	write_lines(nevars, head);
    }
}

static void
finish_vars_h(void)
{
    if (nevars)
	Fprintf(nevars, "\n#endif /* OPT_EVAL */\n");
}

/******************************************************************************/
static void
init_statevars(void)
{
    static const char *const head[] =
    {
	"",
	"/*\tlist of recognized state variables\t*/",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(char *const statevars[]) = {"
    };
    static int done;

    if (!done++)
	write_lines(nevars, head);
}

static void
save_statevars(char **vec)
{
    /* insert into 'all_modes' to provide for common name-completion
     * table, and into 'all_statevars' to get name/index correspondence.
     */
    InsertSorted(&all_modes, vec[1], "state", "", formcond("OPT_EVAL",
							   vec[3]), "");
    InsertSorted(&all_statevars, vec[1], vec[2], "", vec[3], vec[0]);
}

static void
dump_statevars(void)
{
    static const char *const middle[] =
    {
	"\tNULL\t/* ends table for name-completion */",
	"};",
	"#else",
	"extern const char *const statevars[];",
	"#endif",
	"",
	""
	"typedef enum {",
    };
    static const char *const middle1[] =
    {
	"\tNum_StateVars",
	"} enumStateVars;",
	"",
	""
    };
    static const char *const middle2[] =
    {
	"",
	"typedef int (StateFunc)(TBUFF **resultp, const char *valuep);",
	"",
	"",
	"#ifdef realdef",
	"DECL_EXTERN(StateFunc *statevar_func[]) = {",
	""
    };
    static const char *const tail[] =
    {
	"\t(StateFunc *)NULL",
	"};",
	"#else",
	"extern StateFunc *statevar_func[];",
	"#endif /* realdef */",
	"",
    };
    LIST *p;
    char *s;

    BeginIf();
    for (p = all_statevars; p != 0; p = p->nst) {
	if (p == all_statevars)
	    init_statevars();
	WriteIf(nevars, p->Cond);
	Fprintf(nevars, "\t%s,\n", s = Name2Symbol(p->Name));
	free(s);
    }
    FlushIf(nevars);
    write_lines(nevars, middle);

    for (p = all_statevars; p != 0; p = p->nst) {
	WriteIf(nevars, p->Cond);
	Fprintf(nevars, "\tVAR_%s,\n", p->Func);
    }
    FlushIf(nevars);

    write_lines(nevars, middle1);
    /* emit the variable get/set routine prototypes */
    for (p = all_statevars; p != 0; p = p->nst) {
	WriteIf(nevars, p->Cond);
	Fprintf(nevars, "int var_%s(TBUFF **resp, const char *valp);\n",
		p->Func);
    }
    FlushIf(nevars);
    write_lines(nevars, middle2);
    /* emit the variable get/set routine table */
    for (p = all_statevars; p != 0; p = p->nst) {
	WriteIf(nevars, p->Cond);
	Fprintf(nevars, "\tvar_%s,\n", p->Func);
    }
    FlushIf(nevars);
    write_lines(nevars, tail);
}

/******************************************************************************/
static void
save_fsms(char **vec)
{
    InsertSorted(&all_fsms, vec[1], "", vec[2], vec[3], vec[0]);
}

static void
init_fsms(void)
{
    char name[BUFSIZ];
    int n;

    if (fsm_name == 0)
	badfmt("Missing table name");

    (void) strcpy(name, fsm_name);
    for (n = 0; fsm_name[n] != '\0'; n++)
	fsm_name[n] = (char) toUpper(fsm_name[n]);
    Fprintf(nefsms, "\n");
    Fprintf(nefsms, "#if OPT_%s_CHOICES\n", fsm_name);
    Fprintf(nefsms, "#ifndef realdef\n");
    Fprintf(nefsms, "extern const FSM_CHOICES fsm_%s_choices[];\n", name);
    Fprintf(nefsms, "#else\n");
    Fprintf(nefsms, "DECL_EXTERN_CONST(FSM_CHOICES fsm_%s_choices[]) = %c\n",
	    name, L_CURL);
}

static void
dump_fsms(void)
{
    static const char *const middle[] =
    {
	"\tEND_CHOICES\t/* ends table for name-completion */",
	"};",
    };
    char temp[MAX_BUFFER];
    LIST *p;
    int count;

    if (all_fsms != 0) {
	BeginIf();
	for (p = all_fsms, count = 0; p != 0; p = p->nst) {
	    if (!count++)
		init_fsms();
	    WriteIf(nefsms, p->Cond);
	    Sprintf(temp, "\t{ \"%s\",", p->Name);
	    Fprintf(nefsms, "%s%s },\n", PadTo(40, temp), p->Data);
	}
	FlushIf(nefsms);

	write_lines(nefsms, middle);
	Fprintf(nefsms, "#endif\n");
	Fprintf(nefsms, "#endif /* OPT_%s_CHOICES */\n", fsm_name);

	free_LIST(&all_fsms);

	free(fsm_name);
	fsm_name = 0;
    }
}

/******************************************************************************/
static void
start_fsms_h(char **argv, char *name)
{
    static const char *const head[] =
    {
	"#ifndef NEFSMS_H",
	"#define NEFSMS_H 1",
    };

    if (!nefsms) {
	nefsms = OpenHeader("nefsms.h", argv);
	write_lines(nefsms, head);
    }
    dump_fsms();
    fsm_name = StrAlloc(name);
}

static void
finish_fsms_h(void)
{
    if (nefsms)
	Fprintf(nefsms, "\n#endif /* NEFSMS_H */\n");
}

/******************************************************************************/
static void
dump_execs(FILE *fp, int count)
{
    int n;

    for (n = 1; n <= count; n++) {
	Fprintf(fp, "#if OPT_EXEC_MACROS>%d\n", n - 1);
	Fprintf(fp, "int\n");
	Fprintf(fp, "cbuf%d(int f, int n)\n", n);
	Fprintf(fp, "{\n\treturn cbuf(f, n, %d);\n}\n", n);
	Fprintf(fp, "#endif\n");
    }
    fclose(fp);
}

/******************************************************************************/
static void
save_funcs(
	      char *func,
	      char *flags,
	      char *cond,
	      char *old_cond,
	      char *help)
{
    char temp[MAX_BUFFER];
    char *s;

    if (strcmp(cond, old_cond)) {
	if (*old_cond) {
	    SaveEndif(all_funcs);
	    SaveEndif(all__FUNCs);
	    SaveEndif(all__CMDFs);
	}
	if (*cond) {
	    Sprintf(temp, "#if %s", cond);
	    InsertOnEnd(&all_funcs, temp);
	    InsertOnEnd(&all__FUNCs, temp);
	    InsertOnEnd(&all__CMDFs, temp);
	}
	(void) strcpy(old_cond, cond);
    }
    Sprintf(temp, "extern int %s ( int f, int n );", func);
    InsertOnEnd(&all_funcs, temp);

    s = strcpy(temp, "\t");
    s = append(s, "DECL_EXTERN_CONST(CMDFUNC f_");
    s = append(s, func);
    s = append(s, ")");
    (void) PadTo(32, temp);
    s = append(s, "= {\n\tINIT_UNION(");
    s = append(s, func);
    s = append(s, "),\n\t");
    s = append(s, flags);
    s = append(s, "\n#if OPT_MACRO_ARGS\n\t\t,0\n#endif");
    s = append(s, "\n#if OPT_TRACE\n\t\t,\"");
    s = append(s, func);
    s = append(s, "\"\n#endif");
    s = append(s, "\n#if OPT_ONLINEHELP\n\t\t,\"");
    s = append(s, help);
    (void) append(s, "\"\n#endif\n };");
    InsertOnEnd(&all__FUNCs, temp);

    s = append(strcpy(temp, "extern const CMDFUNC f_"), func);
    (void) append(s, ";");
    InsertOnEnd(&all__CMDFs, temp);
}

static void
dump_funcs(FILE *fp, LIST * head)
{
    LIST *p;
    for (p = head; p != 0; p = p->nst)
	Fprintf(fp, "%s\n", p->Name);
}

/******************************************************************************/
static void
save_gmodes(char *type, char **vec)
{
    char *key = Mode2Key(type, vec[1], vec[4], FALSE);

    InsertSorted(&all_gmodes, key, vec[2], vec[3], vec[4], vec[0]);
#if NO_LEAKS
    free(key);
#endif
}

static void
dump_gmodes(void)
{
    static const char *const top[] =
    {
	"",
	"/* global mode flags\t*/",
	"/* the indices of G_VALUES.v[] */",
    };
    static const char *const middle[] =
    {
	"",
	"typedef struct G_VALUES {",
	"\t/* each entry is a val, and a ptr to a val */",
	"\tstruct VAL gv[MAX_G_VALUES+1];",
	"} G_VALUES;",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(struct VALNAMES g_valnames[]) = {",
    };
    static const char *const bottom[] =
    {
	"",
	"\t{ NULL,\tNULL,\tVALTYPE_INT, 0 }",
	"};",
	"#else",
	"extern const struct VALNAMES g_valnames[];",
	"#endif",
    };

    write_lines(nemode, top);
    WriteIndexStruct(nemode, all_gmodes, "Globals");
    WriteModeDefines(all_gmodes, "Globals");
    write_lines(nemode, middle);
    WriteModeSymbols(all_gmodes);
    write_lines(nemode, bottom);
}

/******************************************************************************/
static void
save_names(char *name, char *func, char *cond)
{
    InsertSorted(&all_names, name, func, "", cond, "");
}

static void
dump_names(void)
{
    LIST *m;
    char temp[MAX_BUFFER];

    Fprintf(nename, "\n/* if you maintain this by hand, keep it in */\n");
    Fprintf(nename, "/* alphabetical order!!!! */\n\n");
    Fprintf(nename, "EXTERN_CONST NTAB nametbl[] = {\n");

    BeginIf();
    for (m = all_names; m != NULL; m = m->nst) {
	WriteIf(nename, m->Cond);
	Sprintf(temp, "\t{ \"%s\",", m->Name);
	Fprintf(nename, "%s&f_%s },\n", PadTo(40, temp), m->Func);
    }
    FlushIf(nename);
    Fprintf(nename, "\t{ NULL, NULL }\n};\n");
}

/******************************************************************************/
static void
init_ufuncs(void)
{
    static const char *const head[] =
    {
	"",
	"/*\tlist of recognized macro language functions\t*/",
	"",
	"typedef enum {",
    };
    static const char *const middle[] =
    {
	"} UFuncCode;",
	"",
	"typedef struct UFUNC {",
	"\tconst char *f_name;\t/* name of function */",
	"\tunsigned f_code;",
	"} UFUNC;",
	"",
	"#define NARGMASK	0x000f",
	"#define NUM		0x0010",
	"#define BOOL		0x0020",
	"#define STR		0x0040",
	"#define NRET		0x0100",
	"#define BRET		0x0200",
	"#define SRET		0x0400",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(UFUNC vl_ufuncs[]) = {",
    };
    static int done;
    LIST *p;
    int count;

    if (!done++) {
	write_lines(nevars, head);
	for (p = all_ufuncs, count = 0; p != 0; p = p->nst) {
	    if (!count++)
		Fprintf(nevars, "\t UF%s = 0\n", p->Func);
	    else
		Fprintf(nevars, "\t,UF%s\n", p->Func);
	}
	Fprintf(nevars, "\n\t,NFUNCS /* %d */\n", count);
	write_lines(nevars, middle);
    }
}

static void
save_ufuncs(char **vec)
{
    InsertSorted(&all_ufuncs, vec[1], vec[2], vec[3], "", vec[0]);
}

static void
dump_ufuncs(void)
{
    static const char *const middle[] =
    {
	"};",
	"#else",
	"extern const UFUNC vl_ufuncs[];",
	"#endif",
	"",
    };
    char temp[MAX_BUFFER];
    LIST *p;
    int count;

    for (p = all_ufuncs, count = 0; p != 0; p = p->nst) {
	if (!count++)
	    init_ufuncs();
	Sprintf(temp, "\t{\"%s\",", p->Name);
	(void) PadTo(15, temp);
	Sprintf(temp + strlen(temp), "(unsigned)(%s)},", p->Data);
	if (p->Note[0]) {
	    (void) PadTo(32, temp);
	    Sprintf(temp + strlen(temp), "/* %s */", p->Note);
	}
	Fprintf(nevars, "%s\n", temp);
    }
    write_lines(nevars, middle);
}

/******************************************************************************/
static void
save_wmodes(char *type, char **vec)
{
    char *key = Mode2Key(type, vec[1], vec[4], FALSE);
    InsertSorted(&all_wmodes, key, vec[2], vec[3], vec[4], vec[0]);
#if NO_LEAKS
    free(key);
#endif
}

static void
dump_wmodes(void)
{
    static const char *top[] =
    {
	"",
	"/* these are the boolean, integer, and pointer value'd settings that are",
	" * associated with a window, and usually settable by a user.  There",
	" * is a global set that is inherited into a buffer, and its windows",
	" * in turn are inherit the buffer's set.",
	" */",
    };
    static const char *middle[] =
    {
	"",
	"typedef struct W_VALUES {",
	"\t/* each entry is a val, and a ptr to a val */",
	"\tstruct VAL wv[MAX_W_VALUES+1];",
	"} W_VALUES;",
	"",
	"#ifdef realdef",
	"DECL_EXTERN_CONST(struct VALNAMES w_valnames[]) = {",
    };
    static const char *bottom[] =
    {
	"",
	"\t{ NULL,\tNULL,\tVALTYPE_INT, 0 }",
	"};",
	"#else",
	"extern const struct VALNAMES w_valnames[];",
	"#endif",
    };

    write_lines(nemode, top);
    WriteIndexStruct(nemode, all_wmodes, "Windows");
    WriteModeDefines(all_wmodes, "Windows");
    write_lines(nemode, middle);
    WriteModeSymbols(all_wmodes);
    write_lines(nemode, bottom);
}

/* The accepted format for a Win32 special key binding is:
 *
 *    {<modifier>+}...<key>
 *
 * where:
 *
 *    <modifier> := SHIFT | CTRL | ALT
 *
 *    <key>      := Insert | 6
 *
 * See the comments at the end of the file "cmdtbl" for a complete
 * explanation of why more <key>'s are not supported.
 *
 *			       Caution
 *			       -------
 * The error checking supplied in mkw32binding() and InsertSorted() can
 * be fooled.
 */
#ifdef _WIN32
static void
mkw32binding(char *key, char *conditional, char *func, char *fcond)
{
#define KEYTOKEN "mod_KEY"

    char *cp = key, *match, *tmp, *defp, cum_defn[512];
    int nomore_keys, saw_modifier;

    strcpy(cum_defn, KEYTOKEN);
    defp = cum_defn + sizeof(KEYTOKEN) - 1;
    saw_modifier = 0;
    while (*cp) {
	nomore_keys = 1;	/* An assumption. */
	if ((tmp = strchr(cp, '+')) != NULL)
	    *tmp = '\0';
	match = NULL;
	if (stricmp(cp, "shift") == 0) {
	    match = "mod_SHIFT";
	    nomore_keys = 0;	/* Okay to stack modifiers */
	    saw_modifier = 1;
	} else if (stricmp(cp, "alt") == 0) {
	    match = "mod_ALT";
	    nomore_keys = 0;	/* Okay to stack modifiers */
	    saw_modifier = 1;
	} else if (stricmp(cp, "ctrl") == 0) {
	    match = "mod_CTRL";
	    nomore_keys = 0;	/* Okay to stack modifiers */
	    saw_modifier = 1;
	} else if (stricmp(cp, "insert") == 0)
	    match = "KEY_Insert";
	else if (stricmp(cp, "delete") == 0)
	    match = "KEY_Delete";
	if (match) {
	    defp += sprintf(defp, "|%s", match);
	    if (!tmp)
		break;
	    cp = tmp + 1;
	} else if (*cp == '6')
	    defp += sprintf(defp, "|'%c'", *cp++);
	if (*cp && nomore_keys)
	    badfmt("invalid/unsupported Win32 key sequence");
    }
    if (!saw_modifier)
	badfmt("missing Win32 key modifier");
    InsertSorted(&all_w32bind,
		 cum_defn,
		 func,
		 "",
		 formcond(fcond, conditional),
		 "");
#undef KEYTOKEN
}
#else /* Not a Win32 host -> dummy function -- doesn't do a thing */
#define mkw32binding(key, conditional, func, fcond)	/*EMPTY */
#endif /* _WIN32 */

/******************************************************************************/
#if NO_LEAKS
/*
 * Free all memory allocated within 'mktbls'. This is used both for debugging
 * as well as for allowing 'mktbls' to be an application procedure that is
 * repeatedly invoked from a GUI.
 */
static void
free_mktbls(void)
{
    int k;

    free_LIST(&all_names);
    free_LIST(&all_funcs);
    free_LIST(&all__FUNCs);
    free_LIST(&all__CMDFs);
    free_LIST(&all_statevars);
    free_LIST(&all_ufuncs);
    free_LIST(&all_modes);
    free_LIST(&all_submodes);
    free_LIST(&all_kbind);
    free_LIST(&all_w32bind);
    free_LIST(&all_gmodes);
    free_LIST(&all_mmodes);
    free_LIST(&all_qmodes);
    free_LIST(&all_bmodes);
    free_LIST(&all_wmodes);

    for (k = 0; k < LEN_CHRSET; k++) {
	FreeIfNeeded(bindings[k]);
	FreeIfNeeded(conditions[k]);
    }
#if DOALLOC
    show_alloc();
#endif
}
#else
#define free_mktbls()
#endif /* NO_LEAKS */

/*
 * Fix for quoted octal codes, which are sign-extended if the value is above
 * \177, e.g., vile's KEY_F21 to KEY_F35.
 */
static char *
ok_char(char *src)
{
    static char temp[20];

    if (src[0] != '\\' || src[1] == '\\') {
	Sprintf(temp, "'%s'", src);
    } else {
	strncpy(temp, src, sizeof(temp) - 1);
	*temp = '0';
    }
    return temp;
}

/******************************************************************************/
int
main(int argc, char *argv[])
{
    char *vec[MAX_PARSE];
    char line[MAX_BUFFER];
    char func[LEN_BUFFER];
    char flags[LEN_BUFFER];
    char funchelp[MAX_BUFFER];
    char old_fcond[LEN_BUFFER];
    char fcond[LEN_BUFFER];
    char modetype[LEN_BUFFER];
    SECTIONS section;
    int r;

    func[0] = flags[0] = fcond[0] = old_fcond[0] = modetype[0] = EOS;

    if (setjmp(my_top))
	return (BADEXIT);

    if (argc != 2) {
	Fprintf(stderr, "usage: mktbls cmd-file\n");
	longjmp(my_top, 1);
    }

    if ((cmdtbl = fopen(inputfile = argv[1], "r")) == NULL) {
	Fprintf(stderr, "mktbls: couldn't open cmd-file\n");
	longjmp(my_top, 1);
    }

    *old_fcond = EOS;
    section = SECT_CMDS;

    /* process each input line */
    while (fgets(line, sizeof(line), cmdtbl) != NULL) {
	char col0 = line[0], col1 = line[1];

	l++;
	r = Parse(line, vec);

	switch (col0) {
	case '#':		/* comment */
	case '\n':		/* empty-list */
	    break;

	case '.':		/* a new section */
	    switch (col1) {
	    case 'c':
		section = SECT_CMDS;
		break;
	    case 'e':
		section = SECT_VARS;
		start_vars_h(argv);
		break;
	    case 'f':
		section = SECT_FUNC;
		start_vars_h(argv);
		break;
	    case 'g':
		section = SECT_GBLS;
		break;
	    case 'm':
		section = SECT_MAJR;
		predefine_submodes(vec, r);
		break;
	    case 'q':
		section = SECT_QUAL;
		break;
	    case 'b':
		section = SECT_BUFF;
		break;
	    case 't':
		section = SECT_FSMS;
		start_fsms_h(argv, vec[2]);
		break;
	    case 'w':
		section = SECT_WIND;
		break;
	    default:
		badfmt("unknown section");
	    }
	    break;

	case '\t':		/* a new function */
	    switch (section) {
	    case SECT_CMDS:
		switch (col1) {
		case '"':	/* then it's an english name */
		    if (r < 1 || r > 2)
			badfmt("looking for english name");

		    save_names(vec[1], func, formcond(fcond, vec[2]));
		    break;

		case '\'':	/* then it's a key */
		    if (r < 1 || r > 3)
			badfmt("looking for key binding");

		    if (strcmp("W32KY", vec[2]) == 0) {
			mkw32binding(vec[1], vec[3], func, fcond);
		    } else {
			if (strncmp("KEY_", vec[2], 4) == 0) {
			    if (strncmp("FN-", vec[1], 3) != 0)
				badfmt("KEY_xxx definition must for FN- binding");
			    if (!nefkeys)
				nefkeys = OpenHeader("nefkeys.h", argv);
			    Fprintf(nefkeys, "#define %16s (SPEC|%s)\n",
				    vec[2], ok_char(vec[1] + 3));
			    vec[2] = vec[3];
			}
			save_bindings(vec[1], func, formcond(fcond, vec[2]));
		    }
		    break;

		case '<':	/* then it's a help string */
		    /* put code here. */
		    (void) strcpy(funchelp, vec[1]);
		    break;

		default:
		    badfmt("bad line");
		}
		break;

	    case SECT_GBLS:
		if (r < 2 || r > 4)
		    badfmt("looking for GLOBAL modes");
		save_gmodes(modetype, vec);
		break;

	    case SECT_MAJR:
		if (r < 2 || r > 4)
		    badfmt("looking for MAJOR modes");
		save_mmodes(modetype, vec);
		break;

	    case SECT_QUAL:
		if (r < 2 || r > 4)
		    badfmt("looking for QUALIFIER modes");
		save_qmodes(modetype, vec);
		break;

	    case SECT_BUFF:
		if (r < 2 || r > 4)
		    badfmt("looking for BUFFER modes");
		save_bmodes(modetype, vec);
		break;

	    case SECT_WIND:
		if (r < 2 || r > 4)
		    badfmt("looking for WINDOW modes");
		save_wmodes(modetype, vec);
		break;

	    default:
		badfmt("did not expect a tab");
	    }
	    break;

	default:		/* cache information about funcs */
	    switch (section) {
	    case SECT_CMDS:
		if (r < 2 || r > 3)
		    badfmt("looking for new function");

		/* don't save this yet -- we may get a
		   a help line for it.  save the previous
		   one now, and hang onto this one */
		if (func[0]) {	/* flush the old one */
		    save_funcs(func, flags, fcond,
			       old_fcond, funchelp);
		    funchelp[0] = EOS;
		}
		(void) strcpy(func, vec[1]);
		(void) strcpy(flags, vec[2]);
		(void) strcpy(fcond, vec[3]);
		break;

	    case SECT_FSMS:
		save_fsms(vec);
		break;

	    case SECT_VARS:
		if (r < 2 || r > 3)
		    badfmt("looking for char *statevars[]");
		save_statevars(vec);
		break;

	    case SECT_FUNC:
		if (r < 2 || r > 3)
		    badfmt("looking for UFUNC func[]");
		save_ufuncs(vec);
		break;

	    case SECT_GBLS:
	    case SECT_MAJR:
	    case SECT_QUAL:
	    case SECT_BUFF:
	    case SECT_WIND:
		if (r != 1
		    || (!strcmp(vec[1], "bool")
			&& !strcmp(vec[1], "enum")
			&& !strcmp(vec[1], "int")
			&& !strcmp(vec[1], "string")
			&& !strcmp(vec[1], "regex")))
		    badfmt("looking for mode datatype");
		(void) strcpy(modetype, vec[1]);
		break;

	    default:
		badfmt("section not implemented");
	    }
	}
    }

    if (func[0]) {		/* flush the old one */
	save_funcs(func, flags, fcond, old_fcond, funchelp);
	funchelp[0] = EOS;
    }
    if (*old_fcond) {
	SaveEndif(all_funcs);
	SaveEndif(all__FUNCs);
	SaveEndif(all__CMDFs);
    }

    if (all_names) {
	nebind = OpenHeader("nebind.h", argv);
	neexec = OpenHeader("neexec.h", argv);
	nefunc = OpenHeader("nefunc.h", argv);
	neprot = OpenHeader("neproto.h", argv);
	nename = OpenHeader("nename.h", argv);
	dump_names();
	dump_bindings();
	dump_execs(neexec, OPT_EXEC_MACROS);
	dump_funcs(neprot, all_funcs);

	Fprintf(nefunc, "\n#ifdef real_CMDFUNCS\n\n");
	dump_funcs(nefunc, all__FUNCs);
	Fprintf(nefunc, "\n#else\n\n");
	dump_funcs(nefunc, all__CMDFs);
	Fprintf(nefunc, "\n#endif\n");
    }

    if (all_statevars) {
	dump_statevars();
	dump_ufuncs();
	finish_vars_h();
    }

    dump_fsms();
    finish_fsms_h();

    if (all_wmodes || all_bmodes) {
	nemode = OpenHeader("nemode.h", argv);
	DefineOffset(nemode);
	dump_majors();
	dump_all_modes();
	dump_all_submodes();
	dump_gmodes();
	dump_mmodes();
	dump_qmodes();
	dump_bmodes();
	dump_wmodes();
    }

    free_mktbls();
    return (GOODEXIT);
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          