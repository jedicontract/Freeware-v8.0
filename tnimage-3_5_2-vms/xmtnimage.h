//------------------------------------------------------------------------//
//  xmtnimage.h = Part of xmtnimage   (Motif version)                     //
//  Copyright (C) 2006 by Thomas J. Nelson                                //
//                                                                        // 
//  This program is free software; you can redistribute it and/or modify  //
//  it under the terms of the GNU General Public License as published by  // 
//  the Free Software Foundation; either version 2 of the License, or     //
//  (at your option) any later version.                                   //
//                                                                        //
//  This program is distributed in the hope that it will be useful,       //
//  but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//  GNU General Public License for more details.                          //
//                                                                        //
//  You should have received a copy of the GNU General Public License     //
//  along with this program; if not, write to the Free Software           //
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             //
//------------------------------------------------------------------------//
 
#include "config.h"
#ifndef __CONFIG_H
#error ***config.h not found***
#error ***/Run ./configure before compiling***
#endif
#ifndef XMTNIMAGE
#define XMTNIMAGE 1
#define UNIX 1
/////#define DIGITAL 1     // For Digital 64 bit compiler (should normally be set
                           // in config.h. If you need to set this here, it means
			   // you forgot to run ./configure.)
#define MAXIMAGES 513      // Maximum no. of simultaneous images
#define HELPDIR "/usr/local/lib/tnimage"
#define EDITSIZE 262144    // Size of table in editor
#define DEGPERRAD 57.2957795130823299701
#define RADPERDEG 0.01745329251994329299
#define RPOINTS 10000      // Max. no. of data points in registration

//---------------Include files-------------------------------------------//
#include <stdio.h>
#ifndef __USE_BSD
 #define __USE_BSD 1         // for strdup and popen
#endif
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>        // For index() in Solaris
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#ifdef LINUX
 #include <X11/Xlocale.h>
#else
 #include <locale.h>
#endif
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include "complex.h"        // Use our own complex.h
#include "xmtnimageb.h"
#include <signal.h>         // Needed for semaphores & ipc
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#ifdef HAVE_MOTIF
#include <Xm/XmAll.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>      // For definition of XA_WINDOW  
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/IntrinsicP.h>

#ifdef LINUX
  #include "/usr/include/scsi/sg.h"
#endif

#ifdef SOLARIS
  #include <scsi.h>
#endif

#ifdef IRIX
  #include <sys/scsi.h>
#endif

#include <fcntl.h>
#include <netinet/in.h>  
#ifdef HAVE_XFT
  #include <X11/Xft/Xft.h>  
#endif


#if defined( HAVE_BYTESEX_H ) || defined( HAVE_ENDIAN_H )
 #ifdef HAVE_BYTESEX_H
   #include <bytesex.h>
 #else
   #include <endian.h>
 #endif
 #ifndef LITTLE_ENDIAN
   #if __BYTE_ORDER == __LITTLE_ENDIAN
     #define LITTLE_ENDIAN
   #else
     #undef LITTLE_ENDIAN
   #endif
 #endif
#else
 #ifdef _LITTLE_ENDIAN
   #define LITTLE_ENDIAN
 #else
   #undef LITTLE_ENDIAN
 #endif
#endif

#ifdef IRIX
#undef LITTLE_ENDIAN
#endif



#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXWINDOWS 10
#define PI 3.141592653589793238462643

//---------------#define's-------------------------//

#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#define swapbyte(a,b){ ccc=(a);  (a)=(b); (b)=ccc; }  
#define swap(a,b)  { g.swap_temp=(a);  (a)=(b); (b)=g.swap_temp; }  
#define fswap(a,b) { g.fswap_temp=(a); (a)=(b); (b)=g.fswap_temp; }  
#define dswap(a,b) { g.dswap_temp=(a); (a)=(b); (b)=g.dswap_temp; } 
#define sgn(a) ( (a)>0 ? (1) : ( (a)<0 ? (-1) : (0) ) )
#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long
#define ushort unsigned short int
#ifndef MAXINT
#define MAXINT 2147483647
#endif

#ifdef HAVE_MOTIF
#define XmACB XmAnyCallbackStruct
#define XmSBCB XmSelectionBoxCallbackStruct
#define XmFSBCB XmFileSelectionBoxCallbackStruct
#define XtEH XtEventHandler
#define XtP XtPointer
#define XtCBP XtCallbackProc
#define XmLCB XmListCallbackStruct
#define XmRCCB XmRowColumnCallbackStruct
#endif

//--------------constants-------------------------//

#define NOOFMENUS 9          // no.of menus across top
#define MACROLENGTH 65536    // max. no.of chars in macro
#define FILENAMELENGTH 1024  // max. length of a filename
#define FUZZY_ITEMS 40       // max. no. of items to calculate in fuzzy list

//// Video card types
#define OTHER 0
#define TSENG 1
#define TRIDENT 2
#define VESA 3
#define S3 4
#define ATI 5
#define XGA 6
#define OLDVESA 7

//// Pixel setting modes
#define SET 1
#define MAXIMUM 2
#define MINIMUM 3
#define ADD 4
#define SUB12 5
#define SUB21 6
#define XOR 7
#define AVE 8
#define SUP 9
#define BUFFER 10
#define SCAN 11
#define TRACE 12
#define ANTIALIAS 13

////Line drawing modes
//#define SET   1
#define SUM     2
#define SUM2    3
#define NOT     4
#define NOT2    5
//#define XOR   7

////  Image file types, platforms, & bit packing types
#define RESET -2
#define ERASE -1
#define NONE 0
#define TIF 1
#define PCX 2
#define IMA 3
#define IMG 4
#define GIF 5  // GIF87a
#define IMM 6
#define GEM 7
#define IMDS 8
#define RAW 9
#define PNG 10
#define JPG 11
#define TARGA 12
#define PICT 13
#define RLE 15
#define BMP 16
#define OS2 17
#define CUSTOM 18
#define MAC 20
#define PC 21
#define MVS 22
#define ASCII 23
#define RAW3D 24
#define XWD 25
#define FITS 26
#define PDS 27
#define PDS2 28
#define PBM 29
#define PGM 30
#define PPM 31
#define PBMRAW 32
#define PGMRAW 33
#define PPMRAW 34
#define GIF89 35
#define TEXT 36
#define BIORAD 37
#define MULTIFRAME 38

#define TNI_WRITE 1
////  Bank setting modes (Tseng)
#define TNI_READ 2
////  String comparison
#define SAME 0

////  Write string
#define PERM 1
#define TEMP 0
#define NOEND 2

////  For copyimage
#define GET 0
#define PUT 1

////  Fft display modes
#define FWD 1
#define REV -1

#define ORIGINAL 0
#define REAL 1
#define IMAG 2
#define POWR 3
#define WAVE 4

////  Error codes
#define OK 0
#define YES 0
#define NOMEM 1
#define ABORT 2
#define NOTFOUND 3
#define ERROR 4
#define NOIMAGES 5
#define GOTNEW 6
#define UNKNOWN 7
#define CANTCREATEFILE 8
#define CRITERR 9
#define ZEROLENGTH 10
#define QUIT 11
#define BADPARAMETERS 12
#define NO 14
#define CANCEL 15
#define IGNORE 16

////  For windows
#define TNI_CLOSE 0
#define TNI_OPEN 1
#define HIDE 2
#define UNHIDE 3

////  Movement & positioning modes
#define OFF 0
#define ON 1
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define START 5
#define TNI_CONTINUE 6
#define END 7
#define CENTER 8
#define ULEFT 9
#define URIGHT 10
#define LLEFT 11
#define LRIGHT 12

#define PERMANENT_TILE 2

////  For plot
#define MEASURE 0
#define CHANGE 1
#define SUBTRACT 2
#define BEZIER 3


////  Global states
// #define RESET -2
// #define ERASE -1
#define NORMAL 0
#define MOVE 3
#define COPY 4
#define POSITIONBOX 5
#define DENSITOMETRY 6
#define GETBOX 7
#define REDRAW 8
#define BUSY 9
#define INITIALIZING 10
#define INMENU 11
#define GETNEXTPOINT 12
#define CALCULATING 13
#define CURVE 14
#define SELECT 15
#define GETPOINT 16
#define GETLINE 17
#define DRAWING 18
#define MOVEIMAGE 19
#define DENSITOMETRY2 20
#define DENSITOMETRY3 21
#define DENSITOMETRY4 22
#define READING 23
#define ROTATEPALETTE 24
#define WAITCHAR 25
#define FILL 26
#define WARP 27
#define CALIBRATE 28
#define PRINTING 29
#define TESTING 31
#define ERASING 32
#define NEWIMAGE 33
#define MACRO 35
#define SCANNING 36
#define XTWARNING 37
#define SPRAY 38
#define REDUCE_COLORS 39
#define MESSAGE 40
#define ACQUIRE 41
#define MESSAGE_INVERTED 42
#define PAUSE 43
#define EXPOSE 44
#define SELECTCURVE 45
#define WRITING 46
#define ZOOMING 47
#define MAGNIFYING 48
#define RESIZE_LEFT 49
#define RESIZE_RIGHT 50
#define RESIZE_TOP 51
#define RESIZE_BOTTOM 52
#define GETBOXPOSITION 53

////  Spray modes

#define FINESPRAY 1
#define DIFFUSESPRAY 2
#define MATHSPRAY 3
#define FILTERSPRAY 4
#define ERASESPRAY 5
#define LINESPRAY 6

////  Types of figures
#define LINE 1
#define CIRCLE 2
#define BOX 3
#define RECTANGLE 4
#define FILLEDRECTANGLE 5
#define SKETCH 6
//// #define LABEL 7
#define ARROW 8
#define TEMPLINE 9
#define PIXEL 10
//#define CURVE 14
#define ZOOMED 15
#define CROSS 16
#define ELLIPSE 17
#define OVAL 18
#define ROUNDED_RECTANGLE 19
#define SKETCHMATH 20

////  Number types
#define INTEGER 1
#define FLOAT 2
#define INTARRAY 3
#define DOUBLE 4
// STRING 5
#define CHARACTER 6
#define WORD 7
#define HEXADECIMAL 8
#define RGBVALUE 9
#define RGBHEX 10
#define PARAM_ARRAY 11
#define CALIBRATED_VALUE 12
#define POINTER 13

////  Text direction
#define HORIZONTAL 0
#define VERTICAL 1
#define NOBACKGROUND 0
#define BACKGROUND 1

////  List types in dialog boxes
#define RADIO 0
#define RADIOTEXT 1
#define RADIOPUSHBUTTON 4
#define RADIOMULTI 5

////  Box types in dialog boxes
#define TOGGLE 0        // Toggle on/pff
#define INTCLICKBOX 1   // Get int
#define FILENAME 2      // Get filename 
#define RGBCLICKBOX 3   // Get RGB value
#define NON_EDIT_LIST 4 
#define STRING 5
#define LIST 6
#define LABEL 7
#define NO_LABEL_LIST 8
#define DOUBLECLICKBOX 9
#define MULTIFILENAME 10
#define TOGGLESTRING 11        // Toggle and get a string
#define TOGGLERGBCLICKBOX 12   // Toggle and get RGB value
#define SCROLLEDSTRING 13      // Get string using scrolled text widget
#define MULTISTRING 14         // Get several strings - text widget
#define MULTICLICKBOX 15       // Get 3 values from multiclickbox
#define MULTILABEL 16          // Add 3 labels
#define PUSHBUTTON 17          // Simple pushbutton
#define TOGGLEBUTTON 18        // Simple togglebutton
#define MULTIPUSHBUTTON 19     // Add 3 pushbuttons
#define MULTITOGGLE 20         // Numerous check boxes

////  Message types
// #define NONE 0
#define INFO 1
#define PROMPT 2
#define QUESTION 3
// ERROR 4
#define WARNING 5
#define WORKING 6
#define DONE 7
#define YESNOQUESTION 8
#define BUG 9

////  Palette dragging
#define SCRUNCHDOWN 1
#define ROTATE 2
#define SCRUNCHUP 3

////  List add item mode
#define BOTTOM 0
#define TOP 1

////  Filename and select-area mode
#define SINGLE 1
#define MULTIPLE 2
#define POLYGON 3
#define POINTTOPOINT 4

////  Scanner data modes
#define LOW 0
#define HIGH 1
#define CURRENT 2
#define DEVICE 3

////  Image color types
#define GRAY 0
#define INDEXED 1
#define COLOR 2
#define CMYK 3
#define GRAPH 4

////  Printer languages
#define POSTSCRIPT 0
#define PCL 1

////  Chromakey types
#define CHROMAKEY_NONE 0
#define CHROMAKEY_NORMAL 1
#define CHROMAKEY_EQUAL 2
#define CHROMAKEY_NOTEQUAL 3
#define CHROMAKEY_INVERT -1
#define ANTI_ALIAS 4
#define ANTI_ALIAS_INVERT 5

////  Multiclickbox types
#define COUNTER 0
#define SLIDER 1            // slider only
#define VALSLIDER 2         // slider + string field
#define BUTTONVALSLIDER 3   // slider + string field + button

////  Pattern types
#define GRAIN 0
#define PATTERN 1

////  Multiresolution/wavelet types
#define  PYRAMIDAL 1
#define  LAPLACIAN 2
#define  MALLAT 3
#define  MULTIRESOLUTION 4

////  For clickbox callbacks
#define ENTER_KEY   -2

////  Graph types
#define HISTOGRAM_GRAPH 1
#define GAMMA_GRAPH 2
#define STRIP_DENSITOMETRY_GRAPH 3
#define SIZE_DISTRIBUTION_GRAPH 4
#define SIGNAL_DISTRIBUTION_GRAPH 5
#define CONTRAST_GRAPH 6
#define INTENSITY_GRAPH 7
#define COLORMAP_GRAPH 8
#define TRACE_GRAPH 9
#define CURVE_DENSITOMETRY_GRAPH 10

////  Mask types
#define MASK 0
#define IMASK 1
#define ADDMASK 2
#define SUBMASK 3
#define MULMASK 4

////  Parsing types
#define FORMULA 0
#define TEX 1

////  Zoom modes
//none 0
#define ZOOM 1
#define MAGNIFY 2


//-----------Typedefs------------------------------//

typedef struct
{    uchar red;
     uchar green;
     uchar blue; 
}RGB;

typedef struct Taginfo
{   short uint id;
    short uint ttype;
    uint count;
    uint value;
}taginfo;   

typedef struct undostruct
{    uchar *u_1d;
     uchar **u;
     int x1;
     int y1;
     int x2;
     int y2;
     RGB *pal;
} Undostruct;

typedef struct Lineinfo
{   int ino;              // image no. to draw in, if constraining to image.
    Window window;        // Window to draw in
    int type;             // 0=line 1=ruler
    uint color;           // Color
    int width;            // Line width in pixels
    int wantgrad;         // 0=solid color 1=transverse color gradient 
    int gradr;            // red color gradient 
    int gradg;            // green color gradient 
    int gradb;            // blue color gradient 
    int gradi;            // grayscale gradient 
    int skew;             // Diagonal skew
    int perp_ends;        // 0=don't fix ends 1=make ends perpendicular
    int wantarrow;        // 0=line 1=add arrow head
    int outline;          // 0=line 1=outline
    uint outline_color;   // color of outline of arrow
    double arrow_width;      // Width of arrow
    double arrow_outer_length;  // Outer arrowhead length parallel to line
    double arrow_inner_length;  // Distance from arrowpoint to point at which arrow
                                //   joins the line
    double ruler_scale;
    double ruler_ticlength;
    int wantprint;        // Whether to draw in information box                             
    int antialias;        // 0=Bresenham 1=antialias algorithm
}lineinfo;            
                           // Generic data to be passed to callbacks.
                           // The meaning of each item can vary.
                           // Not all fields are always used.
typedef struct clickboxinfo 
{     int ino;             // Image no. relevant to operation
      int bpp;             // Bits/pixel if relevant
      char *title;         // The number
      int identifier;      // Number to distinguish in common callback
      char *filename;   
      char *path;
      char *dirmask;
      char *buttonlabel;   // For BUTTONVALSLIDER button label
      int answer;          // Generic answer for single clickbox
      double fanswer;      // Generic answer for single clickbox
      double factor;       // Generic scaling factor for fanswer
      int *answers;        // Generic array of answers for multiclickbox. Must
                           //   be a ptr so all individual clickboxes can see
                           //   each other's data. 
      int k;               // Index of which slider in multiclickbox 
      int button;          // Generic button no.
      int noofbuttons;     // No. of buttons in case it is multiclickbox
      int startval;
      double fstartval;
      int minval;
      int maxval;
      double fminval;
      double fmaxval;
      int decimalpoints;   // No. of decimal points to display ints as floats
      int type;            // Data type desired (INTEGER or FLOAT)
      int wantpreview;
      int allowedit;       // 0=cant change 1=ok to change 2=each line is command
      int ask;             // 0=dont ask user (eg for filename) 1=ask user
      int editrow;
      int editcol;
      int position;
      int maxstringsize;   // Maximum length of a string to edit
      Widget form;         // The parent form widget that called clickbox
      Widget listwidget;   // The child list widget of the clickbox if any
      int count;           // No. of items in list if any
      Widget widget[20];   // Generic list of Widgets to be changed by callback
      int param[100];      // Generic list of parameters to be changed by callback
      Widget *w;           // List of Widgets created by callback, to be destroyed
      int wc;              // No. of Widgets in list w to destroy
      char **list;         // List of strings 
      int listsize;        // No. of elements in list
      int helptopic;       // Help topic number
      void *ptr[20];       // Generic pointers for internal clickbox data
      void *client_data;   // Generic data passed to clickbox w/o change
      int done;            // Flag indicating done
      int wantdragcb;      // 1=f1,f2,f3 called while dragging mouse 0=called only
                           //    when value changes. Set to 0 if f1-f3 are slow.
                           //    (used for multiclickbox only)
                           // Functions for updating clickbox
      void(*f1)(int answer);       // Function to call with integer arg
      void(*f2)(double answer);    // Function to call with double arg
      void(*f3)(int answers[10]);  // Function to call with integer array arg
                                   // Function to call with integer + clickboxinfo*
      void(*f4)(int answer, clickboxinfo *c);  
      void(*f5)(clickboxinfo *c);  // Function to call when OK button is clicked
      void(*f6)(clickboxinfo *c);  // Function to call when Cancel button is clicked
      void(*f7)(clickboxinfo *c);  // Function to call 2nd when OK button is clicked
                                   // Function to call 3rd when OK button is 
                                   // clicked (in multiclickbox only)
      void(*f8)(Widget w, void *ptr1, void *ptr2);  
                                   // Function to call with string arg (for editor)
      void(*f9)(void *client_data, char *text);       
} Clickboxinfo;


