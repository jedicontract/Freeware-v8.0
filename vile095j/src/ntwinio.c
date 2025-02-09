/*
 * Uses the Win32 screen API.
 *
 * $Header: /usr/build/vile/vile/RCS/ntwinio.c,v 1.159 2006/04/25 23:13:32 tom Exp $
 * Written by T.E.Dickey for vile (october 1997).
 * -- improvements by Clark Morgan (see w32cbrd.c, w32pipe.c).
 */

#include "estruct.h"		/* includes <windows.h> */

#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdlib.h>
#include <math.h>

#include "edef.h"
#include "pscreen.h"
#include "patchlev.h"
#include "winvile.h"
#include "nefsms.h"
#include "nefunc.h"
#include "chgdfunc.h"

#define ABS(x) (((x) < 0) ? -(x) : (x))

#define MIN_ROWS MINWLNS
#define MIN_COLS 15

#define FIXME_POSCHANGING 1	/* this doesn't seem to help */

#define MAX_CURSOR_STYLE 2

#if OPT_TRACE
#define IGN_PROC(tag,name) \
	case name: \
		TRACE((tag #name " (ignored)\n")); \
		break;

#define DEF_PROC(tag,name) \
	case name: \
		TRACE((tag #name " (%s)\n", which_window(hWnd))); \
		return (DefWindowProc(hWnd, message, wParam, lParam))
#else
#define IGN_PROC(tag,name) case name: break;
#define DEF_PROC(tag,name)	/*nothing */
#endif

#define MAIN_CLASS "VileMain"
#define TEXT_CLASS "VileText"
#define SCRL_CLASS "SCROLLBAR"
#define GRIP_CLASS "VileResize"

#define MY_APPLE "Vile Application"

#define MY_FONT  SYSTEM_FIXED_FONT	/* or ANSI_FIXED_FONT           */

#define NROW	128		/* Max Screen size.             */
#define NCOL    256		/* Edit if you want to.         */
#define	NPAUSE	200		/* # times thru update to pause */
#define NOKYMAP (-1)
#define KYREDIR (-2)		/* sent keystroke elsewhere.    */

#define RSZ_WDW_HGHT  18	/* pixels */
#define RSZ_WDW_WDTH 190

#define SetCols(value) term.cols = cur_win->cols = value
#define SetRows(value) term.rows = cur_win->rows = value

#define RowToPixel(n) ((n) * nLineHeight)
#define ColToPixel(n) ((n) * nCharWidth)

#define PixelToRow(n) ((n) / nLineHeight)
#define PixelToCol(n) ((n) / nCharWidth)

#define RectToCols(rect) (PixelToCol(rect.right - rect.left - SbWidth))
#define RectToRows(rect) (PixelToRow(rect.bottom - rect.top))

#if OPT_SCROLLBARS
#define SbWidth   nLineHeight
#else
#define SbWidth   0
#endif

static DWORD default_bcolor;
static DWORD default_fcolor;
static HANDLE hAccTable;	/* handle to accelerator table */
static HANDLE vile_hinstance;
static HCURSOR arrow_cursor;
static HCURSOR hglass_cursor;
static HCURSOR selection_cursor;
static HCURSOR wdwsize_cursor;
static HMENU vile_menu, popup_menu;
static LOGFONT vile_logfont;
static SETTIMER_RETVAL nIDTimer = 0;
static int ac_active = FALSE;	/* AutoColor active? */
static int caret_disabled = TRUE;
static int caret_exists = 0;
static int caret_visible = 0;
static int desired_wdw_state;
static int dont_update_sb = FALSE;
static int enable_popup = TRUE;
static int font_resize_in_progress;
static int gui_resize_in_progress;
static int initialized = FALSE;	/* winvile open for business */
static int mouse_captured = 0;
static int nCharWidth = 8;
static int nLineHeight = 10;
static int vile_in_getfkey = 0;
static int vile_resizing = FALSE;	/* rely on repaint_window if true */
static int icursor;		/* T -> enable insertion cursor   */
static int icursor_style;	/* 1 -> cmdmode = block,
				 *      insmode = vertical bar
				 * 2 -> cmdmode = vertical bar
				 *      insmode = block
				 */

#ifdef VILE_OLE
static OLEAUTO_OPTIONS oa_opts;
static int redirect_keys;
#endif

static int nfcolor = -1;	/* normal foreground color */
static int nbcolor = -1;	/* normal background color */
static int crow = -1;		/* current row */
static int ccol = -1;		/* current col */

/* ansi to ibm color translation table */
static const char *initpalettestr = "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15";
/* black, red, green, yellow, blue, magenta, cyan, white   */

static int cur_pos = 0;
static VIDEO_ATTR cur_atr = 0;

typedef struct {
    HWND w;
    RECT r;
    int shown;
} SBDATA;

static struct gui_info {
    HWND main_hwnd;		/* the top-level window */
    HWND text_hwnd;		/* the enclosed text area */
    int closed;
    int cols;
    int rows;
    RECT xy_limit;
    int x_border;
    int y_border;
    int y_titles;		/* height of caption/titles */
    int nscrollbars;
    int maxscrollbars;
    SBDATA *scrollbars;
    SBDATA size_box;
    SBDATA size_grip;
} only_window, *cur_win = &only_window;
/* *INDENT-OFF* */
static struct my_font {
    int used;
    HFONT font;
    VIDEO_ATTR attr;
} MyFonts[] = {
    { FALSE, 0, 0 },
    { FALSE, 0, VABOLD },
    { FALSE, 0, VAUL },
    { FALSE, 0, VAITAL },
};
/* *INDENT-ON* */

#if OPT_SCROLLBARS
static int check_scrollbar_allocs(void);
static void update_scrollbar_sizes(void);
#endif

#if OPT_TRACE
static char *
which_window(HWND hwnd)
{
    if (hwnd == 0) {
	return "NULL";
    }
    if (hwnd == cur_win->main_hwnd) {
	return "main";
    } else if (hwnd == cur_win->text_hwnd) {
	return "text";
    } else {
	int n;
	static char temp[20];
	sprintf(temp, "%#lx", hwnd);
	for (n = 0; n < cur_win->nscrollbars; n++) {
	    if (hwnd == cur_win->scrollbars[n].w) {
		sprintf(temp, "sb%d", n);
		break;
	    }
	}
	return temp;
    }
}

static char *
message2s(unsigned code)
{
    static struct {
	WORD code;
	char *name;
    } table[] = {
	/* *INDENT-OFF* */
	{ WM_ACTIVATE,		"WM_ACTIVATE" },
	{ WM_ACTIVATEAPP,	"WM_ACTIVATEAPP" },
	{ WM_CANCELMODE,	"WM_CANCELMODE" },
	{ WM_CAPTURECHANGED,	"WM_CAPTURECHANGED" },
	{ WM_CLOSE,		"WM_CLOSE" },
	{ WM_CONTEXTMENU,	"WM_CONTEXTMENU" },
	{ WM_CREATE,		"WM_CREATE" },
	{ WM_CTLCOLORSCROLLBAR,	"WM_CTLCOLORSCROLLBAR" },
	{ WM_DROPFILES,		"WM_DROPFILES" },
	{ WM_ENABLE,		"WM_ENABLE" },
	{ WM_ENTERIDLE,		"WM_ENTERIDLE" },
	{ WM_ENTERMENULOOP,	"WM_ENTERMENULOOP" },
	{ WM_ENTERSIZEMOVE,	"WM_ENTERSIZEMOVE" },
	{ WM_ERASEBKGND,	"WM_ERASEBKGND" },
	{ WM_EXITMENULOOP,	"WM_EXITMENULOOP" },
	{ WM_EXITSIZEMOVE,	"WM_EXITSIZEMOVE" },
	{ WM_GETMINMAXINFO,	"WM_GETMINMAXINFO" },
	{ WM_GETTEXT,		"WM_GETTEXT" },
	{ WM_IME_NOTIFY,	"WM_IME_NOTIFY" },
	{ WM_IME_SETCONTEXT,	"WM_IME_SETCONTEXT" },
	{ WM_INITMENU,		"WM_INITMENU" },
	{ WM_INITMENUPOPUP,	"WM_INITMENUPOPUP" },
	{ WM_KEYDOWN,		"WM_KEYDOWN" },
	{ WM_KEYUP,		"WM_KEYUP" },
	{ WM_KILLFOCUS,		"WM_KILLFOCUS" },
	{ WM_MENUSELECT,	"WM_MENUSELECT" },
	{ WM_MOUSEACTIVATE,	"WM_MOUSEACTIVATE" },
	{ WM_MOUSEMOVE,		"WM_MOUSEMOVE" },
	{ WM_MOVE,		"WM_MOVE" },
	{ WM_MOVING,		"WM_MOVING" },
	{ WM_NCACTIVATE,	"WM_NCACTIVATE" },
	{ WM_NCCALCSIZE,	"WM_NCCALCSIZE" },
	{ WM_NCCREATE,		"WM_NCCREATE" },
	{ WM_NCHITTEST,		"WM_NCHITTEST" },
	{ WM_NCLBUTTONDOWN,	"WM_NCLBUTTONDOWN" },
	{ WM_NCMOUSEMOVE,	"WM_NCMOUSEMOVE" },
	{ WM_NCPAINT,		"WM_NCPAINT" },
	{ WM_PAINT,		"WM_PAINT" },
	{ WM_PARENTNOTIFY,	"WM_PARENTNOTIFY" },
	{ WM_QUERYNEWPALETTE,	"WM_QUERYNEWPALETTE" },
	{ WM_RBUTTONUP,		"WM_RBUTTONUP" },
	{ WM_SETCURSOR,		"WM_SETCURSOR" },
	{ WM_SETFOCUS,		"WM_SETFOCUS" },
	{ WM_SETTEXT,		"WM_SETTEXT" },
	{ WM_SHOWWINDOW,	"WM_SHOWWINDOW" },
	{ WM_SIZE,		"WM_SIZE" },
	{ WM_SIZING,		"WM_SIZING" },
	{ WM_STYLECHANGED,	"WM_STYLECHANGED" },
	{ WM_STYLECHANGING,	"WM_STYLECHANGING" },
	{ WM_SYSCOMMAND,	"WM_SYSCOMMAND" },
	{ WM_WINDOWPOSCHANGED,	"WM_WINDOWPOSCHANGED" },
	{ WM_WINDOWPOSCHANGING,	"WM_WINDOWPOSCHANGING" },
	/* custom ntwinio WINDOWS messages */
	{ WM_WVILE_CURSOR_ON,	"WM_WVILE_CURSOR_ON" },
	{ WM_WVILE_CURSOR_OFF,	"WM_WVILE_CURSOR_OFF" },
	/* *INDENT-ON* */

    };
    size_t n;
    static char temp[20];

    for (n = 0; n < TABLESIZE(table); n++) {
	if (table[n].code == code) {
	    return table[n].name;
	}
    }
    sprintf(temp, "%#x", code);
    return temp;
}

static char *
syscommand2s(unsigned code)
{
    static struct {
	WORD code;
	char *name;
    } table[] = {
	/* *INDENT-OFF* */
	{ SC_SIZE,		"SC_SIZE" },
	{ SC_MOVE,		"SC_MOVE" },
	{ SC_MINIMIZE,		"SC_MINIMIZE" },
	{ SC_MAXIMIZE,		"SC_MAXIMIZE" },
	{ SC_NEXTWINDOW,	"SC_NEXTWINDOW" },
	{ SC_PREVWINDOW,	"SC_PREVWINDOW" },
	{ SC_CLOSE,		"SC_CLOSE" },
	{ SC_VSCROLL,		"SC_VSCROLL" },
	{ SC_HSCROLL,		"SC_HSCROLL" },
	{ SC_MOUSEMENU,		"SC_MOUSEMENU" },
	{ SC_KEYMENU,		"SC_KEYMENU" },
	{ SC_ARRANGE,		"SC_ARRANGE" },
	{ SC_RESTORE,		"SC_RESTORE" },
	{ SC_TASKLIST,		"SC_TASKLIST" },
	{ SC_SCREENSAVE,	"SC_SCREENSAVE" },
	{ SC_HOTKEY,		"SC_HOTKEY" },
	/* *INDENT-ON* */

    };
    size_t n;
    static char temp[80];
    unsigned remainder = code & 0xf;

    for (n = 0; n < TABLESIZE(table); n++) {
	if (table[n].code == (code - remainder)) {
	    sprintf(temp, "%s+%X", table[n].name, remainder);
	    return temp;
	}
    }
    sprintf(temp, "%#x", code);
    return temp;
}

static void
TraceWindowRect(HWND hwnd)
{
    RECT wrect;
    int rows;
    int cols;
    GetWindowRect(hwnd, &wrect);
    cols = RectToCols(wrect);
    rows = RectToRows(wrect);
    TRACE(("... (%3d,%3d) (%3d,%3d), window is %dx%d cells (%dx%d pixels)%s\n",
	   wrect.top, wrect.left,
	   wrect.bottom, wrect.right,
	   rows,
	   cols,
	   RowToPixel(rows),
	   ColToPixel(cols),
	   IsZoomed(hwnd) ? " zoomed" : ""
	  ));
}

static void
TraceClientRect(HWND hwnd)
{
    RECT crect;
    int rows;
    int cols;
    GetClientRect(hwnd, &crect);
    cols = RectToCols(crect);
    rows = RectToRows(crect);
    TRACE(("... (%3d,%3d) (%3d,%3d), client is %dx%d cells (%dx%d pixels)\n",
	   crect.top, crect.left,
	   crect.bottom, crect.right,
	   rows,
	   cols,
	   RowToPixel(rows),
	   ColToPixel(cols)
	  ));
}

#else
#define TraceWindowRect(hwnd)	/* nothing */
#define TraceClientRect(hwnd)	/* nothing */
#endif

static HBRUSH
Background(HDC hdc)
{
    TRACE(("Background %#06x\n", GetBkColor(hdc)));
    return CreateSolidBrush(GetBkColor(hdc));
}

void
gui_resize(int cols, int rows)
{
    RECT crect;
    RECT wrect;
    int main_wide;
    int main_high;
    int text_wide;
    int text_high;

    /*
     * There's an undesirable feedback loop between gui_resize() and
     * ResizeClient() when the user changes the font.  Here's how it
     * goes:
     *
     * - user changes font
     * - use_font() is eventually called
     * - use_font() calls gui_resize()
     * - gui_resize calls MoveWindow(cur_win->main_hwnd, ...) which triggers
     *       a call to MainWndProc(), which calls ResizeClient()
     * - ResizeClient() calls gui_resize()
     *
     * ResizeClient(), unfortunately, recomputes the editor's rows and
     * cols and sometimes gets it wrong, especially when the number of
     * rows exceeds a magic value (e.g., 37 rows on a 1024 x 768
     * screen).
     *
     * To stop the feedback loop, check to see if an instance of
     * gui_resize() is active, and if so, exit.
     *
     * Note:  even though "gui_resize_in_progress" breaks the
     * aforementioned feedback loop, bear in mind that it's still not
     * possible to define a screen geometry that's greater than the
     * dimensions of the current desktop.  This restriction is enforced by
     * Windows when the first ShowWindow() call is made from
     * winvile_start().
     */
    if (gui_resize_in_progress && font_resize_in_progress)
	return;
    gui_resize_in_progress = TRUE;

    TRACE(("gui_resize(%d x %d)\n", rows, cols));
    TraceWindowRect(cur_win->main_hwnd);
    TraceClientRect(cur_win->main_hwnd);

    GetClientRect(cur_win->main_hwnd, &crect);
    GetWindowRect(cur_win->main_hwnd, &wrect);

    text_wide = ColToPixel(cols);
    text_high = RowToPixel(rows);
    wrect.right += text_wide - crect.right;
    wrect.bottom += text_high - crect.bottom;
#if FIXME_POSCHANGING
    main_wide = text_wide + (2 * cur_win->x_border) + SbWidth;
    main_high = text_high + (2 * cur_win->y_border) + cur_win->y_titles;
#else
    main_wide = wrect.right - wrect.left + SbWidth;
    main_high = wrect.bottom - wrect.top;
#endif

    TRACE(("... gui_resize -> (%d,%d) (%d,%d) main %dx%d, text %dx%d\n",
	   wrect.top,
	   wrect.left,
	   wrect.bottom,
	   wrect.right,
	   main_high,
	   main_wide,
	   text_high,
	   text_wide));

    MoveWindow(cur_win->main_hwnd,
	       wrect.left,
	       wrect.top,
	       main_wide,
	       main_high,
	       TRUE);
    MoveWindow(cur_win->text_hwnd,
	       0,
	       0,
	       text_wide,
	       text_high,
	       TRUE);

    TraceWindowRect(cur_win->main_hwnd);
    TraceClientRect(cur_win->main_hwnd);
#if OPT_SCROLLBARS
    update_scrollbar_sizes();
#endif
    TRACE(("... gui_resize finish\n"));
    gui_resize_in_progress = FALSE;
}

static int
AdjustedHeight(int high)
{
    int extra = cur_win->y_titles + (2 * cur_win->y_border);
    int rows;
    if (high > cur_win->xy_limit.bottom)
	high = cur_win->xy_limit.bottom;
    rows = (high - extra) / nLineHeight;
    if (rows < MIN_ROWS)
	rows = MIN_ROWS;
    return (rows * nLineHeight) + extra;
}

static int
AdjustedWidth(int wide)
{
    int extra = SbWidth + (2 * cur_win->x_border);
    int cols;
    if (wide > cur_win->xy_limit.right)
	wide = cur_win->xy_limit.right;
    cols = (wide - extra) / nCharWidth;
    if (cols < MIN_COLS)
	cols = MIN_COLS;
    return (cols * nCharWidth) + extra;
}

#if FIXME_POSCHANGING
static WINDOW_PROC_RETVAL
AdjustPosChanging(HWND hwnd, WINDOWPOS * pos)
{
    if (!(pos->flags & SWP_NOSIZE)) {
	int wide = AdjustedWidth(pos->cx);
	int high = AdjustedHeight(pos->cy);

	TRACE(("...%s position %d,%d, resize from %d,%d to %d,%d\n",
	       IsZoomed(hwnd) ? " zoomed" : "",
	       pos->y, pos->x,
	       pos->cy, pos->cx, high, wide));
	pos->cx = wide;
	pos->cy = high;
    }
    return 0;
}
#endif

static HWND
sizing_window(void)
{
    RECT crect;
    int szw = RSZ_WDW_WDTH, szh = RSZ_WDW_HGHT, szx, szy;

    GetClientRect(cur_win->main_hwnd, &crect);
    szx = crect.right / 2 - szw / 2;
    szy = crect.bottom / 2 - szh / 2;
    if (szx < 0 || szy < 0)
	szx = szy = CW_USEDEFAULT;
    return (CreateWindow("STATIC",
			 "",
			 WS_CHILD | WS_VISIBLE | SS_CENTER,
			 szx,
			 szy,
			 szw,
			 szh,
			 cur_win->main_hwnd,
			 (HMENU) 0,
			 vile_hinstance,
			 (LPVOID) 0));
}

/*
 * Handle WM_SIZING, forcing the screen size to stay in multiples of a
 * character cell.
 */
static WINDOW_PROC_RETVAL
AdjustResizing(HWND hwnd, WPARAM fwSide, RECT * rect)
{
    int wide = rect->right - rect->left;
    int high = rect->bottom - rect->top;
    int adjX = wide - AdjustedWidth(wide);
    int adjY = high - AdjustedHeight(high);

    TRACE(("AdjustResizing now (%d,%d) (%d,%d) (%dx%d pixels)\n",
	   rect->top, rect->left, rect->bottom, rect->right,
	   rect->bottom - rect->top,
	   rect->right - rect->left));

    TraceWindowRect(hwnd);
    TraceClientRect(hwnd);

    if (fwSide == WMSZ_LEFT
	|| fwSide == WMSZ_TOPLEFT
	|| fwSide == WMSZ_BOTTOMLEFT)
	rect->left += adjX;
    else if (fwSide == WMSZ_RIGHT
	     || fwSide == WMSZ_TOPRIGHT
	     || fwSide == WMSZ_BOTTOMRIGHT)
	rect->right -= adjX;

    if (fwSide == WMSZ_TOP
	|| fwSide == WMSZ_TOPLEFT
	|| fwSide == WMSZ_TOPRIGHT)
	rect->top += adjY;
    else if (fwSide == WMSZ_BOTTOM
	     || fwSide == WMSZ_BOTTOMLEFT
	     || fwSide == WMSZ_BOTTOMRIGHT)
	rect->bottom -= adjY;

    TRACE(("... AdjustResizing (%d,%d) (%d,%d) adjY:%d, adjX:%d\n",
	   rect->top, rect->left, rect->bottom, rect->right,
	   adjY, adjX));

    return TRUE;
}

static void
ResizeClient(void)
{
    int h, w;
    RECT crect;

    if (cur_win->closed) {
	TRACE(("ResizeClient ignored (closed)\n"));
	return;
    }

    /*
     * See comments in gui_resize() for an explanation of
     * gui_resize_in_progress.
     */
    if (gui_resize_in_progress && font_resize_in_progress)
	return;
    TRACE(("ResizeClient begin, currently %dx%d\n", term.rows, term.cols));
    TraceWindowRect(cur_win->main_hwnd);
    TraceClientRect(cur_win->main_hwnd);
    GetClientRect(cur_win->main_hwnd, &crect);

    h = RectToRows(crect);
    w = RectToCols(crect);

    /*
     * The WM_WINDOWPOSCHANGING message is supposed to allow modification
     * to keep a window in bounds.  But it doesn't work.  This does (by
     * forcing the calls on MoveWindow to have a "good" value).
     */
    if (h < MIN_ROWS)
	h = MIN_ROWS;

    if (w < MIN_COLS)
	w = MIN_COLS;

    if ((h > 1 && h != term.rows) || (w > 1 && w != term.cols)) {
	TRACE(("...ResizeClient %dx%d\n", h, w));
	vile_resizing = TRUE;
	newscreensize(h, w);
	SetRows(h);
	SetCols(w);
#if OPT_SCROLLBARS
	if (check_scrollbar_allocs() == TRUE)	/* no allocation failure */
	    update_scrollbar_sizes();
#endif
	vile_resizing = FALSE;
	TRACE(("...ResizeClient %dx%d\n", h, w));
    } else {
	TRACE(("ResizeClient ignored (h=%d, w=%d, vs %d x %d)\n",
	       h, w, term.rows, term.cols));
    }

    gui_resize(w, h);
    TRACE(("...ResizeClient finish\n"));
}

static int RedValue[NCOLORS], GreenValue[NCOLORS], BlueValue[NCOLORS];

/*
 * Ask for a palette setting string.
 * Format is "<entry> <red> <green> <blue>"
 */
int
SetRGBPalette(int f, int n)
{
#define RGB_DATA(name) { name, 0 }
    static struct {
	char *name;
	int value;
    } table[3] = {
	RGB_DATA("red"), RGB_DATA("green"), RGB_DATA("blue")
    };

    char *next;
    char prompt[NLINE + 1];
    char tstring[NLINE + 1];	/* string to add */
    const FSM_CHOICES *fp = name_to_choices("fcolor");
    int code;
    int count = 0;
    int status;			/* status return code */
    long value;

    *tstring = EOS;
    status = kbd_string("Color: ", tstring, sizeof(tstring), ' ',
			KBD_NORMAL, fsm_complete);
    if (status != TRUE)
	return (status);

    code = choice_to_code(fp, tstring, strlen(tstring));
    if (code < 0) {
	next = 0;
	code = strtol(tstring, &next, 0);
	if (code < 0 || code >= NCOLORS)
	    return FALSE;
    }

    /* ask for setting string */
    while (count < 3) {
	*tstring = EOS;
	sprintf(prompt, "Palette value (%s): ", table[count].name);
	status = mlreply(prompt, tstring, sizeof(tstring));
	if (status != TRUE)
	    return (status);
	next = 0;
	value = strtol(tstring, &next, 0);
	if (next == 0 || *next)
	    return FALSE;
	table[count++].value = value & 255;
    }

    RedValue[code] = table[0].value;
    GreenValue[code] = table[1].value;
    BlueValue[code] = table[2].value;

    set_winflags(TRUE, WFHARD | WFCOLR);
    vile_refresh(FALSE, 1);
    return (TRUE);
}

/*
 * Set default color map palette
 */
int
ResetRGBPalette(int f, int n)
{
    int code;
    int red, green, blue;
    int first = f ? n : 0;
    int last = f ? n + 1 : NCOLORS;

    for (code = first; code < last && code < NCOLORS; code++) {
	red = green = blue = 0;
	if (code & 1)
	    red = rgb_normal;
	if (code & 2)
	    green = rgb_normal;
	if (code & 4)
	    blue = rgb_normal;
	if (code & 8) {
	    if (red)
		red = rgb_bright;
	    if (green)
		green = rgb_bright;
	    if (blue)
		blue = rgb_bright;
	    if (code == 8) {
		red = rgb_gray;
		green = rgb_gray;
		blue = rgb_gray;
	    }
	}
	RedValue[code] = red;
	GreenValue[code] = green;
	BlueValue[code] = blue;
    }
    set_winflags(TRUE, WFHARD | WFCOLR);
    vile_refresh(FALSE, 1);
    return TRUE;
}

static COLORREF
color_of(int code)
{
    code = ctrans[code & (NCOLORS - 1)];

    return PALETTERGB(RedValue[code], GreenValue[code], BlueValue[code]);
}

/*
 * Translate the attribute into indices into the color table.
 */
static int
attr_to_colors(VIDEO_ATTR attr, int *fcolor, int *bcolor)
{
    int ninvert = FALSE;	/* normal colors inverted? */

    *fcolor = nfcolor;
    *bcolor = nbcolor;

    attr &= (VASPCOL | VACOLOR | VABOLD | VAITAL | VASEL | VAREV);
    if (attr) {
	ninvert = ((attr & (VASEL | VAREV)) == VASEL
		   || (attr & (VASEL | VAREV)) == VAREV);

	if (attr & VASPCOL)
	    *fcolor = (VCOLORNUM(attr) & (NCOLORS - 1));
	else if (attr & VACOLOR)
	    *fcolor = ((VCOLORNUM(attr)) & (NCOLORS - 1));

	if (ninvert) {
	    int temp = *bcolor;
	    *bcolor = *fcolor;
	    *fcolor = temp;
	}
	TRACE2(("attr_to_colors(%04x) fg=%2d, bg=%2d %s\n",
		attr, *fcolor, *bcolor, ninvert ? "INVERT" : ""));
    }
    return ninvert;
}

static int
fake_color(int current, int nominal)
{
    int x = nominal;
    if (current == ENUM_FCOLOR) {
	int r = GetRValue(nominal) ^ 0x40;
	int g = GetGValue(nominal) ^ 0x40;
	int b = GetBValue(nominal) ^ 0x40;
	x = PALETTERGB(r, g, b);
    }
    TRACE2(("fake_color(%d, %#x) = %#x\n", current, nominal, x));
    nominal = x;
    return nominal;
}

/*
 * Provide an array of inter-character spacing, needed to force bold and italic fonts
 * to align with the normal font.
 */
static INT *
intercharacter(int cols)
{
    if (cols == 0 || nCharWidth == 0)
	return 0;
    else {
	static INT *result;
	static unsigned length;

	if (++cols >= (int) length)
	    result = typereallocn(INT, result, length = cols);
	while (--cols >= 0)
	    result[cols] = nCharWidth;
	return result;
    }
}

static HFONT
GetMyFont(VIDEO_ATTR attr)
{
    unsigned n;

    if ((attr & (VAUL | VABOLD | VAITAL)) != 0) {
	for (n = 1; n < TABLESIZE(MyFonts); ++n) {
	    if ((attr & MyFonts[n].attr) == MyFonts[n].attr)
		break;
	}
    } else {
	n = 0;
    }

    if (n >= TABLESIZE(MyFonts)) {
	return GetMyFont(VAUL);
    } else if (!MyFonts[n].used) {
	LOGFONT logfont = vile_logfont;
	logfont.lfItalic = (BYTE) ((attr & VAITAL) != 0);
	logfont.lfUnderline = (BYTE) ((attr & VAUL) != 0);
	if (attr & VABOLD)
	    logfont.lfWeight = FW_SEMIBOLD;
	if ((MyFonts[n].font = CreateFontIndirect(&logfont)) != 0) {
	    MyFonts[n].used = TRUE;
	} else if ((attr & (VABOLD | VAITAL)) == (VABOLD | VAITAL)) {
	    return GetMyFont(VABOLD);
	} else if ((attr & VAUL) != 0) {
	    return GetMyFont(VAUL);
	} else {
	    n = 0;
	}
    } else if (MyFonts[n].font == 0) {
	n = 0;			/* live with previous failure */
    }

    return MyFonts[n].font;
}

static void
SetMyFont(HFONT font, LOGFONT * lf)
{
    unsigned n;

    for (n = 0; n < TABLESIZE(MyFonts); ++n) {
	if (MyFonts[n].used) {
	    DeleteObject(MyFonts[n].font);
	    MyFonts[n].used = FALSE;
	}
    }
    MyFonts[0].font = font;
    MyFonts[0].used = TRUE;
    if (lf != 0 && lf != &vile_logfont)
	vile_logfont = *lf;
}

static HDC
get_DC_with_Font(HFONT font)
{
    HDC hDC = GetDC(cur_win->text_hwnd);
    if (hDC != 0) {
	if (SelectObject(hDC, font) == 0)
	    ReleaseDC(cur_win->text_hwnd, hDC);
    }
    return hDC;
}

static void
nt_set_colors(HDC hdc, VIDEO_ATTR attr)
{
    int fcolor;
    int bcolor;
    int ninvert;

    SelectObject(hdc, GetMyFont(attr));
#ifdef GVAL_VIDEO
    attr ^= global_g_val(GVAL_VIDEO);
#endif
    ninvert = attr_to_colors(attr, &fcolor, &bcolor);

    if (fcolor < 0)
	fcolor = ninvert
	    ? fake_color(fcolor, default_fcolor)
	    : fake_color(fcolor, default_bcolor);
    else
	fcolor = color_of(fcolor);

    if (bcolor < 0)
	bcolor = ninvert
	    ? fake_color(bcolor, default_bcolor)
	    : fake_color(bcolor, default_fcolor);
    else
	bcolor = color_of(bcolor);

    SetTextColor(hdc, fcolor);
    SetBkColor(hdc, bcolor);
}

static int
fhide_cursor(void)
{
    TRACE(("fhide_cursor pos %#lx,%#lx (visible:%d, exists:%d)\n", ttrow,
	   ttcol, caret_visible, caret_exists));
    if (caret_visible) {
	HideCaret(cur_win->text_hwnd);
	caret_visible = 0;
    }
    if (caret_exists) {
	DestroyCaret();
	caret_exists = 0;
    }
    return 0;
}

static void
fshow_cursor(void)
{
    int x, y, width;
    POINT z;

    if (caret_disabled		/* reject display during font-dialog */
	|| ttrow > term.rows	/* reject bogus position in init */
	|| ttcol > term.cols)
	return;

    TRACE(("fshow_cursor pos %#lx,%#lx (visible:%d, exists:%d)\n", ttrow,
	   ttcol, caret_visible, caret_exists));
    x = ColToPixel(ttcol) + 1;
    y = RowToPixel(ttrow) + 1;
    if (caret_exists) {
	GetCaretPos(&z);
	if (x != z.x
	    || y != z.y)
	    fhide_cursor();
    }

    if (!caret_exists) {
	width = nCharWidth;	/* assume block cursor */
#if OPT_ICURSOR
	if (icursor) {
	    int vertbar = FALSE;

	    if (insertmode) {
		if (icursor_style == 1)
		    vertbar = TRUE;
	    } else {		/* command mode */
		if (icursor_style == 2)
		    vertbar = TRUE;
	    }
	    if (vertbar) {
		if (width < 5)
		    width = 2;
		else
		    width = 3;
	    }
	}
#endif
	TRACE(("...CreateCaret(%d,%d)\n", ttrow, ttcol));
	CreateCaret(cur_win->text_hwnd, (HBITMAP) 0, width, nLineHeight);
	caret_exists = 1;
	SetCaretPos(x, y);
	ShowCaret(cur_win->text_hwnd);
	caret_visible = 1;
    }
}

#if OPT_ICURSOR
static void
nticursor(int unused)
{
    if (icursor && caret_visible) {
	fhide_cursor();		/* Kill the old caret        */
	fshow_cursor();		/* And bring it back to life */
    }
}
#endif

static void
get_borders(void)
{
    SystemParametersInfo(SPI_GETWORKAREA, 0, &cur_win->xy_limit, 0);
    TRACE(("WORKAREA: %d,%d %d,%d\n",
	   cur_win->xy_limit.top,
	   cur_win->xy_limit.left,
	   cur_win->xy_limit.right,
	   cur_win->xy_limit.bottom));
    cur_win->x_border = GetSystemMetrics(SM_CXSIZEFRAME);
    cur_win->y_border = GetSystemMetrics(SM_CYSIZEFRAME);
    cur_win->y_titles = GetSystemMetrics(SM_CYCAPTION);
    TRACE(("X border: %d, Y border: %d\n", cur_win->x_border, cur_win->y_border));
    TRACE(("CYFRAME:   %d\n", GetSystemMetrics(SM_CYFRAME)));
    TRACE(("CYCAPTION: %d\n", cur_win->y_titles));
}

/*
 * Note 1: lpntm is a pointer to a TEXTMETRIC struct if FontType does not
 * have TRUETYPE_FONTTYPE set, but we only need the tmPitchAndFamily member,
 * which has the same alignment as in NEWTEXTMETRIC.
 *
 * Note 2: enumerate_fonts() is an instance of FONTENUMPROC.  The parameter
 * types specified for this function do not match the windows docu I
 * have in my possession (MSDN JAN 1998), but they do match the
 * declarations in wingdi.h .  When in doubt, use the source :-) .
 */
static int CALLBACK
enumerate_fonts(
		   const LOGFONT * lpelf,
		   const TEXTMETRIC * lpntm,
		   DWORD FontType,
		   LPARAM lParam)
{
    int code = 2;
    const LOGFONT *src = lpelf;
    LOGFONT *dst = ((LOGFONT *) lParam);

    if ((src->lfPitchAndFamily & 3) != FIXED_PITCH) {
	code = 1;
    } else {
	*dst = *src;
	if (src->lfCharSet == ANSI_CHARSET) {
	    code = 0;
	    TRACE(("Found good font:%s\n", lpelf->lfFaceName));
	}
	TRACE(("Found ok font:%s\n", lpelf->lfFaceName));
	TRACE(("Pitch/Family:   %#x\n", dst->lfPitchAndFamily));
    }

    return code;
}

static int
is_fixed_pitch(HFONT font)
{
    BOOL ok;
    HDC hDC;
    TEXTMETRIC metrics;

    hDC = get_DC_with_Font(font);
    ok = GetTextMetrics(hDC, &metrics);
    ReleaseDC(cur_win->text_hwnd, hDC);

    if (ok)
	ok = ((metrics.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0);

    TRACE(("is_fixed_pitch: %d\n", ok));
    TRACE(("Ave Text width: %d\n", metrics.tmAveCharWidth));
    TRACE(("Max Text width: %d\n", metrics.tmMaxCharWidth));
    TRACE(("Pitch/Family:   %#x\n", metrics.tmPitchAndFamily));

    return (ok);
}

static int
new_font(LOGFONT * lf)
{
    HFONT font = CreateFontIndirect(lf);

    if (font != 0) {
	if (is_fixed_pitch(font)) {
	    SetMyFont(font, lf);
	    TRACE(("created new font\n"));
	    return TRUE;
	} else
	    DeleteObject(font);
    }
    return FALSE;
}

static void
get_font(LOGFONT * lf)
{
    HDC hDC;

    SetMyFont(GetStockObject(MY_FONT), 0);
    hDC = GetDC(cur_win->text_hwnd);
    if (EnumFontFamilies(hDC, NULL, enumerate_fonts, (LPARAM) lf) <= 0) {
	TRACE(("Creating Pitch/Family: %#x\n", lf->lfPitchAndFamily));
	new_font(lf);
    }
    ReleaseDC(cur_win->text_hwnd, hDC);
}

static void
use_font(HFONT my_font)
{
    HDC hDC;
    TEXTMETRIC textmetric;
    int oLineHeight = nLineHeight;
    int oCharWidth = nCharWidth;

    hDC = get_DC_with_Font(my_font);
    GetTextMetrics(hDC, &textmetric);
    ReleaseDC(cur_win->text_hwnd, hDC);

    TRACE(("Text height:    %d\n", textmetric.tmHeight));
    TRACE(("Ave Text width: %d\n", textmetric.tmAveCharWidth));
    TRACE(("Max Text width: %d\n", textmetric.tmMaxCharWidth));
    TRACE(("Pitch/Family:   %#x\n", textmetric.tmPitchAndFamily));

    /*
     * We'll use the average text-width, since some fonts (e.g., Courier
     * New) have a bogus max text-width.
     */
    nLineHeight = textmetric.tmExternalLeading + textmetric.tmHeight;
    nCharWidth = textmetric.tmAveCharWidth;
    get_borders();

    font_resize_in_progress = (oLineHeight != nLineHeight)
	|| (oCharWidth != nCharWidth);

    gui_resize(term.cols, term.rows);

    font_resize_in_progress = FALSE;
}

static void
set_font(void)
{
    HDC hDC;
    CHOOSEFONT choose;

    TRACE(("set_font\n"));
    fhide_cursor();
    caret_disabled = TRUE;
    memset(&choose, 0, sizeof(choose));
    choose.lStructSize = sizeof(choose);
    choose.hwndOwner = cur_win->text_hwnd;
    choose.Flags = CF_SCREENFONTS
	| CF_FIXEDPITCHONLY
	| CF_FORCEFONTEXIST
	| CF_NOSCRIPTSEL
	| CF_INITTOLOGFONTSTRUCT;
    choose.lpLogFont = &vile_logfont;

    hDC = get_DC_with_Font(GetMyFont(0));
    GetTextFace(hDC, sizeof(vile_logfont.lfFaceName), vile_logfont.lfFaceName);
    ReleaseDC(cur_win->text_hwnd, hDC);

    vile_logfont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    vile_logfont.lfCharSet = ANSI_CHARSET;
    TRACE(("LOGFONT Facename:%s\n", vile_logfont.lfFaceName));

    if (ChooseFont(&choose)) {
	TRACE(("ChooseFont '%s'\n", vile_logfont.lfFaceName));
	if (new_font(&vile_logfont)) {
	    int saverow = ttrow;
	    int savecol = ttcol;
	    mlwrite("[Set font to %s]", vile_logfont.lfFaceName);
	    movecursor(saverow, savecol);
	    use_font(GetMyFont(0));
	    vile_refresh(FALSE, 0);
	    update(FALSE);
	} else {
	    mlforce("[Cannot create font]");
	}
    } else {
	mlforce("[No font selected]");
    }

    caret_disabled = FALSE;
    fshow_cursor();
    TRACE(("...set_font, LOGFONT Facename:%s\n", vile_logfont.lfFaceName));
}

static int
last_w32_error(int use_msg_box)
{
    if (use_msg_box)
	disp_win32_error(W32_SYS_ERROR, winvile_hwnd());
    else {
	char *msg = NULL;

	fmt_win32_error(W32_SYS_ERROR, &msg, 0);
	mlforce(msg);
	LocalFree(msg);
    }
    return (FALSE);
}

/*
 * Set font from string specification.  See the function parse_font_str()
 * in file w32misc.c for acceptable font string syntax.
 *
 * Prerequistes before calling this function:
 *
 *    winvile's windows and default font created.
 *
 * Set use_mb (Boolean):
 *	T -> errors reported via MessageBox.
 *	F -> errors reported via message line.
 *
 * Returns: T -> all is well, F -> failure.
 */
int
ntwinio_font_frm_str(
			const char *fontstr,
			int use_mb)
{
    int face_specified;
    HDC hdc;
    HFONT hfont;
    HWND hwnd;
    LOGFONT logfont;
    char *msg, font_mapper_face[LF_FACESIZE + 1];
    TEXTMETRIC metrics;
    FONTSTR_OPTIONS str_rslts;

    if (!parse_font_str(fontstr, &str_rslts)) {
	msg = "Font syntax invalid";
	if (use_mb)
	    MessageBox(winvile_hwnd(), msg, prognam, MB_ICONSTOP | MB_OK);
	else
	    mlforce(msg);
	return (FALSE);
    }
    hwnd = cur_win->text_hwnd;
    face_specified = (str_rslts.face[0] != '\0');
    if ((hdc = get_DC_with_Font(GetMyFont(0))) == 0) {
	(void) last_w32_error(use_mb);
	return (FALSE);
    }
    if (!face_specified) {
	/* user didn't specify a face name, get current name. */

	if ((GetTextFace(hdc, sizeof(str_rslts.face), str_rslts.face)) == 0) {
	    (void) last_w32_error(use_mb);
	    ReleaseDC(hwnd, hdc);
	    return (FALSE);
	}
    }

    /* Build up LOGFONT data structure. */
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfWeight = (str_rslts.bold) ? FW_BOLD : FW_NORMAL;
    logfont.lfHeight = -MulDiv(str_rslts.size,
			       GetDeviceCaps(hdc, LOGPIXELSY),
			       72);
    if (str_rslts.italic)
	logfont.lfItalic = TRUE;
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    vl_strncpy(logfont.lfFaceName, str_rslts.face, LF_FACESIZE);
    logfont.lfFaceName[LF_FACESIZE - 1] = '\0';
    if (!((hfont = CreateFontIndirect(&logfont)) != 0 && SelectObject(hdc, hfont))) {
	(void) last_w32_error(use_mb);
	if (hfont)
	    DeleteObject(hfont);
	ReleaseDC(hwnd, hdc);
	return (FALSE);
    }

    /*
     * The font mapper will substitute some other font for a font request
     * that cannot exactly be met.  Ck first to see if the face name
     * matches the user chosen face (if applicatble).
     */
    if (face_specified) {
	if ((GetTextFace(hdc, sizeof(font_mapper_face), font_mapper_face))
	    == 0) {
	    (void) last_w32_error(use_mb);
	    ReleaseDC(hwnd, hdc);
	    DeleteObject(hfont);
	    return (FALSE);
	}
	if (stricmp(font_mapper_face, str_rslts.face) != 0) {
	    ReleaseDC(hwnd, hdc);
	    DeleteObject(hfont);
	    msg = "Font face unknown or size/style unsupported";
	    if (use_mb)
		MessageBox(winvile_hwnd(), msg, prognam, MB_ICONSTOP | MB_OK);
	    else
		mlforce(msg);
	    return (FALSE);
	}
    }

    /* Next, font must be fixed pitch. */
    if (!GetTextMetrics(hdc, &metrics)) {
	(void) last_w32_error(use_mb);
	ReleaseDC(hwnd, hdc);
	DeleteObject(hfont);
	return (FALSE);
    }
    if ((metrics.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0) {
	/* Misnamed constant! */

	ReleaseDC(hwnd, hdc);
	DeleteObject(hfont);
	msg = "Font not fixed pitch";
	if (use_mb)
	    MessageBox(winvile_hwnd(), msg, prognam, MB_ICONSTOP | MB_OK);
	else
	    mlforce(msg);
	return (FALSE);
    }
    ReleaseDC(hwnd, hdc);	/* finally done with this */
    SetMyFont(hfont, &logfont);
    use_font(GetMyFont(0));
    vile_refresh(FALSE, 0);
    return (TRUE);
}

char *
ntwinio_current_font(void)
{
    static char *buf;
    HDC hdc;
    HWND hwnd;
    LONG size;
    char *style;

    if (!buf) {
	buf = castalloc(char,
			sizeof("bold-italic") +
			LF_FACESIZE +
			16);	/* space for delimiters and point size */
	if (!buf)
	    return ("out of memory");
    }
    hwnd = cur_win->text_hwnd;
    if ((hdc = get_DC_with_Font(GetMyFont(0))) == 0) {
	char *msg = NULL;
	fmt_win32_error(W32_SYS_ERROR, &msg, 0);
	return (msg);
	/* "msg" leaks here, but this code path should never be taken. */
    }
    if (vile_logfont.lfWeight == FW_BOLD && vile_logfont.lfItalic)
	style = "bold-italic";
    else if (vile_logfont.lfWeight == FW_BOLD)
	style = "bold";
    else if (vile_logfont.lfItalic)
	style = "italic";
    else
	style = NULL;
    size = MulDiv(labs(vile_logfont.lfHeight),
		  72,
		  GetDeviceCaps(hdc, LOGPIXELSY));
    sprintf(buf,
	    "%s,%ld%s%s",
	    vile_logfont.lfFaceName,
	    size,
	    (style) ? "," : "",
	    (style) ? style : "");
    ReleaseDC(hwnd, hdc);
    return (buf);
}

#if OPT_TITLE
static void
nttitle(const char *title)
{				/* set the current window title */
    if (title != 0)
	SetWindowText(winvile_hwnd(), title);
}
#endif

static void
scflush(void)
{
    if (cur_pos && !vile_resizing) {
	HDC hdc;

	TRACE(("PUTC:flush %2d (%2d,%2d) (%.*s)\n", cur_pos, crow, ccol,
	       cur_pos, &CELL_TEXT(crow, ccol)));

	hdc = GetDC(cur_win->text_hwnd);
	nt_set_colors(hdc, cur_atr);

	ExtTextOut(hdc,
		   ColToPixel(ccol),
		   RowToPixel(crow),
		   0,
		   (RECT *) 0,
		   &CELL_TEXT(crow, ccol), cur_pos,
		   intercharacter(cur_pos));

	ReleaseDC(cur_win->text_hwnd, hdc);
    }
    ccol = ccol + cur_pos;
    cur_pos = 0;
}

#if OPT_COLOR
static void
ntfcol(int color)
{				/* set the current output color */
    scflush();
    nfcolor = color;
}

static void
ntbcol(int color)
{				/* set the current background color */
    scflush();
    nbcolor = color;
}
#endif

static void
ntflush(void)
{
    scflush();
    SetCaretPos(ColToPixel(ccol), RowToPixel(crow));
}

static void
ntmove(int row, int col)
{
    scflush();
    crow = (short) row;
    ccol = (short) col;
}

/* erase to the end of the line */
static void
nteeol(void)
{
    HDC hDC;
    HBRUSH brush;
    RECT rect;
    int x;

    scflush();

    TRACE(("NTEEOL %d,%d, atr %#x\n", crow, ccol, cur_atr));
    for (x = ccol; x < term.cols; x++) {
	CELL_TEXT(crow, x) = ' ';
	CELL_ATTR(crow, x) = cur_atr;
    }

    GetClientRect(cur_win->text_hwnd, &rect);
    rect.left = ColToPixel(ccol);
    rect.top = RowToPixel(crow);
    rect.right = ColToPixel(term.cols);
    rect.bottom = RowToPixel(crow + 1);

    hDC = GetDC(cur_win->text_hwnd);
    nt_set_colors(hDC, cur_atr);
    brush = Background(hDC);
    FillRect(hDC, &rect, brush);
    DeleteObject(brush);
    ReleaseDC(cur_win->text_hwnd, hDC);
}

#if	OPT_FLASH
static void
flash_display(void)
{
    PostMessage(cur_win->text_hwnd, WM_WVILE_FLASH_START, 0, 0);
    PostMessage(cur_win->text_hwnd, WM_WVILE_FLASH_STOP, 0, 0);
}
#endif

static void
ntbeep(void)
{
#if	OPT_FLASH
    if (global_g_val(GMDFLASH)) {
	flash_display();
	return;
    }
#endif
    MessageBeep(0xffffffff);
}

/*
 * Move 'n' lines starting at 'from' to 'to'
 *
 * OPT_PRETTIER_SCROLL is prettier but slower -- it scrolls a line at a time
 *	instead of all at once.
 */

/* move howmany lines starting at from to to */
static void
ntscroll(int from, int to, int n)
{
    HDC hDC;
    HBRUSH brush;
    RECT region;
    RECT tofill;

    scflush();
    if (to == from)
	return;
#if OPT_PRETTIER_SCROLL
    if (ABS(from - to) > 1) {
	ntscroll(from, (from < to) ? to - 1 : to + 1, n);
	if (from < to)
	    from = to - 1;
	else
	    from = to + 1;
    }
#endif

    region.left = 0;
    region.right = (SHORT) ColToPixel(term.cols);

    if (from > to) {
	region.top = (SHORT) RowToPixel(to);
	region.bottom = (SHORT) RowToPixel(from + n);
    } else {
	region.top = (SHORT) RowToPixel(from);
	region.bottom = (SHORT) RowToPixel(to + n);
    }

    TRACE(("ScrollWindowEx from=%d, to=%d, n=%d  (%d,%d)/(%d,%d)\n",
	   from, to, n,
	   region.left, region.top,
	   region.right, region.bottom));

    ScrollWindowEx(
		      cur_win->text_hwnd,	/* handle of window to scroll */
		      0,	/* amount of horizontal scrolling */
		      RowToPixel(to - from),	/* amount of vertical scrolling */
		      &region,	/* address of structure with scroll rectangle */
		      &region,	/* address of structure with clip rectangle */
		      NULL,	/* handle of update region */
		      &tofill,	/* invalidated rectangle */
		      0		/* scrolling flags */
	);

    /* Erase invalidated rectangle */
    TRACE(("ntscroll tofill: (%d,%d)/(%d,%d)\n",
	   tofill.left, tofill.top,
	   tofill.right, tofill.bottom));
    hDC = GetDC(cur_win->text_hwnd);
    nt_set_colors(hDC, cur_atr);
    brush = Background(hDC);
    FillRect(hDC, &tofill, brush);
    DeleteObject(brush);
    ReleaseDC(cur_win->text_hwnd, hDC);

    /*
     * force text_hwnd's WindProc to service a WM_PAINT message -- gives
     * faster screen updates.
     */
    UpdateWindow(cur_win->text_hwnd);
}

/*
 * vile very rarely generates any of the ASCII printing control characters
 * except for a few hand coded routines but we have to support them anyway.
 */

/* put a character at the current position in the current colors */
static void
ntputc(int ch)
{
    /* This is an optimization for the most common case. */
    if (ch >= ' ') {
	CELL_TEXT(crow, ccol + cur_pos) = (char) ch;
	CELL_ATTR(crow, ccol + cur_pos) = cur_atr;
	cur_pos++;
    } else {

	switch (ch) {

	case '\b':
	    scflush();
	    if (ccol)
		ccol--;
	    break;

	case '\a':
	    ntbeep();
	    break;

	case '\t':
	    scflush();
	    do {
		CELL_TEXT(crow, ccol + cur_pos) = ' ';
		CELL_ATTR(crow, ccol + cur_pos) = cur_atr;
		cur_pos++;
	    } while ((ccol + cur_pos) % 8 != 0);
	    break;

	case '\r':
	    scflush();
	    ccol = 0;
	    break;

	case '\n':
	    scflush();
	    if (crow < term.rows - 1)
		crow++;
	    else
		ntscroll(1, 0, term.rows - 1);
	    break;

	default:
	    CELL_TEXT(crow, ccol + cur_pos) = (char) ch;
	    CELL_ATTR(crow, ccol + cur_pos) = cur_atr;
	    cur_pos++;
	    break;
	}
    }
}

static void
nteeop(void)
{
    HDC hDC;
    HBRUSH brush;
    RECT rect;
    int y, x;

    scflush();

    TRACE(("NTEEOP %d,%d, atr %#x\n", crow, ccol, cur_atr));
    for (y = crow; y < term.rows; y++) {
	for (x = 0; x < term.cols; x++) {
	    CELL_TEXT(y, x) = ' ';
	    CELL_ATTR(y, x) = cur_atr;
	}
    }

    rect.left = ColToPixel(ccol);
    rect.top = RowToPixel(crow);
    rect.right = ColToPixel(term.cols);
    rect.bottom = RowToPixel(term.rows);

    if (!vile_resizing) {
	hDC = GetDC(cur_win->text_hwnd);
	nt_set_colors(hDC, cur_atr);
	brush = Background(hDC);
	FillRect(hDC, &rect, brush);
	DeleteObject(brush);
	ReleaseDC(cur_win->text_hwnd, hDC);
    }
}

static void
ntrev(UINT reverse)
{				/* change reverse video state */
    scflush();
    cur_atr = (VIDEO_ATTR) reverse;
}

static int
ntcres(const char *res)
{				/* change screen resolution */
    scflush();
    return 0;
}

static void
ntopen(void)
{
    TRACE(("ntopen\n"));

    set_colors(NCOLORS);
    set_palette(initpalettestr);
    ResetRGBPalette(FALSE, 1);
}

static int old_title_set = 0;
static char old_title[256];
static int orig_title_set = 0;
static char orig_title[256];

static void
ntclose(void)
{
    TRACE(("ntclose\n"));

    scflush();
    ntmove(term.rows - 1, 0);
    nteeol();
    ntflush();
}

static void
ntkopen(void)
{				/* open the keyboard */
}

static void
ntkclose(void)
{				/* close the keyboard */
}

#define isModified(state) (state & \
		(LEFT_CTRL_PRESSED \
		 | RIGHT_CTRL_PRESSED \
		 | LEFT_ALT_PRESSED \
		 | RIGHT_ALT_PRESSED \
		 | SHIFT_PRESSED))

static int
modified_key(int key, DWORD state)
{
    key |= mod_KEY;
    if (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
	key |= mod_CTRL;
    if (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
	key |= mod_ALT;
    if (state & SHIFT_PRESSED)
	key |= mod_SHIFT;

    return key;
}

/*
 * The so-called extended keys include the function keys, non-digit keys on
 * the numeric keypad, the cursor-keypad and editing keypad.  The treatment of
 * the tab key is inconsistent:  We can get a tab with control modifier, but a
 * shift-tab will still yield a WM_CHAR later.
 */
static struct keyxlate_struct {
    int windows;
    int vile;
} keyxlate[] = {
    /* *INDENT-OFF* */
    { VK_NEXT,		KEY_Next },
    { VK_PRIOR,		KEY_Prior },
    { VK_END,		KEY_End },
    { VK_HOME,		KEY_Home },
    { VK_LEFT,		KEY_Left },
    { VK_RIGHT,		KEY_Right },
    { VK_UP,		KEY_Up },
    { VK_DOWN,		KEY_Down },
    { VK_INSERT,	KEY_Insert },
    { VK_DELETE,	KEY_Delete },
    { VK_HELP,		KEY_Help },
    { VK_SELECT,	KEY_Select },
#if 0
    /* Merely pressing the Alt key generates a VK_MENU key event. */
    { VK_MENU,		KEY_Menu },
#endif
    { VK_F1,		KEY_F1 },
    { VK_F2,		KEY_F2 },
    { VK_F3,		KEY_F3 },
    { VK_F4,		KEY_F4 },
    { VK_F5,		KEY_F5 },
    { VK_F6,		KEY_F6 },
    { VK_F7,		KEY_F7 },
    { VK_F8,		KEY_F8 },
    { VK_F9,		KEY_F9 },
    { VK_F10,		KEY_F10 },
    { VK_F11,		KEY_F11 },
    { VK_F12,		KEY_F12 },
    { VK_F13,		KEY_F13 },
    { VK_F14,		KEY_F14 },
    { VK_F15,		KEY_F15 },
    { VK_F16,		KEY_F16 },
    { VK_F17,		KEY_F17 },
    { VK_F18,		KEY_F18 },
    { VK_F19,		KEY_F19 },
    { VK_F20,		KEY_F20 },
    { VK_F21,		KEY_F21 },
    { VK_F22,		KEY_F22 },
    { VK_F23,		KEY_F23 },
    { VK_F24,		KEY_F24 },
    /* winuser.h stops with VK_F24 */
    /* Allow ^-6 to invoke the alternate-buffer command, a la Unix.  */
    { '6',		'6' },
    /* Support recognition of ^@ */
    { '2',		'2' },
    /* *INDENT-ON* */

};

static int savedChar;
static int saveCount = 0;

static int
decode_key_event(KEY_EVENT_RECORD * irp)
{
    int i, key;
    struct keyxlate_struct *keyp;

    if ((key = (UCHAR) irp->uChar.AsciiChar) != 0)
	return key;

#if VILE_OLE
    if (redirect_keys &&
	oleauto_redirected_key(irp->wVirtualKeyCode, irp->dwControlKeyState)) {
	return (KYREDIR);	/* Key sent to another window. */
    }
#endif

    key = NOKYMAP;
    for (i = 0, keyp = keyxlate; i < TABLESIZE(keyxlate); i++, keyp++) {
	if (keyp->windows == irp->wVirtualKeyCode) {
	    DWORD state = irp->dwControlKeyState;

	    /*
	     * There are a few special keys that we must deal with here on an
	     * ad hoc basis:
	     *
	     * ALT+F4 - This should _never_ be remapped by any user (nor
	     * messed with by vile).
	     *
	     * SHIFT+6 - This is actually '^^' -- leave it alone.
	     */
	    if ((keyp->windows == VK_F4
		 && (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))
		|| (keyp->windows == '6' && (state & SHIFT_PRESSED))) {
		TRACE(("decode_key_event - special\n"));
		break;
	    }

	    /*
	     * Add the modifiers that we recognize.  Specifically, we don't
	     * care about ENHANCED_KEY, since we already have portable
	     * pageup/pagedown and arrow key bindings that would be lost if we
	     * used the Win32-only definition.
	     */
	    if (isModified(state)) {
		key = modified_key(keyp->vile, state);
		if (keyxlate[i].vile == '2') {
		    if ((key & mod_CTRL) && ((key & mod_ALT) == 0)) {
			/* either ^2 or ^@, => nul char */

			key = 0;
		    } else if ((key & mod_SHIFT) &&
			       ((key & (mod_ALT | mod_CTRL)) == 0)) {
			/*
			 * this will be mapped to '@' if we let Windows do
			 * the translation
			 */

			key = NOKYMAP;
		    }
		}
	    } else {
		key = keyp->vile;
	    }
	    TRACE(("decode_key_event %#x (%#x) -> %#x\n",
		   irp->wVirtualKeyCode, state, key));
	    break;
	}
    }

    return key;
}

static int
get_keyboard_state(void)
{
    int result = 0;

    if (GetKeyState(VK_CONTROL) < 0)
	result |= (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
    if (GetKeyState(VK_MENU) < 0)
	result |= (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
    if (GetKeyState(VK_SHIFT) < 0)
	result |= SHIFT_PRESSED;
    return result;
}

static void
enable_popup_menu(void)
{
    enable_popup = !enable_popup;
    set_global_g_val(GMDPOPUPMENU, enable_popup);
    CheckMenuItem(vile_menu,
		  IDM_MENU,
		  MF_BYCOMMAND | (enable_popup ? MF_CHECKED : MF_UNCHECKED));
    DrawMenuBar(cur_win->main_hwnd);
}

typedef struct popup_menu_info_struct {
    int menu_id;
    char *menu_name;
    char *vile_cmdname;

} POPUP_MENU_INFO;

static POPUP_MENU_INFO popup_menu_tbl[] =
{
    /* *INDENT-OFF* */
    {IDM_OPEN,		"&Open...",	"winopen"},
    {IDM_SAVE_AS,	"&Save As...",	"winsave"},
    {IDM_CHDIR,		"C&D...",	"wincd"},
    {IDM_UNDO,		"&Undo",	"undo-changes-backward"},
    {IDM_REDO,		"&Redo",	"redo-changes-forward"},
    {IDM_CUT,		"Cu&t",		"cut-to-clipboard"},
    {IDM_COPY,		"&Copy",	"copy-unnamed-reg-to-clipboard"},
    {IDM_PASTE,		"&Paste",	"paste-from-clipboard"},
    {IDM_DELETE,	"De&lete",	"delete-text-selection"},
    {IDM_SELECT_ALL,	"Select &All",	"select-all"},
    /* *INDENT-ON* */

};

static void
invoke_popup_menu(MSG msg)
{
    char accel[NSTRING], buf[NSTRING * 2];
    AREGION ar;
    BUFFER *bp;
    const CMDFUNC *cmd;
    int i, limit, flag, seltxt, samebuf;
    POINT point;
    POPUP_MENU_INFO *ptbl;

    TRACE(("invoke_popup_menu\n"));
    if (popup_menu == NULL) {
	popup_menu = LoadMenu(vile_hinstance,
			      "WinvilePopMenu");
	popup_menu = GetSubMenu(popup_menu, 0);
    }
    CheckMenuItem(popup_menu, IDM_MENU, MF_BYCOMMAND | MF_CHECKED);

    /* add accelerators (key bindings) to RMB */
    limit = sizeof(popup_menu_tbl) / sizeof(popup_menu_tbl[0]);
    for (i = 0, ptbl = popup_menu_tbl; i < limit; i++, ptbl++) {
	if ((cmd = engl2fnc(ptbl->vile_cmdname)) != NULL) {

	    if (fnc2kcod(cmd) != -1) {
		/* cmd has a binding */

		(void) kcod2prc(fnc2kcod(cmd), accel);
		sprintf(buf, "%s\t%s", ptbl->menu_name, accel);
	    } else {
		/* rewrite menu text to clear out possible unmapped bindings */

		vl_strncpy(buf, ptbl->menu_name, sizeof(buf));
	    }
	    ModifyMenu(popup_menu,
		       ptbl->menu_id,
		       MF_BYCOMMAND | MF_STRING,
		       ptbl->menu_id,
		       buf);
	}
    }

    /* Can't undo/redo if not applicable. */
    flag = (undo_ok())? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(popup_menu, IDM_UNDO, MF_BYCOMMAND | flag);
    flag = (redo_ok())? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(popup_menu, IDM_REDO, MF_BYCOMMAND | flag);

    /* Can't "select all" in empty buffer. */
    flag = (curbp->b_bytecount > 0) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(popup_menu, IDM_SELECT_ALL, MF_BYCOMMAND | flag);

    /* Can't paste data from clipboard if not TEXT. */
    flag = (IsClipboardFormatAvailable(CF_TEXT)) ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(popup_menu, IDM_PASTE, MF_BYCOMMAND | flag);

    /* Do we have a text selection? */
    if ((bp = get_selection_buffer_and_region(&ar)) != NULL)
	seltxt = MF_ENABLED;
    else
	seltxt = MF_GRAYED;

    /* Is the text selection in the current buffer? */
    if ((seltxt == MF_ENABLED) && (bp == curbp))
	samebuf = MF_ENABLED;
    else
	samebuf = MF_GRAYED;

    /* Modify menu items that depend on seltxt and samebuf flags. */
    EnableMenuItem(popup_menu, IDM_COPY, MF_BYCOMMAND | seltxt);
    EnableMenuItem(popup_menu, IDM_CUT, MF_BYCOMMAND | samebuf);
    EnableMenuItem(popup_menu, IDM_DELETE, MF_BYCOMMAND | samebuf);

    /* Show popup menu. */
    point.x = LOWORD(msg.lParam);
    point.y = HIWORD(msg.lParam);
    ClientToScreen(msg.hwnd, &point);
    TrackPopupMenu(popup_menu,
		   0,
		   point.x,
		   point.y,
		   0,
		   msg.hwnd,
		   NULL);
    TRACE(("...invoke_popup_menu\n"));
}

static DIALOG_PROC_RETVAL CALLBACK
AboutBoxProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    char buf[512];
    HWND hwnd;

    switch (iMsg) {
    case WM_INITDIALOG:
	/* set about box dialog title */
	sprintf(buf, "About %s", prognam);
	SetWindowText(hDlg, buf);

	/* announce program version */
	hwnd = GetDlgItem(hDlg, IDM_ABOUT_PROGNAME);
	sprintf(buf, "%s\n%s%s", prognam, version, VILE_PATCHLEVEL);
	SetWindowText(hwnd, buf);

	/* talk about copyright */
	hwnd = GetDlgItem(hDlg, IDM_ABOUT_COPYRIGHT);
	sprintf(buf,
		"\nCopyright \xA9 Thomas Dickey 1997-2005,2006\n\n"
		"%s is free software, distributed under the terms of the GNU "
		"Public License (see COPYING).",
		prognam);
	SetWindowText(hwnd, buf);
	w32_center_window(hDlg, cur_win->main_hwnd);
	return (TRUE);
    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDOK:
	case IDCANCEL:
	    EndDialog(hDlg, 0);
	    return (TRUE);
	}
	break;
    }
    return (FALSE);
}

static int
handle_builtin_menu(WPARAM code)
{
    int result = TRUE, cmd = LOWORD(code);

    TRACE(("handle_builtin_menu code=%#x\n", code));
    switch (cmd) {
    case IDM_ABOUT:
	DialogBox(vile_hinstance, "AboutBox", cur_win->main_hwnd, AboutBoxProc);
	break;
    case IDM_OPEN:
	winopen_dir(NULL);
	update(FALSE);
	break;
    case IDM_CHDIR:
	wincd_dir(NULL);
	update(FALSE);
	break;
    case IDM_PRINT:
	winprint(0, 0);
	update(FALSE);
	break;
    case IDM_PAGE_SETUP:
	winpg_setup(0, 0);
	update(FALSE);
	break;
    case IDM_FAVORITES:
	winopen_dir(get_favorites());
	update(FALSE);
	break;
    case IDM_SAVE_AS:
	winsave_dir(NULL);
	update(FALSE);
	break;
    case IDM_SELECT_ALL:
	sel_all(0, 0);
	update(FALSE);
	break;
    case IDM_FONT:
	set_font();
	break;
    case IDM_MENU:
	enable_popup_menu();
	update(FALSE);
	break;
    case IDM_PASTE:
	/* it's okay to paste into minibuffer if reading msg line data */
	if ((!reading_msg_line) && b_val(curbp, MDVIEW))
	    return rdonly();
	mayneedundo();
	fhide_cursor();
	cbrdpaste(FALSE, FALSE);
	update(FALSE);
	fshow_cursor();
	break;
    case IDM_REDO:
	forwredo(FALSE, FALSE);
	update(FALSE);
	break;
    case IDM_UNDO:
	backundo(FALSE, FALSE);
	update(FALSE);
	break;
    case IDM_COPY:
	/*
	 * Copy selected text to clipboard.  Start by first yanking selected
	 * text to unnamed register.  In most cases this won't be necessary,
	 * especially if mouse is used to navigate and make selections.
	 * However, there are cases where the user might select some text
	 * (causes automatic yank to unnamed register), do something to
	 * clobber the unnamed register, and then use the menu system to
	 * attempt to copy selected text.
	 */

	sel_yank(0);
	cbrdcpy_unnamed(FALSE, FALSE);
	update(FALSE);
	break;
    case IDM_CUT:
    case IDM_DELETE:
	if (b_val(curbp, MDVIEW))
	    return rdonly();
	mayneedundo();
	fhide_cursor();
	w32_del_selection(LOWORD(code) == IDM_CUT);
	update(FALSE);
	fshow_cursor();
	break;
    default:
	if (cmd >= IDM_RECENT_FILES && cmd < IDM_RECENT_FLDRS) {
	    (void) edit_recent_file(cmd);
	    update(TRUE);	/* force cursor out of mini-buffer */
	} else if (cmd >= IDM_RECENT_FLDRS &&
		   cmd < (IDM_RECENT_FLDRS + MAX_RECENT_FLDRS)) {
	    (void) cd_recent_folder(cmd);
	    update(TRUE);	/* force cursor out of mini-buffer */
	} else
	    result = FALSE;
	break;
    }
    TRACE(("...handle_builtin_menu code ->%d\n", result));
    return result;
}

static void
GetMousePos(POINT * result)
{
    DWORD dword;
    POINTS points;

    dword = GetMessagePos();
    points = MAKEPOINTS(dword);
    POINTSTOPOINT((*result), points);
    ScreenToClient(cur_win->main_hwnd, result);
    result->x /= nCharWidth;
    result->y /= nLineHeight;
}

// Get current mouse position.
// If the mouse is above/below the current window
// then scroll the window down/up proportinally
// to the distance above/below.
// This function is called from WM_TIMER when
// the mouse is captured and a wipe selection is
// active.
static void
AutoScroll(WINDOW *wp)
{
#define DVSR    10
#define INCR    6
#define TRIGGER (DVSR + INCR)

    POINT current;
    int Scroll = 0;
    static int ScrollCount = 0, Throttle = INCR;
    GetMousePos(&current);

    if (wp == 0)
	return;

    // Determine if we are above or below the window,
    // and if so, how far...
    if (current.y < wp->w_toprow) {
	// Above the window
	// Scroll = wp->w_toprow - current.y;
	Scroll = 1;
    }
    if (current.y > mode_row(wp)) {
	// Below
	// Scroll = current.y - mode_row(wp);
	// Scroll *= -1;
	Scroll = -1;
    }

    TRACE(("AutoScroll:(%d, %d, %d)\n", Scroll, ScrollCount, Throttle));
    if (Scroll) {
	int row;
	if (Scroll > 0) {
	    row = wp->w_toprow;
	} else {
	    row = mode_row(wp) - 1;
	}

	// Scroll the pre-determined amount, ensuring at least one line of
	// window movement per timer tick.  Note also that ScrollScale is
	// signed, so it will be negative if we want to scroll down.
	mvupwind(TRUE, Scroll * max(ScrollCount, TRIGGER) / (Throttle + DVSR));

	// Set the cursor. Column doesn't really matter, it will
	// get updated as soon as we get back into the window...
	setcursor(row, 0) && sel_extend(TRUE, TRUE);
	(void) update(TRUE);
	ScrollCount++;
	if (ScrollCount > TRIGGER && Throttle > 0 && ScrollCount % INCR == 0)
	    Throttle--;
    } else {
	// Reset counters
	Throttle = INCR;
	ScrollCount = 0;
    }
#undef DVSR
#undef INCR
#undef TRIGGER
}

static int
MouseClickSetPos(POINT * result, int *onmode)
{
    WINDOW *wp;

    GetMousePos(result);

    TRACE(("GETC:setcursor(%d, %d)\n", result->y, result->x));

    /*
     * If we're getting a button-down in a window, allow it to maybe begin
     * a selection (if the mouse has actually moved).  A button-down on
     * its modeline will allow resizing the window.
     */
    *onmode = FALSE;
    if ((wp = row2window(result->y)) != 0) {
	if (result->y == mode_row(wp)) {
	    *onmode = TRUE;
	    return TRUE;
	}
	return setcursor(result->y, result->x);
    }
    return FALSE;
}

/*
 * Shrink a window by dragging the modeline
 */
static int
adjust_window(WINDOW *wp, POINT * current, POINT * latest)
{
    if (latest->y == mode_row(wp)) {
	if (current->y != latest->y) {
	    WINDOW *save_wp = curwp;
	    set_curwp(wp);
	    shrinkwind(FALSE, latest->y - current->y);
	    set_curwp(save_wp);
	    update(TRUE);
	}
	*latest = *current;
	return TRUE;
    }
    return FALSE;
}

/*
 * FUNCTION
 *   mousemove(int    *sel_pending,
 *             int    onmode,
 *             POINT  *first,
 *             POINT  *latest,
 *             MARK   *lmbdn_mark,
 *             WINDOW *wp)
 *
 *   sel_pending - Boolean, T -> client has recorded a left mouse button (LMB)
 *                 click, and so, a selection is pending.
 *
 *   onmode      - Boolean, T -> left mouse button was initially clicked
 *                 down on a mode line.
 *
 *   first       - editor row/col coordinates where LMB was initially recorded.
 *
 *   latest      - during the mouse move, assuming the LMB is still down,
 *                 "latest" represents the editor's current row/col
 *                 position w/in the same window where "first" was recorded.
 *
 *   lmbdn_mark  - editor MARK when LMB was initially recorded.
 *
 *   wp          - pointer to editor window where LMB was initially recorded.
 *
 * DESCRIPTION
 *   Using several state variables, this function handles all the semantics
 *   of a left mouse button "MOVE" event.  The semantics are as follows:
 *
 *   1) This function will not be called unless that LMB is down (enforced
 *      by client).
 *   2) if the LMB was initially clicked down on a mode line, then the only
 *      operation that can occur is window resizing.
 *   3) if not #2 above, then a LMB move within the current editor window
 *      selects a region of text.  Later, when the user releases the LMB, that
 *      text is yanked to the unnamed register (the yank code is not handled
 *      in this function).
 *
 * RETURNS
 *   None
 */
static void
mousemove(int *sel_pending,
	  int onmode,
	  POINT * first,
	  POINT * latest,
	  MARK *lmbdn_mark,
	  WINDOW *wp)
{
    POINT current;
    int dummy;

    fhide_cursor();
    GetMousePos(&current);
    TRACE(("GETC:MOUSEMOVE (%d,%d)\n", current.x, current.y));

    /* If on mode line, move window. */
    if (onmode) {
	if (!adjust_window(wp, &current, latest)) {
	    /*
	     * left mouse button still down, but cursor moved off mode
	     * line.  Update latest to keep track of cursor in case
	     * it wanders back on the mode line.
	     */

	    *latest = current;
	}
	return;
    }

    if (*sel_pending) {
	/*
	 * Selection pending.  If the mouse has moved at least one char,
	 * start a selection.
	 */

	if (MouseClickSetPos(latest, &dummy)) {
	    /* ignore mouse jitter */

	    if (latest->x != first->x || latest->y != first->y) {
		*sel_pending = FALSE;
		DOT = *lmbdn_mark;
		(void) sel_begin();
		(void) update(TRUE);
	    } else
		return;
	}
    }

    if (wp != row2window(current.y)) {
	/* mouse moved into a different editor window. */

	return;
    }
    if (!setcursor(current.y, current.x))
	return;
    if (get_keyboard_state() & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
	(void) sel_setshape(RECTANGLE);
    if (!sel_extend(TRUE, TRUE))
	return;
    (void) update(TRUE);
}

#if OPT_SCROLLBARS
static int
check_scrollbar_allocs(void)
{
    int newmax = cur_win->rows / 2;
    int oldmax = cur_win->maxscrollbars;

    TRACE(("check_scrollbar_allocs %d > %d ?\n", oldmax, newmax));
    if (newmax > oldmax) {
	GROW(cur_win->scrollbars, SBDATA, oldmax, newmax);
	cur_win->maxscrollbars = newmax;
	TRACE(("GROW scrollbars=%p, oldmax=%d, newmax=%d\n",
	       cur_win->scrollbars, oldmax, newmax));
    }
    return TRUE;
}

static SBDATA
new_scrollbar(void)
{
    SBDATA result;

    result.shown = FALSE;
    result.r.top = -1;
    result.r.left = -1;
    result.r.bottom = -1;
    result.r.right = -1;

    result.w = CreateWindow(
			       SCRL_CLASS,
			       "scrollbar",
			       WS_CHILD | SBS_VERT | WS_CLIPSIBLINGS,
			       0,	/* x */
			       0,	/* y */
			       SbWidth,		/* width */
			       1,	/* height */
			       cur_win->main_hwnd,
			       (HMENU) 0,
			       vile_hinstance,
			       (LPVOID) 0
	);
    return result;
}

static int
show_scrollbar(int n, int flag)
{
    if (cur_win->scrollbars[n].shown != flag) {
	TRACE(("%s_scrollbar %s\n",
	       flag ? "show" : "hide",
	       which_window(cur_win->scrollbars[n].w)));
	ShowScrollBar(cur_win->scrollbars[n].w, SB_CTL, flag);
	cur_win->scrollbars[n].shown = flag;
	return TRUE;
    }
    return FALSE;
}

static void
set_scrollbar_range(int n, WINDOW *wp)
{
    SCROLLINFO info;
    int lnum, lcnt;

    lnum = line_no(wp->w_bufp, wp->w_dot.l);
    lnum = max(lnum, 1);

    lcnt = vl_line_count(wp->w_bufp);
    lcnt = max(lcnt, 1) + wp->w_ntrows - 1;

    TRACE(("set_scrollbar_range(%d, %s) %d:%d\n",
	   n, wp->w_bufp->b_bname, lnum, lcnt));

    info.cbSize = sizeof(info);
    info.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    info.nPos = lnum - 1;
    info.nMin = 0;
    info.nMax = lcnt - 1;
    info.nPage = wp->w_ntrows;
    SetScrollInfo(cur_win->scrollbars[n].w, SB_CTL, &info, TRUE);
}

/*
 * All we want to do here is to paint the gap between the real resize-grip and
 * the borders of the dummy window we're surrounding it with.  That's because
 * the resize-grip itself isn't resizable.
 */
WINDOW_PROC_RETVAL FAR PASCAL
GripWndProc(
	       HWND hWnd,
	       UINT message,
	       WPARAM wParam,
	       LPARAM lParam)
{
    PAINTSTRUCT ps;
    HBRUSH brush;

    TRACE(("GRIP:%s\n", message2s(message)));

    switch (message) {
    case WM_PAINT:
	BeginPaint(hWnd, &ps);
	TRACE(("...painting (%d,%d) (%d,%d)\n",
	       ps.rcPaint.top,
	       ps.rcPaint.left,
	       ps.rcPaint.bottom,
	       ps.rcPaint.right));
	brush = CreateSolidBrush(GetSysColor(COLOR_SCROLLBAR));
	SelectObject(ps.hdc, brush);
	Rectangle(ps.hdc,
		  ps.rcPaint.left,
		  ps.rcPaint.top,
		  ps.rcPaint.right,
		  ps.rcPaint.bottom);
	DeleteObject(brush);
	EndPaint(hWnd, &ps);
	return (0);
    }
    return (DefWindowProc(hWnd, message, wParam, lParam));
}

static void
update_scrollbar_sizes(void)
{
    RECT crect;
    register WINDOW *wp;
    int i, top, left;
    int newsbcnt;
    int oldsbcnt = cur_win->nscrollbars;

    TRACE(("update_scrollbar_sizes\n"));

    i = 0;
    for_each_visible_window(wp)
	i++;
    newsbcnt = i;

    for (i = cur_win->nscrollbars + 1; i <= newsbcnt; i++) {
	if (cur_win->scrollbars[i].w == NULL) {
	    cur_win->scrollbars[i] = new_scrollbar();
	    TRACE(("... created sb%d=%#x\n", i, cur_win->scrollbars[i].w));
	}
    }
    cur_win->nscrollbars = newsbcnt;

    /* Set sizes and positions on scrollbars and sliders */
    i = 0;
    GetClientRect(cur_win->main_hwnd, &crect);
    top = crect.top;
    left = crect.right - SbWidth;
    for_each_visible_window(wp) {
	int high = RowToPixel(wp->w_ntrows + 1);
	int wide = SbWidth;
	if (show_scrollbar(i, TRUE)
	    || cur_win->scrollbars[i].r.top != top
	    || cur_win->scrollbars[i].r.left != left
	    || cur_win->scrollbars[i].r.bottom != high
	    || cur_win->scrollbars[i].r.right != wide) {
	    MoveWindow(cur_win->scrollbars[i].w,
		       cur_win->scrollbars[i].r.left = left,
		       cur_win->scrollbars[i].r.top = top,
		       cur_win->scrollbars[i].r.right = wide,
		       cur_win->scrollbars[i].r.bottom = high,
		       TRUE);
	    TRACE(("... adjusted %s to (%d,%d) (%d,%d)\n",
		   which_window(cur_win->scrollbars[i].w),
		   left,
		   top,
		   SbWidth,
		   high));
	}
	if (cur_win->nscrollbars == i + 1)
	    set_scrollbar_range(i, wp);
	i++;
	top += high;
    }

    while (i < oldsbcnt) {
	(void) show_scrollbar(i, FALSE);
	i++;
    }

    if (cur_win->size_box.w == 0) {
	cur_win->size_box.w = CreateWindow(
					      GRIP_CLASS,
					      "sizebox",
					      WS_CHILD
					      | WS_VISIBLE
					      | WS_CLIPSIBLINGS,
					      cur_win->size_box.r.left = left,
					      cur_win->size_box.r.top = top,
					      cur_win->size_box.r.right =
					      SbWidth + 1,
					      cur_win->size_box.r.bottom = nLineHeight,
					      cur_win->main_hwnd,
					      (HMENU) 0,
					      vile_hinstance,
					      (LPVOID) 0
	    );
	cur_win->size_grip.w = CreateWindow(
					       "SCROLLBAR",
					       "sizebox",
					       WS_CHILD
					       | WS_VISIBLE
					       | SB_CTL
					       | SBS_SIZEGRIP
					       | WS_CLIPSIBLINGS
					       | SBS_SIZEBOXBOTTOMRIGHTALIGN,
					       cur_win->size_box.r.left = 0,
					       cur_win->size_box.r.top = 0,
					       cur_win->size_box.r.right = SbWidth,
					       cur_win->size_box.r.bottom = nLineHeight,
					       cur_win->size_box.w,
					       (HMENU) 0,
					       vile_hinstance,
					       (LPVOID) 0
	    );
	TRACE(("... made SIZEGRIP %x at %d,%d\n", cur_win->size_box.w, left, top));
    } else {
	int ok;

	if (cur_win->size_box.r.left != left
	    || cur_win->size_box.r.top != top
	    || cur_win->size_box.r.right != SbWidth
	    || cur_win->size_box.r.bottom != nLineHeight) {
	    ok = MoveWindow(cur_win->size_box.w,
			    cur_win->size_box.r.left = left,
			    cur_win->size_box.r.top = top,
			    cur_win->size_box.r.right = SbWidth,
			    cur_win->size_box.r.bottom = nLineHeight,
			    TRUE);
	    TRACE(("... move SIZE_BOX %d:%x to %d,%d\n",
		   ok, cur_win->size_box.w, left, top));
	}

	left = 0;
	top = 0;
	if (cur_win->size_grip.r.left != left
	    || cur_win->size_grip.r.top != top
	    || cur_win->size_grip.r.right != SbWidth
	    || cur_win->size_grip.r.bottom != nLineHeight) {
	    ok = MoveWindow(cur_win->size_grip.w,
			    cur_win->size_grip.r.left = left,
			    cur_win->size_grip.r.top = top,
			    cur_win->size_grip.r.right = SbWidth,
			    cur_win->size_grip.r.bottom = nLineHeight,
			    TRUE);
	    TRACE(("... move SIZEGRIP %d:%x to %d,%d\n",
		   ok, cur_win->size_grip.w, left, top));
	}
    }
}

void
gui_update_scrollbar(WINDOW *uwp)
{
    WINDOW *wp;
    int i;

    TRACE(("gui_update_scrollbar uwp=%p %s\n", uwp, uwp->w_bufp->b_bname));
    if (dont_update_sb)
	return;

    i = 0;
    for_each_visible_window(wp) {
	TRACE(("wp=%p name='%s'\n", wp, wp->w_bufp->b_bname));
	if (wp == uwp)
	    break;
	i++;
    }

    TRACE(("i=%d, nscrollbars=%d\n", i, cur_win->nscrollbars));
    if (i >= cur_win->nscrollbars || (wp->w_flag & WFSBAR)) {
	/*
	 * update_scrollbar_sizes will recursively invoke gui_update_scrollbar,
	 * but with WFSBAR disabled.
	 */
	update_scrollbar_sizes();
	return;
    }

    set_scrollbar_range(i, wp);
}

static int
find_scrollbar(HWND hWnd)
{
    WINDOW *wp;
    int i = 0;

    TRACE((T_CALLED "find_scrollbar(hWnd=%p)\n", hWnd));

    for_each_visible_window(wp) {
	if (cur_win->scrollbars[i].w == hWnd
	    || cur_win->main_hwnd == hWnd) {
	    set_curwp(wp);
	    if (wp->w_bufp != curbp) {
		swbuffer(wp->w_bufp);
	    }
	    returnCode(i);
	}
	i++;
    }
    returnCode(-1);
}

/*
 * M$ is only capable of delivering an unsigned 16-bit number for nPos.  Infer
 * the "right" value by minimizing the distance we travel in a given jump.
 */
static int
fix_scrollbar_tracking(int nPos)
{
    int this_line = line_no(curwp->w_bufp, curwp->w_line.l);
    int last_line = line_no(curwp->w_bufp, lback(buf_head(curwp->w_bufp)));
    int diff_line = 0x10000;
    int test_line;

    for (test_line = nPos; test_line < last_line; test_line += 0x10000) {
	int check = ABS(this_line - test_line);
	if (check < diff_line) {
	    diff_line = check;
	    nPos = test_line;
	}
    }
    return nPos;
}

static void
handle_scrollbar(HWND hWnd, int msg, int nPos)
{
    int this_line;
    int snum = find_scrollbar(hWnd);
    int upd = TRUE;		/* do an update() if T */

    TRACE(("handle_scrollbar msg=%d, nPos=%d\n", msg, nPos));

    if (snum < 0) {
	TRACE(("...could not find window for %s\n", which_window(hWnd)));
	return;
    }

    fhide_cursor();
    switch (msg) {
    case SB_BOTTOM:
	TRACE(("-> SB_BOTTOM\n"));
	gotoline(FALSE, 1);
	break;
    case SB_LINEDOWN:
	TRACE(("-> SB_LINEDOWN\n"));
	mvdnwind(FALSE, 1);
	break;
    case SB_LINEUP:
	TRACE(("-> SB_LINEUP\n"));
	mvupwind(FALSE, 1);
	break;
    case SB_PAGEDOWN:
	TRACE(("-> SB_PAGEDOWN\n"));
	forwpage(FALSE, 1);
	break;
    case SB_PAGEUP:
	TRACE(("-> SB_PAGEUP\n"));
	backpage(FALSE, 1);
	break;
    case SB_THUMBPOSITION:
	TRACE(("-> SB_THUMBPOSITION: %d\n", nPos));
	gotoline(TRUE, fix_scrollbar_tracking(nPos) + 1);
	break;
    case SB_THUMBTRACK:
	TRACE(("-> SB_THUMBTRACK: %d\n", nPos));
	/*
	 * M$ is only capable of delivering an unsigned 16-bit number for nPos.
	 * Infer the "right" value by minimizing the distance we travel in a
	 * given jump.
	 */
	this_line = line_no(curwp->w_bufp, curwp->w_line.l);
	mvupwind(TRUE, this_line - fix_scrollbar_tracking(nPos));
	break;
    case SB_TOP:
	TRACE(("-> SB_TOP\n"));
	gotoline(TRUE, 1);
	break;
    case SB_ENDSCROLL:
	TRACE(("-> SB_ENDSCROLL\n"));
	/* Fall through */
    default:
	upd = FALSE;
	break;
    }
    if (upd) {
	(void) update(TRUE);
	set_scrollbar_range(snum, curwp);
    }
    fshow_cursor();
}
#endif /* OPT_SCROLLBARS */

static int
ntgetch(void)
{
    static DWORD lastclick = 0;
    static int clicks = 0, onmode;
    // Save the timer ID so we can kill it.

    DWORD thisclick;
    int buttondown = FALSE;
    int sel_pending = FALSE;	/* Selection pending */
    MARK lmbdn_mark;		/* left mouse button down here */
    MARK last_dot = nullmark;	/* remember where dot was before selection */
    WINDOW *last_win = 0;	/* remember which window dot was in */
    int selecting = FALSE;	/* toggle between cut and paste */
    int result = -1;
    KEY_EVENT_RECORD ker;
    MSG msg;
    POINT first;
    POINT latest;
    UINT clicktime;
    WINDOW *that_wp = 0;
#ifdef VAL_AUTOCOLOR
    int milli_ac, orig_milli_ac;
#endif

    if (saveCount > 0) {
	saveCount--;
	return savedChar;
    }
    clicktime = GetDoubleClickTime();
#ifdef VAL_AUTOCOLOR
    orig_milli_ac = global_b_val(VAL_AUTOCOLOR);
#endif
    vile_in_getfkey = 1;
    if (GetFocus() == cur_win->main_hwnd) {
	fshow_cursor();
    } else {
	fhide_cursor();
    }
    while (result < 0) {
#ifdef VAL_AUTOCOLOR
	milli_ac = orig_milli_ac;
	while (milli_ac > 0) {
	    if (PeekMessage(&msg, (HWND) 0, 0, 0, PM_NOREMOVE)) {
		if (msg.message != 0x118)
		    break;	/* A meaningful event--process it. */
		else {
		    /*
		     * This is the undocumented (as near as I can tell)
		     * 0x118 event that seems to occur whenever the caret
		     * blinks.  This is a background "noise" event that can
		     * be ignored once dispatched.
		     */

		    if (GetMessage(&msg, (HWND) 0, 0, 0) != TRUE) {
			PostQuitMessage(1);
			quit(TRUE, 1);
		    } else
			DispatchMessage(&msg);
		}
	    }
	    Sleep(20);		/* sleep a bit, but be responsive to all events */
	    milli_ac -= 20;
	}
	if (orig_milli_ac && milli_ac <= 0) {
	    ac_active = TRUE;
	    autocolor();
	    ac_active = FALSE;
	}
#endif
	if (GetMessage(&msg, (HWND) 0, 0, 0) != TRUE) {
	    PostQuitMessage(1);
	    TRACE(("GETC:no message\n"));
	    quit(TRUE, 1);
	}

	if (TranslateAccelerator(cur_win->main_hwnd, hAccTable, &msg)) {
	    TRACE(("GETC:no accelerator\n"));
	    continue;
	}

	TranslateMessage(&msg);

	switch (msg.message) {
	case WM_DESTROY:
	    TRACE(("GETC:DESTROY\n"));
	    PostQuitMessage(0);
	    continue;

	case WM_CHAR:
	    TRACE(("GETC:CHAR:%#x\n", msg.wParam));
	    result = (int) msg.wParam;
	    /*
	     * Check for modifieers on control keys such as tab.
	     */
	    if ((result < 256) && isCntrl(result)) {
		DWORD state = get_keyboard_state();
		state &= ~(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
		if (isModified(state))
		    result = modified_key(result, state);
	    }
	    if (result == ESC) {
		sel_release();
		(void) update(TRUE);
	    }
	    break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	    ker.uChar.AsciiChar = 0;
	    ker.wVirtualKeyCode = (SHORT) msg.wParam;
	    ker.dwControlKeyState = get_keyboard_state();
	    result = decode_key_event(&ker);
	    TRACE(("GETC:%sKEYDOWN:%#x ->%#x\n",
		   (msg.message == WM_SYSKEYDOWN) ? "SYS" : "",
		   msg.wParam, result));
	    if (result == NOKYMAP) {
		DispatchMessage(&msg);
		result = -1;
#ifdef VILE_OLE
	    } else if (result == KYREDIR) {
		result = -1;	/* keystroke sent elsewhere */
#endif
	    } else if (result == (int) msg.wParam) {
		result = -1;	/* we'll get a WM_CHAR next */
	    }
	    break;

	case WM_TIMER:
	    TRACE(("Timer fired\n"));
	    {
		if (msg.wParam == nIDTimer) {
		    /*
		     * perform auto-scroll if the mouse cursor
		     * is above/below the current window.
		     */

		    AutoScroll(that_wp);
		} else {
		    DispatchMessage(&msg);
		}
	    }
	    break;
	case WM_LBUTTONDOWN:
	    TRACE(("GETC:LBUTTONDOWN %s\n", which_window(msg.hwnd)));
	    if (msg.hwnd == cur_win->text_hwnd) {
		/* Clear current selection, a la notepad. */
		sel_release();
		selecting = FALSE;
		last_dot = DOT;
		last_win = curwp;
		/* Allow click to change window focus. */
		if (MouseClickSetPos(&first, &onmode)) {
		    fhide_cursor();
		    lmbdn_mark = DOT;
		    latest = first;
		    that_wp = row2window(first.y);
		    (void) update(TRUE);	/* for wdw change */

		    /*
		     * If clicking around on bottom mode line or in the
		     * message line (aka mini-buffer), we're done.  In the
		     * next computation, "term.rows - 1" is last usable
		     * editor row, and additional "- 1" accounts for
		     * bottom mode line.
		     */
		    if (first.y >= term.rows - 1 - 1) {
			fshow_cursor();
			sel_pending = buttondown = FALSE;
			break;
		    }

		    sel_pending = buttondown = TRUE;
		    SetCursor((onmode) ? wdwsize_cursor : selection_cursor);

		    if (onmode) {
			/* no mouse capture necessary if on a mode line */

			break;
		    }

		    /* we _will_ own the mouse, if win32 API cooperates */
		    SetCapture(cur_win->text_hwnd);
		    mouse_captured = TRUE;

		    /* Set a 25 msec timer for handling auto-scroll. */
		    nIDTimer = SetTimer(cur_win->text_hwnd, 1, 25, 0);
		    if (nIDTimer == 0) {
			/* timer resources exhausted */

			disp_win32_error(W32_SYS_ERROR, cur_win->text_hwnd);
			ReleaseCapture();
			mouse_captured = FALSE;
		    } else {
			AutoScroll(that_wp);
		    }
		}
	    } else {
		DispatchMessage(&msg);
	    }
	    break;

	case WM_LBUTTONUP:
	    TRACE(("GETC:LBUTTONUP %s\n", which_window(msg.hwnd)));

	    /* If we captured the mouse, then we will release it. */
	    if (mouse_captured) {
		mouse_captured = FALSE;
		ReleaseCapture();
		KillTimer(cur_win->text_hwnd, nIDTimer);
		nIDTimer = 0;
	    }
	    if (msg.hwnd == cur_win->text_hwnd) {
		fhide_cursor();
		sel_pending = FALSE;
		thisclick = GetTickCount();
		TRACE(("CLICK %d/%d\n", lastclick, thisclick));
		if (thisclick - lastclick < clicktime) {
		    clicks++;
		    TRACE(("MOUSE CLICKS %d\n", clicks));
		} else {
		    clicks = 0;
		}
		lastclick = thisclick;

		switch (clicks) {
		case 1:
		    on_double_click();
		    selecting = TRUE;
		    break;
		case 2:
		    on_triple_click();
		    selecting = TRUE;
		    break;
		}

		if (buttondown) {
		    int dummy;

		    /*
		     * Update editor's current cursor position, if that
		     * position is still within cur_win->text_hwnd .
		     */
		    (void) MouseClickSetPos(&latest, &dummy);

		    if (!onmode) {
			/*
			 * The LMB is down and non-modeline mouse movement
			 * just terminated within a vile window.  Note
			 * however that, due to autoscroll, the cursor
			 * might not now be positioned within
			 * cur_win->main_hwnd--but that doesn't matter.
			 * If the user selected at least one char of
			 * text, yank it to the unnamed register.
			 */

			sel_yank(0);
		    }
		    if (selecting) {
			if (win2index(last_win) >= 0) {
			    set_curwp(last_win);
			    restore_dot(last_dot);
			}
			last_win = 0;
			last_dot = nullmark;
		    }
		}
		onmode = buttondown = FALSE;
		(void) update(TRUE);
		fshow_cursor();
	    } else {
		DispatchMessage(&msg);
	    }
	    break;

	    /*
	     * define _WIN32_WINNT=0x400 (or higher) to include WM_MOUSEWHEEL
	     * code below.  Note that WM_MOUSEWHEEL is really only required
	     * to support a mouse driver that doesn't emit WM_VMSCROLL
	     * messages when the wheel mouse rotates.  Examples:
	     *
	     *    MS Intellimouse driver     -> emits WM_VMSCROLL
	     *    Logitech mousewheel driver -> emits WM_VMSCROLL
	     *    MS PS/2 compatible driver  -> emits WM_MOUSEWHEEL
	     *
	     * The latter driver is often installed when PNP can't
	     * distinguish the native HW.
	     */
#ifdef WM_MOUSEWHEEL
#if OPT_SCROLLBARS
	case WM_MOUSEWHEEL:
	    {
		int c;

		fhide_cursor();
		if ((short) HIWORD(msg.wParam) > 0) {
		    c = (HIWORD(msg.wParam) / WHEEL_DELTA);
		    mvupwind(TRUE, c * 3);
		} else {
		    c = (-((short) HIWORD(msg.wParam)) / WHEEL_DELTA);
		    mvdnwind(TRUE, c * 3);
		}
		update(TRUE);
		fshow_cursor();
	    }
	    break;
#endif
#endif

	case WM_MBUTTONDOWN:
	    TRACE(("GETC:MBUTTONDOWN %s\n", which_window(msg.hwnd)));
	    if (msg.hwnd == cur_win->text_hwnd) {
		if (MouseClickSetPos(&latest, &onmode)
		    && !onmode) {
		    sel_yank(0);
		    sel_release();
		    paste_selection();
		}
		(void) update(TRUE);
	    } else {
		DispatchMessage(&msg);
	    }
	    break;

	case WM_RBUTTONDOWN:
	    TRACE(("GETC:RBUTTONDOWN %s\n", which_window(msg.hwnd)));
	    if (msg.hwnd == cur_win->text_hwnd) {
		if (enable_popup) {
		    invoke_popup_menu(msg);
		} else {
		    if (selecting) {
			sel_yank(0);
			cbrdcpy_unnamed(FALSE, 1);
			selecting = FALSE;
			sel_release();
		    } else {
			mayneedundo();
			cbrdpaste(FALSE, 1);
		    }
		    (void) update(TRUE);
		}
	    } else {
		DispatchMessage(&msg);
	    }
	    break;

	case WM_COMMAND:
	    TRACE(("GETC:WM_COMMAND, popup:%d, wParam:%#x\n", enable_popup, msg.wParam));
	    if (enable_popup) {
		handle_builtin_menu(msg.wParam);
	    } else {
		DispatchMessage(&msg);
	    }
	    break;

	case WM_MOUSEMOVE:
	    {
		POINT pt;
		WINDOW *wp;

		GetMousePos(&pt);
		if ((wp = row2window(pt.y)) != 0) {
		    /*
		     * In the next computation, "term.rows - 1" is last
		     * usable editor row, and additional "- 1" accounts for
		     * bottom mode line.
		     */

		    if (pt.y == mode_row(wp) &&
			(pt.y < term.rows - 1 - 1) &&
			(pt.x < term.cols)) {
			/* 
			 * On movable mode line and cursor not in scrollbar
			 * rectangle, so show appropriate cursor.
			 */

			SetCursor(wdwsize_cursor);
		    }
		}
		if (buttondown) {
		    mousemove(&sel_pending,
			      onmode,
			      &first,
			      &latest,
			      &lmbdn_mark,
			      that_wp);
		    selecting = !sel_pending;
		} else {
		    DispatchMessage(&msg);
		}
	    }
	    break;

	default:
	    TRACE(("GETC:default(%s)\n", message2s(msg.message)));
	    /* FALLTHRU */
	case WM_KEYUP:		/* FALLTHRU */
	case WM_NCHITTEST:	/* FALLTHRU */
	case WM_NCMOUSEMOVE:	/* FALLTHRU */
	case WM_NCLBUTTONDOWN:	/* FALLTHRU */
	case WM_PAINT:		/* FALLTHRU */
	case WM_NCACTIVATE:	/* FALLTHRU */
	case WM_SETCURSOR:	/* FALLTHRU */
	case 0x118:
	    DispatchMessage(&msg);
	    break;
	}
    }
    fhide_cursor();
    vile_in_getfkey = 0;

    TRACE(("...ntgetch %#x\n", result));
    return result;
}

static int
nttypahead(void)
{
    MSG msg;

#ifdef VAL_AUTOCOLOR
    if (ac_active) {
	/*
	 * Came here during an autocolor operation.  Do nothing, in an
	 * attempt to avoid a keyboard lockup (editor loop) that occurs on
	 * rare occasions (not reproducible).
	 */

	return (0);
    }
#endif
    if (saveCount > 0)
	return (1);
    if (PeekMessage(&msg, (HWND) 0, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
	return (1);
    if (PeekMessage(&msg, (HWND) 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_NOREMOVE))
	return (1);
    return (PeekMessage(&msg, (HWND) 0, WM_CHAR, WM_CHAR, PM_NOREMOVE));
}

static void
repaint_window(HWND hWnd)
{
    PAINTSTRUCT ps;
    HBRUSH brush;
    int x0, y0, x1, y1;
    int row, col;

    BeginPaint(hWnd, &ps);
    TRACE(("repaint_window (erase:%d) %s\n", ps.fErase, which_window(hWnd)));
    nt_set_colors(ps.hdc, cur_atr);
    brush = Background(ps.hdc);

    TRACE(("...painting (%d,%d) (%d,%d)\n",
	   ps.rcPaint.top,
	   ps.rcPaint.left,
	   ps.rcPaint.bottom,
	   ps.rcPaint.right));

    y0 = (ps.rcPaint.top) / nLineHeight;
    x0 = (ps.rcPaint.left) / nCharWidth;
    y1 = (ps.rcPaint.bottom + nLineHeight) / nLineHeight;
    x1 = (ps.rcPaint.right + nCharWidth) / nCharWidth;

    if (y0 < 0)
	y0 = 0;
    if (x0 < 0)
	x0 = 0;
    if (y1 > term.maxrows)
	y1 = term.maxrows;
    if (x1 > term.maxcols)
	x1 = term.maxcols;

    TRACE(("...erase %d\n", ps.fErase));
    TRACE(("...cells (%d,%d) - (%d,%d)\n", y0, x0, y1, x1));
    TRACE(("...top:    %d\n", RowToPixel(y0) - ps.rcPaint.top));
    TRACE(("...left:   %d\n", ColToPixel(x0) - ps.rcPaint.left));
    TRACE(("...bottom: %d\n", RowToPixel(y1) - ps.rcPaint.bottom));
    TRACE(("...right:  %d\n", ColToPixel(x1) - ps.rcPaint.right));

    for (row = y0; row < y1; row++) {
	if (pscreen != 0
	    && pscreen[row]->v_text != 0
	    && pscreen[row]->v_attrs != 0) {
	    int old_col = x0;
	    VIDEO_ATTR old_atr = CELL_ATTR(row, old_col);
	    VIDEO_ATTR new_atr;

	    for (col = x0 + 1; col < x1; col++) {
		new_atr = CELL_ATTR(row, col);
		if (new_atr != old_atr) {
		    nt_set_colors(ps.hdc, old_atr);
		    TRACE2(("ExtTextOut [%3d,%3d]%.*s\n", row, old_col, col -
			    old_col, &CELL_TEXT(row, old_col)));
		    ExtTextOut(ps.hdc,
			       ColToPixel(old_col),
			       RowToPixel(row),
			       0,
			       (RECT *) 0,
			       &CELL_TEXT(row, old_col),
			       col - old_col,
			       intercharacter(col));
		    old_atr = new_atr;
		    old_col = col;
		}
	    }
	    if (old_col < x1) {
		nt_set_colors(ps.hdc, old_atr);
		TRACE2(("ExtTextOut [%3d,%3d]%.*s\n", row, old_col, x1 -
			old_col, &CELL_TEXT(row, old_col)));
		ExtTextOut(ps.hdc,
			   ColToPixel(old_col),
			   RowToPixel(row),
			   0,
			   (RECT *) 0,
			   &CELL_TEXT(row, old_col),
			   x1 - old_col,
			   intercharacter(x1));
	    }
	}
    }
    DeleteObject(brush);

    TRACE(("...repaint_window\n"));
    EndPaint(hWnd, &ps);
}

static int
we_are_at_home(void)
{
    return TRUE;
}

static void
receive_dropped_files(HDROP hDrop)
{
    char name[NFILEN];
    UINT inx = 0xFFFFFFFF;
    UINT limit = DragQueryFile(hDrop, inx, name, sizeof(name));
    BUFFER *bp = 0;

    TRACE((T_CALLED "receive_dropped_files(hDrop=%p) %d dropped files\n",
	   hDrop, limit));

    while (++inx < limit) {
	DragQueryFile(hDrop, inx, name, sizeof(name));
	TRACE(("...'%s'\n", name));
	if ((bp = getfile2bp(name, FALSE, FALSE)) != 0)
	    bp->b_flag |= BFARGS;	/* treat this as an argument */
    }
    if (bp != 0) {
	if (swbuffer(bp)) {	/* editor switches to 1st buffer */
	    set_directory_from_file(bp);
	}
	update(TRUE);
    }
    DragFinish(hDrop);
    returnVoid();
}

static void
HandleClose(HWND hWnd)
{
    quit(FALSE, 1);
}

WINDOW_PROC_RETVAL FAR PASCAL
TextWndProc(
	       HWND hWnd,
	       UINT message,
	       WPARAM wParam,
	       LPARAM lParam)
{
    RECT rect;
    HDC hDC;

    TRACE(("TEXT:%s, %s\n", message2s(message), which_window(hWnd)));

    switch (message) {
    case WM_PAINT:
	if ((hWnd == cur_win->main_hwnd
	     || hWnd == cur_win->text_hwnd)
	    && GetUpdateRect(hWnd, (LPRECT) 0, FALSE)) {
	    repaint_window(hWnd);
	} else {
	    /* scrollbars, etc. */
	    TRACE(("...repaint %s\n", which_window(hWnd)));
	    return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	break;
    case WM_WVILE_FLASH_START:
	GetClientRect(cur_win->text_hwnd, &rect);
	hDC = GetDC(cur_win->text_hwnd);
	InvertRect(hDC, &rect);
	ReleaseDC(cur_win->text_hwnd, hDC);
	break;
    case WM_WVILE_FLASH_STOP:
	Sleep(200);
	GetClientRect(cur_win->text_hwnd, &rect);
	hDC = GetDC(cur_win->text_hwnd);
	InvertRect(hDC, &rect);
	ReleaseDC(cur_win->text_hwnd, hDC);
	break;
    default:
	return (DefWindowProc(hWnd, message, wParam, lParam));

	IGN_PROC("TEXT:", WM_ERASEBKGND);
    }
    return (0);
}

WINDOW_PROC_RETVAL FAR PASCAL
MainWndProc(
	       HWND hWnd,
	       UINT message,
	       WPARAM wParam,
	       LPARAM lParam)
{
    static int resize_pending;	/* a resize, not a move */
    static HWND resize_hwnd;
    static int resize_wdw_up;

    TRACE(("MAIN:%s, %s\n", message2s(message), which_window(hWnd)));

    switch (message) {
	HANDLE_MSG(hWnd, WM_CLOSE, HandleClose);
    case WM_COMMAND:
#if FIXME
	switch (wParam) {
	case IDC_button:
	    wParam = IDC_button_x;
	    PostMessage(hWnd, message, wParam, lParam);
	    break;
	}
#endif
	break;

    case WM_SETFOCUS:
	fshow_cursor();
	break;
    case WM_WVILE_CURSOR_ON:
	if (GetFocus() == hWnd)
	    fshow_cursor();
	break;

    case WM_KILLFOCUS:
    case WM_WVILE_CURSOR_OFF:
	fhide_cursor();
	break;

    case WM_DESTROY:
	PostQuitMessage(0);
	break;

    case WM_WINDOWPOSCHANGED:
	if (!IsIconic(hWnd))
	    ResizeClient();	/* ignore if window minimized */
	return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_WINDOWPOSCHANGING:
#if FIXME_POSCHANGING
	if (wheadp != 0)
	    return AdjustPosChanging(hWnd, (LPWINDOWPOS) lParam);
#endif
	return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_SIZING:
	if (initialized) {
	    char buf[32];
	    static int frame_w, frame_h;

	    if (is_winnt()) {
		/*
		 * Win9x/ME GDI doesn't support supplementary GDI activity
		 * while a window resize is in progress.
		 */

		if (!resize_hwnd)
		    resize_hwnd = sizing_window();
		if (resize_hwnd) {
		    if (!resize_wdw_up) {
			RECT crect, wrect;

			resize_wdw_up = TRUE;
			ShowWindow(resize_hwnd, SW_SHOWNORMAL);
			sprintf(buf, "%d cols X %d rows", term.cols, term.rows);

			/* compute real estate consumed by main window frame */
			GetClientRect(cur_win->main_hwnd, &crect);
			GetWindowRect(cur_win->main_hwnd, &wrect);
			frame_w = (wrect.right - wrect.left) - crect.right;
			frame_h = (wrect.bottom - wrect.top) - crect.bottom;
		    } else {
			RECT newrect, arect;
			int h, w;

			/*
			 * The rectangle passed in with this message includes
			 * the outer frame, which is worthless for editing
			 * text.
			 */
			newrect = *((RECT *) lParam);
			newrect.right -= frame_w;
			newrect.bottom -= frame_h;
			h = RectToRows(newrect);
			w = RectToCols(newrect);
			sprintf(buf, "%d cols X %d rows", w, h);

			/*
			 * This code is more complicated than it should be,
			 * but it works (which is another way of saying:  I
			 * have no idea what's happening).  Just too many hoops
			 * to jump through to center one window over another.
			 */
			GetClientRect(GetForegroundWindow(), &arect);
			MoveWindow(resize_hwnd,
				   ((arect.right - arect.left) / 2 -
				    RSZ_WDW_WDTH / 2),
				   ((arect.bottom - arect.top) / 2 -
				    RSZ_WDW_HGHT / 2),
				   RSZ_WDW_WDTH,
				   RSZ_WDW_HGHT,
				   TRUE);
		    }
		    SetWindowText(resize_hwnd, buf);
		}
	    }
	}
	return AdjustResizing(hWnd, wParam, (LPRECT) lParam);

    case WM_EXITSIZEMOVE:
	if (resize_hwnd && resize_wdw_up) {
	    resize_wdw_up = FALSE;
	    SendMessage(resize_hwnd, WM_CLOSE, 0, 0);
	    resize_hwnd = NULL;
	}
	ResizeClient();
	if (resize_pending) {
	    if (initialized) {
		mlwrite("columns: %d, rows: %d", term.cols, term.rows);
		update(TRUE);	/* Force cursor out of message line */
	    }
	    resize_pending = FALSE;
	}
	return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_DROPFILES:
	receive_dropped_files((HDROP) wParam);
	return 0;

    case WM_INITMENUPOPUP:
	TRACE(("MAIN:WM_INITMENUPOPUP %s at %d,%d\n",
	       syscommand2s(LOWORD(wParam)),
	       HIWORD(lParam),
	       LOWORD(lParam)));
	build_recent_file_and_folder_menus();
	return (DefWindowProc(hWnd, message, wParam, lParam));

    case WM_SYSCOMMAND:
	TRACE(("MAIN:WM_SYSCOMMAND %s at %d,%d\n",
	       syscommand2s(LOWORD(wParam)),
	       HIWORD(lParam),
	       LOWORD(lParam)));
	handle_builtin_menu(wParam);
	return (DefWindowProc(hWnd, message, wParam, lParam));

#if OPT_SCROLLBARS
    case WM_VSCROLL:
	handle_scrollbar((HWND) lParam, LOWORD(wParam), HIWORD(wParam));
	return (0);
#endif

    case WM_SIZE:
	if (initialized)
	    resize_pending = TRUE;
	/* FALL THROUGH */
    default:
	return (TextWndProc(hWnd, message, wParam, lParam));

	IGN_PROC("MAIN:", WM_ERASEBKGND);
    }
    return (1);
}

static BOOL
InitInstance(HINSTANCE hInstance)
{
    WNDCLASS wc;

    hglass_cursor = LoadCursor((HINSTANCE) 0, IDC_WAIT);
    arrow_cursor = LoadCursor((HINSTANCE) 0, IDC_ARROW);
    selection_cursor = LoadCursor((HINSTANCE) 0, IDC_IBEAM);
    wdwsize_cursor = LoadCursor((HINSTANCE) 0, IDC_SIZENS);

    default_bcolor = GetSysColor(COLOR_WINDOWTEXT + 1);
    default_fcolor = GetSysColor(COLOR_WINDOW + 1);

    ZeroMemory(&wc, sizeof(&wc));
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "VilewIcon");
    wc.hCursor = arrow_cursor;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = "VileMenu";
    wc.lpszClassName = MAIN_CLASS;

    if (!RegisterClass(&wc))
	return FALSE;

    TRACE(("Registered(%s)\n", MAIN_CLASS));

    vile_hinstance = hInstance;
    hAccTable = LoadAccelerators(vile_hinstance, "VileAcc");

    cur_win->main_hwnd = CreateWindow(
					 MAIN_CLASS,
					 MY_APPLE,
					 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 1,
					 1,
					 (HWND) 0,
					 (HMENU) 0,
					 hInstance,
					 (LPVOID) 0
	);
    TRACE(("CreateWindow(main) -> %#lx\n", cur_win->main_hwnd));
    if (!cur_win->main_hwnd)
	return (FALSE);

    ZeroMemory(&wc, sizeof(&wc));
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = (WNDPROC) TextWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = arrow_cursor;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = TEXT_CLASS;

    if (!RegisterClass(&wc))
	return FALSE;

    TRACE(("Registered(%s)\n", TEXT_CLASS));

    cur_win->text_hwnd = CreateWindow(
					 TEXT_CLASS,
					 "text",
					 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
					 CW_USEDEFAULT,
					 CW_USEDEFAULT,
					 1,
					 1,
					 cur_win->main_hwnd,
					 (HMENU) 0,
					 hInstance,
					 (LPVOID) 0
	);
    TRACE(("CreateWindow(text) -> %#lx\n", cur_win->text_hwnd));
    if (!cur_win->text_hwnd)
	return (FALSE);

    /*
     * Register the GRIP_CLASS now also, otherwise it won't succeed when
     * we create the first scrollbars, until we resize the window.
     */
#if OPT_SCROLLBARS
    ZeroMemory(&wc, sizeof(&wc));
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = (WNDPROC) GripWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = vile_hinstance;
    wc.hCursor = arrow_cursor;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = GRIP_CLASS;

    if (!RegisterClass(&wc)) {
	TRACE(("could not register class %s:%#x\n", GRIP_CLASS, GetLastError()));
	return (FALSE);
    }
    TRACE(("Registered(%s)\n", GRIP_CLASS));
#endif

    cur_win->nscrollbars = -1;

    /* Insert Winvile's menu items in the system menu. */
    vile_menu = GetSystemMenu(cur_win->main_hwnd, FALSE);
    AppendMenu(vile_menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(vile_menu, MF_STRING, IDM_OPEN, "&Open...");
    AppendMenu(vile_menu, MF_STRING, IDM_SAVE_AS, "&Save As...");
    AppendMenu(vile_menu, MF_STRING, IDM_CHDIR, "C&D...");
    AppendMenu(vile_menu, MF_STRING, IDM_FAVORITES, "Fa&vorites...");
    AppendMenu(vile_menu, MF_STRING, IDM_FONT, "&Font...");
    AppendMenu(vile_menu, MF_STRING, IDM_ABOUT, "&About...");
    AppendMenu(vile_menu, MF_SEPARATOR, 0, NULL);
    AppendMenu(vile_menu, MF_STRING, IDM_PAGE_SETUP, "Page Set&up...");
    AppendMenu(vile_menu, MF_STRING, IDM_PRINT, "&Print...");
    AppendMenu(vile_menu, MF_SEPARATOR, 0, NULL);

    /*
     * NB -- don't change the order of the next 3 menu items!
     *
     * The popup menus associated with the next two menu items are created
     * as necessary.
     */
    AppendMenu(vile_menu, MF_POPUP, 0, "Recent Fi&les");
    AppendMenu(vile_menu, MF_POPUP, 0, "Recent Fol&ders");
    AppendMenu(vile_menu, MF_SEPARATOR, IDM_SEP_AFTER_RCNT_FLDRS, NULL);
    /*
     * NB -- don't change the order of the previous 3 menu items!
     */

    AppendMenu(vile_menu, MF_STRING | MF_CHECKED, IDM_MENU, "&Menu");

#if OPT_SCROLLBARS
    if (check_scrollbar_allocs() != TRUE)
	return (FALSE);
#endif

    get_font(&vile_logfont);
    use_font(GetMyFont(0));

    DragAcceptFiles(cur_win->main_hwnd, TRUE);

    return (TRUE);
}

int WINAPI
WinMain(
	   HINSTANCE hInstance,
	   HINSTANCE hPrevInstance,
	   LPSTR lpCmdLine,
	   int nCmdShow)
{
    int argc;
    int n;
    size_t maxargs = strlen(lpCmdLine) + 1;
    char **argv = typecallocn(char *, maxargs + 10);
    char *ptr, *fontstr;
#ifdef VILE_OLE
    int oa_invoke, oa_reg;

    memset(&oa_opts, 0, sizeof(oa_opts));
    oa_invoke = oa_reg = FALSE;
#endif

    TRACE(("Starting ntvile, CmdLine:%s\n", lpCmdLine));

    argv[argc = 0] = "VILE";

    if (ffaccess(lpCmdLine, FL_READABLE)) {
	argv[++argc] = lpCmdLine;
    } else {
	char *dst;
	for (ptr = lpCmdLine; *ptr != '\0';) {
	    char delim = ' ';

	    /* skip spaces before parameter */
	    while (*ptr == ' ')
		ptr++;

	    if (*argv[argc]) {
		TRACE(("argv[%d]:%s\n", argc, argv[argc]));
		++argc;
	    }
	    if ((size_t) (argc + 1) >= maxargs) {
		break;
	    }
	    argv[argc] = dst = ptr;
	    while (*ptr != delim && *ptr != '\0') {
		if (*ptr == '"') {
		    ptr++;
		    delim = (char) ((delim == ' ') ? '"' : ' ');
		} else {
		    *dst++ = *ptr++;
		}
	    }
	    if (*ptr == '"') {
		++ptr;
	    }
	    if (dst != ptr) {
		*dst = '\0';
	    } else if (*ptr == ' ') {
		*ptr++ = '\0';
	    }
	}
    }
    if (*argv[argc]) {
	TRACE(("argv[%d]:%s\n", argc, argv[argc]));
	++argc;
    }
    fontstr = 0;

    /*
     * If our working directory is ${HOMEDRIVE}${HOME} and we're given a
     * filename, try to set the working directory based on the last one.
     * Drag/drop would only do this for us if we registered for each file
     * type; otherwise it's useful when we have the window already open.
     */
    if (argc >= 2
	&& we_are_at_home()
	&& ffaccess(argv[argc - 1], FL_READABLE))
	cd_on_open = -1;

    SetCols(80);
    SetRows(24);

    /*
     * Get screen size and OLE options, if any.  Parsing logic is
     * messy, but must remain that way to handle the various command
     * line options available with and without OLE automation.
     */
    for (n = 1; n < argc; n++) {
	int m = n, eat = 0;
	if (n + 1 < argc) {
	    if (strcmp(argv[n], "-geometry") == 0) {
		char *src = argv[n + 1];
		char *dst = 0;
		int value = strtol(src, &dst, 0);
		if (dst != src) {
		    if (value > 2)
			SetCols(value);
		    if (*dst++ == 'x') {
			src = dst;
			value = strtol(src, &dst, 0);
			if (value > 2) {
			    SetRows(value);
#ifdef VILE_OLE
			    oa_opts.cols = term.cols;
			    oa_opts.rows = term.rows;
#endif
			}
		    }
		    eat = 2;
		}
	    } else if (strcmp(argv[n], "-font") == 0 ||
		       strcmp(argv[n], "-fn") == 0) {
		fontstr = argv[n + 1];
		eat = 2;
	    }
	}
#ifdef VILE_OLE
	if (eat == 0) {
	    /* No valid options seen yet. */

	    if (argv[n][0] == '-' && argv[n][1] == 'O') {
		int which = argv[n][2];

		if (which == 'r') {
		    /*
		     * Flag OLE registration request,
		     * but don't eat argument.  Instead,
		     * registration will be carried out
		     * in main.c, so that the regular
		     * cmdline parser has an opportunity
		     * to flag misspelled OLE options.
		     *
		     * Ex:  winvile -Or -multiple
		     */

		    oa_reg = TRUE;
		} else if (which == 'u')
		    ExitProgram(oleauto_unregister());
		else if (which == 'a') {
		    oa_invoke = TRUE;
		    eat = 1;
		}
	    } else if (strcmp(argv[n], "-invisible") == 0) {
		oa_opts.invisible = TRUE;
		eat = 1;
	    } else if (strcmp(argv[n], "-multiple") == 0) {
		oa_opts.multiple = TRUE;
		eat = 1;
	    }
	}
#endif
	if (eat) {
	    while (m + eat <= argc) {
		argv[m] = argv[m + eat];
		m++;
	    }
	    n--;
	    argc -= eat;
	}
    }

#ifdef VILE_OLE
    if (oa_reg && oa_invoke) {
	/* tsk tsk */

	MessageBox(cur_win->main_hwnd,
		   "-Oa and -Or are mutually exclusive",
		   prognam,
		   MB_OK | MB_ICONSTOP);
	ExitProgram(BADEXIT);
    }
    if (oa_reg) {
	/*
	 * The main program's cmd line parser will eventually cause
	 * OLE autoamation registration to occur, at which point
	 * winvile exits.  So don't show a window.
	 */

	nCmdShow = SW_HIDE;
    }
    if (oa_opts.invisible)
	nCmdShow = SW_HIDE;
#endif
    desired_wdw_state = nCmdShow;

    if (!InitInstance(hInstance))
	return (FALSE);

    /*
     * Vile window created and default font set.  It's now kosher to set
     * the font from a cmdline switch.
     */
    if (fontstr) {
	int success = ntwinio_font_frm_str(fontstr, TRUE);

#ifdef VILE_OLE
	if (oa_reg) {
	    if (!success) {
		/*
		 * That's it, game over -- crummy font spec detected during
		 * OLE registration.
		 */

		ExitProgram(BADEXIT);
	    } else
		oa_opts.fontstr = fontstr;
	}
#else
	(void) success;
#endif
	/* Regardless of success or failure, continue with new/default font. */
    }
#ifdef VILE_OLE
    if (oa_invoke) {
	/* Intialize OLE Automation */

	if (!oleauto_init(&oa_opts))
	    ExitProgram(BADEXIT);
    }
#endif

    return MainProgram(argc, argv);
}

/* return winvile's main window handle */
void *
winvile_hwnd(void)
{
    return (cur_win->main_hwnd);
}

/*
 * To prevent drawing winvile's main frame twice during startup (once for
 * the default font/geometry and then again when the user specifies
 * his/her preferences), this entry point is provided to do the job just
 * once.  Called from main.c .
 */
void
winvile_start(void)
{
    RECT desktop, vile, tray;
    int moved_window = 0;
    HWND tray_hwnd, desktop_hwnd;

    enable_popup = global_g_val(GMDPOPUPMENU);

    /*
     * Before displaying the main window, see if its bottom border lies
     * beneath the Win95/NT taskbar.  If so, move the frame up out of the
     * way (hope this code works for Win98, too :-) ).
     */
    tray_hwnd = FindWindow("Shell_TrayWnd", NULL);
    desktop_hwnd = GetDesktopWindow();
    GetWindowRect(desktop_hwnd, &desktop);
    GetWindowRect(cur_win->main_hwnd, &vile);
    if (tray_hwnd != NULL) {
	GetWindowRect(tray_hwnd, &tray);
	if (vile.bottom > tray.top) {
	    /*
	     * Could be a conflict...but only if the taskbar is parked
	     * horizontally, at bottom of screen.
	     */

	    if ((tray.bottom + 10 >= desktop.bottom) &&
		(tray.right - tray.left + 10 >= desktop.right - desktop.left)) {
		int diff = vile.bottom - tray.top + 5;

		if (vile.top - diff >= desktop.top) {
		    /* editor's title bar won't be shifted offscreen. */

		    MoveWindow(cur_win->main_hwnd,
			       vile.left,
			       vile.top - diff,
			       vile.right - vile.left,
			       vile.bottom - vile.top,
			       FALSE);
		    moved_window = 1;
		}
	    }
	}
    }
    if (!moved_window) {
	/*
	 * no conflict with the task bar...but is the editor's bottom edge
	 * below the deskop rect?
	 */

	if (vile.bottom > desktop.bottom) {
	    int diff = vile.bottom - desktop.bottom + 5;

	    if (vile.top - diff >= desktop.top) {
		/* editor's title bar won't be shifted offscreen. */

		MoveWindow(cur_win->main_hwnd,
			   vile.left,
			   vile.top - diff,
			   vile.right - vile.left,
			   vile.bottom - vile.top,
			   FALSE);
	    }
	}
    }
    ShowWindow(cur_win->main_hwnd, desired_wdw_state);
    UpdateWindow(cur_win->main_hwnd);
    caret_disabled = FALSE;
    fshow_cursor();
    initialized = TRUE;
}

/*
 * Unfortunately, winvile cannot use vile's standard cursor visible/invisible
 * API.  Instead, winvile manages its cursor state internally (and does a
 * pretty good job, at that).  However, there are times when the editor
 * makes display changes that require an external "override".
 *
 * Set queue_change (Boolean) true to defer cursor change until winvile next
 * reads its message queue).
 *
 * Returns:  Boolean (previous cursor visibility state).
 */
int
winvile_cursor_state(
			int visible,	/* Boolean, T -> cursor on */
			int queue_change)
{
    int rc = caret_visible;

    if (!queue_change) {
	if (visible) {
	    /* don't set turn on cursor unless editor has focus */
	    if (GetFocus() == winvile_hwnd())
		fshow_cursor();
	} else
	    fhide_cursor();
    } else {
	PostMessage(winvile_hwnd(),
		    (visible) ? WM_WVILE_CURSOR_ON : WM_WVILE_CURSOR_OFF,
		    0,
		    0);
    }
    return (rc);
}

/*
 * winvile doesn't exit via a traditional event loop.  provide a hook
 * to cleanup allocated resources.
 */
void
winvile_cleanup(void)
{
    if (cur_win->main_hwnd)
	DragAcceptFiles(cur_win->main_hwnd, FALSE);
    if (popup_menu) {
	DestroyMenu(popup_menu);
	popup_menu = NULL;	/* be re-entrant */
    }
}

#ifdef VILE_OLE
void
ntwinio_oleauto_reg(void)
{
    /* Pound a bunch of OLE registration data into the registry & exit. */

    ExitProgram(oleauto_register(&oa_opts));
}

void
ntwinio_redirect_hwnd(int redirect)
{
    redirect_keys = redirect;
}
#endif

/*
 * Handle changes to "popup-menu" global setting.
 */
int
chgd_popupmenu(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    if (!testing)
	enable_popup_menu();
    return TRUE;
}

#if OPT_ICURSOR

/* supported syntax is described in chgd_icursor() */
static int
parse_icursor_string(char *str)
{
    int failed, rc = TRUE;
    ULONG style;

    style = vl_atoul(str, 10, &failed);
    if (failed || style > MAX_CURSOR_STYLE)
	rc = FALSE;
    else {
	icursor_style = style;
	icursor = (style > 0);
    }
    return (rc);
}

/*
 * user changed icursor mode
 *
 * Insertion cursor mode is a string that specifies one of three values:
 *
 *     0 -> disable insertion cursor
 *     1 -> insert mode displays vertical bar, cmd mode displays block cursor
 *     2 -> cmd mode displays vertical bar, insert mode displays block cursor
 */
int
chgd_icursor(BUFFER *bp, VALARGS * args, int glob_vals, int testing)
{
    int rc = TRUE;

    if (!testing) {
	char *val = args->global->vp->p;

	if (!parse_icursor_string(val)) {
	    mlforce("[invalid icursor syntax]");
	    rc = FALSE;
	} else {
	    if (caret_visible) {
		fhide_cursor();	/* Kill the old caret        */
		fshow_cursor();	/* And bring it back to life */
	    }
	}
    }
    return (rc);
}
#endif /* OPT_ICURSOR */

/*
 * Split the version-message to allow us to format with tabs, so the
 * proportional font doesn't look ugly.
 */
static size_t
option_size(const char *option)
{
    if (*option == '-') {
	const char *next = skip_ctext(option);
	if (next[0] == ' '
	    && next[1] != ' '
	    && next[1] != 0) {
	    next = skip_ctext(next + 1);
	    return next - option;
	}
	return 14;		/* use embedded blanks to fix the tabs ... */
    }
    return 0;
}

void
gui_version(char *program)
{
    ShowWindow(cur_win->main_hwnd, SW_HIDE);
    MessageBox(cur_win->main_hwnd, getversion(), prognam, MB_OK | MB_ICONSTOP);
}

void
gui_usage(char *program, const char *const *options, size_t length)
{
    char *buf, *s;
    size_t need, n;
    const char *fmt1 = "%s\n\nOptions:\n";
    const char *fmt2 = "    %s\t%s\n";
    const char *fmt3 = "%s\n";

    /*
     * Hide the (partly-constructed) main window.  It'll flash (FIXME).
     */
    ShowWindow(cur_win->main_hwnd, SW_HIDE);

    need = strlen(fmt1) + strlen(prognam);
    for (n = 0; n < length; n++) {
	if (option_size(options[n]))
	    need += strlen(fmt2) + strlen(options[n]);
	else
	    need += strlen(fmt3) + strlen(options[n]);
    }

    if ((buf = typeallocn(char, need)) != 0) {

	sprintf(s = buf, fmt1, prognam);
	for (n = 0; n < length; n++) {
	    char temp[80];

	    s += strlen(s);
	    if ((need = option_size(options[n])) != 0) {
		strncpy(temp, options[n], need);
		temp[need] = EOS;
		sprintf(s, fmt2, temp, skip_cblanks(options[n] + need));
	    } else {
		sprintf(s, fmt3, options[n]);
	    }
	}

	MessageBox(cur_win->main_hwnd, buf, prognam, MB_OK | MB_ICONSTOP);
    }
}

/*
 * Standard terminal interface dispatch table. None of the fields point into
 * "termio" code.
 */

TERM term =
{
    NROW,
    NROW,
    NCOL,
    NCOL,
    NPAUSE,
    ntopen,
    ntclose,
    ntkopen,
    ntkclose,
    nullterm_clean,
    nullterm_unclean,
    nullterm_openup,
    ntgetch,
    ntputc,
    nttypahead,
    ntflush,
    ntmove,
    nteeol,
    nteeop,
    ntbeep,
    ntrev,
    ntcres,
#if OPT_COLOR
    ntfcol,
    ntbcol,
    set_ctrans,
#else
    nullterm_setfore,
    nullterm_setback,
    nullterm_setpal,
#endif
    nullterm_setccol,
    ntscroll,
    nullterm_pflush,
#if OPT_ICURSOR
    nticursor,
#else
    nullterm_icursor,
#endif
#if OPT_TITLE
    nttitle,
#else
    nullterm_settitle,
#endif
    nullterm_watchfd,
    nullterm_unwatchfd,
    nullterm_cursorvis,
    nullterm_mopen,
    nullterm_mclose,
    nullterm_mevent,
};
