/*
 * w32cmd:  collection of functions that add Win32-specific editor
 *          features (modulo the clipboard interface) to [win]vile.
 *
 * $Header: /usr/build/vile/vile/RCS/w32cmd.c,v 1.37 2006/04/21 11:56:00 tom Exp $
 */

#include "estruct.h"
#include "edef.h"
#include "nefunc.h"
#include "winvile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commdlg.h>
#include <direct.h>

#define FOOTER_OFFS      0.333  /* inches from bottom of page */
#define _FF_             '\f'

#define REGKEY_RECENT_FILES VILE_SUBKEY "\\MRUFiles"
#define REGKEY_RECENT_FLDRS VILE_SUBKEY "\\MRUFolders"
#define RECENT_REGVALUE_FMT "%02X"

/* --------------------------------------------------------------------- */
/* ----------------------- misc common routines ------------------------ */
/* --------------------------------------------------------------------- */

static DWORD
comm_dlg_error(void)
{
    DWORD errcode;

    if ((errcode = CommDlgExtendedError()) != 0)
        mlforce("[CommDlgExtendedError() returns err code %d]", errcode);
    return (errcode);
}

/* --------------------------------------------------------------------- */
/* -------- commdlg-based routines that open and save files ------------ */
/* --------------------------------------------------------------------- */

static OPENFILENAME ofn;
static int          ofn_initialized;

static void
ofn_init(void)
{
    static char         custFilter[128] = "All Files (*.*)\0*.*\0";
    static char         filter[]        =
"C/C++ Files (*.c;*.cpp;*.h;*.rc)\0*.c;*.cpp;*.h;*.rc\0Text Files (*.txt)\0*.txt\0\0";

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize       = sizeof(ofn);
    ofn.lpstrFilter       = filter;
    ofn.lpstrCustomFilter = custFilter;
    ofn.nMaxCustFilter    = sizeof(custFilter);
    ofn_initialized       = TRUE;
}



/* glob a canonical filename, and if a true directory, put it in win32 fmt */
int
w32_glob_and_validate_dir(const char *inputdir, char *outputdir)
{
#define NOT_A_DIR  "[\"%s\" is not a directory]"

    int  outlen, rc;
    char tmp[NFILEN];

    strcpy(outputdir, inputdir);
    if (! doglob(outputdir))      /* doglob updates outputdir */
        rc = FALSE;
    else
    {
        /*
         * GetOpenFileName()/GetSaveFileName() will accept bogus initial dir
         * values without reporting an error.  Handle that.
         */

        if (! is_directory(outputdir))
        {
            outlen = (term.cols - 1) - (sizeof(NOT_A_DIR) - 3);
            mlforce(NOT_A_DIR, path_trunc(outputdir, outlen, tmp, sizeof(tmp)));
            rc = FALSE;
        }
        else
        {
            strcpy(outputdir, sl_to_bsl(outputdir));
            rc = TRUE;
        }
    }
    return (rc);

#undef NOT_A_DIR
}



/*
 * FUNCTION
 *   commdlg_open_files(int chdir_allowed, const char *dir)
 *
 *   chdir_allowed - permissible for GetOpenFileName() to cd to the
 *                   folder that stores the file(s) the user opens.
 *
 *   dir           - path of directory to browse.  If NULL, use cwd.
 *
 * DESCRIPTION
 *   Use the Windows common dialog library to open one or more files.
 *
 * RETURNS
 *   Boolean, T -> all is well.  F -> error noted and reported.
 */

static int
commdlg_open_files(int chdir_allowed, const char *dir)
{
#define RET_BUF_SIZE_ (24 * 1024)

    int   chdir_mask, i, len, nfile, rc = TRUE, status;
    char  *filebuf;
    char  oldcwd[FILENAME_MAX];
    char  newcwd[FILENAME_MAX];
    char  *cp;
    char  validated_dir[FILENAME_MAX];

    TRACE((T_CALLED "commdlg_open_files(chdir_allowed=%d, dir=%s)\n",
           chdir_allowed, TRACE_NULL(dir)));

    if (dir)
    {
        if (! w32_glob_and_validate_dir(dir, validated_dir))
            returnCode(FALSE);
        else
            dir = validated_dir;

    }
    oldcwd[0]  = newcwd[0] = '\0';
    chdir_mask = (chdir_allowed) ? 0 : OFN_NOCHANGEDIR;
    filebuf    = typeallocn(char, RET_BUF_SIZE_);
    if (! filebuf)
        returnCode(no_memory("commdlg_open_files()"));
    filebuf[0] = '\0';
    if (! ofn_initialized)
        ofn_init();
    (void) _getcwd(oldcwd, sizeof(oldcwd));
    ofn.lpstrInitialDir = (dir) ? dir : oldcwd;
    ofn.lpstrFile       = filebuf;
    ofn.nMaxFile        = RET_BUF_SIZE_;
    ofn.Flags           = (OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                          OFN_ALLOWMULTISELECT | OFN_EXPLORER) | chdir_mask;
#if DISP_NTWIN
    ofn.hwndOwner       = winvile_hwnd();
#else
    ofn.hwndOwner       = GetForegroundWindow();
#endif
    status              = GetOpenFileName(&ofn);

#if DISP_NTCONS
    /* attempt to restore focus to the console editor */
    (void) SetForegroundWindow(ofn.hwndOwner);
#endif

    if (! status)
    {
        /* user CANCEL'd the dialog box (err code 0), else some other error. */

        if (comm_dlg_error() == 0)
        {
            /* canceled -- restore old cwd if necessary */

            if (chdir_allowed)
            {
                if (stricmp(newcwd, oldcwd) != 0)
                    (void) chdir(oldcwd);
            }
        }
        else
            rc = FALSE;  /* Win32 error */
        free(filebuf);
        returnCode(rc);
    }
    if (chdir_allowed)
    {
        /* tell editor cwd changed, if it did... */

        if (! _getcwd(newcwd, sizeof(newcwd)))
        {
            free(filebuf);
            mlerror("_getcwd() failed");
            returnCode(FALSE);
        }
        if (stricmp(newcwd, oldcwd) != 0)
        {
            if (! set_directory(newcwd))
            {
                free(filebuf);
                returnCode(FALSE);
            }
        }
    }

    /*
     * Got a list of one or more files in filebuf (each nul-terminated).
     * Make one pass through the list to count the number of files
     * returned.  If #files > 0, then first file in the list is actually
     * the returned files' folder (er, directory).
     */
    cp    = filebuf;
    nfile = 0;
    for_ever
    {
        len = (int) strlen(cp);
        if (len == 0)
            break;
        nfile++;
        cp += len + 1;  /* skip filename and its terminator */
    }
    if (nfile)
    {
        BUFFER *bp, *first_bp;
        char   *dir2 = 0, tmp[FILENAME_MAX * 2], *nxtfile;
        int    have_dir;

        if (nfile > 1)
        {
            /* first "file" in the list is actually a folder */

            have_dir = 1;
            dir2     = filebuf;
            cp       = dir2 + strlen(dir2) + 1;
            nfile--;
        }
        else
        {
            have_dir = 0;
            cp       = filebuf;
        }
        for (i = 0, first_bp = NULL; rc && i < nfile; i++)
        {
            if (have_dir)
            {
                sprintf(tmp, "%s\\%s", dir2, cp);
                nxtfile = tmp;
            }
            else
                nxtfile = cp;
            if ((bp = getfile2bp(nxtfile, FALSE, FALSE)) == 0)
                rc = FALSE;
            else
            {
                bp->b_flag |= BFARGS;   /* treat this as an argument */
                if (! first_bp)
                    first_bp = bp;
                cp += strlen(cp) + 1;
            }
        }
        if (rc)
            rc = swbuffer(first_bp);  /* editor switches to 1st buffer */
    }

    /* Cleanup */
    free(filebuf);
    returnCode(rc);

#undef RET_BUF_SIZE_
}



static int
wopen_common(int chdir_allowed)
{

    char         dir[NFILEN];
    static TBUFF *last;
    int          rc;

    dir[0] = '\0';
    rc     = mlreply_dir("Directory: ", &last, dir);
    if (rc == TRUE)
    {
        mktrimmed(dir);
        rc = commdlg_open_files(chdir_allowed, (dir[0]) ? dir : NULL);
    }
    else if (rc == FALSE)
    {
        /* empty response */

        rc = commdlg_open_files(chdir_allowed, NULL);
    }
    /* else rc == ABORT or SORTOFTRUE */
    return (rc);
}



int
winopen_nocd(int f, int n)
{
    return (wopen_common(FALSE));
}



int
winopen(int f, int n)
{
    return (wopen_common(TRUE));
}



/* explicitly specify initial directory */
int
winopen_dir(const char *dir)
{
    return (commdlg_open_files(TRUE, dir));
}



/*
 * FUNCTION
 *   commdlg_save_file(int chdir_allowed, char *dir)
 *
 *   chdir_allowed - permissible for GetSaveFileName() to cd to the
 *                   folder that stores the file the user saves.
 *
 *   dir           - path of directory to browse.  If NULL, use cwd.
 *
 * DESCRIPTION
 *   Use the Windows common dialog library to save the current buffer in
 *   a filename and folder of the user's choice.
 *
 * RETURNS
 *   Boolean, T -> all is well.  F -> error noted and reported.
 */

