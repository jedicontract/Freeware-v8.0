/* $Id: esc.h,v 1.54 2005/01/07 20:01:40 tom Exp $ */

#ifndef ESC_H
#define ESC_H 1

#define BEL 0007
#define SO  0016       /* 14 */
#define SI  0017       /* 15 */
#define ESC 0033
#define CSI 0233
#define SS3 0217
#define DCS 0220
#define ST  0234

/*
 * "ANSI" modes for DECRQM, DECRPM, SM and RM are a subset of the modes listed
 * in ECMA-48.  The ones noted as "non-DEC" are not implemented in any of
 * DEC's terminals.
 */
#define GATM      1    /* guarded area transfer (disabled) */
#define KAM       2    /* keyboard action */
#define CRM       3    /* control representation (setup) */
#define IRM       4    /* insert/replace */
#define SRTM      5    /* status reporting transfer (disabled) */
#define ERM       6    /* erasure mode (non-DEC) */
#define VEM       7    /* vertical editing (disabled) */
#define BDSM      8    /* bi-directional support mode (non-DEC) */
#define DCSM      9    /* device component select mode (non-DEC) */
#define HEM      10    /* horizontal editing (disabled) */
#define PUM      11    /* positioning unit (disabled) */
#define SRM      12    /* send/receive */
#define FEAM     13    /* format effector action (disabled) */
#define FETM     14    /* format effector transfer (disabled) */
#define MATM     15    /* multiple area transfer (disabled) */
#define TTM      16    /* transfer termination (disabled) */
#define SATM     17    /* selected area transfer (disabled) */
#define TSM      18    /* tabulation stop (disabled) */
#define EBM      19    /* editing boundary (disabled) */
#define LNM      20    /* line feed/new line */

/* DEC private modes for DECRQM, DECRPM, SM and RM, based on list from
 * VT520 reference manual, as well as dec_term_function_checklist.ps from
 * Shuford's archive.
 */
#define DECCKM    1    /* cursor keys */
#define DECANM    2    /* ANSI */
#define DECCOLM   3    /* column */
#define DECSCLM   4    /* scrolling */
#define DECSCNM   5    /* screen */
#define DECOM     6    /* origin */
#define DECAWM    7    /* autowrap */
#define DECARM    8    /* autorepeat */
#define DECEDM   10    /* edit */
#define DECLTM   11    /* line transmit */
#define DECSCFDM 13    /* space compression field delimiter */
#define DECTEM   14    /* transmission execution */
#define DECEKEM  16    /* edit key execution */
#define DECPFF   18    /* print form feed */
#define DECPEX   19    /* printer extent */
#define DECTCEM  25    /* text cursor enable */
#define DECRLM   34    /* left-to-right */
#define DECTEK   35    /* 4010/4014 emulation */
#define DECHEM   36    /* Hebrew encoding */
#define DECNRCM  42    /* national replacement character set */
#define DECGEPM  43    /* graphics expanded print */
#define DECGPCM  44    /* graphics print color */
#define DECGPCS  45    /* graphics print color syntax */
#define DECGPBM  46    /* graphics print background */
#define DECGRPM  47    /* graphics rotated print */
#define DEC131TM 53    /* VT131 transmit */
#define DECNAKB  57    /* Greek/N-A Keyboard Mapping */
#define DECHCCM  60    /* horizontal cursor coupling (disabled) */
#define DECVCCM  61    /* vertical cursor coupling */
#define DECPCCM  64    /* page cursor coupling */
#define DECNKM   66    /* numeric keypad */
#define DECBKM   67    /* backarrow key */
#define DECKBUM  68    /* keyboard usage */
#define DECVSSM  69    /* vertical split */
#define DECXRLM  73    /* transmit rate linking */
#define DECKPM   81    /* keyboard positioning */
#define DECNCSM  95    /* no clearing screen on column change */
#define DECRLCM  96    /* right-to-left copy */
#define DECCRTSM 97    /* CRT save */
#define DECARSM  98    /* auto resize */
#define DECMCM   99    /* modem control */
#define DECAAM   100   /* auto answerback */
#define DECCANSM 101   /* conceal answerback */
#define DECNULM  102   /* null */
#define DECHDPXM 103   /* half duplex */
#define DECESKM  104   /* enable secondary keyboard language */
#define DECOSCNM 106   /* overscan */
#define DECFWM   111   /* framed windows */
#define DECRPL   112   /* review previous lines */
#define DECHWUM  113   /* host wake-up mode (CRT and energy saver) */
#define DECATCUM 114   /* alternate text color underline */
#define DECATCBM 115   /* alternate text color blink */
#define DECBBSM  116   /* bold and blink style */
#define DECECM   117   /* erase color */

