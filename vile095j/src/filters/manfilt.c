/*
 * manfilt.c		-- replace backspace sequences with attribute
 *			   information for vile
 *
 * Author: Kevin A. Buettner
 * Creation: 4/17/94
 *
 * This program filters backspace sequences often found in manual pages
 * for vile/xvile.  Backspace sequences representing italicized or bold
 * text are fixed up by removing the backspaces, underlines, and duplicate
 * characters (leaving just the text as it should appear on the screen).
 * Attributed text is so indicated by inserting a Cntrl-A sequence in front
 * of the text to be attributed.  These Cntrl-A sequences take the following
 * form:
 *   	^A<Count><Attr>:
 *
 * <Count> is a sequence of digits representing the number of characters
 * following the ':' to be attributed.
 *
 * <Attr> is a sequence of characters which indicates how to attribute the
 * characters following the ':'.  The following characters are presently
 * recognized by vile:
 *
 *	'I'	-- italic
 *	'B'	-- bold
 *	'U'	-- underline
 *	'R'	-- reverse video
 *
 * Examples:
 *	Before					After
 *	------					-----
 *	_^Hi_^Ht_^Ha_^Hl_^Hi_^Hc		^A6I:italic
 *
 *	b^Hbo^Hol^Hld^Hd			^A4B:bold
 *
 *	_^HB^HB_^Ho^Ho_^Ht^Ht_^Hh^Hh		^A4IB:Both
 *
 * On many system, bold sequences are actually quite a bit longer.  On
 * some systems, the repeated character is repeated as many as four times.
 * Thus the letter "B" would be represented as B^HB^HB^HB.
 *
 * For comparison, see the description of the BSD 'col' program (for
 * information about the types of escape sequences that might be emitted by
 * nroff).
 *
 * vile will choose some appropriate fallback (such as underlining) if
 * italics are not available.
 *
 * $Header: /usr/build/vile/vile/filters/RCS/manfilt.c,v 1.36 2004/10/27 19:18:30 tom Exp $
 *
 */

#define CAN_TRACE 0
#define CAN_VMS_PATH 0
#include <filters.h>

#if OPT_LOCALE
#include	<locale.h>
#include	<ctype.h>
#endif

#if !defined(HAVE_STDLIB_H)
# if !defined(HAVE_CONFIG_H) || defined(MISSING_EXTERN_CALLOC)
extern char *calloc(size_t nmemb, size_t size);
# endif
# if !defined(HAVE_CONFIG_H) || defined(MISSING_EXTERN_REALLOC)
extern char *realloc(char *ptr, size_t size);
# endif
#endif

#define backspace() \
		if (cur_line != 0 \
		 && cur_line->l_this > 0) \
		    cur_line->l_this -= 1;

#define MAX_LINES 200

	/* SGR codes that we use as mask values */
#define ATR_NORMAL 0
#define ATR_BOLD   1
#define ATR_ITAL   2
#define ATR_UNDER  4
#define ATR_REVERS 8

#define SHL_COLOR  4
#define ATR_COLOR  (0xf << SHL_COLOR)

	/* character names */
#define ESCAPE    '\033'
#define CNTL_A    '\001'
#define SHIFT_OUT '\016'
#define SHIFT_IN  '\017'

#define SPACE     ' '
#define UNDERLINE '_'

#define CS_NORMAL    0
#define CS_ALTERNATE 1
/*
 * Each column of a line is represented by a linked list of the characters that
 * are printed to that position.  When storing items in this list, we keep a
 * canonical order to simplify analysis when dumping the line.
 */
typedef struct CharCell {
    struct CharCell *link;
    int c_attrs;		/* base cell only: ATR_NORMAL, etc. */
    char c_ident;		/* CS_NORMAL/CS_ALTERNATE */
    char c_level;		/* 0=base, 1=up halfline, -1=down halfline */
    char c_value;		/* the actual value */
} CHARCELL;

typedef struct LineData {
    struct LineData *l_next;
    struct LineData *l_prev;
    size_t l_last;		/* the number of cells allocated in l_cells[] */
    size_t l_used;		/* the number of cells used in l_cells[] */
    size_t l_this;		/* the current cell within the line */
    CHARCELL *l_cell;
} LINEDATA;

static void flush_line(void);

static LINEDATA *all_lines;
static LINEDATA *cur_line;
static long total_lines;
static int (*my_getc) (FILE *);

#if OPT_LOCALE
static int utf8_locale = 0;
#endif

static void
failed(const char *s)
{
    perror(s);
    exit(BADEXIT);
}