static int
commdlg_save_file(int chdir_allowed, const char *dir)
{
    int   chdir_mask, rc = TRUE, status;
    char  filebuf[FILENAME_MAX], validated_dir[FILENAME_MAX],
          oldcwd[FILENAME_MAX], newcwd[FILENAME_MAX], *fname, *leaf,
          scratch[FILENAME_MAX];

    if (dir)
    {
        if (! w32_glob_and_validate_dir(dir, validated_dir))
            return (FALSE);
        else
            dir = validated_dir;

    }

    /*
     * The "Save As" dialog box is traditionally initialized with the
     * name of the current open file.  Handle that detail now.
     */
    fname      = curbp->b_fname;
    filebuf[0] = '\0';           /* copy the currently open filename here */
    if (! is_internalname(fname))
    {
        strcpy(scratch, fname);
        if ((leaf = last_slash(scratch)) == NULL)
        {
            /* easy case -- fname is a pure leaf */

            leaf = scratch;
        }
        else
        {
            /* fname specifies a path */

            *leaf++ = '\0';
            if (! dir)
            {
                /* save dirspec from path */

                dir = sl_to_bsl(scratch);
            }
        }
        strcpy(filebuf, leaf);
    }
    oldcwd[0]  = newcwd[0] = '\0';
    chdir_mask = (chdir_allowed) ? 0 : OFN_NOCHANGEDIR;
    if (! ofn_initialized)
        ofn_init();
    (void) _getcwd(oldcwd, sizeof(oldcwd));
    ofn.lpstrInitialDir = (dir) ? dir : oldcwd;
    ofn.lpstrFile       = filebuf;
    ofn.nMaxFile        = sizeof(filebuf);
    ofn.Flags           = (OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                          OFN_OVERWRITEPROMPT | OFN_EXPLORER) | chdir_mask;
#if DISP_NTWIN
    ofn.hwndOwner       = winvile_hwnd();
#else
    ofn.hwndOwner       = GetForegroundWindow();
#endif
    status              = GetSaveFileName(&ofn);

#if DISP_NTCONS
    /* attempt to restore focus to the console editor */
    (void) SetForegroundWindow(ofn.hwndOwner);
#endif

    if (! status)
    {
        /* user CANCEL'd the dialog box (err code 0), else some other error. */

        if (comm_dlg_error() == 0)
        {
            /* canceled -- restore old cwd if necessary */

            if (chdir_allowed)
            {
                if (stricmp(newcwd, oldcwd) != 0)
                    (void) chdir(oldcwd);
            }
        }
        else
            rc = FALSE;  /* Win32 error */
        return (rc);
    }
    if (chdir_allowed)
    {
        /* tell editor cwd changed, if it did... */

        if (! _getcwd(newcwd, sizeof(newcwd)))
        {
            mlerror("_getcwd() failed");
            return (FALSE);
        }
        if (stricmp(newcwd, oldcwd) != 0)
        {
            if (! set_directory(newcwd))
                return (FALSE);
        }
    }

    /*
     * Now mimic these standard vile commands to change the buffer name
     * and write its contents to disk:
     *
     *       :f
     *       :w
     */
    /* ------------ begin :f ------------ */
#if OPT_LCKFILES
    if ( global_g_val(GMDUSEFILELOCK) ) {
        if (!b_val(curbp,MDLOCKED) && !b_val(curbp,MDVIEW))
            release_lock(curbp->b_fname);
        ch_fname(curbp, fname);
        make_global_b_val(curbp,MDLOCKED);
        make_global_b_val(curbp,VAL_LOCKER);
        grab_lck_file(curbp, fname);
    } else
#endif
        ch_fname(curbp, filebuf);

    /* remember new filename if later written out to disk */
    b_clr_registered(curbp);

    curwp->w_flag |= WFMODE;
    updatelistbuffers();
    /* ------------ begin :w ------------ */
    return (filesave(0, 0));
}



static int
wsave_common(int chdir_allowed)
{

    char         dir[NFILEN];
    static TBUFF *last;
    int          rc;

    dir[0] = '\0';
    rc     = mlreply_dir("Directory: ", &last, dir);
    if (rc == TRUE)
    {
        mktrimmed(dir);
        rc = commdlg_save_file(chdir_allowed, (dir[0]) ? dir : NULL);
    }
    else if (rc == FALSE)
    {
        /* empty user response */

        rc = commdlg_save_file(chdir_allowed, NULL);
    }
    /* else rc == ABORT or SORTOFTRUE */
    return (rc);
}



int
winsave(int f, int n)
{
    return (wsave_common(TRUE));
}


int
winsave_nocd(int f, int n)
{
    return (wsave_common(FALSE));
}



/* explicitly specify initial directory */
int
winsave_dir(const char *dir)
{
    return (commdlg_save_file(TRUE, dir));
}

/* --------------------------------------------------------------------- */
/* ------------------ delete current text selection -------------------- */
/* --------------------------------------------------------------------- */
int
windeltxtsel(int f, int n)  /* bound to Alt+Delete */
{
    return (w32_del_selection(FALSE));
}

/* --------------------------------------------------------------------- */
/* ------------------------- printing support -------------------------- */
/* --------------------------------------------------------------------- */
/* - Note that a good deal of this implementation is based on chapters - */
/* - 4 & 15 of Petzold's "Programming Windows 95". --------------------- */
/* --------------------------------------------------------------------- */

#if DISP_NTWIN                 /* Printing is only supported for winvile.
                                * There are hooks in the code to support
                                * this feature in console vile, but the
                                * following items require attention:
                                *
                                * - console vile users must be able to
                                *   specify a printing font (via a state var)
                                * - some mechanism must be added to permit
                                *   emulation of a modeless "cancel printing"
                                *   dialog box (it's not possible for console
                                *   apps to create modeless dlg boxes).
                                *
                                * A query of the vile user base returned
                                * zero interest in this feature, so there's
                                * not much incentive to add the missing
                                * pieces.
                                */

typedef struct print_param_struct
{
    char *        buf;         /* scratch formatting buffer, large enough to
                                * hold mcpl chars + line numbers, if any.
                                */
    int           collate,     /* T -> print all copies of page 1 first,
                                * then all copies of page 2, etc.
                                */
                  endpg_req,   /* T -> "end page" operation pending      */
                  mcpl,        /* max printable chars per line           */
                  mlpp,        /* max lines per page                     */
                  ncopies,
                  pagenum,     /* running page count                     */
                  plines,      /* running count of lines printed         */
                  xchar,       /* avg char width                         */
                  ychar,       /* max character height                   */
                  xorg,        /* X coord viewport origin -- strips out the
                                * unprintable margin reserved by the printer.
                                */
                  yorg,        /* Y coord viewport origin                */
                  ypos;        /* Y coord of "cursor"  on output page.   */
    HFONT         hfont;       /* printing font handle                   */
} PRINT_PARAM;

typedef struct split_line_struct
{
    LINE *        splitlp;     /* points at a buffer line that has been split
                                * across a page break while printing multiple,
                                * uncollated copies.
                                */
    ULONG         outlen;      /* #bytes of splitlp that were "output" prior
                                * to page break.
                                */
} SPLIT_LINE;


static int  printing_aborted,  /* T -> user aborted print job.     */
            print_low, print_high, print_tabstop,
            print_number,      /* T -> line numbering is in effect */
            pgsetup_chgd,      /* T -> user invoked page setup dlg
                                *      and did not cancel out
                                */
            printdlg_chgd,     /* T -> user invoked print dialog and
                                *      did not cancel out
                                */
            spooler_failure,
            yfootpos;          /* device Y coordinate of the footer */

/* selection printing requires a few saved state vars */
static REGIONSHAPE oshape;

static PAGESETUPDLG *pgsetup;
static PRINTDLG     *pd;

static RECT printrect;         /* Area where printing is allowed, includes
                                * the ruser's preferred margin (or a default).
                                * This area is  scaled in device coordinates
                                * (pixels).
                                */

static HWND hDlgCancelPrint,
            hPrintWnd;         /* window that spawned print request */

/*
 * GetLastError() doesn't return a legitimate Win32 error if the spooler
 * encounters problems.  How inconvenient.
 */
static void
display_spooler_error(HWND hwnd)
{
#define ERRPREFIX "Spooler Error: "

    static const char *spooler_errs[] =
    {
        "general error",
        "canceled from program",
        "canceled by user",
        "out of disk space",
        "out of memory",
    };
    char       buf[256], *cp;
    long       errcode,
               max_err = (sizeof(spooler_errs) / sizeof(spooler_errs));

    errcode         = GetLastError();
    spooler_failure = TRUE;
    if ((errcode & SP_NOTREPORTED) == 0)
        return;
    errcode *= -1;
    strcpy(buf, ERRPREFIX);
    cp = buf + (sizeof(ERRPREFIX) - 1);
    if (errcode >= max_err || errcode < 0)
        sprintf(cp, "unknown error code (%ld)", errcode);
    else
        strcat(cp, spooler_errs[errcode]);
    MessageBox(hwnd, buf, prognam, MB_ICONSTOP | MB_OK);

#undef ERRPREFIX
}