typedef struct FormatSpecifier
{ 
                                 // Bytes Meaning
   char identifier[64];          // 64    String format identifier in file
   char headerfile[64];          // 64    File to copy from
   int  platform;                //  2    PC, MAC, or MVS
   int  packing;                 //  2    TIF, GIF, or NONE bit packing
   int  useheader;               //  2    1=copy some bytes from other file
   int  useidentifier;           //  2    0=use extension 1=use identifier
   int  headerbytes;             //  4    No.of bytes to copy
   int  skipbytes;               //  4    Fixed no.of bytes to skip reading image
   int  compressflag;            //  2    compression type (currently 0 or 1)
   int  datatype;                //  2    0=unsigned 1=signed

   int defaultbpp;               //  2    Default bpp
   int defaultcolortype;         //  2    Default color type
   int defaultxsize;             //  4    X size in pixels if not found
   int defaultysize;             //  4    Y size in pixels if not found
   int defaultrbpp;              //  2    red bpp if not found
   int defaultgbpp;              //  2    green bpp if not found
   int defaultbbpp;              //  2    blue bpp if not found
   int defaultkbpp;              //  2    black bpp if not found
   int defaultframes;            //  4    no. of frames if not found
   
                                 // Offsets in file - all pointers
                                 // must be 2 bytes for compatibility with 
                                 // other formats
                                 //  Bytes of item pointed to
                                 //  |
   short uint identifieroff;     // <64   Offset for identifier string
   short uint xoff;              //  2    Offset for x size 
   short uint yoff;              //  2    Offset for y size
   short uint bppoff;            //  2    Offset for total bpp
   short uint colortypeoff;      //  2    Offset for color type (GRAY,INDEXED, or COLOR)
   short uint compressflagoff;   //  2    Offset for flag (not used)
   short uint redbppoff;         //  2    Offset for red bpp
   short uint greenbppoff;       //  2    Offset for green bpp
   short uint bluebppoff;        //  2    Offset for blue bpp
   short uint blackbppoff;       //  2    Offset for black bpp
   short uint xposoff;           //  2    Offset for x position on screen
   short uint yposoff;           //  2    Offset for y position on screen
   short uint framesoff;         //  4    Offset for start of no. of frames
   short uint fpsoff;            //  2    Offset for frames/second
   short uint chromakeyoff;      //  2    Offset for chromakey flag
   short uint ck_grayminoff;     //  4    Offset for chromakey gray min
   short uint ck_graymaxoff;     //  4    Offset for chromakey gray max
   short uint ck_minoff;         //  4    Offset for chromakey RGB min
   short uint ck_maxoff;         //  4    Offset for chromakey RGB max
   short uint paletteoff;        // 768   Offset for start of palette
   short uint imageoff;          // var.  Offset for start of image data (all frames
                                 //    are stored sequentially). Image must be
                                 //    the last item. Can only be 2 bytes for 
                                 //    compatibility with .lum files.
}Format;



typedef struct Listinfo
{     char* title;         // Title of list box
      char** info;         // Contents of list (strings)
      int count;           // No. of items
      int itemstoshow;     // No. of items to show at a time
      int firstitem;       // 1st item to show
      int wantsort;        // 1=sort items before showing list
      int wantsave;        // 1=prompt to save list before closing
      int helptopic;       // Help topic 
      int *selection;      // Return for answer selected by user. This must
                           //   point to an actual variable, even if no selection
                           //   is wanted. (e.g.,  list.selection = &crud; )
      int allowedit;       // 1=allow user to change items in list
                           // The following items don't need to be specified if
                           //   allowedit is 0.
      char* edittitle;     // Title of menu box if user edits a line *
      int editcol;         // Left most column user is allowed to edit
      int editrow;         // 1st row that can be edited
      int width;           // Minimum width in pixels of dialog widget to create 
                           //  (If 0, width is calculated from 'info').
      Widget widgetlist[100];  // List of all widgets in list for deletion
      Widget form;         // Form widget containing list
      Widget widget;       // Generic widget
      Widget browser;      // Browser widget
      int transient;       // 1=list pops up  0=user must position the list
      int maxstringsize;   // Maximum allowable size for an 'info[]' item
      int wantfunction;    // 1=call function f1
                           // Function to call when click on list item (only
                           //    used if wantfunction is 1 or if click OK)
      void (*f1)(Listinfo *l);   // Called after Accept button

      void (*f2)(int listno);    // Update list (only used if autoupdate is 1)

      void (*f3)(Listinfo *l);   // Cancel button, before unmanaging list
                                 // Function to call to delete extra stuff 
                                 //    when finished (must be set to null() if 
                                 //    not needed)

      void (*f4)(Listinfo *l);   // Cancel button, after unmanaging list
                                 // Function to call to delete the list data

      void (*f5)(Listinfo *l);   // Called after Clear button

      void (*f6)(Listinfo *l);   // Function to call after cancel button, 
                                 // before doing anything else

      int autoupdate;      // 1=automatically update list after every command        
      int clearbutton;     // 1=add button to erase contents
      int highest_erased;  // No. of items erased by clearbutton
      int wc;              // widget count
      clickboxinfo *c;     // For passing clickboxes
      int param[100];      // For passing int parameters
      int *iptr[10];       // For passing int arrays
      double *dptr[10];    // For passing double arrays
      void *ptr[100];      // Generic pointers, see in dialoginfo for conventions
      int want_finish;     //
      int finished;        // Flag if listfinish has been called
      int busy;            // List is in use, don't delete it.
      int listnumber;      // Parameter for f6, to distinguish between lists
} listinfo;


#ifdef DIGITAL
#define DCOUNT 30
#define WCOUNT 4
#else
const int DCOUNT=30;             // No. of box or menu items
const int WCOUNT=4;              // No. of widgets per box      
#endif
typedef struct dialoginfo
{                                /* Select only 1 from each set             */
    char title[50];              /* Title across top                        */ 
    char radio[10][DCOUNT][100]; /* 10 sets of up to DCOUNT options length 100 */

    int radiono[10];             /* no.of actual radio groups (max=10)      */
    int radiotype[10][DCOUNT];   /* Input method for obtaining answer in a  */
                                 /* radio box (0=no answer, 1=message(),    */
                                 /* 2, clickbox().                          */
    int radioset[DCOUNT];        /* Radio box selection number              */
    int radiousexy;              /* 1=use radioxy for each group 0=automatic*/

    int boxtype[DCOUNT];         /* Character type to be returned in answer */
                                 /* box (0=string, 1=integer). If integer it*/
                                 /* uses clickbox to select the integer.    */
                                    
    int boxmin[DCOUNT];          /* Minimum value returnable in box         */
    int boxmax[DCOUNT];          /* Maximum value returnable in box         */
    int wantfunc[DCOUNT];        /* Flag if clickbox in answer box calls    */
                                 /*   a callback                            */ 
                                 /* For selecting parameters in dialog box  */
                                 /* 1 set of up to DCOUNT multiple choices  */
    char **boxlist[DCOUNT];      /* Multi-filename list for box             */
    int boxmenuselection[DCOUNT];/* Number of menu item selected.           */
                                 /*  boxmax[k] is the total no. of items.   */
    char boxes[DCOUNT][100];     /* or up to DCOUNT strings in boxes        */
    int boxset[DCOUNT];          /* Toggle box setting (0 or 1)             */
    int boxcount[DCOUNT];        /* Returns number of items in boxlist      */
    int boxusexy;                /* 1=use boxxy for each box 0=automatic    */

                                 /* If boxusexy or radiousexy is set, you   */
                                 /* must also set the following 8, and also */
                                 /* fill radioxy and/or boxxy.              */
    double spacing;              /* spacing in pixels between radio groups  */
    double width;                /* overall size of dialogbox form          */
    double height;
    double radiostart;
    double radiowidth;
    double boxstart;
    double boxwidth;
    double labelstart;
    double labelwidth;
    
    double radioxy[10][4];       /* x,y,width,height coords for radio       */
    double boxxy[DCOUNT][4];     /* x,y,width,height coords for box         */
 
    int noofradios;              /* no. of radio groups                     */
    int noofboxes;               /* no. of string boxes + check boxes       */

                                 /* Prior value of most recently-changed    */
                                 /* string if non-zero                      */
    char lastanswer[DCOUNT][100];
                                 /* Corresponding strings returned from box */
                                 /* This has to be >= 2 chars longer than   */
                                 /*  the maxlength used in 'message'.       */
    char answer[DCOUNT][WCOUNT][FILENAMELENGTH]; 
    int wcount[DCOUNT];          /* No. of widgets in multi*box, max=WCOUNT */
    char path[FILENAMELENGTH];   /* Path to start in if getting filename    */
    int helptopic;               /* ID # of associated help topic           */
    int startcol[DCOUNT];        /* Leftmost allowable editing position     */
    Widget radiowidget[10][DCOUNT];  /* Radio button Widget (needed in callback)  */ 
    Widget boxwidget[DCOUNT][WCOUNT];/* Max of WCOUNT widgets per line (eg, r,g,b */
                                 /* or a toggle button + string, or 4 strings)    */
                                
    int transient;               /* Whether to make window transient        */
    int wantraise;               /* Make the dialog box stay on top         */
    int bpp;                     /* Bits/pixel to use in multiclickbox      */
    int want_changecicb;         /* 1=call dialogchangecicb when changing ci*/
    clickboxinfo *c[DCOUNT];     /* clickboxinfo struct needed in callbacks */             

    /* All list params in dialoglists except info, wantfunction, f1, f2, f4 
       are set automatically by dialog box and should not be set by client. */

    listinfo *l[DCOUNT];         /* lists/menus for each box                */    
    int list_visible[DCOUNT];    /* 1=list is already visible, don't open   */
    Widget *widget;              /* Array of widgets to delete              */
    Widget form;                 /* Form widget to delete                   */
    int widgetcount;             /* no. of widgets on form                  */
    clickboxinfo *clickboxdata;  /* Array of clickboxinfo structs           */

                                 /* Function to call when accept button is clicked */
    void (*f1)(dialoginfo *a, int radio, int box, int boxbutton);                                 

                                 /* User function to call when accept button is clicked */
    void (*f2)(dialoginfo *a, int radio, int box, int boxbutton);                                 

                                 /* Function to call when finished          */
    void (*f3)(dialoginfo *a);   /* (extra activities before closing dialog)*/

                                 /* Function to call when finished          */ 
    void (*f4)(dialoginfo *a);   /* (for releasing memory)                  */

              /* These 4 only need to be set if dialog has a multiclickbox  */
              /* Func to call when dialog clickbox OK button is clicked     */
    void(*f5)(clickboxinfo *c);  

              /* Func to call when dialog clickbox Cancel button is clicked */
    void(*f6)(clickboxinfo *c);  

              /* Func to call 2nd when dialog clickbox OK button is clicked */
    void(*f7)(clickboxinfo *c);  

              /* Func to call 3nd when dialog clickbox OK button is clicked */
    void(*f8)(Widget w, void *ptr1, void *ptr2);  

                                 /* Function to call when cancel button is clicked */
                                 /* for preliminary stuff, e.g. setting busy to 0 */
    void (*f9)(dialoginfo *a); 

    void *radiocb[10];           /* Callback for RADIOPUSHBUTTONs           */

                                 /* Labels for RADIOPUSHBUTTONs             */
    char boxpushbuttonlabel[DCOUNT][WCOUNT][100];
    char radiopushbuttonlabel[10][100];
    void *radiopushbuttonptr[10];/* Data to send to RADIOPUSHBUTTONs        */
    /*
       Callers must adhere to the following conventions in clickboxes and
       dialogboxes:                                 
       client_data = data passed unchanged to f5 and f6 in clickbox (Must be static)  
       ptr[0] = dialogboxinfo*                                            
       ptr[1] = char*             for strings                             
       ptr[2] = int*              for single variables                    
       ptr[3] = &double           for single variables                    
       ptr[4] = XYData*           for data                                
       ptr[5] = int[]             for int arrays                          
       ptr[6] = &int              for single int parameter                
       ptr[7] = &int              for int want_block                      
       ptr[8] = palettecbstruct*                                          
       ptr[9] = Pixmap                                                    
       ptr[10] = PlotData*                                                
       ptr[11] = listinfo*        for lists                               
       ptr[12] = filter*          for filters                             
       ptr[13] = char**           for image arrays
       ptr[14] = char*            for image arrays      
       ptr[15] = FILE*            for file pointers
    */
    void *ptr[100];              /* Array of general ptrs for user stuff    */
    int hit;                     /* Flag if callback has ever been used     */
    Widget w[DCOUNT];            /* Generic widgets for callbacks           */

    /* 
        param[0] and param[1] are used internally in dialogbox(). 
        All others can be used by caller.
    */
    int param[100];              /* Generic parameters for use by f1 and f2 */ 

    char message[1024];          /* String 1 to be placed on the dialog     */
    int message_x1;              /* Location of message 1                   */
    int message_x2;              /* Location of message 1                   */

    /* use  dialog_message(message_widget, char*) to set label              */
    Widget message_widget;       /* Widget for message string 1             */
    char message2[1024];         /* String 2 to be placed on the dialog     */
    int message_y1;              /* Location of message 2                   */
    int message_y2;              /* Location of message 2                   */
    Widget message_widget2;      /* Widget for message string 2             */
                                 /* Set after calling dialogbox()           */
    int busy;                    /* prevents cancel button from closing dialog */
} Dialog;


typedef struct ButtonStruct
{   int button;
    int state;
    Widget widget;
    char *label;
    char *activate_cmd;
    char *deactivate_cmd;
} buttonStruct;


typedef struct palettecbstruct
{  int top;
   int center;
   int bottom;
   int button;
   Window win;
   int dragging;
} PaletteCBStruct;


typedef struct PaletteCallbackStruct
{  int x;
   int y;
   int focus;
} pcb_struct;


typedef struct PrinterStruct
{   char command[256];
    char filename[FILENAMELENGTH];      // Print file name
    char devicename[FILENAMELENGTH];    // Printer device name
    int copies;
    int depletion;
    int device;         // 1=command 2=device 3=file
    int dither;         // No. of printer pixels/screen pixel
    int entire;         // 1=printing an entire image
    int graybalance;
    int intensity;
    int interpolate;
    int language;       // 0=postscript 1=pcl
    int palette;        // 1=B/W, 2=RGB, 3=CMY, 4=CMYK
    int papertype;      // media type
    int positive;
    int quality;
    int res;            // dpi
    int rotation;
    int vertical;
    double bfac;
    double gfac;
    double hpos;        // horiz offset inches
    double hsize;       // paper horizontal size
    double ifac;
    double paperxsize;
    double paperysize;
    double ratio;
    double rfac;
    double vpos;        // vertical offset inches
    double vsize;       // paper vertical size
    double xsize;       // bounding box horiz size inches
    double ysize;       // bounding box vert size inches
} printerStruct;


typedef struct XYDataInfo
{   int *x;                      // x coordinates
    int *y;                      // y coordinates
    int *x1;                     // x coordinates
    int *y1;                     // y coordinates
    int *x2;                     // x coordinates
    int *y2;                     // y coordinates
    double *u;                   // uncalibrated value for x,y
    double *v;                   // calibrated value for x,y
    double *r;                   // radius for x,y
    double *theta;               // angle for x,y
    int ino;                     // image number
    int dims;                    // no. of dimensions
    int n;                       // no. of data points so far (should start out at 0)
    int nmin;                    // min. no. of data points or 0 for any number
    int nmax;                    // max. no. of data points
    int width;                   // width for rectangles
    int type;                    // type of curve to use (see bezier_curve_start())
    int duration;                // TEMP or PERM
    int wantpause;               // Allow user to wait for keypress while showing area
    int order;                   // Regression order for lin. reg. lines
    Window win;                  // Window to plot the data on (0=drawing_area or
                                 //    image, calculated automatically).
    char **label;                // String for each data point
    char **label2;               // String for each data point
    char *title;                 // Title or filename
    int param[10];               // Misc. parameters
    int wantmessage;             // 1=message box for user
} XYData;


/*  
    To open an updatable graph, declare a PlotData* and call open_graph().
    Don't allocate or delete PlotData. See curve_densitometry for example.
    Then call graph_is_open(), if !=0 reset pd->n, deleta and resize 
    pd->data (using new), copy the new data, and call redraw_graph. Graph 
    is automatically closed and all internal data are automatically deleted 
    when user clicks Cancel.
*/
     
typedef struct PlotDataInfo
{   Widget *graph;               // Array of widgets, one for each graph
    Widget *w;                   // Array of all widgets for deleting
    Widget *but;                 // Array of all buttons if any
    Widget form;                 // Form Widget of graph window
    Window *win;                 // Array of Xlib windows for graph, i.e. win[dims]
    Window formwin;              // Xlib window for form widget the Window is on
    Window *ylabelwindow;        // Y axis label window for labels
    Window xlabelwindow;         // X axis label window for labels
    char *title;                 // Graph title
    char *xtitle;                // x axis title
    char **ytitle;               // y axis titles for each graph
    double **data;               // 2d y data array
    double *xdata;               // 1d x data array
    int  *edata;                 // extra data array
    int  *fdata;                 // extra data array
    int  *gdata;                 // extra data array
    int dims;                    // No. of dimensions in 'data'
    int x1,y1,x2,y2;
    int xsize;                   // x size of entire graph form in pixels
    int ysize;                   // y size of entire graph form in pixels
    int n;                       // Current no. of data points
    int nmax;                    // Maximum no. of data points
    int focus;                   // Which dimension in 'data' to plot
    int wc;                      // Widget count
    double xcalib;
    double ycalib;
    double *scale;               // Y scale[dims] to fit graph vertically in window
    double xscale;               // User-defined x scale applied to all dims
    double yscale;               // User-defined y scale applied to all dims
    double xdisplayfac;          // Factor to multiply x text display value only
    double ydisplayfac;          // Factor to multiply y text display value only
    double *area;                // Most recently-selected area 
    double ymin, ymax;           // Min & max expected on graph
    double llimit, ulimit;       // Lower & upper limit (0=dont care)
    int button;                  // Mouse button currently clicked
    int *markmin;                // Lowest x value that needs to be redrawn
    int *markmax;                // Highest x value that needs to be redrawn
    int *markstart;              // X start of marked region 
    int *markend;                // X end of marked region
    int type;                    // MEASURE, CHANGE, SUBTRACT, BEZIER
    int otype;                   // MEASURE, CHANGE, SUBTRACT, BEZIER
    int helptopic;               // Topic in help file
    void (*f)(pcb_struct v);     // Callback called when dragging mouse                                 
    void (*f2)(void *pd, int n1, int n2);  // What to do when redrawing graph                                 
    void (*f3)(void *pd, int n1, int n2);  // Callback for maximize button (must set pd->min & pd->max)
    void (*f4)(void *pd, int n1, int n2);  // Callback for save button 
    void (*f5)(void *pd, int n1, int n2);  // Callback for finished button
    int usef2;                   // 1=f2 is valid function
    int status;                  // Return status
    int gcolor;                  // Graph drawing color
    int reason;                  // NORMAL, EXPOSE
    clickboxinfo *c;             // Array of clickboxdata structs
    pcb_struct *v;               // pcb for adjusting params
    listinfo *l;                 // Listbox
    int gotlist;                 // 1=listbox allocated
    int hit;                     // Flag for first time button pressed
    XYData *xydata;              // For baseline data
} PlotData;