static int
ansi_getc(FILE *fp)
{
    return fgetc(fp);
}

#if OPT_LOCALE
/*
 * groff 1.18 versions generates UTF-8 - we want Latin1.  Decode it.
 */
static int
utf8_getc(FILE *fp)
{
#define UCS_REPL '?'
    int utf_count = 0;
    int utf_char = 0;

    do {
	int c = fgetc(fp);
	/* Combine UTF-8 into Unicode */
	if (c < 0x80) {
	    /* We received an ASCII character */
	    /* if (utf_count > 0), prev. sequence incomplete */
	    utf_char = c;
	    utf_count = 0;
	} else if (c < 0xc0) {
	    /* We received a continuation byte */
	    if (utf_count < 1) {
		utf_char = UCS_REPL;	/* ... unexpectedly */
	    } else {
		if (!utf_char && !((c & 0x7f) >> (7 - utf_count))) {
		    utf_char = UCS_REPL;
		}
		/* characters outside UCS-2 become UCS_REPL */
		if (utf_char > 0x03ff) {
		    /* value would be >0xffff */
		    utf_char = UCS_REPL;
		} else {
		    utf_char <<= 6;
		    utf_char |= (c & 0x3f);
		}
		if ((utf_char >= 0xd800 &&
		     utf_char <= 0xdfff) ||
		    (utf_char == 0xfffe) ||
		    (utf_char == 0xffff)) {
		    utf_char = UCS_REPL;
		}
		utf_count--;
	    }
	} else {
	    /* We received a sequence start byte */
	    /* if (utf_count > 0), prev. sequence incomplete */
	    if (c < 0xe0) {
		utf_count = 1;
		utf_char = (c & 0x1f);
		if (!(c & 0x1e))
		    utf_char = UCS_REPL;	/* overlong sequence */
	    } else if (c < 0xf0) {
		utf_count = 2;
		utf_char = (c & 0x0f);
	    } else if (c < 0xf8) {
		utf_count = 3;
		utf_char = (c & 0x07);
	    } else if (c < 0xfc) {
		utf_count = 4;
		utf_char = (c & 0x03);
	    } else if (c < 0xfe) {
		utf_count = 5;
		utf_char = (c & 0x01);
	    } else {
		utf_char = UCS_REPL;
		utf_count = 0;
	    }
	}
    } while (utf_count > 0);
    if (utf_char > 255) {
	switch (utf_char) {
	case 0x2018:
	    utf_char = '`';
	    break;
	case 0x2019:
	    utf_char = '\'';
	    break;
	case 0x2010:
	case 0x2212:
	    utf_char = '-';
	    break;
	default:
	    utf_char = '?';
	    break;
	}
    }
    return utf_char;
}
#endif

/*
 * Allocate a CHARCELL struct
 */
static CHARCELL *
allocate_cell(void)
{
    CHARCELL *p = typecallocn(CHARCELL, 1);
    if (p == 0)
	failed("allocate_cell");
    return p;
}

/*
 * Allocate a LINEDATA struct
 */
static LINEDATA *
allocate_line(void)
{
    LINEDATA *p = typecallocn(LINEDATA, 1);
    if (p == 0)
	failed("allocate_line");

    if (all_lines == 0)
	all_lines = p;

    if (total_lines++ > MAX_LINES)
	flush_line();

    return p;
}

/*
 * (Re)allocate the l_cell[] array for the current line
 */
static void
extend_line(void)
{
    size_t have = cur_line->l_last;
    size_t want = cur_line->l_this;
    if (want >= have) {
	CHARCELL *c = cur_line->l_cell;
	want += 80;
	if (c == 0) {
	    c = typecallocn(CHARCELL, want);
	} else {
	    c = typereallocn(CHARCELL, c, want);
	}
	if (c == 0)
	    failed("extend_line");
	while (have < want) {
	    c[have].link = 0;
	    c[have].c_attrs = ATR_NORMAL;
	    c[have].c_value = SPACE;
	    c[have].c_level = 0;
	    c[have].c_ident = CS_NORMAL;
	    have++;
	}
	cur_line->l_last = want;
	cur_line->l_cell = c;
    }
}

/*
 * Store a character at the current position, updating the current position.
 * We expect (but do not require) that an underscore precedes a nonblank
 * character that will overstrike it.  (Some programs produce the underscore
 * second, rather than first).
 */