/* Obtain/adjust default info about the printed page. */
static int
handle_page_dflts(HWND hwnd)
{
    RECT m;

    if (pgsetup == NULL)
    {
        if ((pgsetup = typecalloc(PAGESETUPDLG)) == NULL)
            return (no_memory("get_page_dflts"));
        pgsetup->Flags       = PSD_RETURNDEFAULT|PSD_DEFAULTMINMARGINS;
        pgsetup->lStructSize = sizeof(*pgsetup);

        /* ask system to return current settings without showing dlg box */
        if (! PageSetupDlg(pgsetup))
        {
            if (comm_dlg_error() == 0)
            {
                /* uh-oh */

                mlforce("BUG: PageSetupDlg() fails default data call");
            }
            return (FALSE);
        }

        /*
         * Now, configure the actual default margins used by the program,
         * which are either 0.5 inches or 125 mm, as appropriate for the
         * desktop env.
         */
        if (pgsetup->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
            m.bottom = m.top = m.left = m.right = 1250;
        else
            m.bottom = m.top = m.left = m.right = 500;
        pgsetup->rtMargin  = m;
        pgsetup->Flags    &= ~PSD_RETURNDEFAULT;
        pgsetup->Flags    |= PSD_MARGINS;
        pgsetup->hwndOwner = hwnd;
    }
    return (TRUE);
}



/*
 * Handles two chores:
 *
 *   1) converts all user-specified margin values to thousandths of inches,
 *      if required, prior to rationalizing.
 *
 *   2) rationalizes user's margin settings with respect to the printer's
 *      unprintable page border (typically quite small--1/4" for a LJ4Plus).
 *
 * CAVEATS:  call handle_page_dflts() before this function.
 */
static void
compute_printrect(int  xdpi,    /* dots (pixels) per inch in x axis */
                  int  ydpi,    /* dots (pixels) per inch in y axis */
                  RECT *minmar, /* unprintable offsets (pixels)     */
                  int  horzres, /* max horizontal print pixels      */
                  int  vertres  /* max vertical print pixels        */
                  )
{
    RECT u;

    /* Extract user's preferred margins, scaled in * 0.001in or 0.01mm . */
    u = pgsetup->rtMargin;

    if (pgsetup->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
    {
        /* convert user's margins to thousandths of inches */

        u.top    = (int) (1000.0 * u.top / 2500.0);
        u.bottom = (int) (1000.0 * u.bottom / 2500.0);
        u.right  = (int) (1000.0 * u.right / 2500.0);
        u.left   = (int) (1000.0 * u.left / 2500.0);
    }

    /*
     * now, convert preferred margins to device coordinates, in standard
     * rectangular notation
     */
    u.top    = (int) (u.top * ydpi / 1000.0);
    u.bottom = minmar->top + minmar->bottom + vertres -
                                    ((int) (u.bottom * ydpi / 1000.0));
    u.left   = (int) (u.left * xdpi / 1000.0);
    u.right  = minmar->left + minmar->right + horzres -
                                    ((int) (u.right * xdpi / 1000.0));

    /*
     * Compute maximum printing rectangle coordinates (pixels).  Note that
     * many printers define a minimum margin -- printing not allowed there.
     */
    printrect.top    = minmar->top;
    printrect.bottom = minmar->top + vertres;
    printrect.left   = minmar->left;
    printrect.right  = minmar->left + horzres;

    /* Factor in user's preferred margins. */
    if (u.top > printrect.top)
        printrect.top += u.top - printrect.top;
    if (u.bottom < printrect.bottom)
        printrect.bottom -= printrect.bottom - u.bottom;
    if (u.left > printrect.left)
        printrect.left += u.left - printrect.left;
    if (u.right < printrect.right)
        printrect.right -= printrect.right - u.right;
}



static void
compute_foot_hdr_pos(int  ydpi,    /* dots (pixels) per inch in y axis */
                     RECT *minmar, /* unprintable offsets (pixels)     */
                     int  ychar,   /* max character height (pixels)    */
                     int  vertres  /* max vertical, printable pixels   */
                     )
{
    int    halfchar;
    double ymin;

    /*
     * FIXME -- once user-definable headers are allowed, this code needs
     * to handle the presence/absence of both headers and footers.
     *
     * For now, assume a hardwired footer--a centered, running page number.
     */

    /*
     * Consider the issue of the location of the footer and header.  We
     * could place the header/footer at the exact point of the minimum
     * margin, but that's not gonna work for printers that offer up the
     * entire page as a canvas.  So, we try to place the header and footer
     * at the printer minimums, but if that's too small, we pick arbitrary
     * values that look good (don't use up much real estate, but aren't on
     * the edge of the paper).
     */
    ymin = minmar->bottom / (double) ydpi;
    if (ymin < FOOTER_OFFS)
        ymin = FOOTER_OFFS;

    /*
     * In the following computation, "ychar" (equivalent height of one line)
     * is subtracted from the footer position so that the _bottom_ of the
     * footer text will align with the desired margin.
     */
    yfootpos = minmar->top + minmar->bottom + vertres -
                                             ((int) (ymin * ydpi)) - ychar;

    /*
     * if the header/footer positions overlap with the edges of the printing
     * rectangle plus a single blank line, adjust the printing rectangle out
     * of harm's way.
     */
    halfchar = ychar / 2 + 1;
    if (printrect.bottom + halfchar > yfootpos)
        printrect.bottom = yfootpos - halfchar;
}



/*
 * save current buffer and misc state info and then switch to the
 * current selection buffer.  some of the code in this function is based
 * on sel_yank().
 */
static WINDOW *
push_curbp(BUFFER *selbp)
{
    WINDOW *wp;

    if ((wp = push_fake_win(selbp)) == NULL)
        return (NULL);
    return (wp);
}



static void
pop_curbp(WINDOW *owp)
{
    pop_fake_win(owp, (BUFFER *)0);
}



static int
ck_empty_rgn_data(void *argp, int l, int r)
{
    LINE          *lp;
    int           rc = TRUE, vile_llen, *empty;

    lp = DOT.l;

    /* Rationalize offsets */
    if (llength(lp) < l)
        return (TRUE);
    if (r > llength(lp))
        r = llength(lp);
    vile_llen = r - l;
    if (vile_llen > 0)
    {
        empty  = argp;
        *empty = rc = FALSE;
    }
    return (rc);
}



static int
empty_text_selection(BUFFER *selbp, AREGION *psel)
{
    DORGNLINES  dorgn;
    int         empty = TRUE;
    WINDOW      *ocurwp;
    MARK        odot;

    if ((ocurwp = push_curbp(selbp)) == NULL)
        return (FALSE);  /* eh? */
    oshape      = regionshape;
    odot        = DOT;          /* do_lines_in_region() moves DOT. */
    dorgn       = get_do_lines_rgn();
    haveregion  = &psel->ar_region;
    regionshape = psel->ar_shape;
    (void) dorgn(ck_empty_rgn_data, &empty, TRUE);
    DOT         = odot;
    haveregion  = NULL;
    regionshape = oshape;
    pop_curbp(ocurwp);
    return (empty);
}



static BOOL CALLBACK
printer_abort_proc(HDC hPrintDC, int errcode)
{
    MSG msg;

    while (! printing_aborted && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (hDlgCancelPrint || ! IsDialogMessage(hDlgCancelPrint, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (! printing_aborted);
}



static DIALOG_PROC_RETVAL CALLBACK
printer_dlg_proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            w32_center_window(hDlg, hPrintWnd);
            EnableMenuItem(GetSystemMenu(hDlg, FALSE), SC_CLOSE, MF_GRAYED);
            return (TRUE);
        case WM_COMMAND:       /* user cancels printing */
            printing_aborted = TRUE;
            EnableWindow(GetParent(hDlg), TRUE);
            DestroyWindow(hDlg);
            hDlgCancelPrint = NULL;
            return (TRUE);
    }
    return (FALSE);
}



static void
winprint_cleanup(HFONT hfont_prev)
{
    if (hfont_prev)
        (void) DeleteObject(SelectObject(pd->hDC, hfont_prev));
    (void) EnableWindow(pd->hwndOwner, TRUE);
    (void) DeleteDC(pd->hDC);
}



static int
winprint_startpage(PRINT_PARAM *pparam)
{
    /* Petzold tests "< 0", API ref sez, "<= 0".  I trust Petzold */
    if (StartPage(pd->hDC) < 0)
    {
        display_spooler_error(pd->hwndOwner);
        return (FALSE);
    }

    /*
     * On a win9x host, StartPage() resets the device context and the
     * mapping mode.  Thanks a bunch, guys.
     */
    SelectObject(pd->hDC, pparam->hfont);
    SetMapMode(pd->hDC, MM_TEXT);

    /*
     * Likewise the Viewport.
     *
     * It's necessary here to adjust the viewport by the portion of the page
     * on which it's physically impossible to print.  Put another way,
     * printing_rect is biased with an (xorg, yorg) offset.
     */
    SetViewportOrgEx(pd->hDC, -pparam->xorg, -pparam->yorg, NULL);

    /* FIXME -- emit header */

    pparam->endpg_req = TRUE;
    return (TRUE);
}



static int
winprint_endpage(PRINT_PARAM *pparam)
{
    char     footbuf[256];
    UINT     footbuflen;
    int      rc = TRUE;
    UINT     txtmode;

    /* FIXME -- footer is hardwired as page number */
    txtmode    = (GetTextAlign(pd->hDC) & ~TA_LEFT);
    footbuflen = sprintf(footbuf, "%lu", pparam->pagenum);
    txtmode    = SetTextAlign(pd->hDC, txtmode | TA_CENTER);
    TextOut(pd->hDC,
            (printrect.left + printrect.right) / 2,
            yfootpos,
            footbuf,
            footbuflen);
    (void) SetTextAlign(pd->hDC, txtmode | TA_LEFT);

    /* Petzold tests "< 0", API ref sez, "<= 0".  I trust Petzold */
    if (EndPage(pd->hDC) < 0)
    {
        display_spooler_error(pd->hwndOwner);
        rc = FALSE;
    }
    pparam->endpg_req = FALSE;
    return (rc);
}



/*
 * FUNCTION
 *   winprint_fmttxt(char *dst,
 *                   char *src,
 *                   ULONG srclen,
 *                   ULONG mcpl,
 *                   ULONG line_num,
 *                   int   *saw_ff)
 *
 *   dst      - formatted output data
 *   src      - vile buffer LINE
 *   srclen   - length of LINE
 *   mcpl     - max chars per (printable) line.
 *   line_num - cur line number (for display purposes if the user has enabled
 *              numbering via ":se nu").  If 0, the data being output is
 *              wrapped and a line number should not be emitted.
 *   saw_ff   - Boolean, by ref, T -> input line contains a formfeed.
 *              Caller must process appropriately.
 *
 * DESCRIPTION
 *   Format a vile buffer LINE -- turning unprintable data into printable
 *   data and properly expanding tabs.
 *
 * RETURNS
 *   Number of src chars consumed.
 */

static ULONG
winprint_fmttxt(char *dst,
                char *src,
                ULONG srclen,
                ULONG mcpl,
                ULONG line_num,
                int   *saw_ff)
{
    UINT c;
    ULONG i, nout, nconsumed, orig_srclen = srclen, line_num_offset;

    *saw_ff   = FALSE;
    nconsumed = nout = line_num_offset = 0;
    if (print_number && line_num > 0)
    {
        nout = sprintf(dst, "%6lu  ", line_num);
        if (nout >= mcpl)
        {
            /* absurd printer output width -- skip line numbering */

            nout = 0;
        }
        else
        {
            dst += nout;
            line_num_offset = nout;
        }
    }

    /*
     * add a simple pre-formatting optimization:  if the current input is all
     * whitespace (modulo a formfeed), or empty, handle that case now.
     */
    i = 0;
    while (i < srclen && isSpace((UCHAR)src[i]) && src[i] != _FF_)
        i++;
    if (i == srclen)
    {
        /* input line is empty or all white space */

        *dst++ = ' ';
        *dst   = '\0';
        return (srclen);  /* collapsed entire input line to single byte */
    }

    while (srclen--)
    {
        c = (UCHAR) *src;
        if (c == _TAB_)
        {
            ULONG nspaces;

            nspaces = print_tabstop - (nout - line_num_offset) % print_tabstop;
            if (nout + nspaces > mcpl)
                break;
            nout += nspaces;
            for (i = 0; i < nspaces; i++)
                *dst++ = ' ';
        }
        else if  (c < _SPC_)  /* assumes ASCII char set  */
        {
            if (nout + 2 > mcpl)
                break;
            if (c == _FF_)
                *saw_ff = TRUE;
            nout  += 2;       /* account for '^' meta char */
            *dst++ = '^';
            *dst++ = ctrldigits[c];
        }
        else if (c > _TILDE_ && (! PASS_HIGH(c))) /* assumes ASCII char set */
        {
            if (nout + 4 > mcpl)
                break;
            nout  += 4;       /* account for '\xdd' meta chars */
            *dst++ = '\\';
            *dst++ = 'x';
            *dst++ = hexdigits[(c & 0xf0) >> 4];
            *dst++ = hexdigits[c & 0xf];
        }
        else
        {
            if (nout + 1 > mcpl)
                break;
            nout++;
            *dst++ = (char) c;
        }
        src++;
        nconsumed++;
    }
    *dst = '\0';

    if (srclen > 0)
    {
        /*
         * Still more data in the current input line that needs to be
         * formatted, but there's no more room left on the current output
         * row.  Add a simple post-formatting optimization:  if the
         * remainder of the current input line is all whitespace, return a
         * value that will flag the calling routine that this line has been
         * completely formatted (no use wrapping this line just to output
         * whitespace).
         */

        i = 0;
        while (i < srclen && isSpace((UCHAR)src[i]))
        {
            if (src[i] == _FF_ && (! *saw_ff))
                break;
            i++;
        }
        if (i == srclen)
        {
            /* remainder of input line is all white space */

            nconsumed = orig_srclen;
        }
    }
    return (nconsumed);
}



/* user wants to print empty page(s) -- whatever */
static int
print_blank_pages(PRINT_PARAM *pparam)
{
    int i, rc = TRUE;

    for (i = 0; i < pparam->ncopies && (! printing_aborted); i++)
    {
        if (! winprint_startpage(pparam))
        {
            rc = FALSE;
            break;
        }
        pparam->pagenum++;
        if (! winprint_endpage(pparam))
        {
            rc = FALSE;
            break;
        }
    }
    return (rc);
}



/*
 * Prints a single line in a selected region.  Code structure based upon
 * copy_rgn_data() in w32cbrd.c .
 */
static int
print_rgn_data(void *argp, int l, int r)
{
    LINE          *lp;
    PRINT_PARAM   *pparam;
    char          *src;
    int           saw_ff, isempty_line;
    ULONG         vile_llen, outlen;

    if (printing_aborted)
        return (FALSE);

    lp = DOT.l;

    /* Rationalize offsets */
    if (llength(lp) < l)
        return (TRUE);
    if (r > llength(lp))
        r = llength(lp);
    if (r >= l)
        vile_llen = r - l;
    else
        return (TRUE);  /* prevent a disaster */
    pparam = argp;
    src    = (lp->l_text + l);
    saw_ff = FALSE;

    /*
     * printing a line of text is not as simple as it seems, since care
     * must be taken to wrap buffer data that exceeds max chars per line
     * (mcpl).
     */
    isempty_line = (vile_llen == 0);
    outlen       = 0;
    while (isempty_line || outlen < vile_llen)
    {
        if (pparam->plines % pparam->mlpp == 0)
        {
            pparam->ypos = 0;
            if (! winprint_startpage(pparam))
            {
                break;
            }
        }
        if (isempty_line)
        {
            isempty_line = FALSE;

            if (print_number)
            {
                /* winprint_fmttxt() handles line number formatting */

                (void) winprint_fmttxt(pparam->buf,
                                       " ",
                                       1,
                                       pparam->mcpl,
                                       lp->l_number,
                                       &saw_ff);
            }
            else
            {
                pparam->buf[0] = ' ';
                pparam->buf[1] = '\0';
            }
        }
        else
        {
            outlen += winprint_fmttxt(pparam->buf,
                                      src + outlen,
                                      vile_llen - outlen,
                                      pparam->mcpl,
                                      (outlen == 0) ? lp->l_number : 0,
                                      &saw_ff);
        }
        TextOut(pd->hDC,
                printrect.left,
                printrect.top + pparam->ychar * pparam->ypos++,
                pparam->buf,
                (int) strlen(pparam->buf));
        if (saw_ff || (++pparam->plines % pparam->mlpp == 0))
        {
            if (! winprint_endpage(pparam))
            {
                break;
            }
            if (saw_ff)
            {
                /* resync on arbitrary page boundary */

                pparam->plines = pparam->mlpp;
            }
            pparam->pagenum++;
        }
    }
    return (TRUE);
}



/*
 * Print a text selection--does not support uncollated printing.  Why?
 * Ans:  "dorgn" iterates over a line at a time, making it necessary
 * to save an entire page of output for iterative replay.  And of course,
 * if the last line of the page wraps, you get to worry about that as well.
 * Too much work for too little payback.  If the user wants multiple copies
 * of a selection, s/he gets the copies collated or none at all.
 */
static int
winprint_selection(PRINT_PARAM *pparam, AREGION *selarp)
{
    DORGNLINES     dorgn;
    int            i, rc = TRUE;
    MARK           odot;

    print_tabstop = tabstop_val(curbp);
    print_number  = (nu_width(curwp) > 0);
    oshape        = regionshape;
    odot          = DOT;          /* do_lines_in_region() moves DOT. */
    dorgn         = get_do_lines_rgn();

    for (i = 0; i < pparam->ncopies && rc && (! printing_aborted); i++)
    {
        /*
         * do_lines_in_region(), via a call to getregion(), resets
         * "haveregion" each time it's called.
         */

        haveregion      = &selarp->ar_region;
        regionshape     = selarp->ar_shape;
        pparam->pagenum = 1;
        pparam->plines  = pparam->ypos = 0;
        rc              = dorgn(print_rgn_data, pparam, TRUE);
        if (rc && pparam->endpg_req)
            rc = winprint_endpage(pparam);
    }

    /* clean up the global state that was whacked */
    DOT         = odot;
    haveregion  = NULL;
    regionshape = oshape;
    return (rc);
}



/*
 * prints curbp from bob to eob.
 *
 * returns TRUE if all is well, else FALSE if a print spooling function
 * failed or some other error was detected.  note that this function is
 * called iff curbp is not empty.
 */
static int
winprint_curbuffer_collated(PRINT_PARAM *pparam)
{
    int           eob,    /* end of buffer */
                  rc,
                  saw_ff,
                  isempty_line;
    LINE *        lp;
    ULONG         vile_llen, outlen;

    rc  = TRUE;
    eob = saw_ff = FALSE;
    lp  = lforw(buf_head(curbp));

    /* if printing uncollated, emit "n" copies of the same page */
    while (rc && (! printing_aborted) && (! eob))
    {
        /*
         * printing a line of text is not as simple as it seems, since
         * care must be taken to wrap buffer data that exceeds max chars
         * per line (mcpl).
         */

        vile_llen    = llength(lp);
        isempty_line = (vile_llen == 0);
        outlen       = 0;
        while (isempty_line || outlen < vile_llen)
        {
            if (pparam->plines % pparam->mlpp == 0)
            {
                pparam->ypos = 0;
                if (! winprint_startpage(pparam))
                {
                    rc = FALSE;
                    break;
                }
            }
            if (isempty_line)
            {
                isempty_line = FALSE;

                if (print_number)
                {
                    /* winprint_fmttxt() handles line number formatting */

                    (void) winprint_fmttxt(pparam->buf,
                                           " ",
                                           1,
                                           pparam->mcpl,
                                           lp->l_number,
                                           &saw_ff);
                }
                else
                {
                    pparam->buf[0] = ' ';
                    pparam->buf[1] = '\0';
                }
            }
            else
            {
                outlen += winprint_fmttxt(pparam->buf,
                                          lp->l_text + outlen,
                                          vile_llen - outlen,
                                          pparam->mcpl,
                                          (outlen == 0) ? lp->l_number : 0,
                                          &saw_ff);
            }
            TextOut(pd->hDC,
                    printrect.left,
                    printrect.top + pparam->ychar * pparam->ypos++,
                    pparam->buf,
                    (int) strlen(pparam->buf));
            if (saw_ff || (++pparam->plines % pparam->mlpp == 0))
            {
                if (! winprint_endpage(pparam))
                {
                    rc = FALSE;
                    break;
                }
                if (saw_ff)
                {
                    /* resync on arbitrary page boundary */

                    pparam->plines = pparam->mlpp;
                }
                pparam->pagenum++;
            }
        }
        lp = lforw(lp);
        if (lp == buf_head(curbp))
            eob = TRUE;
    }
    if (rc && pparam->endpg_req)
        rc = winprint_endpage(pparam);
    return (rc);
}



/*
 * FUNCTION
 *   winprint_curbuffer_uncollated(PRINT_PARAM *pparam
 *                                 LINE *      curpg,
 *                                 LINE *      &nxtpg,
 *                                 int         *eob,
 *                                 SPLIT_LINE  *pcursplit,
 *                                 SPLIT_LINE  *pnxtsplit)
 *
 *   pparam    - common print parameters.
 *   curpg     - start printing at this buffer ptr if no data split across
 *               pages.
 *   nxtpg     - by ref, when an end of page is encountered, this buffer ptr
 *               marks the start of the next printable page.
 *   eob       - by ref, while printing this page, end of buffer was detected.
 *   pcursplit - split line info (if any) that must be flushed prior to
 *               printing current page.
 *   pnxtsplit - by ref, when end of page is encountered, if the current buffer
 *               line could not be fit on one printable output line, then this
 *               data structure includes the data that must be split across
 *               printer pages.
 *
 * DESCRIPTION
 *   Prints a single page from curbp, handling the case where data from
 *   the "previous" buffer line has been split across logical printer pages.
 *   What fun.
 *
 * RETURNS
 *   Boolean, T -> all is well, F -> print spooling function or some other
 *   error was detected.
 */

static int
winprint_curbuffer_uncollated(PRINT_PARAM *pparam,
                              LINE *      curpg,
                              LINE *      *nxtpg,
                              int         *eob,
                              SPLIT_LINE  *pcursplit,
                              SPLIT_LINE  *pnxtsplit)
{
    int            eop,       /* T -> end of page */
                   rc,
                   saw_ff,
                   isempty_line;
    LINE *         lp;
    ULONG          vile_llen, outlen;

    rc           = TRUE;
    lp           = curpg;       /* valid most of the time */
    eop          = saw_ff = FALSE;
    pparam->ypos = pparam->plines = 0;

    /* while not (eop or error)... */
    while (rc && (! printing_aborted) && (! eop))
    {
        /*
         * printing a line of text is not as simple as it seems, since
         * care must be taken to wrap buffer data that exceeds the
         * max chars per line (mcpl).
         */

        outlen = 0;               /* valid most of the time */
        if (pparam->plines == 0 && pcursplit->outlen > 0)
        {
            /*
             * at top of page and there is data carried forward from a
             * line previously split across a page.
             */

            outlen = pcursplit->outlen;
            lp     = pcursplit->splitlp;
        }
        vile_llen    = llength(lp);
        isempty_line = (vile_llen == 0);
        while ((isempty_line || outlen < vile_llen) && (! eop))
        {
            if (pparam->plines % pparam->mlpp == 0)
            {
                pparam->ypos = 0;
                if (! winprint_startpage(pparam))
                {
                    rc = FALSE;
                    break;
                }
            }
            if (isempty_line)
            {
                isempty_line = FALSE;

                if (print_number)
                {
                    /* winprint_fmttxt() handles line number formatting */

                    (void) winprint_fmttxt(pparam->buf,
                                           " ",
                                           1,
                                           pparam->mcpl,
                                           lp->l_number,
                                           &saw_ff);
                }
                else
                {
                    pparam->buf[0] = ' ';
                    pparam->buf[1] = '\0';
                }
            }
            else
            {
                outlen += winprint_fmttxt(pparam->buf,
                                          lp->l_text + outlen,
                                          vile_llen - outlen,
                                          pparam->mcpl,
                                          (outlen == 0) ? lp->l_number : 0,
                                          &saw_ff);
            }
            TextOut(pd->hDC,
                    printrect.left,
                    printrect.top + pparam->ychar * pparam->ypos++,
                    pparam->buf,
                    (int) strlen(pparam->buf));
            if (saw_ff || (++pparam->plines % pparam->mlpp == 0))
            {
                eop = TRUE;
                if (! winprint_endpage(pparam))
                {
                    rc = FALSE;
                    break;
                }
            }
        }
        if (outlen < vile_llen)
        {
            /*
             * forced to split a long line across a printer page (i.e., carry
             * forward data to the next page).
             */

            pnxtsplit->outlen  = outlen;
            pnxtsplit->splitlp = lp;
        }
        lp = lforw(lp);
        if (eop)
            *nxtpg = lp;
        if (lp == buf_head(curbp))
        {
            eop = TRUE;   /* by defn */
            if (pnxtsplit->outlen == 0)
                *eob = TRUE;
        }
    }
    if (rc && pparam->endpg_req)
        rc = winprint_endpage(pparam);
    return (rc);
}



/*
 * controls winprint_curbuffer_driver() as required by user's printing
 * options.
 */
static int
winprint_curbuffer(PRINT_PARAM *pparam)
{
    LINE *  curpglp;
    LINE *  nextpglp;
    int     i, eob, rc = TRUE;

    if (is_empty_buf(curbp))
        return (print_blank_pages(pparam));
    print_tabstop = tabstop_val(curbp);
    print_number  = (nu_width(curwp) > 0);
    if (pparam->collate || pparam->ncopies == 1)
    {
        /*
         * request for collated copy(s), or uncollated single copy
         * (same diff).
         */

        for (i = 0; rc && i < pparam->ncopies && (! printing_aborted); i++)
        {
            pparam->pagenum = 1;
            pparam->plines  = pparam->ypos = 0;

            /* print curbuf from bob to eob */
            rc = winprint_curbuffer_collated(pparam);
        }
    }
    else
    {
        SPLIT_LINE cursplit, nxtsplit;

        /* request for uncollated copy(s) */
        pparam->pagenum = 1;
        memset(&cursplit, 0, sizeof(cursplit));
        memset(&nxtsplit, 0, sizeof(nxtsplit));
        eob      = FALSE;
        curpglp  = lforw(buf_head(curbp));
        nextpglp = NULL;
        while ((! eob) && rc && (! printing_aborted))
        {
            /* print a single page of output n times */

            for (i = 0; rc && i < pparam->ncopies && (! printing_aborted); i++)
            {
                rc = winprint_curbuffer_uncollated(pparam,
                                                   curpglp,
                                                   &nextpglp,
                                                   &eob,
                                                   &cursplit,
                                                   &nxtsplit);
            }
            pparam->pagenum++;
            curpglp  = nextpglp;
            cursplit = nxtsplit;
            nextpglp = NULL;
            memset(&nxtsplit, 0, sizeof(nxtsplit));
        }
    }
    return (rc);
}



/*
 * Map winvile's display font to, hopefully, a suitable printer font.
 * Some of the code in this function is borrowed from Petzold.
 */
static HFONT
get_printing_font(HDC hdc, HWND hwnd)
{
    char            *curfont;
    HFONT           hfont;
    LOGFONT         logfont;
    char            font_mapper_face[LF_FACESIZE + 1],
                    msg[LF_FACESIZE * 2 + 128];
    POINT           pt;
    FONTSTR_OPTIONS str_rslts;
    TEXTMETRIC      tm;
    double          ydpi;

    /*
     * FIXME -- allow user to specify distinct printing font via a
     * a state var.
     */
#if DISP_NTWIN
    curfont = ntwinio_current_font();
#else
    curfont = "courier new,8";     /* A console port would substitute a
                                    * user-defined printing font here.
                                    */
#endif
    if (! parse_font_str(curfont, &str_rslts))
    {
        mlforce("BUG: Invalid internal font string");
        return (NULL);
    }
    SaveDC(hdc);
    SetGraphicsMode(hdc, GM_ADVANCED);
    ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    SetViewportOrgEx(hdc, 0, 0, NULL);
    SetWindowOrgEx(hdc, 0, 0, NULL);
    ydpi = GetDeviceCaps(hdc, LOGPIXELSY);
    pt.x = 0;
    pt.y = (long) (str_rslts.size * ydpi / 72);
    DPtoLP(hdc, &pt, 1);

    /* Build up LOGFONT data structure. */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfWeight = (str_rslts.bold) ? FW_BOLD : FW_NORMAL;
    logfont.lfHeight = -pt.y;
    if (str_rslts.italic)
        logfont.lfItalic = TRUE;
    logfont.lfCharSet        = DEFAULT_CHARSET;
    logfont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    vl_strncpy(logfont.lfFaceName, str_rslts.face, LF_FACESIZE);
    logfont.lfFaceName[LF_FACESIZE - 1] = '\0';
    if ((hfont = CreateFontIndirect(&logfont)) == NULL)
    {
        disp_win32_error(W32_SYS_ERROR, hwnd);
        RestoreDC(hdc, -1);
        return (NULL);
    }

    /* font must be fixed pitch -- if not we bail */
    (void) SelectObject(hdc, hfont);
    GetTextFace(hdc, sizeof(font_mapper_face), font_mapper_face);
    GetTextMetrics(hdc, &tm);
    if ((tm.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0)
    {
        /* Misnamed constant! */

        RestoreDC(hdc, -1);
        DeleteObject(hfont);
        sprintf(msg,
            "Fontmapper selected proportional printing font (%s), aborting...",
                font_mapper_face);
        MessageBox(hwnd, msg, prognam, MB_ICONSTOP | MB_OK);
        return (NULL);
    }

    if (stricmp(font_mapper_face, str_rslts.face) != 0)
    {
        /*
         * Notify user that fontmapper substituted a font for the printer
         * context that does not match the display context.  This is an
         * informational message only -- printing can proceed.
         */

        sprintf(msg,
                "info: Fontmapper substituted \"%s\" for \"%s\"",
                font_mapper_face,
                str_rslts.face);
        mlforce(msg);
    }
    RestoreDC(hdc, -1);
    return (hfont);
}


static HINSTANCE
GetHinstance(HWND hwnd)
{
#if (_MSC_VER >= 1300)
    return (HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
#else
    return (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);
#endif
}

/*
 * this feature doesn't support printing by "pages" because vile doesn't
 * rigorously support this concept (what's the page height? cur window?).
 */
int
winprint(int f, int n)
{
    char            buf[NFILEN + 64];
    DOCINFO         di;
    HDC             hdc;
    HFONT           hfont, old_hfont;
    HWND            hwnd;
    int             horzres,
                    mcpl,    /* max chars per line                 */
                    mlpp,    /* max lines per page                 */
                    rc = TRUE,
                    status,
                    vertres,
                    xchar,   /* avg char width (pixels)            */
                    xdpi,    /* dots per inch, x axis              */
                    xwidth,  /* physical page width (pixels)       */
                    ychar,   /* max character height (pixels)      */
                    ydpi,    /* dots per inch, y axis              */
                    ylen;    /* physical page height (pixels)      */
    RECT            minmar;  /* printer's minimum margins (pixels) */
    DEVMODE         *pdm_print, *pdm_setup;
    PRINT_PARAM     pparam;
    AREGION         selar;
    BUFFER          *selbp;
    TEXTMETRIC      tm;

    memset(&pparam, 0, sizeof(pparam));
#if DISP_NTWIN
    hwnd = hPrintWnd = winvile_hwnd();
#else
    hwnd = hPrintWnd = GetForegroundWindow();
#endif
    if (! pd)
    {
        if ((pd = typecalloc(PRINTDLG)) == NULL)
            return (no_memory("winprint"));
        pd->lStructSize = sizeof(*pd);
        pd->nCopies     = 1;
        pd->Flags       = PD_COLLATE|PD_ALLPAGES|PD_RETURNDC|PD_NOPAGENUMS;

    }
    pd->hwndOwner = hwnd;

    /*
     * configure print dialog box to print the current selection, by
     * default, as long as that selection is in the current buffer
     */
    if ((selbp = get_selection_buffer_and_region(&selar)) == NULL)
        pd->Flags |= PD_NOSELECTION;   /* disable "selection" radio button */
    else
    {
        pd->Flags &= ~PD_NOSELECTION;  /* enable "selection" radio button  */
        if (curbp == selbp && (! empty_text_selection(selbp, &selar)))
        {
            /*
             * uncollated printing of text selections doesn't work in all
             * cases, so enable collation by default.
             */

            pd->Flags |= (PD_SELECTION | PD_COLLATE);
        }
        else
            pd->Flags &= ~PD_SELECTION;
    }
    if (! handle_page_dflts(hwnd))
        return (FALSE);

    if (pgsetup_chgd)
    {
        ULONG nbytes;

        /*
         * Force printer to use parameters specified in page setup dialog.
         * This is a rather subtle implementation issue (i.e., undocumented).
         *
         * Famous last words:
         *
         *    "Wouldn't it be enlightening to spend an afternoon or so and add
         *    win32 printing to an app."
         *
         * Sure, you betcha'.
         */

        if (pd->hDevMode)
        {
            GlobalFree(pd->hDevMode);
            pd->hDevMode = NULL;
        }
        if (pd->hDevNames)
        {
            /* this is stale -- force PrintDlg to reload it */

            GlobalFree(pd->hDevNames);
            pd->hDevNames = NULL;
        }
        pdm_setup = GlobalLock(pgsetup->hDevMode);
        nbytes    = pdm_setup->dmSize + pdm_setup->dmDriverExtra;
        if ((pd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, nbytes)) == NULL)
        {
            GlobalUnlock(pgsetup->hDevMode);
            return (no_memory("winprint"));
        }
        pdm_print = GlobalLock(pd->hDevMode);
        memcpy(pdm_print, pdm_setup, nbytes);
        GlobalUnlock(pgsetup->hDevMode);
        GlobalUnlock(pd->hDevMode);
        pgsetup_chgd = FALSE;
    }

    /* Up goes the canonical win32 print dialog */
    status = PrintDlg(pd);

#if DISP_NTCONS
    /* attempt to restore focus to the console editor */
    (void) SetForegroundWindow(ofn.hwndOwner);
#endif

    if (! status)
    {
        /* user cancel'd dialog box or some error detected. */

        if (comm_dlg_error() != 0)
        {
            /* legit error */

            rc = FALSE;
        }
        return (rc);
    }

    printdlg_chgd    = TRUE;
    printing_aborted = spooler_failure = FALSE;
    print_high       = global_g_val(GVAL_PRINT_HIGH);
    print_low        = global_g_val(GVAL_PRINT_LOW);
    pparam.ncopies   = pd->nCopies;
    pparam.collate   = (pd->Flags & PD_COLLATE);
    if ((! pparam.collate) && (pd->nCopies > 1) && (pd->Flags & PD_SELECTION))
    {
        /*
         * The issue here is that uncollated printing of multiple copies of
         * a text selection is _not_ supported.  This could be done, but
         * would require adding quite a bit of code for very little return
         * on investment.  The underlying problem is that
         * do_lines_in_region() is line-oriented, which means that in a
         * multicopy scenario, uncollated printing logic in this module
         * must accumulate a printer page's worth of data and replay it "n"
         * times--not worth the trouble or code bloat (at this time) to
         * support what is most likely a seldom used feature (uncollated
         * printing).
         */

        status = MessageBox(hwnd,
                            "Uncollated text selection printing is not "
                            "supported when using this printer.\r\r"
                            "Continue printing with collation?",
                            prognam,
                            MB_ICONQUESTION | MB_YESNO);
        if (status != IDYES)
        {
            winprint_cleanup(NULL);
            return (FALSE);
        }
        pparam.collate = TRUE;
    }

    /* compute some printing parameters */
    hdc = pd->hDC;
    SetMapMode(hdc, MM_TEXT);
    if ((hfont = get_printing_font(hdc, hwnd)) == NULL)
    {
        winprint_cleanup(NULL);
        return (FALSE);
    }
    old_hfont = SelectObject(hdc, hfont);

    /*
     * didja' know -- there exist printers drivers that render inside
     * asymmetrical margins?  For example, an HPDJ 870CSE.
     */
    xdpi          = GetDeviceCaps(hdc, LOGPIXELSX);
    ydpi          = GetDeviceCaps(hdc, LOGPIXELSY);
    horzres       = GetDeviceCaps(hdc, HORZRES);
    vertres       = GetDeviceCaps(hdc, VERTRES);
    xwidth        = GetDeviceCaps(hdc, PHYSICALWIDTH);
    ylen          = GetDeviceCaps(hdc, PHYSICALHEIGHT);
    minmar.left   = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    minmar.top    = GetDeviceCaps(hdc, PHYSICALOFFSETY);
    minmar.right  = xwidth - horzres - minmar.left;
    minmar.bottom = ylen - vertres - minmar.top;
    if (minmar.bottom < 0 || minmar.right < 0)
    {
        MessageBox(hwnd,
                   "Printer driver's physical dimensions not rationale",
                   prognam,
                   MB_ICONSTOP | MB_OK);
        winprint_cleanup(old_hfont);
        return (FALSE);
    }
    (void) GetTextMetrics(hdc, &tm);
    ychar = tm.tmHeight + tm.tmExternalLeading;
    xchar = tm.tmAveCharWidth;
    compute_printrect(xdpi, ydpi, &minmar, horzres, vertres);
    compute_foot_hdr_pos(ydpi, &minmar, ychar, vertres);
    mcpl = (printrect.right - printrect.left) / xchar;
    if (mcpl <= 0)
    {
        MessageBox(hwnd,
                   "Left/Right margins too wide",
                   prognam,
                   MB_ICONSTOP | MB_OK);
        winprint_cleanup(old_hfont);
        return (FALSE);
    }
    mlpp = (printrect.bottom - printrect.top) / ychar;
    if (mlpp <= 0)
    {
        MessageBox(hwnd,
                   "Top/Bottom margins too wide",
                   prognam,
                   MB_ICONSTOP | MB_OK);
        winprint_cleanup(old_hfont);
        return (FALSE);
    }
    TRACE(("mlpp: %d, mcpl: %d\n", mlpp, mcpl));

    /* disable access to winvile message queue during printing */
    (void) EnableWindow(hwnd, FALSE);

    /* up goes the _modeless_ 'abort printing' dialog box */
    hDlgCancelPrint = CreateDialog(GetHinstance(hwnd),
                                   "PrintCancelDlg",
                                   hwnd,
                                   printer_dlg_proc);
    if (! hDlgCancelPrint)
    {
        disp_win32_error(W32_SYS_ERROR, hwnd);
        winprint_cleanup(old_hfont);
        return (FALSE);
    }
    (void) SetAbortProc(hdc, printer_abort_proc);

    /* create a job name for spooler */
    memset(&di, 0, sizeof(di));
    di.cbSize = sizeof(di);
    if (pd->Flags & PD_SELECTION)
    {
        sprintf(buf, "%s (selection)", curbp->b_bname);
        di.lpszDocName = buf;
    }
    else
        di.lpszDocName = curbp->b_bname;
    if (StartDoc(hdc, &di) <= 0)
    {
        display_spooler_error(hwnd);
        winprint_cleanup(old_hfont);
        return (FALSE);
    }

    /*
     * Init printing parameters structure and start spooling data.  In the
     * code below, "+32" conservatively accounts for line numbering
     * prefix space.
     */
    if ((pparam.buf = typeallocn(char, mcpl + 32)) != NULL)
    {
        pparam.xchar = xchar;
        pparam.ychar = ychar;
        pparam.mcpl  = mcpl;
        pparam.mlpp  = mlpp;
        pparam.hfont = hfont;
        pparam.xorg  = minmar.left;
        pparam.yorg  = minmar.top;
        if (pd->Flags & PD_SELECTION)
        {
            WINDOW *ocurwp;

            if ((ocurwp = push_curbp(selbp)) != NULL)
            {
                rc = winprint_selection(&pparam, &selar);
                pop_curbp(ocurwp);
            }
            else
            {
                rc = FALSE;
            }
        }
        else
        {
            rc = winprint_curbuffer(&pparam);
        }
        (void) free(pparam.buf);
    }
    else
    {
        rc = no_memory("winprint");
    }
    if (! spooler_failure)
    {
        /*
         * don't close document if spooler failed -- causes a fault
         * on Win9x hosts.
         */

        (void) EndDoc(hdc);
    }
    if (! printing_aborted)
    {
        /*
         * order is important here:  enable winvile's message queue before
         * destroying the cancel printing dlg, else winvile loses focus
         */

        (void) EnableWindow(hwnd, TRUE);
        (void) DestroyWindow(hDlgCancelPrint);
    }
    (void) DeleteObject(SelectObject(hdc, old_hfont));
    (void) DeleteDC(hdc);
    return (rc);
}



int
winpg_setup(int f, int n)
{
    HWND hwnd;
    int  rc = TRUE, status;

#if DISP_NTWIN
    hwnd = winvile_hwnd();
#else
    hwnd = GetForegroundWindow();
#endif
    if (! handle_page_dflts(hwnd))
        return (FALSE);
    if (printdlg_chgd)
    {
        ULONG          nbytes;
        DEVMODE       *pdm_print, *pdm_setup;

        /*
         * Force PageSetupDlg() to use parameters specified by last
         * invocation of PrinterDlg().
         */
        if (pgsetup->hDevMode)
        {
            GlobalFree(pgsetup->hDevMode);
            pgsetup->hDevMode = NULL;
        }
        if (pgsetup->hDevNames)
        {
            /* stale by defn -- force PrintSetupDlg to reload it */

            GlobalFree(pgsetup->hDevNames);
            pgsetup->hDevNames = NULL;
        }
        pdm_print = GlobalLock(pd->hDevMode);
        nbytes    = pdm_print->dmSize + pdm_print->dmDriverExtra;
        if ((pgsetup->hDevMode = GlobalAlloc(GMEM_MOVEABLE, nbytes)) == NULL)
        {
            GlobalUnlock(pd->hDevMode);
            return (no_memory("winpg_setup"));
        }
        pdm_setup = GlobalLock(pgsetup->hDevMode);
        memcpy(pdm_setup, pdm_print, nbytes);
        GlobalUnlock(pgsetup->hDevMode);
        GlobalUnlock(pd->hDevMode);
        printdlg_chgd = FALSE;
    }
    status = PageSetupDlg(pgsetup);

#if DISP_NTCONS
    /* attempt to restore focus to the console editor */
    (void) SetForegroundWindow(hwnd);
#endif

    if (! status)
    {
        /* user cancel'd dialog box or some error detected. */

        if (comm_dlg_error() != 0)
        {
            /* legit error -- user did not cancel dialog box */

            rc = FALSE;
        }
    }
    pgsetup_chgd = TRUE;
    return (rc);
}
#endif /* DISP_NTWIN */

/* --------------------------------------------------------------------- */
/* -------- routines that remember/purge recent folders/files -----------*/
/* ----------------------------------------------------------------------*/

/*
 * Note 1:  These routines use the following registry key/value formats:
 *
 * Recent Files
 * ------------
 * Key:        REGKEY_RECENT_FILES
 * Value Name: Hex(0 to (MAX_RECENT_FILES - 1))
 * Value Data: nul-terminated file path
 *
 * Recent Folders
 * --------------
 * Key:        REGKEY_RECENT_FLDRS
 * Value Name: Hex(0 to (MAX_RECENT_FLDRS - 1))
 * Value Data: nul-terminated directory path
 *
 * Note 2:  For the most part, these routines are winvile-specific.
 */

/* --------------------------------------------------------------------- */

/*
 * Note that delete_recent_files_folder_registry_data() is coded for
 * maximum portability among the various flavors of Windows.  It's _not_
 * coded for maximum speed.
 */
static int
delete_recent_files_folder_registry_data(int is_file)
{
    HKEY hkey;
    int  i, maxpaths;
    char *subkey, value_name[32];

    subkey = (is_file) ? REGKEY_RECENT_FILES : REGKEY_RECENT_FLDRS;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     subkey,
                     0,
                     KEY_WRITE,
                     &hkey) != ERROR_SUCCESS)
    {
        /* assume there's no there, there */

        return (TRUE);
    }
    maxpaths = (is_file) ? MAX_RECENT_FILES : MAX_RECENT_FLDRS;
    for (i = 0; i < maxpaths; i++)
    {
        sprintf(value_name, RECENT_REGVALUE_FMT, i);
        (void) RegDeleteValue(hkey, value_name);
    }
    (void) RegCloseKey(hkey);
    return (TRUE);
}

/* callable from both console vile and winvile */
int
purge_recent_files(int f, int n)
{
    return (delete_recent_files_folder_registry_data(TRUE));
}

/* callable from both console vile and winvile */
int
purge_recent_folders(int f, int n)
{
    return (delete_recent_files_folder_registry_data(FALSE));
}

/* ---------------- begin winvile-specific functionality ----------------- */

#if DISP_NTWIN

static void
free_mru_list(char **list)
{
    char *p, **orig_listp;

    if (! list)
        return;
    orig_listp = list;
    while (*list)
    {
        p = *list++;
        (void) free(p);
    }
    (void) free(orig_listp);
}

/*
 * Read a Files/Folders MRU list from the registry, returns NULL if
 * error or no data in registry.  If non-NULL list returned, caller
 * must free same.
 */
static char **
fetch_mru_list(int is_file, int maxpaths)
{
    DWORD dwSzPath;
    HKEY  hkey;
    int   i;
    char  **list, value_name[32], *subkey, path[FILENAME_MAX + 1];

    subkey = (is_file) ? REGKEY_RECENT_FILES : REGKEY_RECENT_FLDRS;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     subkey,
                     0,
                     KEY_READ,
                     &hkey) != ERROR_SUCCESS)
    {
        /* assume there's no there, there */

        return (NULL);
    }
    if ((list = typecallocn(char *, maxpaths + 1)) == NULL)
    {
        (void) RegCloseKey(hkey);
        (void) no_memory("fetch_mru_list()");
        return (NULL);
    }
    for (i = 0; i < maxpaths; i++)
    {
        sprintf(value_name, RECENT_REGVALUE_FMT, i);
        dwSzPath = sizeof(path);
        if (RegQueryValueEx(hkey,
                            value_name,
                            NULL,
                            NULL,
                            (LPBYTE) path,
                            &dwSzPath) != ERROR_SUCCESS)
        {
            /* user can muck with registry...don't assume no more data */

            continue;
        }

        if ((list[i] = typeallocn(char, (dwSzPath) ? dwSzPath : 1)) == NULL)
        {
            (void) RegCloseKey(hkey);
            free_mru_list(list);
            (void) no_memory("fetch_mru_list()");
            return (NULL);
        }

        /* user can muck with registry...be paranoid */
        strncpy(list[i], path, dwSzPath - 1);
        (list[i])[dwSzPath - 1] = '\0';
    }
    (void) RegCloseKey(hkey);

    /* handle degenerate case */
    if (list[0] == NULL)
    {
        free_mru_list(list);
        list = NULL;
    }
    return (list);
}