typedef struct GraphInfo
{   int ino;
    int type;
    PlotData *pd;
}Graph;


typedef struct Wavelet
{   char filename[FILENAMELENGTH];
    double filter[2][1024];
    double hifilter[2][1024];
    int size;
    int isize;
    int jsize;
    int ioffset;
    int joffset;
} WaveletStruct;

typedef struct WaveletParams
{  
   Wavelet wave;
   char format[128];
   int direction;
   int ino;
   int want_gray_offset;
   int gray_offset;
   int imin;
   int imax;
   int levels;
   int minres;
   double cmin; 
   double cmax;
   double ignmin;
   double ignmax;
   int ntabs;  
} WaveletParamStruct;



typedef struct ScannerSettings
{
    char scanner_name[FILENAMELENGTH];     // Scanner filename in scanner list
    char scan_device[FILENAMELENGTH];      // Scanner name to operating system
    int xres;                    // x dpi
    int yres;                    // y dpi
    int xpos;                    // x root coordinate for image
    int ypos;                    // y root coordinate for image
    int x;                       // x starting coordinate to scan in 1/720ths of inch
    int y;                       // y starting coordinate to scan in 1/720ths of inch
    int w;                       // width to scan in 1/720ths of inch
    int h;                       // height to scan in 1/720ths of inch
    int brightness;              // brightness
    int contrast;                // contrast
    int bpp;                     // bits/pixel to scan
    int hit;                     // flag whether struct was filled yet
    int color;                   // 0=grayscale 1=RGB 
    int preview;                 // 0=full resolution scan 1=preview scan
    int ino;                     // Image no. associated with scan
    int shell;                   // 1=put new image in separate window
    int scantype;                // desired bpp and color mode
    int mirror;                  // mirror image
    int border;                  // 1=give new image a border
    Widget widget;               // widget to put image in; 0=create new Widget
    int scannerid;               // Which scanner to use
} scannerSettings; 

typedef struct CameraSettings
{
    uchar *buffer_1d;            // Temporary buffer for camera image
    uchar **buffer;              // Temporary buffer for camera image
    char name[FILENAMELENGTH];             // Camera name
    char defaults[FILENAMELENGTH];         // Default settings (e.g., .cqcrc)
    char post_processing_command[FILENAMELENGTH]; // e.g., filter(1,1,20) to sharpen
    int command_is_modified;     // 1=post processing command was changed by user
    int w;                       // width 
    int h;                       // height
    int scale;                   // scale factor
    int fps;                     // Frames per second
    int brightness;              // brightness
    int contrast;
    int black;                   // black level
    int white;                   // white level
    int hue;                     // hue
    int saturation; 
    Widget widget;               // widget to put image in; 0=create new Widget
    Dialog *dialog;              // For sending dialog settings to acquire button
    int camera_id;
    int depth;
    char **args;
    int argno;
    int hit;
    int ino;
    int bpp;
    int state;                   // can be NORMAL, PAUSE, or INITIALIZING
    int shell;                   // 1=image in separate shell
    int acq_mode;                // Replace, average, add, or subtract successive frames
    int frames;                  // Total no. of frames so far
    int continuous;              // 0=single frame 1=continuous video
    int want_processing;         // 0=dont process  1=execute post_proc_string
} cameraSettings; 


typedef struct ScanParams        // For Densitometry
{   char filename[FILENAMELENGTH];
    int ino;                     // image number if applicable
    int autosave;                // 1=Automatically save scan data
    int count;                   // No. of measurements made so far 
    int leavemarked;             // 1=Leave area marked after densitometry
    int invert;                  // 1=max. signal = black
    int gamma_inverted;          // 1=gamma table is highest for pixel=0
    int pixels;                  // Total no. of pixels
    int fpixels;                 // Pixels in spot
    int selection_method;        // Which method for getting coordinates
    int scanwidth;
    int snapthreshold;
    int compensate;              // 1=User wants pixel compensation
    int wantfixedwidth;
    int wantrepeat;
    int wantpause;               // 1=Pause after showing boxes (coordinates)
    int wantplot;                // 1=plot scan in graph
    int wantcolor;               // 1=scan in color 0=grayscale
    int wantbox;
    int autobkg;                 // 1=automatically calculate background each time
    int od[256];                 // Gamma table
    int skip;                    // For use by other routines, which want to skip
                                 //   certain calculations
    int bpp;                     // Bits/pixel to convert result to
    int diameter;                // Diameter for circle select area
    int color;                   // Color for labels
    Graph graph;                 // Graph struct 
    double *scan;                // Array to put results
    double bdensity;             // Background color 0..1
    double bsignal;              // Total background signal
    double fdensity;             // Spot density 0..1
    double fsignal;              // Total signal of spot
    double netsignal;            // Total spot - background signal
    double netdensity;           // Total spot - background density
    double calfac;               // Calibration factor
} scanParams;


typedef struct Spreadsheet
{
    Widget   sswidget;                 // Xbae widget
    Widget   ssform;                   // form widget (parent of Xbae widget)
    Window   ssform_window;            // Window of form widget
    Widget   radiowidget[10][DCOUNT];  // List of <=10 groups of radio button widgets
    Widget  *widget;                   // Array of widgets to delete 
    int widgetcount;                   // no. of widgets to delete
    char ***text;                      // User-entered text for each cell
    uchar  **selected;                 // 1=cell is selected 0=not selected
    uchar   *selected_1d;              //
    uchar  **number;                   // 1=cell contains a number (not used)
    uchar   *number_1d;                //
    uchar   *column_selected;          // 1=column was selected
    int     *maxrow;                   // Highest non-blank row in each column 
    int      maxcol;                   // Highest non-blank column
    int      current_row;              // Cursor row
    int      current_col;              // Cursor column
    int      rows;                     // Total no. of rows
    int      cols;                     // Total no. of cols   
    int      visible;                  // 1 = spreadsheet data is allocated 
    int      created;                  // 1 = spreadsheet Widget has been created 
    int      number_format;            // Number format (int, hex, rgb)
    int      display;                  // What to display (image, real, imag)
    int      radiono[10];              // No. of radio buttons in each group
    int      noofradios;               // No. of radio button groups
    int      ino;                      // Image no. in spreadsheet
} spreadSheet;


typedef struct XWDStruct
{      int header_size;
       int file_version;       
       int pixmap_format;      
       int pixmap_depth;       
       int pixmap_width;       
       int pixmap_height;      
       int xoffset;            
       int byte_order;         
       int bitmap_unit;
       int bitmap_bit_order;   
       int bitmap_pad;
       int bits_per_pixel;     
       int bytes_per_line;
       int visual_class;       
       int red_mask;           
       int green_mask;         
       int blue_mask;          
       int bits_per_rgb;       
       int colormap_entries;   
       int ncolors;           
       int window_width;      
       int window_height;     
       int window_x;          
       int window_y;          
       int window_bdrwidth; 
}XWDHeader;
typedef struct X11ColorMap
{      uint entry_number;
       short uint red;
       short uint green;
       short uint blue;
       char flags;
       char padding;
}XWDColorMap;


typedef struct Pattern
{      int ino;             // Image no. pattern is confined to
       int xsize;           // Width of pattern
       int ysize;           // Height of pattern
       int bpp;             // Bits/pixel of pattern
       int colortype;
       uint *pat_1d;
       uint **pat;
} PatternStruct;

typedef struct Tiftag
{   int tag;
    int length;
    int type;
    int value;
} TifTag;

class BinomialFilter
{
  private:
    int length;
    double *coefficient;
  public:
  	BinomialFilter( int ntabs );
  	BinomialFilter( const BinomialFilter& );
        BinomialFilter& operator = ( const BinomialFilter& );
  	~BinomialFilter(){  delete[] coefficient; }
    int tabs( void ) const
    {
    	return length;
    }
    const double *data( void ) const
    {
      return (const double *)(coefficient);
    }
};

typedef struct p3d
{ 
   Widget w[100];             // Widgets to be deleted
   int answer[100];
   int buttonstate[100];
   Widget slider[20];
   Widget buttonwidget[20];
   Widget labelwidget[20];
   Widget text[20];
   Widget miscwidget[20];
   clickboxinfo *clickboxdata[20];
   clickboxinfo *c;
   int wc;                    // Widget count
   int clickboxcount;
   int param[20];
   Widget form;
   Widget parent;
   void *ptr[20];             // generic array of pointers
   void *client_data;         // Generic data passed to clickbox w/o change
};


typedef struct Filter
{   char filename[FILENAMELENGTH];
    int ino;                  // Image no.
    int type;                 // Filter type
    int x1,y1,x2,y2;          // Bounds for filtering
    int ksize;                // Kernel size
    int diffsize;             // For spatial differencing filter
    int kmult;                // Multiple of kernel size
    int sharpfac;             // 0=100% old values 1=100% new filtered values
    int range;                // For noise filter
    int maxbkg;               // 0=black 1=white
    int entireimage;          // 1=filtering an entire image (for speed optimization)
    int ithresh;              // For engrave filter
    int want_progress_bar;    // 1=progress bar while filtering
    int local_scale;          // Distance to scan in local contrast max filter
    int do_filtering;         // 0=just set up 1=actually filter
    int background;           // Desired background value (force bkg filter)
    double decay;
} filter;


typedef struct PromptStruct
{   char   *filename;
    void   *ptr;
    int    helptopic;
    int    n;
    int    ino;
    int    maxsize;
    int    *params;
    double *dparams;

    void   *ptr1;
    char   *text;
    int    *data;
    int    *intptr;
    double *fdata;

    void   *ptr2[20];
    char   *text2[20];
    int    *data2[20];
    double *fdata2[20];

    void   **ptr3[20];
    char   **text3[20];
    int    **data3[20];
    double **fdata3[20];
    void (*f1)(PromptStruct *ps);  // Function to call if yes
    void (*f2)(PromptStruct *ps);  // Function to call if no
    Widget widget[20];
};