static void
put_cell(int c, int level, int ident, int attrs)
{
    int col;
    int len;
    CHARCELL *p, *q;

    if (cur_line == 0)
	cur_line = allocate_line();

    len = cur_line->l_used;
    col = cur_line->l_this++;
    extend_line();

    p = &(cur_line->l_cell[col]);
    p->c_attrs = attrs;

    if (len > col) {		/* some type of overstrike */
	if (c == UNDERLINE) {
	    while ((q = p->link) != 0)
		p = q;
	    q = allocate_cell();
	    p->link = q;
	    p = q;
	} else {
	    if ((c != SPACE)
		|| (p->c_value == UNDERLINE)) {
		q = allocate_cell();
		*q = *p;
		p->link = q;
	    } else if (c == SPACE)
		return;
	}
    }

    p->c_value = c;
    p->c_level = level;
    p->c_ident = ident;

    if (cur_line->l_used < cur_line->l_this)
	cur_line->l_used = cur_line->l_this;
}

/*
 * Interpret equivalent overstrike/underline for an ANSI escape sequence.
 */
static int
ansi_escape(FILE *ifp, int last_code)
{
    int ansi = 1;
    int code = ATR_NORMAL;
    int value = 0;
    int digits = 0;
    int c;

    while ((c = my_getc(ifp)) != EOF) {
	c = CharOf(c);
	if (isdigit(c)) {
	    value = (value * 10) + (c - '0');
	    digits++;
	} else {
	    if (digits) {
		switch (value) {
		case 1:
		    code |= ATR_BOLD;
		    break;
		case 3:
		    code |= ATR_ITAL;
		    break;
		case 4:
		    code |= ATR_UNDER;
		    break;
		case 7:
		    code |= ATR_REVERS;
		    break;
		case 22:
		    code &= ~ATR_BOLD;
		    break;
		case 23:
		    code &= ~ATR_BOLD;
		    break;
		case 24:
		    code &= ~ATR_UNDER;
		    break;
		case 27:
		    code &= ~ATR_REVERS;
		    break;
		default:
		    /* we handle only foreground (text) colors */
		    if (value >= 30 && value <= 37) {
			value -= 30;
			value <<= SHL_COLOR;
			code &= ~ATR_COLOR;
			code |= value;
		    }
		    break;
		}
	    }
	    if (isalpha(c))
		break;
	    else if (c != ';')
		ansi = 0;
	    value = 0;
	    digits = 0;
	}
    }

    if ((c != 'm') || !ansi) {
	code = last_code;
    }
    return code;
}

/*
 * Set the current pointer to the previous line, allocating it if necessary
 */
static void
prev_line(void)
{
    LINEDATA *old_line;

    if (cur_line == 0)
	cur_line = allocate_line();

    if (cur_line->l_prev == 0) {
	cur_line->l_prev = allocate_line();
	if (cur_line == all_lines)
	    all_lines = cur_line->l_prev;
    }
    old_line = cur_line;
    cur_line = cur_line->l_prev;
    cur_line->l_next = old_line;
    cur_line->l_this = old_line->l_this;
}

/*
 * Set the current pointer to the next line, allocating it if necessary
 */
static void
next_line(void)
{
    LINEDATA *old_line;

    if (cur_line == 0)
	cur_line = allocate_line();

    if (cur_line->l_next == 0)
	cur_line->l_next = allocate_line();

    old_line = cur_line;
    cur_line = cur_line->l_next;
    cur_line->l_prev = old_line;
    cur_line->l_this = old_line->l_this;
}

/*
 * If we've got a blank line to write onto, fake half-lines that way.
 * Otherwise, eat them.  We assume that half-line controls occur in pairs.
 */
static int
half_up(int level)
{
    prev_line();
    if (cur_line->l_this < cur_line->l_used) {
	next_line();
	return level + 1;
    }
    return 0;
}

static int
half_down(int level)
{
    if (level == 0) {
	next_line();
	return 0;
    }
    return level - 1;
}

static int
cell_code(LINEDATA * line, size_t col)
{
    CHARCELL *p = &(line->l_cell[col]);
    CHARCELL *q;
    int code = p->c_attrs;

    while ((q = p->link) != 0) {
	if (q->c_value == UNDERLINE
	    && q->c_value != p->c_value) {
	    code |= ATR_UNDER;
	} else
	    code |= ATR_BOLD;
	p = q;
    }
    return code;
}

/*
 * Write the oldest line from memory to standard output and deallocate its
 * storage.
 */
