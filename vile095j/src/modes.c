/*
 *	modes.c
 *
 * Maintain and list the editor modes and value sets.
 *
 * Original code probably by Dan Lawrence or Dave Conroy for MicroEMACS.
 * Major extensions for vile by Paul Fox, 1991
 * Majormode extensions for vile by T.E.Dickey, 1997
 *
 * $Header: /usr/build/vile/vile/RCS/modes.c,v 1.317 2006/05/21 19:24:48 tom Exp $
 *
 */

#include	"estruct.h"
#include	"edef.h"
#include	"chgdfunc.h"

#define	NonNull(s)	((s == 0) ? "" : s)
#define	ONE_COL	(80/3)

#define isLocalVal(valptr)          ((valptr)->vp == &((valptr)->v))
#define makeLocalVal(valptr)        ((valptr)->vp = &((valptr)->v))

#define NO_PREFIX	"no"
#define noPrefix(mode)	(!strncmp(mode, NO_PREFIX, 2))

#include "nefsms.h"

/*--------------------------------------------------------------------------*/

#if OPT_UPBUFF
static void relist_settings(void);
#else
#define relist_settings()	/* nothing */
#endif

#if OPT_ENUM_MODES
static const FSM_CHOICES *valname_to_choices(const struct VALNAMES *names);
#endif

/*--------------------------------------------------------------------------*/

#if OPT_EVAL || OPT_MAJORMODE
static const char **my_varmodes;	/* list for modename-completion */
#endif

#if OPT_MAJORMODE
typedef struct {
    char *shortname;		/* copy of MAJORMODE.shortname */
    char *longname;		/* copy of MAJORMODE.longname */
    MAJORMODE *data;		/* pointer to actual data */
    int init;			/* predefined during initialization */
    int flag;			/* true when majormode is active/usable */
    struct VALNAMES qual[MAX_M_VALUES + 1];	/* majormode qualifier names */
    struct VALNAMES used[MAX_B_VALUES + 1];	/* submode names */
    struct VALNAMES subq[MAX_Q_VALUES + 1];	/* submode qualifier names */
} MAJORMODE_LIST;

static MAJORMODE_LIST *my_majormodes;
static int *majormodes_order;	/* index, for precedence */
static M_VALUES global_m_values;	/* dummy, for convenience */
static struct VAL *major_g_vals;	/* on/off values of major modes */
static struct VAL *major_l_vals;	/* dummy, for convenience */
static struct VALNAMES *major_valnames;

static int did_attach_mmode;

static const char **my_mode_list;	/* copy of 'all_modes[]' */
static int sizeof_my_mode_list;	/* ...its length */

#define is_bool_type(type) ((type) == VALTYPE_BOOL || (type) == VALTYPE_MAJOR)

static MAJORMODE_LIST *lookup_mm_list(const char *name);
static char *majorname(char *dst, const char *majr, int flag);
static const char *ModeName(const char *name);
static int attach_mmode(BUFFER *bp, const char *name);
static int detach_mmode(BUFFER *bp, const char *name);
static int enable_mmode(const char *name, int flag);
static struct VAL *get_sm_vals(MAJORMODE * ptr);
static void init_my_mode_list(void);

#if OPT_UPBUFF
static void relist_majormodes(void);
#endif

#else

#define ModeName(s) s
#define init_my_mode_list()	/* nothing */
#define is_bool_type(type) ((type) == VALTYPE_BOOL)
#define my_mode_list all_modes
#define relist_majormodes()	/* nothing */

#endif /* OPT_MAJORMODE */

#define is_str_type(type) ((type) == VALTYPE_REGEX || (type) == VALTYPE_STRING)

/*--------------------------------------------------------------------------*/

#if OPT_ENUM_MODES || !SMALLER
int
choice_to_code(const FSM_CHOICES * choices, const char *name, size_t len)
{
    char temp[NSTRING];
    int code = ENUM_ILLEGAL;
    int i;

    if (choices != 0 && name != 0 && len != 0) {
	if (len > NSTRING - 1)
	    len = NSTRING - 1;

	memcpy(temp, name, len);
	temp[len] = EOS;
	(void) mklower(temp);

	for (i = 0; choices[i].choice_name != 0; i++) {
	    if (!strncmp(temp, choices[i].choice_name, len)) {
		if (choices[i].choice_name[len] == EOS
		    || choices[i + 1].choice_name == 0
		    || strncmp(temp, choices[i + 1].choice_name, len))
		    code = choices[i].choice_code;
		break;
	    }
	}
    }
    return code;
}

const char *
choice_to_name(const FSM_CHOICES * choices, int code)
{
    const char *name = 0;
    int i;

    if (choices == 0) {
	name = "";
    } else {
	for (i = 0; choices[i].choice_name != 0; i++) {
	    if (choices[i].choice_code == code) {
		name = choices[i].choice_name;
		break;
	    }
	}
    }
    return name;
}
#endif /* OPT_ENUM_MODES || !SMALLER */

#if !SMALLER
/*
 * Add the values from one or more keywords.  If any error is found, return that
 * code instead.
 */
int
combine_choices(const FSM_CHOICES * choices, const char *string)
{
    char temp[NSTRING];
    const char *s;
    int code;
    int result = 0;

    while (*string) {
	vl_strncpy(temp, string, sizeof(temp));
	if ((s = strchr(string, '+')) != 0) {
	    temp[s - string] = EOS;
	    string = s + 1;
	} else {
	    string += strlen(string);
	}
	if ((code = choice_to_code(choices, temp, strlen(temp))) >= 0) {
	    result += code;
	} else {
	    result = code;
	    break;
	}
    }
    return result;
}
#endif

/*--------------------------------------------------------------------------*/

void
set_winflags(int glob_vals, USHORT flags)
{
    if (glob_vals) {
	WINDOW *wp;
	for_each_visible_window(wp) {
	    if ((wp->w_bufp == NULL)
		|| !b_is_scratch(wp->w_bufp)
		|| !(flags & WFMODE))
		wp->w_flag |= flags;
	}
    } else {
	curwp->w_flag |= flags;
    }
}

static int
same_val(const struct VALNAMES *names, struct VAL *tst, struct VAL *ref)
{
    if (ref == 0)		/* can't test, not really true */
	return -TRUE;

    switch (names->type) {
#if OPT_MAJORMODE
    case VALTYPE_MAJOR:
	/*FALLTHRU */
#endif
    case VALTYPE_BOOL:
    case VALTYPE_ENUM:
    case VALTYPE_INT:
	return (tst->vp->i == ref->vp->i);
    case VALTYPE_STRING:
	return (tst->vp->p != 0)
	    && (ref->vp->p != 0)
	    && !strcmp(tst->vp->p, ref->vp->p);
    case VALTYPE_REGEX:
	return (tst->vp->r != 0)
	    && (ref->vp->r != 0)
	    && (tst->vp->r->pat != 0)
	    && (ref->vp->r->pat != 0)
	    && !strcmp(tst->vp->r->pat, ref->vp->r->pat);
    default:
	mlforce("BUG: bad type %s %d", ModeName(names->name), names->type);
    }

    return FALSE;
}

/*
 * Returns the value if it is a string, null otherwise.
 */
static const char *
string_val(const struct VALNAMES *names, struct VAL *values)
{
    const char *s = 0;

    switch (names->type) {
#if OPT_ENUM_MODES		/* will show the enum name too */
    case VALTYPE_ENUM:
	s = choice_to_name(valname_to_choices(names), values->vp->i);
	break;
#endif
    case VALTYPE_STRING:
	s = values->vp->p;
	break;
    case VALTYPE_REGEX:
	if (values->vp->r)
	    s = values->vp->r->pat;
	break;
    }

    return isEmpty(s) ? 0 : s;
}

/*
 * Returns the formatted length of a string value.
 */
static size_t
size_val(const struct VALNAMES *names, struct VAL *values)
{
    return strlen(ModeName(names->name))
	+ 3
	+ strlen(NonNull(string_val(names, values)));
}

/*
 * Returns a mode-value formatted as a string
 */
const char *
string_mode_val(VALARGS * args)
{
    const struct VALNAMES *names = args->names;
    struct VAL *values = args->local;
    static TBUFF *result;
    switch (names->type) {
#if OPT_MAJORMODE
    case VALTYPE_MAJOR:
#endif
    case VALTYPE_BOOL:
	return values->vp->i ? "TRUE" : "FALSE";
    case VALTYPE_ENUM:
#if OPT_ENUM_MODES
	{
	    static char temp[20];	/* FIXME: const workaround */
	    (void) strcpy(temp,
			  choice_to_name(valname_to_choices(names), values->vp->i));
	    return temp;
	}
#endif /* else, fall-thru to use int-code */
    case VALTYPE_INT:
	return render_int(&result, values->vp->i);
    case VALTYPE_STRING:
	return NonNull(values->vp->p);
    case VALTYPE_REGEX:
	if (values->vp->r == 0)
	    break;
	return NonNull(values->vp->r->pat);
    }
    return error_val;
}

/* listvalueset:  print each value in the array according to type, along with
 * its name, until a NULL name is encountered.  If not local, only print if the value in the
 * two arrays differs, or the second array is nil.  If local, print only the
 * values in the first array that are local.
 */
static int
listvalueset(const char *which,
	     int nflag,
	     int local,
	     const struct VALNAMES *names,
	     struct VAL *values,
	     struct VAL *globvalues)
{
    int show[MAX_G_VALUES + MAX_B_VALUES + MAX_W_VALUES];
    int any = 0;
    int passes = 1;
    int ncols = term.cols / ONE_COL;
    int padded;
    int perline;
    int percol;
    int total;
    int j, pass;

    if (ncols > MAXCOLS)
	ncols = MAXCOLS;

    /*
     * First, make a list of values we want to show.
     * Store:
     *      0 - don't show
     *      1 - show in first pass
     *      2 - show in second pass (too long)
     */
    for (j = 0; names[j].name != 0; j++) {
	int ok;
	show[j] = 0;
	if (local) {
	    ok = is_local_val(values, j);
	} else {
	    ok = (same_val(names + j, values + j, globvalues ? globvalues +
			   j : 0) != TRUE);
	}
	if (ok) {
	    switch (names[j].type) {
	    case VALTYPE_ENUM:
	    case VALTYPE_STRING:
	    case VALTYPE_REGEX:
		if (string_val(names + j, values + j) == 0) {
		    ok = FALSE;
		    break;
		}
		if (size_val(names + j, values + j) >= ONE_COL) {
		    show[j] += 1;
		    passes = 2;
		}
		/* fall-thru */
	    default:
		show[j] += 1;
	    }
	}
	if (ok && !any++) {
	    if (nflag)
		bputc('\n');
	    bprintf("%s:\n", which);
	}
    }
    total = j;

    if (any) {
	if (!passes)
	    passes = 1;
    } else
	return nflag;

    /*
     * Now, go back and display the values
     */
    for (pass = 1; pass <= passes; pass++) {
	int line, col, k;
	int offsets[MAXCOLS + 1];

	offsets[0] = 0;
	if (pass == 1) {
	    for (j = percol = 0; j < total; j++) {
		if (show[j] == pass)
		    percol++;
	    }
	    for (j = 1; j < ncols; j++) {
		offsets[j]
		    = (percol + ncols - j) / ncols
		    + offsets[j - 1];
	    }
	    perline = ncols;
	} else {		/* these are too wide for ONE_COL */
	    offsets[1] = total;
	    perline = 1;
	}
	offsets[ncols] = total;

	line = 0;
	col = 0;
	any = 0;
	for_ever {
	    k = line + offsets[col];
	    for (j = 0; j < total; j++) {
		if (show[j] == pass) {
		    if (k-- <= 0)
			break;
		}
	    }
	    if (k >= 0)		/* no more cells to display */
		break;

	    if (col == 0)
		bputc(' ');
	    padded = DOT.o + ((col + 1) < perline ? ONE_COL : 1);
	    if (is_bool_type(names[j].type)) {
		bprintf("%s%s",
			values[j].vp->i ? "  " : NO_PREFIX,
			ModeName(names[j].name));
	    } else {
		VALARGS args;	/* FIXME */
		args.names = names + j;
		args.local = values + j;
		args.global = 0;
		bprintf("  %s=%s",
			ModeName(names[j].name),
			string_mode_val(&args));
	    }
	    bpadc(' ', padded - DOT.o);
	    any++;
	    if (++col >= perline) {
		col = 0;
		bputc('\n');
		if (++line >= offsets[1])
		    break;
	    } else if (line + offsets[col] >= offsets[col + 1])
		break;
	}
	if (any && (pass != passes))
	    bputc('\n');
	if (col != 0)
	    bputc('\n');
    }
    return TRUE;
}

#ifdef lint
static /*ARGSUSED */ WINDOW *
ptr2WINDOW(void *p)
{
    return 0;
}
#else
#define	ptr2WINDOW(p)	(WINDOW *)p
#endif

/* list the current modes into the current buffer */
/* ARGSUSED */
static void
makemodelist(int local, void *ptr)
{
    static const char gg[] = "Universal";
    static const char bb[] = "Buffer";
    static const char ww[] = "Window";
    int nflag, nflg2;

    WINDOW *localwp = ptr2WINDOW(ptr);	/* alignment okay */
    BUFFER *localbp = localwp->w_bufp;
    struct VAL *local_b_vals = localbp->b_values.bv;
    struct VAL *local_w_vals = localwp->w_values.wv;
    struct VAL *globl_b_vals = global_b_values.bv;

#if OPT_UPBUFF
    if (relisting_b_vals != 0)
	local_b_vals = relisting_b_vals;
    if (relisting_w_vals != 0)
	local_w_vals = relisting_w_vals;
#endif

#if OPT_MAJORMODE
    if (local && (localbp->majr != 0)) {
	bprintf("--- \"%s\" settings, if different than \"%s\" majormode ",
		localbp->b_bname,
		localbp->majr->shortname);
	globl_b_vals = get_sm_vals(localbp->majr);
    } else
#endif
	bprintf("--- \"%s\" settings, if different than globals ",
		localbp->b_bname);
    bpadc('-', term.cols - DOT.o);
    bputc('\n');

    nflag = listvalueset(bb, FALSE, FALSE, b_valnames, local_b_vals, globl_b_vals);
    nflg2 = listvalueset(ww, nflag, FALSE, w_valnames, local_w_vals, global_w_values.wv);
    if (!(nflag || nflg2))
	bputc('\n');
    bputc('\n');

    bprintf("--- %s settings ", local ? "Local" : "Global");
    bpadc('-', term.cols - DOT.o);
    bputc('\n');

    if (local) {
	nflag = listvalueset(bb, nflag, local, b_valnames, local_b_vals,
			     (struct VAL *) 0);
	(void) listvalueset(ww, nflag, local, w_valnames, local_w_vals,
			    (struct VAL *) 0);
    } else {
	nflag = listvalueset(gg, nflag, local, g_valnames,
			     global_g_values.gv, (struct VAL *) 0);
	nflag = listvalueset(bb, nflag, local, b_valnames, globl_b_vals,
			     (struct VAL *) 0);
	(void) listvalueset(ww, nflag, local, w_valnames,
			    global_w_values.wv, (struct VAL *) 0);
    }
}

/*
 * Set tab size
 */
int
settab(int f, int n)
{
    WINDOW *wp;
#if OPT_MAJORMODE
    int val = VAL_TAB;
    const char *whichtabs = "T";
#else
    int val;
    const char *whichtabs;
    if (is_c_mode(curbp)) {
	val = VAL_C_TAB;
	whichtabs = "C-t";
    } else {
	val = VAL_TAB;
	whichtabs = "T";
    }
#endif
    if (f && n >= 1) {
	make_local_b_val(curbp, val);
	set_b_val(curbp, val, n);
	for_each_visible_window(wp) {
	    if (wp->w_bufp == curbp)
		wp->w_flag |= WFHARD;
	}
    } else if (f) {
	mlwarn("[Illegal tabstop value]");
	return FALSE;
    }
    if (!global_b_val(MDTERSE) || !f)
	mlwrite("[%sabs are %d columns apart, using %s value.]", whichtabs,
		tabstop_val(curbp),
		is_local_b_val(curbp, val) ? "local" : "global");
    return TRUE;
}