#ifdef DIGITAL
#define INFOWINDOWS 4
#else
const int INFOWINDOWS=4;
#endif
typedef struct Globals
{
    XtAppContext app; 
    Widget main_widget;
    Widget drawing_area;            // Area for images
    Widget drawing_area2;           // Area at left
    Widget info_area[INFOWINDOWS];  // Information areas at left
    Widget title_area;              // Information area at bottom
    Widget menu;
    Widget menubar;
    Widget menupane[30];
    listinfo    **openlist;         // Array of list data to update
    Display      *display;          // Connection to X display  
    GC            gc;               // The graphics context for main
    GC            image_gc;         // Graphics context for all images
    GC            xor_gc;           // GC for xor'ing
    XEvent        event;            // Structure for current event
    Window        main_window;      // Application's window
    Window        root_window;      // Root window
    Window        info_window[INFOWINDOWS];   // The windows at left for information
    Visual       *visual;
    int           visual_id_wanted;           
    int           visual_bpp_wanted;
    int           ximage_byte_order;          // LSBFirst, MSBFirst
    Pixmap        spixmap;                    // Pixmap for drawing text on theWindow
    Pixmap        mpixmap[INFOWINDOWS];       // Pixmap for drawing text on theMain
    Colormap      colormap;
    Colormap      xcolormap;
    XColor        def_colors[256];            // Pre-existing color values in server
    int           screen;
    char         *appname;                    // Application's name
    XFontStruct  *fontstruct;                 // Default font
    XFontStruct  *image_font_struct;          // User-selected font
    XmFontList    fontlist;                   // Font list for Motif 1.2 and below
    int font_selection;                       // Entry in font selector list
    int xlib_cursor;
    char *font;
    char *imagefont;
    XImage *stringbufr_ximage;         // XImage form of stringbufr for Xlib
    Cursor fleur_cursor, normal_cursor, busy_cursor, cursor, no_cursor,
        up_cursor, down_cursor, left_cursor, right_cursor, grab_cursor;
    Dimension mainmenuheight;
    lineinfo line;                     // Line width, etc
    int cursor_symbol;
    int drawarea2width;
    double  variable[100];              // variables
    double  luminr,luming,luminb;      // for luminosity calculations
    double  calib[1000];               // gel calibration
    double  fswap_temp;                // Temporary double for swap macro
    double  tif_xsize;
    double  tif_ysize;
    double  dswap_temp;                // Temporary double for swap macro
    double *scan;                      // Data from scanning a trapezoidal region
                                       // Length is max. no. of x pixels expected
    int image_stack[MAXIMAGES];        // List of images from front to back
    char   version[100];
    char   formatpath[FILENAMELENGTH];
    char   waveletpath[FILENAMELENGTH];
    char   nomemory[FILENAMELENGTH];
    char   startdir[FILENAMELENGTH];             // Start-up directory to return to
    char   currentdir[FILENAMELENGTH];           // Current directory
    char   homedir[FILENAMELENGTH];              // $HOME/.tnimage directory for tnimage.ini
    char   helpdir[FILENAMELENGTH];              // Location of tnimage.hlp if changed
    char   fontpath[FILENAMELENGTH];

    char   scan_device[FILENAMELENGTH];          // Scanner device name
    char   camera[FILENAMELENGTH];               // Scanner driver path
    char   *macrotext;
    char   *sketchtext;
    Widget sketch_editor_widget;
    char   emailaddress[1024];         // Where to send all those bug reports to
    char   compression[1024];          // Program to exec to compress image
    char   decompression[1024];        // Program to exec to decompress image
    char   compression_ext[1024];      // Extension added by compression program
    char   decimal_point;              // What char to use for decimal points

    RGB    fc, bc;                     //
    RGB    palette[256];               // current palette
    RGB    b_palette[256];             // Actual bkg palette
    RGB    fc_palette[256];            // Current false-color palette (starts out as grayscale)
    RGB    gray_palette[256];          // Grayscale palette (treated as static)
    uchar  **stringbufr;               // Image buffer for strings
    uchar  *stringbufr_1d;             // 1d alias for stringbufr
    uchar  **region;                   // Selected area (for crawling dots)
    uchar  *region_1d;                 // 1d alias for selected area
    int    region_ino;                 // Image no. for which region is valid
    int    torn_off[100];

    uint fcolor, bcolor, bkg_image_color, maxcolor, main_bcolor, main_fcolor;
    uint main_mask, mouse_mask;
    int    crud; 
    int    block;                      // Blocking is active so don't close parent
    int    block_okay;                 // Overrides the effects of g.block
    int    ignore;                     // Signal to ignore all new events
    int    waiting;                    // Flag if waiting for input
    int    moving;
    int    want_slew;
    int    want_raise;
    int    want_switching;             // 1=allow changing images
    int    total_xsize;                // X size of theWindow
    int    total_ysize;                // Y size of theWindow
    int    main_xsize;                 // X size of theMain
    int    main_ysize;                 // Y size of theMain
    int    main_xpos;                  // Upper left corner of theWindow
    int    main_ypos;                  // Upper left corner of theWindow
    int    getout;                     // Flag for 'cancel'
    int    escape;                     // Flag for 'cancel'
    int    getpointstate, getboxstate, getlinestate, getlabelstate;
    double maxvalue[65];
    int    redbpp[65];
    int    greenbpp[65];
    int    bluebpp[65];
    int    blackbpp[65];
    int    off[65];
    int    maxred[65];
    int    maxgreen[65];
    int    maxblue[65];
    int    maxgray[65];
    int    want_redbpp;
    int    want_greenbpp;
    int    want_bluebpp;
    int    want_blackbpp;
    int    want_noofcolors;
    int    want_colormaps;
    int    want_byte_order;            // -1 = use default, other values = set byte order
    int    bitsperpixel;
    int    colortype;
    int    sparse_packing;             // 1 = X windows sparse pseudo 24bpp mode
    int    sparse;                     // 1 = force 32 bpp instead of 24 bpp
    int    nosparse;                   // 1 = force 24 bpp instead of 32 bpp
    int    autoundo;                   // 0=never 1=manual 2=automatic 
    int    imode;                      // Pixel interaction mode
    int    want_messages;              // 0=none 1=minimal 2=all
    int    draw_figure;
    int    sprayfac;
    int    diameter;
    int    cross_length;
    int    wantr;
    int    wantg;
    int    wantb;
    int    mouse_button;
    int    last_mouse_button;
    int    mouse_x;
    int    mouse_y;
    int    key_ascii;
    int    key_alt;
    int    key_ctrl;
    int    key_shift;
    int    reserved[256];              // Color values in palette to leave untouched

    int    image_count;                // Total number of images. Background is also
                                       // an image so  image_count==2 = 1 user image 
                                       // + 1 background image)
    int    imageatcursor;              // Image no. of pixel at cursor pos.
    int    text_direction;
    double text_angle;
    int    want_format;                // Overrides default save image format unless -1.
                                       // (Allows macro to specify file format on a
                                       // separate line)
    int    read_cmyk;                  // ==1 if image is to be interpreted as a cmyk
                                       // ==0 if upper 8 bits are to be thrown away
    int    save_cmyk;                  // ==1 if create cmyk image
    int    want_quantize;              // ==1 if quantize,2=fit current palette 0=none
    int    jpeg_qfac;
    int    useheader;                  // for custom image files
    int    swap_temp;                  // Temporary int for swap macro
    int    xres;
    int    yres;
    int    signif;                     // Significant digits to display
    int    state;                      // Program button-pressing state
    int    busy;                       // 1=acquiring data, don't delete image
    int    bezier_state;               // CURVE=drawing bezier (event handler active)
    int    selectcurve;                // 1=selecting a non-rectangular region
    int    inmenu;                     // If in a menu, dialog box, sample palette,
                                       // etc. prevents putting pixels in
                                       // the current image buffer.
    int    indialog;                   // 1=a dialog box with changecicb is showing
    int    tif_positive;
    int    tif_xoffset;
    int    tif_yoffset;
    int    tif_skipbytes;              // No. of extra junk bytes to skip at start
    int    currentpalette;             // Default starting palette no.
    int    bkgpalette;                 // Palette of the background
    int    background_touched;         // Flag if pixels were set on background
    int    csize;                      // Amount of cursor movement
    int    selected_ulx;               // Selected region in screen coordinates
    int    selected_uly;               // Selected region in screen coordinates
    int    selected_lrx;               // Selected region in screen coordinates
    int    selected_lry;               // Selected region in screen coordinates
    int    get_x1;
    int    get_y1;
    int    get_x2;
    int    get_y2;
    int    ulx;                        // Selected region in image coordinates
    int    uly;                        // Selected region in image coordinates 
    int    lrx;                        // Selected region in image coordinates
    int    lry;                        // Selected region in image coordinates
    int    region_ulx;                 // Selected region in image coordinates
    int    region_uly;                 // Selected irregular region in image coordinates 
    int    region_lrx;                 // Selected irregular region in image coordinates
    int    region_lry;                 // Selected irregular region in image coordinates
    int   *highest;                    // Array of y values for each x value, used by
                                       //  'trace'and 'bezier' for getting data.
    int    scancount;
    int    want_color_type;
    int    want_bpp;
    int    selectedimage;              // Most recently-selected image
    int    selected_is_square;         // 1 = selected area is rectangular
    int    usegrayscale;
    int    false_color;                // 1 = false color map in effect
    int    invertbytes;
    int    want_shell;                 // 1 = each image in separate frame
    int    window_border;              // 1 = windows get complete frame
    int    want_title;                 // 1 = title above image
    int    wm_topborderwidth;          // Top border provided by window manager
    int    wm_leftborderwidth;         // Left border provided by window manager
    int    window_handle_size;         // Active area for grabbing windows
    int    nbuttons;                   // No. of user-defined buttons at left
    ButtonStruct **button;             // User-defined buttons at left
    int    zoom_button;                // Which of above buttons controls zoom
    int    spacing;                    // Spacing between dots in selectbox
    int    diagnose;                   // 1=diagnostic mode
    int    text_spacing;               // No. of pixels between letters in text
    int    openlistcount;              // No. of list widgets currently visible that
                                       //    want automatic updates
    int    graphcount;                 // No. of graphs currently visible that
                                       //    want automatic updates
    Graph *graph;                      // Graphs to update (Graph structs)
    int    scannerid;                  // No. in list of scanners to use
    int    camera_id;                  // No. in list of cameras to use
    RGB    object_threshold;           // Pixel range to be considered same object
    int    double_click_msec;          // Time in msec below which click is double click
    int    copy;                       // 1=copy 0=move
    int    want_crosshairs;            // 1=full screen crosshairs at cursor
    int    swap_red_blue;              // 0=RGB 1=BGR byte order in default visual
    int    area_selection_mode;        // 0=SINGLE 1=MULTIPLE 2=POLYGON in scissors
    
    /* Global defaults for dialog box entries, needed because dialog boxes 
       don't block waiting for input */

    ScannerSettings sp;
    CameraSettings cam;
    int    filter_type;
    int    filter_ksize;
    int    filter_maxbkg;
    int    filter_range;
    int    filter_kmult; 
    int    filter_sharpfac;
    char   filter_file[FILENAMELENGTH];
    int    filter_diffsize;
    int    filter_ithresh;
    int    filter_local_scale;
    double filter_decay;
    int    filter_background;
  
    int    measure_angle;
    int    trace_wantsave;
    int    trace_wantplot;
    int    trace_wantslow;
    int    trace_trackcolor;

    int    mask_mode;
    int    mask_ino1;
    int    mask_ino2;
    int    mask_xoffset;
    int    mask_yoffset;
    int    rotate_anti_alias;
    double rotate_degrees;
    double rotate_label_degrees;

    uint   fill_color;
    uint   fill_mingrad;
    uint   fill_maxgrad;
    uint   fill_minborder;
    uint   fill_maxborder;
    int    fill_type;
    int    fill_usermode;
    
    int    read_raw;
    int    read_grayscale;
    int    read_convert;
    int    read_skipswitch;
    int    read_swap_bytes;
    int    read_signedpixels;
    int    read_split_frames;

    int    paste_set0;
    int    spray_mode;
    
    int    create_method;   
    int    create_border;
    int    create_xsize;
    int    create_ysize;
    int    create_xpos;
    int    create_ypos;
    int    create_frames;
    int    create_cimage;
    int    create_shell;
    int    create_auto_edge;
    int    create_find_edge;
    int    create_rows;
    int    create_cols;
    int    create_panel;
    int    create_xspacing;
    int    create_yspacing;
    int    create_ulx;
    int    create_uly;
    int    create_lrx;
    int    create_lry;
    int    create_fixed_xsize;
    int    create_want_fixed_xsize;
    int    create_fixed_ysize;
    int    create_want_fixed_ysize;
    int    create_xmargin;
    int    create_ymargin;
    
    int    pattern_method;
    int    pattern_wantlabels;
    int    pattern_wantrectangles;
    int    pattern_wantgraph;
    int    pattern_wantdensgraph;
    int    pattern_wantsave;
    int    pattern_labelcolor;
    int    pattern_size;                
    int    pattern_minsize;                
    int    pattern_maxsize;                
    int    pattern_labelflags;                
    double pattern_thresh;
    double pattern_pweight;    
    double pattern_bweight;
    char   pattern_filename[FILENAMELENGTH];
    char   pattern_datafile[FILENAMELENGTH];
        
    int    wavelet_type;
    int    wavelet_grid;
    int    wavelet_maxlevels;
    char   wavelet_file[1024];
    char   wavelet_index_range[80];
    char   wavelet_coeff_range[80];
    char   wavelet_ignore_range[80];
    WaveletParams wp;

    int    morph_maxsignal;  // black
    int    morph_type;       // grayscale
    int    morph_se;         // 1=4 2=8 conn set
    int    morph_operation;
    double morph_thresh;
    int    morph_ksize;      // watershed kernel
    int    morph_contour_separation;

    int    warp_gridpoints;
    int    warp_cursor;
    int    calib_type;
    int    attributes_set0;
    int    attributes_isource;
    int    attributes_idest;
    int    raw_platform;
    int    raw_colortype;
    int    raw_xsize;
    int    raw_ysize;
    int    raw_skip;
    int    raw_bpp;
    int    raw_frames;
    int    raw_rbpp;
    int    raw_gbpp;
    int    raw_bbpp;
    int    raw_kbpp;
    int    raw_packing;
    int    raw_want_increment;
    int    raw_want_color;
    int    raw_inc_inc;
    int    raw_flipbits;
    int    raw_signed;
    char   raw_filename[FILENAMELENGTH];
    char   raw_filename2[FILENAMELENGTH];

    int    read3d_bpp;
    int    read3d_frames;
    int    read3d_xsize;
    int    read3d_ysize;
    int    read3d_skipbytes;
    int    curve_type;

    int    fft_direction;
    int    fft_ino1;
    int    fft_ino2;
    int    fft_wantdisplay;

    int    rcontrast;
    int    gcontrast;
    int    bcontrast;
    int    hcontrast;
    int    scontrast;
    int    vcontrast;
    int    icontrast;

    int    radjust;
    int    gadjust;
    int    badjust;
    int    hadjust;
    int    sadjust;
    int    vadjust;
    int    iadjust;

    int    raddvalue;
    int    gaddvalue;
    int    baddvalue;
    int    haddvalue;
    int    saddvalue;
    int    vaddvalue;
    int    iaddvalue;

    int    initializing;

    int    save_ascii_format;
    int    spot_dens_source;
    char  *table;       /* spot matching table */
    char  *spotlist;    /* spot list */
    double zoomfac;
    int    zooming;     /* 1=in zoom mode */
    int    leavemarked_box; 
    int    floating_magnifier;
    int    floating_magnifier_on_root;
    int    draw_pattern;
    int    inpattern;
    int    fix_cmyk;
    int    circlewidth;
    int    width;      /* for passing box sizes */
    int    height;     /* for passing box sizes */ 
    int    curve_line_width;
    int    circle_fill;     /* 0=no fill, 1=fill */
    int    circle_outline;  /* 0=no outline, 1=outline */
    int    circle_antialias;/* 0=normal, 1=antialias edge of circle or ellipse */
    int    gradient1_type;  /* gradient type 0=none, 1=radial*/
    int    gradient1_inner; /* inner gradient color for filled ellipses */
    int    gradient1_outer; /* outer gradient color for filled ellipses */
    double gradientx1;      /* x angle for gradient 0-1 */
    double gradienty1;      /* y angle for gradient 0-1 */
    int    gradient2_type;  /* gradient type 0=none, 1=radial*/
    int    gradient2_inner; /* inner gradient color for filled ellipses */
    int    gradient2_outer; /* outer gradient color for filled ellipses */
    double gradientx2;      /* x angle for gradient 0-1 */
    double gradienty2;      /* y angle for gradient 0-1 */
    double reflectivity;
    int    uselibtiff;      /* set to 0 to prevent using libtiff */
    int    in_crop;         /* cropping image */
    int    *helptopic;      /* ugly hack to pass integer parameters to callbacks */
} GLObals;
 

typedef struct ImageStruct
{   uchar   ***backup;      // Image buffer to back up images
    uchar   ***image;       // Image buffer for up to MAXIMAGES images at original bpp
    uchar   ***alpha;       // Alpha channel
    uchar   **img;          // Images converted to bpp of screen
    complex **fft;          // FFT'd image buffer
    double  **wavelet;      // Wavelet image buffer
    double  *wavelet_1d;    // 1D alias for wavelet[][]
    double  ***wavelet_3d;  // Wavelet image pyramid buffer
    uchar   *backup_1d;     // 1D alias for backup[][]
    uchar   *image_1d;      // 1D alias for image[][]
    uchar   *img_1d;        // 1D alias for img[][]
    complex *fft_1d;        // 1D alias for fft[][]
    uchar   *alpha_1d;      // 1D alias for alpha channel
    XImage  *image_ximage;  // XImage structs for image[][], for getting images from screen
    XImage  *img_ximage;    // XImage structs for img[][]
    Widget   widget;        // Widget for the image
    Window   win;           // Window for image to move it around quickly
    Pixmap   icon;          // Icon in case image gets iconified
    RGB *palette;           // Current palette for image
    RGB *opalette;          // Original palette for image before current operation
    RGB *spalette;          // Starting palette for image for backup
    int backedup;           // Flag to indicate if image was backed up
    int xsize;              // Width of image in pixels
    int ysize;              // Length of image in pixels
    int origxsize;          // Width of image in pixels before resizing
    int origysize;          // Length of image in pixels before resizing
    int xpos;               // x position of images
    int ypos;               // y position of images
    int xscreensize;        // Width of image in pixels as displayed on screen
    int yscreensize;        // Length of image in pixels as displayed on screen
    int overlap;            // List which image is in front (0=front).
                            //  i_overlap[0] = image # in foreground
                            //  i_overlap[1] = 2nd from foreground, etc.
    char *name;             // Image filename
    char *notes;            // User-supplied notes about file
    int bpp;                // Image bits/pixel as stored in memory
    int originalbpp;        // Image bits/pixel of original image
    int colortype;          // GRAY, INDEXED, COLOR, GRAPH
    int backup_colortype;   // GRAY, INDEXED, COLOR, GRAPH
    int ncolors;            // No. of colors in image (for information only)
    int floatexists;        // 1 = complex array exists for image (e.g.,FFT)
    int waveletexists;      // 1 = double array exists for image (e.g.,wavelet)
    int wavelettype;        // NONE, PYRAMIDAL, LAPLACIAN, MALLAT, MULTIRESOLUTION
    int wavelet_xminres;    // x size of low resolution part of wavelet
    int wavelet_yminres;    // y size of low resolution part of wavelet
    int wavelet_xlrstart;   // x upper left corner of low resolution part of wavelet
    int wavelet_ylrstart;   // y upper left corner of low resolution part of wavelet
    int fftdisplay;         // Display NONE,REAL,IMAG,POWR,WAVE
    double fmin;            // Lowest FFT value or wavelet value (high res. part)
    double fmax;            // Highest FFT value or wavelet value (high res. part)
    double wmin;            // Lowest wavelet value (low resolution part)
    double wmax;            // Highest wavelet value (low resolution part)
    double ffac;            // Conversion factor fft->pixel value
    double wfac;            // Conversion factor wavelet->pixel value
    int fftxsize;           // x size of fft buffer  
    int fftysize;           // y size of fft buffer  
    int fftstate;           // -1,0,1 depending on FFT history
    int wavexsize;          // x size of wavelet buffer
    int waveysize;          // y size of wavelet buffer
    int nsubimages;         // number of subimages in wavelet pyramid
    int nlevels;            // number of wavelet levels
    int *subimagexsize;     // x size of subimages
    int *subimageysize;     // x size of subimages
    int waveletstate;       // -1,0,1 depending on wavelet history

    char format_name[64];   // original file format in ASCII
    int format;             // format type
    int cal_dims;           // 1=1D 2=2D
    double cal_q[3][10];    // lin.reg. params. for calibration 
    double cal_slope[3];    // slope of calibration line 
    double cal_int[3];      // intercept of calib. line
    int cal_log[3];         // -1=uncalibrated 0=linear 1=logarithm
    int cal_xorigin[3];     // Starting point in calibration 
    int cal_yorigin[3];     // Starting point in calibration 
    char *cal_title[3];     // title for calib.box
    double cal_scale;       // factor for non-directional scale

    int hitgray;            // Flag if gray scaling in effect
    int adjustgray;         // 1=ok to automatically rescale gray mapping
    int gray[8];            // Scaling factors if image is gray
    int sort;               // Size sorted list of images       
    short int *gamma;       // Gamma (grayscale) table for image
    int touched;            // 1=image was modified
    int hit;                // Temporary variable for marking whether image was hit
    int frames;             // No. of frames in image
    double fps;             // Frames/sec
    int animation;          // 1=currently being animated
    int cf;                 // Current frame visible
    int shell;              // 1=in separate shell
    int scrollbars;         // 1=image has scrollbars (has no effect if shell=0)
    int permanent;          // TEMP =image is skipped in whichimage()
    int window_border;      // 1=shell window has border
    int chromakey;          // 1=image has transparency, -1=inverse chromakey
    int ck_graymin;         // Lowest pixel value that is opaque (for grayscale)
    int ck_graymax;         // Highest pixel value that is opaque  (for grayscale)
    RGB ck_min;             // Lowest pixel value that is opaque (for rgb)
    RGB ck_max;             // Highest pixel value that is opaque (for rgb)
    int transparency;       // 0=normal (opaque) 255=completely transparent
    Spreadsheet *s;         // Spreadsheet struct
    int is_zoomed;          // 0=normal size 1=zoomed 2=magnified   
    int was_compressed;     // 0=normal 1 was compressed
    double zoomx;           // x zoom factor (1=normal)
    double zoomy;           // y zoom factor (1=normal)
    double zoomx_inv;       // 1/x zoom factor (1=normal) to avoid division
    double zoomy_inv;       // 1/y zoom factor (1=normal) to avoid division
    int zoom_x1;            // upper left x of zoom
    int zoom_y1;            // upper left y of zoom    
    int zoom_x2;            // lower right x of zoom
    int zoom_y2;            // lower right  y of zoom    
    int xangle;             // Current angle around x axis
    int yangle;             // Current angle around y axis
    int zangle;             // Current angle around z axis
    int gparam[20];         // 3d graph parameters for image
    int gbutton[20];        // 3d graph buttons for image
    void (*exposefunc)(void *);  // function to call during expose event
    void *exposeptr;        // Pointer for passing data to exposefunc
    clickboxinfo c;         // For multiframe images
    int oframe_count;       // original no. of frames
    int split_frames;       // 1=image was derived from a multiframe image
    int split_frame_start;  // 1st frame ino 
    int split_frame_end;    // last frame ino (frames are contiguous)
    int floating;           // 1=image is a magnification of some other image
    int red_brightness[256];    // red brightness
    int green_brightness[256];  // green brightness
    int blue_brightness[256];   // blue brightness
    int gray_brightness[256];   // grayscale brightness
    int is_font;                // 1=not a real image, just fonts
} Image;



//--------------Function Prototypes------------------//
Widget add_bb_button(Widget parent, char *s, int x, int y, int w, int h, 
  int want_default);
Widget add_button(Widget parent, char *s1, int x, int y, int buttonwidth);
Widget add_mainbutton(char *name, int buttonno, void *cb);
Widget add_pushbutton(char *label, Widget parent, int x1, int y1, int x2, int y2, clickboxinfo *cb);
Widget add_slider(Widget parent, const char *name, int width, int left, int right, 
     int top, int bottom, int minval, int maxval, int startval, int increment);
Widget add_togglebutton(Widget parent, char *name, int x, int y, void *cb, void *ptr);
Widget desensitize_tearoff(void);
Widget edit(const char *title, const char *subtitle, char *&text, int rows, int columns, 
     int width, int height, int maxlength, int helptopic, int want_execute_button, 
     char *editfilename, void (*f9)(void *ptr, char *text), void *client_data);
Widget get_math_formulas(char *text, int mode);
Widget imagewidget(int xpos, int ypos, int xsize, int ysize, ImageStruct *z);
Widget list(listinfo *l);
Widget open_fuzzy_list(listinfo *l);
Widget message_window_open(char *heading);
Window init_subwindow(Window win, int xoff, int yoff);
XImage *createximage(int xsize, int ysize, int bpp, uchar *buf_1d); 
Pixmap createxpixmap(Window win, int xsize, int ysize, int bpp);
Graph open_graph(PlotData *pd, int ino, int type);
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, int *pdata, 
   int n, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v), 
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2), 
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac=1, double ydisplayfac=1);
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, double *pdata, 
   int n, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2), 
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac=1, double ydisplayfac=1);
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, int **pdata, 
   int n, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2), 
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac=1, double ydisplayfac=1);
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, double **pdata, 
   int npoints, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2), 
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac=1, double ydisplayfac=1);

FILE *open_file(char *filename, int &compress, int mode, char *compressor, 
    char *decompressor, char *ext);
char *basefilename(char* pathname);
char *getfilename(char *oldtitle, char *path);
char *getfilename(char *oldtitle, char *path, Widget widget);
char *itoa(int value, char *str, int radix); 
char *ltoa(long int value, char *str, int radix); 
char **make_2d_alias(char *b, int x, int y);
char *strip_relative_path(char* pathname);
char *doubletostring(double val, int signif, char *buf);
char *strlwr(char* s);
char *strupr(char* s);
complex **make_2d_alias(complex *b, int x, int y);
double area(int x1, int x2, int y1, int y2);
double atan3(double num, double denom);
double **make_2d_alias(double *b, int x, int y);
double checktime(int mode);
double cdensity(double xx, double yy, double diam);
double closeness(double a1, double a2);
double density(int ino, int x1, int x2, int y1, int y2, int compensate, int invert);
double diagonalscan(ScanParams *sp, double x, double y, double m, double b, 
      double denom);
