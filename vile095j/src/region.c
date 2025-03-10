/*
 * The routines in this file deal with the region, that magic space between "."
 * and mark.  Some functions are commands.  Some functions are just for
 * internal use.
 *
 * $Header: /usr/build/vile/vile/RCS/region.c,v 1.133 2006/04/19 23:55:02 tom Exp $
 *
 */

#include	"estruct.h"
#include	"edef.h"

typedef int (*CharProcFunc) (int c);

static CharProcFunc charprocfunc;

/*
 * Walk through the characters in a line, applying a function.  the line is
 * marked changed if the function returns other than -1.  if it returns other
 * than -1, then the char is replaced with that value.
 *
 * The ll and rr args are OFFSETS, so if you use this routine with
 * do_lines_in_region, tell it to CONVERT columns to offsets.
 */
/*ARGSUSED*/
static int
do_chars_in_line(void *flagp GCC_UNUSED, int ll, int rr)
{
    int c, nc;
    int changed = 0;
    LINE *lp;
    int i;

    lp = DOT.l;

    if (llength(lp) < ll)
	return TRUE;

    DOT.o = ll;
    if (rr > llength(lp))
	rr = llength(lp);

    for (i = ll; i < rr; i++) {
	c = lgetc(lp, i);
	nc = (charprocfunc) (c);
	if (nc != -1) {
	    copy_for_undo(lp);
	    lputc(lp, i, (char) nc);
	    changed++;
	}
    }
    if (changed)
	chg_buff(curbp, WFHARD);
    return TRUE;
}

/*
 * The (*linefunc)() routine that this calls _must_ be prepared to
 * to get empty lines passed to it from this routine.
 *
 * convert_cols, if true, says when we're processing a rectangle to
 * convert columns to offsets.
 */
static int
do_lines_in_region(int (*linefunc) (REGN_ARGS), void *argp, int convert_cols)
{
    LINE *linep;
    int status;
    REGION region;
    C_NUM l, r;

    if ((status = getregion(&region)) == TRUE) {

	/* for each line in the region, ... */
	linep = region.r_orig.l;
	for_ever {
	    /* move through the region... */
	    /* it's important that the linefunc get called
	       for every line, even if blank, since it
	       may want to keep track of newlines, for
	       instance */
	    DOT.l = linep;
	    DOT.o = w_left_margin(curwp);
	    if (regionshape == RECTANGLE) {
		if (convert_cols) {
		    C_NUM reached;
		    l = getoff(region.r_leftcol, &reached);
		    if (l < 0)
			l = -l + llength(linep);
		    r = getoff(region.r_rightcol, &reached);
		    if (r < 0)
			r = -r + llength(linep);
		    if (reached > region.r_rightcol)	/* a tab? */
			reached = region.r_rightcol;
		} else {
		    l = region.r_leftcol;
		    r = region.r_rightcol;
		}
	    } else {
		l = w_left_margin(curwp);
		r = llength(DOT.l);
		if (sameline(region.r_orig, DOT))
		    l = region.r_orig.o;
		if (sameline(region.r_end, DOT)) {
		    r = region.r_end.o;
		    /*
		     * if we're on the end-of- region, in col 0, we're done. 
		     * we don't want to call the line function for the empty
		     * case.
		     */
		    if (r == 0)
			break;
		}
	    }

	    /* ...and process each line */
	    if ((status = (*linefunc) (argp, l, r)) != TRUE) {
		return status;
	    }

	    if (linep == region.r_end.l)
		break;

	    linep = lforw(linep);

	    /*
	     * If this happened to be a full-line region, then we've bumped the
	     * end-region pointer to the next line so we won't run into
	     * problems with the width.  But we don't really want to _process_
	     * the end-region line, so check here also.  (Leave the above check
	     * alone just in case the buffer is empty).
	     */
	    if (regionshape == FULLLINE
		&& linep == region.r_end.l)
		break;

	}
	if (regionshape == FULLLINE) {
	    (void) kinsertlater(-1);
	    (void) firstnonwhite(FALSE, 1);
	}
    }
#if OPT_SELECTIONS
    if (linefunc == kill_line)	/* yuck.  it's the only one that deletes */
	find_release_attr(curbp, &region);
#endif
    return status;
}