/*
 * Set fill column to n.
 */
int
setfillcol(int f, int n)
{
    if (f) {
	make_local_b_val(curbp, VAL_FILL);
	set_b_val(curbp, VAL_FILL, n);
    }
    if (!global_b_val(MDTERSE) || !f)
	mlwrite("[Fill column is %d, and is %s]",
		getfillcol(curbp),
		is_local_b_val(curbp, VAL_FILL) ? "local" : "global");
    return (TRUE);
}

/*
 * Returns the effective fill-column
 */
int
getfillcol(BUFFER *bp)
{
    int result = b_val(bp, VAL_FILL);
    if (result == 0) {
	result = term.cols - b_val(bp, VAL_WRAPMARGIN);
    } else if (result < 0) {
	result = term.cols + result;
    }
    return (result);
}

/*
 * Release storage of a REGEXVAL struct
 */
REGEXVAL *
free_regexval(REGEXVAL * rp)
{
    if (rp != 0) {
	beginDisplay();
	FreeAndNull(rp->pat);
	FreeAndNull(rp->reg);
	free((char *) rp);
	endofDisplay();
    }
    return 0;
}

/*
 * Allocate/set a new REGEXVAL struct
 */
REGEXVAL *
new_regexval(const char *pattern, int magic)
{
    REGEXVAL *rp = 0;

    if (pattern != 0) {
	beginDisplay();
	if ((rp = typecalloc(REGEXVAL)) != 0) {
	    if ((rp->pat = strmalloc(pattern)) == 0
		|| (rp->reg = regcomp(rp->pat, strlen(rp->pat), magic)) == 0)
		rp = free_regexval(rp);
	}
	endofDisplay();
    }

    return rp;
}

/*
 * Release storage of a VAL struct
 */
static void
free_val(const struct VALNAMES *names, struct VAL *values)
{
    switch (names->type) {
    case VALTYPE_STRING:
	beginDisplay();
	FreeAndNull(values->v.p);
	endofDisplay();
	break;
    case VALTYPE_REGEX:
	values->v.r = free_regexval(values->v.r);
	break;
    default:			/* nothing to free */
	break;
    }
}

/*
 * Copy a VAL-struct, preserving the sense of local/global.
 */
static int
copy_val(struct VAL *dst, struct VAL *src)
{
    int local = isLocalVal(src);

    *dst = *src;
    if (local)
	makeLocalVal(dst);
    return local;
}

void
copy_mvals(int maximum,
	   struct VAL *dst,
	   struct VAL *src)
{
    int n;
    for (n = 0; n < maximum; n++)
	(void) copy_val(&dst[n], &src[n]);
}

/*
 * This is a special routine designed to save the values of local modes and to
 * restore them.  The 'recompute_buffer()' procedure assumes that global modes
 * do not change during the recomputation process (so there is no point in
 * trying to convert any of those values to local ones).
 */
#if OPT_UPBUFF
void
save_vals(int maximum,
	  struct VAL *gbl,
	  struct VAL *dst,
	  struct VAL *src)
{
    int n;
    for (n = 0; n < maximum; n++)
	if (copy_val(&dst[n], &src[n]))
	    make_global_val(src, gbl, n);
}
#endif

/*
 * free storage used by local mode-values, called only when we are freeing
 * all other storage associated with a buffer or window.
 */
void
free_local_vals(const struct VALNAMES *names,
		struct VAL *gbl,
		struct VAL *val)
{
    int j;

    for (j = 0; names[j].name != 0; j++) {
	if (is_local_val(val, j)) {
	    make_global_val(val, gbl, j);
	    free_val(names + j, val + j);
	}
    }
}

/*
 * Convert a string to boolean, checking for errors
 */
static int
string_to_bool(const char *base, int *np)
{
    if (is_truem(base))
	*np = TRUE;
    else if (is_falsem(base))
	*np = FALSE;
    else {
	mlforce("[Not a boolean: '%s']", base);
	return FALSE;
    }
    return TRUE;
}

/*
 * Convert a string to number, checking for errors
 */
int
string_to_number(const char *from, int *np)
{
    long n;
    char *p;

    /* accept decimal, octal, or hex */
    n = strtol(from, &p, 0);
    if (p == from || *p != EOS) {
	mlforce("[Not a number: '%s']", from);
	return FALSE;
    }
    *np = (int) n;
    return TRUE;
}

/*
 * Validate a 'glob' mode-value.  It is either a boolean, or it must be a
 * pipe-expression with exactly one "%s" embedded (no other % characters,
 * unless escaped).  That way, we can use the string to format the pipe
 * command.
 */
#if defined(GMD_GLOB) || defined(GVAL_GLOB)
static int
legal_glob_mode(const char *base)
{
#ifdef GVAL_GLOB		/* string */
    if (isShellOrPipe(base)) {
	const char *s = base;
	int count = 0;
	while (*s != EOS) {
	    if (*s == '%') {
		if (*++s != '%') {
		    if (*s == 's')
			count++;
		    else
			count = 2;
		}
	    }
	    if (*s != EOS)
		s++;
	}
	if (count == 1)
	    return TRUE;
    }
#endif
    if (!strcmp(base, "off")
	|| !strcmp(base, "on"))
	return TRUE;

    mlforce("[Illegal value for glob: '%s']", base);
    return FALSE;
}
#endif

/*
 * FSM stands for fixed string mode, so called because the strings which the
 * user is permitted to enter are non-arbitrary (fixed).
 *
 * It is meant to handle the following sorts of things:
 *
 *	:set popup-choices off
 *	:set popup-choices immediate
 *	:set popup-choices delayed
 *
 *	:set error quiet
 *	:set error beep
 *	:set error flash
 */
#if OPT_ENUM_MODES

#if VILE_NEVER
FSM_CHOICES fsm_error[] =
{
    {"beep", 1},
    {"flash", 2},
    {"quiet", 0},
    END_CHOICES
};
#endif

#if OPT_COLOR_CHOICES
static const char s_fcolor[] = "fcolor";
static const char s_bcolor[] = "bcolor";
static const char s_ccolor[] = "ccolor";
#endif

#if OPT_COLOR_SCHEMES
static const char s_default[] = "default";
static const char s_color_scheme[] = "color-scheme";
static const char s_palette[] = "palette";
static const char s_video_attrs[] = "video-attrs";
static const
FSM_CHOICES fsm_no_choices[] =
{
    {s_default, 0},
    END_CHOICES			/* ends table for name-completion */
};
#endif /* OPT_COLOR_SCHEMES */

static struct FSM fsm_tbl[] =
{
    {"*bool", fsm_bool_choices},
#if OPT_COLOR_SCHEMES
    {s_color_scheme, fsm_no_choices},
#endif
#if OPT_COLOR_CHOICES
    {s_fcolor, fsm_color_choices},
    {s_bcolor, fsm_color_choices},
    {s_ccolor, fsm_color_choices},
#endif
#if OPT_POPUP_CHOICES
    {"popup-choices", fsm_popup_choices},
#endif
#if VILE_NEVER
    {"error", fsm_error},
#endif
#if OPT_BACKUP_CHOICES
    {"backup-style", fsm_backup_choices},
#endif
#if OPT_FORBUFFERS_CHOICES
    {"for-buffers", fsm_forbuffers_choices},
#endif
#if OPT_HILITE_CHOICES
    {"mcolor", fsm_hilite_choices},
    {"visual-matches", fsm_hilite_choices},
    {"mini-hilite", fsm_hilite_choices},
#endif
#if OPT_COLOR
    {s_video_attrs, fsm_videoattrs_choices},
#endif
#if OPT_VTFLASHSEQ_CHOICES
    {"vtflash", fsm_vtflashseq_choices},
#endif
#if SYS_VMS
    {"record-format", fsm_recordformat_choices},
    {"record-attrs", fsm_recordattrs_choices},
#endif
#if OPT_READERPOLICY_CHOICES
    {"reader-policy", fsm_readerpolicy_choices},
#endif
#if OPT_RECORDSEP_CHOICES
    {"recordseparator", fsm_recordsep_choices},
#endif
#if OPT_SHOWFORMAT_CHOICES
    {"showformat", fsm_showformat_choices},
#endif
#if OPT_MMQUALIFIERS_CHOICES
    {"qualifiers", fsm_mmqualifiers_choices},
#endif
};

static int fsm_idx;

static size_t
fsm_size(const FSM_CHOICES * list)
{
    size_t result = 0;
    while ((list++)->choice_name != 0)
	result++;
    return result;
}

const FSM_CHOICES *
name_to_choices(const char *name)
{
    int i;
    const FSM_CHOICES *result = 0;

    fsm_idx = -1;
    for (i = 1; i < (int) TABLESIZE(fsm_tbl); i++) {
	if (strcmp(fsm_tbl[i].mode_name, name) == 0) {
	    fsm_idx = i;
	    result = fsm_tbl[i].choices;
	    break;
	}
    }
    return result;
}

static const FSM_CHOICES *
valname_to_choices(const struct VALNAMES *names)
{
    return name_to_choices(names->name);
}

static int
is_fsm(const struct VALNAMES *names)
{
    size_t i;

    if (names->type == VALTYPE_ENUM
	|| names->type == VALTYPE_STRING) {
	for (i = 1; i < TABLESIZE(fsm_tbl); i++) {
	    if (strcmp(fsm_tbl[i].mode_name, names->name) == 0) {
		fsm_idx = (int) i;
		return TRUE;
	    }
	}
    } else if (is_bool_type(names->type)) {
	fsm_idx = 0;
	return TRUE;
    }
    fsm_idx = -1;
    return FALSE;
}

/*
 * Test if we're processing an enum-valued mode.  If so, lookup the mode value.
 * We'll allow a numeric index also (e.g., for colors).  Note that we're
 * returning the table-value in that case, so we'll have to ensure that we
 * don't corrupt the table.
 */
static const char *
legal_fsm(const char *val)
{
    if (fsm_idx >= 0) {
	int i;
	int idx = fsm_idx;
	const FSM_CHOICES *p = fsm_tbl[idx].choices;
	const char *s;

	if (isDigit(*val)) {
	    if (!string_to_number(val, &i))
		return 0;
	    if ((s = choice_to_name(p, i)) != 0)
		return s;
	} else {
	    if (choice_to_code(p, val, strlen(val)) != ENUM_ILLEGAL)
		return val;
	}
	mlforce("[Illegal value for %s: '%s']",
		fsm_tbl[idx].mode_name,
		val);
	return 0;
    }
    return val;
}

int
fsm_complete(DONE_ARGS)
{
    int result = FALSE;

    if (buf != 0) {
	if (isDigit(*buf)) {	/* allow numbers for colors */
	    if (c != NAMEC)	/* put it back (cf: kbd_complete) */
		unkeystroke(c);
	    result = isSpace(c);
	} else {
	    result = kbd_complete(PASS_DONE_ARGS,
				  (const char *) (fsm_tbl[fsm_idx].choices),
				  sizeof(FSM_CHOICES));
	}
    }
    return result;
}
#endif /* OPT_ENUM_MODES */

/*
 * Lookup the mode named with 'cp[]' and adjust its value.
 */
int
adjvalueset(const char *cp,	/* name of the mode we are changing */
	    int defining,	/* for majormodes, suppress side_effect */
	    int setting,	/* true if setting, false if unsetting */
	    int global,
	    VALARGS * args)
{				/* symbol-table entry for the mode */
    const struct VALNAMES *names = args->names;
    struct VAL *values = args->local;
    struct VAL *globls = args->global;

    char prompt[NLINE];
    char respbuf[NFILEN];
    int no;
    const char *rp = NULL;
    int status;

    if (isEmpty(cp))
	return FALSE;

    no = noPrefix(cp);
    if (no && !is_bool_type(names->type))
	return FALSE;		/* this shouldn't happen */

    /*
     * Check if we're allowed to change this mode in the current context.
     */
    if (!defining
	&& (names->side_effect != 0)
	&& !(*(names->side_effect)) (curbp, args, (values == globls), TRUE)) {
	return FALSE;
    }

    /* get a value if we need one */
    if ((end_string() == '=')
	|| (!is_bool_type(names->type) && setting)) {
	int regex = (names->type == VALTYPE_REGEX);
	KBD_OPTIONS opts = regex ? 0 : KBD_NORMAL;
	int eolchar = is_str_type(names->type) ? '\n' : ' ';
	int (*complete) (DONE_ARGS) = no_completion;

	respbuf[0] = EOS;
	(void) lsprintf(prompt, "New %s %s: ",
			cp,
			regex ? "pattern" : "value");

#if OPT_ENUM_MODES
	if (is_fsm(names))
	    complete = fsm_complete;
#endif

	status = kbd_string(prompt, respbuf, sizeof(respbuf), eolchar,
			    opts, complete);
	if (status != TRUE)
	    return status;

	/*
	 * Allow an empty response to a string-value if we're running in a
	 * macro rather than interactively.
	 */
	if (!strlen(rp = respbuf)
	    && !clexec) {
	    switch (names->type) {
	    case VALTYPE_REGEX:	/* FALLTHRU */
	    case VALTYPE_STRING:
		break;
	    default:
		return FALSE;
	    }
	}
    } else if (!setting) {
	rp = is_str_type(names->type) ? "" : "0";
    }
#if OPT_HISTORY
    else
	hst_glue(' ');
#endif
    status = set_mode_value(curbp, cp, defining, setting, global, args, rp);
    TRACE(("...adjvalueset(%s)=%d\n", cp, status));

    return status;
}

int
set_mode_value(BUFFER *bp, const char *cp, int defining, int setting, int
	       global, VALARGS * args, const char *rp)
{
    const struct VALNAMES *names = args->names;
    struct VAL *values = args->local;
    struct VAL *globls = args->global;
    REGEXVAL *r;

    struct VAL oldvalue;
    int no = noPrefix(cp);
    int nval, status;
    int unsetting = !setting && !global;
    int changed = FALSE;

    /*
     * Check if we're allowed to change this mode in the current context.
     */
    if (!defining
	&& (names->side_effect != 0)
	&& !(*(names->side_effect)) (bp, args, (values == globls), TRUE)) {
	return FALSE;
    }

    if (rp == NULL) {
	rp = no ? cp + 2 : cp;
    } else {
	if (no && !is_bool_type(names->type))
	    return FALSE;	/* this shouldn't happen */

#if defined(GMD_GLOB) || defined(GVAL_GLOB)
	if (!strcmp(names->name, "glob")
	    && !legal_glob_mode(rp))
	    return FALSE;
#endif
#if OPT_ENUM_MODES
	(void) is_fsm(names);	/* evaluated for its side effects */
	if ((rp = legal_fsm(rp)) == 0)
	    return FALSE;
#endif
	/* Test after fsm, to allow translation */
	if (is_bool_type(names->type)) {
	    if (!string_to_bool(rp, &setting))
		return FALSE;
	}
    }

    /* save, to simplify no-change testing */
    (void) copy_val(&oldvalue, values);

    if (unsetting) {
	make_global_val(values, globls, 0);
#if OPT_MAJORMODE
	switch (names->type) {
	case VALTYPE_MAJOR:
	    if (values == globls)
		changed = enable_mmode(names->shortname, FALSE);
	    else
		changed = detach_mmode(bp, names->shortname);
	    break;
	}
#endif
    } else {
	makeLocalVal(values);	/* make sure we point to result! */

	/* we matched a name -- set the value */
	switch (names->type) {
#if OPT_MAJORMODE
	case VALTYPE_MAJOR:
	    values->vp->i = no ? !setting : setting;
	    if (values == globls) {
		changed = enable_mmode(names->shortname, values->vp->i);
	    } else {
		changed = no
		    ? detach_mmode(bp, names->shortname)
		    : attach_mmode(bp, names->shortname);
		if (changed)
		    did_attach_mmode = TRUE;
	    }
	    break;
#endif
	case VALTYPE_BOOL:
	    values->vp->i = no ? !setting : setting;
	    break;

	case VALTYPE_ENUM:
#if OPT_ENUM_MODES
	    {
		const FSM_CHOICES *fp = valname_to_choices(names);

		if (isDigit(*rp)) {
		    if (!string_to_number(rp, &nval))
			return FALSE;
		    if (choice_to_name(fp, nval) == 0)
			nval = ENUM_ILLEGAL;
		} else {
		    nval = choice_to_code(fp, rp, strlen(rp));
		}
		if (nval == ENUM_ILLEGAL) {
		    mlforce("[Not a legal enum-index: %s]",
			    rp);
		    return FALSE;
		}
	    }
	    values->vp->i = nval;
	    break;
#endif /* OPT_ENUM_MODES */

	case VALTYPE_INT:
	    if (!string_to_number(rp, &nval))
		return FALSE;
	    values->vp->i = nval;
	    break;

	case VALTYPE_STRING:
	    values->vp->p = strmalloc(rp);
	    break;

	case VALTYPE_REGEX:
	    if ((r = new_regexval(rp, TRUE)) == 0) {
		values->vp->r = new_regexval("", TRUE);
		return FALSE;
	    }
	    values->vp->r = r;
	    break;

	default:
	    mlforce("BUG: bad type %s %d", names->name, names->type);
	    return FALSE;
	}
    }

    /*
     * Set window flags (to force the redisplay as needed), and apply
     * side-effects.
     */
    status = TRUE;
    if (!same_val(names, values, &oldvalue)) {
	changed = TRUE;
    }

    if (!defining
	&& changed
	&& (names->side_effect != 0)
	&& !(*(names->side_effect)) (bp, args, (values == globls), FALSE)) {
	if (!same_val(names, values, &oldvalue)) {
	    free_val(names, values);
	}
	(void) copy_val(values, &oldvalue);
	mlforce("[Cannot set this value]");
	status = FALSE;
    }

    else if (isLocalVal(&oldvalue)
	     || (values == globls))
	free_val(names, &oldvalue);

    return status;
}