/* esc.c */
char *csi_input(void);
char *csi_output(void);
char *dcs_input(void);
char *dcs_output(void);
char *get_reply(void);
char *instr(void);
char *osc_input(void);
char *osc_output(void);
char *ss3_input(void);
char *ss3_output(void);
char *st_input(void);
char *st_output(void);
char inchar(void);
int cup(int pn1, int pn2);
int decdc(int pn);
int decic(int pn);
int println(char *s);
int tprintf(const char *fmt, ...);
void brc(int pn, int c);
void brc2(int pn1, int pn2, int c);
void brc3(int pn1, int pn2, int pn3, int c);
void cbt(int pn);
void cha(int pn);
void cht(int pn);
void cnl(int pn);
void cpl(int pn);
void cub(int pn);
void cud(int pn);
void cuf(int pn);
void cuu(int pn);
void da(void);
void dch(int pn);
void decaln(void);
void decarm(int flag);
void decawm(int flag);
void decbi(void);
void decbkm(int flag);
void deccara(int top, int left, int bottom, int right, int attr);
void decckm(int flag);
void deccolm(int flag);
void deccra(int Pts, int Pl, int Pbs, int Prs, int Pps, int Ptd, int Pld, int Ppd);
void decdhl(int lower);
void decdwl(void);
void decefr(int top, int left, int bottom, int right);
void decelr(int all_or_one, int pixels_or_cells);
void decera(int top, int left, int bottom, int right);
void decfi(void);
void decfra(int c, int top, int left, int bottom, int right);
void decid(void);
void deckbum(int flag);
void deckpam(void);
void deckpm(int flag);
void deckpnm(void);
void decll(char *ps);
void decnkm(int flag);
void decnrcm(int flag);
void decom(int flag);
void decpex(int flag);
void decpff(int flag);
void decrara(int top, int left, int bottom, int right, int attr);
void decrc(void);
void decreqtparm(int pn);
void decrqlp(int mode);
void decrqss(char *pn);
void decsace(int flag);
void decsasd(int pn);
void decsc(void);
void decsca(int pn1);
void decsclm(int flag);
void decscnm(int flag);
void decsed(int pn1);
void decsel(int pn1);
void decsera(int top, int left, int bottom, int right);
void decsle(int mode);
void decsnls(int pn);
void decssdt(int pn);
void decstbm(int pn1, int pn2);
void decstr(void);
void decswl(void);
void dectst(int pn);
void dl(int pn);
void do_csi(char *fmt, ...) GCC_PRINTFLIKE(1,2);
void do_dcs(char *fmt, ...) GCC_PRINTFLIKE(1,2);
void do_osc(char *fmt, ...) GCC_PRINTFLIKE(1,2);
void dsr(int pn);
void ech(int pn);
void ed(int pn);
void el(int pn);
void esc(char *s);
void extra_padding(int msecs);
void holdit(void);
void hpa(int pn);
void hts(void);
void hvp(int pn1, int pn2);
void ich(int pn);
void il(int pn);
void ind(void);
void inflush(void);
void inputline(char *s);
void mc_autoprint(int flag);
void mc_print_all_pages(void);
void mc_print_composed(void);
void mc_print_cursor_line(void);
void mc_print_page(void);
void mc_printer_assign(int flag);
void mc_printer_controller(int flag);
void mc_printer_start(int flag);
void nel(void);
void padding(int msecs);
void put_char(FILE *fp, int c);
void put_string(FILE *fp, char *s);
void readnl(void);
void rep(int pn);
void reset_inchar(void);
void ri(void);
void ris(void);
void rm(char *ps);
void s8c1t(int flag);
void scs(int g, int c);
void sd(int pn);
void sgr(char *ps);
void sl(int pn);
void sm(char *ps);
void sr(int pn);
void srm(int flag);
void su(int pn);
void tbc(int pn);
void vpa(int pn);
void vt52cub1(void);
void vt52cud1(void);
void vt52cuf1(void);
void vt52cup(int l, int c);
void vt52cuu1(void);
void vt52ed(void);
void vt52el(void);
void vt52home(void);
void vt52ri(void);
void zleep(int t);

#endif /* ESC_H */
r                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            