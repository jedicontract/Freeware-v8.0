//-----------------------------------------------------------------------//
//  xmtnimagec.h                                                         //
//  Latest revision: 10-29-1999                                          //
//  Copyright (C) 1999 by Thomas J. Nelson                               //
//  See xmtnimage.h for Copyright Notice                                 //
//-----------------------------------------------------------------------//

#ifdef HAVE_XBAE
#include <Xbae/Matrix.h>      // From ftp.x.org/contrib
#endif  // HAVE_XBAE

#ifdef DIGITAL
#define CELLWIDTH  11         // Width of each cell to display
#define CELLBYTES  12         // Size of string in each cell
#define EDITORSIZE 65536      // Why would anyone ever need more than this

////  Digit types
#define DIGIT 1
#define POINT 2
#define SIGN 3
#define EXPONENT 4
#define HEXDIGIT 5
#else
const int CELLWIDTH = 11;         // Width of each cell to display
const int CELLBYTES = 12;         // Size of string in each cell
const int EDITORSIZE = 65536;     // Why would anyone ever need more than this

////  Digit types
const int DIGIT=1;
const int POINT=2;
const int SIGN=3;
const int EXPONENT=4;
const int HEXDIGIT=5;
#endif

#ifdef HAVE_XBAE

#define XBMECB XbaeMatrixEnterCellCallbackStruct
#define XBMSCCB XbaeMatrixSelectCellCallbackStruct
#define XBMWCCB XbaeMatrixWriteCellCallbackStruct 
#define XBMMVCB XbaeMatrixModifyVerifyCallbackStruct 
#define XBMLACB XbaeMatrixLabelActivateCallbackStruct 
#define XBMLCCB XbaeMatrixLeaveCellCallbackStruct 
#define XBMDCCB XbaeMatrixDrawCellCallbackStruct 
void columnselectcb(Widget w, XtPointer client_data, XBMLACB *call_data);
void dragcb(Widget w, XtPointer client_data, XBMECB *call_data);
void entercellcb(Widget w, XtPointer client_data, XBMECB *call_data);
void leavecellcb(Widget w, XtPointer client_data, XBMLCCB *call_data);
void modifycb(Widget w, XtPointer client_data, XBMMVCB *call_data);
void selectcb(Widget w, XtPointer client_data, XBMSCCB *call_data);

#endif  // HAVE_XBAE

Spreadsheet *new_spreadsheet(int ino, Widget oldwidget);
void add_button_group(Widget parent, int count, int &ystart, int selection, 
     Spreadsheet *s, void *cb, char *title, const char label[][20]);
void copypaste(double *numbers, int total, int mode);
void copy_spreadsheet(int ino1, int ino2);
void create_spreadsheet(int ino);
void delete_spreadsheet(int ino, Widget savewidget);
void deselectallcells(int ino);
void putimageinspreadsheet(int ino);
void puttextinimage(Spreadsheet *s, char *text, int x, int y);
void refreshspreadsheet(int ino);
void select_cell(int ino, int row, int col, int value);
void select_column(int ino, int col, int value);
void ssdisplaycb(Widget w, XtPointer client_data, XmACB *call_data);
void toggle_cell(int ino, int row, int col);