/* ARGSUSED */
int
listmodes(int f, int n GCC_UNUSED)
{
    WINDOW *wp = curwp;
    int s;

    TRACE((T_CALLED "listmodes(f=%d)\n", f));

    s = liststuff(SETTINGS_BufName, FALSE, makemodelist, f, (void *) wp);
    /* back to the buffer whose modes we just listed */
    if (swbuffer(wp->w_bufp))
	curwp = wp;
    returnCode(s);
}

/*
 * The 'mode_complete()' and 'mode_eol()' functions are invoked from
 * 'kbd_reply()' to setup the mode-name completion and query displays.
 */
static int
mode_complete(DONE_ARGS)
{
    init_my_mode_list();

    return kbd_complete(PASS_DONE_ARGS,
			(const char *) &my_mode_list[0], sizeof(my_mode_list[0]));
}

int
/*ARGSUSED*/
mode_eol(EOL_ARGS)
{
    (void) buffer;
    (void) cpos;

    return (c == ' ' || c == eolchar);
}

int
lookup_valnames(const char *rp, const struct VALNAMES *table)
{
    int j;

    if (rp != 0) {
	for (j = 0; table[j].name != 0; j++) {
	    if (!strcmp(rp, table[j].name)
		|| !strcmp(rp, table[j].shortname)) {
		return j;
	    }
	}
    }
    return -1;
}

static const char *
strip_no(const char *mode)
{
    return noPrefix(mode) ? (mode + 2) : mode;
}

#if OPT_MAJORMODE
/*
 * Find a submode value for the current majormode.
 */
int
find_submode(BUFFER *bp, const char *mode, int global, VALARGS * args)
{
    const char *rp = strip_no(mode);
    int j;

    /* major submodes (buffers) */
    if (my_majormodes != 0) {
	int k = 0;
	size_t n = strlen(rp);

	for (j = 0; my_majormodes[j].shortname; j++) {
	    MAJORMODE_LIST *p = my_majormodes + j;
	    struct VAL *my_vals = get_sm_vals(p->data);
	    size_t len = strlen(p->shortname);

	    if (n >= len
		&& !strncmp(rp, p->shortname, len)
		&& (k = lookup_valnames(rp + len + (rp[len] == '-'),
					b_valnames)) >= 0
		&& is_local_val(my_vals, k)) {
		TRACE(("...found submode %s\n", b_valnames[k].name));
		if (global == FALSE) {
		    if (valid_buffer(bp)
			&& (bp->majr == 0
			    || strcmp(bp->majr->shortname, p->shortname))) {
			TRACE(("...not applicable\n"));
			return FALSE;
		    }
		    args->names = b_valnames + k;
		    args->global = my_vals + k;
		    args->local = (valid_buffer(bp)
				   ? bp->b_values.bv + k
				   : (struct VAL *) 0);
		} else {
		    args->names = b_valnames + k;
		    args->global = my_vals + k;
		    args->local = args->global;
		}
		return TRUE;
	    }
	}
    }
    return FALSE;
}
#endif

/*
 * Check if the given mode exists for the given class.  If so, fill in its
 * data and return true.  Otherwise return false.  The 'args' data is filled
 * in unless there is no such mode class.
 */
int
find_mode_class(BUFFER *bp,
		const char *mode,
		int global,
		VALARGS * args,
		MODECLASS mode_class)
{
    const char *rp = strip_no(mode);
    int j;

    memset(args, 0, sizeof(*args));
    switch (mode_class) {
    default:
    case UNI_MODE:		/* universal modes */
	args->names = g_valnames;
	args->global = global_g_values.gv;
	args->local = ((global !=FALSE)
		       ? args->global
		       : (struct VAL *)0);
	break;
    case BUF_MODE:		/* buffer modes */
	args->names = b_valnames;
	args->global = global_b_values.bv;
	args->local = ((global == TRUE)
		       ? args->global
		       : (valid_buffer(bp)
			  ? bp->b_values.bv
			  : (struct VAL *)0));
	break;
    case WIN_MODE:		/* window modes */
	args->names = w_valnames;
	args->global = global_w_values.wv;
	args->local = ((global == TRUE)
		       ? args->global
		       : ((curwp != 0)
			  ? curwp->w_values.wv
			  : (struct VAL *)0));
	break;
#if OPT_MAJORMODE
    case MAJ_MODE:		/* major modes */
	args->names = major_valnames;
	args->global = major_g_vals;
	args->local = ((global == TRUE)
		       ? args->global
		       : (valid_buffer(bp)
			  ? major_l_vals
			  : (struct VAL *)0));
	break;
    case SUB_MODE:		/* major submodes (qualifiers) */
	if (my_majormodes != 0) {
	    size_t n = strlen(rp);

	    for (j = 0; my_majormodes[j].shortname; j++) {
		MAJORMODE_LIST *p = my_majormodes + j;
		size_t len = strlen(p->shortname);

		if (n >= len
		    && !strncmp(rp, p->shortname, len)
		    && (lookup_valnames(rp, p->qual)) >= 0) {
		    args->names = p->qual;
		    args->global = p->data->mm.mv;
		    args->local = ((global !=FALSE)
				   ? args->global
				   : (struct VAL *)0);
		    break;
		}
	    }
	}
	break;
#endif
    }
    if (args->names != 0
	&& args->local != 0) {
	if ((j = lookup_valnames(rp, args->names)) >= 0) {
	    args->names += j;
	    args->local += j;
	    args->global +=j;
	    TRACE(("...found class %d %s\n", mode_class, rp));
#if OPT_MAJORMODE
	    if (mode_class == MAJ_MODE) {
		const char *it = ((bp->majr != 0)
				  ? bp->majr->shortname
				  : "?");
		make_global_val(args->local, args->global, 0);
		if (global) {
		    MAJORMODE_LIST *ptr = lookup_mm_list(it);
		    args->local[0].v.i = (ptr != 0 && ptr->flag);
		} else {
		    char temp[NSTRING];
		    majorname(temp, it, TRUE);
		    make_local_val(args->local, 0);
		    args->local[0].v.i = !strcmp(temp, rp);
		}
	    }
#endif
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * Search the list of modes across all classes to find the first match, and
 * fill in the corresponding data, returning true.  If no match is found,
 * return false.
 */
int
find_mode(BUFFER *bp, const char *mode, int global, VALARGS * args)
{
    int mode_class;

    TRACE2(("find_mode(%s) %s\n", mode, global ? "global" : "local"));

    for (mode_class = 0; mode_class < END_MODE; mode_class++) {
	if (find_mode_class(bp, mode, global, args, (MODECLASS) mode_class))
	      return TRUE;
    }
#if OPT_MAJORMODE
    if (find_submode(bp, mode, global, args)) {
	return TRUE;
    }
#endif
    TRACE2(("...not found\n"));
    return FALSE;
}

/*
 * Process a single mode-setting
 */
static int
do_a_mode(int kind, int global)
{
    VALARGS args;
    int s;
    static TBUFF *cbuf;		/* buffer to receive mode name into */

    /* prompt the user and get an answer */
    tb_scopy(&cbuf, "");
    if ((s = kbd_reply(global ? "Global value: " : "Local value: ",
		       &cbuf,
		       mode_eol, '=', KBD_NORMAL, mode_complete)) != TRUE)
	  return ((s == FALSE) ? SORTOFTRUE : s);
    if (tb_length(cbuf) == 0)
	return FALSE;
    if (!strcmp(tb_values(cbuf), "all")) {
	hst_glue(' ');
	return listmodes(FALSE, 1);
    }

    if ((s = find_mode(curbp, tb_values(cbuf), global, &args)) != TRUE) {
#if OPT_EVAL
	if (!global
	    &&(s = find_mode(curbp, tb_values(cbuf), TRUE, &args)) == TRUE) {
	    mlforce("[Not a local mode: \"%s\"]", tb_values(cbuf));
	    return FALSE;
	}
	return set_state_variable(tb_values(cbuf), NULL);
#else
	mlforce("[Not a legal set option: \"%s\"]", tb_values(cbuf));
#endif
    } else if ((s = adjvalueset(tb_values(cbuf), FALSE, kind, global,
				&args)) != 0) {
	    if (s == TRUE)
		mlerase();	/* erase the junk */
	    return s;
	}

    return FALSE;
}

/*
 * Process the list of mode-settings.
 * If 'kind' is true, set otherwise delete.
 */
static int
adjustmode(int kind, int global)
{
    int s;
    int anything = 0;

    if (kind && global &&isreturn(end_string()))
	return listmodes(TRUE, 1);

    while (((s = do_a_mode(kind, global)) == TRUE) && (end_string() == ' '))
	  anything++;
    if ((s == SORTOFTRUE) && anything)	/* fix for trailing whitespace */
	return TRUE;

    /* if the settings are up, redisplay them */
    relist_settings();
    relist_majormodes();

    return s;
}

/*
 * Buffer-animation for [Settings]
 */
#if OPT_UPBUFF
static int
show_Settings(BUFFER *bp)
{
    b_clr_obsolete(bp);
    return listmodes(FALSE, 1);
}

static void
relist_settings(void)
{
    update_scratch(SETTINGS_BufName, show_Settings);
}
#endif /* OPT_UPBUFF */

/* prompt and set an editor mode */
/* ARGSUSED */
int
setlocmode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return adjustmode(TRUE, FALSE);
}

/* prompt and delete an editor mode */
/* ARGSUSED */
int
dellocmode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return adjustmode(FALSE, FALSE);
}

/* prompt and set a global editor mode */
/* ARGSUSED */
int
setglobmode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return adjustmode(TRUE, TRUE);
}

/* prompt and delete a global editor mode */
/* ARGSUSED */
int
delglobmode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return adjustmode(FALSE, TRUE);
}

/*
 * The following functions are invoked to carry out side effects of changing
 * modes.
 */
/*ARGSUSED*/
int
chgd_autobuf(BUFFER *bp GCC_UNUSED,
	     VALARGS * args GCC_UNUSED,
	     int glob_vals,
	     int testing GCC_UNUSED)
{
    if (glob_vals)
	sortlistbuffers();
    return TRUE;
}

/*ARGSUSED*/
int
chgd_buffer(BUFFER *bp GCC_UNUSED,
	    VALARGS * args GCC_UNUSED,
	    int glob_vals,
	    int testing GCC_UNUSED)
{
    if (!glob_vals) {		/* i.e., ":setl" */
	if (bp == 0)
	    return FALSE;
	b_clr_counted(bp);
	(void) bsizes(bp);
    }
    return TRUE;
}

int
chgd_charset(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	charinit();
    }
    return chgd_window(bp, args, glob_vals, testing);
}

#if OPT_COLOR
int
chgd_color(BUFFER *bp GCC_UNUSED, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	if (&args->local->vp->i == &gfcolor)
	    term.setfore(gfcolor);
	else if (&args->local->vp->i == &gbcolor)
	    term.setback(gbcolor);
	else if (&args->local->vp->i == &gccolor)
	    term.setccol(gccolor);
	set_winflags(glob_vals, WFHARD | WFCOLR);
	need_update = TRUE;
    }
    return TRUE;
}

#if OPT_ENUM_MODES || OPT_COLOR_SCHEMES
static void
set_fsm_choice(const char *name, const FSM_CHOICES * choices)
{
    size_t n;

    TRACE((T_CALLED "set_fsm_choices(%s)\n", name));
#if OPT_TRACE
    for (n = 0; choices[n].choice_name != 0; n++)
	TRACE(("   [%d] %s = %d (%#x)\n", n,
	       choices[n].choice_name,
	       choices[n].choice_code,
	       choices[n].choice_code));
#endif
    for (n = 0; n < TABLESIZE(fsm_tbl); n++) {
	if (!strcmp(name, fsm_tbl[n].mode_name)) {
	    fsm_tbl[n].choices = choices;
	    break;
	}
    }
    returnVoid();
}
#endif /* OPT_EVAL */

static int
is_white(int n)
{
    if (ncolors <= 8)
	return ((n & 7) == 7);
    return (n == NCOLORS - 1);
}

static int
reset_color(int n)
{
    int oldvalue = global_g_val(n);
    if (oldvalue > ncolors && !is_white(oldvalue)) {
	set_global_g_val(n, oldvalue % ncolors);
	TRACE(("reset_color(%scolor) from %d to %d\n",
	       (n == GVAL_FCOLOR) ? "f" : "b",
	       oldvalue,
	       global_g_val(n)));
	return TRUE;
    }
    return FALSE;
}

#if OPT_ENUM_MODES
static FSM_CHOICES *my_colors;
static FSM_CHOICES *my_hilite;

static int
choosable_color(const char *name, int n)
{
    if (ncolors > 2 && n < NCOLORS) {
	if (ncolors <= 8	/* filter out misleading names */
	    && (!strncmp(name, "bright", 6)
		|| !strncmp(name, "light", 5)
		|| !strcmp(name, "gray")
		|| !strcmp(name, "brown")))
	    return FALSE;
	return TRUE;
    }
    return (n == C_BLACK) || (n == NCOLORS - 1);
}
#endif

/*
 * Set the number of colors to a subset of that which is configured.  The main
 * use for this is to switch between 16-colors and 8-colors, though it should
 * work for setting any value up to the NCOLORS value.
 */