static int
delete_popup_menu(HMENU vile_menu, int mnu_posn)
{
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    if (! GetMenuItemInfo(vile_menu, mnu_posn, TRUE, &mii))
        return (FALSE);

    /* only delete popup menu if it exists */
    if (mii.hSubMenu)
    {
        if (! DestroyMenu(mii.hSubMenu))
            return (FALSE);
        mii.hSubMenu = NULL;
        if (! SetMenuItemInfo(vile_menu, mnu_posn, TRUE, &mii))
            return (FALSE);
    }
    return (TRUE);
}

static int
create_popup_menu(HMENU vile_menu,
                  int   mnu_posn,
                  char  **list,
                  int   base_mnu_item_id,
                  int   maxitems)
{
    HMENU        hPopupMenu = CreateMenu();
    int          i;
    MENUITEMINFO mii;

    if (! hPopupMenu)
        return (FALSE);
    for (i = 0; i < maxitems && *list; i++, list++)
        AppendMenu(hPopupMenu, MF_STRING, base_mnu_item_id++, *list);
    memset(&mii, 0, sizeof(mii));
    mii.cbSize   = sizeof(mii);
    mii.fMask    = MIIM_SUBMENU;
    mii.hSubMenu = hPopupMenu;
    if (! SetMenuItemInfo(vile_menu, mnu_posn, TRUE, &mii))
        return (FALSE);
    return (TRUE);
}