double distance(int x1, int y1, int x2, int y2);
double draw_graph(PlotData *pd, int n1, int n2);
double eval(char* string, char *answer=NULL);
#ifndef HAVE_ERF
double erf(double x);
#endif
double fbetween(double a, double b, double c);
double findbiggest(double* pdata, int n);
double fit(XYData *data, int k0, int k1, int ***closest, 
       double **xdist, int **xclosest, double ***angle);
double gammln(double x);
double gammq(double a, double x);
double double_image_number(char *s);
double largest(int col);
double macro_double_input(char *prompt);
double ***make_3d_alias(double *b, int x, int y, int z);
double notzero(double a);
double pixels(double image_number);
double plotdata(Window win, double **data,int dims,int focus,int n,
     int ysize,int x1,int y1,int x2,int y2,int* markstart,int* markend);
double plotsection(double** data,int dims,int focus,int n,int n1,int n2,
     int ysize, int x1,int y1,int x2,int y2, int* markstart,int* markend);
double pointtoline(double x, double y, double m, double b);
double pixeldensity(int x, int y, int ino, int compensate, int invert);
double pixeldensity_image(int x, int y, int ino, int f, int compensate, 
     int invert);
double pixeldensityat(int x, int y, int frame, int ino);
double rdensity(double xx1, double yy1, double xx2, double yy2);
double read_surrounding_pixels(int i, int j, int x1, int y1, int x2, int y2,
     uchar **add, int bpp, int b, int dist);
double read_variable(char *name, char *answer, int &datatype);
double scan_arbitrary_polygon(ScanParams *sp, double xx[], double yy[], int &scancount);
double scan_fixed_width_area(ScanParams *sp, double xx[], double yy[], int &scancount);
double scanline(ScanParams *sp, int xs, int ys, int xe, int ye);
double scan_normal_polygon(ScanParams *sp, int x[], int y[], int &scancount);
double smallest(int col);
double spot_size(double spotno);
double spot_signal(double spotno);
double spot_x(double spotno);
double spot_y(double spotno);
double tree_score(XYData *data, int k0, int k1, double ***dist, int ***sorted, 
       double ***closestangle, int ***sortangle, int nclosest, int level);

int about_image_fill(int ino, char **info);
int acquire(void);
int acquire_hp_scan(ScannerSettings *sp);
int acquire_scan(ScannerSettings *sp);
int add_frame(int ino);

int allocate_image_wavelet(int ino, int xsize, int ysize, int transform_type, int levels);
int anti_aliased_value(int gatevalue, int opix, int npix, int colortype, int bpp);
int asciikey(void);
int between(int a,int b,int c);
int between(double a, double b, double c);
int bits(char byte, int start, int n);
int bits_per_pixel(void);
int boxoverlap(int x1,int y1,int x2,int y2,int x3,int y3,int x4,int y4);
int button_clicked(void);
int camera(void);
int changecolordepth(int noofargs, char **arg);
int change_depth(int ino, int frames, int colortype, uchar **buf1, uchar **buf2, 
    int bpp1, int bpp2, int want_palette, RGB palette[256]);
int change_depth(int ino, int frames, int colortype, uchar ***buf1, uchar ***buf2, 
    int bpp1, int bpp2, int want_palette, RGB palette[256]);
int check_if_png(char *identifier);
int change_image_depth(int ino, int bpp, int permanent);
int check_image_list(int *source, int count, int check_size);
int check_gamma_table(ScanParams *sp, int ino, int bpp);
int check_printer(void);
int check_save_parameters(char *filename, int ino, int type, int write_all);
int chop_image(int ino, int xpos, int ypos, int shell, int border);
int cint(double a);
int close_file(FILE *fp, int compress);
int combine_colors(int noofargs, char **arg);
int combine_frames(int xpos, int ypos, int shell, int border);
int combine_image_frames(int xpos, int ypos, int shell, int border, int *source, int count);
int compose_camera_command(CameraSettings *cam);
int composite_image(int xpos, int ypos, int shell, int border); 
int convolute(int ino1, int ino2, int direction);
int count_custom_formats(void);
int createimage(int noofargs, char **arg);
int create_distance_image(XYData *data, int xsize, int ysize, char *extension);
int create_fixed_size_image(int xsize, int ysize, int xpos, int ypos, int frames, int shell, int border);
int create_image_from_selected(int frames, int shell, int border);
int create_image_with_mouse(int xpos, int ypos, int frames, int shell, int border);
int create_multiframe_image(int cimage, int xsize, int ysize, int xpos, int ypos,  
    int frames, int shell, int border, int auto_edge);
int create_new_image(int method, int xpos, int ypos, int xsize,
   int ysize, int frames, int shell, int border, int find_edge, int cols, 
   int panel, int xspacing, int yspacing, int xmargin, int ymargin,
   int want_fixed_xsize, int fixed_xsize,
   int want_fixed_ysize, int fixed_ysize, int ino);
int create_subimage(int ino, int xsize, int ysize, int frames, int shell, int border);
int createpalette(void);
int create_panel(int cimage, int xpos, int ypos, int shell, int border,int cols);
int create_spotlist_panel(int ino, int xsize, int ysize, int xpos, int ypos, 
    int frames, int shell, int border, int xspacing, int yspacing, 
    int xmargin, int ymargin,
    int want_fixed_xsize, int fixed_xsize,
    int want_fixed_ysize, int fixed_ysize);
int cui_cmp(const void *p, const void *q);
int densitometry_calc(dialoginfo *a);
int do_line(int xs, int ys, int xe, int ye, int mode, lineinfo *li);
int do_line_antialias(int xs, int ys, int xe, int ye, int mode, lineinfo *li);
int do_pixel_math(int x1, int y1, int x2, int y2, char *text, int oktoconvert);
int dragpalette(palettecbstruct *pp);
int duplicate(int noofargs, char **arg);
int duplicate_image(int cimage, int xpos, int ypos, int shell, int border);
int erase_pixel(int x, int y, int *repairflags);
int execute(char *command, int noofargs, char **arg, int loopmode=0);
int execute_plugin(char *filename, char **arg, int noofargs, int mode);
int false_color(int val);
int filesize(char *filename);
int filter_region(filter *ff);
int find_pattern(Pattern p, int pattern_type, float **layer, char **hit, int ino, 
   int threshold, double pweight, double bweight, float min_signal, int is_color,
   int x1, int y1, int x2, int y2, int want_progress_bar);
int first_non_comment(char *text);
int fitpalette(int bpp, uchar ***buf1, uchar ***buf2, RGB *pal, int xsize, 
    int ysize, int frames, int &ncolors);
int format_selection(int file_format);
int four2d(int ino, int isign);
int fuzzy_cluster(int n, int c, double**u, int want_progress_bar);
int get_alpha_bit(int ino, int f, int x, int y);
int getbits (int n, uchar* block, int blocksize, int mode);
int getbox(int& x1,int& y1,int& x2,int& y2);
int getboxposition(int &x,int &y, int w, int h);
int get_camera_image(CameraSettings *cam);
int getcharacter(void);
int get_image_list(int *source, int &count);
int getLZWbits(int n, int &blocksize, char* block, int mode, FILE* fp,
      int noofstrips, int* bytesperstrip, int* strip);
int getnextnumber(char*buffer, int &bufpos, FILE *fp);
int getpoint(int &x,int &y);
int getstring(const char *heading, char *returnstring, int maxstringsize, int helptopic);
int grab_window(void);
int gradfillcolor(int startcolor, int x, int y, 
                  int x1, int y1, int x2, int y2,
                  int rmin, int rmax, int gmin, 
                  int gmax, int bmin, int bmax, 
                  int type, int bpp);
int graph_is_open(PlotData *pd);
int graypixel(int value, int bpp);
int grayscalemap(void);
int graytocolor(int ino, int value, int bpp);
int grayvalue(int value, int ino, int bpp);
int handle_command_line(int argc, char **argv);
int handle_option(char* option, int noofargs, char **arg);
int imagefiletype(char *filename, char *identifier, int &compressed);
int image_has_opaque_pixels(int ino);
int image_number(char *s);
int imagesize(int x, int y, int bpp, int wantall=0, int wantbackup=0, int frames=1);
int initialize_floating_magnifier(int size, int scale);
int insidebox(int x, int y, int x1, int y1, int x2, int y2);
int insidecircle(int i, int j, int x, int y, int d);
int inside_irregular_region(int x, int y);
int intensity(int pixelvalue, int bpp);
int invertpalette(void);
int intermediate_pixel_color(int rc, int gc, int bc, 
     int r1, int g1, int b1, int r2, int g2, int b2,
     int r3, int g3, int b3, int r4, int g4, int b4, int bpp);
int intermediate_pixel_gray(int vc, int v1, int v2, int v3, int v4);
int inwindows(void);
int isend(int c);
int is_customfile(char *filename, char *identifier, char *extension, char *match);
int is_dir(char *filename);
int is_opaque(int v, int tmin, int tmax, int chromakey);
int is_opaque(int rr, int rmin, int rmax,
              int gg, int gmin, int gmax,
              int bb, int bmin, int bmax, int chromakey);
int isoperator(int c);
int isparen(int c);
int joins_features4(int ino, int f, int i, int j, int bpp, uint thresh);
int joins_features8(int ino, int f, int i, int j, int bpp, uint thresh);
int keyhit(void);
int line(int x1, int y1, int x2, int y2, uint color, int mode, int win=0, int ino=0);
int line(int xs, int ys, int xe, int ye, int mode, lineinfo *line);
int linreg(double* x, double* y, int n, int regorder, double* q, double &f,
    double &r2);
int list_match(char **list, char *string, int n);
int macro(char *text, int loopmode=0);
int macro_find_spots(int noofargs, char **arg);
int main_callback(XEvent *xev, void *data);
int **make_2d_alias(int *b, int x, int y);
int ***make_3d_alias(int *b, int x, int y, int z);
int maxgamma(int ino);
int math(char *text);
int memorylessthan(int amount);
int message(const char *heading);
int message(const char *heading, int mode, int helptopic=0);
int message(const char *heading, char *returnstring, int mode, int maxstringsize, int helptopic);
int move(void);
int msm_getstatus(int *x, int *y);
int msm_getstatus(uint *x, uint *y);
int multiclickbox(const char* title, int noofboxes, clickboxinfo* a, 
      void f(int values[10]), int helptopic);
int neuron_count_mismatch(int ino, int x1, int y1, int x2, int y2) ;
int newimage(char *name, int xpos, int ypos, int xsize, int ysize, int bpp, 
   int colortype, int frames, int shell, int scrollbars, int permanent, 
   int wantpalette, int border, Widget widget);
int newximage(int xsize, int ysize); 
int nextchar(void); 
int next_code();
int nextpoint(XYData *data, int x, int y);
int nextpowerof2(int x);
int next_unused_color(int r, int g, int b);
int onedge(int x, int y, int x1, int y1, int x2, int y2);
int packbits(int n,uchar* inblock,uchar* outblock, int blocksize);
int palettecolor(int i, int j);
int parse_digits(char *inputstring, int *source, int maxcount);
int pixel_equivalent(int ino, int i, int ioffset, int j);
int pixel_on_pattern(int x, int y);
int pointinfo(int x1,int y1,int &bpp,int &iscolor,int &isbw,int &white,int &black);
int pseek(FILE *fp, int bytes, int read_mode, int compressed);
int random_color(void);
int random_number(void); 
int ranfunc(void);
int read3Dfile(char *filename);
int readLZWencodedfile(FILE* fp, int frame, int startsize, int clearcode, 
    int imagewidth, int imagelength, int kxfac, int kyfac, int bpp,
    int filetype, int noofstrips, int *bytesperstrip, int *strip, 
    int *yvalue);
int read_aliased_pixel(double x, double y, int ino);
int read_anti_aliased_pixel(double x, double y, int ino);
int readbmpfile(char *filename);
int readcustomfile(char *filename, char *identifier);
int read_data(char *filename, double data[], int count);
int read_fd(uchar *dest, int fd, int size);
int read_format_descriptor(char *identifier, Format &format);
int readfft(int noofargs, char **arg);
int readfiles(char* filename, int noofargs, char **arg);
int readfitsfile(char *filename);
int readgemfile(char *filename,int subtype);
int readgiffile(char *filename);
int readimafile(char *filename);
int readimage(char* filename, int noofargs, char **arg);
int read_image_wavelet(int noofargs, char **arg, Wavelet &wave, WaveletParams &wp);
int readimgfile(int type, int noofargs, char **arg, int compress, char *identifier);
int readjpgfile(char *filename);
int readmacrofile(char *filename, char *text);
int readmultiframefile(char *filename);
int read_next_non_comment(FILE *fp, char *buffer);
int readpalette(void);
int readbioradfile(char *filename);
int readpbmfile(char *filename, uchar **buffer);
int readpdsfile(char *filename);
int readpds2file(char *filename);
int readpcxfile(char *filename);
int readpngfile(char *filename);
int readtextfile(char *filename);
int readtgafile(char *filename);
int readtiffile(char *filename, char *actual_filename);
int readtiffile_libtiff(char *filename, char *actual_filename);
int readtiffile_general(char *filename, char *actual_filename);
int readxwdfile(char *filename);
int rebuild_display(int ino);
int recombine_frames(int &ino);
int remove_gradient(int noofargs, char **arg);
int resize_image(int ino, int xsize, int ysize);
int resize_img(int ino, int xsize, int ysize);
int rotate(char *title, double *angle, int noofargs, char **arg, int *param, 
    void(*user_ok_function)(dialoginfo *a, int radio, int box, int boxbutton),
    void(*user_cancel_function)(dialoginfo *a));
void rotate_image(int ino, double degrees);
int save_ok(int ino);
int saveimage(int ino, int noofargs, char **arg);
int savepalette(void);
int savetouchedimages(void);
int scanner_type(int fd, char *vendor, char *model);
int select_plugin(int noofargs, char **arg);
int send_scsi_command(int fd, uchar *wbuf, int wsize, uchar *rbuf, int rsize);
int separate_colors(int ino, int noofargs, char **arg);
int set_palette(void); 
int set_antialiased_pixel_on_image(double x, double y, uint color, int mode, int bpp, 
    int skip, int noscreen, Window win, int want_alpha, int onlyino, int zoomed);
int setpixelonimage(int x, int y, uint color, int mode, int bpp=0, 
     int skip=-1, int noscreen=0, Window win=0, int want_alpha=0, 
     int onlyino=0, int zoomed=0);
int setupfft(int ino, int& xsize3, int& ysize3);
int sg_ask(int fd, int code, int kind, int *answer);
int sg_in(int fd, char *data, int len);
int sg_out(int fd, char *format, int value);
int sg_out(int fd, char *data);
int shift_frame(int noofargs, char **arg);
int shrink(int noofargs, char **arg, int mode);
int shrink_image(int ino, double sxfactor, double syfactor);
int shrink_image_interpolate(int ino, double sxfactor, double syfactor);
int shrink_image_interpolate_in_place(int ino, double sxfactor, double syfactor);
int sketch(int xs, int ys, int xe, int ye, uint color, int mode, lineinfo *line, int inmath);
int spray(int noofargs, char **arg);
int sprayrand(int sprayfac);
int split_frames(int xpos, int ypos, int cimage, int shell, int border, int cols, int panel);
int start_fft(int direction, int ino1, int ino2, int wantdisplay);
int stringtohex(char* string);
int string_index(char *text, int direction, int c);
int strpos(char *s, int n, char c);
int swap_pixel_bytes(int value, int bpp);
int tif_cmp(const void *p, const void *q);
int tif_fread(char *a, int bytes, FILE* fp, int mac, int bytestouse);
int tif_fread(int *a, int bytes, FILE* fp, int mac, int bytestouse);
int tif_fread(short int *a, int bytes,FILE* fp, int mac, int bytestouse);
int tif_fread(short uint *a, int bytes, FILE* fp, int mac, int bytestouse);
int tif_fread(uchar *a, int bytes, FILE* fp, int mac, int bytestouse);
int tif_fread(uint *a, int bytes, FILE* fp, int mac, int bytestouse);
int trace_curve(void);
int transform(int noofargs, char **arg);
int unmodified_grayscale(RGB *p);
int update_floating_magnifier(int x, int y);
int wavelet_laplacian_calc(WaveletParams &wp);
int whatbpp(int x, int y);
int whatmode(int xresolution);
int which_file_format(char *identifier);
int whichimg(int x, int y, int& bpp, int skip=-2);
int whichimage(int x, int y, int& bpp, int skip=-2);
int which_image_number(int x, int y, int& bpp, int skip, int from_screen);
int whichimage(Widget w, int& bpp);
int whichimage(Window w, int& bpp);
int which_key_pressed(XEvent *event);
int whichwindow(int x, int y);
int write3Dfile(char *filename,int write_all, int compress);
int writeasciifile(char *filename, int write_all, int compress);
int writebmpfile(char *filename, int write_all, int subtype, int compress);
int writecustomfile(char *filename, int write_all, char *identifier, int compress);
int write_fd(uchar *source, int fd, int size);
int writefitsfile(char *filename, int write_all, int compress);
int writefft(int ino);
int writegiffile(char *filename, int write_all, int compress);
int writegif89file(char *filename, int write_all, int compress);
int write_image_wavelet(Wavelet &wave, WaveletParams &wp);
int writeimgfile(char *filename, int write_all, int compress);
int writejpeg(char *filename, int write_all, int compress);
int writemultiframefile(char *filename, int compress);
int writepcxfile(char *filename,int write_all, int compress);
int writepngfile(char *filename, int write_all, int compress);
int writerawfile(char *filename,int write_all, int compress);
int writetgafile(char *filename,int write_all, int compress);
int writetiffile(char *filename,int write_all, int compress);
int write_pcl_file(PrinterStruct p);
int write_postscript_file(PrinterStruct p);
int wuquantize(int bpp, uchar*** buf1, uchar*** buf2, int xsize, int ysize, 
    int frames, int &ncolors);
int yyparse(void);
int zoom_x_coordinate(int x, int ino);
int zoom_y_coordinate(int x, int ino);
int zoom_x_coordinate_of_index(int x, int ino);
int zoom_y_coordinate_of_index(int y, int ino);
int zoom_x_index(int x, int ino);
int zoom_y_index(int y, int ino);

short int tif_value_at(unsigned int tif_pointer, int size, FILE* fp, int mac);
template<class type> void testtemplate(type k);
uchar **make_2d_alias(uchar *b, int x, int y);
uchar getbyte(FILE* fp); 
uchar readbyte(int x,int y);
uint **make_2d_alias(uint *b, int x, int y);
uchar ***make_3d_alias(uchar *b, int x, int y, int z);
uint fix_negative_value(int signedvalue, int bpp);
uint getdword(FILE *);
uint getlittledword(FILE* fp);  /* endian-specific routine */
uint getbigdword(FILE* fp);  /* endian-specific routine */
uint RGBvalue(int red, int green, int blue, int bpp);
uint convertpixel(uint color, int bpp1, int bpp2, int wantcolor=1);
uint ink_color(uint color, int ink, int p_palette, 
     int bpp, int p_positive, double p_rfac, double p_gfac, double p_bfac,
     double p_ifac);