int
set_colors(int n)
{
    static int initialized;
#if OPT_ENUM_MODES
    const FSM_CHOICES *the_colors, *the_hilite;
    size_t s, d;
    int code;
#endif

    TRACE((T_CALLED "set_colors(%d)\n", n));

    if (n > NCOLORS || n < 2)
	returnCode(FALSE);
    if (!initialized)
	initialized = n;
    if (n > initialized)
	returnCode(FALSE);
    ncolors = n;

    if (reset_color(GVAL_FCOLOR)
	|| reset_color(GVAL_BCOLOR)) {
	set_winflags(TRUE, WFHARD | WFCOLR);
	need_update = TRUE;
	relist_settings();
    }
#if OPT_ENUM_MODES
    if (ncolors == NCOLORS) {
	the_colors = fsm_color_choices;
	the_hilite = fsm_hilite_choices;
    } else {

	beginDisplay();
	my_colors = typecallocn(FSM_CHOICES, fsm_size(fsm_color_choices));
	my_hilite = typecallocn(FSM_CHOICES, fsm_size(fsm_hilite_choices));
	endofDisplay();

	if (my_colors == 0
	    || my_hilite == 0) {
	    FreeIfNeeded(my_colors);
	    return FALSE;
	}

	the_colors = my_colors;
	the_hilite = my_hilite;
	for (s = d = 0; fsm_color_choices[s].choice_name != 0; s++) {
	    my_colors[d] = fsm_color_choices[s];
	    if (choosable_color(my_colors[d].choice_name, my_colors[d].choice_code)) {
		if (ncolors <= 8)
		    my_colors[d].choice_code %= 8;
		d++;
	    }
	}
	my_colors[d].choice_name = 0;

	for (s = d = 0; fsm_hilite_choices[s].choice_name != 0; s++) {
	    my_hilite[d] = fsm_hilite_choices[s];
	    code = my_hilite[d].choice_code;
	    if (code >= 0 && (code & VASPCOL) != 0) {
		if (ncolors > 2
		    && choosable_color(my_hilite[d].choice_name, VCOLORNUM(code))) {
		    my_hilite[d].choice_code = (VASPCOL | code);
		    d++;
		}
	    } else if (ncolors > 2 || (code & VACOLOR) == 0) {
		d++;
	    }
	}
	my_hilite[d].choice_name = 0;
    }
    set_fsm_choice(s_fcolor, the_colors);
    set_fsm_choice(s_bcolor, the_colors);
    set_fsm_choice(s_ccolor, the_colors);
    set_fsm_choice("visual-matches", the_hilite);
    set_fsm_choice("mini-hilite", the_hilite);
    relist_descolor();
#endif /* OPT_ENUM_MODES */
    returnCode(TRUE);
}
#endif /* OPT_COLOR */

#if OPT_SHOW_COLORS
int
is_color_code(int n)
{
#if OPT_COLOR_CHOICES
    int j;
    const FSM_CHOICES *the_colors;
    the_colors = name_to_choices(s_fcolor);
    for (j = 0; the_colors[j].choice_name != 0; j++) {
	if (n == the_colors[j].choice_code)
	    return TRUE;
    }
    return FALSE;
#else
    return (c >= C_BLACK && n < ncolors);
#endif
}

const char *
get_color_name(int n)
{
    static char temp[80];
#if OPT_COLOR_CHOICES
    int j;
    const FSM_CHOICES *the_colors;
    the_colors = name_to_choices(s_fcolor);
    for (j = 0; the_colors[j].choice_name != 0; j++) {
	if (n == the_colors[j].choice_code)
	    return the_colors[j].choice_name;
    }
#endif
    lsprintf(temp, "color #%d", n);
    return temp;
}
#endif

	/* Report mode that cannot be changed */
/*ARGSUSED*/
int
chgd_disabled(BUFFER *bp GCC_UNUSED,
	      VALARGS * args,
	      int glob_vals GCC_UNUSED,
	      int testing GCC_UNUSED)
{
    mlforce("[Cannot change \"%s\" ]", args->names->name);
    return FALSE;
}

	/* Change "fences" mode */
/*ARGSUSED*/
int
chgd_fences(BUFFER *bp GCC_UNUSED, VALARGS * args, int glob_vals GCC_UNUSED, int testing)
{
    if (!testing) {
	/* was even number of fence pairs specified? */
	char *value = args->local->v.p;
	size_t len = strlen(value);

	if (len & 1) {
	    value[len - 1] = EOS;
	    mlwrite("[Fence-pairs not in pairs:  truncating to \"%s\"",
		    value);
	    return FALSE;
	}
    }
    return TRUE;
}

	/* Change a "major" mode */
int
chgd_major(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    /* prevent major-mode changes for scratch-buffers */
    if (testing) {
	if (!glob_vals) {
	    if (b_is_scratch(bp))
		return chgd_disabled(bp, args, glob_vals, testing);
	}
    } else {
	set_winflags(glob_vals, WFMODE);
    }
    return TRUE;
}

	/* Change the "undoable" mode */
int
chgd_undoable(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (chgd_major(bp, args, glob_vals, testing)) {
	if (!testing
	    && !(args->local->v.i))
	    freeundostacks(bp, TRUE);
	return TRUE;
    }
    return FALSE;
}

	/* Change a major mode that affects the windows on the buffer */
int
chgd_major_w(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (testing) {
	if (!chgd_major(bp, args, glob_vals, testing))
	    return FALSE;
	return chgd_window(bp, args, glob_vals, testing);
    }

    set_winflags(glob_vals, WFHARD | WFMODE);
    return TRUE;
}

void
set_record_sep(BUFFER *bp, RECORD_SEP code)
{
    const char *recordsep = "\n";

    if (code == RS_AUTO)
	code = RS_DEFAULT;

    make_local_b_val(bp, MDDOS);
    make_local_b_val(bp, VAL_RECORD_SEP);
    set_b_val(bp, MDDOS, (code == RS_CRLF));
    set_b_val(bp, VAL_RECORD_SEP, code);
    b_clr_counted(bp);
    (void) bsizes(bp);
    relist_settings();
    updatelistbuffers();
    set_winflags(TRUE, WFMODE);

    switch (code) {
    case RS_AUTO:
	/* see above */
	break;
    case RS_LF:
	/* see above */
	break;
    case RS_CRLF:
	recordsep = "\r\n";
	break;
    case RS_CR:
	recordsep = "\r";
	break;
    }
    (bp)->b_recordsep_str = recordsep;
    (bp)->b_recordsep_len = (int) strlen(get_record_sep(bp));
}

	/* Change the record separator */
int
chgd_rs(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	if (bp == 0)
	    return FALSE;
	if (args->local->vp->i == RS_AUTO)
	    args->local->vp->i = RS_DEFAULT;
	set_record_sep(bp, (RECORD_SEP) args->local->vp->i);
    }

    set_winflags(TRUE, WFMODE);
    return chgd_major_w(bp, args, glob_vals, testing);
}

	/* Change something on the mode/status line */
/*ARGSUSED*/
int
chgd_status(BUFFER *bp GCC_UNUSED, VALARGS * args GCC_UNUSED, int glob_vals, int testing)
{
    if (!testing) {
	set_winflags(glob_vals, WFSTAT);
    }
    return TRUE;
}

#if OPT_TITLE
	/* Changed swap-title */
int
chgd_swaptitle(BUFFER *bp GCC_UNUSED,
	       VALARGS * args GCC_UNUSED,
	       int glob_vals GCC_UNUSED,
	       int testing)
{
    if (!testing)
	set_editor_title();
    return TRUE;
}
#endif

#if OPT_FINDPATH
/*
 * user changed find-cfg mode
 *
 * Strictly speaking, there is no reason to require that a chgd() function
 * track changes to find-cfg mode.  However, this routine does allow the
 * editor to flag incorrect syntax before a shell command is initiated.
 */
int
chgd_find_cfg(BUFFER *bp GCC_UNUSED,
	      VALARGS * args,
	      int glob_vals GCC_UNUSED,
	      int testing)
{
    int rc = TRUE;
    FINDCFG unused;

    if (!testing)
	rc = parse_findcfg_mode(&unused, args->global->vp->p);
    return (rc);
}
#endif

	/* Change a mode that affects the windows on the buffer */
/*ARGSUSED*/
int
chgd_window(BUFFER *bp GCC_UNUSED, VALARGS * args GCC_UNUSED, int glob_vals, int testing)
{
    if (!testing) {
	set_winflags(glob_vals, WFHARD);
    }
    return TRUE;
}

	/* if unique-buffers is turned on, make sure all buffers that
	   can have a valid fuid */
/*ARGSUSED*/
int
chgd_uniqbuf(BUFFER *bp GCC_UNUSED,
	     VALARGS * args GCC_UNUSED,
	     int glob_vals GCC_UNUSED,
	     int testing)
{
    if (!testing) {
	if (global_g_val(GMDUNIQ_BUFS)) {
	    FUID fuid;
	    BUFFER *bp2;

	    for_each_buffer(bp2) {
		if (bp2->b_fname != 0
		    && !isInternalName(bp2->b_fname)
		    && fileuid_get(bp2->b_fname, &fuid)) {
		    fileuid_set(bp2, &fuid);
		}
	    }
	}
    }
    return TRUE;
}

	/* Change the working mode */
#if OPT_WORKING
/*ARGSUSED*/
int
chgd_working(BUFFER *bp GCC_UNUSED,
	     VALARGS * args GCC_UNUSED,
	     int glob_vals,
	     int testing GCC_UNUSED)
{
    if (glob_vals)
	imworking(0);
    return TRUE;
}
#endif

	/* Change the xterm-mouse mode */
/*ARGSUSED*/
int
chgd_xterm(BUFFER *bp GCC_UNUSED,
	   VALARGS * args GCC_UNUSED,
	   int glob_vals GCC_UNUSED,
	   int testing GCC_UNUSED)
{
#if OPT_XTERM
    if (glob_vals && !testing) {
	int new_state = global_g_val(GMDXTERM_MOUSE);
	set_global_g_val(GMDXTERM_MOUSE, !new_state);
	term.kclose();
	set_global_g_val(GMDXTERM_MOUSE, new_state);
	term.kopen();
	vile_refresh(FALSE, 0);
    }
#endif
    return TRUE;
}

	/* Change the xterm-fkeys mode */
/*ARGSUSED*/
int
chgd_xtermkeys(BUFFER *bp GCC_UNUSED,
	       VALARGS * args GCC_UNUSED,
	       int glob_vals GCC_UNUSED,
	       int testing GCC_UNUSED)
{
#if DISP_TERMCAP || DISP_CURSES
    if (glob_vals && !testing) {
	tcap_init_fkeys();
    }
#endif
    return TRUE;
}

	/* Change the mouse mode */
#ifdef GMDMOUSE
/*ARGSUSED*/
int
chgd_mouse(BUFFER *bp, VALARGS * args GCC_UNUSED, int glob_vals, int testing GCC_UNUSED)
{
    if (glob_vals) {
	int new_state = global_g_val(GMDMOUSE);
	set_global_g_val(GMDMOUSE, TRUE);
	if (!new_state)
	    term.kclose();
	else
	    term.kopen();
	set_global_g_val(GMDMOUSE, new_state);
    }
    return TRUE;
}
#endif

	/* Change a mode that affects the search-string highlighting */
/*ARGSUSED*/
int
chgd_hilite(BUFFER *bp, VALARGS * args GCC_UNUSED, int glob_vals GCC_UNUSED, int testing)
{
    if (!testing) {
#if OPT_HILITEMATCH
	if (bp->b_highlight & HILITE_ON)
	    bp->b_highlight |= HILITE_DIRTY;
	if (bp == curbp)	/* FIXME: attrib_matches only does curbp */
	    attrib_matches();
#endif
#if OPT_COLOR
	set_winflags(glob_vals, WFHARD | WFCOLR);
	need_update = TRUE;
#endif
    }
    return TRUE;
}

#if SYS_WINNT
int
chgd_rcntfiles(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	int nfiles = args->global->v.i;

	if (nfiles < 0 || nfiles > MAX_RECENT_FILES)
	    return (FALSE);
    }
    return (TRUE);
}

int
chgd_rcntfldrs(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	int nfolders = args->global->v.i;

	if (nfolders < 0 || nfolders > MAX_RECENT_FLDRS)
	    return (FALSE);
    }
    return (TRUE);
}
#endif /* SYS_WINNT */

/*--------------------------------------------------------------------------*/

#if OPT_EVAL || OPT_MAJORMODE
/*
 * Test for mode-names that we'll not show in the variable name-completion.
 */
int
is_varmode(const char *name)
{
    return (!noPrefix(name)
	    && strcmp(name, "all"));
}

/*
 * Returns the current number of items in the list of modes
 */
static size_t
count_modes(void)
{
    init_my_mode_list();

    if (sizeof_my_mode_list < 0) {
	size_t n;

	if (my_mode_list != 0) {
	    for (n = 0; my_mode_list[n] != 0; n++)
		continue;
	} else {
	    n = 0;
	}
	sizeof_my_mode_list = (int) n;
    }
    return sizeof_my_mode_list;
}

/*
 * Returns true if the given pointer is in all_modes[].  We use this to check
 * whether a name is in the original const array or not, so we can free it.
 */
static int
in_all_modes(const char *name)
{
    unsigned n;
    for (n = 0; all_modes[n] != 0; n++) {
	if (all_modes[n] == name)
	    return TRUE;
    }
    return FALSE;
}

/*
 * Return a list of only the modes that can be set with ":setv", ignoring
 * artifacts such as "all".
 */
const char *const *
list_of_modes(void)
{
    if (my_varmodes == 0) {
	const char *const *s;
	const char **d;
	size_t n = count_modes();

	beginDisplay();
	my_varmodes = typeallocn(const char *, n + 1);
	endofDisplay();

	if (my_varmodes != 0 && my_mode_list != 0) {
	    for (s = my_mode_list, d = my_varmodes; (*d = *s) != 0; s++) {
		if (is_varmode(*d)) {
		    d++;
		}
	    }
	}
    }
    return my_varmodes;
}
#endif /* OPT_EVAL || OPT_MAJORMODE */

/*--------------------------------------------------------------------------*/

#if OPT_MAJORMODE
static int
is_identifier(const char *name)
{
    if (name != 0) {
	int first = TRUE;

	while (*name != EOS) {
	    if (first) {
		if (!isAlpha(*name))
		    return FALSE;
		first = FALSE;
	    } else if (!isident(*name))
		return FALSE;
	    name++;
	}
	return TRUE;
    }
    return FALSE;
}

static int
ok_submode(const char *name)
{
    /* like is_varmode, but allow "no" prefix */
    return strcmp(name, "all") != 0 ? TRUE : FALSE;
}

/* format the name of a majormode's qualifier */
static char *
per_major(char *dst, const char *majr, size_t code, int brief)
{
    if (brief) {
	if (!strcmp(m_valnames[code].shortname, "X")) {
	    *dst = EOS;
	} else {
	    (void) lsprintf(dst, "%s%s", majr, m_valnames[code].shortname);
	}
    } else {
	(void) lsprintf(dst, "%s-%s", majr, m_valnames[code].name);
    }
    return dst;
}

/* format the name of a majormode's submode */
static char *
per_submode(char *dst, const char *majr, int code, int brief)
{
    if (brief) {
	if (!strcmp(b_valnames[code].shortname, "X")) {
	    *dst = EOS;
	    return 0;
	}
	(void) lsprintf(dst, "%s%s", majr, b_valnames[code].shortname);
    } else {
	(void) lsprintf(dst, "%s-%s", majr, b_valnames[code].name);
    }
    return dst;
}

static char *TheMajor;

static const char *
ModeName(const char *name)
{
    if (TheMajor != 0) {
	static char *dst;

	beginDisplay();
	if (dst != 0)
	    free(dst);
	dst = typeallocn(char, strlen(TheMajor) + strlen(name) + 3);
	endofDisplay();

	if (dst != 0) {
	    (void) lsprintf(dst, "%s-%s", TheMajor, name);
	    return dst;
	}
    }
    return name;
}

/* format the name of a majormode */
static char *
majorname(char *dst, const char *majr, int flag)
{
    (void) lsprintf(dst, "%s%smode", flag ? "" : NO_PREFIX, majr);
    return dst;
}

/*
 * Returns the current number of items in the list of modes
 */
static size_t
count_majormodes(void)
{
    size_t n = 0;

    if (my_majormodes != 0) {
	for (n = 0; my_majormodes[n].shortname != 0; n++)
	    continue;
    }
    return n;
}

static int
get_mm_number(int n, int m)
{
    struct VAL *mv = my_majormodes[n].data->mm.mv;

    if (mv[m].vp->i != 0) {
	TRACE(("get_mm_number(%s) %d\n",
	       my_majormodes[n].shortname,
	       mv[m].vp->i));
	return mv[m].vp->i;
    }
    return 0;
}

/*
 * Returns the regular expression for the given indices, checking that the
 * pattern is non-null.
 */
static regexp *
get_mm_rexp(int n, int m)
{
    struct VAL *mv = my_majormodes[n].data->mm.mv;

    if (mv[m].vp != 0
	&& mv[m].vp->r != 0
	&& mv[m].vp->r->pat != 0
	&& mv[m].vp->r->pat[0] != 0
	&& mv[m].vp->r->reg != 0) {
	TRACE(("get_mm_rexp(%s) %s\n",
	       my_majormodes[n].shortname,
	       mv[m].vp->r->pat));
	return mv[m].vp->r->reg;
    }
    return 0;
}