static void
flush_line(void)
{
    size_t col;
    int ref_code;
    int tst_code;
    int counter;
    LINEDATA *l = all_lines;
    CHARCELL *p;

    if (l != 0) {
	all_lines = l->l_next;
	if (cur_line == l)
	    cur_line = all_lines;

	ref_code = ATR_NORMAL;
	counter = 0;
	for (col = 0; col < l->l_used; col++) {
	    if (--counter <= 0) {
		tst_code = cell_code(l, col);
		if (tst_code != ref_code) {
		    ref_code = tst_code;
		    for (counter = 1; counter + col < l->l_used; counter++) {
			tst_code = cell_code(l, col + counter);
			if (tst_code != ref_code)
			    break;
		    }
		    if (ref_code != ATR_NORMAL) {
			printf("%c%d", CNTL_A, counter);
			if (ref_code & ATR_BOLD)
			    putchar('B');
			if (ref_code & ATR_ITAL)
			    putchar('I');
			if (ref_code & ATR_UNDER)
			    putchar('U');
			if (ref_code & ATR_REVERS)
			    putchar('R');
			if (ref_code & ATR_COLOR)
			    printf("C%X", (ref_code >> SHL_COLOR) & 0xf);
			putchar(':');
		    }
		}
	    }
	    putchar(l->l_cell[col].c_value);

	    while ((p = l->l_cell[col].link) != 0) {
		l->l_cell[col].link = p->link;
		free((char *) p);
	    }
	}
	putchar('\n');

	if (l != 0) {
	    if (l->l_cell != 0)
		free((char *) l->l_cell);
	    free((char *) l);
	}
    }
}

/*
 * Filter an entire file, writing the result to the standard output.
 */
static void
ManFilter(FILE *ifp)
{
    int c;
    int level = 0;
    int ident = CS_NORMAL;
    int esc_mode = ATR_NORMAL;

    while ((c = my_getc(ifp)) != EOF) {
	c = CharOf(c);
	switch (c) {
	case '\b':
	    backspace();
	    break;

	case '\r':
	    if (cur_line != 0)
		cur_line->l_this = 0;
	    break;

	case '\n':
	    next_line();
	    cur_line->l_this = 0;
	    break;

	case '\t':
	    do {
		put_cell(SPACE, level, ident, esc_mode);
	    } while (cur_line->l_this & 7);
	    break;

	case '\v':
	    prev_line();
	    break;

	case SHIFT_IN:
	    ident = CS_NORMAL;
	    break;

	case SHIFT_OUT:
	    ident = CS_ALTERNATE;
	    break;

	case ESCAPE:
	    switch (my_getc(ifp)) {
	    case '[':
		esc_mode = ansi_escape(ifp, esc_mode);
		break;
	    case '\007':
	    case '7':
		prev_line();
		break;
	    case '\010':
	    case '8':
		level = half_up(level);
		break;
	    case '\011':
	    case '9':
		level = half_down(level);
		break;
	    default:		/* ignore everything else */
		break;
	    }
	    break;

	case 0xad:		/* mis-used softhyphen */
	    c = '-';
	    /* FALLTHRU */

	default:		/* ignore other nonprinting characters */
	    if (isprint(c)) {
		put_cell(c, level, ident, esc_mode);
		if (c != SPACE) {
		    if (esc_mode & ATR_BOLD) {
			backspace();
			put_cell(c, level, ident, esc_mode);
		    }
		    if (esc_mode & ATR_UNDER) {
			backspace();
			put_cell('_', level, ident, esc_mode);
		    }
		}
	    }
	    break;
	}
    }

    while (all_lines != 0)
	flush_line();

    total_lines = 0;
}

int
main(int argc, char **argv)
{
    int n;

    my_getc = ansi_getc;
#if OPT_LOCALE
    {
	char *env;

	if (((env = getenv("LC_ALL")) != 0 && *env != 0) ||
	    ((env = getenv("LC_CTYPE")) != 0 && *env != 0) ||
	    ((env = getenv("LANG")) != 0 && *env != 0)) {

	    if (strstr(env, ".UTF-8") != 0) {
		utf8_locale = 1;
		my_getc = utf8_getc;
	    }
	}
	setlocale(LC_CTYPE, "");
    }
#endif

    if (argc > 1) {
	for (n = 1; n < argc; n++) {
	    FILE *fp = fopen(argv[n], "r");
	    if (fp == 0)
		failed(argv[n]);
	    ManFilter(fp);
	    (void) fclose(fp);
	}
    } else {
	ManFilter(stdin);
    }
    fflush(stdout);
    fclose(stdout);
    exit(GOODEXIT);		/* successful exit */
    /*NOTREACHED */
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          