static int
killrectmaybesave(int save)
{
    int s;
    MARK savedot;

    savedot = DOT;

    if (save) {
	kregcirculate(TRUE);
	ksetup();
	if (regionshape == FULLLINE) {
	    kregflag |= KLINES;
	} else if (regionshape == RECTANGLE) {
	    kregflag |= KRECT;
	}
    }
    s = do_lines_in_region(kill_line, (void *) &save, FALSE);
    restore_dot(savedot);

    if (s && do_report(klines + (kchars != 0))) {
	mlwrite("[%d line%s, %d character%s killed]",
		klines, PLURAL(klines),
		kchars, PLURAL(kchars));
    }
    if (save) {
	kdone();
	ukb = 0;
    }
    return (s);
}

/* Helper function for vile commands that must access do_lines_in_region(). */
DORGNLINES
get_do_lines_rgn(void)
{
    return (DORGNLINES) (do_lines_in_region);
}

/*
 * Kill the region.  Ask "getregion" to figure out the bounds of the region. 
 * Move "." to the start, and kill the characters.
 */
int
killregion(void)
{
    if (regionshape == RECTANGLE)
	return killrectmaybesave(TRUE);
    else
	return killregionmaybesave(TRUE);
}

int
killregionmaybesave(int save)
{
    int status;
    REGION region;

    if ((status = getregion(&region)) == TRUE) {
	if (save) {
	    kregcirculate(TRUE);
	    ksetup();		/* command, so do magic */
	    if (regionshape == FULLLINE)
		kregflag |= KLINES;
	}
	DOT = region.r_orig;
	status = ldelete(region.r_size, save);
	if (save) {
	    kdone();
	    ukb = 0;
	}
    }
#if OPT_SELECTIONS
    find_release_attr(curbp, &region);
#endif
    return status;
}

int
kill_line(void *flagp, int l, int r)
{
    int s;
    int save = *(int *) flagp;
    LINE *lp = DOT.l;

    s = detabline((void *) FALSE, 0, 0);
    if (s != TRUE)
	return s;

    DOT.o = l;

    if (r > llength(lp))
	r = llength(lp);

    if (r > l) {
	s = ldelete((B_COUNT) (r - l), save);
	if (s != TRUE)
	    return s;
    }

    if (save)
	kinsert('\n');

    if (b_val(curbp, MDTABINSERT))
	s = entabline((void *) TRUE, 0, 0);

    DOT.o = l;
    return s;
}

/*
 * Open up a region -- shift the "selected" area of each line by its own
 * length.  This is most useful for rectangular regions.  Fill character is
 * space, unless a string is passed in, in which case it is used instead.
 */
/*ARGSUSED*/
static int
open_hole_in_line(void *flagp, int l, int r)
{
    char *string = (char *) flagp;
    int len;
    int s;
    int saveo = DOT.o;
    LINE *lp = DOT.l;

    s = detabline((void *) FALSE, 0, 0);
    if (s != TRUE)
	return s;

    if (llength(lp) <= l) {	/* nothing to do if no string */
	if (!string) {
	    if (b_val(curbp, MDTABINSERT))
		s = entabline((void *) TRUE, 0, 0);
	    DOT.o = saveo;
	    return s;
	} else {
	    DOT.o = llength(lp);
	    if (l - DOT.o)
		linsert(l - DOT.o, ' ');
	}
    }
    DOT.o = l;
    if (string) {
	len = (int) strlen(string);
	if (len < r - l)
	    len = r - l;
    } else {
	len = r - l;
    }
    s = lstrinsert(string, len);
    if (s != TRUE)
	return s;

    DOT.o = saveo;
    if (b_val(curbp, MDTABINSERT))
	s = entabline((void *) TRUE, 0, 0);
    return s;
}

/*
 * open up a region
 */
int
openregion(void)
{
    return do_lines_in_region(open_hole_in_line, (void *) NULL, FALSE);
}

/*
 * insert a shiftwidth at the front of the line
 * don't do it if we're in cmode and the line starts with '#'
 */
/*ARGSUSED*/
static int
shift_right_line(void *flagp GCC_UNUSED, int l GCC_UNUSED, int r GCC_UNUSED)
{
    int s, t;

    if (is_empty_line(DOT)
	|| (is_c_mode(curbp)
	    && llength(DOT.l) > 0
	    && char_at(DOT) == '#'
	    && b_val(curbp, MDCINDENT))) {
	return TRUE;
    }
    s = shiftwid_val(curbp);
    t = tabstop_val(curbp);
    DOT.o = w_left_margin(curwp);
    if (s) {			/* try to just insert tabs if possible */
	if (b_val(curbp, MDTABINSERT) && s >= t && (s % t == 0)) {
	    linsert(s / t, '\t');
	} else {
	    detabline((void *) TRUE, 0, 0);
	    DOT.o = w_left_margin(curwp);
	    linsert(s, ' ');
	}
	if (b_val(curbp, MDTABINSERT))
	    entabline((void *) TRUE, 0, 0);
    }
    return firstnonwhite(FALSE, 1);
}