static char *
get_mm_string(int n, int m)
{
    struct VAL *mv = my_majormodes[n].data->mm.mv;

    if (mv[m].vp->p != 0) {
	TRACE(("get_mm_string(%s) %s\n",
	       my_majormodes[n].shortname,
	       mv[m].vp->p));
	return mv[m].vp->p;
    }
    return 0;
}

static int
find_majormode_order(int mm)
{
    int found = -1;
    int n;

    if (majormodes_order != 0) {
	for (n = 0; majormodes_order[n] >= 0; n++) {
	    if (majormodes_order[n] == mm) {
		found = n;
		break;
	    }
	}
    }
    return found;
}

#if OPT_TRACE
static void
show_majormode_order(char *tag)
{
    static TBUFF *order;

    int n;
    if (majormodes_order != 0) {
	tb_scopy(&order, "order ");
	tb_sappend0(&order, tag);
	for (n = 0; majormodes_order[n] >= 0; n++) {
	    tb_sappend0(&order, " ");
	    tb_sappend0(&order, my_majormodes[majormodes_order[n]].shortname);
	}
	TRACE(("%s\n", tb_values(order)));
    }
}
#else
#define show_majormode_order(tag)	/* nothing */
#endif

static int
put_majormode_before(unsigned j, char *s)
{
    char *t;
    char *told = "~";		/* FIXME: majormode names are 7-bit ASCII */
    int k;
    int kk;
    int found = -1;

    if (majormodes_order != 0) {
	TRACE(("put_majormode_before(%d:%s, %s)\n",
	       j,
	       my_majormodes[majormodes_order[j]].shortname,
	       s));

	for (k = 0; (kk = majormodes_order[k]) >= 0; k++) {
	    t = my_majormodes[kk].shortname;
	    if (strcmp(t, s) <= 0
		&& (found < 0 || strcmp(told, t) < 0)) {
		found = k;
		told = t;
	    }
	}
	if (found >= 0 && found < (int) j) {
	    show_majormode_order("before");
	    kk = majormodes_order[j];
	    while (found < (int) j) {
		majormodes_order[j] =
		    majormodes_order[j - 1];
		j--;
	    }
	    majormodes_order[j] = kk;
	    show_majormode_order("after:");
	} else if (found < 0) {
	    TPRINTF(("cannot put %s before %s (not found)\n",
		     my_majormodes[majormodes_order[j]].shortname,
		     s));
	    TRACE(("...did not find %s\n", s));
	}
	TRACE(("->%d\n", j));
    }
    return j;
}

static int
put_majormode_after(unsigned j, char *s)
{
    char *t;
    char *told = "";
    int k;
    int kk;
    int found = -1;

    if (majormodes_order != 0) {
	TRACE(("put_majormode_after(%d:%s, %s)\n",
	       j,
	       my_majormodes[majormodes_order[j]].shortname,
	       s));

	for (k = (int) count_majormodes() - 1;
	     k >= 0 && (kk = majormodes_order[k]) >= 0;
	     k--) {
	    t = my_majormodes[kk].shortname;
	    if (strcmp(t, s) >= 0
		&& (found < 0 || strcmp(told, t) > 0)) {
		found = k;
		told = t;
	    }
	}
	if (found >= 0 && found > (int) j) {
	    show_majormode_order("before");
	    kk = majormodes_order[j];
	    while (found > (int) j) {
		majormodes_order[j] =
		    majormodes_order[j + 1];
		j++;
	    }
	    majormodes_order[found] = kk;
	    show_majormode_order("after:");
	} else if (found < 0) {
	    TPRINTF(("cannot put %s after %s (not found)\n",
		     my_majormodes[majormodes_order[j]].shortname,
		     s));
	    TRACE(("...did not find %s\n", s));
	}
	TRACE(("->%d\n", j));
    }
    return j;
}

/*
 * Make an ordered index into my_majormodes[], accounting for "before" and
 * "after" qualifiers.
 */
static void
compute_majormodes_order(void)
{
    char *s;
    size_t need = count_majormodes();
    size_t want;
    unsigned j;
    int jj;
    static size_t have;

    TRACE((T_CALLED "compute_majormodes_order(%d)\n", need));
    if (need) {
	want = (need + 1) * 2;

	beginDisplay();
	if (have >= want) {
	    /* EMPTY */ ;
	} else if (have) {
	    have = want;
	    majormodes_order = typereallocn(int, majormodes_order, have);
	} else {
	    have = want;
	    majormodes_order = typecallocn(int, have);
	}
	endofDisplay();

	if (majormodes_order != 0) {
	    /* set the default order */
	    for (j = 0; j < need; j++) {
		majormodes_order[j] = j;
	    }
	    majormodes_order[need] = -1;

	    /* handle special cases */
	    for (j = 0; j < need; j++) {
		jj = majormodes_order[j];
		if ((s = get_mm_string(jj, MVAL_BEFORE)) != 0
		    && *s != EOS) {
		    jj = put_majormode_before(j, s);
		    TRACE(("JUMP %d to %d\n", j, jj));
		    if (jj < (int) j)
			j = jj;
		}
	    }
	    for (j = 0; j < need; j++) {
		jj = majormodes_order[j];
		if ((s = get_mm_string(jj, MVAL_AFTER)) != 0
		    && *s != EOS) {
		    jj = put_majormode_after(j, s);
		    TRACE(("JUMP %d to %d\n", j, jj));
		    if (jj < (int) j)
			j = jj;
		}
	    }
	    show_majormode_order("final:");
	}
    }
    returnVoid();
}

/*
 * Search my_mode_list[] for the given name, using 'count' for the array size.
 * We don't use bsearch because we need to handle insertions into the list.
 * If we find it, and will_insert is true, return -1, otherwise the index.
 * If we do not find it, return the closest index if will_insert is true,
 * otherwise -1.
 */
static int
search_mode_list(const char *name, int count, int will_insert)
{
    int prev = -1;
    int next = 0;
    int lo = 0;
    int hi = count;
    int cmp;
    const char *test;

    if (my_mode_list != 0) {
	while (lo < hi) {
	    next = (lo + hi + 0) / 2;
	    if (next == prev) {	/* didn't find it - stop iterating */
		if (will_insert) {
		    next = (strcmp(name, my_mode_list[lo]) > 0
			    ? hi : lo);
		} else {
		    next = -1;
		}
		break;
	    }
	    prev = next;
	    test = ((next < count)
		    ? my_mode_list[next]
		    : (strcmp("\177", "\377") < 0
		       ? "\377"
		       : "\177"));
	    if ((cmp = strcmp(name, test)) == 0) {
		if (will_insert)
		    next = -1;
		break;
	    } else if (cmp > 0) {
		lo = next;
	    } else {
		hi = next;
	    }
	}
    } else {
	next = -1;
    }
    return next;
}

static int
found_per_submode(const char *majr, int code)
{
    char temp[NSTRING];

    init_my_mode_list();

    (void) per_submode(temp, majr, code, TRUE);
    return search_mode_list(temp, (int) count_modes(), FALSE) >= 0;
}

/*
 * Insert 'name' into 'my_mode_list[]', which has 'count' entries.
 */
static size_t
insert_per_major(size_t count, const char *name)
{
    if (name != 0 && *name != EOS && my_mode_list != 0) {
	size_t j, k;
	int found;

	TRACE(("insert_per_major %lu %s\n", (ULONG) count, name));
	if ((found = search_mode_list(name, (int) count, TRUE)) >= 0) {
	    char *newname = strmalloc(name);

	    if (newname != 0) {
		j = found;
		for (k = ++count; k != j; k--)
		    my_mode_list[k] = my_mode_list[k - 1];
		my_mode_list[j] = newname;
		sizeof_my_mode_list++;
	    } else {
		no_memory("insert_per_major");
	    }
	}
    }
    return count;
}

/*
 * Remove 'name' from 'my_mode_list[]', which has 'count' entries.
 */
static size_t
remove_per_major(size_t count, const char *name)
{
    if (name != 0 && my_mode_list != 0) {
	int found;
	size_t j, k;

	if ((found = search_mode_list(name, (int) count, FALSE)) >= 0) {
	    j = found;
	    if (my_mode_list != all_modes
		&& !in_all_modes(my_mode_list[j])) {
		beginDisplay();
		free(TYPECAST(char, my_mode_list[j]));
		endofDisplay();
	    }
	    count--;
	    for (k = j; k <= count; k++)
		my_mode_list[k] = my_mode_list[k + 1];
	    sizeof_my_mode_list--;
	}
    }
    return count;
}

/*
 * Lookup a majormode's data area, given its short name, e.g., "c" vs "cmode".
 * We store the majormodes in an array to simplify name completion, though this
 * complicates definition and removal.
 */
static MAJORMODE *
lookup_mm_data(const char *name)
{
    size_t n;
    if (my_majormodes != 0) {
	for (n = 0; my_majormodes[n].shortname != 0; n++) {
	    if (!strcmp(name, my_majormodes[n].shortname))
		return my_majormodes[n].data;
	}
    }
    return 0;
}

/*
 * Lookup a majormode's data area, given its short name, e.g., "c" vs "cmode".
 * We store the majormodes in an array to simplify name completion, though this
 * complicates definition and removal.
 */
static MAJORMODE_LIST *
lookup_mm_list(const char *name)
{
    size_t n;
    if (my_majormodes != 0) {
	for (n = 0; my_majormodes[n].shortname != 0; n++) {
	    if (!strcmp(name, my_majormodes[n].shortname))
		return my_majormodes + n;
	}
    }
    return 0;
}

/* Check if a majormode is predefined.  There are some things we don't want to
 * do to them (such as remove them).
 */
static int
predef_majormode(const char *name)
{
    size_t n;
    int status = FALSE;

    if (my_majormodes != 0) {
	for (n = 0; my_majormodes[n].shortname != 0; n++) {
	    if (!strcmp(name, my_majormodes[n].shortname)) {
		status = my_majormodes[n].init;
		break;
	    }
	}
    }
    return status;
}

static int
check_majormode_name(const char *name, int defining)
{
    int status = TRUE;
    if (defining && lookup_mm_data(name) != 0) {
	TRACE(("Mode '%s' already exists\n", name));
	status = SORTOFTRUE;
    } else if (!defining && lookup_mm_data(name) == 0) {
	TRACE(("Mode '%s' does not exist\n", name));
	status = SORTOFTRUE;
    }
    return status;
}

/* name-completion for "longname" */
int
major_complete(DONE_ARGS)
{
    return kbd_complete(PASS_DONE_ARGS,
			(const char *) &my_majormodes[0].longname,
			sizeof(my_majormodes[0]));
}

/* name-completion for "shortname" */
static int
short_major_complete(DONE_ARGS)
{
    return kbd_complete(PASS_DONE_ARGS,
			(const char *) &my_majormodes[0].shortname,
			sizeof(my_majormodes[0]));
}

static int
prompt_majormode(char **result, int defining)
{
    static TBUFF *cbuf;		/* buffer to receive mode name into */
    int status;

    /* prompt the user and get an answer */
    tb_scopy(&cbuf, "");
    if ((status = kbd_reply("majormode: ",
			    &cbuf,
			    eol_history, ' ',
			    KBD_NORMAL,		/* FIXME: KBD_MAYBEC if !defining */
			    (defining || clexec)
			    ? no_completion
			    : short_major_complete)) == TRUE) {
	/* check for legal name (alphanumeric) */
	if ((status = is_identifier(tb_values(cbuf))) != TRUE) {
	    mlwarn("[Not an identifier: %s]", tb_values(cbuf));
	    return status;
	}
	if ((status = is_varmode(tb_values(cbuf))) == TRUE) {
	    *result = tb_values(cbuf);
	    status = check_majormode_name(*result, defining);
	    if (status != TRUE && status != SORTOFTRUE)
		mlwarn("[No such majormode: %s]", *result);
	    return status;
	}
    }
    if (status != FALSE)
	mlwarn("[Illegal name: %s]", tb_values(cbuf));
    return status;
}

static int
submode_complete(DONE_ARGS)
{
    return kbd_complete(PASS_DONE_ARGS, (const char *) &all_submodes[0],
			sizeof(all_submodes[0]));
}

/*
 * Set the submode values to a known state
 */
static void
init_sm_vals(struct VAL *dst)
{
    int k;
    for (k = 0; k < NUM_B_VALUES; k++) {
	make_global_val(dst, global_b_values.bv, k);
    }
}

/*
 * Free data allocated in get_sm_vals().
 */
static void
free_sm_vals(MAJORMODE * ptr)
{
    MINORMODE *p;

    while (ptr->sm != 0) {
	p = ptr->sm;
	ptr->sm = p->sm_next;
	free_local_vals(b_valnames, global_b_values.bv, p->sm_vals.bv);

	beginDisplay();
	free(p->sm_name);
	free(p);
	endofDisplay();
    }
}

/*
 * Help initialization of submode groups by copying mode values from the
 * base, e.g., ignorecase for majormodes that need this in their fence patterns.
 *
 * We do not copy the regular expressions since those are what submode groups
 * are designed for, and do not want to have leftover patterns from the
 * base contaminating the fence-fi, fence-else, etc.
 */
static void
copy_sm_vals(struct VAL *dst, struct VAL *src, int type)
{
    int n;

    if (src != 0) {
	int last = NUM_B_VALUES;
	for (n = 0; n < last; n++) {
	    if (b_valnames[n].type == type
		&& is_local_val(src, n)) {
		dst[n] = src[n];
		make_local_val(dst, n);
	    }
	}
    }
}

/*
 * Using the currently specified 'group' qualifier, lookup the corresponding
 * MINORMODE structure and return a pointer to the B_VALUES VALS data.  If
 * no structure is found, create one.
 *
 * We maintain the list of MINORMODE structures in the order they are defined.
 * This has the desirable side-effect of returning immediately for the default
 * "" group.
 */
static struct VAL *
get_sm_vals(MAJORMODE * ptr)
{
    struct VAL *result = 0;
    MINORMODE *p, *q;
    char *name = ptr->mq.qv[QVAL_GROUP].vp->p;

    for (p = ptr->sm, q = 0; p != 0; q = p, p = p->sm_next) {
	if (!strcmp(name, p->sm_name)) {
	    break;
	}
    }

    if (p == 0) {

	beginDisplay();
	if ((p = typecalloc(MINORMODE)) != 0) {
	    if ((p->sm_name = strmalloc(name)) == 0) {
		free(p);
		p = 0;
	    }
	}
	endofDisplay();

	if (p != 0) {
	    init_sm_vals(&(p->sm_vals.bv[0]));
	    if (ptr->sm != 0) {
		copy_sm_vals(&(p->sm_vals.bv[0]),
			     &(ptr->sm->sm_vals.bv[0]),
			     VALTYPE_BOOL);
		copy_sm_vals(&(p->sm_vals.bv[0]),
			     &(ptr->sm->sm_vals.bv[0]),
			     VALTYPE_INT);
		copy_sm_vals(&(p->sm_vals.bv[0]),
			     &(ptr->sm->sm_vals.bv[0]),
			     VALTYPE_ENUM);
	    }
	    if (q != 0)
		q->sm_next = p;
	    else
		ptr->sm = p;
	}
    }
    if (p != 0) {
	TRACE2(("...get_sm_vals(%s:%s)\n", ptr->shortname, p->sm_name));
	result = &(p->sm_vals.bv[0]);
    }
    return result;
}

/*
 * Returns the name of the n'th submode group
 */
char *
get_submode_name(BUFFER *bp, int n)
{
    char *result = "";
    MAJORMODE *data;

    if (n == 0) {
	result = "";
    } else if ((data = bp->majr) != 0) {
	MINORMODE *sm = data->sm;
	while (n-- > 0) {
	    if ((sm = sm->sm_next) == 0) {
		break;
	    } else if (n == 0) {
		result = sm->sm_name;
		break;
	    }
	}
    }
    return (result);
}

/*
 * Fetch a pointer to the n'th group of submode values.  The 0th group consists
 * of the normal buffer values.  Other values are defined in the majormode.
 *
 * FIXME: shouldn't I have a local submode value for the data which is defined
 * in a group?
 */