/* Find positions of two system menu items of interest...do this only once. */
static int
find_files_folder_menu_posns(int *files_mnu_posn, int *fldrs_mnu_posn)
{
    static int cached_files_posn = -1, cached_fldrs_posn = -1;

    int      i, nitems;
    HMENU    vile_menu;

    if (cached_files_posn >= 0)
    {
        *files_mnu_posn = cached_files_posn;
        *fldrs_mnu_posn = cached_fldrs_posn;
        return (TRUE);
    }
    vile_menu = GetSystemMenu(winvile_hwnd(), FALSE);
    nitems    = GetMenuItemCount(vile_menu);
    if (nitems < 0)
    {
        mlforce("[system menu inaccessible]");
        return (FALSE);
    }
    for (i = nitems - 1 ; i >= 0; i--)
    {
        if (GetMenuItemID(vile_menu, i) == IDM_SEP_AFTER_RCNT_FLDRS)
        {
            cached_fldrs_posn = i - 1;
            cached_files_posn = i - 2;
            break;
        }
    }
    if (cached_files_posn < 0)
    {
        /* search failed, something is quite wrong.... */

        mlforce("[system menu RECENT FILE/FLDR items missing]");
        return (FALSE);
    }
    else
    {
        *files_mnu_posn = cached_files_posn;
        *fldrs_mnu_posn = cached_fldrs_posn;
        return (TRUE);
    }
}

