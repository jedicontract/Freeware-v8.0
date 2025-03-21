/*
 * This file contains the command processing functions for the commands
 * that take motion operators.
 * written for vile: Copyright (c) 1990, 1995-2003 by Paul Fox
 *
 * $Header: /usr/build/vile/vile/RCS/opers.c,v 1.90 2004/12/07 01:29:50 tom Exp $
 *
 */

#include	"estruct.h"
#include	"edef.h"
#include	"nefunc.h"

/* For the "operator" commands -- the following command is a motion, or
 *  the operator itself is repeated.  All operate on regions.
 */
int
vile_op(int f, int n, OpsFunc fn, const char *str)
{
    int c = 0;
    int thiskey;
    int status;
    const CMDFUNC *cfp;		/* function to execute */
    BUFFER *ourbp;
#if OPT_MOUSE
    WINDOW *wp0 = curwp;
#endif

    doingopcmd = TRUE;

    pre_op_dot = DOT;
    ourbp = curbp;

    if (havemotion != NULL) {
	cfp = havemotion;
	havemotion = NULL;
    } else {
	TBUFF *tok = 0;

	mlwrite("%s operation pending...", str);
	(void) update(FALSE);

	/* get the next command from the keyboard */
	/* or a command line, as approp. */
	if (clexec) {
	    char *value = mac_tokval(&tok);	/* get the next token */
	    if (value != 0 && strcmp(value, "lines"))
		cfp = engl2fnc(value);
	    else
		cfp = &f_godotplus;
	} else {
	    thiskey = lastkey;
	    c = kbd_seq();

#if OPT_MOUSE
	    if (curwp != wp0) {
		unkeystroke(c);
		doingopcmd = FALSE;
		return FALSE;
	    }
#endif
	    /* allow second chance for entering counts */
	    do_repeats(&c, &f, &n);

	    if (thiskey == lastkey)
		cfp = &f_godotplus;
	    else
		cfp = DefaultKeyBinding(c);

	}
	if (cfp != 0) {
	    mlerase();
	} else {
	    if (!clexec) {
		char temp[NSTRING];
		lsprintf(temp, "(%d)", c);
		tb_scopy(&tok, temp);
	    }
	    (void) no_such_function(tb_values(tok));
	}
	tb_free(&tok);
    }
    if (!cfp) {
	doingopcmd = FALSE;
	return FALSE;
    }

    if ((cfp->c_flags & MOTION) == 0) {
	kbd_alarm();
	doingopcmd = FALSE;
	return (ABORT);
    }

    /* motion is interpreted as affecting full lines */
    if (regionshape == EXACT) {
	if (cfp->c_flags & FL)
	    regionshape = FULLLINE;
	if (cfp->c_flags & VL_RECT)
	    regionshape = RECTANGLE;
    }

    /* and execute the motion */
    status = execute(cfp, f, n);
    post_op_dot = DOT;

    if (status != TRUE) {
	doingopcmd = FALSE;
	regionshape = EXACT;
	mlforce("[Motion failed]");
	return FALSE;
    }

    opcmd = 0;

    MK = pre_op_dot;

    /* we've successfully set up a region */
    if (!fn) {			/* be defensive */
	mlforce("BUG -- null func pointer in operator");
	status = FALSE;
    } else {
	status = (fn) ();
    }

    if (ourbp == curbp)		/* in case the func switched buffers on us */
	swapmark();

    if (regionshape == FULLLINE)
	(void) firstnonwhite(FALSE, 1);

    regionshape = EXACT;

    doingopcmd = FALSE;

    haveregion = FALSE;

    return status;
}

int
operdel(int f, int n)
{
    int status;

    opcmd = OPDEL;
    lines_deleted = 0;
    status = vile_op(f, n, killregion,
		     ((regionshape == FULLLINE)
		      ? "Delete of full lines"
		      : "Delete"));
    if (do_report(lines_deleted))
	mlforce("[%d lines deleted]", lines_deleted);
    return status;
}

int
operlinedel(int f, int n)
{
    regionshape = FULLLINE;
    return operdel(f, n);
}

static int
chgreg(void)
{
    if (regionshape == RECTANGLE) {
	return stringrect();
    } else {
	killregion();
	if (regionshape == FULLLINE && !is_empty_buf(curbp)) {
	    if (backline(FALSE, 1) == TRUE)
		/* backline returns FALSE at top of buf */
		return opendown(TRUE, 1);
	    else
		return openup(TRUE, 1);
	}
	return ins();
    }
}

int
operchg(int f, int n)
{
    int s;

    opcmd = OPOTHER;
    s = vile_op(f, n, chgreg, "Change");
    if (s == TRUE)
	swapmark();
    return s;
}

#if 0
#define toggle_b_val(bp, mode) set_b_val(bp, mode, !b_val(bp, mode))

