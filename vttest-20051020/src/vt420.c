/* $Id: vt420.c,v 1.64 2004/08/03 01:21:06 tom Exp $ */

/*
 * Reference:  Installing and Using the VT420 Video Terminal (North American
 *             Model (EK-VT420-UG.002)
 */
#include <vttest.h>
#include <draw.h>
#include <esc.h>
#include <ttymodes.h>

typedef struct {
  int mode;
  char *name;
} MODES;

/******************************************************************************/

static int
rpt_DECSACE(MENU_ARGS)
{
  return any_decrqss(the_title, "*x");
}

static int
rpt_DECSNLS(MENU_ARGS)
{
  return any_decrqss(the_title, "*|");
}

static int
rpt_DECSLRM(MENU_ARGS)
{
  return any_decrqss(the_title, "s");
}

static int
rpt_DECELF(MENU_ARGS)
{
  return any_decrqss(the_title, "+q");
}

/*
 * VT420 manual shows "=}", but the terminal returns an error.  VT510 sequences
 * show "*}".
 */
static int
rpt_DECLFKC(MENU_ARGS)
{
  return any_decrqss(the_title, "*}");
}

static int
rpt_DECSMKR(MENU_ARGS)
{
  return any_decrqss(the_title, "+r");
}

/******************************************************************************/

static void
show_DataIntegrity(char *report)
{
  int pos = 0;
  int code = scanto(report, &pos, 'n');
  char *show;

  switch (code) {
  case 70:
    show = "No communication errors";
    break;
  case 71:
    show = "Communication errors";
    break;
  case 73:
    show = "Not reported since last power-up or RIS";
    break;
  default:
    show = SHOW_FAILURE;
  }
  show_result(show);
}

static void
show_keypress(int row, int col)
{
  char *report;
  char last[BUFSIZ];

  last[0] = '\0';
  vt_move(row++, 1);
  println("When you are done, press any key twice to quit.");
  vt_move(row, col);
  fflush(stdout);
  while (strcmp(report = instr(), last)) {
    vt_move(row, col);
    vt_clear(0);
    chrprint(report);
    strcpy(last, report);
  }
}

static void
show_MultisessionStatus(char *report)
{
  int pos = 0;
  int Ps1 = scan_any(report, &pos, 'n');
  int Ps2 = scanto(report, &pos, 'n');
  char *show;

  switch (Ps1) {
  case 80:
    show = "SSU sessions enabled (%d max)";
    break;
  case 81:
    show = "SSU sessions available but pending (%d max)";
    break;
  case 83:
    show = "SSU sessions not ready";
    break;
  case 87:
    show = "Sessions on separate lines";
    break;
  default:
    show = SHOW_FAILURE;
  }
  show_result(show, Ps2);
}

/******************************************************************************/

/*
 * VT400 & up.
 * DECBI - Back Index
 * This control function moves the cursor backward one column.  If the cursor
 * is at the left margin, then all screen data within the margin moves one
 * column to the right.  The column that shifted past the right margin is lost.
 * 
 * Format:  ESC 6
 * Description:
 * DECBI adds a new column at the left margin with no visual attributes.  DECBI
 * is not affected by the margins.  If the cursor is at the left border of the
 * page when the terminal received DECBI, then the terminal ignores DECBI.
 */
static int
tst_DECBI(MENU_ARGS)
{
  int n, m;
  int last = max_lines - 4;
  int final = min_cols / 4;

  for (n = final; n > 0; n--) {
    cup(1, 1);
    if (n != final) {
      for (m = 0; m < 4; m++)
        decbi();
    }
    printf("%3d", n);
  }

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("If your terminal supports DECBI (backward index), then the top row");
  printf("should be numbered 1 through %d.\n", final);
  return MENU_HOLD;
}