uint interact(int color1, int color2, int bpp, int imode);
uint pixelat(uchar* p, int bpp);
uint readpixel(int x, int y, int skip=-2);
uint readpixelonimage(int x, int y, int& bpp, int& ino, int skip=-1);
uint readpixelonimg(int x, int y);
uint setpixel(int x, int y, uint color, int mode, Window win=0, int ino=-2);
ushort getlittleword(FILE* fp); /* endian-specific routine */
ushort getbigword(FILE* fp); /* endian-specific routine */
ushort getword(FILE *);

void add_all_bbbuttons(dialoginfo *a, Widget bb, Widget *button, int y,
   char *s1, int x1, char *s2, int x2, char *s3, int x3, int buttonwidth, int helptopic);
void add_all_variables(int ino);
void addbbbuttons(dialoginfo *a, Widget bb, Widget *buttons, int y, int helptopic, int width); 
void add_bb_dismiss_cancel_help(dialoginfo *a, Widget bb, Widget *buttons, double y); 
void add_cam_option(CameraSettings *cam, char *option);
void add_cam_option(CameraSettings *cam, char *option, int value);
void add_fits_key(char *r, int n, const char *keyword, int value);
void add_fits_key(char *r, int n, const char *keyword, const char *value);
void additemtolist(Widget w, Widget form, char *s, int showpos, int wantraise);
void add_option(char *command, char *option);
void add_option(char *command, char *option, int value);
void add_pointer_variable(char *name, void *ptr);
void add_string_variable(char *name, char *string, double d);
void add_wm_close_callback(Widget w, XtCallbackProc closecb, void *arg);
void add_variable(char *name, double value);
void aboutcustomfile(char *filename, char *identifier);
void aboutfitsfile(char *filename);
void aboutgiffile(char *filename);
void about_image_update(int listno);
void aboutpcxfile(char *filename);
void aboutpdsfile(char *filename);
void aboutthefile(int noofargs, char **arg);
void abouttheimage(int noofargs, char **arg);
void abouttheprogram(void);
void abouttiffile(char *filename, int compressed);
void aboutxwdfile(char *filename);
void addvalue(int noofargs, char **arg);
void addpalettecolors(int d[10]);
void allocate_grain_arrays(void);
void alternate(int noofargs, char **arg);
void alternate_images(int n, int delay, int *image_list);
void animate(int noofargs, char **arg);
void apply_newpalette(void);
void assign_variable(double value,double &variable, int op);
void attributes(void);
void background(ScanParams *sp, int x1, int y1, int x2, int y2, int invert,
   double *results, listinfo *li);
void backupregion(int x1, int y1, int x2, int y2);
void backup_registration_points(XYData *data);
void backupimage(int ino, int notify);
void baseline(PlotData *pd, int iterations);
void beziercb(Widget w, XtP client_data, XEvent *event);
void bezier_curve_end(XYData *data);
void bezier_curve_start(XYData *data, dialoginfo *a);
void blackbox(int x1,int y1,int x2,int y2, uint color, Window win=0); 
void block(Widget w, int *want_block);
void block_modal(Widget w, int *want_block);
void box(int x1, int y1, int x2, int y2, uint color, int mode, int win=0, int ino=0);
void box(int x1, int y1, int x2, int y2, int mode, lineinfo *line);
void brightnessadjust(int a[10]);
void calc_distance(int dx, int dy, double &r, double &theta);
void calc_distance(double dx, double dy, double &r, double &theta);
void calculate_rotation_map(XYData *data, double **dr, int xsize, int ysize);
void calculate_vector_map(XYData *data, int **dx, int **dy, int xsize, int ysize);
void calculate_shifts(int bpp, int &rr, int &gg, int &bb, int &ii);
void calculate_selected_region(void);
void calculate_size(XYData *data, int &xsize, int &ysize);
void calibrate(void);
void calibrate_calc(XYData *data, int dim, double &f, double &r2);
void calibratepixel(int ux, int uy, int ino, int dim, double &c, char* answerstring);
void camera_load_defaults(listinfo *l);
void camera_read_config(CameraSettings *cam);
void camera_read_defaults(CameraSettings *cam);
void changebrightness(int noofargs, char **arg);
void changecontrast2(int noofargs, char **arg);
void changecontrast(int noofargs, char **arg);
void changecontrastcb(void *pd, int n1, int n2);
void changegraycontrast(int noofargs, char **arg);
void changegraycontrastcb(void *p, int n1, int n2);
void changefont(void);
void change_intensity(int noofargs, char **arg);
void changepalette(int noofargs, char **arg);
void changetitle(int noofargs, char **arg);
void check_calibration_data(XYData *data);
void check_camera_dialog(CameraSettings *cam);
void check_diagnose(int argc, char **argv);
void check_event(void);
void check_overwrite_file(PromptStruct *ps);
void check_value(char *s);
void chromaticaberration(void);
void circle(int x, int y, int d, int width, uint color, int mode, int win=0);
void clear_alpha(int ino);
void clear_busy_flags(void);
void clickbox(const char* title, int identifier, int *answer, int minval, int maxval,
     void f(int answer), void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *ptr, Widget parent, int helptopic);
void clickbox(const char* title, int identifier, double *fanswer, double fminval, double fmaxval, 
     void f(double answer), void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *ptr, Widget parent, int helptopic);
void clickbox(const char* title,  int identifier, int *answer, double *fanswer, double factor,
     int startval, int minval, int maxval, 
     void f1(int answer),  void f2(double answer), 
     void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *ptr, Widget parent, int decimalpoints, int type, int helptopic);
void click_prompt(char *heading, PromptStruct *ps, int helptopic);
void close_graph(PlotData *pd);
void cls(uint color);
void cluster(double *pixels, int n, int c, int N, double *answers, listinfo *li,
   int want_progress_bar);
void colorpixeldensity(int x, int y, double &rdensity, double &gdensity, 
     double &bdensity, int invert);
void configure(void);
void convertRGBpixel(int &r, int &g, int &b, int bpp1, int bpp2);
void converttocolor(int ino);
void converttograyscale(int ino);
void copybackground(int x1, int y1, int x2, int y2, int skipimage);
void copy_calib_values(Dialog *a, int j, int ino);
void copyimage(int x1, int y1, int x2, int y2, int xoff, 
      int yoff, int mode, XImage *image, Window win);
void copyimageparameters(int k1, int k2);
void copythebackup(int k);
void copy_wavelets(int ino);
void correlate_points(XYData *data, int ***closest, int *match, int n, 
     double ***angle, char *table, int xsize, int ysize);
void create_warped_spot_list(XYData *data, char *spotlist, int **xbias, 
     int **ybias, int *match, char *spot_filename, int xsize, int ysize);
void create_custom_format(void);
void create_edm(int ino, int se, uint thresh, array<uint> *e);
void create_image_initialize(int ino, char *tempname);
void create_landmarks(XYData *data, char *table, int &xsize, int &ysize);
void create_main_icon(void);
void create_print_filename(int ino, char *print_filename);
void create_contour_map(int ino, int struct_element, int maxsignal, int ksize);
void create_rotation_map(XYData *data, char *table, int *match, int &xsize, int &ysize);
void create_unwarped_spot_list(XYData *data, char *spotlist, int **xbias, 
     int **ybias, int *match, char *spot_filename, int xsize, int ysize);
void create_vector_map(XYData *data, char *table, int *match, int &xsize, int &ysize);
void crop(int noofargs, char **arg);
void crop_rectangle(int xs, int ys, int xe, int ye, double *cc, double *dd, int color);
void cross(int x, int y, int d, uint color, int mode, int win=0);
void crosshairs(int x, int y);
void cross_sort(XYData *data, double **xdist, int **xclosest);
void cset(clickboxinfo *c);
void cswap(complex &a, complex &b);
void curve(void);
void curve_densitometry(void);
void custom_read(FILE *fp, int &pos, int offset, int *value, int bytes, int defaultvalue);
void custom_write(FILE *fp, int offset, uint val, int &imagestart, int &pos);
void custom_write(FILE *fp, int offset, ushort val, int &imagestart, int &pos);
void custom_write(FILE *fp, int offset, RGB val, int &imagestart, int &pos);
void custom_write(FILE *fp, int offset, void *val, int bytes, int &imagestart, int &pos);
void darken(int noofargs, char **arg);
void dashed_box(int x1, int y1, int x2, int y2, uint color1, uint color2,
      int spacing, int mode);
void delete_list(listinfo *l);
void delete_previous_command(CameraSettings *cam);
void densitometry(void);
void destroy_floating_magnifier(void);
void dialogbox(dialoginfo *dialog);
void dialog_message(Widget w, char *s);
void dialogmulticlickboxfinish(clickboxinfo *c);
void diff_distrib(int ino, double thresh, int size, int wantlabels, 
     int pattern_wantrectangles, int labelcolor, int wantgraph, int wantdensgraph);
void difference_filter(int ino, int size, int sharpfac);
void dilate_binary(int ino, int se, int maxsignal, uint thresh);
void dilate_erode_gray(int ino, int se, int want_erode, int maxsignal, uint thresh);
void dilate_erode_rgb(int ino, int se, int want_erode, int maxsignal, double erodefac);
void do_masking(void);
void do_read_img_file(char *filename, char *oldfilename, char *identifier, int type, int compress);
void draw(int mode);
void draw_arc(int xx1, int yy1, int x, int y, int orientation, int width, 
              int color, int mode);
void draw_boxes(XYData *data);
void draw_curve(XYData *data, int mode, int modifiable);
void draw_grid(void *ptr);
void draw_main_menu(void);
void draw_string(char *string,int x,int y, uint fcol, uint bcol,
      GC *gc, Window win, Pixmap spixmap, int wantbackground, 
      int vertical, int ino);
void draw_arrow(int x1, int y1, int x2, int y2, lineinfo *line);
void drawbezier(XYData *data, uint color, int mode);
void drawbspline(XYData *data, uint color, int mode);
void draw_cross(int x, int y, int color);
void draw_image_title(int ino);
void drawlabel(int noofargs, char **arg);
void drawline(int noofargs, char **arg, int whatever);
void drawopenpolygon(XYData *data, int color, int mode);
void drawpalettearrows(palettecbstruct *pc);
void draw_point_to_point_curve(XYData *data, uint color, int mode);
void drawpolygon(XYData *data, int color, int mode);
void drawrectangle(XYData *data, int color, int mode);
void drawregressionline(XYData *data, uint color, int mode);
void draw_sample_color(Window win, int rr, int gg, int bb, int x, int y);
void drawselectbox(int mode);
void drawsketch(XYData *data, int color, int mode);
void drawsnaptrapezoid(XYData *data, int color, int mode, int modifiable);
void draw_spot_labels(XYData *data, int color);
void draw_spots(XYData *data, int color);
void drawtrapezoid(XYData *data, int color, int mode, int modifiable);
void draw_vector(int x1, int y1, int x2, int y2, int color);
void edit_table(XYData *data, char *title, char *table, char *filename); 
void ellipse(int xx1, int yy1, int xx2, int yy2, int width, int color, int mode, int figure_type);
void enhance(void);
void erasebackground(void);
void erase_fft(int ino);
void erasefromscreen(int ino);
void eraseimage(int ino, int ask, int savewidget, int want_redraw);
void erase_resize_wavelet(int ino);
void erase_wavelet(int ino);
void erode_binary(int ino, int se, int maxsignal, uint thresh);
void error_message(const char *s, int error);
void evaluate(char *string, int wantmessage);
void evaluate(int noofargs, char **arg, int wantmessage);
void evaluate_args(int noofargs, char **arg);
void fill(int x, int y, int minx, int miny, int maxx, int maxy, int fc, int type, 
       int bpp, int minborder, int maxborder, int mingrad, int maxgrad);
void fill(int x, int y, int &xmin, int &ymin, int &xmax, int &ymax,
       double fc, double maxfc, double minfc, int &totalpixels, double &totalsignal);
void fillbuf(int **buf, int x, int y, int minimumx, int minimumy, int maximumx,
     int maximumy, int &xmin, int &ymin, int &xmax, int &ymax, int fc, 
     int maxfc, int minfc, int &totalpixels, double &totalsignal,
     int signalfromimage, int ino);
void fill_gradient(int minx, int miny, int maxx, int maxy, int fc, 
     int type, int bpp, int mingrad, int maxgrad);
void fill_interactive(int x, int y, int minx, int miny, int maxx, int maxy, 
          int fc, int type, 
          int bpp, int minborder, int maxborder, int mingrad, int maxgrad);
void fill_level(int ***cell, int h, int xsize, int ysize, int d, int ithresh);
void fill_polygon(int *xx, int *yy, int color) ;
void fillregion(void);
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);  
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, lineinfo *li);
void fill_triangle_fast(int x1, int y1, int x2, int y2, int x3, int y3, int color);  
void filter_start(int noofargs, char **arg);
void filter_diff(filter *ff);
void filter_engrave(filter *ff);
void filter_image(filter *ff);
void filter_sharpen_edges(filter *ff);
void filter_user_defined(filter *ff);
void find_closest_point(int ***grid, int ino, int x, int y, int xar, int yar, int nx,
     int ny, int &xclosest, int &yclosest, int &gxmin, int &gymin, int &gxmax, int &gymax);
void find_selected_point(int **selected_point, int ino, int x1, int y1, int x2, int y2, 
     int xar, int yar, int &gxmin, int &gymin, int &gxmax, int &gymax);
void filter_rank_level(filter *ff);
void find_alpha(int x, int y, int ino, int minx, int miny, int maxx, int maxy,
                 int &left, int &top, int &right, int &bottom);
void find_highest_neuron(float **layer, int x1, int y1, int x2, int y2, 
     int &xmax, int &ymax, float &vmax);
void find_inside(int x, int y);
void find_next_statement(char *&text, char *line, char **arg, int &argno);
void findpeaks(PlotData *pd);
void findedge(int ino, int &x, int &y);
void find_region(int x, int y, int ino, int minx, int miny, int maxx, int maxy,
                 int &left, int &top, int &right, int &bottom);
void find_region_by_color(int x, int y, int ino, int minx, int miny, 
     int maxx, int maxy, int &left, int &top, int &right, int &bottom);
void find_spots(int ino, int method, double thresh, double pweight, double bweight, int size,
     int wantlabels, int pattern_wantrectangles, int labelcolor, int wantgraph, 
     int wantdensgraph);
void flip_horiz(void);
void flip_vert(void);
void floating_magnifier_dialog();
void float_raisecb(void *ptr);
void force_background(filter *ff);
void for_loop(char *textptr);
void formula_error(char *error_message, char *s);
void forward_seek(FILE *fp, int &pos, int destination, int mode);
void fourn(complex data[], int nn[], int ndim, int isign);
void framenext(int answers[100]);
void free_3d(double ***b, int z);
void free_3d(int ***b, int z);
void free_3d(uchar ***b, int z);
void free_pattern(Pattern *p);
void free_grain_counting_arrays(void);
void freetype(char *string, int xpos, int ypos, int face, int weight, int slant, int size);
void fuzzy_list_fill(listinfo *l);
void gamma(int noofargs, char **arg);
void gcf(double *gammcf, double a, double x, double *gln);
void get_average_value(int x1,int y1,int x2,int y2,int &v,int &rv,int &gv,int &bv);
void get_calibration_data(XYData *data);
void getcolor(const char* title, uint *answer, int bpp, int cmyk, 
     void(*f7)(clickboxinfo *c), void *ptr);
void get_expression(char *&text, char *line, int delimiter1, int delimiter2);
void getfilenameps(PromptStruct *ps);
void getfilenames(clickboxinfo *c);
void getinteger(const char *title, int *answer, int minval, int maxval, 
    void f5(clickboxinfo *c), 
    void f6(clickboxinfo *c), void *client_data, int helptopic);
void getline(int &x1, int &y1, int &x2, int &y2);
void getnextchar(void);
void get_next_statement(char *&text, char *line, char **arg, int &argno);
void get_next_tex_word(char *buffer, char *word, int maxlen, int &start);
void get_next_word(char *buffer, char *word, int maxlen, int &start);
void getxselection(void);
void get_special_character(void);
void getstrings(const char *title, char **label, char **headings, char ***answer, 
     int dims, int nstrings, int maxlen);
void gotoxy(int xpos,int ypos);
void grains(double thresh);
void graph_adjust(clickboxinfo *c);
void gser(double *gamser, double a, double x, double *gln);
void handle_scanner_error(int error_code);
void handle_setpoint(void);
void help(int topic);
void histogram(Graph graph);
void histogram_equalize(int ino);
void HSVtoRGB(int h, int s, int v, int &r, int &g, int &b, int bpp);
void identify_object(int x, int y);
void image_border(void);
void image_bounds(void *p, int n1, int n2);
void image_jump(int noofargs, char **arg, int direction);
void image_list(void);
void image_list_switchto(listinfo *l);
void image_list_update(int listno);
void image_shift_frame(int ino, int hshift, int vshift);
void image_stats(int x1, int y1, int x2, int y2, int &ino, int &bpp, int &ymin, 
     int &ymax, int &rmin, int &gmin, int &bmin, int &rmax, int &gmax, int &bmax);
void image_valuetoRGB(uint pix, int &rr, int &gg, int &bb, int bpp, int ino);
void init_data(void);
void initialize_evaluator(void); 
void initialize_format_descriptor(Format &format);
void initialize_globals(void);
void initialize_grid(int ***grid, int xar, int yar, int nx, int ny);
void initialize_image(char *fname, char *filename, int ino, int type, 
                      int compress, int signedpixels, char *identifier, 
                      int status);
void initialize_selected_point(int **s, int xar, int yar);
void init_xlib(void);
void initializegif(uchar* buffer, char* block, FILE* fp);
void initialize_vector_map(int xsize, int ysize);
void inttoRGB(int &r, int &g, int &b, int bpp);
void invertcolors(int noofargs, char **arg);
void invertimage(void);
void label(void);
void lamptest(ScannerSettings *sp);
void laplace_copy2level0( Image a );
void laplace_copylevel0_2image( Image a, WaveletParams &wp );
void laplace_expand_x( double **coarse_level, double **fine_level, const BinomialFilter & filter,
  			int size_y, int size_x_coarse, int size_x_fine );
void laplace_expand_y_and_add( double **coarse_level, double **fine_level,
				const BinomialFilter & filter,
  			int size_x, int size_y_coarse, int size_y_fine );