void
build_recent_file_and_folder_menus(void)
{

    char     **list;   /* dynamic array of files or folders */
    int      maxfiles, maxfldrs, files_mnu_posn, fldrs_mnu_posn;
    unsigned mnu_state;
    HMENU    vile_menu;

    if (! find_files_folder_menu_posns(&files_mnu_posn, &fldrs_mnu_posn))
        return;  /* system menu scrogged */

    vile_menu = GetSystemMenu(winvile_hwnd(), FALSE);

    /* --------- create or disable "recent files" popup menu --------- */

    maxfiles  = global_g_val(GVAL_RECENT_FILES);
    mnu_state = MF_GRAYED;
    if (maxfiles != 0)
    {
        if ((list = fetch_mru_list(TRUE, maxfiles)) != NULL)
        {
            /* first things first -- whack the old popup menu */

            if (delete_popup_menu(vile_menu, files_mnu_posn))
            {
                if (create_popup_menu(vile_menu,
                                      files_mnu_posn,
                                      list,
                                      IDM_RECENT_FILES,
                                      maxfiles))
                {
                    mnu_state = MF_ENABLED;
                }
            }
            free_mru_list(list);
        }
    }
    EnableMenuItem(vile_menu, files_mnu_posn, MF_BYPOSITION | mnu_state);

    /* --------- create or disable "recent folders" popup menu --------- */

    maxfldrs  = global_g_val(GVAL_RECENT_FLDRS);
    mnu_state = MF_GRAYED;
    if (maxfldrs != 0)
    {
        if ((list = fetch_mru_list(FALSE, maxfldrs)) != NULL)
        {
            /* first things first -- whack the old popup menu */

            if (delete_popup_menu(vile_menu, fldrs_mnu_posn))
            {
                if (create_popup_menu(vile_menu,
                                      fldrs_mnu_posn,
                                      list,
                                      IDM_RECENT_FLDRS,
                                      maxfldrs))
                {
                    mnu_state = MF_ENABLED;
                }
            }
            free_mru_list(list);
        }
    }
    EnableMenuItem(vile_menu, fldrs_mnu_posn, MF_BYPOSITION | mnu_state);
}