static int
tst_DECBKM(MENU_ARGS)
{
  char *report;

  vt_move(1, 1);
  println(the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  reset_inchar();
  decbkm(TRUE);
  println("Press the backspace key");
  vt_move(3, 10);
  report = instr();
  chrprint(report);
  show_result(!strcmp(report, "\010") ? SHOW_SUCCESS : SHOW_FAILURE);

  reset_inchar();
  vt_move(5, 1);
  decbkm(FALSE);
  println("Press the backspace key again");
  vt_move(6, 10);
  report = instr();
  chrprint(report);
  show_result(!strcmp(report, "\177") ? SHOW_SUCCESS : SHOW_FAILURE);

  vt_move(max_lines - 1, 1);
  restore_ttymodes();
  return MENU_HOLD;
}

/*
 * VT400 & up
 * Change Attributes in Rectangular Area
 */
static int
tst_DECCARA(MENU_ARGS)
{
  int last = max_lines - 4;
  int top = 5;
  int left = 5;
  int right = 45;
  int bottom = max_lines - 10;

  decsace(TRUE);
  decaln();     /* fill the screen */
  deccara(top, left, bottom, right, 7);   /* invert a rectangle) */
  deccara(top + 1, left + 1, bottom - 1, right - 1, 0);   /* invert a rectangle) */

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("There should be an open rectangle formed by reverse-video E's");
  holdit();

  decsace(FALSE);
  decaln();     /* fill the screen */
  deccara(top, left, bottom, right, 7);   /* invert a rectangle) */
  deccara(top + 1, left + 1, bottom - 1, right - 1, 0);   /* invert a rectangle) */

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("There should be an open rectangle formed by reverse-video E's");
  println("combined with wrapping at the margins.");
  return MENU_HOLD;
}

static int
tst_DECCKSR(MENU_ARGS, int Pid, char *the_csi)
{
  char *report;
  int pos = 0;

  vt_move(1, 1);
  printf("Testing DECCKSR: %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("%s", the_csi);
  report = get_reply();
  vt_move(3, 10);
  chrprint(report);
  if ((report = skip_dcs(report)) != 0
      && strip_terminator(report)
      && strlen(report) > 1
      && scanto(report, &pos, '!') == Pid
      && report[pos++] == '~'
      && (report = skip_digits(report + pos + 1)) != 0
      && *report == '\0') {
    show_result(SHOW_SUCCESS);
  } else {
    show_result(SHOW_FAILURE);
  }

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

/*
 * VT400 & up.
 * Copy Rectangular area
 */
static int
tst_DECCRA(MENU_ARGS)
{
  BOX box;

  if (make_box_params(&box, 10, 30) < 0)
    return MENU_NOHOLD;
  box.top = 5;
  box.left = 5;
  draw_box_outline(&box, '*');

  vt_move(max_lines - 3, 1);
  println(the_title);
  println("The box of *'s will be copied");
  holdit();

  deccra(box.top, box.left, box.bottom, box.right, 1,
         box.top + 3, box.left + 4, 1);

  vt_move(max_lines - 2, 1);
  vt_clear(0);

  println("The box should be copied, overlapping");
  return MENU_HOLD;
}

/*
 * VT400 & up.
 * Delete column.
 */
static int
tst_DECDC(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(n, last - n + 22) && printf("*");
    cup(1, 1) && decdc(1);
  }
  cup(1, 1) && decdc(20);

  vt_move(last + 1, 1);
  println("If your terminal supports DECDC, there will be a column of *'s on the left");
  return MENU_HOLD;
}

/*
 * VT400 & up
 * Erase Rectangular area
 */
static int
tst_DECERA(MENU_ARGS)
{
  decaln();
  decera(5, 5, max_lines - 10, min_cols - 5);

  vt_move(max_lines - 3, 1);
  vt_clear(0);

  println(the_title);
  println("There should be a rectangle cleared in the middle of the screen.");
  return MENU_HOLD;
}

/*
 * VT400 & up.
 *
 * DECFI - Forward Index
 * This control function moves the column forward one column.  If the cursor is
 * at the right margin, then all screen data within the margins moves one
 * column to the left.  The column shifted past the left margin is lost.
 * 
 * Format: ESC 9
 * Description:
 * DECFI adds a new column at the right margin with no visual attributes. 
 * DECFI is not affected by the margins.  If the cursor is at the right border
 * of the page when the terminal receives DECFI, then the terminal ignores
 * DECFI.
 */
static int
tst_DECFI(MENU_ARGS)
{
  int n, m;
  int last = max_lines - 4;
  int final = min_cols / 4;

  for (n = 1; n <= final; n++) {
    cup(1, min_cols - 3);
    printf("%3d", n);   /* leaves cursor in rightmost column */
    if (n != final) {
      for (m = 0; m < 4; m++)
        decfi();
    }
  }

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("If your terminal supports DECFI (forward index), then the top row");
  printf("should be numbered 1 through %d.\n", final);
  return MENU_HOLD;
}

/*
 * VT400 & up
 * Fill Rectangular area
 */
static int
tst_DECFRA(MENU_ARGS)
{
  decfra('*', 5, 5, max_lines - 10, min_cols - 5);

  vt_move(max_lines - 3, 1);
  vt_clear(0);

  println(the_title);
  println("There should be a rectangle filled in the middle of the screen.");
  return MENU_HOLD;
}

/*
 * VT400 & up.
 * Insert column.
 */
static int
tst_DECIC(MENU_ARGS)
{
  int n;
  int last = max_lines - 3;

  for (n = 1; n < last; n++) {
    cup(n, min_cols - 22 - last + n) && printf("*");
    cup(1, 1) && decic(1);
  }
  decic(20);

  vt_move(last + 1, 1);
  println("If your terminal supports DECIC, there will be a column of *'s on the right");
  return MENU_HOLD;
}

static int
tst_DECKBUM(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  deckbum(TRUE);
  println("The keyboard is set for data processing.");
  show_keypress(3, 10);

  vt_move(10, 1);
  deckbum(FALSE);
  println("The keyboard is set for normal (typewriter) processing.");
  show_keypress(11, 10);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static int
tst_DECKPM(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  deckpm(TRUE);
  println("The keyboard is set for position reports.");
  show_keypress(3, 10);

  vt_move(10, 1);
  deckpm(FALSE);
  println("The keyboard is set for character codes.");
  show_keypress(11, 10);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static int
tst_DECNKM(MENU_ARGS)
{
  vt_move(1, 1);
  println(the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  decnkm(FALSE);
  println("Press one or more keys on the keypad.  They should generate numeric codes.");
  show_keypress(3, 10);

  vt_move(10, 1);
  decnkm(TRUE);
  println("Press one or more keys on the keypad.  They should generate control codes.");
  show_keypress(11, 10);

  decnkm(FALSE);
  vt_move(max_lines - 1, 1);
  restore_ttymodes();
  return MENU_HOLD;
}

/*
 * VT400 & up
 * Reverse Attributes in Rectangular Area
 */
static int
tst_DECRARA(MENU_ARGS)
{
  int last = max_lines - 4;
  int top = 5;
  int left = 5;
  int right = 45;
  int bottom = max_lines - 10;

  decsace(TRUE);
  decaln();     /* fill the screen */
  decrara(top, left, bottom, right, 7);   /* invert a rectangle) */
  decrara(top + 1, left + 1, bottom - 1, right - 1, 7);   /* invert a rectangle) */

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("There should be an open rectangle formed by reverse-video E's");
  holdit();

  decsace(FALSE);
  decaln();     /* fill the screen */
  decrara(top, left, bottom, right, 7);   /* invert a rectangle) */
  decrara(top + 1, left + 1, bottom - 1, right - 1, 7);   /* invert a rectangle) */

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("There should be an open rectangle formed by reverse-video E's");
  println("combined with wrapping at the margins.");
  return MENU_HOLD;
}

static int
tst_DECRQSS(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320_DECRQSS },
      { "Select attribute change extent (DECSACE)",          rpt_DECSACE },
      { "Set number of lines per screen (DECSNLS)",          rpt_DECSNLS },
      { "Set left and right margins (DECSLRM)",              rpt_DECSLRM },
      { "Enable local functions (DECELF)",                   rpt_DECELF },
      { "Local function key control (DECLFKC)",              rpt_DECLFKC },
      { "Select modifier key reporting (DECSMKR)",           rpt_DECSMKR },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Status-Strings Reports");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/*
 * Selective-Erase Rectangular area
 */
static int
tst_DECSERA(MENU_ARGS)
{
  int top = 5;
  int left = 5;
  int right = 45;
  int bottom = max_lines - 10;
  int last = max_lines - 3;

  decaln();
  decsca(1);
  decfra('E', top + 1, left + 1, bottom - 1, right - 1);
  decsca(1);
  decsera(top, left, bottom, right);  /* erase the inside */

  vt_move(last, 1);
  vt_clear(0);

  println(the_title);
  println("There should be an open rectangle formed by blanks on a background of E's");

  holdit();
  decaln();
  return MENU_NOHOLD;
}

/* FIXME: use DECRQSS to get reports */
static int
tst_DECSNLS(MENU_ARGS)
{
  int rows;

  vt_move(1, 1);
  println("Testing Select Number of Lines per Screen (DECSNLS)");

  for (rows = 48; rows >= 24; rows -= 12) {
    set_tty_raw(TRUE);
    set_tty_echo(FALSE);

    printf("%d Lines/Screen: ", rows);
    decsnls(rows);
    decrqss("*|");
    chrprint(instr());
    println("");

    restore_ttymodes();
    holdit();
  }

  return MENU_NOHOLD;
}

static int
tst_DSR_area_sum(MENU_ARGS)
{
  return tst_DECCKSR(PASS_ARGS, 1, "1;1;10;10;20;20*y");
}

static int
tst_DSR_data_ok(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?75n", show_DataIntegrity);
}

static int
tst_DSR_macrospace(MENU_ARGS)
{
  char *report;
  char *show;

  vt_move(1, 1);
  printf("Testing DECMSR: %s\n", the_title);

  set_tty_raw(TRUE);
  set_tty_echo(FALSE);

  do_csi("?62n");
  report = instr();
  vt_move(3, 10);
  chrprint(report);
  if ((report = skip_csi(report)) != 0
      && (report = skip_digits(report)) != 0
      && !strcmp(report, "*{")) {
    show = SHOW_SUCCESS;
  } else {
    show = SHOW_FAILURE;
  }
  show_result(show);

  restore_ttymodes();
  vt_move(max_lines - 1, 1);
  return MENU_HOLD;
}

static int
tst_DSR_memory_sum(MENU_ARGS)
{
  return tst_DECCKSR(PASS_ARGS, 1, "?63;1n");
}

static int
tst_DSR_multisession(MENU_ARGS)
{
  return any_DSR(PASS_ARGS, "?85n", show_MultisessionStatus);
}

int
tst_SRM(MENU_ARGS)
{
  int oldc, newc;

  vt_move(1, 1);
  println(the_title);

  set_tty_raw(TRUE);

  set_tty_echo(FALSE);
  srm(FALSE);

  println("Local echo is enabled, remote echo disabled.  Press any keys, repeat to quit.");
  vt_move(3, 10);

  oldc = -1;
  while ((newc = inchar()) != oldc)
    oldc = newc;

  set_tty_echo(TRUE);
  srm(TRUE);

  vt_move(10, 1);
  println("Local echo is disabled, remote echo enabled.  Press any keys, repeat to quit.");
  vt_move(11, 10);

  oldc = -1;
  while ((newc = inchar()) != oldc)
    oldc = newc;

  vt_move(max_lines - 1, 1);
  restore_ttymodes();
  return MENU_HOLD;
}

/******************************************************************************/

static int
tst_PageFormat(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test set columns per page (DECSCPP)",               not_impl },
      { "Test columns mode (DECCOLM)",                       not_impl },
      { "Test set lines per page (DECSLPP)",                 not_impl },
      { "Test set left and right margins (DECSLRM)",         not_impl },
      { "Test set vertical split-screen (DECVSSM)",          not_impl },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("Page Format Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * The main vt100 module tests CUP, HVP, CUF, CUB, CUU, CUD
 */
static int
tst_VT420_cursor(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320_cursor },
      { "Test Back Index (DECBI)",                           tst_DECBI },
      { "Test Forward Index (DECFI)",                        tst_DECFI },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Cursor-Movement Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * The main vt100 module tests IRM, DL, IL, DCH, ICH, ED, EL
 * The vt220 module tests ECH and DECSCA
 */
static int
tst_VT420_editing(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Delete Column (DECDC)",                        tst_DECDC },
      { "Test Insert Column (DECIC)",                        tst_DECIC },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Editing Sequence Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * The main vt100 module tests LNM, DECKPAM, DECARM, DECAWM
 */
static int
tst_VT420_keyboard_ctl(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Backarrow key (DECBKM)",                       tst_DECBKM },
      { "Test Numeric keypad (DECNKM)",                      tst_DECNKM },
      { "Test Keyboard usage (DECKBUM)",                     tst_DECKBUM },
      { "Test Key position (DECKPM)",                        tst_DECKPM },
      { "Test Enable Local Functions (DECELF)",              not_impl },
      { "Test Local Function-Key Control (DECLFKC)",         not_impl },
      { "Test Select Modifier-Key Reporting (DECSMKR)",      not_impl }, /* DECEKBD */
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Keyboard-Control Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/*
 * These apply only to VT400's & above
 */
static int
tst_VT420_rectangle(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Change-Attributes in Rectangular Area (DECCARA)", tst_DECCARA },
      { "Test Copy Rectangular area (DECCRA)",               tst_DECCRA },
      { "Test Erase Rectangular area (DECERA)",              tst_DECERA },
      { "Test Fill Rectangular area (DECFRA)",               tst_DECFRA },
      { "Test Reverse-Attributes in Rectangular Area (DECRARA)", tst_DECRARA },
      { "Test Selective-Erase Rectangular area (DECSERA)",   tst_DECSERA },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Rectangular Area Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

/* UDK and rectangle-checksum status are available only on VT400 */

static int
tst_VT420_report_device(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test Printer Status",                               tst_DSR_printer },
      { "Test UDK Status",                                   tst_DSR_userkeys },
      { "Test Keyboard Status",                              tst_DSR_keyboard },
      { "Test Locator Status",                               tst_DSR_locator },
      { "Test Macro Space",                                  tst_DSR_macrospace },
      { "Test Memory Checksum",                              tst_DSR_memory_sum },
      { "Test Data Integrity",                               tst_DSR_data_ok },
      { "Test Multiple Session Status",                      tst_DSR_multisession },
      { "Test Checksum of Rectangular Area",                 tst_DSR_area_sum },
      { "Test Extended Cursor-Position (DECXCPR)",           tst_DSR_cursor },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Device Status Reports (DSR)");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
tst_VT420_report_presentation(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320_report_presentation },
      { "Request Mode (DECRQM)/Report Mode (DECRPM)",        tst_DECRPM },
      { "Status-String Report (DECRQSS)",                    tst_DECRQSS },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  int old_DECRPM = set_DECRPM(4);

  do {
    vt_clear(2);
    title(0) && printf("VT420 Presentation State Reports");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  set_DECRPM(old_DECRPM);
  return MENU_NOHOLD;
}

static int
tst_VT420_reports(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320_reports },
      { "Test Presentation State Reports",                   tst_VT420_report_presentation },
      { "Test Device Status Reports (DSR)",                  tst_VT420_report_device },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Reports");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

static int
tst_VT420_screen(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320_screen },
      { "Test Select Number of Lines per Screen (DECSNLS)",  tst_DECSNLS },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Screen-Display Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}

/******************************************************************************/

int
tst_vt420(MENU_ARGS)
{
  /* *INDENT-OFF* */
  static MENU my_menu[] = {
      { "Exit",                                              0 },
      { "Test VT320 features",                               tst_vt320 },
      { "Test cursor-movement",                              tst_VT420_cursor },
      { "Test editing sequences",                            tst_VT420_editing },
      { "Test keyboard-control",                             tst_VT420_keyboard_ctl },
      { "Test macro-definition (DECDMAC)",                   not_impl },
      { "Test page-format controls",                         tst_PageFormat },
      { "Test rectangular area functions",                   tst_VT420_rectangle },
      { "Test reporting functions",                          tst_VT420_reports },
      { "Test screen-display functions",                     tst_VT420_screen },
      { "",                                                  0 }
    };
  /* *INDENT-ON* */

  do {
    vt_clear(2);
    title(0) && printf("VT420 Tests");
    title(2) && println("Choose test type:");
  } while (menu(my_menu));
  return MENU_NOHOLD;
}
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      