/*
 * shift region right by a tab stop
 * if region is rectangular, "open it up"
 */
int
shiftrregion(void)
{
    if (regionshape == RECTANGLE)
	return do_lines_in_region(open_hole_in_line, (void *) NULL, FALSE);

    regionshape = FULLLINE;
    return do_lines_in_region(shift_right_line, (void *) 0, FALSE);
}

/*
 * delete a shiftwidth-equivalent from the front of the line
 */
/*ARGSUSED*/
static int
shift_left_line(void *flagp GCC_UNUSED, int l GCC_UNUSED, int r GCC_UNUSED)
{
    int i;
    int lim;
    int s;
    LINE *linep = DOT.l;

    if (llength(linep) == 0)
	return TRUE;

    s = shiftwid_val(curbp);

    detabline((void *) TRUE, 0, 0);

    /* examine the line to the end, or the first shiftwidth, whichever
       comes first */
    lim = (s < llength(linep)) ? s : llength(linep);

    i = 0;
    /* count the leading spaces */
    while (lgetc(linep, i) == ' ' && i < lim)
	i++;

    if (i != 0) {		/* did we find space/tabs to kill? */
	DOT.o = w_left_margin(curwp);
	if ((s = ldelete((B_COUNT) i, FALSE)) != TRUE)
	    return s;
    }

    DOT.o = w_left_margin(curwp);
    if (b_val(curbp, MDTABINSERT))
	entabline((void *) TRUE, 0, 0);
    return TRUE;
}

/*
 * shift region left by a tab stop
 */
int
shiftlregion(void)
{
    if (regionshape == RECTANGLE)
	return killrectmaybesave(FALSE);

    regionshape = FULLLINE;
    return do_lines_in_region(shift_left_line, (void *) 0, FALSE);
}

/*
 * change all tabs in the line to the right number of spaces.
 * leadingonly says only do leading whitespace
 */
/*ARGSUSED*/
int
detabline(void *flagp, int l GCC_UNUSED, int r GCC_UNUSED)
{
    int s;
    int c;
    int ocol;
    int leadingonly = (flagp != 0);
    LINE *lp = DOT.l;

    if (llength(lp) == 0)
	return TRUE;

    ocol = getccol(FALSE);

    DOT.o = 0;

    /* remove tabs from the entire line */
    while (DOT.o < llength(lp)) {
	c = char_at(DOT);
	if (leadingonly && !isSpace(c))
	    break;
	/* if we have found a tab to remove */
	if (c == '\t') {
	    if ((s = ldelete(1L, FALSE)) != TRUE)
		return s;
	    if ((s = insspace(TRUE,
			      tabstop_val(curbp)
			      - (DOT.o % tabstop_val(curbp)))) != TRUE)
		return s;
	}
	DOT.o++;
    }
    (void) gocol(ocol);
    return TRUE;
}

/*
 * change all tabs in the region to the right number of spaces
 */
int
detab_region(void)
{
    regionshape = FULLLINE;
    return do_lines_in_region(detabline, (void *) FALSE, FALSE);
}

/*
 * change leading tabs in the region to the right number of spaces
 */
int
l_detab_region(void)
{
    regionshape = FULLLINE;
    return do_lines_in_region(detabline, (void *) TRUE, FALSE);
}

/*
 * convert all appropriate spaces in the line to tab characters.
 * leadingonly says only do leading whitespace
 */
/*ARGSUSED*/
int
entabline(void *flagp, int l GCC_UNUSED, int r GCC_UNUSED)
{
    int savecol;
    int leadingonly = (flagp != 0);
    LINE *lp = DOT.l;
    C_NUM ocol, ncol;
    C_NUM ooff, noff;
    int ch;

    if (llength(lp) == 0)
	return TRUE;

    savecol = getccol(FALSE);

    ooff = noff = 0;
    ocol = ncol = 0;
    for_ever {

	if (ooff == llength(lp))
	    ch = '\n';
	else
	    ch = lgetc(lp, ooff);

	switch (ch) {
	case ' ':
	    ocol++;
	    break;
	case '\t':
	    ocol = next_tabcol(ocol);
	    break;
	default:
	    while (next_tabcol(ncol) <= ocol) {
		if (ncol + 1 == ocol)
		    break;
		(void) lreplc(lp, noff++, '\t');
		ncol = next_tabcol(ncol);
	    }
	    while (ncol < ocol) {
		(void) lreplc(lp, noff++, ' ');
		ncol++;
	    }
	    if (ch == '\n' || leadingonly) {
		if (noff != ooff) {
		    while (ooff < llength(lp))
			(void) lreplc(lp, noff++, lgetc(lp, ooff++));
		    DOT.o = noff;
		    if (ooff > noff)
			(void) ldelete(ooff - noff, FALSE);
		}
		(void) gocol(savecol);
		return TRUE;
	    }
	    (void) lreplc(lp, noff++, ch);
	    ncol++, ocol++;
	}
	ooff++;
    }
}