struct VAL *
get_submode_vals(BUFFER *bp, int n)
{
    struct VAL *result = 0;
    MAJORMODE *data;

    if (n == 0) {
	result = bp->b_values.bv;
    } else if ((data = bp->majr) != 0) {
	MINORMODE *sm = data->sm;
	while (n-- > 0) {
	    if ((sm = sm->sm_next) == 0) {
		break;
	    } else if (n == 0) {
		result = sm->sm_vals.bv;
		break;
	    }
	}
    }
    return (result);
}

struct VAL *
get_submode_valx(BUFFER *bp, int n, int *m)
{
    struct VAL *result = 0;
    int next = *m + 1;

    if (next != n) {
	if ((result = get_submode_vals(bp, next)) == 0) {
	    next = 0;
	    if (next != n) {
		result = get_submode_vals(bp, next);
	    }
	}

    }
    *m = next;
    return (result);
}

/*
 * Look in the submode's list of qualifiers, e.g., for "group" or "name".
 */
static int
ok_subqual(MAJORMODE * ptr, char *name)
{
    VALARGS args;
    int j;

    if ((j = lookup_valnames(name, q_valnames)) >= 0) {
	args.names = q_valnames;
	args.local = ptr->mq.qv;
	args.global = global_m_values.mv;
    } else {
	return FALSE;
    }

    args.names += j;
    args.global +=j;
    args.local += j;

    return adjvalueset(name, TRUE, TRUE, FALSE, &args);
}

static int
prompt_submode(MAJORMODE * ptr, char **result, int defining)
{
    static TBUFF *cbuf;		/* buffer to receive mode name into */
    const char *rp;
    int status;

    /* prompt the user and get an answer */
    tb_scopy(&cbuf, "");
    *result = 0;
    while ((status = kbd_reply("submode: ",
			       &cbuf,
			       eol_history, '=',
			       KBD_NORMAL,
			       submode_complete)) == TRUE) {
	if ((status = ok_subqual(ptr, tb_values(cbuf))) == TRUE) {
	    continue;
	} else if ((status = ok_submode(tb_values(cbuf))) == TRUE) {
	    *result = tb_values(cbuf);
	    rp = strip_no(*result);
	    status = check_majormode_name(rp, defining);
	}
	break;
    }
    if (status != TRUE
	&& status != SORTOFTRUE)
	mlwarn("[Illegal submode name: %s]", tb_values(cbuf));
    return status;
}

/*
 * Attach a buffer to the given majormode.  Adjust all of the non-local buffer
 * modes to point to the majormode's values where those in turn are local.
 */
static int
attach_mmode(BUFFER *bp, const char *name)
{
    int n;
    VALARGS args;

    if (valid_buffer(bp)) {
	if (bp->majr != 0
	    && strcmp(bp->majr->shortname, name) != 0)
	    (void) detach_mmode(bp, bp->majr->shortname);

	TRACE(("attach_mmode '%s' to '%s'\n", name, bp->b_bname));
	if ((bp->majr = lookup_mm_data(name)) != 0) {
	    struct VAL *mm = get_sm_vals(bp->majr);

	    /* adjust buffer modes */
	    for (n = 0; n < NUM_B_VALUES; n++) {
		if (!is_local_b_val(bp, n)
		    && is_local_val(mm, n)) {
		    make_global_val(bp->b_values.bv, mm, n);
		    if (b_valnames[n].side_effect != 0) {
			args.names = &(b_valnames[n]);
			args.local = &(bp->b_values.bv[n]);
			args.global = &mm[n];
			b_valnames[n].side_effect(bp,
						  &args,
						  TRUE,
						  FALSE);
		    }
		} else if (n == MDDOS
			   && is_local_val(mm, n)
			   && !b_val(bp, n)) {
		    /*
		     * This is a special case - we need a
		     * way to force vile to go back and
		     * strip the ^M's from the end of each
		     * line when reading a .bat file on
		     * a Unix system.
		     */
		    set_rs_crlf(0, 1);
		}
	    }
	    return TRUE;
	}
	return (bp->majr != 0);
    }

    return FALSE;
}

/*
 * Detach a buffer from the given majormode.  Modify the buffer's minor modes
 * to point to global modes where they've been pointed to the majormode's data.
 */
static int
detach_mmode(BUFFER *bp, const char *name)
{
    size_t n;
    MAJORMODE *mp;

    if (valid_buffer(bp)
	&& (mp = bp->majr) != 0
	&& !strcmp(mp->shortname, name)) {
	TRACE(("detach_mmode '%s', given '%s'\n", name, mp->shortname));
	/* readjust the buffer's modes */
	for (n = 0; n < NUM_B_VALUES; n++) {
	    if (!is_local_b_val(bp, n)
		&& is_local_val(get_sm_vals(mp), n)) {
		make_global_b_val(bp, n);
	    }
	}
	relist_settings();
	bp->majr = 0;
	return TRUE;
    }

    return FALSE;
}

static int
enable_mmode(const char *name, int flag)
{
    MAJORMODE_LIST *ptr = lookup_mm_list(name);
    if (ptr != 0
	&& ptr->flag != flag) {
	ptr->flag = flag;
	return TRUE;
    }
    return FALSE;
}

static int
free_majormode(const char *name)
{
    MAJORMODE *ptr = lookup_mm_data(name);
    size_t j, k;
    int n;
    char temp[NSTRING];
    BUFFER *bp;

    if (ptr != 0) {
	int init = TRUE;
	for (j = 0; my_majormodes[j].shortname != 0; j++) {
	    if (my_majormodes[j].data == ptr) {
		init = my_majormodes[j].init;
		for_each_buffer(bp) {
		    if (detach_mmode(bp, my_majormodes[j].shortname)) {
			set_winflags(TRUE, WFHARD | WFMODE);
		    }
		}
		free_local_vals(m_valnames, major_g_vals, ptr->mm.mv);
		free_local_vals(b_valnames, global_b_values.bv, get_sm_vals(ptr));

		beginDisplay();
		for (k = 0; k < MAX_M_VALUES; k++) {
		    free_val(m_valnames + k, my_majormodes[j].data->mm.mv + k);
		    free(TYPECAST(char, my_majormodes[j].qual[k].name));
		    free(TYPECAST(char, my_majormodes[j].qual[k].shortname));
		}
		for (k = 0; k < MAX_Q_VALUES; k++) {
		    free_val(q_valnames + k, my_majormodes[j].data->mq.qv + k);
		    free(TYPECAST(char, my_majormodes[j].subq[k].name));
		    free(TYPECAST(char, my_majormodes[j].subq[k].shortname));
		}
		free_sm_vals(ptr);
		free(ptr->shortname);
		free(ptr->longname);
		free(ptr);
		endofDisplay();

		do {
		    my_majormodes[j] = my_majormodes[j + 1];
		} while (my_majormodes[j++].shortname != 0);
		break;
	    }
	}
	if (my_mode_list != all_modes && !init) {
	    j = count_modes();
	    j = remove_per_major(j, majorname(temp, name, FALSE));
	    j = remove_per_major(j, majorname(temp, name, TRUE));
	    for (n = 0; n < MAX_M_VALUES; n++) {
		j = remove_per_major(j,
				     per_major(temp, name, n, TRUE));
		j = remove_per_major(j,
				     per_major(temp, name, n, FALSE));
	    }
	}
	if (major_valnames != 0) {
	    for (n = 0; major_valnames[n].name != 0; n++) {
		if (!strcmp(name, major_valnames[n].shortname)) {
		    beginDisplay();
		    free(TYPECAST(char, major_valnames[n].name));
		    free(TYPECAST(char, major_valnames[n].shortname));
		    endofDisplay();
		    while (major_valnames[n].name != 0) {
			major_valnames[n] = major_valnames[n + 1];
			n++;
		    }
		    break;
		}
	    }
	}
	compute_majormodes_order();
	return TRUE;
    }
    return FALSE;
}

static void
init_my_mode_list(void)
{
    if (my_mode_list == 0) {
	my_mode_list = TYPECAST(const char *, all_modes);
	sizeof_my_mode_list = -1;
    }
}

static size_t
extend_mode_list(size_t increment)
{
    size_t j, k;

    beginDisplay();

    j = count_modes();
    k = increment + j + 1;

    TRACE(("extend_mode_list from %d by %d\n", j, increment));

    if (my_mode_list == all_modes) {
	my_mode_list = typeallocn(const char *, k);
	memcpy(TYPECAST(char *, my_mode_list), all_modes, (j + 1) * sizeof(*my_mode_list));
    } else {
	my_mode_list = typereallocn(const char *, TYPECAST(char *,
							   my_mode_list), k);
    }
    endofDisplay();

    return j;
}

static struct VAL *
extend_VAL_array(struct VAL *ptr, size_t item, size_t len)
{
    size_t j, k;

    TRACE(("extend_VAL_array %p item %ld of %ld\n", ptr, (long) item, (long) len));

    if (ptr == 0) {
	beginDisplay();
	ptr = typeallocn(struct VAL, len + 1);
	endofDisplay();
    } else {
	beginDisplay();
	ptr = typereallocn(struct VAL, ptr, len + 1);
	endofDisplay();

	if (ptr != 0) {
	    for (j = k = 0; j < len; j++) {
		k = (j >= item) ? j + 1 : j;
		ptr[k] = ptr[j];
		make_local_val(ptr, k);
	    }
	}
    }
    if (ptr != 0) {
	make_local_val(ptr, item);
	make_local_val(ptr, len);
	ptr[item].v.i = FALSE;
    }
    return ptr;
}

static void
set_qualifier(const struct VALNAMES *names,
	      struct VAL *values,
	      const char *s,
	      int n)
{
    free_val(names, values);
    switch (names->type) {
    case VALTYPE_BOOL:
    case VALTYPE_ENUM:
    case VALTYPE_INT:
	values->v.i = n;
	break;
    case VALTYPE_STRING:
	values->v.p = strmalloc(s);
	break;
    case VALTYPE_REGEX:
	values->v.r = new_regexval(s, TRUE);
	break;
    }
    make_local_val(values, 0);
}

static void
reset_qualifier(const struct VALNAMES *names, struct VAL *values)
{
    set_qualifier(names, values, "", 0);
}

/*
 * Reset/initialize all of the qualifiers.  We will use any that become set as
 * parameters for the current submode operation.
 */
static void
reset_sm_qualifiers(MAJORMODE * ptr)
{
    int k;
    for (k = 0; k < MAX_Q_VALUES; k++)
	reset_qualifier(q_valnames + k, ptr->mq.qv + k);
}

/*
 * Buffer-animation for [Major Modes]
 */
#if OPT_UPBUFF
static int
show_majormodes(BUFFER *bp)
{
    b_clr_obsolete(bp);
    return list_majormodes(FALSE, 1);
}

static void
relist_majormodes(void)
{
    update_scratch(MAJORMODES_BufName, show_majormodes);
}
#endif /* OPT_UPBUFF */

static int
show_active_majormodes(int nflag)
{
    int n;
    for (n = 0; major_valnames[n].name != 0; n++) {
	make_local_val(major_g_vals, n);
	major_g_vals[n].vp->i = my_majormodes[n].flag;
    }
    return listvalueset("Majormodes", nflag, FALSE, major_valnames,
			major_g_vals, (struct VAL *) 0);
}

/* list the current modes into the current buffer */
/* ARGSUSED */
static void
makemajorlist(int local, void *ptr GCC_UNUSED)
{
    int j;
    int nflag;
    MAJORMODE *data;
    MINORMODE *vals;

    if (my_majormodes != 0) {
	unsigned count = (unsigned) count_majormodes();
	if (show_active_majormodes(0))
	    bputc('\n');
	for (j = 0; my_majormodes[j].shortname != 0; j++) {
	    if (local)
		TheMajor = my_majormodes[j].shortname;
	    data = my_majormodes[j].data;

	    bprintf("--- \"%s\" majormode settings ",
		    my_majormodes[j].shortname);
	    bpadc('-', term.cols - 12 - DOT.o);
	    bprintf("(%d:%u)",
		    find_majormode_order(j) + 1,
		    count);
	    bpadc('-', term.cols - DOT.o);
	    bputc('\n');

	    nflag = listvalueset("Qualifier", FALSE, TRUE,
				 m_valnames,
				 data->mm.mv,
				 data->mm.mv);
	    for (vals = data->sm; vals != 0; vals = vals->sm_next) {
		char *group = vals->sm_name;
		char *name;

		beginDisplay();
		name = typeallocn(char, 80 + strlen(group));
		endofDisplay();

		if (name != 0) {
		    if (*group)
			lsprintf(name, "Buffer (\"%s\" group)", group);
		    else
			strcpy(name, "Buffer");
		    nflag = listvalueset(name, nflag, TRUE,
					 b_valnames,
					 vals->sm_vals.bv,
					 global_b_values.bv);
		    beginDisplay();
		    free(name);
		    endofDisplay();
		}
	    }
	    if (my_majormodes[j + 1].data)
		bputc('\n');
	}
    }
    TheMajor = 0;
}

/* ARGSUSED */
int
list_majormodes(int f, int n GCC_UNUSED)
{
    WINDOW *wp = curwp;
    int s;

    TRACE((T_CALLED "list_majormodes(f=%d)\n", f));

    s = liststuff(MAJORMODES_BufName, FALSE, makemajorlist, f, (void *) wp);
    /* back to the buffer whose modes we just listed */
    if (swbuffer(wp->w_bufp))
	curwp = wp;

    returnCode(s);
}