/* chdir() to  a path selected from the winvile "recent folders" menu */
int
cd_recent_folder(int mnu_index)
{
    char     dir[FILENAME_MAX + 1];
    int      files_mnu_posn, fldrs_mnu_posn;
    HMENU    vile_menu, fldrs_menu;

    if (! find_files_folder_menu_posns(&files_mnu_posn, &fldrs_mnu_posn))
        return (FALSE);  /* system menu scrogged */

    vile_menu = GetSystemMenu(winvile_hwnd(), FALSE);
    if ((fldrs_menu = GetSubMenu(vile_menu, fldrs_mnu_posn)) == NULL)
    {
        mlforce("BUG: folders popup menu damaged");
        return (FALSE);
    }
    if (! GetMenuString(fldrs_menu,
                        mnu_index,
                        dir,
                        sizeof(dir),
                        MF_BYCOMMAND))
    {
        mlforce("BUG: folders popup menu(%d) bogus", mnu_index);
        return (FALSE);
    }
    return (set_directory(dir));

    /*
     * note that set_directory() eventually calls
     * store_recent_file_or_folder() to update the Recent Folders MRU.
     */
}

/* open a file selected from the winvile "recent files" menu */
int
edit_recent_file(int mnu_index)
{
    BUFFER   *bp;
    char     file[FILENAME_MAX + 1];
    int      files_mnu_posn, fldrs_mnu_posn, rc;
    HMENU    vile_menu, files_menu;

    if (! find_files_folder_menu_posns(&files_mnu_posn, &fldrs_mnu_posn))
        return (FALSE);  /* system menu scrogged */

    vile_menu = GetSystemMenu(winvile_hwnd(), FALSE);
    if ((files_menu = GetSubMenu(vile_menu, files_mnu_posn)) == NULL)
    {
        mlforce("BUG: files popup menu damaged");
        return (FALSE);
    }
    if (! GetMenuString(files_menu,
                        mnu_index,
                        file,
                        sizeof(file),
                        MF_BYCOMMAND))
    {
        mlforce("BUG: files popup menu(%d) bogus", mnu_index);
        return (FALSE);
    }

    rc = TRUE;  /* an assumption */

    /*
     * Perhaps the editor has the target file in a buffer already?
     *
     * Calling getfile() first is actually important, since if the requested
     * file was previously sucked into a buffer and swbuffer() is called first,
     * winvile occasionally drops incorrect highlights on existing buffers.
     * Yes, it's all magic to me.
     */
    if (! getfile(file, TRUE))
    {
        /* guess not */

        if ((bp = getfile2bp(file, FALSE, FALSE)) != NULL)
        {
            bp->b_flag |= BFARGS;         /* treat this as an argument */
            rc = swbuffer(bp);            /* editor switches buffer    */

            /*
             * note that swbuffer() eventually calls
             * store_recent_file_or_folder() to update the Recent Files MRU.
             */
        }
    }
    else
    {
        /*
         * Subtle points here:
         *
         * 1) The user just used winvile's Recent Files feature to switch
         *    the editor's current buffer to an existing file.
         * 2) This file, which is in the Recent Files MRU list, might not
         *    be at the front of the MRU.
         * 3) The user will expect this file to go to the front of the MRU.
         *    But it won't because the file's BUFFER pointer has BFREGD set.
         *
         * Fix that.
         */

         store_recent_file_or_folder(file, TRUE);
    }
    return (rc);
}