static int
toggle_autoindent(void)
{
    int code = b_val(curbp, MDAIND);

    toggle_b_val(curbp, MDAIND);

    return code;
}

int
operchg_i(int f, int n)
{
    int s, indent;

    opcmd = OPOTHER;
    indent = toggle_autoindent();
    s = vile_op(f, n, chgreg, indent ? "Change (noai)" : "Change (ai)");
    if (s == TRUE)
	swapmark();
    (void) toggle_autoindent();
    return s;
}
#endif

int
operlinechg(int f, int n)
{
    int s;

    regionshape = FULLLINE;
    opcmd = OPOTHER;
    s = vile_op(f, n, chgreg, "Change of full lines");
    if (s == TRUE)
	swapmark();
    return s;
}

int
operjoin(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, joinregion, "Join");
}

int
operjoin_x(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, joinregion_x, "Join Exact");
}

int
operyank(int f, int n)
{
    MARK savedot;
    REGION region;
    int s;

    savedot = DOT;
    opcmd = OPDEL;
    wantregion = &region;
    s = vile_op(f, n, yankregion, "Yank");
    wantregion = 0;
    /*
     * If the associated motion was to the left, or up, we want to set DOT to
     * the beginning of the region, to match vi's behavior.  Otherwise leave
     * DOT where it is.
     */
    if (s == TRUE && b_val(curbp, MDYANKMOTION)) {
	if (line_no(curbp, post_op_dot.l) != 0
	    && line_no(curbp, post_op_dot.l) < line_no(curbp, savedot.l)) {
	    savedot = post_op_dot;
	} else if (!samepoint(region.r_orig, region.r_end)) {
	    if (sameline(region.r_orig, savedot)
		&& sameline(region.r_end, savedot)
		&& region.r_orig.o < savedot.o) {
		savedot = region.r_orig;
	    }
	}
    }
    DOT = savedot;
    return s;
}

int
operlineyank(int f, int n)
{
    MARK savedot;
    int s;
    savedot = DOT;
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    s = vile_op(f, n, yankregion, "Yank of full lines");
    DOT = savedot;
    return s;
}

int
operflip(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, flipregion, "Flip case");
}

int
operupper(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, upperregion, "Upper case");
}

int
operlower(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, lowerregion, "Lower case");
}

/*
 * The shift commands are special, because vi allows an implicit repeat-count
 * to be specified by repeating the '<' or '>' operators.
 */
static int
shift_n_times(int f, int n, OpsFunc func, const char *msg)
{
    register int status = FALSE;

    regionshape = FULLLINE;
    opcmd = OPOTHER;

    if (havemotion != NULL) {
	const CMDFUNC *cfp = havemotion;
	while (n-- > 0) {
	    havemotion = cfp;
	    if ((status = vile_op(FALSE, 1, func, msg)) != TRUE)
		break;
	}
    } else
	status = vile_op(f, n, func, msg);
    return status;
}

int
operlshift(int f, int n)
{
    return shift_n_times(f, n, shiftlregion, "Left shift");
}

int
operrshift(int f, int n)
{
    return shift_n_times(f, n, shiftrregion, "Right shift");
}

int
operwrite(int f, int n)
{
    register int s;
    char fname[NFILEN];

    if (ukb != 0) {
	if ((s = mlreply_file("Write to file: ", (TBUFF **) 0,
			      FILEC_WRITE | FILEC_PROMPT, fname)) != TRUE)
	    return s;
	return kwrite(fname, TRUE);
    } else {
	opcmd = OPOTHER;
	return vile_op(f, n, writeregion, "File write");
    }
}

#if OPT_FORMAT
int
operformat(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, formatregion, "Format");
}
#endif

#if OPT_SHELL
int
operfilter(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, filterregion, "Filter");
}
#endif

int
operprint(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, plineregion, "Line print");
}

int
operpprint(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, pplineregion, "Line-number print");
}

int
operlist(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, llineregion, "Line list");
}

int
opernumber(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, nlineregion, "Numbered list");
}

int
opersubst(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, substregion, "Substitute");
}

int
opersubstall(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, subst_all_region, "Substitute-all");
}

int
opersubstagain(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, subst_again_region, "Substitute-again");
}

int
operentab(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, entab_region, "Spaces-->Tabs");
}

int
oper_l_entab(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, l_entab_region, "Spaces-->Tabs");
}

int
operdetab(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, detab_region, "Tabs-->Spaces");
}

int
oper_l_detab(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, l_detab_region, "Tabs-->Spaces");
}

int
opertrim(int f, int n)
{
    regionshape = FULLLINE;
    opcmd = OPOTHER;
    return vile_op(f, n, trim_region, "Trim whitespace");
}

int
operblank(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, blank_region, "Blanking");
}

int
operopenrect(int f, int n)
{
    opcmd = OPOTHER;
    regionshape = RECTANGLE;
    return vile_op(f, n, openregion, "Opening");
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                