/*
 * convert all appropriate spaces in the region to tab characters
 */
int
entab_region(void)
{
    regionshape = FULLLINE;
    return do_lines_in_region(entabline, (void *) FALSE, FALSE);
}

/*
 * convert leading spaces in the region to tab characters
 */
int
l_entab_region(void)
{
    regionshape = FULLLINE;
    return do_lines_in_region(entabline, (void *) TRUE, FALSE);
}

/*
 * Trim trailing whitespace from a line.  dot is preserved if possible.
 * (dot is even preserved if it was sitting on the newline).
 */
/*ARGSUSED*/
int
trimline(void *flag GCC_UNUSED, int l GCC_UNUSED, int r GCC_UNUSED)
{
    int off;
    LINE *lp;
    int odoto, s;
    B_COUNT delcnt;
    int was_at_eol;

    lp = DOT.l;

    if (llength(lp) == 0)
	return TRUE;

    /* may return -1 if line is all whitespace.  but
       that's okay, since the math still works. */
    off = lastchar(lp);

    if (llength(lp) <= (off + 1))
	return TRUE;

    delcnt = llength(lp) - (off + 1);

    odoto = DOT.o;
    was_at_eol = (odoto == llength(lp));

    DOT.o = off + 1;
    s = ldelete(delcnt, FALSE);

    if (odoto > off) {		/* do we need to back up? */
	odoto = llength(lp);
	if (!was_at_eol)
	    odoto--;		/* usually we want the last char on line */
    }

    if (odoto < 0)
	DOT.o = 0;
    else
	DOT.o = odoto;
    return s;
}

/*
 * trim trailing whitespace from a region
 */
int
trim_region(void)
{
    regionshape = FULLLINE;
    return do_lines_in_region(trimline, (void *) 0, FALSE);
}

/* turn line, or part, to whitespace */
/*ARGSUSED*/
static int
blankline(void *flagp, int l, int r)
{
    char *string = (char *) flagp;
    int len;
    int s = TRUE;
    int saveo;
    LINE *lp = DOT.l;

    saveo = l;

    /* if the shape is rectangular, then l and r are columns, not
       offsets */
    if (regionshape == RECTANGLE) {
	s = detabline((void *) FALSE, 0, 0);
	if (s != TRUE)
	    return s;
    }

    if (llength(lp) <= l) {	/* nothing to do if no string */
	if (!string) {
	    if (regionshape == RECTANGLE && b_val(curbp, MDTABINSERT))
		s = entabline((void *) TRUE, 0, 0);
	    DOT.o = saveo;
	    return s;
	} else {
	    DOT.o = llength(lp);
	    if (l - DOT.o)
		linsert(l - DOT.o, ' ');
	}
    }

    DOT.o = l;

    if (llength(lp) <= r) {
	/* then the rect doesn't extend to the end of line */
	if (llength(lp) > l)
	    ldelete((B_COUNT) (llength(lp) - l), FALSE);

	/* so there's nothing beyond the rect, so insert at
	   most r-l chars of the string, or nothing */
	if (string) {
	    len = (int) strlen(string);
	    if (len > r - l)
		len = r - l;
	} else {
	    len = 0;
	}
    } else if (r > l) {
	/* the line goes on, so delete and reinsert exactly */
	len = r - l;
	ldelete((B_COUNT) len, FALSE);
    } else {
	len = 0;
    }

    s = lstrinsert(string, len);
    if (s != TRUE)
	return s;

    if (regionshape == RECTANGLE && b_val(curbp, MDTABINSERT))
	s = entabline((void *) TRUE, 0, 0);

    return s;
}

/*
 * open up a region, filling it with a supplied string
 * this is pretty simplistic -- could be a lot more clever
 */