/*
 * Save a file or folder in a registry MRU list. Saved object goes to the
 * front of the list.
 */
void
store_recent_file_or_folder(const char *path, int is_file)
{
    char *newlist[MAX_RECENT_FILES+MAX_RECENT_FLDRS + 1]; /* overdim'd */
    char **oldlist, *subkey, **listp, value_name[32];
    HKEY hkey;
    int  i, j, maxpaths;

    maxpaths = global_g_val((is_file) ? GVAL_RECENT_FILES : GVAL_RECENT_FLDRS);
    if (maxpaths == 0)
        return;   /* feature disabled */
    subkey = (is_file) ? REGKEY_RECENT_FILES : REGKEY_RECENT_FLDRS;

    /* read all existing MRU data into dynamic array of strings */
    if ((oldlist = fetch_mru_list(is_file, maxpaths)) == NULL)
    {
        /* assume no MRU data -- degenerate case */

        if (RegCreateKeyEx(HKEY_CURRENT_USER,
                           subkey,
                           0,
                           "",
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hkey,
                           NULL) != ERROR_SUCCESS)
        {
            disp_win32_error(W32_SYS_ERROR, winvile_hwnd());
            return;
        }
        sprintf(value_name, RECENT_REGVALUE_FMT, 0);
        if (RegSetValueEx(hkey,
                          value_name,
                          0,
                          REG_SZ,
                          (const BYTE *) path,
                          (DWORD) strlen(path) + 1) != ERROR_SUCCESS)
        {
            disp_win32_error(W32_SYS_ERROR, winvile_hwnd());
        }
        (void) RegCloseKey(hkey);
        return;
    }
    if (oldlist[0] && stricmp(path, oldlist[0]) == 0)
    {
        /* degenerate case -- path already at head of MRU list */

        free_mru_list(oldlist);
        return;
    }

    /* ----------- do some actual work -> construct new list -------- */

    newlist[0] = (char *) path;
    for (i = 0, j = 1, listp = oldlist; *listp && i < maxpaths; i++, listp++)
    {
        if (stricmp(*listp, path) != 0)
        {
            /* don't dup "path" from oldMRU */

            newlist[j++] = *listp;
        }
    }
    newlist[j] = NULL;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     subkey,
                     0,
                     KEY_WRITE,
                     &hkey) != ERROR_SUCCESS)
    {
        /* what? */

        disp_win32_error(W32_SYS_ERROR, winvile_hwnd());
    }
    else
    {
        for (i = 0, listp = newlist; *listp && i < maxpaths; i++, listp++)
        {
            sprintf(value_name, RECENT_REGVALUE_FMT, i);
            if (RegSetValueEx(hkey,
                              value_name,
                              0,
                              REG_SZ,
                              (const BYTE *) *listp,
                              (DWORD) strlen(*listp) + 1) != ERROR_SUCCESS)
            {
                disp_win32_error(W32_SYS_ERROR, winvile_hwnd());
                break;
            }
        }
        (void) RegCloseKey(hkey);
    }
    free_mru_list(oldlist);
}

#endif /* DISP_NTWIN */
-                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                