int
alloc_mode(const char *shortname, int predef)
{
    size_t j = 0, k;
    int n;
    int status = TRUE;
    char longname[NSTRING];
    char temp[NSTRING];
    static struct VAL *new_vals;

    if (major_valnames == 0) {
	beginDisplay();
	major_valnames = typecallocn(struct VALNAMES, 2);
	endofDisplay();
	j = 0;
	k = 1;
    } else {
	k = count_majormodes();
	beginDisplay();
	major_valnames = typereallocn(struct VALNAMES, major_valnames, k + 2);
	endofDisplay();
	if (major_valnames != 0) {
	    for (j = k++; j != 0; j--) {
		major_valnames[j] = major_valnames[j - 1];
		if (strcmp(major_valnames[j - 1].shortname, shortname) < 0) {
		    break;
		}
	    }
	}
    }

    if (major_valnames == 0)
	return FALSE;

    (void) majorname(longname, shortname, TRUE);
    major_valnames[j].name = strmalloc(longname);
    major_valnames[j].shortname = strmalloc(shortname);
    major_valnames[j].type = VALTYPE_MAJOR;
    major_valnames[j].side_effect = chgd_major_w;

    if (major_valnames[j].name == 0
	|| major_valnames[j].shortname == 0) {
	do {
	    major_valnames[j] = major_valnames[j + 1];
	    ++j;
	} while (major_valnames[j].name != 0);
	return FALSE;
    }

    memset(major_valnames + k, 0, sizeof(*major_valnames));

    /* build arrays needed for 'find_mode()' bookkeeping */
    if ((new_vals = extend_VAL_array(major_g_vals, j, k)) == 0)
	return FALSE;
    major_g_vals = new_vals;

    if ((new_vals = extend_VAL_array(major_l_vals, j, k)) == 0)
	return FALSE;
    major_l_vals = new_vals;

    if (my_majormodes == 0) {
	beginDisplay();
	my_majormodes = typecallocn(MAJORMODE_LIST, 2);
	endofDisplay();
	j = 0;
	k = 1;
    } else {
	k = count_majormodes();
	beginDisplay();
	my_majormodes = typereallocn(MAJORMODE_LIST, my_majormodes, k + 2);
	endofDisplay();
	for (j = k++; j != 0; j--) {
	    my_majormodes[j] = my_majormodes[j - 1];
	    if (strcmp(my_majormodes[j - 1].shortname, shortname) < 0) {
		break;
	    }
	}
    }

    if (my_majormodes == 0)
	return FALSE;

    /* allocate the names for the major mode */
    beginDisplay();

    my_majormodes[j].shortname = strmalloc(shortname);
    my_majormodes[j].longname = strmalloc(longname);

    my_majormodes[j].init = predef;
    my_majormodes[j].flag = TRUE;

    if ((my_majormodes[j].data = typecalloc(MAJORMODE)) != 0) {
	my_majormodes[j].data->shortname = my_majormodes[j].shortname;
	my_majormodes[j].data->longname = my_majormodes[j].longname;

	memset(my_majormodes + k, 0, sizeof(*my_majormodes));
    }

    endofDisplay();

    if (my_majormodes[j].shortname == 0
	|| my_majormodes[j].longname == 0
	|| my_majormodes[j].data == 0) {
	beginDisplay();
	FreeIfNeeded(my_majormodes[j].data);
	FreeIfNeeded(my_majormodes[j].longname);
	endofDisplay();
	do {
	    my_majormodes[j] = my_majormodes[j + 1];
	    ++j;
	} while (my_majormodes[j].shortname != 0);
	return FALSE;
    }

    /* copy array to get types, then overwrite the name-pointers */
    memcpy(my_majormodes[j].subq, q_valnames, sizeof(q_valnames));
    for (k = 0; k < MAX_Q_VALUES; k++) {
	reset_qualifier(q_valnames + k, my_majormodes[j].data->mq.qv + k);

	my_majormodes[j].subq[k].name =
	    strmalloc(q_valnames[k].name);
	my_majormodes[j].subq[k].shortname =
	    strmalloc(q_valnames[k].shortname);

	if (my_majormodes[j].subq[k].name == 0
	    || my_majormodes[j].subq[k].shortname == 0) {
	    free_majormode(shortname);
	    return FALSE;
	}
    }

    init_sm_vals(get_sm_vals(my_majormodes[j].data));

    /* copy array to get types, then overwrite the name-pointers */
    memcpy(my_majormodes[j].qual, m_valnames, sizeof(m_valnames));
    for (k = 0; k < MAX_M_VALUES; k++) {
	reset_qualifier(m_valnames + k, my_majormodes[j].data->mm.mv + k);

	my_majormodes[j].qual[k].name =
	    strmalloc(per_major(temp, shortname, k, TRUE));
	my_majormodes[j].qual[k].shortname =
	    strmalloc(per_major(temp, shortname, k, FALSE));

	if (my_majormodes[j].qual[k].name == 0
	    || my_majormodes[j].qual[k].shortname == 0) {
	    free_majormode(shortname);
	    return FALSE;
	}
    }

    /*
     * Create the majormode-specific names.  If this is predefined, we
     * already had mktbls do this.
     */
    if (!predef) {
	j = extend_mode_list((MAX_M_VALUES + 2) * 2);
	j = insert_per_major(j, majorname(longname, shortname, FALSE));
	j = insert_per_major(j, majorname(longname, shortname, TRUE));
	for (n = 0; n < MAX_M_VALUES; n++) {
	    j = insert_per_major(j, per_major(temp, shortname, n, TRUE));
	    j = insert_per_major(j, per_major(temp, shortname, n, FALSE));
	}
    }

    compute_majormodes_order();
    return status;
}

/* ARGSUSED */
int
define_mode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    char *name;
    int status;

    if ((status = prompt_majormode(&name, TRUE)) == TRUE) {
	TRACE(("define majormode:%s\n", name));
	status = alloc_mode(name, FALSE);
	relist_settings();
	relist_majormodes();
    } else if (status == SORTOFTRUE) {
	status = TRUE;		/* don't complain if it's already true */
    }
    return status;
}

static int
do_a_submode(int defining)
{
    char *name;
    char *subname;
    int status;
    MAJORMODE *ptr;
    VALARGS args;
    int j;
    size_t k;
    int qualifier = FALSE;
    char temp[NSTRING];
    const char *rp;

    if ((status = prompt_majormode(&name, FALSE)) != TRUE)
	return status;

    ptr = lookup_mm_data(name);
    reset_sm_qualifiers(ptr);

    if ((status = prompt_submode(ptr, &subname, TRUE)) != TRUE) {
	reset_sm_qualifiers(ptr);
	return status;
    }

    rp = strip_no(subname);
    if ((j = lookup_valnames(rp, m_valnames)) >= 0) {
	qualifier = TRUE;
	args.names = m_valnames;
	args.local = ptr->mm.mv;
	args.global = global_m_values.mv;
    } else if ((j = lookup_valnames(rp, b_valnames)) >= 0) {
	args.names = b_valnames;
	args.local = get_sm_vals(ptr);
	args.global = defining ? args.local : global_b_values.bv;
    } else {
	mlwarn("[BUG: no such submode: %s]", rp);
	reset_sm_qualifiers(ptr);
	return FALSE;
    }

    args.names += j;
    args.global +=j;
    args.local += j;

    /*
     * We store submodes in the majormode as local values.
     */
    status = adjvalueset(subname, TRUE, defining, FALSE, &args);

    /*
     * Check if we deleted one of the qualifiers, since there's no global
     * value to inherit back to, we'll have to ensure there's valid data.
     */
    if (status == TRUE
	&& qualifier
	&& !defining) {
	reset_qualifier(args.names, args.global);
    }

    if (status == TRUE
	&& !qualifier) {
	if (defining && found_per_submode(name, j)) {
	    TRACE(("submode names for %d present\n", j)) /*EMPTY */ ;
	} else if (defining) {
	    TRACE(("construct submode names for %d\n", j));
	    k = extend_mode_list(2);
	    k = insert_per_major(k,
				 per_submode(temp, name, j, TRUE));
	    (void) insert_per_major(k,
				    per_submode(temp, name, j, FALSE));
	} else {
	    TRACE(("destroy submode names for %d\n", j));
	    k = count_modes();
	    k = remove_per_major(k,
				 per_submode(temp, name, j, TRUE));
	    (void) remove_per_major(k,
				    per_submode(temp, name, j, FALSE));
	}
    }

    /* FIXME: remember to adjust all buffers that used this mode, in
     * case we make a minor mode part-of or removed from the major mode.
     */
    relist_settings();
    relist_majormodes();
    reset_sm_qualifiers(ptr);
    return status;
}

/* ARGSUSED */
int
define_submode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return do_a_submode(TRUE);
}

/* ARGSUSED */
int
remove_submode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return do_a_submode(FALSE);
}

/* ARGSUSED */
int
remove_mode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    char *name;
    int status;

    if ((status = prompt_majormode(&name, FALSE)) == TRUE) {
	if ((status = !predef_majormode(name)) == TRUE) {
	    TRACE(("remove majormode:%s\n", name));
	    free_majormode(name);
	    relist_settings();
	    relist_majormodes();
	} else {
	    mlwarn("[This is a predefined mode: %s]", name);
	}
    } else if (status == SORTOFTRUE) {
	status = TRUE;
    }
    return status;
}

#if !(OPT_CASELESS || OPT_VMS_PATH)
/*
 * For the given majormode (by index into my_majormodes[]), return the
 * corresponding buffer mode value.
 */
static int
get_sm_b_val(int n, int m)
{
    struct VAL *bv = get_sm_vals(my_majormodes[n].data);
    return (bv[m].vp->i);
}
#endif

/*
 * For the given majormode (by index into my_majormodes[]), return the
 * corresponding buffer mode value.
 */
static regexp *
get_sm_rexp(int n, int m)
{
    struct VAL *mv = get_sm_vals(my_majormodes[n].data);

    if (mv[m].vp != 0
	&& mv[m].vp->r != 0
	&& mv[m].vp->r->pat != 0
	&& mv[m].vp->r->pat[0] != 0
	&& mv[m].vp->r->reg != 0) {
	TRACE(("get_sm_rexp(%s) %s\n",
	       my_majormodes[n].shortname,
	       mv[m].vp->r->pat));
	return mv[m].vp->r->reg;
    }
    return 0;
}

/*
 * Use a regular expression (normally a suffix, such as ".c") to match the
 * buffer's filename.
 */
static int
test_by_suffix(int n, BUFFER *bp)
{
    int result = -1;

    if (my_majormodes[n].flag) {
	regexp *exp;
	int savecase = ignorecase;
	char *pathname = bp->b_fname;
	char *filename;
	char *suffix;
	TBUFF *savename = 0;
#if OPT_VMS_PATH
	TBUFF *stripname = 0;
#endif

#if OPT_CASELESS || OPT_VMS_PATH
	ignorecase = TRUE;
#else
	ignorecase = get_sm_b_val(n, MDIGNCASE);
#endif
#if OPT_VMS_PATH
	tb_scopy(&stripname, pathname);
	pathname = tb_values(stripname);
	strip_version(pathname);
#endif

	if (((exp = get_sm_rexp(n, VAL_STRIPSUFFIX)) != 0
	     || (exp = b_val_rexp(bp, VAL_STRIPSUFFIX)->reg) != 0)
	    && regexec(exp, pathname, (char *) 0, 0, -1)) {
	    if (tb_scopy(&savename, pathname) != 0) {
		strcpy(tb_values(savename) + (exp->startp[0] - pathname),
		       exp->endp[0]);
		pathname = tb_values(savename);
	    }
	}
	filename = pathleaf(pathname);
	suffix = strchr(filename, '.');
#if OPT_VMS_PATH
	if (suffix != 0 && suffix[1] == 0) {
	    *suffix = 0;
	    suffix = 0;
	}
#endif

	if ((exp = get_mm_rexp(n, MVAL_MODE_PATHNAME)) != 0
	    && regexec(exp, pathname, (char *) 0, 0, -1)) {
	    TRACE(("test_by_pathname(%s) %s\n",
		   pathname,
		   my_majormodes[n].shortname));
	    result = n;
	} else if ((exp = get_mm_rexp(n, MVAL_MODE_FILENAME)) != 0
		   && regexec(exp, filename, (char *) 0, 0, -1)) {
	    TRACE(("test_by_filename(%s) %s %s\n",
		   pathname,
		   filename,
		   my_majormodes[n].shortname));
	    result = n;
	} else if (!isShellOrPipe(pathname)
		   && suffix != 0
		   && (exp = get_mm_rexp(n, MVAL_MODE_SUFFIXES)) != 0
		   && regexec(exp, suffix, (char *) 0, 0, -1)) {
	    TRACE(("test_by_suffixes(%s) %s %s\n",
		   pathname,
		   suffix,
		   my_majormodes[n].shortname));
	    result = n;
	}
	ignorecase = savecase;
	tb_free(&savename);
#if OPT_VMS_PATH
	tb_free(&stripname);
#endif
    }
    return result;
}

static LINE *
get_preamble(BUFFER *bp)
{
    if (!is_empty_buf(bp)) {
	LINE *lp = lforw(buf_head(bp));
	if (lisreal(lp))
	    return lp;
    }
    return 0;
}

/*
 * Match the first line of the buffer against a regular expression.
 */
static int
test_by_preamble(int n, BUFFER *bp GCC_UNUSED, LINE *lp)
{
    int result = -1;

    if (lp != 0
	&& my_majormodes[n].flag) {
	regexp *exp = get_mm_rexp(n, MVAL_PREAMBLE);
	int savecase = ignorecase;
#if OPT_CASELESS || OPT_VMS_PATH
	ignorecase = TRUE;
#else
	ignorecase = get_sm_b_val(n, MDIGNCASE);
#endif
	if (exp != 0
	    && lregexec(exp, lp, 0, llength(lp))) {
	    TRACE(("test_by_preamble(%s) %s\n",
		   bp->b_fname,
		   my_majormodes[n].shortname));
	    result = n;
	}
	ignorecase = savecase;
    }
    return result;
}

static int
need_suffix_and_preamble(int n)
{
    if (get_mm_number(n, MVAL_QUALIFIERS) == MMQ_ALL
	&& (get_mm_rexp(n, MVAL_MODE_PATHNAME) != 0
	    || get_mm_rexp(n, MVAL_MODE_FILENAME) != 0
	    || get_mm_rexp(n, MVAL_MODE_SUFFIXES) != 0)
	&& get_mm_rexp(n, MVAL_PREAMBLE) != 0) {
	TRACE(("need both suffix/preamble for %s\n",
	       my_majormodes[n].shortname));
	return TRUE;
    }
    return FALSE;
}

void
infer_majormode(BUFFER *bp)
{
    static int level;

    TRACE((T_CALLED "infer_majormode(%s)\n", bp->b_bname));

    if (level++) {
	;
    } else if (my_majormodes != 0
	       && majormodes_order != 0
	       && !b_is_directory(bp)
	       && bp->b_fname != 0
	       && !is_internalname(bp->b_fname)) {
	int n, m;
	int result = -1;
	LINE *lp = get_preamble(bp);

	did_attach_mmode = FALSE;
	if (bp == curbp		/* otherwise we cannot script it */
	    && run_a_hook(&majormodehook) == TRUE
	    && did_attach_mmode) {
	    ;
	} else {
	    for (m = 0; (n = majormodes_order[m]) >= 0; m++) {
		if (need_suffix_and_preamble(n)) {
		    if (test_by_suffix(n, bp) >= 0
			&& test_by_preamble(n, bp, lp) >= 0) {
			TPRINTF(("matched preamble and suffix of %s\n", bp->b_bname));
			result = n;
			break;
		    }
		} else if (test_by_suffix(n, bp) >= 0) {
		    TPRINTF(("matched suffix of %s\n", bp->b_bname));
		    result = n;
		    break;
		} else if (test_by_preamble(n, bp, lp) >= 0) {
		    result = n;
		    TPRINTF(("matched preamble of %s\n", bp->b_bname));
		    break;
		}
	    }
	    if (result >= 0) {
		TPRINTF(("...inferred majormode %s\n", my_majormodes[result].shortname));
		attach_mmode(bp, my_majormodes[result].shortname);
	    }
	}
    }
    --level;

    returnVoid();
}

void
set_submode_val(const char *name, int n, int value)
{
    MAJORMODE *p;

    TRACE((T_CALLED "set_submode_val(%s, %d, %d)\n", name, n, value));
    if ((p = lookup_mm_data(name)) != 0) {
	struct VAL *q = get_sm_vals(p);
	q[n].v.i = value;
	make_local_val(q, n);
    }
    returnVoid();
}

void
set_submode_txt(const char *name, int n, char *value)
{
    MAJORMODE *p;

    TRACE((T_CALLED "set_submode_txt(%s, %d, %s)\n", name, n, value));
    if ((p = lookup_mm_data(name)) != 0) {
	struct VAL *q = get_sm_vals(p);
	q[n].v.p = strmalloc(value);
	make_local_val(q, n);
    }
    returnVoid();
}

void
set_majormode_rexp(const char *name, int n, const char *r)
{
    MAJORMODE *p;

    TRACE((T_CALLED "set_majormode_rexp(%s, %d, %s)\n", name, n, r));
    if ((p = lookup_mm_data(name)) != 0)
	set_qualifier(m_valnames + n, p->mm.mv + n, r, 0);
    returnVoid();
}

/*ARGSUSED*/
int
chgd_mm_order(BUFFER *bp GCC_UNUSED,
	      VALARGS * args GCC_UNUSED,
	      int glob_vals GCC_UNUSED,
	      int testing GCC_UNUSED)
{
    if (!testing) {
	compute_majormodes_order();
	relist_majormodes();
    }
    return TRUE;
}

/*ARGSUSED*/
int
chgd_filter(BUFFER *bp, VALARGS * args GCC_UNUSED, int glob_vals GCC_UNUSED, int testing)
{
    if (!testing) {
	struct VAL *values = args->local;
	BUFFER *bp2;

	if (values->vp->i == FALSE) {
	    if (glob_vals) {
		for_each_buffer(bp2) {
		    free_attribs(bp2);
		}
	    } else {
		free_attribs(bp);
	    }
	}
	set_winflags(glob_vals, WFHARD);
    }
    return TRUE;
}

/*ARGSUSED*/
int
reset_majormode(int f GCC_UNUSED, int n GCC_UNUSED)
{
    if (curbp->majr != 0)
	(void) detach_mmode(curbp, curbp->majr->shortname);
    infer_majormode(curbp);
    set_winflags(TRUE, WFMODE);
    return TRUE;
}

void
set_vilemode(BUFFER *bp)
{
    static const char *my_mode = "vilemode";
    VALARGS args;

    if (find_mode(bp, my_mode, FALSE, &args) == TRUE
	&& !b_is_scratch(bp)) {
	(void) set_mode_value(bp, my_mode, FALSE, TRUE, FALSE, &args,
			      (char *) 0);
    }
}
#endif /* OPT_MAJORMODE */