int
stringrect(void)
{
    int s;
    static char buf[NLINE];

    s = mlreply("Rectangle text: ", buf, sizeof(buf));
    if (s == TRUE) {
/* i couldn't decide at first whether we should be inserting or
	overwriting... this chooses. */
#ifdef insert_the_string
	s = do_lines_in_region(open_hole_in_line, (void *) buf, FALSE);
#else /* overwrite the string */
	s = do_lines_in_region(blankline, (void *) buf, FALSE);
#endif
    }
    return s;
}

/*
 * Copy all of the characters in the region to the kill buffer.  Don't move dot
 * at all.  This is a bit like a kill region followed by a yank.
 */
static int
_yankchar(int c)
{
    kinsert(c);
    /* FIXME check return value, longjmp back to yank_line */
    return -1;
}

/*ARGSUSED*/
static int
yank_line(void *flagp GCC_UNUSED, int l, int r)
{
    int s;

    charprocfunc = _yankchar;
    s = do_chars_in_line((void *) NULL, l, r);
    if (s) {
	if (r == llength(DOT.l) || regionshape == RECTANGLE) {
	    /* we don't necessarily want to insert the last newline
	       in a region, so we delay it */
	    s = kinsertlater('\n');
	} else if (r > llength(DOT.l)) {
	    /* This can happen when selecting with the mouse. */
	    kinsert('\n');
	}
    }
    return s;
}

int
yankregion(void)
{
    int s;

    kregcirculate(TRUE);
    ksetup();
    if (regionshape == FULLLINE) {
	kregflag |= KLINES | KYANK;
    } else if (regionshape == RECTANGLE) {
	kregflag |= KRECT | KYANK;
    }
    s = do_lines_in_region(yank_line, (void *) 0, TRUE);
    if (s && do_report(klines + (kchars != 0))) {
	mlwrite("[%d line%s, %d character%s yanked]",
		klines, PLURAL(klines),
		kchars, PLURAL(kchars));
    }
    kdone();
    ukb = 0;
    return (s);
}

#if VILE_NEEDED
int
_blankchar(int c)
{
    if (!isSpace(c))
	return ' ';
    return -1;
}
#endif

static int
_to_lower(int c)
{
    if (isUpper(c))
	return toLower(c);
    return -1;
}

static int
_to_upper(int c)
{
    if (isLower(c))
	return toUpper(c);
    return -1;
}

static int
_to_caseflip(int c)
{
    if (isAlpha(c)) {
	if (isUpper(c))
	    return toLower(c);
	else
	    return toUpper(c);
    }
    return -1;
}

/*
 * turn region to whitespace
 */
int
blank_region(void)
{
    return do_lines_in_region(blankline, (void *) NULL, FALSE);
}

int
flipregion(void)
{
    charprocfunc = _to_caseflip;
    return do_lines_in_region(do_chars_in_line, (void *) NULL, TRUE);
}

int
lowerregion(void)
{
    charprocfunc = _to_lower;
    return do_lines_in_region(do_chars_in_line, (void *) NULL, TRUE);
}

int
upperregion(void)
{
    charprocfunc = _to_upper;
    return do_lines_in_region(do_chars_in_line, (void *) NULL, TRUE);
}

/* finish filling in the left/right column info for a rectangular region */
static void
set_rect_columns(REGION * rp)
{
    if (regionshape == RECTANGLE) {
	MARK lc, rc;

	lc = (rp->r_orig.o < rp->r_end.o) ? rp->r_orig : rp->r_end;
	rc = (rp->r_orig.o > rp->r_end.o) ? rp->r_orig : rp->r_end;

	/* convert to columns */
	rp->r_leftcol = getcol(lc, FALSE);
	rp->r_rightcol = getcol(rc, FALSE) + 1;
    }
}

static int
found_region(REGION * rp)
{
    if (wantregion != 0)
	*wantregion = *rp;
    return TRUE;
}

/*
 * This routine figures out the bounds of the region in the current window, and
 * fills in the fields of the "REGION" structure pointed to by "rp".  Because
 * the dot and mark are usually very close together, we scan outward from dot
 * looking for mark.  This should save time.  Return a standard code.
 */