void laplace_expand_y_and_subtract( double **coarse_level, double **fine_level,
				const BinomialFilter & filter,
  			int size_x, int size_y_coarse, int size_y_fine );
void laplace_reduce_x( double **fine_level, double **coarse_level, const BinomialFilter & filter,
  			int size_y, int size_x_fine, int size_x_coarse );
void laplace_reduce_y( double **fine_level, double **coarse_level, const BinomialFilter & filter,
  			int size_x, int size_y_fine, int size_y_coarse );
void lighten(int noofargs, char **arg);
void lineselect(int xs, int ys, int xe, int ye);
void lineto(int xe, int ye, uint color, int mode, Window win);
void listokcb(Widget w, XtP client_data, XmACB *call_data);
void localcontrast(filter *ff);
void localcontrast(int noofargs, char **arg);
void localcontrast_color(filter *ff);
void localcontrast_gray(filter *ff);
void localcontrast_adaptive(filter *ff);
void local_smooth(double **grid, int xsize, int ysize, int fsize);
void local_smooth(int **grid, int xsize, int ysize, int fsize);
void loop(char *textptr, int looptype);
void macro_string_input(char *prompt, char *returnstring);
void macro_open(char *filename);
void macro_close(char *filename);
void make_fuzzy_list_unbusy(listinfo *l);
void manual_baseline(PlotData *pd, XYData *data);
void mapimage(int x1, int y1, int x2, int y2, XImage *image);
void mask(int noofargs, char **arg);
void match_points(XYData *data, char *table, int **xbias, int **ybias,
    int *match, int ino);
void match_spots(XYData *data, int xsize, int ysize);
void maximize_contrast(int button, clickboxinfo *cb);
void maximize_contrast(int noofargs, char **arg);
void measure(void);
void menu3d(void);
void message_window_close(Widget w);
void message_window_update(Widget www, char *string);
void morphfilter(int noofargs, char **arg);
void mousecursor(unsigned mx,unsigned my);
void moveimage(int ino, int xpos, int ypos, int want_redraw=1);
void movewindow(int wino);
void movie(int delay);
void multiple_crosshairs(int x, int y);
void multiplypalettecolors(int d[10]);
void mult_sobel(filter *ff);
void nexttag(short int tagid, short int type,int length,
      int value); 
void null(double a);
void null(int a);
void null(int a[10]);
void null(char *s);
void null(void);
void null(pcb_struct v);
void null(dialoginfo *a, int b, int c, int d);
void null(void *pd, int n1, int n2);
void null(dialoginfo *a);
void null(listinfo *a);
void null(listinfo *l, int a);
void null(clickboxinfo *c);
void null(PromptStruct *ps);
void null(int k, clickboxinfo *c);
void null(void *ptr, char *string);
void null(void *ptr, char *string, int answer);
void null(Widget w, void *ptr1, void *ptr2);
void obtain_control_points(XYData *data, int dimension); 
void paint(int color);
void parsecommand(char *cmd, int type, char **arg, int &noofargs, int maxlen, int maxargs);
void parse_table(void *data, char *table); 
void parse_spotdata(XYData *data, char *table); 
void parse_string(char *cmd, char **buf, int &noofargs, int startarg, int maxlen);
void parse_word(char *buffer, char *word, int maxlen, int &count);
void partition(void);
void paste(void);
void pattern_recognition(void);
void patterns(Pattern p, int pattern_type, double thresh, double pweight, 
     double bweight, int wantlabels, int wantrectangles, int wantgraph,
     int wantdensgraph);
void pixelmode(int noofargs, char **arg);
void pixel_text(char *result, int ino, int x, int y);
void plot3d(int ino);
void plot3dgraph(p3d *p);
void plotinstallcolormapcb(void *p, int n1, int n2);
void plotpalettecallback(pcb_struct v);
void plugincheck(listinfo *l);
void print(char *string,int x, int y, uint fcol, uint bcol, GC *gc, Window win, 
           int wantbkg, int vertical, int wantwarp=1, int onlyino=0);
void print_configuration(void);
void printcoordinates(uint mx, uint my, int exposed);
void print_diagnostic_information(void);
void print_grain_label(int val, int x, int &y, int fc, int bc, int ino);
void print_grain_label(char *valstring, int x, int &y, int fc, int bc, int ino);
void print_graph_coordinates(PlotData *pd, int x, int y);
void printimage(void);
void print_image_info(void);
void print_information(void);
void print_label(int val, int x, int y, int fc, int bc);
void print_label(char *valstring, int x, int y, int fc, int bc);
void print_rotated(int x, int y, char *label, double degrees);
void printstatus(int state);
void printstatus(int state, char *message);
void print_image_title_at_bottom(int ino);
void process_tex_command(char *string, int pos, int &xoff, int &yoff);
void progress_bar_open(Widget &www, Widget &scrollbar);
void progress_bar_update(Widget w, int percent);
void progress_bar_close(Widget w);
void prompt(char *heading, PromptStruct *ps, int helptopic);
void push_main_button(int k);
void putbigdword(uint i, FILE* fp);
void putbigtag(taginfo t, FILE* fp);
void putbigword(ushort i, FILE* fp);
void putbyte(uchar c, FILE* fp);
void put_area_in_list(PlotData *pd);
void putdword(uint i, FILE *fp);
void putlittledword(uint i, FILE* fp);
void putlittletag(taginfo t, FILE* fp);
void putlittleword(ushort i, FILE* fp);
void putword(ushort i, FILE *fp);
void putpixel(uint v, int size, FILE* fp);
void putpixelbytes(uchar *address, uint color, 
      int repetitions=1, int bpp=0, int direction=1);
void putRGBbytes(uchar *address, int rr, int gg, int bb, int bpp);
void putstring(char *s, int count, FILE* fp);
void puttag(taginfo t, FILE* fp);
void quick_segmentation(int ino, int struct_element, int maxsignal, uint thresh);
void randompalette(int seed);
void randompalettecb(int seed);
void read_calib(XYData *data);
void read_custom_format_list(char **custom_format_list);
void *read_ptr(char *name);
void readRGB(int x, int y, int &rr, int &gg, int &bb, int skip);
void readRGBonimage(int x, int y, int& bpp, int& ino, int &rr, int &gg, int &bb, int skip);
void read_table(XYData *data, char *table, char *filename); 
void readsettings(int rereading);
void readstart(int noofargs, char **arg);
void read_warp_data(XYData *data, int dim);
void read_wavelet(Wavelet &w);
void rebuild_table(XYData *data, char *table); 
void redraw(int ino);
void redraw_graph(PlotData *pd);
void redrawscreen(void);
void redraw_chromakey_image(int ino, int want_redraw=1);
void redraw_transparent_image(int ino, int want_redraw=1);
void redraw_zoom_image(int ino);
void registration(void);
void registration_rotate(dialoginfo *a, int warpmode);
void registration_set_ino(Dialog *a, int dim, int ino);
void registration_warp(dialoginfo *a, int warpmode);
void reinitialize(void);
void remapcolors(void);
void remap_coordinates(int ino, int f, int newino, int ***grid, int gridx1,
     int gridy1, int gridx2, int gridy2, int nx, int ny);
void remap_image_colormap(int ino);
void remap_palette(RGB *pal1, RGB *pal2, int *remap);
void remap_pixels(uchar **buf, int w, int h, int ino_src, int ino_dest);
void remove_arg(char *string, int pos, int arglen);
void remove_leading_space(char *s1, char *s2);
void remove_outer_parentheses(char *s, char c1, char c2);
void remove_quotes(char *s);
void remove_terminal_cr(char *s);
void remove_trailing_junk(char *s);
void remove_trailing_space(char *s);
void repair(int ino);
void repairimg(int ino, int x1, int y1, int x2, int y2);
void reselect_area(int mode);
void resensitize_tearoff(Widget w);
void reset_fft_toggle(dialoginfo *a);
void resetselectbox(int x1, int y1, int x2, int y2, int *save1, int *save2,
                    int *save3, int *save4, int square);
void reset_user_buttons(void);
void resize(int noofargs, char **arg);
void resize_selection(void);
void resize_widget(Widget w, int xsize, int ysize);
void restoreregion(void);
void restore_registration_points(XYData *data);
void restoreimage(int want_restore_colortype);
void getstring(const char *heading, PromptStruct *ps);
void RGBat(uchar* p, int bpp, int &rr, int &gg, int &bb);
void RGBtoint(int &r, int &g, int &b, int bpp);
void RGBtoHSV(int rr, int gg, int bb, int &h, int &s, int &v, int bpp);
void root(int ino, int mode);
void rotate3d(int ino, double xdegrees, double ydegrees, double zdegrees);
void rotatepalette(int k);
void rounded_rectangle(int xx1, int yy1, int xx2, int yy2, int width, int color, int mode);
void samplecolor(int c);
void samplecolor(int c[10]);
void samplepalette(void);
void sample_palette_color(void);
void sample_palette_pseudo(void);
void sanity_check(void);
void save_calib(XYData *data);
void save_scan(char *filename, double data[], int count, int mode);
void save_scan(char *filename, int data[], int count, int mode);
void save_image_file(char *filename, int file_format, int write_all, int ino, int compress); 
void save_table(XYData *data, char *table, char *filename); 
void savesettings(void);
void save_sizes(int ino, int *grain, int *size, double *signal, int **coordinate, 
     int *signal_distribution, int count, int maxcount, int most_frequent_size, 
     int biggest, double biggest_signal, double xfac, double thresh, int minsize,
     int maxsize, char *filename, int want_ask_filename);
void save_warp_data(XYData *data);
void scalefft(int ino);
void scale_signed_image(int ino);
void scan_fixed_width_area3d(ScanParams *sp, double xx[], double yy[],  double zz[], 
  int &scancount);
void scan_region(void);
void scan_region_calc(XYData *data, ScanParams *sp, dialoginfo *a);
void scan_region_macro(int noofargs, char **arg);
void scan_region_start(XYData *data, ScanParams *sp, dialoginfo *a);
void seg_size_distrib(int ino, double thresh, int wantlabels, int pattern_wantrectangles, 
     int labelcolor, int wantgraph, int wantdensgraph, int source, int **sourcebuf);
void select_color(void);
void select_freetype_font(void);
void selectimage(int k);
void select_list_item(Widget w, int pos);
void select_pattern(void);
void select_region(int x1, int y1, int x2, int y2);
void select_area(int mode);
void send_expose_event(Window w, int eventtype, int x, int y, int width, int height);
void send_buttonpress_event(Window w, int eventtype, int x, int y, int state, int button);
void set_alpha_bit(int ino, int f, int x, int y, int value);
void setbcolor(int noofargs, char **arg);
void set_chromakey(int ino);
void set_circle_parameters(void);
void set_ellipse_pixels(int xx, int yy, int xx1, int yy1, int xx2, int yy2,
     int width, int color, int mode);
void setfont(char *fontname);
void setSVGApalette(int paletteno,int wantset=1);
void setfcolor(int noofargs, char **arg);
void setfloatpixel(int x,int y,uint color,int ino);
void set_grain_pattern(Pattern *p);
void set_gray_levels(int ino);
void setimagetitle(int ino, const char *name);
void set_line_parameters(void);
void set_list_top(Widget w, int pos);
void set_motif_colors(void);
void setpalette(RGB* pal);
void set_pattern_threshold(Pattern *p1, Pattern *p2, double thresh);
void setpixelonscreen(int x, int y, int color);
void setRGBonimage(int x, int y, int rr, int gg, int bb, int mode, int bpp=0, 
     int skip=-1, int noscreen=0, Window win=0, int want_alpha=0, int ino=0,
     int zoomed=0);
void setslider(p3d *p, int sliderno, int value);
void setslider(void **ptr, int sliderno, int value);
void set_transparency(int ino, int noofargs, char **arg);
void setup_colors(void);
void setup_display(void);
void set_user_pattern(Pattern *p);
void set_value(char *s);
void setwaveletpixel(int x, int y, uint color, int ino);
void set_widget_double(Widget w, double value);
void set_widget_label(Widget w, char* s);
void set_widget_radio(Dialog *a, int radio, int value);
void set_widget_resizable_label(Widget w, char* s);
void set_widget_string(Widget w, char* s);
void set_widget_toggle(Widget w, int value);
void set_widget_value(Widget w, int value);
void shareware(int skip);
void shareware_finish(void);
void shift_selected_area(int ox, int oy, int &newx, int &newy);
void show_alpha(int ino);
void showcalib(uint ux, uint uy);
void show_data_vectors(XYData *data, int *match);
void showmenubar(void);
void showodtable(void);
void showoptions(void);
void show_densitometry_results(int x, int y, listinfo *dlist, XYData *data, int count);
void show_notes(int ino);
void show_selected(int ino);
void show_wavelet_grid(int ino, int transform_type);
void shrink(double *x, int m, double *x2, int n);
void sig_pipe(int signo);
void sig_child(int signo);
void simplemessage(const char *heading, int mode, int helptopic);
void single_crosshairs(int x, int y);
void skeletonize_binary(int ino, int struct_element, int maxsignal, uint thresh);
void skeletonize_gray(int ino, int struct_element, int maxsignal, uint thresh);
void smooth(double* data,int count,int width);
void smooth(int* data,int count,int width);
void sort(char *item[], int sorted[], int count);
void sort(int item[], int sorted[], int count);
void sort(double item[], int sorted[], int count);
void sortpalette(int ci);
void spray_things(dialoginfo *a);
void specialcharacter(void);
void status_error_message(int status);
void strip_extraneous_dashes(char *s);
void substitute_font(char *font, int param, char *portion);
void subtract_background(filter *ff);
void subtract_pattern(Pattern p, float **layer, char **hit,
     int ino, int xmax, int ymax);
void swapbytes(uchar* a, int n, int size);
void swap_decimal_point(char* text, char *s);
void swap_image_bytes(int ino);
void switch_palette(int ino);
void switch_selected_region(void);
void switchto(int k);
void tempmessageopen(Widget &www);
void tempmessageupdate(Widget www, char *heading);
void tempmessageclose(Widget www);
void test(void);
void test1(void);
void threshold(int ino, uint thresh);
void tif_getlineofpixels(FILE* fp, uchar* buffer, int length, int compression, 
   int bpp, int mac, int rowsperstrip, int* tif_strip, int nooftiles);
void translate_points(XYData *data); 
void transparency_set_image(int ino, int t);
void unload_image(int noofargs, char **arg);
void unpush_main_button(int k);
void unpush_null_buttons(void);
void unsetpixel(int noofargs, char **arg);
void unset_radio(Dialog *a, int radio);
void unzoom(int ino);
void update_calib_dialog(dialoginfo *a, int dimension);
void update_camera_dialog(CameraSettings *cam);
void update_cell(int ino, int x, int y, int color);
void update_cell(int ino, int x, int y);
void update_chromatic_aberration_dialog(Dialog *a, int j);
void update_color(void);
void update_custom_dialog(Dialog *a, Format &format, int update_widgets);
void update_image(int ino, int **xbias, int **ybias, double **rbias,
     int xsize, int ysize);
void update_fuzzy_list(listinfo *l);
void update_graph(int graphno);
void update_list(int listno);
void update_sample_palette(void);
void user_position_copy(int citoerase, int x1, int y1, int x2, int y2,
    int want_remap, int mode, int force_alpha, 
    int gray_min, int gray_max,  RGB rgb_min, RGB rgb_max, 
    int bpp, uchar **savebpp, uchar **alpha, int &hitbkg, int want_background);
void ushorttoa(unsigned short i, char* a);
void valuetoRGB(uint pixel, int &r, int &g, int &b, int bpp);
void warp(void);
void warp1d(int direction);
void warp2d(dialoginfo *a, int nx, int ny);
void watershed(int ino, int struct_element, int maxsignal, int wantboundaries);
void wavelet(int noofargs, char **arg);
void wavelet_pyramidal_calc(Wavelet &wave, WaveletParams &wp);
void wavelet_transform(Wavelet &wave, double *aa, int n, int direction);
void windowsoff(void);
void windowson(void);
void write_code(unsigned short int code, char* block, FILE* fp);
void write_format_descriptor(Format &format);
void xor_box(int x1, int y1, int x2, int y2);
void zoom(int noofargs, char **arg, int mode);
void zoom_image(int ino, double zfac, int x, int y, int mode);

#ifdef HAVE_MOTIF

Widget addbblabel(Widget parent, char *string, int position, double x, double y); 
Widget addlabel(Widget parent, char *string, int position, double x1, double y1, double x2, double y2);
Widget create_info_area(Widget parent, const char *title, int ystart, int height);
Widget createmenu(Widget parent, const char *name, const char *label, char mnemonic);
Widget createmenuseparator(Widget parent, const char *name);
Widget createmenuitem(Widget parent, const char *name, const char *label,
	char mnemonic, 
        void (* callback)(Widget w, XtP client_data, XmACB *call_data),
        const char *cbArg);

void addOkCancelHelpButtons(Widget form, Widget *button, char *s1, int x1,
   char *s2, int x2, char *s3, int x3, int buttonwidth, int *data);
void addstandardbuttons(Widget form, Widget *buttons, int *data, int width=0);
void create_drawing_area(Widget parent);
void handle_button_press(void);
void handle_button_release(void);
void makewidgets(Widget parent);
void xminitializemisc(Widget parent);

int string_width(char *string, char *tag);
int string_height(char *string, char *tag);
int current_graph(Widget obj);
int graphhandle(Widget obj, int event, int mx, int my, int key, void *xev);