/*--------------------------------------------------------------------------*/

#if OPT_COLOR_SCHEMES
typedef struct {
    char *name;
    UINT code;
    int fcol;			/* foreground color */
    int bcol;			/* background color */
    int ccol;			/* cursor color */
    int attr;			/* bold, reverse, underline, etc. */
    char *list;
} PALETTES;

static UINT current_scheme;
static UINT num_schemes;
static PALETTES *my_schemes;
static FSM_CHOICES *my_scheme_choices;

static int
set_scheme_color(const FSM_CHOICES * fp, int *d, char *s)
{
#if OPT_ENUM_MODES
    int nval;

    if (isDigit(*s)) {
	if (!string_to_number(s, &nval))
	    return FALSE;
	if (choice_to_name(fp, nval) == 0)
	    nval = ENUM_ILLEGAL;
    } else {
	nval = choice_to_code(fp, s, strlen(s));
    }
    *d = nval;
    return TRUE;
#else
    return string_to_number(s, d);
#endif
}

static void
set_scheme_string(char **d, char *s)
{
    beginDisplay();
    if (*d)
	free(*d);
    *d = s ? strmalloc(s) : 0;
    endofDisplay();
}

/*
 * Find a scheme
 */
static PALETTES *
find_scheme(const char *name)
{
    PALETTES *result = 0, *p;

    if ((p = my_schemes) != 0) {
	while (p->name != 0) {
	    if (!strcmp(p->name, name)) {
		result = p;
		break;
	    }
	    p++;
	}
    }
    return result;
}

static PALETTES *
find_scheme_by_code(UINT code)
{
    PALETTES *result = 0, *p;

    if ((p = my_schemes) != 0) {
	while (p->name != 0) {
	    if (p->code == code) {
		result = p;
		break;
	    }
	    p++;
	}
    }
    return result;
}

/*
 * Update the list of choices for color scheme
 */
static void
update_scheme_choices(void)
{
    int n;

    if (my_schemes != 0) {
	beginDisplay();
	if (my_scheme_choices != 0) {
	    my_scheme_choices = typereallocn(FSM_CHOICES, my_scheme_choices, num_schemes);
	} else {
	    my_scheme_choices = typeallocn(FSM_CHOICES, num_schemes);
	}
	endofDisplay();

	if (my_scheme_choices != 0) {
	    for (n = 0; n < (int) num_schemes; n++) {
		my_scheme_choices[n].choice_name = my_schemes[n].name;
		my_scheme_choices[n].choice_code = my_schemes[n].code;
	    }
	    set_fsm_choice(s_color_scheme, my_scheme_choices);
	}
    }
}

static int
same_string(char *p, char *q)
{
    if (p == 0)
	p = "";
    if (q == 0)
	q = "";
    return !strcmp(p, q);
}

/*
 * Set the current color scheme, given a pointer to its attributes
 */
static int
set_current_scheme(PALETTES * p)
{
    PALETTES *q = find_scheme_by_code(current_scheme);

    TRACE((T_CALLED "set_current_scheme()\n"));
    current_scheme = p->code;

    if (p != 0
	&& q != 0
	&& (p->fcol != q->fcol
	    || p->bcol != q->bcol
	    || p->ccol != q->ccol
	    || p->attr != q->attr
	    || !same_string(p->list, q->list))) {

	if (p->list != 0 && p->list[0] != 0)
	    set_palette(p->list);

	set_global_g_val(GVAL_FCOLOR, p->fcol);
	term.setfore(p->fcol);

	set_global_g_val(GVAL_BCOLOR, p->bcol);
	term.setback(p->bcol);

	set_global_g_val(GVAL_CCOLOR, p->ccol);
	term.setccol(p->ccol);

	set_global_g_val(GVAL_VIDEO, p->attr);

	returnCode(TRUE);
    }
    returnCode(FALSE);
}

/*
 * Remove a color scheme, except for the 'default' scheme.
 */
static int
free_scheme(char *name)
{
    PALETTES *p;

    if (strcmp(name, s_default)
	&& (p = find_scheme(name)) != 0) {
	UINT code = p->code;

	TRACE(("free_scheme(%s)\n", name));

	beginDisplay();
	free(p->name);
	if (p->list != 0)
	    free(p->list);
	endofDisplay();

	while (p->name != 0) {
	    p[0] = p[1];
	    p++;
	}
	num_schemes--;
	update_scheme_choices();

	if (code == current_scheme)
	    (void) set_current_scheme(find_scheme(s_default));

	return TRUE;
    }
    TRACE(("cannot free_scheme(%s)\n", name));
    return FALSE;
}

/*
 * Allocate a new scheme
 */
static PALETTES *
alloc_scheme(const char *name)
{
    static UINT code;
    PALETTES *result;

    if ((result = find_scheme(name)) == 0) {
	int len = ++num_schemes;

	beginDisplay();
	if (len == 1) {
	    len = ++num_schemes;
	    my_schemes = typecallocn(PALETTES, len);
	} else {
	    my_schemes = typereallocn(PALETTES, my_schemes, len);
	}
	endofDisplay();

	if (my_schemes == 0) {
	    no_memory("alloc_scheme");
	    return 0;
	}

	len--;			/* point to list-terminator */
	memset(&my_schemes[len], 0, sizeof(PALETTES));
	while (--len > 0) {
	    my_schemes[len] = my_schemes[len - 1];
	    if (my_schemes[len - 1].name != 0
		&& strcmp(my_schemes[len - 1].name, name) < 0)
		break;
	}
	if ((my_schemes[len].name = strmalloc(name)) != 0) {
	    my_schemes[len].code = code++;	/* unique identifier */
	    my_schemes[len].fcol = -1;
	    my_schemes[len].bcol = -1;
	    my_schemes[len].ccol = -1;
	    my_schemes[len].attr = 0;
	    my_schemes[len].list = 0;
	    result = &my_schemes[len];
	    update_scheme_choices();
	}
    }
    return result;
}

static int
scheme_complete(DONE_ARGS)
{
    return kbd_complete(PASS_DONE_ARGS, (const char *) &my_schemes[0],
			sizeof(my_schemes[0]));
}

static int
prompt_scheme_name(char **result, int defining)
{
    static TBUFF *cbuf;		/* buffer to receive mode name into */
    int status;

    /* prompt the user and get an answer */
    tb_scopy(&cbuf, "");
    if ((status = kbd_reply("name: ",
			    &cbuf,
			    eol_history, ' ',
			    KBD_NORMAL,		/* FIXME: KBD_MAYBEC if !defining */
			    (defining || clexec)
			    ? no_completion
			    : scheme_complete)) == TRUE) {
	/* check for legal name (alphanumeric) */
	if ((status = is_identifier(tb_values(cbuf))) != TRUE) {
	    mlwarn("[Not an identifier: %s]", tb_values(cbuf));
	    return status;
	}
	*result = tb_values(cbuf);
	return status;
    }
    return status;
}

/* FIXME: generate this with mktbls? */
/* this table must be sorted, since we do name-completion on it */
/* *INDENT-OFF* */
static const struct VALNAMES scheme_values[] =
{
    { s_bcolor,		s_bcolor,	VALTYPE_ENUM,	0 },
    { s_ccolor,		s_ccolor,	VALTYPE_ENUM,	0 },
    { s_fcolor,		s_fcolor,	VALTYPE_ENUM,	0 },
    { s_palette,	s_palette,	VALTYPE_STRING, 0 },
    { "use",		"use",		VALTYPE_ENUM,	0 },
    { s_video_attrs,	s_video_attrs,	VALTYPE_ENUM,	0 },
    { 0, 0, 0, 0 }
};
/* *INDENT-ON* */

static int
scheme_value_complete(DONE_ARGS)
{
    return kbd_complete(PASS_DONE_ARGS,
			(const char *) &scheme_values[0],
			sizeof(scheme_values[0]));
}

static int
prompt_scheme_value(PALETTES * p)
{
    static TBUFF *cbuf;		/* buffer to receive mode name into */
    PALETTES *q;
    int status;

    /* prompt the user and get an answer */
    tb_scopy(&cbuf, "");
    if ((status = kbd_reply("scheme value: ",
			    &cbuf,
			    eol_history, '=',
			    KBD_NORMAL,		/* FIXME: KBD_MAYBEC if !defining */
			    scheme_value_complete)) == TRUE) {
	char respbuf[NFILEN];
	char *name = tb_values(cbuf);
	int code = *name;
	int eolchar = (code == *s_palette) ? '\n' : ' ';
	int (*complete) (DONE_ARGS) = no_completion;
#if OPT_ENUM_MODES
	const FSM_CHOICES *fp = 0;
	const struct VALNAMES *names;
	for (code = 0; scheme_values[code].name; code++) {
	    if (!strcmp(name, scheme_values[code].name)) {
		names = &scheme_values[code];
		if (!strcmp("use", scheme_values[code].name))
		    complete = scheme_complete;
		else if (is_fsm(names))
		    complete = fsm_complete;
		fp = valname_to_choices(names);
		break;
	    }
	}
#else
	void *fp = 0;
#endif
	tb_sappend0(&cbuf, " value: ");
	*respbuf = EOS;
	status = kbd_string(name, respbuf, sizeof(respbuf), eolchar,
			    KBD_NORMAL, complete);
	if (status == TRUE) {
	    mktrimmed(respbuf);
	    if (*name == *s_video_attrs) {
		status = set_scheme_color(fp, &(p->attr), respbuf);
	    } else if (*name == *s_bcolor) {
		status = set_scheme_color(fp, &(p->bcol), respbuf);
	    } else if (*name == *s_ccolor) {
		status = set_scheme_color(fp, &(p->ccol), respbuf);
	    } else if (*name == *s_fcolor) {
		status = set_scheme_color(fp, &(p->fcol), respbuf);
	    } else if (*name == *s_palette) {
		set_scheme_string(&(p->list), respbuf);
		return status;
	    } else if (*name == 'u'
		       && (q = find_scheme(respbuf)) != 0) {
		char *newlist = 0;
		if (q->list == 0 || (newlist = strmalloc(q->list)) != 0) {
		    p->fcol = q->fcol;
		    p->bcol = q->bcol;
		    p->ccol = q->ccol;
		    p->attr = q->attr;
		    if (p->list)
			FreeAndNull(p->list);
		    p->list = newlist;
		} else {
		    return no_memory("prompt_scheme_value");
		}
	    }
	}
	if (status == TRUE)
	    return (end_string() == ' ') ? -1 : TRUE;
    }
    return status;
}

#if OPT_UPBUFF
/*ARGSUSED*/
static int
update_schemelist(BUFFER *bp GCC_UNUSED)
{
    return desschemes(FALSE, 1);
}
#endif /* OPT_UPBUFF */

/*
 * Define the default color scheme, based on the current settings of color
 * and $palette.
 */
void
init_scheme(void)
{
    PALETTES *p = alloc_scheme(s_default);
    char *list = tb_values(tb_curpalette);

    p->fcol = -1;
    p->bcol = -1;
    p->ccol = -1;
    if (list != 0)
	p->list = strmalloc(list);
}

/*
 * Define a color scheme, e.g.,
 *	define-color-scheme {name} [fcolor={value}|bcolor={value}|{value}]
 * where the {value}'s are all color names or constants 0..NCOLORS-1.
 */
/*ARGSUSED*/
int
define_scheme(int f GCC_UNUSED, int n GCC_UNUSED)
{
    char *name;
    int status;

    if ((status = prompt_scheme_name(&name, TRUE)) == TRUE) {
	PALETTES *p = alloc_scheme(name);
	while ((status = prompt_scheme_value(p)) < 0)
	    continue;
    }
    update_scratch(COLOR_SCHEMES_BufName, update_schemelist);
    return status;
}

/*
 * Remove a color scheme
 */
/*ARGSUSED*/
int
remove_scheme(int f GCC_UNUSED, int n GCC_UNUSED)
{
    char *name;
    int status;

    if (my_schemes == 0 || my_schemes->name == 0) {
	status = FALSE;
    } else if ((status = prompt_scheme_name(&name, FALSE)) == TRUE) {
	status = free_scheme(name);
    }
    update_scratch(COLOR_SCHEMES_BufName, update_schemelist);
    return status;
}

/*ARGSUSED*/
static void
makeschemelist(int dum1 GCC_UNUSED, void *ptr GCC_UNUSED)
{
    static const char fmt[] = "\n   %s=%s";
    PALETTES *p;
    int num = (num_schemes > 1) ? (num_schemes - 1) : 0;

    if (my_schemes != 0) {
	bprintf("There %s %d color scheme%s defined",
		(num != 1) ? "are" : "is", num, PLURAL(num));

	for (p = my_schemes; p != 0 && p->name != 0; p++) {
	    bprintf("\n\n%s", p->name);
	    bprintf(fmt, s_fcolor, get_color_name(p->fcol));
	    bprintf(fmt, s_bcolor, get_color_name(p->bcol));
	    bprintf(fmt, s_ccolor, get_color_name(p->ccol));
	    bprintf(fmt, s_video_attrs,
		    choice_to_name(fsm_videoattrs_choices, p->attr));
	    if (p->list != 0)
		bprintf(fmt, s_palette, p->list);
	}
    }
}

/*ARGSUSED*/
int
desschemes(int f GCC_UNUSED, int n GCC_UNUSED)
{
    return liststuff(COLOR_SCHEMES_BufName, FALSE, makeschemelist,
		     0, (void *) 0);
}

int
chgd_scheme(BUFFER *bp GCC_UNUSED, VALARGS * args, int glob_vals, int testing)
{
    if (!testing) {
	PALETTES *p = find_scheme_by_code(args->local->vp->i);
	if (p == 0) {
	    return FALSE;
	} else if (set_current_scheme(p)) {
	    set_winflags(glob_vals, WFHARD | WFCOLR);
	    need_update = TRUE;
	}
    }
    return TRUE;
}
#endif /* OPT_COLOR_SCHEMES */

/*--------------------------------------------------------------------------*/
#if SYS_VMS
const char *
vms_record_format(int code)
{
    return choice_to_name(fsm_recordformat_choices, code);
}

const char *
vms_record_attrs(int code)
{
    return choice_to_name(fsm_recordattrs_choices, code);
}
#endif

/*--------------------------------------------------------------------------*/

#if NO_LEAKS
void
mode_leaks(void)
{
    beginDisplay();
#if OPT_COLOR_SCHEMES
    if (my_schemes != 0) {
	int last;
	for (last = 0; my_schemes[last].name != 0; last++) ;
	if (--last >= 0) {
	    while (my_schemes != 0 && my_schemes[last].name != 0) {
		if (!free_scheme(my_schemes[last].name)) {
		    if (last <= 0)
			break;
		    last = 0;
		} else if (last > 0) {
		    last--;
		}
	    }
	}
	if (my_schemes != 0) {	/* it's ok to free the default scheme */
	    FreeAndNull(my_schemes->list);
	    FreeAndNull(my_schemes->name);
	    free(my_schemes);
	}
	FreeAndNull(my_scheme_choices);
    }
#endif

#if OPT_ENUM_MODES && OPT_COLOR
    FreeAndNull(my_colors);
    FreeAndNull(my_hilite);
#endif

#if OPT_EVAL || OPT_MAJORMODE
    FreeAndNull(my_varmodes);
#endif

#if OPT_MAJORMODE
    while (my_majormodes != 0 && my_majormodes->shortname != 0) {
	char temp[NSTRING];
	free_majormode(strcpy(temp, my_majormodes->shortname));
    }
    FreeAndNull(my_majormodes);

    FreeAndNull(major_g_vals);
    FreeAndNull(major_l_vals);
    if (my_mode_list != all_modes
	&& my_mode_list != 0) {
	int j = count_modes();
	while (j > 0)
	    remove_per_major(j--, my_mode_list[0]);
	FreeAndNull(my_mode_list);
    }
    FreeAndNull(major_valnames);
    FreeAndNull(majormodes_order);
#endif
    endofDisplay();
}
#endif /* NO_LEAKS */