int
getregion(REGION * rp)
{
    LINE *flp;
    LINE *blp;
    B_COUNT fsize;
    B_COUNT bsize;
    int len_rs = len_record_sep(curbp);

    if (haveregion) {
	*rp = *haveregion;
	haveregion = NULL;
	return found_region(rp);
    }
#if OPT_SELECTIONS
    rp->r_attr_id = (USHORT) assign_attr_id();
#endif

    /*
     * If the buffer is completely empty, the region has to match.
     */
    if (valid_buffer(curbp) && is_empty_buf(curbp)) {
	memset(rp, 0, sizeof(*rp));
	rp->r_orig.l = rp->r_end.l = buf_head(curbp);
	return TRUE;
    }

    if (MK.l == NULL) {
	mlforce("BUG: getregion: no mark set in this window");
	return FALSE;
    }

    if (sameline(DOT, MK)) {
	rp->r_orig =
	    rp->r_end = DOT;
	if (regionshape == FULLLINE) {
	    rp->r_orig.o =
		rp->r_end.o = w_left_margin(curwp);
	    rp->r_end.l = lforw(DOT.l);
	    rp->r_size = (B_COUNT) (line_length(DOT.l) - w_left_margin(curwp));
	} else {
	    if (DOT.o < MK.o) {
		rp->r_orig.o = rp->r_leftcol = DOT.o;
		rp->r_end.o = rp->r_rightcol = MK.o;
	    } else {
		rp->r_orig.o = rp->r_leftcol = MK.o;
		rp->r_end.o = rp->r_rightcol = DOT.o;
	    }
	    rp->r_size = rp->r_end.o - rp->r_orig.o;
	    set_rect_columns(rp);
	}
	return found_region(rp);
    }
#if !SMALLER
    if (b_is_counted(curbp)) {	/* we have valid line numbers */
	LINE *flp_start;
	L_NUM dno, mno;
	dno = line_no(curbp, DOT.l);
	mno = line_no(curbp, MK.l);
	if (mno > dno) {
	    flp = DOT.l;
	    blp = MK.l;
	    rp->r_orig = DOT;
	    rp->r_end = MK;
	} else {
	    flp = MK.l;
	    blp = DOT.l;
	    rp->r_orig = MK;
	    rp->r_end = DOT;
	}
	fsize = (B_COUNT) (line_length(flp) -
			   ((regionshape == FULLLINE) ?
			    w_left_margin(curwp) : rp->r_orig.o));
	flp_start = flp;
	while (flp != blp) {
	    flp = lforw(flp);
	    if (flp != buf_head(curbp) && flp != flp_start) {
		fsize += line_length(flp) - w_left_margin(curwp);
	    } else if (flp != blp) {
		mlforce("BUG: hit buf end in getregion");
		return FALSE;
	    }

	    if (flp == blp) {
		if (regionshape == FULLLINE) {
		    rp->r_orig.o =
			rp->r_end.o = w_left_margin(curwp);
		    rp->r_end.l = lforw(rp->r_end.l);
		} else {
		    fsize -=
			(line_length(flp) - rp->r_end.o);
		    set_rect_columns(rp);
		}
		rp->r_size = fsize;
		return found_region(rp);
	    }
	}
    } else
#endif /* !SMALLER */
    {
	blp = DOT.l;
	flp = DOT.l;
	if (regionshape == FULLLINE) {
	    bsize = fsize =
		(B_COUNT) (line_length(blp) - w_left_margin(curwp));
	} else {
	    bsize = (B_COUNT) (DOT.o - w_left_margin(curwp));
	    fsize = (B_COUNT) (line_length(flp) - DOT.o);
	}
	while ((flp != buf_head(curbp)) ||
	       (lback(blp) != buf_head(curbp))) {
	    if (flp != buf_head(curbp)) {
		flp = lforw(flp);
		if (flp != buf_head(curbp))
		    fsize += line_length(flp) - w_left_margin(curwp);
		if (flp == MK.l) {
		    rp->r_orig = DOT;
		    rp->r_end = MK;
		    if (regionshape == FULLLINE) {
			rp->r_orig.o =
			    rp->r_end.o = w_left_margin(curwp);
			rp->r_end.l = lforw(rp->r_end.l);
		    } else {
			fsize -= (line_length(flp) - MK.o);
			set_rect_columns(rp);
		    }
		    rp->r_size = fsize;
		    return found_region(rp);
		}
	    }
	    if (lback(blp) != buf_head(curbp)) {
		blp = lback(blp);
		bsize += line_length(blp) - w_left_margin(curwp);
		if (blp == MK.l) {
		    rp->r_orig = MK;
		    rp->r_end = DOT;
		    if (regionshape == FULLLINE) {
			rp->r_orig.o =
			    rp->r_end.o = w_left_margin(curwp);
			rp->r_end.l = lforw(rp->r_end.l);
		    } else {
			bsize -= (MK.o - w_left_margin(curwp));
			set_rect_columns(rp);
		    }
		    rp->r_size = bsize;
		    return found_region(rp);
		}
	    }
	}
    }
    mlforce("BUG: lost mark");
    return FALSE;
}