// Check functions 
void attributescheck(dialoginfo *a, int radio, int box, int boxbutton);
void calibcheck(dialoginfo *a, int radio, int box, int boxbutton);
void cameracheck(dialoginfo *a, int radio, int box, int boxbutton);
void chromakeycheck(dialoginfo *a, int radio, int box, int boxbutton);
void chromaticaberrationcheck(dialoginfo *a, int radio, int box, int boxbutton);
void circlecheck(dialoginfo *a, int radio, int box, int boxbutton);
void composite_image_check(dialoginfo *a, int radio, int box, int boxbutton);
void configcheck(dialoginfo *a, int radio, int box, int boxbutton);
void createimagecheck(dialoginfo *a, int radio, int box, int boxbutton);
void curvecheck(dialoginfo *a, int radio, int box, int boxbutton);
void customcheck(dialoginfo *a, int radio, int box, int boxbutton);
void densitometry_check(dialoginfo *a, int radio, int box, int boxbutton);
void fillcheck(dialoginfo *a, int radio, int box, int boxbutton);
void filtercheck(dialoginfo *a, int radio, int box, int boxbutton);
void floating_magnifier_check(dialoginfo *a, int radio, int box, int boxbutton);
void freetypecheck(dialoginfo *a, int radio, int box, int boxbutton);
void label_check(dialoginfo *dialog, int radio, int box, int boxbutton);
void lamptestcheck(dialoginfo *a, int radio, int box, int boxbutton);
void lineparamcheck(dialoginfo *a, int radio, int box, int boxbutton);
void maskcheck(dialoginfo *a, int radio, int box, int boxbutton);
void measurecheck(dialoginfo *a, int radio, int box, int boxbutton);
void morphcheck(dialoginfo *a, int radio, int box, int boxbutton);
void patterncheck(dialoginfo *a, int radio, int box, int boxbutton);
void pixelmodecheck(dialoginfo *a, int radio, int box, int boxbutton);
void printcheck(dialoginfo *a, int radio, int box, int boxbutton);
void readcheck(dialoginfo *a, int radio, int box, int boxbutton);
void read3dfilecheck(dialoginfo *a, int radio, int box, int boxbutton);
void readimgfilecheck(dialoginfo *a, int radio, int box, int boxbutton);
void registrationcheck(dialoginfo *a, int radio, int box, int boxbutton);
void remove_gradient_check(dialoginfo *a, int radio, int box, int boxbutton);
void rotatecheck(dialoginfo *a, int radio, int box, int boxbutton);
void savecheck(dialoginfo *a, int radio, int box, int boxbutton);
void scancheck(dialoginfo *a, int radio, int box, int boxbutton);
void scanregioncheck(dialoginfo *a, int radio, int box, int boxbutton);
void spraycheck(dialoginfo *a, int radio, int box, int boxbutton);
void tracecheck(dialoginfo *a, int radio, int box, int boxbutton);
void warpcheck(dialoginfo *a, int radio, int box, int boxbutton);
void wavelet_check(dialoginfo *a, int radio, int box, int boxbutton);


// OK functions
void addvalueok(clickboxinfo *c);
void alternateok(clickboxinfo *c);
void changebrightnessok(clickboxinfo *c);
void changecontrastok(clickboxinfo *c);
void combinecolorsok(clickboxinfo *c);
void dialoglistokcb(listinfo *l);
void getcolorok(clickboxinfo *c);
void grayscalemapok(clickboxinfo *c);
void menu3dok(p3d *c);
void registration_set_ino_ok(clickboxinfo *c);
void setcalterms(clickboxinfo *c);
void shiftframeok(clickboxinfo *c);


// Finish functions
void addvaluefinish(clickboxinfo *c);
void alternatefinish(clickboxinfo *c);
void calibfinish(dialoginfo *a);
void camerafinish(dialoginfo *a);
void camerafinish2(dialoginfo *a);
void changebrightnessfinish(clickboxinfo *c);
void changecontrastfinish(clickboxinfo *c);
void changefontfinish(listinfo *l);
void changepalettefinish(listinfo *l);
void chromaticaberrationfinish(dialoginfo *a);
void circlefinish(dialoginfo *a);
void combinecolorsfinish(clickboxinfo *c);
void configcheck(dialoginfo *a);
void configfinish(dialoginfo *a);
void curvefinish(dialoginfo *a);
void customfinish(dialoginfo *a);
void densitometry_start(dialoginfo *a);
void densitometry_cancel(dialoginfo *a);
void densitometry_done(dialoginfo *a);
void densitometry_finish(dialoginfo *a);
void densitometry_list_close(listinfo *l);
void dialoglistcancelcb(listinfo *l);
void dlist_finish(listinfo *l);
void fftfinish(dialoginfo *a);
void fillfinish(dialoginfo *a);
void filterfinish(dialoginfo *a);
void findpeaksfinish(listinfo *l);
void freetypecancel(dialoginfo *a);
void freetypefinish(dialoginfo *a);
void getcolorfinish(clickboxinfo *c);
void grab_window_finish(PromptStruct *ps);
void graph_list_finish(listinfo *l);
void grayscalemapfinish(clickboxinfo *c);
void label_cancel(dialoginfo *a);
void lineparamfinish(dialoginfo *a);
void manual_baseline_finish(PlotData *pd, XYData *data);
void manual_bezier_end(PlotData *pd, XYData *data);
void maskfinish(dialoginfo *a);
void measurecancel(dialoginfo *a);
void measurefinish(dialoginfo *a);
void measure_list_clear(listinfo *l);
void measure_list_finish(listinfo *l);
void paletteselectfinish(listinfo *l);
void patternfinish(dialoginfo *a);
void printfinish(dialoginfo *a);
void read3dfilefinish(dialoginfo *a);
void readimgfilefinish(dialoginfo *a);
void readfinish(dialoginfo *a);
void registrationfinish(dialoginfo *a);
void remove_gradient_finish(dialoginfo *a);
void rotatecancel(dialoginfo *a);
void rotsmoothingcb(clickboxinfo *c);
void savefinish(dialoginfo *a);
void scanfinish(dialoginfo *a);
void scanregionfinish(dialoginfo *a);
void shiftframefinish(clickboxinfo *c);
void sprayfinish(dialoginfo *a);
void tracefinish(dialoginfo *a);
void unblock_save(dialoginfo *a);
void warpfinish(dialoginfo *a);
void warpsmoothingcb(clickboxinfo *c);
void savetext_finish(PromptStruct *ps);


// Part 2 & 3 functions
void changecolordepth_part2(int ino, int bytesperpixel);
void dialogclickboxcb_part2(clickboxinfo *c);
void dialogclickboxcb_cancel(clickboxinfo *c);
void dialogdoubleclickboxcb_part2(clickboxinfo *c);
void dialogmulticlickboxcb_part2(clickboxinfo *c);
void dialogrgbclickboxcb_part2(clickboxinfo *c);
void grab_window_part2(PromptStruct *ps);
void listfinish_part2(void *ptr, char *string, int answer);
void remap_image_colormap_part2(clickboxinfo *c);
void save_calib_part2(PromptStruct *ps);
void savecb_part2(PromptStruct *ps);
void save_scan_double_part2(PromptStruct *ps);
void save_scan_double_part3(PromptStruct *ps);
void save_scan_int_part2(PromptStruct *ps);
void save_scan_int_part3(PromptStruct *ps);
void save_table_part2(PromptStruct *ps);
void savetext_part2(PromptStruct *ps);
void save_image_file_part2(PromptStruct *ps);
void savepalette_part2(PromptStruct *ps);
void save_sizes_part2(PromptStruct *ps);
void save_warp_data_part2(PromptStruct *ps);
void setbcolor_part2(clickboxinfo *c);
void setfcolor_part2(clickboxinfo *c);
void setgraybcolor_part2(clickboxinfo *c);
void setgrayfcolor_part2(clickboxinfo *c);
void spreadsheet_savecb_part2(PromptStruct *ps);
void writefft_part2(PromptStruct *ps);
void write_image_wavelet_part2(PromptStruct *ps);
void write_pcl_file_part2(PromptStruct *ps);
void write_postscript_file_part2(PromptStruct *ps);


void okmsgcb(Widget w, XtP client_data, XmACB *call_data);
void cancelmsgcb(Widget w, XtP client_data, XmACB *call_data);
void nomsgcb(Widget w, XtP client_data, XmACB *call_data);
void helpmsgcb(Widget w, XtP client_data, XmACB *call_data);

// Callbacks

void alternatecb(XtP client_data, XtIntervalId *timer_id);
void armboxcb(Widget w, XtP client_data, XmACB *call_data);
void boxneedrepeatcb(Widget w, XtP client_data, XmACB *call_data);
void buttoncb(Widget w, XtP client_data, XmACB *call_data);
void camera_acquirecb(Widget w, XtP client_data, XmACB *call_data);
void cancelcb(Widget w, XtP client_data, XmACB *call_data);
void clearcb(Widget w, XtP client_data, XmACB *call_data);
void clickboxcancelcb(Widget w, XtP client_data, XmACB *call_data);
void clickboxcb(Widget w, XtP client_data, XmACB *call_data);
void clickboxkeycb(Widget w, XtP client_data, XEvent *event);
void clickboxokcb(Widget w, XtP client_data, XmACB *call_data);
void clickboxunmapcb(Widget w, XtP client_data, XmACB *call_data);
void configurecb(Widget w, XtP client_data, XEvent *event);
void delaycb(Widget w, XtP client_data, XmACB *call_data);
void dialogcancelcb(Widget w, XtP client_data, XmACB *call_data);
void dialogchangecicb(Widget w, XtP client_data, XmACB *call_data);
void dialogclickboxcb(Widget w, XtP client_data, 
      XmACB *call_data);
void dialogcursorcb(Widget w, XtP client_data, XmACB *call_data);
void dialogdoubleclickboxcb(Widget w, XtP client_data, XmACB *call_data);
void dialogentercb(Widget w, XtP client_data, XEvent *event);
void dialogexposecb(Widget w, XtP client_data, XtP *call_data);
void dialogfilenamecb(Widget w, XtP client_data, XmACB *call_data);
void dialoglistcb(Widget w, XtP client_data, XmACB *call_data);
void dialogmulticlickboxcb(Widget w, XtP client_data, XmACB *call_data);
void dialogokcb(Widget w, XtP client_data, XmACB *call_data);
void dialogrgbclickboxcb(Widget w, XtP client_data, XmACB *call_data);
void dialogstringcb(Widget w, XtP client_data, XmACB *call_data);
void dialogunmapcb(Widget w, XtP client_data, XmACB *call_data);
void disarmcb(Widget w, XtP client_data, XmACB *call_data);
void downcb(Widget w, XtP client_data, XtP call_data);
void dragpaletteclickcb (Widget w, XtP client_data, XmACB *call_data);
void dragpaletteexposecb(Widget w, XtP client_data, XmACB *call_data);
void dragpalettemousecb(Widget w, XtP client_data, XEvent *event);
void drawcb(Widget widget, XtP client_data, XtP *call_data);
void editcursorcb(Widget w, XtP client_data, XmACB *call_data);
void editdonecb(Widget w, XtP client_data, XmACB *call_data);
void editentercb(Widget w, XtP client_data, XEvent *event);
void editokcb(Widget w, XtP client_data, XmACB *call_data);
void editmodifycb(Widget w, XtP client_data, XmACB *call_data);
void editunmapcb(Widget w, XtP client_data, XmACB *call_data);
void entercb(Widget w, XtP client_data, XEvent *event);
void eventcb(Widget w, XtP client_data, XEvent *event);
void executecb(Widget w, XtP client_data, XmACB *call_data);
void executetextcb(Widget w, XtP client_data, XmACB *call_data);
void fftcheck(dialoginfo *a, int radio, int box, int boxbutton);
void filenamecb(Widget widget, XtP client_data, XmACB *call_data);
void formback(Widget obj, long menuno);
void fscdcb(Widget w, XtP client_data, XmFSBCB *call_data);
void fscancelcb(Widget w, XtP client_data, XmFSBCB *call_data);
void fsdirmaskcb(Widget w, XtP client_data, XmFSBCB *call_data);
void fsokcb(Widget w, XtP client_data, XmFSBCB *call_data);
void fsscb(Widget w, XtP client_data, XmLCB *call_data);
void fsunmapcb(Widget w, XtP client_data, XmFSBCB *call_data);
void getstringcb(Widget w, XtP client_data, XmACB *call_data);
void getstringcancelcb(Widget w, XtP client_data, XmACB *call_data);
void getstringentercb(Widget w, XtP client_data, XEvent *event);
void getstringokcb(Widget w, XtP client_data, XmACB *call_data);
void getstring_no(Widget w, XtP client_data, XmACB *call_data);
void getstring_unmap(Widget w, XtP client_data, XmACB *call_data);
void getstring_yes(Widget w, XtP client_data, XmACB *call_data);
void graphcb(Widget w, XtP client_data, XmACB *call_data);
void graph_bezier_startcb(Widget w, XtP client_data, XmACB *call_data);
void graphbuttoncb(Widget w, XtP client_data, XmACB *call_data);
void graphcancelcb(Widget w, XtP client_data, XmACB *call_data);
void graphexposecb(Widget w, XtP client_data, XmACB *call_data);
void graphunmapcb(Widget w, XtP client_data, XmACB *call_data);
void graphxlabelexposecb(Widget w, XtP client_data, XEvent *event);
void graphylabelexposecb(Widget w, XtP client_data, XEvent *event);
void graphmousecb(Widget w, XtP client_data, XEvent *event);
void helpcb(Widget w, XtP client_data, XmACB *call_data);
void histogram_savecb(void *ptr, int n1, int n2);
void image_exposecb(Widget widget, XtP client_data, XtP *call_data);
void imagefocuscb(Widget w, XtP client_data, XEvent *event);
void imageunmapcb(Widget w, XtP client_data, XmACB *call_data);
void infocb(Widget widget, XtP client_data, XtP *call_data);
void leftcb(Widget w, XtP client_data, XtP call_data);
void listcb(Widget w, XtP client_data, XmACB *call_data);
void listfinish(Widget w, XtP client_data, XmACB *call_data);
void listentercb(Widget w, XtP client_data, XEvent *event);
void listunmapcb(Widget w, XtP client_data, XmACB *call_data);
void loadtextcb(Widget w, XtP client_data, XmACB *call_data);
void mainbuttoncb(Widget w, XtP client_data, XmACB *call_data);
void maincancelcb(Widget w, XtP client_data, XmACB *call_data);
void main_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void manualblfinishedcb(Widget w, XtP client_data, XmACB *call_data);
void menu3dokcb(Widget w, XtP client_data, XmACB *call_data);
void menu3dcancelcb(Widget w, XtP client_data, XmACB *call_data);
void messageunmapcb(Widget w, XtP client_data, XmACB *call_data);
void minimal_cancelcb(Widget w, XtP client_data, XmACB *call_data);
void mousecb(Widget w, XtP client_data, XEvent *event);
void movecb(Widget w, XtP client_data, XmACB *call_data);
void movecallback(Widget obj, long direction);
void moviecb(XtP client_data, XtIntervalId *timer_id);
void msgcb(Widget w, XtP client_data, XmACB *call_data);
void multiclickboxcancelcb(Widget w, XtP client_data, XmACB *call_data);
void multiclickboxkeycb(Widget w, XtP client_data, XEvent *event);
void multiclickboxokcb(Widget w, XtP client_data, XmACB *call_data);
void multiclickboxunmapcb(Widget w, XtP client_data, XmACB *call_data);
void multitoggleboxcb(Widget w, XtP client_data, XmACB *call_data);
void multitoggleboxokcb(Widget w, XtP client_data, XmACB *call_data);
void multitoggleboxcancelcb(Widget w, XtP client_data, XmACB *call_data);
void okcb(Widget w, XtP client_data, XmACB *call_data);
void palettecb(Widget w, XtP client_data, XmACB *call_data);
void plot3dbuttoncb(Widget w, XtP client_data, XmACB *call_data);
void plot3ddonecb(Widget w, XtP client_data, XmACB *call_data);
void plot3dslidercb(Widget w, XtP client_data, XmACB *call_data);
void plot3dunmapcb(Widget w, XtP client_data, XmACB *call_data);
void progress_bar_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void promptmsgcb(Widget w, XtP client_data, XmACB *call_data);
void prompt_no(Widget w, XtP client_data, XmACB *call_data);
void prompt_unmap(Widget w, XtP client_data, XmACB *call_data);
void prompt_yes(Widget w, XtP client_data, XmACB *call_data);
void propertycb(Widget w, XtP client_data, XmACB *call_data);
void quitcb(Widget w, XtP client_data, XmACB *call_data);
void raisecb(Widget w, XtP client_data, XEvent *event);
void resize_scrolled_imagecb(Widget w, XtP client_data, XEvent *event);
void rightcb(Widget w, XtP client_data, XtP call_data);
void samplepaletteclickcb(Widget w, XtP client_data, XmACB *call_data);
void samplepalettecolorunmapcb(Widget w, XtP client_data, XmACB *call_data);
void samplepalettecolorexposecb(Widget w, XtP client_data, XmACB *call_data);
void samplepalettecolorclickcb(Widget w, XtP client_data, XmACB *call_data);
void samplepaletteexposecb(Widget w, XtP client_data, XmACB *call_data);
void samplepalettemousecb (Widget w, XtP client_data, XEvent *event);
void sample_palette_pseudocb(Widget w, XtP client_data, XmACB *call_data);
void samplepalettepseudounmapcb(Widget w, XtP client_data, XmACB *call_data);
void savecb(Widget w, XtP client_data, XmACB *call_data);
void savetextcb(Widget w, XtP client_data, XmACB *call_data);
void selectioncb(Widget widget, XtP client_data, Atom *selection, Atom *type, 
        XtP value, ulong *value_length, int *format);
void select_colorcb(Widget widget, XtP client_data, XtP *call_data);
void select_color_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void select_color_unmanagecb(Widget w, XtP client_data, XmACB *call_data);
void select_color_answercb(Widget w, XtP client_data, XEvent *event);
void select_pattern_answercb(Widget w, XtP client_data, XEvent *event);
void select_patterncb(Widget widget, XtP client_data, XtP *call_data);
void select_pattern_unmanagecb(Widget w, XtP client_data, XmACB *call_data);
void select_pattern_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void simplemessagecb(Widget w, XtP client_data, XmACB *call_data);
void simplemessageunmapcb(Widget w, XtP client_data, XmACB *call_data);
void slidercb(Widget w, XtP client_data, XmACB *call_data);
void slider_nof_cb(Widget w, XtP client_data, XmACB *call_data);
void slider_now_cb(Widget w, XtP client_data, XmACB *call_data);
void spreadsheet_ok(Widget w, XtP client_data, XmACB *call_data);
void spreadsheet_finish(Widget w, XtP client_data, XmACB *call_data);
void spreadsheet_savecb(Widget w, XtP client_data, XmACB *call_data);
void spreadsheet_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void startcb(Widget w, XtP client_data, XmACB *call_data);
void tearoff_activatecb(Widget widget, XtP client_data, XtP *call_data);
void tearoff_deactivatecb(Widget widget, XtP client_data, XtP *call_data);
void tempmessage_unmapcb(Widget w, XtP client_data, XmACB *call_data);
void tempmessage_configurecb(Widget w, XtP client_data, XEvent *event);
void timer_callback(XtP client_data, XtIntervalId *timer_id);
void toggleboxcb(Widget w, XtP client_data, XmACB *call_data);
void togglecb(Widget w, XtP client_data, XmACB *call_data);
void upcb(Widget w, XtP client_data, XtP call_data);
void unmanagecb(Widget w, XtP client_data, XmACB *call_data);
void warningmessagecb(String aaa);
void xrotcb(Widget w, XtP client_data, XmACB *call_data);
void yrotcb(Widget w, XtP client_data, XmACB *call_data);
void zrotcb(Widget w, XtP client_data, XmACB *call_data);
#else
#error Motif required
#endif // ifdef HAVE_MOTIF


//---------end of function prototypes-------------//

#endif  // ifndef XMTNIMAGE