int
get_fl_region(REGION * rp)
{
    int status;

    regionshape = FULLLINE;
    status = getregion(rp);
    regionshape = EXACT;

    return status;
}

#if OPT_SELECTIONS

typedef struct {
    TBUFF **enc_list;
    REGION enc_region;
} ENCODEREG;

static void
encode_one_attribute(TBUFF **result, int count, char *hypercmd, VIDEO_ATTR attr)
{
    char temp[80];

    tb_append(result, CONTROL_A);
    lsprintf(temp, "%d", count);
    tb_sappend(result, temp);

    if (attr & VAUL)
	tb_append(result, 'U');
    if (attr & VABOLD)
	tb_append(result, 'B');
    if (attr & (VAREV | VASEL))
	tb_append(result, 'R');
    if (attr & VAITAL)
	tb_append(result, 'I');

    if (attr & VACOLOR) {
	lsprintf(temp, "C%X", VCOLORNUM(attr));
	tb_sappend(result, temp);
    }
#if OPT_HYPERTEXT
    if (hypercmd != 0) {
	tb_append(result, 'H');
	tb_sappend(result, hypercmd);
	tb_append(result, EOS);
    }
#endif
    tb_sappend(result, ":");
}

static void
recompute_regionsize(REGION * region)
{
    MARK save_DOT;
    MARK save_MK;
    save_DOT = DOT;
    save_MK = MK;
    DOT = region->r_orig;
    MK = region->r_end;
    getregion(region);
    DOT = save_DOT;
    MK = save_MK;
}

/*
 * Compute the corresponding text of the given line with attributes expanded
 * back to ^A-sequences.
 */
TBUFF *
encode_attributes(LINE *lp, BUFFER *bp, REGION * top_region)
{
    TBUFF *result = 0;
    int j, k, len;

    if ((len = llength(lp)) > 0) {
	AREGION *ap;
	AREGION *my_list = 0;
	AREGION ar_temp;
	int my_used = 0;
	int my_size = 0;
	L_NUM top_rsl = line_no(bp, top_region->r_orig.l);
	L_NUM top_rel = line_no(bp, top_region->r_end.l);
	L_NUM tst_rsl, tst_rel;
	L_NUM this_line = line_no(bp, lp);

	tb_init(&result, EOS);
	memset(&ar_temp, 0, sizeof(ar_temp));

	/*
	 * Make a list of the attribute-regions which include this line.
	 * If they do not start/end with this line, we will compare against
	 * the top_region within which we are encoding, and compute an adjusted
	 * attribute region which does not extend out of that region.
	 */
	for (ap = bp->b_attribs; ap != NULL; ap = ap->ar_next) {
	    int found = 0;

	    if (lp == ap->ar_region.r_orig.l) {
		ar_temp = *ap;
		found = 1;
		if (lp != ap->ar_region.r_end.l) {
		    found = -1;
		    tst_rel = line_no(bp, ap->ar_region.r_end.l);
		    if (tst_rel > top_rel) {
			ar_temp.ar_region.r_end = top_region->r_end;
		    }
		}
	    } else if (this_line == top_rsl) {
		tst_rsl = line_no(bp, ap->ar_region.r_orig.l);
		tst_rel = line_no(bp, ap->ar_region.r_end.l);
		if (tst_rsl <= this_line && tst_rel >= this_line) {
		    ar_temp = *ap;
		    found = -2;
		    if (tst_rsl < top_rsl)
			ar_temp.ar_region.r_orig = top_region->r_orig;
		    if (tst_rel > top_rel)
			ar_temp.ar_region.r_end = top_region->r_end;
		}
	    }
	    if (found) {
		if (found < 0) {	/* recompute limits */
		    recompute_regionsize(&ar_temp.ar_region);
		}
		if (my_list == 0)
		    my_list = typeallocn(AREGION, my_size = 10);
		else if (my_used + 1 > my_size)
		    my_list = typereallocn(AREGION, my_list, my_size *= 2);
		if (my_list == 0) {
		    no_memory("encode_attributes");
		    return 0;
		}
		my_list[my_used++] = ar_temp;
	    }
	}

	/*
	 * Now, walk through the line, emitting attributes as needed before
	 * each character.
	 */
	for (j = 0; j < len; ++j) {
	    /* get the buffer attributes for this line, by column */
	    for (k = 0; k < my_used; ++k) {
		if (j == my_list[k].ar_region.r_orig.o) {
		    encode_one_attribute(&result,
					 my_list[k].ar_region.r_size,
#if OPT_HYPERTEXT
					 my_list[k].ar_hypercmd,
#else
					 (char *) 0,
#endif
					 my_list[k].ar_vattr);
		}
	    }
#if OPT_LINE_ATTRS
	    if (lp->l_attrs != 0) {
		if (lp->l_attrs[j] > 1
		    && (j == 0 || lp->l_attrs[j - 1] != lp->l_attrs[j])) {
		    for (k = j + 1; k < len; ++k) {
			if (lp->l_attrs[j] != lp->l_attrs[k])
			    break;
		    }
		    encode_one_attribute(&result, k - j, (char *) 0,
					 line_attr_tbl[lp->l_attrs[j]].vattr);
		}
	    }
#endif
	    if (tb_append(&result, lgetc(lp, j)) == 0) {
		break;
	    }
	}
	FreeIfNeeded(my_list);
	if (result == 0)
	    (void) no_memory("encode_attributes");
    }
    return result;
}

static int
encode_line(void *flagp GCC_UNUSED, int l GCC_UNUSED, int r GCC_UNUSED)
{
    LINE *lp = DOT.l;
    TBUFF *text;
    ENCODEREG *work = (ENCODEREG *) flagp;
    L_NUM this_line = line_no(curbp, lp);
    L_NUM base_line = line_no(curbp, work->enc_region.r_orig.l);

    if (llength(lp) != 0) {
	if ((text = encode_attributes(lp, curbp, &(work->enc_region))) == 0) {
	    return FALSE;
	}
	work->enc_list[this_line - base_line] = text;
    } else {
	work->enc_list[this_line - base_line] = 0;
    }
    return TRUE;
}

/*
 * This builds an array of TBUFF's which represents the result of the encoding.
 * When the encoding is complete, we can use it to replace the region.  That
 * avoids entanglement with the attribute pointers moving around.
 */
static int
encode_region(void)
{
    ENCODEREG work;
    int status;
    L_NUM base_line;
    L_NUM last_line;
    L_NUM total, n;

    TRACE((T_CALLED "encode_region()\n"));

    regionshape = FULLLINE;
    if ((status = getregion(&work.enc_region)) == TRUE
	&& (base_line = line_no(curbp, work.enc_region.r_orig.l)) > 0
	&& (last_line = line_no(curbp, work.enc_region.r_end.l)) > base_line) {
	total = (last_line - base_line);
	if ((work.enc_list = typeallocn(TBUFF *, total)) == 0) {
	    status = no_memory("encode_region");
	} else {
	    status = do_lines_in_region(encode_line, (void *) &work, FALSE);

	    if (status == TRUE) {
		/*
		 * Clear attributes for the range we're operating upon, and
		 * disable highlighting to prevent autocolor from trying to
		 * fix it.
		 */
		videoattribute = 0;
		attributeregion();
#if OPT_MAJORMODE
		make_local_b_val(curbp, MDHILITE);
		set_b_val(curbp, MDHILITE, FALSE);
#endif

		DOT = work.enc_region.r_orig;
		for (n = 0; n < total; ++n) {
		    TBUFF *text = work.enc_list[n];

		    DOT.o = b_left_margin(curbp);
		    regionshape = EXACT;
		    deltoeol(TRUE, 1);
		    DOT.o = b_left_margin(curbp);
		    lstrinsert(tb_values(text), (int) tb_length(text));

		    forwbline(FALSE, 1);
		    gotobol(FALSE, 1);
		    tb_free(&(text));
		}
	    }
	    free(work.enc_list);
	}
    }

    returnCode(status);
}

/*
 * Decode the current attributes back to ^A-sequences.
 */
int
operattrencode(int f, int n)
{
    opcmd = OPOTHER;
    return vile_op(f, n, encode_region, "Encoding");
}

/*
 * Write attributed text to a file
 */
int
oper_wrenc(int f, int n)
{
    int s;
    char fname[NFILEN];

    if (ukb != 0) {
	if ((s = mlreply_file("Write to file: ", (TBUFF **) 0,
			      FILEC_WRITE | FILEC_PROMPT, fname)) != TRUE)
	    return s;
	return kwrite(fname, TRUE);
    } else {
	opcmd = OPOTHER;
	return vile_op(f, n, write_enc_region, "File write (encoded)");
    }
}

#endif /* OPT_SELECTIONS */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          