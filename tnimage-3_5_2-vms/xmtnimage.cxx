//--------------------------------------------------------------------//
// xmtnimage.cc                                                       //
// Motif version of tnimage                                           //
// Latest revision: 03-20-2006                                        //
// Copyright (C) 2006 by Thomas J. Nelson                             //
// See xmtnimage.h for Copyright Notice                               //
//--------------------------------------------------------------------//
 
#include "xmtnimage.h"
#include "xmtnimagec.h"

Image *z;
PrinterStruct printer;
Globals g;
int ci=-1;                      // current image
int mouse_near_selection=0;
int cancelled=0;
const int SLOW_DELAY = 1000;
int key_mapping = 0;
int crashing = 0;
int blocked_while_saving = 0;
int setpoint = 0;
int title_ypos = 0;
static void sig_quit(int signo);
static void sig_abort(int signo);
static inline void drawpixel(int x, int y, int color, Window win);
static inline int getpixel(int x, int y, int ino);
extern int in_printimage, in_save, in_read, in_calibrate, in_warp;
extern int in_camera, in_acquire, in_pattern_recognition;
extern int in_spot_dens_dialog;
extern int in_strip_dens_dialog;

////  Don't set `transient' resource in fallbacks.
static const char *fallback_resources[] = 
{    
   "*.marginHeight: 0",
   "*.marginWidth: 0",
   "*.highlightThickness: 0",
   "*.background: gray",
   "*.foreground: black",
   "*.shadowThickness: 1",

   "*.MenuBar*spacing: 3",
   "*.MenuBar*background: gray",
   "*.MenuBar*fontList: *helvetica-bold-r-normal--12*",
   "*.fontName: *times-bold-r-normal--12*",
   "*.Editor*fontList: *helvetica-bold-r-normal--12*",
   "*.Editor.background:gray",   
   "*.Editor.foreground:black",
   "*.EditForm*fontList: *helvetica-bold-r-normal--12*",
   "*.Accept*fontList: *helvetica-bold-r-normal--12*",
   "*.Ok*fontList: *helvetica-bold-r-normal--12*",
   "*.OK*fontList: *helvetica-bold-r-normal--12*",
   "*.Dismiss*fontList: *helvetica-bold-r-normal--12*",
   "*.Edit*fontList: *helvetica-bold-r-normal--12*",
   "*.Cancel*fontList: *helvetica-bold-r-normal--12*",
   "*.Help*fontList: *helvetica-bold-r-normal--12*",
   "*.Save*fontList: *helvetica-bold-r-normal--12*",
   "*.Clear*fontList: *helvetica-bold-r-normal--12*",
   "*.Start*fontList: *helvetica-bold-r-normal--12*",
   "*.XmLabel*fontList: *helvetica-bold-r-normal--12*",
   "*.About the File*fontList: *helvetica-bold-r-normal--12*",
   "*.About the Image*fontList: *helvetica-bold-r-normal--12*",
   "*.Select Image*fontList: *helvetica-bold-r-normal--12*",
   "*.drawing_area2*XmPushButton*fontList: *helvetica-bold-r-normal--12*",
   "*.drawing_area2*XmToggleButton*fontList: *helvetica-bold-r-normal--12*",
   "*.Plot3DForm*XmToggleButton*fontList: *helvetica-bold-r-normal--12*",
   "*.XmText*background: #CaC5C2",
   "*.XmTextField*background: #CaC5C2",
   "*.DialogForm*XmText*background: #CaC5C2",
   "*.DialogForm*XmTextField*background: #CaC5C2",

   "*.info_area0.background:gray",   
   "*.info_area0.foreground:black",
   "*.info_area0*fontList: *helvetica-bold-r-normal--24*",
   "*.info_area1.background:gray",   
   "*.info_area1.foreground:black",
   "*.info_area1*fontList: *helvetica-bold-r-normal--24*",
   "*.info_area2.background:gray",   
   "*.info_area2.foreground:black",
   "*.info_area2*fontList: *helvetica-bold-r-normal--24*",
   "*.info_area3.background:gray",   
   "*.info_area3.foreground:black",
   "*.info_area3*fontList: *helvetica-bold-r-normal--24*",


//   "*.title_area.background:red",   

   "*.DialogForm*background: gray",
   "*.DialogForm*foreground: black",
   "*.DialogForm*fontList:*helvetica-bold-r-normal--12*",                // labels
   "*.DialogForm.XmToggleButton.fontList:fixed", // toggle buttons
   "*.DialogForm.XmPushButton.fontList:fixed",   // push buttons
   "*.DialogForm*BoxPushButton*fontList:*helvetica-bold-r-normal--12*",
   "*.DialogForm*radiobox*fontList:fixed",       // radio boxes
   "*.DialogForm*Accept*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*Ok*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*Cancel*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*Dismiss*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*Help*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*START*fontList: *helvetica-bold-r-normal--12*",
   "*.DialogForm*transient: True",
   "*.label.fontList:*helvetica-bold-r-normal--12*",
   "*.image*background:black",

   "*XmMessageBox*marginHeight: 5",
   "*XmMessageBox*marginWidth: 5",
   "*XmMessageBox*background: gray",
   "*XmMessageBox*foreground: black",
   "*XmMessageBox*fontList: *helvetica-bold-r-normal--12*",

   "*.FileSelector*background: gray",
   "*.FileSelector*foreground: black",
   "*.FileSelector*fontList:*helvetica-bold-r-normal--12*",
   "*.FileSelector*marginWidth: 5",
   "*.FileSelector*marginHeight: 5",
   "*.FileSelector*listVisibleItemCount: 30",
   "*.FileSelector*borderWidth: 0",
   "*.FileSelector*doubleClickInterval: 500",

   "*.Prompt*marginHeight: 4",
   "*.Prompt*marginWidth: 6",
   "*.Prompt*fontList: *helvetica-bold-r-normal--12*",
   "*.ClickboxForm*fontList:*helvetica-bold-r-normal--12*",
   "*.MultiClickboxForm*fontList:*helvetica-bold-r-normal--12*",
   "*.List.XmLabel.fontList:*helvetica-bold-r-normal--12*",
   "*.Menu3DForm.XmLabel.fontList:*helvetica-bold-r-normal--12*",
   "*.drawing_area.background:gray",   
   "*.GraphForm*fontList:*helvetica-bold-r-normal--12*",

   "*.f101.acceleratorText: Ctrl+O",
   "*.f102.acceleratorText: Ctrl+S",
   "*.f103.acceleratorText: Ctrl+P",
   "*.f104.acceleratorText: Ctrl+A",
   "*.f106.acceleratorText: Ctrl+E",
   "*.f108.acceleratorText: Ctrl+N",
   "*.f112.acceleratorText: Alt+X",
   "*.f203.acceleratorText: Alt+B",
   "*.f212.acceleratorText: Ctrl+B",
   "*.f213.acceleratorText: Ctrl+U",
   "*.f214.acceleratorText: Ctrl+R",
   "*.f301.acceleratorText: Ctrl+F",
   "*.f304.acceleratorText: Ctrl+D",
   "*.f305.acceleratorText: Ctrl+T",
   "*.f312.acceleratorText: Ctrl+I",
   "*.f314.acceleratorText: Shift+Enter",
   "*.f400.acceleratorText: Ctrl+C",
   "*.f402.acceleratorText: Ctrl+M",
   "*.f405.acceleratorText: Ctrl+V",
   "*.f502.acceleratorText: Ctrl+L",
   "*.f507.acceleratorText: F2",
   "*.f508.acceleratorText: F3",

   "Interactionmode.labelString: Interaction Mode",
   "*XmArrowButton.background: gray",   
   NULL
};


//--------------------------------------------------------------------//

int main(int argc, char **argv)
{
   int i, argc2=argc;
   char **argv2 = new char*[argc2];
   char title[256];
   g.initializing = 1;
   check_diagnose(argc,argv);
   for(i=0;i<argc2;i++) argv2[i] = strdup(argv[i]);
   g.appname = argv[0];
   initialize_globals();
   readsettings(0);
   sprintf(title, "Image Measurement and Analysis Lab (tnimage %s)", g.version);
   if(g.diagnose){ printf("Setting XtLanguageProc\n");fflush(stdout); }
   XtSetLanguageProc(NULL,NULL,NULL);
   if(g.diagnose){ printf("Initializing Xt resources...");fflush(stdout); }
   g.main_widget = XtVaAppInitialize(&g.app, "Tnimage", 
                        NULL, 0, &argc, argv, (char**)fallback_resources,  
                        XmNdeleteResponse, XmDO_NOTHING,
                        XmNtitle, title,
                        XmNwidth, g.total_xsize,
                        XmNheight, g.total_ysize,
                        NULL);

   add_wm_close_callback(g.main_widget, (XtCBP)main_unmapcb, (XtP)NULL);
   if(signal(SIGSEGV, sig_quit) == SIG_ERR) 
       fprintf(stderr,"Can't setup signal handler\n");
   if(signal(SIGABRT, sig_abort) == SIG_ERR) 
       fprintf(stderr,"Can't setup signal handler\n");
   init_xlib(); 
   resize_widget(g.main_widget, g.total_xsize, g.total_ysize);
   set_motif_colors();
   makewidgets(g.main_widget);
   create_drawing_area(g.main_widget);
   setup_display();
   setup_colors();
   xminitializemisc(g.main_widget);
   create_main_icon();
   init_data();
   handle_command_line(argc2,argv2);    // Take care of app-specific commands
   reset_user_buttons();
   XtAppAddTimeOut(g.app,1000,timer_callback,NULL);
   g.initializing = 0;
   XtAppMainLoop(g.app);
}


//--------------------------------------------------------------------//
// create_main_icon                                                   //
// Set the icon in case user iconifies program. Need a bigger one.    //
//--------------------------------------------------------------------//
void create_main_icon(void)
{
   if(g.diagnose){ printf("Creating icons...");fflush(stdout); }
   Window win;
   XIconSize *size;
   int count, k, ok=0;
   int alloc1=0, n;
   Arg args[100];
   Pixmap icon;
   const uchar icon_data[] = {
   0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
   0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
   0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
   0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,

   0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
   0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
   0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
   0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,

   0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
   0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
   0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
   0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,

   0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
   0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
   0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
   0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,

   };

   win = RootWindow(g.display,DefaultScreen(g.display));

   ////  Make sure it is ok to create a 32x32 icon. XGetIconSizes returns 0
   ////  if preferred icon sizes are not set. Should ideally iterate through 
   ////  size[k].width_inc and size[k].height_inc.

   if(XGetIconSizes(g.display, win, &size, &count))
   {   alloc1=1;
       for(k=0;k<count;k++)  
       {    if(between(32, size[k].min_width, size[k].max_width) &&
               between(32, size[k].min_height, size[k].max_height)){ ok=1; break;}
       }
   }else ok=1;
   if(ok)
   {    icon = XCreateBitmapFromData(g.display, win, (char*)icon_data, 32, 32);
        n=0;
        if(icon != (Pixmap)None)
        {    XtSetArg(args[n], XmNiconic, False); n++;
             XtSetArg(args[n], XmNiconName, "tnimage"); n++;
             XtSetArg(args[n], XmNiconPixmap, icon); n++;
        }else fprintf(stderr, "Can't create icon\n");
        XtSetValues(g.main_widget, args, n);
   }else fprintf(stderr, "Not allowed to create a 32x32 icon!\n");
   if(alloc1) XFree(size);
   if(g.diagnose){ printf("done\n");fflush(stdout); }
}



//--------------------------------------------------------------------//
// set_motif_colors                                                   //
// Call before makewidgets(), otherwise Motif can't allocate its own  //
// colors and retaliates by making everything black & white. After    //
// widgets are set up, then it is safe to allocate application colors.//                       //
// main_fcolor and main_bcolor are set here.                          //
//--------------------------------------------------------------------//
void set_motif_colors(void)
{ 
  if(g.diagnose){ printf("Setting Motif colors...");fflush(stdout); }
  long uint aa, main_top_shadow, main_bottom_shadow, main_select_color;

  ////  Find out what colors Motif will insist on using. Motif has to
  ////  grab its colors before you allocate any colors, otherwise it will
  ////  complain and set everything to black & white.

  ////  Read the foreground & background pixel values chosen by Motif.
  XtVaGetValues(g.main_widget, 
      XmNbackground, &g.main_bcolor,
      XmNforeground, &g.main_fcolor,  NULL);         

  ////  Let Motif calculate the other colors based on the background value.
  XmGetColors(XtScreen(g.main_widget), g.colormap, g.main_bcolor, &aa, 
      &main_top_shadow, &main_bottom_shadow, &main_select_color);
  g.main_fcolor = aa;

  ////  Set the colors accordingly.
  XtVaSetValues(g.main_widget,
      XmNtopShadowColor, main_top_shadow,
      XmNbottomShadowColor, main_bottom_shadow,
      XmNarmColor, main_select_color,
      XmNborderColor, g.main_fcolor,
      NULL);

  if(g.diagnose)
  { printf("done\n main fcolor %d\n main bcolor %d\n",g.main_fcolor,g.main_bcolor); }

}


//--------------------------------------------------------------------//
// Make widgets                                                       //
// Call after set_motif_colors and before setup_colors.               //
// Widgets that exist for the life of the application.                //
//--------------------------------------------------------------------//
void makewidgets(Widget parent)
{ 
  if(g.diagnose){ printf("Creating menu widgets\n");fflush(stdout); }
  int k,n;
  Arg args[100];
  Widget menupane, main_window;

  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;

  main_window = XmCreateMainWindow(parent, (char*)"Main", args, n);

  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  g.menubar = XmCreateMenuBar(main_window, (char*)"MenuBar", args, n);

  //------Create 'File' pull-down menu---------------//

  g.menupane[0] = menupane = createmenu(g.menubar, "fileMenu", " File ", 'F');
  createmenuitem(menupane, "f101",  "Open Image...", 'O', executecb, "load");
  createmenuitem(menupane, "f102",  "Save Image...", 'S', executecb, "save");
  createmenuitem(menupane, "f103",  "Print Image...", 'P', executecb, "print");
  createmenuseparator(menupane, "sep1");
  createmenuitem(menupane, "f104",  "Scanner...", 'S', executecb, "acquire");
  createmenuitem(menupane, "f105",  "Camera...", 'C', executecb, "camera");
  createmenuitem(menupane, "f106",  "Execute plugin...", 'E', executecb, "executeplugin");
  createmenuseparator(menupane, "sep3");
  createmenuitem(menupane, "f107",  "Change title", 'T', executecb, "changetitle");
  createmenuitem(menupane, "f108",  "New/Resize Image...", 'N', executecb, "createimage");
  createmenuitem(menupane, "f108a", "Grab screen", 'G', executecb, "grabfromscreen");
  createmenuitem(menupane, "f109",  "Create File Format...", 'R', executecb, "createfileformat");
  createmenuseparator(menupane, "sep4");
  createmenuitem(menupane, "f110", "Close Image", 'L', executecb, "unload");
  createmenuitem(menupane, "f111", "Close All Images", 'A', executecb, "unloadall");
  createmenuitem(menupane, "f112", "Quit", 'Q', quitcb, NULL);

  //------Create 'Image' pull-down menu--------------//

  g.menupane[1] = menupane = createmenu(g.menubar, "imageMenu", " Image ", 'I');
  createmenuitem(menupane, "f201", "Delete region", 'D', executecb, "deleteregion");
  createmenuitem(menupane, "f202", "Crop", 'C', executecb, "crop");
  createmenuitem(menupane, "f203", "Erase background", 'E', executecb, "erasebackground");
  createmenuitem(menupane, "f204", "Paste", 'P', executecb, "paste");
  createmenuseparator(menupane, "sep5");
  createmenuitem(menupane, "f205", "Change size (reduplicate)", 'C', executecb, "changesizereduplicate");
  createmenuitem(menupane, "f205a","Change size (interpolate)", 'C', executecb, "changesizeinterpolate");
  createmenuitem(menupane, "f206", "Zoom", 'Z', executecb, "zoom");
  createmenuitem(menupane, "f221", "Unzoom", 'o', executecb, "unzoom");
  createmenuitem(menupane, "f207", "Rotate...", 'R', executecb, "rotate");
  createmenuitem(menupane, "f207", "Floating magnifier...", 'F', executecb, "floatingmagnifier");
  createmenuitem(menupane, "f208", "Warp...", 'W', executecb, "warp");
  createmenuitem(menupane, "f221", "Image registration...", 'G', executecb, "registration");
  createmenuitem(menupane, "f209", "Shift...", 'S', executecb, "shift");
  createmenuitem(menupane, "f209", "Chromatic aberration...", 'S', executecb, "chromaticaberration");
  createmenuitem(menupane, "f210", "Flip horiz.", 'F', executecb, "fliphoriz");
  createmenuitem(menupane, "f211", "Flip vertically", 'V', executecb, "flipvertically");
  createmenuseparator(menupane, "sep6");
  createmenuitem(menupane, "f212", "Backup", 'B', executecb, "backup");
  createmenuitem(menupane, "f213", "Restore (Undo)", 'U', executecb, "restore");
  createmenuitem(menupane, "f214", "Repair", 'P', executecb, "repair");
  createmenuitem(menupane, "f221", "Reset program", 'T', executecb, "reinitialize");
  createmenuseparator(menupane, "sep6a");
  createmenuitem(menupane, "f218","Image properties...", 'I', executecb, "attributes");
  createmenuitem(menupane, "f219","Frame Controls...", 'N', executecb, "3d");
#ifdef HAVE_XBAE
  createmenuitem(menupane, "f220","Spreadsheet...", 'p', executecb, "spreadsheet");
#endif
  //------Create 'Process' pull-down menu------------//

  g.menupane[2] = menupane = createmenu(g.menubar, "processMenu", " Process ", 'P');
  createmenuitem(menupane, "f301", "Filter...", 'F', executecb, "filter");
  createmenuitem(menupane, "f313", "Mask...", 'A', executecb, "mask");
  createmenuitem(menupane, "f302", "Measure...", 'E', executecb, "measure");
  createmenuitem(menupane, "f303", "Calibration...", 'C', executecb, "calibration");
  createmenuitem(menupane, "f304", "Spot densitometry..", 'S', executecb, "spotdensitometry");
  createmenuitem(menupane, "f305", "Strip densitometry..", 'D', executecb, "stripdensitometry");
  createmenuitem(menupane, "f317", "Curve densitometry..", 'U', executecb, "curvedensitometry");
  createmenuitem(menupane, "f306", "Trace curve...", 'T', executecb, "tracecurve");
  createmenuitem(menupane, "f307", "FFT/convolution...", 'T', executecb, "fft");
  createmenuitem(menupane, "f308", "Wavelets...", 'W', executecb, "wavelets");
  createmenuseparator(menupane, "sep6b");
  createmenuitem(menupane, "f309", "Partition", 'R', executecb, "partition");
  createmenuitem(menupane, "f310", "Grain/pattern counting...", 'G', executecb, "grains");
  createmenuitem(menupane, "f316", "Morphological Analysis...", 'O', executecb, "morphfilter");
  createmenuseparator(menupane, "sep6b");
  createmenuitem(menupane, "f312", "Macro/image math...", 'M', executecb, "macro");
  createmenuitem(menupane, "f314", "Repeat last command", 'L', executecb, "repeat");
  createmenuitem(menupane, "f315", "3D plot...", '3', executecb, "3dplot");
  createmenuitem(menupane, "f317", "Animate...", '3', executecb, "animate");
  createmenuseparator(menupane, "sep6b");
  createmenuitem(menupane, "f315", "Put image on root window", '3', executecb, "rootpermanent");

  //------Create 'Color' pull-down menu--------------//

  g.menupane[3] = menupane = createmenu(g.menubar, "colorMenu", " Color ", 'C');
  createmenuitem(menupane, "f400", "Change colors..", 'C', executecb, "contrast");
  createmenuitem(menupane, "f401", "Brightness..", 'V', executecb, "brightness");
  createmenuitem(menupane, "f401a","Gamma..", 'A', executecb, "gamma");
  createmenuitem(menupane, "f402", "Contrast..", 'H', executecb, "changecontrast");
  createmenuitem(menupane, "f403", "Colormap/False color...", 'O', executecb, "colormap");
  createmenuitem(menupane, "f404", "Remap colors...", 'R', executecb, "remapcolors");
  createmenuitem(menupane, "f405", "Invert colors", 'I',  executecb, "invertcolors");
  createmenuitem(menupane, "f406", "Separate RGB", 'S',  executecb, "separatergb");
  createmenuitem(menupane, "f407", "Composite RGB", 'B',  executecb, "compositergb");
  createmenuseparator(menupane, "sep6c");
  createmenuitem(menupane, "f408", "Change image depth", 'D',  executecb, "changecolordepth");
  createmenuseparator(menupane, "sep6c");
  createmenuitem(menupane, "f408a", "Change gray values..", 'C', executecb, "graycontrast");
  createmenuitem(menupane, "f409", "Change pixel value..",'P',executecb, "changepixelvalues");
  createmenuitem(menupane, "f411", "Grayscale map...", 'M', executecb, "grayscalemap");
  createmenuitem(menupane, "f412", "Color->gray scale", 'Y',  executecb, "color->grayscale");
  createmenuitem(menupane, "f413", "Gray scale->color", 'G',  executecb, "grayscale->color");
  createmenuseparator(menupane, "sep6d");
  createmenuitem(menupane, "f414", "Histogram", 'H',  executecb, "histogram");
  createmenuitem(menupane, "f415", "Histogram equalization", 'E',  executecb, "histogramequalize");

  //------Create 'Draw' pull-down menu---------------//

  g.menupane[4] = menupane = createmenu(g.menubar, "drawMenu", " Draw ", 'D');
  createmenuitem(menupane, "f500", "Foreground color", 'F', executecb, "foregroundcolor");
  createmenuitem(menupane, "f501", "Background color", 'B', executecb, "backgroundcolor");
  createmenuitem(menupane, "f500a","Update from..", 'G', executecb, "updatecolor");
  createmenuitem(menupane, "f501a","Foreground pattern", 'N', executecb, "foregroundpattern");
  createmenuitem(menupane, "f502", "Label...", 'L',  executecb, "label");
  createmenuitem(menupane, "f503", "Line/arrow...", 'I',  executecb, "line");
  createmenuitem(menupane, "f504", "Circle/ellipse...", 'C',  executecb, "circle");
  createmenuitem(menupane, "f504a","Cross", 'X',  executecb, "cross");
  createmenuitem(menupane, "f505", "Box",'B',  executecb, "box");
  createmenuitem(menupane, "f506", "Curve...", 'U',  executecb, "curve");
  createmenuitem(menupane, "f507", "Sketch", 'S',  executecb, "sketch");
  createmenuitem(menupane, "f507a", "Sketch math", 'S',  executecb, "sketchmath");
  createmenuseparator(menupane, "sep6a");
  createmenuitem(menupane, "f513", "Font...", 'F',  executecb, "font");
#ifdef HAVE_XFT
  createmenuitem(menupane, "f513", "Freetype Font...", 'F',  executecb, "freetypefont");
#endif
  createmenuitem(menupane, "f513a","Special character...", 'R',  executecb, "specialcharacter");
  createmenuitem(menupane, "f514", "Text direction", 'T',  executecb, "textdirection");
  createmenuseparator(menupane, "sep8");
  createmenuitem(menupane, "f416", "Add/remove Gradient...", 'R', executecb, "removegradient");
  createmenuitem(menupane, "f515", "Flood Fill...", 'G',  executecb, "fill");
  createmenuitem(menupane, "f516", "Paint region", 'P',  executecb, "paint");
  createmenuitem(menupane, "f517", "Spray...", 'Y',  executecb, "spray");
  createmenuitem(menupane, "f518", "Add border", 'O',  executecb, "border");

 //------Create 'Select' pull-down menu---------------//

  g.menupane[5] = menupane = createmenu(g.menubar, "selectMenu", " Select ", 'S');
  createmenuitem(menupane, "f508", "Scissors area selection", 'C', executecb, "selectarea");
  createmenuitem(menupane, "f509", "Reselect previous area", 'R', executecb, "reselectarea");
  createmenuitem(menupane, "f510","Switch selected/unselected", 'W', executecb, "switchselected");
  createmenuitem(menupane, "f510a","Show alpha channel", 'A', executecb, "showalpha");
  createmenuitem(menupane, "f511","Clear alpha channel", 'A', executecb, "clearalpha");
  createmenuitem(menupane, "f512","Show selected region", 'H', executecb, "showselected");
  createmenuitem(menupane, "f605", "Alternate between images",'B',  executecb, "alternate");
  createmenuitem(menupane, "f604", "Select image...",'S',  executecb, "selectimage");
  createmenuseparator(menupane, "sep7");


 //------Create 'About' pull-down menu---------------//

  g.menupane[6] = menupane = createmenu(g.menubar, "aboutMenu", " About ", 'A');
  createmenuitem(menupane, "f601", "About the program", 'P', executecb, "abouttheprogram");
  createmenuitem(menupane, "f602", "About the file", 'F',  executecb, "aboutthefile");
  createmenuitem(menupane, "f603", "About the image",'I',  executecb, "abouttheimage");
  createmenuseparator(menupane, "sep9");
  createmenuitem(menupane, "f604", "Image notes",'N',  executecb, "imagenotes");

  //----Create 'Config' pull-down menu---------------//

  g.menupane[7] = menupane = createmenu(g.menubar, "configMenu", " Config ", 'O');
  createmenuitem(menupane, "f701", "Show colormap", 'S',  executecb, "showcolormap");
  createmenuitem(menupane, "f702", "Show O.D. table", 'O',  executecb, "showodtable");
  createmenuitem(menupane, "f701", "Show color palette", 'S',  executecb, "showcolorpalette");
  createmenuitem(menupane, "f703", "Pixel interact mode",'P',  executecb, "pixelinteractmode");
  createmenuitem(menupane, "f704", "Line style...", 'L',  executecb, "linestyle");
  createmenuitem(menupane, "f705", "Configure...", 'C',  executecb, "configure");

  //------Create 'Help' pull-down menu---------------//

  g.menupane[8] = menupane = createmenu(g.menubar, "helpMenu", " Help ", 'H');
  createmenuitem(menupane, "f801", "Help", 'H',  executecb, "help");

  for(k=0; k<10; k++) g.torn_off[k] = 0;  
  XtManageChild(g.menubar);
  XtManageChild(main_window);
  XtRealizeWidget(parent);
  XtMapWidget(parent);

  if(g.diagnose){ printf("done\n"); }
}


//--------------------------------------------------------------------//
// createmenu                                                         //
//--------------------------------------------------------------------//
Widget createmenu(Widget parent, const char *name, const char *label, char mnemonic)
{

    if(g.diagnose){ printf(" Creating menu %s\n",label); }
    int n;
    Arg args[100];
    Widget menu,w;
    XmString xmstring;
#ifdef MOTIF2
    XmString xms;
#endif
    char accelerator[256] = "Ctrl<Key>X";  // Substitute mnemonic for the 'X'
    accelerator[9] = mnemonic;

    menu = XmCreatePulldownMenu(parent, (char*)name, NULL, 0);
    XtVaSetValues(menu, 
        XmNtearOffModel, XmTEAR_OFF_ENABLED, 
        XmNmenuAccelerator, accelerator,            
        XmNaccelerator, accelerator,            
        XmNacceleratorText, "A",            
#ifdef MOTIF2
        XmNtearOffTitle, xms=XmStringCreateSimple((char*)label),
#endif
        XmNmnemonic, mnemonic,            
        NULL);

#ifdef MOTIF2
    XmStringFree(xms);
#endif
    XtAddCallback(menu, XmNtearOffMenuActivateCallback, (XtCBP)tearoff_activatecb, NULL);
    XtAddCallback(menu, XmNtearOffMenuDeactivateCallback, (XtCBP)tearoff_deactivatecb, NULL);
    n = 0;
    XtSetArg(args[n], XmNsubMenuId, menu); n++;
    XtSetArg(args[n], XmNmnemonic, mnemonic); n++;
    XtSetArg(args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED); n++;
    XtSetArg(args[n], XmNlabelString, xmstring=XmStringCreateSimple((char*)label)); n++;
    w = XmCreateCascadeButton(parent, (char*)"cascadebutton", args, n);
    XtManageChild(w);
    XmStringFree(xmstring);
    return menu;
}


//--------------------------------------------------------------------//
// createmenuitem                                                     //
//--------------------------------------------------------------------//
Widget createmenuitem(Widget parent, const char *name, const char *label,
	char mnemonic, 
        void (* callback)(Widget w, XtP client_data, XmACB *call_data),
        const char *cbArg)
{
    Widget w;
    XmString xmstring;
    char accelerator[256] = "Ctrl<Key>X";  
    accelerator[9] = mnemonic;
    
    w = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent, 
    	    XmNlabelString, xmstring=XmStringCreateSimple((char*)label),         
    	    XmNmnemonic, mnemonic, NULL);
    XtVaSetValues(w, XmNaccelerator, accelerator, NULL);
    XtAddCallback(w, XmNactivateCallback, (XtCBP)callback, (void *)cbArg);
    XmStringFree(xmstring);
    return w;
}


//--------------------------------------------------------------------//
// createmenuseparator                                                //
//--------------------------------------------------------------------//
Widget createmenuseparator(Widget parent, const char *name)
{
    Widget button;
    button = XmCreateSeparator(parent, (char*)name, NULL, 0);
    XtManageChild(button);
    return button;
}


//--------------------------------------------------------------------//
// create drawing area                                                //
// Must be called after read_settings() and init_xlib()               //
// so it uses correct sizes.                                          //
//--------------------------------------------------------------------//
void create_drawing_area(Widget parent)
{ 
  if(g.diagnose){ printf("Creating drawing area...");fflush(stdout); }
  int k, n, x1, y1;
  XGCValues xgcv;
  Widget form, arrow_frame, separator;
  Widget button[4];                     // Arrow buttons on left
  Widget arrow_area;
  Arg args[100];
  int data[10];
  int x[4] = {-15,-15,-45,15};
  int y[4] = {2,32,17,17};

  int type[4] = {XmARROW_UP, XmARROW_DOWN, XmARROW_LEFT, XmARROW_RIGHT};
  void *cb[4];
  cb[0] = (void*)upcb;// = {upcb, downcb, leftcb, rightcb};
  cb[1] = (void*)downcb;
  cb[2] = (void*)leftcb;
  cb[3] = (void*)rightcb;

  g.main_mask =  KeyPressMask |         // Keyboard
                StructureNotifyMask |
                SubstructureNotifyMask |
                ColormapChangeMask |
                EnterWindowMask |
                LeaveWindowMask ;   
  g.mouse_mask = Button1MotionMask |    // Click & Drag
                PointerMotionMask;      // Mouse movement  
 

  ////  Note - XmNheight puts data into a float on some platforms 
  ////  and an int on others.  No compiler warning occurs if you put 
  ////  the data into the wrong type. 'mainmenuheight' must be defined
  ////  as a Dimension and then type casted as needed.
     
  XtVaGetValues(g.menubar, XmNheight, &g.mainmenuheight, NULL);       
  y1 = (int)g.mainmenuheight;
  x1 = g.drawarea2width;

 //------------------- drawing area form------------------------------//

  n=0;
  XtSetArg(args[n], XmNwidth, g.main_xsize + x1); n++;
  XtSetArg(args[n], XmNheight, g.main_ysize); n++; // leave space for title at bottom
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNy, y1); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, g.menubar); n++;
  form = XmCreateForm(parent, (char*)"drawing_area_form", args, n);
  XtManageChild(form);

  //-------------------separator--------------------------------------//

  n=0;
  XtSetArg(args[n], XmNx, x1); n++; 
  XtSetArg(args[n], XmNseparatorType, XmSHADOW_ETCHED_IN); n++;
  XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  separator = XmCreateSeparator(form, (char*)"separator", args, n);
  XtManageChild(separator);
  XtRealizeWidget(separator);

  //--------------drawing area 2 (at left)----------------------------//

  n=0;
  XtSetArg(args[n], XmNtitle, "Drawing Area"); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNwidth, x1 - 5); n++;
  XtSetArg(args[n], XmNrightWidget, separator); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  g.drawing_area2 = XmCreateDrawingArea(form, (char*)"drawing_area2", args, n);
  XtRealizeWidget(g.drawing_area2);
  XtManageChild(g.drawing_area2);
  XtMapWidget(g.drawing_area2); 

  //--------------------add buttons at left----------------------------//

  for(k=0;k<g.nbuttons;k++) 
  {   g.button[k]->widget = add_mainbutton(g.button[k]->label, k, (void*)mainbuttoncb);
  }

 //-------------------add information areas---------------------------//

  int ystart = y1 + 55 + 20*(g.nbuttons/2);
  g.info_area[0] = create_info_area(g.drawing_area2,"info_area0",ystart,190);
  ystart += 190;
  g.info_area[1] = create_info_area(g.drawing_area2,"info_area1",ystart,92);
  ystart += 92;
  g.info_area[2] = create_info_area(g.drawing_area2,"info_area2",ystart,56);
  ystart += 56;
  g.info_area[3] = create_info_area(g.drawing_area2,"info_area3",ystart,90);


 //--------------------drawing area (at right)------------------------//

  n=0;
  XtSetArg(args[n], XmNtitle, "Drawing Area"); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
           // Don't map window with call to XtRealizeWidget
  XtSetArg(args[n], XmNmappedWhenManaged, False); n++;
  XtSetArg(args[n], XmNx, x1 + 5); n++;
  XtSetArg(args[n], XmNwidth, g.main_xsize); n++;
  XtSetArg(args[n], XmNheight, g.main_ysize); n++;
           // Margin width must be 0, otherwise images will be moved by
           // Motif automatically at random times, causing pixels to be
           // drawn in the wrong location.
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, separator); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;

  g.drawing_area = XmCreateDrawingArea(form, (char*)"drawing_area", args, n);
  XtManageChild(g.drawing_area);


  XtAddCallback(g.drawing_area, XmNexposeCallback, (XtCBP)drawcb, (XtP)&g.image_gc);
  XtAddCallback(g.drawing_area, XmNresizeCallback, (XtCBP)drawcb, (XtP)&g.image_gc);
  XtAddCallback(g.drawing_area, XmNinputCallback, (XtCBP)drawcb, (XtP)&g.image_gc);

  for(k=0;k<INFOWINDOWS;k++)
     XtAddCallback(g.info_area[k], XmNexposeCallback, (XtCBP)infocb, (XtP)&g.image_gc);

  ////  Need event handler to catch those wacky mouse movement events.
  ////  This is removed in quit().

  XtAddEventHandler(g.drawing_area, g.mouse_mask, False, (XtEH)mousecb, 
      (XtP)&g.image_gc);
  ////  Moving main window    
  XtAddEventHandler(g.main_widget, StructureNotifyMask, False, 
      (XtEH)configurecb, (XtP)NULL);

  ////  Stop information area from getting keyboard focus when clicked
  for(k=0;k<INFOWINDOWS;k++)
      XtSetKeyboardFocus(g.info_area[k], g.drawing_area);

 //--------------------title area-------------------------------------//

  n=0;
  XtSetArg(args[n], XmNtitle, "Title Area"); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNy, g.main_ysize-15); n++;
  XtSetArg(args[n], XmNwidth, g.main_xsize); n++;
  XtSetArg(args[n], XmNheight, 15); n++;

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, separator); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightWidget, form); n++;
  g.title_area = XmCreateDrawingArea(form, (char*)"title_area", args, n);
  XtRealizeWidget(g.title_area);
  XtManageChild(g.title_area);
  XtMapWidget(g.title_area); 

 //--------------------arrow buttons (at left)------------------------//

  n=0;
  XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, 0); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNwidth, x1-4); n++;
  XtSetArg(args[n], XmNheight, 65); n++;
  XtSetArg(args[n], XmNresizable, False); n++;
  arrow_frame = XmCreateFrame(g.drawing_area2, (char*)"arrowframe", args, n);
  XtManageChild(arrow_frame);

  n=0;
  XtSetArg(args[n], XmNtitle, "Arrow Area"); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNwidth, x1 - 4); n++;
  XtSetArg(args[n], XmNheight, 65); n++;
  arrow_area = XmCreateDrawingArea(arrow_frame, (char*)"information_area", args, n);
  XtManageChild(arrow_area);


  for(k=0;k<4;k++)
  {  n=0;
     XtSetArg(args[n], XmNx, x1/2 + x[k] -2); n++;
     XtSetArg(args[n], XmNy, y[k]-2); n++;
     XtSetArg(args[n], XmNheight, 30); n++;
     XtSetArg(args[n], XmNwidth, 30); n++;
     XtSetArg(args[n], XmNarrowDirection, type[k]); n++;
     XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
     button[k] = XmCreateArrowButton(arrow_area, (char*)"Button", args, n);
     data[0] = k;
     XtAddCallback(button[k], XmNdisarmCallback, (XtCBP)disarmcb, (XtP)data);
     XtAddCallback(button[k], XmNarmCallback,(XtCBP)cb[k],(XtP)data);

     ////  Stop arrow buttons from getting keyboard focus when clicked
     XtSetKeyboardFocus(button[k], g.drawing_area);
     XtManageChild(button[k]);
  }

 //-------------------------------------------------------------------//

  ////  Must realize widgets before getting window ID
  XtRealizeWidget(g.drawing_area);
  XtRealizeWidget(arrow_frame);
  XtRealizeWidget(arrow_area);
  XtRealizeWidget(separator);


  for(k=0;k<INFOWINDOWS;k++)
  {   XtRealizeWidget(g.info_area[k]);
      g.info_window[k] = XtWindow(g.info_area[k]);
  }
  g.main_window = XtWindow(g.drawing_area);

  ////  Create a graphics context for images
  g.image_gc = XCreateGC(g.display, g.main_window, 0L,(XGCValues*)NULL);
  XSetFont(g.display, g.image_gc, g.image_font_struct->fid);
  XSetForeground(g.display, g.image_gc, BlackPixel(g.display, g.screen));
  XSetBackground(g.display, g.image_gc, WhitePixel(g.display, g.screen));
  g.main_window = XtWindow(g.drawing_area);

  ////  Create a graphics context for menus, graphs, etc
  g.gc = XCreateGC(g.display,g.info_window[0],0L,(XGCValues*)NULL);
  XSetFont(g.display, g.gc, g.fontstruct->fid);
  XSetForeground(g.display, g.gc, BlackPixel(g.display, g.screen));
  XSetBackground(g.display, g.gc, WhitePixel(g.display, g.screen));

  ////  Create a graphics context for XOR'ing in menus, graphs, etc
  xgcv.foreground = g.fcolor ^ g.bcolor;
  xgcv.background = g.bcolor;
  xgcv.function = GXxor;
  g.xor_gc = XCreateGC(g.display, g.info_window[0], 
  GCForeground|GCBackground|GCFunction, &xgcv);
  XSetForeground(g.display, g.xor_gc, BlackPixel(g.display, g.screen));
  XSetBackground(g.display, g.xor_gc, WhitePixel(g.display, g.screen));

  //----------Now map and manage all the Widgets----------------------//

  for(k=0;k<INFOWINDOWS;k++)
      XtManageChild(g.info_area[k]);
  XtMapWidget(parent);
  XtMapWidget(g.drawing_area); 
  XtMapWidget(separator); 
  XtMapWidget(arrow_frame); 
  XtMapWidget(arrow_area); 
  for(k=0;k<INFOWINDOWS;k++) XtMapWidget(g.info_area[k]); 
  if(g.diagnose){ printf("done\n"); }

}


//--------------------------------------------------------------------//
// create_info_area                                                   //
//--------------------------------------------------------------------//
Widget create_info_area(Widget parent, const char *title, int ystart, int height)
{
  int n;
  Widget w,frame;
  n=0;
  Arg args[20];
  int x1 = g.drawarea2width - 4;

  XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, ystart); n++;
  XtSetArg(args[n], XmNwidth, x1); n++;
  XtSetArg(args[n], XmNheight, height); n++;
  frame = XmCreateFrame(parent, (char*)"frame", args, n);

  //// Info areas (inside frame)

  n=0;
  XtSetArg(args[n], XmNtitle, title); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNwidth, x1); n++;
  XtSetArg(args[n], XmNheight, height); n++;
  w = XmCreateDrawingArea(frame, (char*)title, args, n);
  XtRealizeWidget(frame);
  XtManageChild(frame);
  XtMapWidget(frame); 
  return w;
}


//--------------------------------------------------------------------//
//  add_mainbutton                                                    //
//--------------------------------------------------------------------//
Widget add_mainbutton(char *name, int buttonno, void *cb)
{
  static int y=70;
  Widget w;
  Arg args[100];
  int n,x;
  if((buttonno/2)*2 == buttonno) x=4; else x=60;
  n=0;
  XtSetArg(args[n], XmNx, x); n++;
  XtSetArg(args[n], XmNy, y); n++;
  XtSetArg(args[n], XmNheight, 20); n++;
  XtSetArg(args[n], XmNwidth, 55); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrecomputeSize, False); n++;
  XtSetArg(args[n], XmNtraversalOn, True); n++;
#ifdef MOTIF2
  XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_NONE); n++;
#else
  XtSetArg(args[n], XmNindicatorOn, False); n++;
#endif
  w = XmCreateToggleButton(g.drawing_area2, name, args, n);
  g.button[buttonno]->button = buttonno;
  g.button[buttonno]->widget = w;
  g.button[buttonno]->state  = OFF;

  XtAddCallback(w, XmNvalueChangedCallback, (XtCBP)cb, (XtP)&g.button[buttonno]->button);
  XtManageChild(w);
  if((buttonno/2)*2 != buttonno) y += 20;
  return w;
}


//--------------------------------------------------------------------//
//  add_togglebutton                                                  //
//--------------------------------------------------------------------//
Widget add_togglebutton(Widget parent, char *name, int x, int y, void *cb, 
   void *ptr)
{
  Widget w;
  Arg args[100];
  int n;
  n=0;
  XtSetArg(args[n], XmNleftPosition, (Dimension)x); n++;      // % of width fromleft
  XtSetArg(args[n], XmNrightPosition, (Dimension)x+10); n++;  // % of width fromleft
  XtSetArg(args[n], XmNtopPosition, (Dimension)y); n++;       // % of height from top
  XtSetArg(args[n], XmNheight, 20); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrecomputeSize, False); n++;
  XtSetArg(args[n], XmNtraversalOn, True); n++;
#ifdef MOTIF2
  XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_NONE); n++;
#else
  XtSetArg(args[n], XmNindicatorOn, False); n++;
#endif
  w = XmCreateToggleButton(parent, name, args, n);
  XtAddCallback(w, XmNvalueChangedCallback, (XtCBP)cb, ptr);
  XtManageChild(w);
  return w;
}


//--------------------------------------------------------------------//
//  add_pushbutton - generic button for form widget                   //
//--------------------------------------------------------------------//
Widget add_pushbutton(char *label, Widget parent, int x1, int y1, int x2, 
    int y2, clickboxinfo *cb)
{
  Widget w;
  Arg args[100];
  int n;
  n=0;
  XtSetArg(args[n], XmNleftPosition, (Dimension)x1); n++;   // % of width fromleft
  XtSetArg(args[n], XmNrightPosition, (Dimension)x2); n++;  // % of width fromleft
  XtSetArg(args[n], XmNtopPosition, (Dimension)y1); n++;    // % of height from top
  XtSetArg(args[n], XmNbottomPosition, (Dimension)y2); n++; // % of width from top
  XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  w = XmCreatePushButton(parent, label, args, n);
  XtAddCallback(w, XmNactivateCallback, (XtCBP)buttoncb, (XtP)cb);
  XtManageChild(w);
  return w;
}


//--------------------------------------------------------------------//
// imagewidget                                                        //
// Create a drawing area widget for images. Later this can be extended//
// to add resizing and other features.                                //
//--------------------------------------------------------------------//
Widget imagewidget(int xpos, int ypos, int xsize, int ysize, ImageStruct *i)
{
   Arg args[100];
   int n;
   Widget w, parent;
   
   ////  Use `bitmap' to create icon
   const uchar icon_data[] = {
   0x54, 0x01, 0xa8, 0x02, 0x50, 0x05, 0xa0, 0x0a, 
   0x50, 0x15, 0xa8, 0x2a, 0x14, 0x55, 0x0a, 0xaa, 
   0x05, 0x55, 0x82, 0x2a, 0x45, 0x15, 0xaa, 0x0a,
   0x54, 0x05, 0xa8, 0x02, 0x54, 0x01, 0xaa, 0x00,
   };
   
   ////  Margin width must be 0, otherwise images will be moved by
   ////  Motif automatically at random times, causing pixels to be
   ////  drawn in the wrong location.

   ////  Create a Form Dialog Shell if user wants all images in 
   ////  separate windows

   n=0;
   XtSetArg(args[n], XmNtitle, i->name); n++;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, ysize); n++;

   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   if(i->shell)
   {  
        w = XmCreateFormDialog(g.main_widget, i->name, args, n);
        XtRealizeWidget(w);
        XtManageChild(w);
        XtMapWidget(w);        

        n=0;
        XtSetArg(args[n], XmNwidth, xsize); n++;
        XtSetArg(args[n], XmNheight, ysize); n++;
        if(i->shell && i->scrollbars)
        {   XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++; }
        else
        {  XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); n++; }
        parent = XmCreateScrolledWindow(w, i->name, args, n);
        XtRealizeWidget(parent);
        XtManageChild(parent);
        XtMapWidget(parent);        
        XtAddCallback(w, XmNunmapCallback, (XtCBP)imageunmapcb, (XtP)&i->widget);
   }else
   {    XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
        XtSetArg(args[n], XmNx, xpos); n++;
        XtSetArg(args[n], XmNy, ypos); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
        parent=g.drawing_area;
   }

// These 3 lines prevent compiling in new versions for unknown reason.
//        XtSetArg(args[n], XmNselectScrollVisible, True); n++;
//        XtSetArg(args[n], XmNhorizontalScrollBarDisplayPolicy, XmDISPLAY_STATIC); n++;
//        XtSetArg(args[n], XmNverticalScrollBarDisplayPolicy, XmDISPLAY_STATIC); n++;
   w = XmCreateDrawingArea(parent, (char*)"image", args, n);

           ////  If no window frame, move widget to the right place
           ////  and make sure the main window is on top.
   if(i->shell && !i->window_border)
   {    XtMoveWidget(XtParent(parent), xpos, ypos);
        XRaiseWindow(g.display,XtWindow(g.main_widget));
   }
   if(i->shell )
   {  
        XtAddEventHandler(w, StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
        XtAddEventHandler(XtParent(parent), StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
        XtAddEventHandler(XtParent(XtParent(parent)), StructureNotifyMask, False, (XtEH)resize_scrolled_imagecb, (XtP)z);
   }

           ////  Expose events = redraw part of the image
   XtAddCallback(w, XmNexposeCallback, (XtCBP)image_exposecb, (XtP)z);
           ////  Keypress or mouse click on the image
   XtAddCallback(w, XmNinputCallback, (XtCBP)drawcb, (XtP)z);
           ////  Mouse events    
   XtAddEventHandler(w, g.mouse_mask, False, (XtEH)mousecb, (XtP)z);
           ////  Moving window. Event handlers must be removed in eraseimage().
   XtAddEventHandler(XtParent(XtParent(w)), StructureNotifyMask, False, 
       (XtEH)configurecb, (XtP)z);
           ////  If user clicks on frame, event handler is needed to switch
           ////  to the image. Event handlers must be removed in eraseimage().
           ////  Clicking on window is handled by mousecb.
   XtAddEventHandler(XtParent(XtParent(w)), FocusChangeMask, False, 
       (XtEH)imagefocuscb, (XtP)z);

           ////  Map widget
   XtRealizeWidget(w);
   XtMapWidget(w); 
   XtManageChild(w);

           ////  Add icon
   i->icon = XCreateBitmapFromData(g.display, RootWindow(g.display, 
        DefaultScreen(g.display)), (char*)icon_data, 16, 16);
   if(i->icon != (Pixmap)None)
   {    n=0;
        XtSetArg(args[n], XmNiconic, False); n++;
        XtSetArg(args[n], XmNiconName, "Icon"); n++;
        XtSetArg(args[n], XmNiconPixmap, i->icon); n++;
        XtSetValues(XtParent(parent), args, n);
   }else fprintf(stderr, "Can't create icon\n");
   return w;
}



//--------------------------------------------------------------------------//
// add_button                                                               //
//--------------------------------------------------------------------------//
Widget add_button(Widget parent, char *s1, int x, int y, int buttonwidth)
{
   Arg args[100];
   Cardinal n;
   Widget button;
   n=0;
   XtSetArg(args[n],XmNresizable,       False); n++;
   XtSetArg(args[n],XmNtopAttachment,   XmATTACH_NONE); n++;
   XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
   XtSetArg(args[n],XmNleftAttachment,  XmATTACH_FORM); n++;
   XtSetArg(args[n],XmNrightAttachment, XmATTACH_NONE); n++;
   XtSetArg(args[n],XmNwidth,           buttonwidth); n++;
   XtSetArg(args[n],XmNleftOffset,      x); n++;
   XtSetArg(args[n],XmNbottomOffset,    y); n++;
   XtSetArg(args[n],XmNdefaultButton,   True); n++;
   if(x<50)
   {   XtSetArg(args[n],XmNshowAsDefault,   True); n++; }
   else
   {   XtSetArg(args[n],XmNshowAsDefault,   False); n++; }
   XtSetArg(args[n],XmNsensitive,       True); n++;
   XtSetArg(args[n],XmNnavigationType,  XmEXCLUSIVE_TAB_GROUP); n++;
   XtSetArg(args[n],XmNinitialFocus,    True); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   button = XmCreatePushButton(parent,s1,args,n); 
   XtManageChild(button);
   return button;
}



//--------------------------------------------------------------------------//
// send_event                                                               //
// Artificially send an Expose event to a window. Each type of event uses   //
// a different event structure.                                             //
//--------------------------------------------------------------------------//
void send_expose_event(Window w, int eventtype, int x, int y, int width, int height)
{
   XExposeEvent event;
   event.type = eventtype;
   event.send_event = True;
   event.display = g.display;
   event.window = w;
   event.x = x;  
   event.y = y;  
   event.width = width;  
   event.height = height;
   event.count = 0;
   XSendEvent(g.display, w, False, NoEventMask, (XEvent *)&event);
   
   XmDrawingAreaCallbackStruct xdacbs;
   XEvent ee;
   ////  In case rotating image is on root window
   if(w==g.root_window && eventtype == Expose)
   {   ee.xexpose = event;
       xdacbs.reason = XmCR_EXPOSE;
       xdacbs.event = &ee;
       image_exposecb(z[ci].widget, (XtP)NULL, (XtP*)(&xdacbs)); 
   }
}


//--------------------------------------------------------------------------//
// send_buttonpress_event                                                   //
// Artificially send a buttonpress event to a window. Each type of event    //
// uses a different event structure.                                        //
//--------------------------------------------------------------------------//
void send_buttonpress_event(Window w, int eventtype, int x, int y, int state, int button)
{
   XButtonEvent event;
   event.type = eventtype;
   event.send_event = True;
   event.display = g.display;
   event.window = w;
   event.x = x;  
   event.y = y;  
   event.state = state;
   event.button = button;
   XSendEvent(g.display, w, True, ButtonPressMask, (XEvent *)&event);
}


//--------------------------------------------------------------------------//
// check_event  -  waits for events without blocking.                       //
//--------------------------------------------------------------------------//
void check_event(void)
{
   while(XtAppPending(g.app)) XtAppProcessEvent(g.app,XtIMAll);
   send_expose_event(g.main_window, NoExpose, 0,0,1,1);
}


//--------------------------------------------------------------------------//
// block -  block(w, &want_block);  ...                                     //
//          want_block = 0;    = unblocks                                   //
//--------------------------------------------------------------------------//
void block(Widget w, int *want_block)
{
   w = g.main_widget; // Set to main widget to ensure it returns
   XtAppContext xac = XtWidgetToApplicationContext(w);
   XSync(g.display, 0);
   int ogetout = g.getout;

   //// Do not remove this line or it will crash if you close the dialog box
   //// while program is waiting for input.
   g.block++;

   while(*want_block && g.getout<=ogetout) XtAppProcessEvent(xac, XtIMAll);
   g.block = max(0, g.block-1);
   XSync(g.display, 0);
}


//--------------------------------------------------------------------//
// block_modal -  block(w, &want_block);  ...                         //
//                want_block = 0;    = unblocks                       //
//--------------------------------------------------------------------//
void block_modal(Widget w, int *want_block)
{
   Widget previous_focus;
   previous_focus = XmGetFocusWidget(w);
   XmProcessTraversal(w, XmTRAVERSE_CURRENT);
   XtAddGrab(w, True, False); 
   XtAppContext app = XtWidgetToApplicationContext(w);
   while(*want_block) XtAppProcessEvent(app, XtIMAll);
   XSync(XtDisplayOfObject(w), 0);
   XmUpdateDisplay(w);
   XtRemoveGrab(w);
   XmProcessTraversal(previous_focus, XmTRAVERSE_CURRENT);
}


//--------------------------------------------------------------------//
// redrawscreen - redraws main window including images on main window //
//--------------------------------------------------------------------//
void redrawscreen(void)
{
  copybackground(1,1,g.main_xsize-1,g.main_ysize-1,-1); 
}


//--------------------------------------------------------------------//
// redraw  - redraws an image                                         //
//--------------------------------------------------------------------//
void redraw(int ino)
{
  if(g.ignore || ino<0) return;
  if(!between(ino,0,g.image_count-1)) return;
  if(XtIsManaged(z[ino].widget))
      send_expose_event(z[ino].win, Expose, 0, 0, 1+z[ino].xscreensize, 1+z[ino].yscreensize);
  if(z[ino].chromakey)     
      redraw_chromakey_image(ino, 0);
  if(z[ino].transparency)     
      redraw_transparent_image(ino, 0);
}


//--------------------------------------------------------------------//
// init_xlib                                                          //
// Initialize miscellaneous Xlib functions.                           //
// Call after read_settings so want_colormaps & colors are known.     //
//--------------------------------------------------------------------//
void init_xlib(void)
{
  XVisualInfo          vis_template;
  XVisualInfo         *vis_list;
  int                  num_visuals; 
  int                  win_depth;  // depth of visual
  Window               rwin;       // Window ID of root
  int                  depth;      // Bits/pixel of server
  uint uw,uh,ubw,udepth;           // width, height, bdr.width, & depth of root window
  int k,xroot,yroot;
  int bitmapunit;

  if(g.diagnose){ printf("done\nInitializing Xlib...");fflush(stdout); }

  //-------Define the type of visual desired here---------------------//

  int depth_wanted;
  g.display = XtDisplay(g.main_widget);

  //------Set up fonts------------------------------------------------//

  if((g.fontstruct=XLoadQueryFont(g.display, g.font)) == NULL) 
  {   fprintf(stderr, "%s: display %s cannot load font %s\ntrying fixed font",
              g.appname, DisplayString(g.display), g.font);
      strcpy(g.font, "fixed");
      if((g.fontstruct=XLoadQueryFont(g.display, g.font)) == NULL) 
      {   fprintf(stderr, "%s: display %s cannot load fixed font\n",
              g.appname, DisplayString(g.display));
          exit(1);
      } 
  } 

  if((g.image_font_struct = XLoadQueryFont(g.display, g.imagefont)) == NULL) 
  {   fprintf(stderr, "%s: display %s cannot load font %s\n",
              g.appname, DisplayString(g.display), g.imagefont);
      if((g.image_font_struct = XLoadQueryFont(g.display, g.font)) == NULL) 
          fprintf(stderr, "%s: display %s cannot load font %s\ntrying fixed font",
              g.appname, DisplayString(g.display), g.font);
      {   strcpy(g.font, "fixed");
          if((g.image_font_struct = XLoadQueryFont(g.display, g.font)) == NULL) 
          {   fprintf(stderr, "%s: display %s cannot load fixed font\n",
                  g.appname, DisplayString(g.display));
              exit(1);
          } 
      }
  }  

  //------------------------------------------------------------------//

  g.screen = DefaultScreen(g.display); 
  g.root_window = RootWindow(g.display,g.screen);
  vis_template.screen = g.screen;

  //------Get a list of visuals for this screen-----------------------//

  depth_wanted = DefaultDepth(g.display, g.screen);
  depth = depth_wanted;

  vis_list = XGetVisualInfo(g.display, VisualScreenMask, &vis_template, &num_visuals);  
  if(g.visual_id_wanted==0 && g.visual_bpp_wanted==0)
      g.visual = DefaultVisual(g.display, g.screen);
  else if(g.visual_id_wanted)
  {   for(k=0;k<num_visuals;k++)
         if(vis_list[k].visualid == (uint)g.visual_id_wanted) g.visual=vis_list[k].visual;
  }
  else if(g.visual_bpp_wanted)
  {   for(k=0;k<num_visuals;k++)
         if(vis_list[k].depth == g.visual_bpp_wanted) g.visual=vis_list[k].visual;
  }

  //// There are several device dependencies that must be tested for:
  ////    byte_order in XImage
  ////    bitmap_bit_order in XImage
  ////    rgb vs. gbr order in Visual
  //// This gives 8 possible ways the bytes can be arranged in X windows.
  
  if(g.visual->red_mask < g.visual->blue_mask) g.swap_red_blue = 1;
  else g.swap_red_blue = 0;

  int vid = g.visual->visualid;
  if(num_visuals==0) { fprintf(stderr,"No visuals found\n"); exit(1); }
  if(g.diagnose)
  {  printf("\n%d Visuals found\n",num_visuals);
     printf("Display type: ");
     switch(g.visual->c_class)    // Use c_class in C++, class if using C
     {   case PseudoColor: printf("Pseudo color (0x%x)\n", vid); break;
         case GrayScale:   printf("Gray scale (0x%x)\n", vid);   break;
         case DirectColor: printf("Direct color (0x%x)\n", vid); break;
         case TrueColor:   printf("True color (0x%x)\n", vid);   break;
         default:          printf("Other (0x%x)\n", vid);        break;
      }
     printf("swap red/blue %d\n", g.swap_red_blue);
     printf("red %lx grn %lx blu %lx\n",
             g.visual->red_mask, 
             g.visual->green_mask, 
             g.visual->blue_mask );
  }
  XFree(vis_list);      

  win_depth     = depth; 
  bitmapunit    = depth;
  if(win_depth>=24) bitmapunit=BitmapUnit(g.display);  // Actual depth for xserver

  if(g.diagnose)
      printf("bitmap_unit %d\ndepth %d %d \n",bitmapunit,depth,g.nosparse);

  //----Set the colormap to system default----------------------------//

  g.screen = DefaultScreen(g.display);
  g.colormap = DefaultColormap(g.display, g.screen);
  g.bitsperpixel  = depth;
  ////  Calculates bpp by creating an Ximage then examining its struct
  depth = bits_per_pixel();   
  g.bitsperpixel  = depth;
  if(g.bitsperpixel>=32 && win_depth==24) g.sparse_packing=1;
  if(g.bitsperpixel == 8 && 
    (g.visual->c_class == TrueColor || g.visual->c_class == DirectColor))
    {  g.bitsperpixel = 7; 
       g.want_colormaps = 0;
       printf("8-bit TrueColor mode, yuck!!!!!!!\n");  // VNC uses this
    }

  //// Allow user to override in case of server bug
  if(g.sparse){ g.sparse_packing=1; g.bitsperpixel=32; }
  if(g.nosparse){ g.sparse_packing=0; g.bitsperpixel=24; } 
  g.ximage_byte_order = ImageByteOrder(g.display);
  if(g.want_byte_order != -1) g.ximage_byte_order = g.want_byte_order;

  XGetGeometry(g.display,g.root_window,&rwin,&xroot,&yroot,&uw,&uh,&ubw,&udepth);
  g.xres = (int) uw;         // xres of root screen
  g.yres = (int) uh;         // yres of root screen

  g.main_xsize = min(g.main_xsize, g.xres-1);
  g.main_ysize = min(g.main_ysize, g.yres-1);
  g.total_xsize = min(g.total_xsize, g.xres-1);
  g.total_ysize = min(g.total_ysize, g.yres-1);

  if(g.diagnose)
  {   printf("done\n");
      printf(" xres        %d\n",g.xres);
      printf(" yres        %d\n",g.yres);
      printf(" main xsize  %d\n",g.main_xsize);
      printf(" main ysize  %d\n",g.main_ysize);
      printf(" total xsize %d\n",g.total_xsize);
      printf(" total ysize %d\n",g.total_ysize);
      printf(" depth       %d\n",depth);
      printf(" bitmapunit  %d\n",bitmapunit);
      printf(" sparse_pack %d\n",g.sparse_packing);
  }
}


//--------------------------------------------------------------------//
// bits_per_pixel                                                     //
//--------------------------------------------------------------------//
int bits_per_pixel(void)
{
/*   The canonically correct way
  XPixmapFormatValues *xpfv;
  int count, k, bpp=0;
  if(g.bitsperpixel==15) return 15;
  xpfv = XListPixmapFormats(g.display, &count); 
  for(k=0;k<count;k++) bpp = max(bpp, xpfv[k].bits_per_pixel);
  XFree(xpfv);
  return bpp;
*/
  ////  This way seems safer
  int bitmap_pad = 8;   // Data is byte-organized
  int bytesperline=0;   // 0 means: scan lines are contiguous - X server
                        // calculates it for you 
  int xsize=10;
  int ysize=10;
  int bpp=g.bitsperpixel;
  char *buf;
  if(bpp==15) return 15;
  buf = (char*)malloc(1200);  // Must use malloc because XDestroyImage deallocates it "for" you.
  int offset = 0;
  XImage *ximage;
  ximage = XCreateImage(g.display, g.visual, (uint)bpp, ZPixmap, offset,
      buf, xsize, ysize, bitmap_pad, bytesperline);
  if(ximage==NULL){ printf("Cant create test image\n"); exit(1); }
  bpp = ximage->bits_per_pixel;
  XDestroyImage(ximage);
  return bpp;

}


//--------------------------------------------------------------------//
// xminitializemisc                                                   //
// Initialize miscellaneous Xm functions.                             //
// Must be called after create_drawing_area() so g.main_window &      //
//   g.display are defined.                                           //
// Must be called after init_xlib so g.fontstruct is defined.         //
//--------------------------------------------------------------------//
void xminitializemisc(Widget parent)
{
  Pixmap p, q;
  XColor fc, bc;
  char data[256];
  if(g.diagnose){ printf("Initializing Motif fonts...");fflush(stdout); }

  ////  This function is supposed to be obsolete in Motif 2.0 but Irix and
  ////  Solaris are still using 1.x 

  memset(data, 0, 256);
  g.fontlist = XmFontListCreate(g.fontstruct, XmSTRING_DEFAULT_CHARSET);
  g.normal_cursor = XCreateFontCursor(g.display, g.xlib_cursor);
  g.fleur_cursor = XCreateFontCursor(g.display, XC_fleur);
  g.busy_cursor = XCreateFontCursor(g.display, XC_watch);
  g.left_cursor = XCreateFontCursor(g.display, XC_left_side);
  g.right_cursor = XCreateFontCursor(g.display, XC_right_side);
  g.up_cursor = XCreateFontCursor(g.display, XC_top_side);
  g.down_cursor = XCreateFontCursor(g.display, XC_bottom_side);
  g.grab_cursor = XCreateFontCursor(g.display, XC_dotbox);
  p = createxpixmap(g.main_window, 16, 16, 1);
  q = XCreatePixmapFromBitmapData(g.display, g.main_window, data, 16, 16, 1, 0, 1);
  g.no_cursor = XCreatePixmapCursor(g.display, p, q, &fc, &bc, 0, 0);
  XFreePixmap(g.display, p);
  XFreePixmap(g.display, q);

  if(g.want_crosshairs)
      g.cursor = g.no_cursor;
  else
      g.cursor = g.normal_cursor;
  g.cursor_symbol = XC_cross;  
  XDefineCursor(g.display,g.main_window,g.cursor);

  ////  Try to determine window border
  XWindowAttributes xwa;
  XGetWindowAttributes(g.display, XtWindow(parent), &xwa);
  g.wm_leftborderwidth = xwa.x;
  g.wm_topborderwidth = xwa.y;

  ////  Get rid of those pesky error messages that spew out on the screen
  ////  all the time.
  if(!g.diagnose) 
     XtAppSetWarningHandler(XtWidgetToApplicationContext(parent), warningmessagecb);
  if(g.diagnose)
  {    printf("done\n");
       printf("left border width %d\n",g.wm_leftborderwidth);
       printf("top border width  %d\n",g.wm_topborderwidth);
  }
}



//--------------------------------------------------------------------//
// next_unused_color                                                  //
// Finds the lowest color that is not marked as being reserved, sets  //
// its r,g, and b values, returns pixel no.                           //
// Returns 0 if all colors are reserved.                              //
//--------------------------------------------------------------------//
int next_unused_color(int red, int grn, int blu)
{  
    int k, color=0;
    for(k=0;k<256;k++) if(g.reserved[k]==0){ color=k;break; }
    g.palette[color].red = red;
    g.palette[color].green = grn;
    g.palette[color].blue = blu;
    return color;
}


//--------------------------------------------------------------------//
// setup_colors - must be called after setup_display                  //
// Find out how many colors are already used by other apps.           //
// They are not necessarily continuous.                               //
//--------------------------------------------------------------------//
void setup_colors(void)
{    
  if(g.diagnose){ printf("Allocating colors...");fflush(stdout); }
  XColor color;
  int count=256, k, status=0;
  ulong plane_masks[256];
  ulong pixels[256];
  int menubarfcolor=0,menubarbcolor=0; // Motif colors for top menu bar
  int widgetfcolor=0, widgetbcolor=0;  // Motif colors for widgets
  int drawarea2fcolor=0, drawarea2bcolor=0;
  int main_bcolor=0, main_fcolor=0, main_top_shadow=0, main_bottom_shadow=0, 
      main_select_color=0, border_fcolor=0;

  if(g.bitsperpixel != 8 || g.visual->c_class!=PseudoColor)
  {   g.want_colormaps = 0;
      return;
  }

  //// Start with gray scale palette

  setSVGApalette(g.bkgpalette,0);

  //// Try to allocate all but 32 colors
  //// Main window's background & foreground colors
  //// Use white background in case of failure     
  
  if(g.fcolor==0 && g.bcolor==0)
  {   if(XParseColor(g.display, g.colormap,"gray", &color)==0 ||
               XAllocColor(g.display, g.colormap, &color)==0)
          g.bcolor = WhitePixel(g.display, g.screen);
      else
          g.bcolor = color.pixel;

      if(XParseColor(g.display, g.colormap, "black", &color)==0 ||
               XAllocColor(g.display, g.colormap, &color)==0)
          g.fcolor = BlackPixel(g.display, g.screen);
      else
          g.fcolor = color.pixel;
  }
  if(g.fcolor==0 && g.bcolor==0)
  {   g.want_colormaps=1;
      fprintf(stderr,"Using colormaps\n");
  }

  ////  Read the default colormap
  ////  Note: The rgb values range from 0 to 65535.
 
  for(k=0;k<256;k++)
  {   g.def_colors[k].pixel = k;
      g.def_colors[k].red   = 0;
      g.def_colors[k].green = 0;
      g.def_colors[k].blue  = 0;
      g.def_colors[k].flags = DoRed | DoGreen | DoBlue;
  } 
  XQueryColors(g.display, g.colormap, g.def_colors,256);

  status=0;
  count = 256;
  if(!g.want_colormaps)
  {                    ////  Find out how many colors can be allocated  
      while(status==0 && count>=32)
      {   status = XAllocColorCells(g.display,g.colormap,0,plane_masks,
               0,pixels,--count);
      }
      if(count<=32)    ////  Leave 32 colors for everyone else
      {    fprintf(stderr,"\nLess than 32 free color cells available\n");
           fprintf(stderr,"Using modifiable colormaps\n");
           g.want_colormaps=2;
      }else
      if(count<=128)   ////  Annoy them if less than 127 free
      {   fprintf(stderr,"\nWarning: only %d free color cells available\n",count);
          fprintf(stderr,"Recommend configuring tnimage to use colormaps\n");
      }
      if(status==0) count=0;  // Not able to allocate any colors
  }else count=0;

  ////  Array 'pixels' now contains a sorted list of available pixels.
  ////  Mark pixels allocated by other apps as reserved.
  ////  If unable to get any colors, or user wants to use colormaps,
  ////  set all the colors free.
  ////  After the last entry (i.e., the entry for which pixels[k] is
  ////  255), 'pixels[]' contains garbage.

  if(g.want_colormaps)
  {     if(count>0)
        {   fprintf(stderr,"Freeing %d colors \n",count); 
            XFreeColors(g.display, g.colormap, pixels, count, 0);
        }
        for(k=0;k<256;k++) g.reserved[k]=0;

        XtVaGetValues(g.menubar,
              XmNbackground, &menubarbcolor,
              XmNforeground, &menubarfcolor, NULL);

        ////  These two should be same as main_fcolor & main_bcolor unless 
        ////  user changed it.

        XtVaGetValues(g.info_area[0],   
              XmNbackground, &widgetbcolor,
              XmNforeground, &widgetfcolor, NULL); 
        if(g.fcolor==g.bcolor){ g.fcolor=0; g.bcolor=144; }
        g.reserved[0]=1;
        g.reserved[1]=1;
        g.reserved[menubarfcolor]=1;
        g.reserved[menubarbcolor]=1;
        g.reserved[widgetfcolor]=1;
        g.reserved[widgetbcolor]=1;
        XtVaGetValues(g.drawing_area2,   
              XmNbackground, &drawarea2bcolor,
              XmNforeground, &drawarea2fcolor, NULL); 
        g.reserved[drawarea2bcolor]=1;
        g.reserved[drawarea2fcolor]=1;

        XtVaGetValues(g.main_widget, 
              XmNbackground, &main_bcolor,
              XmNforeground, &main_fcolor, 
              XmNtopShadowColor, &main_top_shadow,
              XmNbottomShadowColor, &main_bottom_shadow,
              XmNarmColor, &main_select_color,
              XmNborderColor, &border_fcolor,
              NULL);         
        g.reserved[main_bcolor]=1;
        g.reserved[main_fcolor]=1;
        g.reserved[g.main_bcolor]=1;
        g.reserved[g.main_fcolor]=1;
        g.reserved[main_top_shadow]=1;
        g.reserved[main_bottom_shadow]=1;
        g.reserved[main_select_color]=1;
        g.reserved[border_fcolor]=1;

  }else
  {     XtVaGetValues(g.main_widget,
              XmNforeground, &g.main_fcolor,
              XmNbackground, &g.main_bcolor,
              NULL);       
        count = 0;
        for(k=0;k<256;k++) g.reserved[k]=1;
        for(k=0;k<256;k++)
        {   if(pixels[k]<256) 
            {     g.reserved[pixels[k]]=0;
                  count++;
            }
            if(pixels[k]>=255) break; 
        }
        if(g.bitsperpixel==8)
        {  if(g.reserved[g.fcolor])
           {   fprintf(stderr,"\nfcolor %d is already used by another application\n",
                   g.fcolor);
               if((g.fcolor = next_unused_color(g.fc.red, g.fc.green, g.fc.blue)))
                   fprintf(stderr,"fcolor is now set to %d \n",g.fcolor);
           }    

           if(g.reserved[g.bcolor])
           {   fprintf(stderr,"\nbcolor %d is already used by another application\n",g.bcolor);
               if((g.bcolor = next_unused_color(g.bc.red, g.bc.green, g.bc.blue)))
                   fprintf(stderr,"bcolor is now set to %d \n",g.bcolor);
           }    
        }
  }    
       
  ////  Set the palette to use when user clicks on background area.

  memcpy(g.b_palette,g.palette,768);
  if(g.diagnose)
  {   printf("done\n");
      printf("  colormaps %d\n",g.want_colormaps);
      printf("  colormaps %d\n",g.want_colormaps);
      printf("  count     %d\n",count);
      printf("  main fcolor   %d\n",g.main_fcolor);
      printf("  main bcolor   %d\n",g.main_bcolor);
      printf("  menubarfcolor %d\n",menubarfcolor);
      printf("  menubarbcolor %d\n",menubarbcolor);
      printf("  widgetfcolor  %d\n",widgetfcolor);
      printf("  widgetbcolor  %d\n",widgetbcolor);
      printf("  drawarea2fcolor %d\n",drawarea2fcolor);
      printf("  drawarea2bcolor %d\n",drawarea2bcolor);
      if(g.bitsperpixel==8)
      {   printf("  Reserved colors:\n");
          for(k=0;k<256;k++) if(g.reserved[k]) printf("%d ",k); 
          printf("\n");
      }
  }
  if(g.diagnose) printf("done\n");
}



//--------------------------------------------------------------------//
// string_width                                                       //
//--------------------------------------------------------------------//
int string_width(char *string, char *tag)
{
   int width=0;
   XmString xmstring;
   xmstring = XmStringCreateLtoR(string, tag);
   width = XmStringWidth(g.fontlist, xmstring);
   XmStringFree(xmstring);
   return width;
}


//--------------------------------------------------------------------//
// string_height                                                      //
//--------------------------------------------------------------------//
int string_height(char *string, char *tag)
{
   int height=0;
   XmString xmstring;
   xmstring = XmStringCreateLtoR(string, tag);
   height = XmStringHeight(g.fontlist, xmstring);
   XmStringFree(xmstring);
   return height;
}


//--------------------------------------------------------------------//
// addlabel                                                           //
// Print text on a Widget (used in dialog boxes)                      //
// Creates a Widget, don't forget to destroy it.                      //
// 'string' is the labelString resource specified in fallbacks        //
// 'position' can be LEFT, CENTER, or RIGHT.                          //
// Make boundaries as big as possible in case user changes string to  //
//  something wide. If x2-x1 is too small for the string, Motif will  //
//  enlarge the dialog box to gigantic size.                          //
//--------------------------------------------------------------------//
Widget addlabel(Widget parent, char *string, int position, double x1, double y1, 
   double x2, double y2) 
{
   Widget label;
   Arg args[100];
   int n=0;
   XtSetArg(args[n], XmNleftPosition, x1); n++;     // % of width from left
   XtSetArg(args[n], XmNtopPosition, y1); n++;      // % of height from top
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightPosition, x2); n++;  
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomPosition, y2); n++;  
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   switch(position)
   {  case LEFT: XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; break;
      case CENTER: XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++; break;
      case RIGHT: XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++; break;
   }
   label = XmCreateLabel(parent, string, args, n);
   XtManageChild(label);   
   return label;
} 


//--------------------------------------------------------------------//
// addbblabel                                                         //
// Print text on a bb Widget (used in dialog boxes)                   //
//--------------------------------------------------------------------//
Widget addbblabel(Widget parent, char *string, int position, double x, double y) 
{
   Widget label;
   Arg args[100];
   int n=0;
   XtSetArg(args[n], XmNx, x); n++;   
   XtSetArg(args[n], XmNy, y); n++; 
   XtSetArg(args[n], XmNresizable, False); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   switch(position)
   {  case LEFT: XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; break;
      case CENTER: XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++; break;
      case RIGHT: XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++; break;
   }
   label = XmCreateLabel(parent, string, args, n);
   XtManageChild(label);   
   return label;
} 



//--------------------------------------------------------------------//
// check_diagnose                                                     //
// Read command line options that are needed to setup xlib            //
// Test if "-diag" is on command line. If so, set g.diagnose flag.    //
//--------------------------------------------------------------------//
void check_diagnose(int argc, char **argv)
{ 
   int k;
   char tempstring[1024];
   g.diagnose=0;
   g.nosparse=0;
   g.sparse=0;
   g.visual_id_wanted = 0;       // Command line specified visual id
   g.visual_bpp_wanted = 0;      // Command line specified bpp
   for(k=1;k<argc;k++)
   {  strcpy(tempstring,argv[k]);
      strupr(tempstring);
      if(strcmp(tempstring,"-DIAG")==SAME) g.diagnose=1;  
      if(strcmp(tempstring,"-DIAGNOSE")==SAME) g.diagnose=1;  
      if(strcmp(tempstring,"-NOSPARSE")==SAME) g.nosparse=1;
      if(strcmp(tempstring,"-SPARSE")==SAME)   g.sparse=1;
      if(strcmp(tempstring,"-VISUAL")==SAME)
      {    g.visual_id_wanted =  strtoul(argv[k+1], NULL, 16);
           strcpy(argv[k+1], "-");
      }
      if(strcmp(tempstring,"-BPP")==SAME) g.visual_bpp_wanted = atoi(argv[k+1]);
   }
   if(g.diagnose) printf("\nStarting (diagnose mode)\n");
}


//--------------------------------------------------------------------//
// handle_command_line                                                //
// Take care of command-line options (except diagnose flag)           //
//--------------------------------------------------------------------//
int handle_command_line(int argc, char **argv)
{ 
   if(g.diagnose){ printf("Handling command line...");fflush(stdout); }
   int k;
   int status=OK;
   char *macrofile = NULL;
   char *listfilename = NULL;
   char *filename = NULL;
   char tempstring[1024];
   FILE *fp;

   filename = new char[FILENAMELENGTH];
   macrofile = new char[FILENAMELENGTH];
   listfilename = new char[FILENAMELENGTH];
   macrofile[0] = 0; listfilename[0] = 0; filename[0] = 0;
   for(k=1;k<argc;k++)
   {
      strcpy(tempstring,argv[k]);
      strupr(tempstring);
      if(strcmp(tempstring,      "?"       )==SAME) showoptions();  
      else if(strcmp(tempstring, "-?"      )==SAME) showoptions();  
      else if(strcmp(tempstring, "-OPTIONS")==SAME) showoptions();
      else if(strcmp(tempstring, "-FILES"  )==SAME) strcpy(listfilename,argv[++k]); 
      else if(strcmp(tempstring, "-MACRO"  )==SAME) strcpy(macrofile,argv[++k]);
      else if(argv[k][0]!='-')   
      {   strcpy(filename,argv[k]);
          readfiles(filename,0,NULL);         // Read file(s) 
      }
   }
   switch_palette(ci);
   if(status==OK && ci>=0) switchto(ci);

   // If a file list was specified, load all the filenames in the list.
   if(strlen(listfilename))
   {  if((fp=fopen(listfilename,"r"))!=NULL)
      {  while(!feof(fp))
         {   fgets(tempstring,99,fp);
             for(k=0;k<(int)strlen(tempstring);k++) 
                 if((uchar)tempstring[k]<=13) tempstring[k]=0;
             if(strlen(tempstring)>0)
                 readfiles(tempstring,0,NULL);
             else
                 break;   
         }
	 fclose(fp);
      }else error_message(listfilename, errno);
   }
   if(strlen(macrofile)) 
   {   readmacrofile(macrofile, g.macrotext);
       macro(g.macrotext);
   }
   if(g.diagnose) printf("done\n");
   
   delete[] filename;
   delete[] listfilename;
   delete[] macrofile;
   return status;
}


//--------------------------------------------------------------------//
// createximage - set up an XImage structure for Xlib                 //
// buf_1d is an alias for a 2d image buffer.                          //
//--------------------------------------------------------------------//
XImage *createximage(int xsize, int ysize, int bpp, uchar *buf_1d)
{
  int bitmap_pad = 8;   // Data is byte-organized
  int bytesperline = 0; // 0 means: scan lines are contiguous - X server
                        // calculates it for you 
  int offset = 0;
  XImage *ximage;
  if(g.sparse_packing && bpp==32) bpp=24;
  if(bpp==7) bpp=8;
  ximage = XCreateImage(g.display, g.visual, (uint)bpp, ZPixmap, offset,
      (char*)(buf_1d), (uint)xsize, (uint)ysize, bitmap_pad, bytesperline);
  if(ximage==NULL) fprintf(stderr,"XCreateImage failed!\n");
  return ximage;  
}



//--------------------------------------------------------------------//
// createxpixmap - set up a Pixmap structure for Xlib                 //
//--------------------------------------------------------------------//
Pixmap createxpixmap(Window win, int xsize, int ysize, int bpp)
{
  Pixmap pmap;
  if(g.sparse_packing && bpp==32) bpp=24;
  if(bpp==7) bpp=8;
  pmap = XCreatePixmap(g.display, win, xsize, ysize, bpp);
  return pmap;
}


//-------------------------------------------------------------------// 
// copyimage  (Motif) - copy a rectangular region from the screen to //
//  a buffer or from a buffer to the screen.                         //
//                                                                   //
//  If mode = GET it copies from screen to the buffer.               //
//  If mode = PUT it copies from buffer to the screen.               //
//  Copyimage always copies the upper left of the image.             //
//                                                                   //
//  x1,y1     = screen starting coordinates                          //
//  x2,y2     = screen ending coordinates                            //
//  xoff,yoff = offset into buffer to get or put image (pixel units) //
//  mode      = GET or PUT                                           //
//  buf       = an array storing the image at same bpp as screen     //
//  The coordinates are clipped to fit within theWindow, to prevent  //
//    errors from Xlib.                                              //
//  For Xlib version, you have to pass an XImage* and Window.        //
//  In Xlib, this is the slowest routine.                            //
//  In Motif, subtract x and y upper left coordinates of image       //
//     because each image is on a separate window.                   // 
//-------------------------------------------------------------------//
void copyimage(int x1, int y1, int x2, int y2, int xoff, int yoff,
                int mode, XImage *ximage, Window win)
{
   uint uw, uh;
   x1 = max(0,x1);
   y1 = max(0,y1);
   x2 = min(g.main_xsize+xoff,x2);
   y2 = min(g.main_ysize+yoff,y2);
   uw = 1+x2-x1;
   uh = 1+y2-y1;
   if(mode==GET)
   {
      XGetSubImage(g.display, win, x1, y1, 
            uw, uh, XAllPlanes(), ZPixmap, ximage, xoff, yoff);
   }
   if(mode==PUT)
   {
      XPutImage(g.display, win, g.image_gc, ximage, x1, y1,
            xoff, yoff, uw, uh);
   }
}


//--------------------------------------------------------------------//
// blackbox (Xlib)                                                    //
// blank out an area of the screen to specified color.                //
// Not permanent.                                                     //
//--------------------------------------------------------------------//
void blackbox(int x1,int y1,int x2,int y2, uint color, Window win)
{
   uint uw,uh;
   XWindowAttributes xwa;
   int width,height;
     
   if(win==0) win=g.main_window;
   XGetWindowAttributes(g.display, win, &xwa);
   width = xwa.width;
   height = xwa.height;
   x1 = max(0, min(width-1,x1)); 
   y1 = max(0, min(height-1,y1)); 
   x2 = max(0, min(width-1,x2)); 
   y2 = max(0 ,min(height-1,y2)); 
   uw = 1+x2-x1;
   uh = 1+y2-y1;
   uw = max(1,min(width-1,(int)uw)); 
   uh = max(1,min(height-1,(int)uh)); 
   
   XSetForeground(g.display, g.image_gc, (ulong)color);
   XFillRectangle(g.display, win, g.image_gc, x1, y1, uw, uh);
   XSetForeground(g.display, g.image_gc, (ulong)g.fcolor);
}
 

//--------------------------------------------------------------------// 
// setup_display                                                      //
// Initializes data about video resolution                            //
// Uses the global variables: type  display_type                      //
// Call after init_xlib, so that bitsperpixel is defined.             //
//--------------------------------------------------------------------//
void setup_display()
{
    ////  Display formats: 8,15,16,24,32
    int k;
    if(g.diagnose){ printf("Setting up display...");fflush(stdout); }

    switch(g.bitsperpixel)
    {   case 7 :  g.maxcolor=255;
                  g.want_noofcolors=3;
                  g.colortype=COLOR;
                  if(g.fcolor==g.bcolor){ g.fcolor=0; g.bcolor=192; g.bkg_image_color=192; }
                  break;
        case 8 :  g.maxcolor=255;
                  g.want_noofcolors=1;
                  g.colortype=INDEXED;
                  if(g.fcolor==g.bcolor){ g.fcolor=0; g.bcolor=144; g.bkg_image_color=127; }                 
                  break;
        case 15:  if(g.fcolor==g.bcolor)
                  {   g.fcolor=0; 
                      g.line.color=32767;
                      g.bcolor=15855;
                      g.bkg_image_color=15855;
                  }
                  g.maxcolor=32767;
                  g.colortype=COLOR;
                  g.want_noofcolors=3;
                  break;
        case 16:  if(g.fcolor==g.bcolor)
                  {   g.fcolor=0; 
                      g.line.color=65536;
                      g.bcolor=31727;
                      g.bkg_image_color=31727;
                  }
                  g.maxcolor=65535;
                  g.colortype=COLOR;
                  g.want_noofcolors=3;
                  break;
        case 24:  if(g.fcolor==g.bcolor)
                  {   g.fcolor = 0;  
                      g.line.color = 16777215;
                      g.bcolor = RGBvalue(144,144,144,24);              
                      g.bkg_image_color = RGBvalue(144,144,144,24);              
                  }
                  g.colortype=COLOR;
                  g.maxcolor=16777215;
                  g.want_noofcolors=3;
                  break;
        case 32: 
        case 48: if(g.fcolor==g.bcolor)
                  {   g.fcolor=0;  
                      g.line.color=16777215;
                      g.bcolor = RGBvalue(144,144,144,32);              
                      g.bkg_image_color = RGBvalue(144,144,144,32);
                  }
                  g.colortype=COLOR;
                  g.maxcolor = 16777215;
                  g.want_noofcolors=3;
                  break;
    }

    //// Internal storage formats: 8,16,24,32,48   
    //// Pseudo 32 bits/pixel is 8 bpp of r,g, and b & ignores alpha channel.
    //// Images with 9 to 16 bpp of r,g, or b should use 48 bpp.

    g.maxred[7] =  8; g.maxgreen[7] =  8; g.maxblue[7] =  8; g.maxvalue[7] =255.0;
    g.maxred[8] = 63; g.maxgreen[8] = 63; g.maxblue[8] = 63; g.maxvalue[8] =255.0;
    g.maxred[15]= 31; g.maxgreen[15]= 31; g.maxblue[15]= 31; g.maxvalue[15]=32767.0;
    g.maxred[16]= 31; g.maxgreen[16]= 63; g.maxblue[16]= 31; g.maxvalue[16]=65535.0;
    g.maxred[24]=255; g.maxgreen[24]=255; g.maxblue[24]=255; g.maxvalue[24]=16777215.0;
    g.maxred[32]=255;   g.maxgreen[32]=255; g.maxblue[32]=255; g.maxvalue[32]=16777215.0;
    g.maxred[48]=65535; g.maxgreen[48]=65535; g.maxblue[48]=65535; 
       g.maxvalue[48]=2.814749767e14;

    g.maxgray[7] =   8;
    g.maxgray[8] = 255;
    g.maxgray[15]=  31;
    g.maxgray[16]=  31;
    g.maxgray[24]= 255;    
    g.maxgray[32]= 255;
    g.maxgray[48]= 65536;

    ////  For 8bpp, set r,g & b bits/pixel to 6 for 0..63 palette
          
    g.redbpp[7] =3;  g.greenbpp[7] =3;  g.bluebpp[7] =3;  g.blackbpp[7]=0;
    g.redbpp[8] =6;  g.greenbpp[8] =6;  g.bluebpp[8] =6;  g.blackbpp[8]=0;
    g.redbpp[15]=5;  g.greenbpp[15]=5;  g.bluebpp[15]=5;  g.blackbpp[15]=0;
    g.redbpp[16]=5;  g.greenbpp[16]=6;  g.bluebpp[16]=5;  g.blackbpp[16]=0;
    g.redbpp[24]=8;  g.greenbpp[24]=8;  g.bluebpp[24]=8;  g.blackbpp[24]=0;
    g.redbpp[32]=8;  g.greenbpp[32]=8;  g.bluebpp[32]=8;  g.blackbpp[32]=8;
    g.redbpp[48]=16; g.greenbpp[48]=16; g.bluebpp[48]=16; g.blackbpp[48]=0;

    g.want_color_type = g.colortype;
    g.want_bpp        = g.bitsperpixel;
    g.want_redbpp     = g.redbpp[g.bitsperpixel];
    g.want_greenbpp   = g.greenbpp[g.bitsperpixel];
    g.want_bluebpp    = g.bluebpp[g.bitsperpixel];
    g.want_blackbpp   = 0;

    for(k=0;k<64;k++) g.off[k]=0;
    g.off[7]=1;
    g.off[8]=1;
    g.off[15]=2;
    g.off[16]=2;
    g.off[24]=3;
    g.off[32]=4;   
    g.off[48]=6;   

    g.fcolor = min(g.fcolor, g.maxcolor);
    g.line.color = min(g.line.color, g.maxcolor);
    g.bcolor = min(g.bcolor, g.maxcolor);    
    if(g.diagnose)
    {   printf("done \n");
        printf("  color type %d \n",g.colortype);
        printf("  bits/pixel %d \n",g.bitsperpixel);
        printf("  fcolor     %d \n",g.fcolor);
        printf("  bcolor     %d \n",g.bcolor);
        printf("  line color %d \n",g.line.color);
    }
}       


//--------------------------------------------------------------------------//
// main_unmapcb - called when user closes a dialog box using window         //
// manager instead of the dismiss button like they are supposed to.         //
//--------------------------------------------------------------------------//
void main_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
   quitcb(w, client_data, call_data);
}


//--------------------------------------------------------------------//
// quitcb - callback for quitting.                                    //
//--------------------------------------------------------------------//
void quitcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(g.diagnose)
  { 
    if(crashing) printf("in quitcb, crashing=%d\n", crashing);
    else         printf("Quitting...");
    fflush(stdout); 
  }
  w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
  int status=OK;
  status = savetouchedimages();
  if(g.background_touched)
  {   status = message("Save background pixels?",YESNOQUESTION);
      if(status==NO) status=OK;
  }      
  if(status==OK)
  {   XtRemoveEventHandler(g.drawing_area, g.mouse_mask ,False, 
             (XtEH)mousecb, (XtP) &g.image_gc);
      savesettings();
      if(g.diagnose){ printf("done\n"); fflush(stdout); }
      exit(0);
  }
}


//--------------------------------------------------------------------//
// savetouchedimages  - returns OK or ABORT                           //
//--------------------------------------------------------------------//
int savetouchedimages(void)
{  int k;
   int status=OK;
   for(k=1; k<g.image_count; k++) 
   {   if(z[k].touched) status = save_ok(k);
       if(status==ABORT) return ABORT;
   }
   return OK;
}


//--------------------------------------------------------------------//
// save_ok - returns YES if all images have been saved & ok to quit.  //
//           Returns NO if user clicks "no".                          //
//           Returns ABORT if user clicks "cancel"                    //
//--------------------------------------------------------------------//
int save_ok(int ino)
{  
    int status = OK;
    char name[FILENAMELENGTH];
    int answer=YES;
    int helptopic;
    if(memorylessthan(4096)){ message(g.nomemory,ERROR); return ABORT; } 
    char tempstring[FILENAMELENGTH];
    g.getout=0;
    if(!between(ino,0,g.image_count-1)) return(ERROR); 
    if(strlen(z[ino].name) >0 )
       strcpy(name, z[ino].name);
    else
       strcpy(name, "[Untitled]");
    if(crashing)
    {  helptopic = 67;
       if(g.diagnose) printf("save_ok crashing\n");
       sprintf(tempstring,"Program closing due to errors\nSave image #%d? \n(%s)",ino,name);
    }else
    {  helptopic = 1;
       sprintf(tempstring,"Save image #%d? \n(%s)",ino,name);
    }
    switch(message(tempstring, YESNOQUESTION, helptopic))
    {  case YES: 
                 blocked_while_saving = 1;
                 status = saveimage(ino,0,NULL); 
                 //// Must wait until user dismisses save dialog before
                 //// quitting. Also gives user another chance to abort the quit.
                 block(g.main_widget, &blocked_while_saving);
                 answer=YES; 
                 if(g.getout)answer=ABORT;  // Clicked Cancel in saveimage()
                 break;
       case NO:
               answer=NO; break;            // No, dont save but ok to quit
       case CANCEL: answer=ABORT; break;    // Clicked Cancel in message box
    }
    return answer;
}


//--------------------------------------------------------------------//
// drawselectbox                                                      //
// Put a dashed box around the selected area and background tasks     //
// such as drawing figures (called by timer_callback once/second and  //
// whenever mouse moves with left button pressed).                    //
// 'mode' can be ON or OFF.                                           //
//--------------------------------------------------------------------//
void drawselectbox(int mode)
{
   static int *save1=NULL, *save2=NULL, *save3=NULL, *save4=NULL;
   static int s=0,allocated=0;
   static int ox1=0, ox2=0, oy1=0, oy2=0, osquare=0;
   int color,count,i,j,x1,x2,y1,y2,on_main_window; 
   int black = BlackPixel(g.display, g.screen);
   int white = WhitePixel(g.display, g.screen);
   if(g.state==GETBOX)
   {    x1 = g.get_x1;
        x2 = g.get_x2;
        y1 = g.get_y1;
        y2 = g.get_y2;
   }else
   {    x1 = g.selected_ulx;
        x2 = g.selected_lrx;
        y1 = g.selected_uly;
        y2 = g.selected_lry;
   }
   if(x1>x2) swap(x1,x2);
   if(y1>y2) swap(y1,y2);
   if(x1==x2 && y1==y2) return;
   g.inmenu++;
   s++; 
   if(s>=2*g.spacing) s=0;
   crosshairs(0, 0);
   switch(mode)
   {   case ON:
           if(allocated && (x1!=ox1 || x2!=ox2 || y1!=oy1 || y2!=oy2 ))
           {  
               resetselectbox(ox1,oy1,ox2,oy2,save1,save2,save3,save4,osquare);
               allocated=0;  
           }
           if(!allocated) 
           {  
               if(g.selected_is_square)
               {   
                   save1 = new int[x2-x1+2];
                   save2 = new int[x2-x1+2];
                   save3 = new int[y2-y1+2];
                   save4 = new int[y2-y1+2];
                   allocated=1;
                   for(i=x1;i<=x2;i++)
                   {   save1[i-x1] = readpixel(i,y1);
                       save2[i-x1] = readpixel(i,y2);
                   }
                   for(j=y1;j<=y2;j++)
                   {   save3[j-y1] = readpixel(x1,j);
                       save4[j-y1] = readpixel(x2,j);
                   }
                   dashed_box(x1,y1,x2,y2,white,black,g.spacing,SET);
               }else
               {  
                   save1 = new int[(x2-x1+2)*(y2-y1+2)];
                   save2 = NULL;
                   save3 = NULL;
                   save4 = NULL;
                   allocated=1;
                   count=0;
                   for(j=y1;j<=y2;j++)
                   for(i=x1;i<=x2;i++)
                       save1[count++] = readpixel(i,j);
               }
               s=0;
           }else
           {  
              if(g.selected_is_square)
              {  
                 for(i=x1+s; i<x2; i+=2*g.spacing)
                 {   setpixel(i,y1,white,SET);
                     setpixel(i,y2,white,SET);
                     if(i+g.spacing>x2) continue;
                     setpixel(i+g.spacing,y1,black,SET);
                     setpixel(i+g.spacing,y2,black,SET);
                 }
                 for(j=y1+s;j<y2;j+=2*g.spacing)
                 {   setpixel(x1,j,white,SET);
                     setpixel(x2,j,white,SET);
                     if(j+g.spacing>y2) continue;
                     setpixel(x1,j+g.spacing,black,SET);
                     setpixel(x2,j+g.spacing,black,SET); 
                 }
              }else if(between(g.region_ino, 0, g.image_count-1))
              {  
                 on_main_window = (ci>0) && (!z[ci].shell);
                 for(j=y1;j<y2;j++)
                 {  
                     if(on_main_window && !between(j,0,g.main_ysize)) break;
                     for(i=x1;i<x2;i++)
                     {   
                         if(on_main_window && !between(i,0,g.main_xsize)) break;
                         if((s+i>>2)&1)color=white; else color=black;
                         if(inside_irregular_region(i,j)==3) setpixel(i,j,color,SET);
                     }
                 }
              }  
           }
           if(g.floating_magnifier) update_floating_magnifier(g.mouse_x, g.mouse_y);
           break;
       default:
           if(allocated) 
              resetselectbox(ox1,oy1,ox2,oy2,save1,save2,save3,save4,osquare);
           allocated=0;
           break;
   }     
   osquare=g.selected_is_square;
   ox1=x1; ox2=x2; oy1=y1; oy2=y2;
   g.inmenu--;
}


//--------------------------------------------------------------------//
// resetselectbox                                                     //
//--------------------------------------------------------------------//
void resetselectbox(int x1, int y1, int x2, int y2, int *save1, int *save2,
                    int *save3, int *save4, int square)
{
    static int inreset=0;
    if(inreset) return;
    inreset=1;
    int i,j,count=0;
    if(square)
    {   
        g.inmenu++;
        for(i=x1;i<=x2;i++)
        {   setpixel(i,y1,save1[i-x1],SET);
            setpixel(i,y2,save2[i-x1],SET);
        }
        for(j=y1;j<=y2;j++)
        {   setpixel(x1,j,save3[j-y1],SET);
            setpixel(x2,j,save4[j-y1],SET);
        }
        if(save1!=NULL) delete[] save1; save1=NULL;
        if(save2!=NULL) delete[] save2; save2=NULL;
        if(save3!=NULL) delete[] save3; save3=NULL;
        if(save4!=NULL) delete[] save4; save4=NULL;
        g.inmenu--;
    }else
    {   
        if((y2-y1)*(x2-x1) < 16000)  // If small, just put the pixels back
        {  for(j=y1;j<=y2;j++)
           for(i=x1;i<=x2;i++)
                  setpixel(i,j,save1[count++],SET);
        }else if(between(g.region_ino, 0, g.image_count-1))
        {  rebuild_display(g.region_ino);
           redraw(g.region_ino);
        }
        if(save1!=NULL) delete[] save1; save1=NULL;
    }
    inreset=0;
}


//--------------------------------------------------------------------//
// draw - Figure drawing with unsorted coordinates                    //
//--------------------------------------------------------------------//
void draw(int mode)
{
   int dx,dy,ino,bpp,x,y,xx1,xx2,yy1,yy2,zoom,oldoutline;
   double angle;
   static int ox,oy,oxx1,oxx2,oyy1,oyy2,odiameter,ozoom,ocross_length;
   if(g.state==GETBOXPOSITION)
   {    g.get_x1 = g.mouse_x;
        g.get_y1 = g.mouse_y;
        g.get_x2 = g.mouse_x + g.width;
        g.get_y2 = g.mouse_y + g.height;
   }
   xx1 = g.get_x1;
   xx2 = g.get_x2;
   yy1 = g.get_y1;
   yy2 = g.get_y2;
   x = g.mouse_x;
   y = g.mouse_y;  
   int hitinmenu=0;

   ino = whichimg(x,y,bpp);
   if(ino>=0 && z[ino].is_zoomed) zoom=1; else zoom=0;
   g.line.color = g.fcolor;
   if(mode!=END)
   {  switch(g.draw_figure)
      {            // For figures that only set pixels when mode==END:
                   // 'inmenu=1' prevents pixels from sticking on image.
         case LINE:
         case BOX:   
         case ARROW:  
         case CIRCLE: 
         case ELLIPSE: 
         case OVAL: 
         case ROUNDED_RECTANGLE: 
         case RECTANGLE: 
         g.inmenu++; hitinmenu=1; break;
         default:     break;
      }
   }
   switch(g.draw_figure)
   {  case LINE:
         if(mode==START)
            line(xx1,yy1,xx2,yy2,XOR,&g.line);
         if(mode==TNI_CONTINUE)
         {  line(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
            line(xx1,yy1,xx2,yy2,XOR,&g.line);
         }
         if(mode==END)
         { 
            g.inmenu++;
            line(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
            g.inmenu--;
            line(xx1,yy1,xx2,yy2,g.imode,&g.line);
         }
         break;
      case TEMPLINE:
         if(mode==START)
            line(xx1,yy1,xx2,yy2,g.maxcolor,XOR);
         if(mode==TNI_CONTINUE)
         {  line(oxx1,oyy1,oxx2,oyy2,g.maxcolor,XOR);
            line(xx1,yy1,xx2,yy2,g.maxcolor,XOR);
         }
         if(mode==END)
         {  line(oxx1,oyy1,oxx2,oyy2,g.maxcolor,XOR);
            if(g.draw_figure==LINE)  
                line(xx1,yy1,xx2,yy2,g.fcolor,g.imode);
         }
         break;
      case ARROW:
         if(mode==START)
            line(xx1,yy1,xx2,yy2,XOR,&g.line);
         if(mode==TNI_CONTINUE)
         {  line(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
            line(xx1,yy1,xx2,yy2,XOR,&g.line);
         }
         if(mode==END)
         { 
            g.inmenu++;
            line(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
            g.inmenu--;
            ////  Shorten the line so wide lines don't extend past arrow
            angle = atan2(yy2-yy1,xx2-xx1);
            dx = cint(g.line.arrow_inner_length * cos(angle));
            dy = cint(g.line.arrow_inner_length * sin(angle));
            x = xx1+dx;
            y = yy1+dy;
            line(x,y,xx2,yy2,g.imode,&g.line);
            draw_arrow(xx1,yy1,xx2,yy2,&g.line);
            ////  Fix up head-tail junction 
            if(g.line.outline == 1)
            {   oldoutline = g.line.outline;
                g.line.outline = 0;
                line(x-dx/4,y-dy/4,x+dx/4,y+dy/4,g.imode,&g.line);
                g.line.outline = oldoutline;
            }
         }
         break;
      case BOX:      
         if(mode==START)
            box(xx1,yy1,xx2,yy2,XOR,&g.line);
         if(mode==TNI_CONTINUE)
         { 
            box(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
            box(xx1,yy1,xx2,yy2,XOR,&g.line);
         }
         if(mode==END && g.state !=GETBOXPOSITION)
         {   g.inmenu++;
                box(oxx1,oyy1,oxx2,oyy2,XOR,&g.line);
             g.inmenu--;
                box(xx1,yy1,xx2,yy2,g.imode,&g.line);
         }
         if(mode==END && g.state==GETBOXPOSITION) g.state = NORMAL; 
         break;
      case SKETCH:  
         if(mode!=START && zoom==ozoom)
            sketch(ox,oy,x,y,g.fcolor,g.imode,&g.line,0);
         break;
      case SKETCHMATH:  
         if(mode==START)
         {   if(g.sketch_editor_widget)
                  XtVaGetValues(g.sketch_editor_widget,  XmNvalue, &g.sketchtext, NULL);
         } 
         if(mode!=START && zoom==ozoom)
         {  g.getout = 0;   // set to 1 if they close editor window
            sketch(ox,oy,x,y,g.fcolor,g.imode,&g.line,1);
         }
         break;
      case CIRCLE:  
         if(mode==START)
            circle(x,y,g.diameter,g.circlewidth,g.maxcolor,XOR);
         if(mode==TNI_CONTINUE)
         {  circle(ox,oy,odiameter,g.circlewidth,g.maxcolor,XOR);
            circle(x,y,g.diameter,g.circlewidth,g.maxcolor,XOR);   
         }
         if(mode==END)
         {  circle(ox,oy,odiameter,g.circlewidth,g.maxcolor,XOR);
            circle(x,y,g.diameter,g.circlewidth,g.fcolor,g.imode);
         }
         odiameter=g.diameter;
         break;
      case CROSS:  
         if(mode==START)
            cross(x,y,g.cross_length,g.maxcolor,XOR);
         if(mode==TNI_CONTINUE)
         {  cross(ox,oy,ocross_length,g.maxcolor,XOR);
            cross(x,y,g.cross_length,g.maxcolor,XOR);   
         }
         if(mode==END)
         {  cross(ox,oy,ocross_length,g.maxcolor,XOR);
            cross(x,y,g.cross_length,g.fcolor,g.imode);
         }
         ocross_length = g.cross_length;
         break;
      case RECTANGLE:
      case ROUNDED_RECTANGLE:
      case OVAL:
      case ELLIPSE:
         if(mode==START)
            ellipse(xx1,yy1,xx2,yy2,g.circlewidth,g.fcolor,XOR,g.draw_figure);
         if(mode==TNI_CONTINUE)
         {  ellipse(oxx1,oyy1,oxx2,oyy2,g.circlewidth,g.fcolor,XOR,g.draw_figure);
            ellipse(xx1,yy1,xx2,yy2,g.circlewidth,g.fcolor,XOR,g.draw_figure);
         }
         if(mode==END)
         {  g.inmenu++;
            ellipse(oxx1,oyy1,oxx2,oyy2,g.circlewidth,g.fcolor,XOR,g.draw_figure);
            g.inmenu--;
            ellipse(xx1,yy1,xx2,yy2,g.circlewidth,g.fcolor,g.imode,g.draw_figure);
         }
         break;
      default: break; 
   }
   if(hitinmenu) g.inmenu--;
   ozoom=zoom;
   ox=x;oy=y;
   oxx1=xx1;
   oxx2=xx2;
   oyy1=yy1;
   oyy2=yy2;
}


    



//--------------------------------------------------------------------//
// Callbacks                                                          //
//--------------------------------------------------------------------//


//--------------------------------------------------------------------//
// timer_callback                                                     //
// Callback function for asynchronous or automatic events             //
//--------------------------------------------------------------------//
void timer_callback(XtP client_data, XtIntervalId *timer_id)
{  
    static int orx=0, ory=0;
    XEvent event;
    client_data=client_data; timer_id=timer_id;  // Keep compiler quiet
    // Restart timer once every 'DELAY' milliseconds
    int DELAY = 256;
    if(g.state==GETBOX) DELAY=128;
    if(g.moving) DELAY=64;
    if(!g.selected_is_square) DELAY=SLOW_DELAY;  // Remove if your computer is fast enough
    if(XCheckMaskEvent(g.display, g.main_mask | g.mouse_mask, &event))
         XtAppProcessEvent(g.app, XtIMAll);
    if(g.floating_magnifier) 
    {    int rx,ry,wx,wy; 
         uint keys;   
         Window rwin, cwin;
         XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
         //// Only update if not on main window
         if(!cwin && (rx != orx || ry != ory))
         {  if(update_floating_magnifier(g.mouse_x, g.mouse_y)) DELAY=2;   
            orx = rx;
            ory = ry;
         }
    }
    XtAppAddTimeOut(g.app, DELAY, timer_callback, NULL);
    if(g.selectedimage==-1 && g.state==NORMAL) drawselectbox(ON);
    if(g.state==GETBOX) drawselectbox(ON);
    if(g.want_slew && g.state!=BUSY) switch(g.moving)
    {  case 0: break;
       case 1: upcb((Widget)0, (XtP)0, (XtP)0); break;
       case 2: downcb((Widget)0, (XtP)0, (XtP)0); break;
       case 3: leftcb((Widget)0, (XtP)0, (XtP)0); break;
       case 4: rightcb((Widget)0, (XtP)0, (XtP)0); break;
    }
}


//--------------------------------------------------------------------//
// infocb  - handle expose events for information_area                //
//--------------------------------------------------------------------//
void infocb(Widget widget, XtP client_data, XtP *call_data)
{  
   widget=widget;client_data=client_data;call_data=call_data;
   printstatus(g.state);
   printcoordinates(g.mouse_x, g.mouse_y, 1); 
}

 
//--------------------------------------------------------------------//
// drawcb                                                             //
// This is the main callback for events in the main drawing area.     //
// Also handles input events for images.                              //
// Don't use 'widget' or call_data->window for anything, they don't   //
// always point to the correct window.                                //
//--------------------------------------------------------------------//
void drawcb(Widget widget, XtP client_data, XtP *call_data)
{ 
  widget=widget; client_data=client_data; // Keep compiler quiet
  XmDrawingAreaCallbackStruct *ptr;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  int bufsize=20;
  int bpp,iii,key_x, key_y,rx,ry,wx,wy; 
  KeySym key;
  XComposeStatus compose;
  char buffer[20];
  char s[10]="";
  XEvent event;
  XKeyEvent keyevent;
  uint keys=0,keystate;
  Window rwin, cwin;
  if(g.main_window==0) return;

                       // Find current position of theWindow & image number
  XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
  g.main_xpos = rx-wx;
  g.main_ypos = ry-wy;

  if(ptr==NULL) return;
  switch(ptr->reason)
  {  
     case XmCR_RESIZE:
       break;
     case XmCR_EXPOSE:
       printstatus(REDRAW);
       event = *(ptr->event);
       printstatus(g.state);
       break;
     case XmCR_INPUT:
       if(ptr->event==NULL) break;
       event = *(ptr->event);

       ////  All X events except MotionNotify in the image window are handled
       ////  here.
       switch(ptr->event->type)
       {  case KeyPress: 
            keyevent = event.xkey;
            keystate = event.xkey.state;
            key_x    = event.xkey.x;
            key_y    = event.xkey.y;
            
            g.key_alt = 0;
            if(keystate & ControlMask) g.key_ctrl=1;
            if(keystate & Mod1Mask) g.key_alt=1;
            if(keystate & Mod3Mask) g.key_alt=1;
            if(keystate & ShiftMask) g.key_shift=1;

            iii = whichimg(g.mouse_x, g.mouse_y, bpp);
            if(iii>=0 && z[iii].permanent) g.imageatcursor = iii;
            // Because each image has a separate window, add image position to
            // key coordinates to get screen coordinates.

            if(g.imageatcursor>=0)
            {   key_x += z[g.imageatcursor].xpos;
                key_y += z[g.imageatcursor].ypos;
            }

            XLookupString(&keyevent,buffer,bufsize,&key,&compose);
            g.key_ascii= key & 0xff; // A global, reset to 0 by getcharacter()
            if(g.key_ascii>=225) g.key_ascii = 0;  // Don't want shift, alt, ctrl
            s[0] = (uint)g.key_ascii;
            s[1] = ((uint)key)>>8;
            s[2] = 0;
            s[3] = 0;

            ////  If the key is a menu mnemonic, send the focus over to menu 
            ////  so alt-F pops up a menu.   

            if(g.key_alt && strchr("fipcdaoh", g.key_ascii))
                 XtSetKeyboardFocus(g.main_widget, g.menubar);
            else
                 print(s, g.mouse_x, g.mouse_y, g.fcolor, g.bcolor, &g.image_gc, 
                     g.main_window, 0, g.text_direction);
            if(g.state == GETLINE){ g.getlinestate=0; g.state=NORMAL; }
            break;
          case Expose: fprintf(stderr, "Expose event in wrong place\n"); break;
          case KeyRelease:   break;
          case MotionNotify: break;       //  MotionNotify is handled in 'mousecb'.
          case ButtonPress: 
              g.mouse_button = (int)event.xbutton.button;
              g.last_mouse_button = g.mouse_button;
              if(keys & 0xff & ShiftMask) handle_setpoint(); // Shift-click, set point
              handle_button_press(); 
              break;
          case ButtonRelease: 
              handle_button_release(); break;
          default: break;
       }  
       break;
     default:
       fprintf(stderr,"Unknown event at %d\n",__LINE__);
       break;
  }
}


//--------------------------------------------------------------------//
// handle_setpoint                                                    //
//--------------------------------------------------------------------//
void handle_setpoint(void)
{
   static int oino,ino,x1=0,y1=0;
   int bpp,x2,y2;
   setpoint++;
   ino = whichimage(g.mouse_x, g.mouse_y, bpp);              
   switch(setpoint)
   {   case 1:  if(ino >= 0)
                {    x1 = g.mouse_x - z[ino].xpos;
                     y1 = g.mouse_y - z[ino].ypos;
                     oino = ci;
                     message("Point set");
                }
                break;
       case 2:  if(ino >=0 && ino == oino)
                {   x2 = g.mouse_x - z[ino].xpos;
                    y2 = g.mouse_y - z[ino].ypos;
                    g.selected_ulx = x1 + z[ino].xpos;
                    g.selected_uly = y1 + z[ino].ypos;
                    g.selected_lrx = g.mouse_x;
                    g.selected_lry = g.mouse_y;
                    g.ulx = g.selected_ulx;
                    g.lrx = g.selected_lrx;
                    g.uly = g.selected_uly;
                    g.lry = g.selected_lry;
                    g.selectedimage = -1;
                    drawselectbox(ON);
                }
                setpoint = 0;
                break;
       default: setpoint=0;
   }

}


//--------------------------------------------------------------------//
// handle_button_press                                                //
//--------------------------------------------------------------------//
void handle_button_press(void)
{
   if(g.state==MESSAGE) return;  // Stop it from selecting if mouse clicked in message
   g.get_x1 = g.mouse_x;
   g.get_y1 = g.mouse_y;
   g.get_x2 = g.mouse_x;
   g.get_y2 = g.mouse_y;

   ////  If positioning a label, prevent clicking on other images.
   if(g.getlabelstate) return;

   ////  If clicking on an image, bring it to the foreground. 
   ////  If clicking on bkgd, set bkgd palette & turn off selected area.                                          //
   ////  If image is already in foreground, switch to the image's  
   ////  palette and select it.                         
   ////  If clicking on selected area, copy it.
   ////  If double-clicking inside preselected rectangular area, identify
   ////  everything in the area as an object.

   if(g.imageatcursor == -1) setSVGApalette(-4); 

   ////  Mouse wheel

   if(g.mouse_button == 4 && ci >0)
   {   z[ci].ypos += g.csize;
       moveimage(ci, z[ci].xpos, z[ci].ypos);
       selectimage(ci);    
       return;
   }
   if(g.mouse_button == 5 && ci >0)
   {   z[ci].ypos -= g.csize;
       moveimage(ci, z[ci].xpos, z[ci].ypos);
       selectimage(ci);    
       return;
   }
   if(g.mouse_button == 2) getxselection();

   if(g.want_switching)
   {  
       if(mouse_near_selection) 
       {   switch(g.cursor_symbol)
           {   case XC_fleur      : g.state=COPY; break;
               case XC_left_side  : g.state=RESIZE_LEFT; break;
               case XC_right_side : g.state=RESIZE_RIGHT; break;
               case XC_top_side   : g.state=RESIZE_TOP; break;
               case XC_bottom_side: g.state=RESIZE_BOTTOM; break;
           }
       }else if(g.imageatcursor >= 0 && g.imageatcursor != ci && !g.in_crop) // Switch images
       {    switchto(g.imageatcursor);
            if(ci>=0) setpalette(z[ci].palette);
       }else if(g.selectedimage>-1 || !insidebox(g.mouse_x, g.mouse_y,
            g.selected_ulx, g.selected_uly, g.selected_lrx, g.selected_lry) &&
            !g.in_crop)
       {    selectimage(g.imageatcursor);   
            if(g.imageatcursor>=0) setpalette(z[g.imageatcursor].palette);
       } 
   }
   if(z[ci].is_font) get_special_character();
   if(g.draw_figure) draw(START);
   printstatus(g.state);

#ifdef LINUX
   if(g.state == GETBOX || g.state == GETLINE) usleep(500);
#endif
   XtSetKeyboardFocus(g.main_widget, g.drawing_area);
}
 

//--------------------------------------------------------------------//
// handle_button_release                                              //
//--------------------------------------------------------------------//
void handle_button_release(void)
{
   static double oldtime=0.0;
   int k,msec;
   double newtime;

   if(g.mouse_button > 3) return;
   g.mouse_button = 0;
   g.getboxstate = 0;
   g.getpointstate = 0;
   g.getlinestate = 0;
   g.get_x2 = g.mouse_x;
   g.get_y2 = g.mouse_y;
   if(g.getlabelstate) return;
   if(g.state==SELECT)   //// Selecting rectangular area
   {   g.state = NORMAL;
       g.selected_lrx = g.mouse_x;
       g.selected_lry = g.mouse_y;
       if(g.selected_ulx>g.selected_lrx) swap(g.selected_ulx,g.selected_lrx);
       if(g.selected_uly>g.selected_lry) swap(g.selected_uly,g.selected_lry);
       g.ulx = zoom_x_coordinate(g.selected_ulx, g.imageatcursor);
       g.uly = zoom_y_coordinate(g.selected_uly, g.imageatcursor);
       g.lrx = zoom_x_coordinate(g.selected_lrx, g.imageatcursor);
       g.lry = zoom_y_coordinate(g.selected_lry, g.imageatcursor);       
   }else if(!g.ignore  && !g.in_crop)
   {   
      ////  Double-click, find object
       newtime = checktime(RESET);
       msec = int((newtime-oldtime)*1000); 
       if(msec<g.double_click_msec && oldtime!=0.0)
       {   if(!g.draw_figure && g.state==NORMAL)
              identify_object(g.mouse_x, g.mouse_y);
           g.state=NORMAL;
       }
       oldtime = newtime;
   }

   ////  If selected area is too small, throw it back
   if(abs(g.selected_ulx-g.selected_lrx)<2 || 
      abs(g.selected_uly-g.selected_lry)<2)
            selectimage(g.imageatcursor);
   if(g.state==GETBOX && (g.get_x1!=g.get_x2 || g.get_y1!=g.get_y2)) g.state = NORMAL;
   if(g.state==GETPOINT) g.state = NORMAL; 
   if(g.state==MOVEIMAGE) g.state = NORMAL;
   if(g.state==COPY) g.state = NORMAL;
   if(g.state==RESIZE_LEFT) g.state = NORMAL;
   if(g.state==RESIZE_RIGHT) g.state = NORMAL;
   if(g.state==RESIZE_TOP) g.state = NORMAL;
   if(g.state==RESIZE_BOTTOM) g.state = NORMAL;
   if(g.state==GETLINE && (g.get_x1!=g.get_x2 || g.get_y1!=g.get_y2)) g.state=NORMAL; 
   if(g.draw_figure) draw(END);
   printstatus(g.state);
   //// Select corresponding area in spreadsheet
   for(k=0;k<g.image_count;k++) if(z[k].s->visible) refreshspreadsheet(k);     
   //// If releasing mouse button after image drag, update lists. 
   for(k=0;k<g.openlistcount;k++) update_list(k);
}



//--------------------------------------------------------------------------//
// resize_scrolled_imagecb                                                  //
//--------------------------------------------------------------------------//
void resize_scrolled_imagecb(Widget w, XtP client_data, XEvent *event)
{
   w=w; 
   Widget scrolledwindow;
   Window rwin, cwin;
   int ino, rx,ry,wx,wy,bpp,rrx,rry; 
   uint uw, uh, ubw, ud, keys;
   event=event;  client_data=client_data;  w=w; // Keep compiler quiet
   ino = whichimage(w, bpp);
   if(ino<=0) return;
   drawselectbox(OFF);                          // prevent stray pixels if scrollbar
   w = XtParent(XtParent(XtParent(z[ino].widget)));
   XGetGeometry(g.display, XtWindow(w), &rwin, &rrx, &rry, &uw, &uh, &ubw, &ud);  
   scrolledwindow = XtParent(XtParent(z[ino].widget));
   if(uw != (uint)z[ino].xsize || uh != (uint)z[ino].ysize)
   {   if(!g.busy)                   // If filling camera bufr, don't resize.
           resize_widget(scrolledwindow,uw,uh); 
   }
   if(z[ino].shell)
   {   XQueryPointer(g.display, z[ino].win, &rwin, &cwin, &rx, &ry, &wx, &wy, &keys);
       z[ino].xpos = rx - wx - g.main_xpos;
       z[ino].ypos = ry - wy - g.main_ypos; 
   }
}


//--------------------------------------------------------------------//
// configurecb - Handle ConfigureNotify events to images              //
//--------------------------------------------------------------------//
void configurecb(Widget w, XtP client_data, XEvent *event)
{
  event=event;
  client_data=client_data;  w=w; // Keep compiler quiet
  int ino, oci=ci, k,rx,ry,wx,wy; 
  uint keys, uw, uh, ubw, ud;
  Window rwin, cwin;
  Widget widget;
  if(g.draw_figure) draw(END);
  drawselectbox(OFF);
                                // Find current position of main window & image number
  if(w==g.main_widget)
  {    XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
       g.main_xpos = rx-wx;
       g.main_ypos = ry-wy;
  }                             // Find position of all of them 
                                // If a shell widget, it can be resized so get its size too.
  for(ino=0; ino<g.image_count; ino++)
  {   if(z[ino].shell || ino==0)
      {    XQueryPointer(g.display, z[ino].win, &rwin, &cwin, &rx, &ry, &wx, &wy, &keys);
           z[ino].xpos = rx-wx-g.main_xpos;
           z[ino].ypos = ry-wy-g.main_ypos; 
           widget = XtParent(z[ino].widget);           
           //// If user resized main window, resize image 0
           if(ino==0)
           {  
              //// Get new size of g.drawing_area
              XGetGeometry(g.display, XtWindow(XtParent(XtParent(widget))), &rwin, &rx, &ry, &uw, &uh, &ubw, &ud);
              // These two lines are not necessary because X returns wrong information.
              uw -= g.drawarea2width; 
              uh -= + g.mainmenuheight;
               if(uw != (uint)z[ino].xsize || uh != (uint)z[ino].ysize)
               {   
                  if(!g.busy)                   // If filling camera bufr, don't resize.
                  {   
                      resize_image(ino,uw,uh);  // This changes ci
                      z[ino].xsize = uw;
                      z[ino].ysize = uh;
                      if(ino==0) 
                      {    g.main_xsize = uw; 
                           g.main_ysize = uh; 
                           g.total_xsize = uw + g.drawarea2width; 
                           g.total_ysize = uh + (int)g.mainmenuheight; 
                     }
                     title_ypos = g.main_ysize - 4;
                  }
               }
           }else // user resized window of image in shell
           {   
               XGetGeometry(g.display, XtWindow(widget), &rwin, &rx, &ry, &uw, &uh, &ubw, &ud);            
               if(uw != (uint)z[ino].xsize ||  uh != (uint)z[ino].ysize)
               {  if(!g.busy)                   // If filling camera bufr, don't resize.
                  {   resize_image(ino,uw,uh);  // This changes ci
                      z[ino].xsize = uw;
                      z[ino].ysize = uh;
                  }
               }
           }
      }

  }

  ////  Deselect any non-rectangular region otherwise selection is in wrong place
  g.selected_is_square = 1;

  ////  Recalculate ulx, uly, etc
  switchto(oci);  //// Necessary if image is loaded on command line
  selectimage(ci);
  if(!g.moving && g.state!=MOVEIMAGE) for(k=0;k<g.openlistcount;k++) update_list(k);
  if(w==g.main_widget || w==g.title_area)
      XtMoveWidget(g.title_area, 120, g.main_ysize-15);

}


//--------------------------------------------------------------------//
// imagefocuscb - Handle FocusIn events to images. If image is in a   //
// separate frame, clicking on the frame generates a FocusIn.         //
// Only do this for images in separate windows. Focus for images on   //
// main window is handled in drawcb() and handle_button_press().      //
// Don't switch to the image if a non-rectangular region was selected.//
//--------------------------------------------------------------------//
void imagefocuscb(Widget w, XtP client_data, XEvent *event)
{
  client_data=client_data;  w=w; // Keep compiler quiet
  int k; 
  if(g.getlabelstate) return;
  if(event->type==FocusIn && g.selected_is_square)
    for(k=0; k<g.image_count; k++) 
       if(z[k].shell && k!=ci && w==XtParent(XtParent(z[k].widget))) switchto(k);
  printstatus(g.state);
}


//--------------------------------------------------------------------//
// mousecb - Handle MotionNotify events in image window               //
//--------------------------------------------------------------------//
void mousecb(Widget w, XtP client_data, XEvent *event)
{
  w=w;client_data=client_data;  // Keep compiler quiet

  ////  If dragging the mouse, go into select mode. 'mouse_button' is set in 
  ////  drawcb(). Can't read mouse button here because this is not called in 
  ////  response to ButtonPress. Make sure mouse has moved before selecting 
  ////  an area.

  int bpp, iii, oiac, x1=0, x2=0, y1=0, y2=0, x, y, ino, dx, dy;
  static int omouse_x=0, omouse_y=0;
  static Cursor ocursor = 0;
  Window window;
  int ostate=g.state;
  int modifykey = 0;       // shift, alt, ctrl key pressed
  Cursor cursor = g.cursor;
  g.mouse_x = event->xmotion.x_root - g.main_xpos;
  g.mouse_y = event->xmotion.y_root - g.main_ypos;
  oiac = g.imageatcursor;
  iii = whichimg(g.mouse_x, g.mouse_y, bpp);
  if(iii>=0 && z[iii].permanent) g.imageatcursor = iii;
  if(g.imageatcursor>=0) window = z[g.imageatcursor].win;
  else window = g.main_window;

  //// If positioning a label, don't let them click on another image
  if(g.getlabelstate) return;
  //// alt, shift, caps lock, etc
  modifykey = (event->xmotion.state & 0xff & (ControlMask | ShiftMask | LockMask | Mod1Mask));  

  if(between(g.mouse_button, 1, 3))
  {   

       crosshairs(0, 0); 
       if(g.mouse_x != omouse_x || g.mouse_y != omouse_y)                        
       {    g.get_x2 = g.mouse_x;
            g.get_y2 = g.mouse_y;
            if(g.draw_figure)                     
                draw(TNI_CONTINUE);
            else if(g.state==MOVEIMAGE && ci>=0 && !z[ci].shell) 
            {   z[ci].xpos = g.mouse_x;
                z[ci].ypos = g.mouse_y;
                moveimage(ci, g.mouse_x, g.mouse_y);
            }
            else if(g.state==COPY)
            {   move();   
                mouse_near_selection=0;
                g.state=NORMAL;
            }
            else if(between(g.state, RESIZE_LEFT, RESIZE_BOTTOM))
            {   resize_selection();   
                drawselectbox(ON);           // Draw dashed outline box
            }
            else if(g.state==NORMAL && !g.in_crop) 
            {   g.selected_ulx = g.mouse_x;
                g.selected_uly = g.mouse_y;
                g.ulx = zoom_x_coordinate(g.selected_ulx, g.imageatcursor);
                g.uly = zoom_y_coordinate(g.selected_uly, g.imageatcursor);
                g.state = SELECT;
                g.selectedimage= -1;
                g.selected_is_square=1;
            }
            if(g.state==SELECT && !g.in_crop) 
            {   g.selected_lrx = g.mouse_x;
                g.selected_lry = g.mouse_y;
                g.lrx = zoom_x_coordinate(g.selected_lrx, g.imageatcursor);
                g.lry = zoom_y_coordinate(g.selected_lry, g.imageatcursor);
                drawselectbox(ON);           // Draw dashed outline box
            }
       }      
  }else if(g.getboxstate==0)
  {    
       ino = g.imageatcursor;  
       if(between(ino,1,g.image_count-1) && !z[ino].shell)
       {    x1 = z[ino].xpos;
            y1 = z[ino].ypos;
            x2 = z[ino].xpos + cint(z[ino].xscreensize);
            y2 = z[ino].ypos + cint(z[ino].yscreensize);
            if(onedge(g.mouse_x,g.mouse_y,x1,y1,x2,y2))
            {   if(g.state==NORMAL && !modifykey) g.state = MOVEIMAGE; 
            }            
            else if(g.state==MOVEIMAGE) g.state=NORMAL; 
       }else if(g.state==MOVEIMAGE) g.state=NORMAL; 
       
       mouse_near_selection=0;
       if(g.selectedimage==-1 && g.state!=GETBOX && g.state!=GETPOINT && g.state!=GETBOXPOSITION)
       {   
            if(g.selected_is_square)
            {   if(onedge(g.mouse_x, g.mouse_y, g.selected_ulx, 
                           g.selected_uly, g.selected_lrx, g.selected_lry))
                {    mouse_near_selection=1;
                     g.state=NORMAL;
                }
            }else
            {   x1 = max(g.selected_ulx, g.mouse_x-2);
                y1 = max(g.selected_uly, g.mouse_y-2);
                x2 = min(g.selected_lrx, g.mouse_x+2);
                y2 = min(g.selected_lry, g.mouse_y+2);
                for(y=y1;y<=y2;y++)
                for(x=x1;x<=x2;x++)
                {   if(inside_irregular_region(x,y)==3)
                    {    mouse_near_selection=1;
                         g.state=NORMAL;
                         break; 
                    }
                }
            }
       }
  }

  if((mouse_near_selection || (g.state==MOVEIMAGE && g.imageatcursor>0)) &&
     g.want_switching && 
     !modifykey)    // If alt or shift button is not down, select region
  {
     x1 = g.selected_ulx;
     x2 = g.selected_lrx;
     y1 = g.selected_uly;
     y2 = g.selected_lry;
     dx = x2-x1;
     dy = y2-y1;
     if(between(g.mouse_x, x1-2, x1+2)      && between(g.mouse_y, y1+min(20,dy/4), y2-min(20,dy/4))) 
     {   cursor = g.left_cursor; g.cursor_symbol = XC_left_side; }
     else if(between(g.mouse_x, x2-2, x2+2) && between(g.mouse_y, y1+min(20,dy/4), y2-min(20,dy/4))) 
     {   cursor = g.right_cursor;  g.cursor_symbol = XC_right_side; }
     else if(between(g.mouse_y, y1-2, y1+2) && between(g.mouse_x, x1+min(20,dx/4), x2-min(20,dx/4))) 
     {   cursor = g.up_cursor; g.cursor_symbol = XC_top_side; }
     else if(between(g.mouse_y, y2-2, y2+2) && between(g.mouse_x, x1+min(20,dx/4), x2-min(20,dx/4))) 
     {   cursor = g.down_cursor;  g.cursor_symbol = XC_bottom_side; }
     else 
     {   cursor = g.fleur_cursor; g.cursor_symbol = XC_fleur; }
  }else 
  {    cursor = g.cursor;
       g.cursor_symbol = XC_cross;
  }
  if(g.state==GETBOXPOSITION) draw(TNI_CONTINUE);

  ////   oiac may be different from g.imageatcursor if mouse moves fast
  if(cursor!=ocursor || oiac!=g.imageatcursor)
      XDefineCursor(g.display, window, cursor);
  
  crosshairs(g.mouse_x, g.mouse_y);
  if(g.state != ostate) printstatus(g.state);
  if(g.floating_magnifier && 
      (omouse_x != g.mouse_x || omouse_y != g.mouse_y)) 
      update_floating_magnifier(g.mouse_x, g.mouse_y);
  omouse_x = g.mouse_x;
  omouse_y = g.mouse_y;
  ocursor = cursor;
  printcoordinates(g.mouse_x, g.mouse_y, 0);    
  return;
}


//--------------------------------------------------------------------//
// crosshairs - call with x,y = 0,0 to erase                          //
//--------------------------------------------------------------------//
void crosshairs(int x, int y)
{
  switch(g.want_crosshairs) 
  {   case 1: single_crosshairs(x,y); break;
      case 2: multiple_crosshairs(x,y); break;
      default: break;
  }
  return;
}  


//--------------------------------------------------------------------//
// drawpixel - must appear before first call to drawpixel()           //
//--------------------------------------------------------------------//
static inline void drawpixel(int x, int y, int color, Window win)
{
    XSetForeground(g.display, g.gc, color);   
    XDrawPoint(g.display, win, g.gc, x, y);  
}


//--------------------------------------------------------------------//
// getpixel must appear before first call to getpixel()               //
//--------------------------------------------------------------------//
static inline int getpixel(int x, int y, int ino)
{
  if(!between(ino, 0, g.image_count-1)) return 0;
  if(between(x, 0, z[ino].xscreensize-1))  // Can't be one line, compiler 
  if(between(y, 0, z[ino].yscreensize-1))  // optimizes it away
      return pixelat(z[ino].img[y] + x*g.off[g.bitsperpixel], g.bitsperpixel);
  return 0;
}


//--------------------------------------------------------------------//
// single_crosshairs - call with x,y = 0,0 to erase                   //
//--------------------------------------------------------------------//
void single_crosshairs(int x, int y)
{
  static int hit=0, *xsave, *ysave, ox, oy, odx, ody, oxsize, oysize,
      oxpos, oypos;
  static Window owin;
  Window win;
  int i,j,ino,bpp,dy,dx,xsize,ysize,xpos,ypos;
  if(!hit)
  {   xsave = new int[g.xres];
      ysave = new int[g.yres];
      if(ysave==NULL){ printf(g.nomemory); exit(1); }
  }

  ino = whichimg(x,y,bpp);
  if(!between(ino,0,g.image_count-1)) return;
  x -= z[ino].xpos;
  y -= z[ino].ypos;
  xsize = z[ino].xscreensize;
  ysize = z[ino].yscreensize;
  xpos  = z[ino].xpos;
  ypos  = z[ino].ypos;
  win   = z[ino].win;
  if(ino==0 || !z[ino].shell)
  {   dx = g.main_xpos;
      dy = g.main_ypos;
  }else
  {   dx = xpos + g.main_xpos;
      dy = ypos + g.main_ypos;
  }
  if(hit)
  {   for(j=0;j<oysize;j++) if(between(j+oypos,0,g.yres-1)) drawpixel(ox,j,ysave[j+oypos],owin);
      for(i=0;i<oxsize;i++) if(between(i+oxpos,0,g.xres-1)) drawpixel(i,oy,xsave[i+oxpos],owin);
  }
  for(j=0;j<ysize;j++) if(between(j+ypos,0,g.yres-1)) ysave[j+ypos] = getpixel(x,j,ino);
  for(i=0;i<xsize;i++) if(between(i+xpos,0,g.xres-1)) xsave[i+xpos] = getpixel(i,y,ino);
  if(x || y)
  {   for(j=0;j<ysize;j++) if(between(j+ypos,0,g.yres-1)) drawpixel(x,j,g.fcolor,win);
      for(i=0;i<xsize;i++) if(between(i+xpos,0,g.xres-1)) drawpixel(i,y,g.fcolor,win);
  }
  ox = x; oy = y; owin = win; oxsize = xsize; oysize = ysize;
  ody = dy; odx = dx; oxpos = xpos; oypos = ypos;
  hit=1;
}


//--------------------------------------------------------------------//
// multiple_crosshairs - call with x,y = 0,0 to erase                 //
//--------------------------------------------------------------------//
void multiple_crosshairs(int x, int y)
{
  static int hit[MAXIMAGES], **xsave, **ysave, ox[MAXIMAGES], oy[MAXIMAGES], 
         odx[MAXIMAGES], ody[MAXIMAGES], oxsize[MAXIMAGES], oysize[MAXIMAGES];
  static int mainhit=0;
  static Window owin[MAXIMAGES];
  Window win;
  int i,j,bpp,current_ino,ino,dy,dx,xsize,ysize,xpos,ypos,x1,y1;
  int color;
  current_ino = whichimg(x,y,bpp);
  if(!between(current_ino, 0, g.image_count-1)) return;
  g.inmenu++;

  if(!mainhit)
  {   xsave = new int*[MAXIMAGES];
      ysave = new int*[MAXIMAGES];
      mainhit = 1;
      for(ino=0; ino<MAXIMAGES; ino++) hit[ino]=0;
  }

  //// Free backup bufr if they deleted or resized an image
  for(ino=0; ino<MAXIMAGES; ino++) 
  {   
      if(hit[ino] && 
        (ino >= g.image_count || 
        (ino < g.image_count && 
             (oxsize[ino] != z[ino].xscreensize || 
              oysize[ino] != z[ino].yscreensize)
        ))
        )
      {   
          if(xsave[ino] != NULL) delete[] xsave[ino];
          if(ysave[ino] != NULL) delete[] ysave[ino];
          hit[ino] = 0;
      }
  }
  XFlush(g.display);

  for(ino=0; ino<g.image_count; ino++)
  {  
      if(!hit[ino])
      {   xsave[ino] = new int[z[ino].xscreensize];
          ysave[ino] = new int[z[ino].yscreensize];
          if(ysave[ino] == NULL){ printf(g.nomemory); exit(1); }
      }
      if(ino == current_ino) color = g.fcolor; else color = g.fcolor/2;
      x1 = x - z[current_ino].xpos;
      y1 = y - z[current_ino].ypos;
      xsize = z[ino].xscreensize;
      ysize = z[ino].yscreensize;
      xpos  = z[ino].xpos;
      ypos  = z[ino].ypos;
      win   = z[ino].win;
      if(!z[ino].shell)
      {   dx = g.main_xpos;
          dy = g.main_ypos;
      }else
      {   dx = xpos + g.main_xpos;
          dy = ypos + g.main_ypos;
      }

      if(hit[ino])
      {   for(j=0; j<oysize[ino]; j++) 
             if(between(j+ypos, 0, g.yres-1)) 
                drawpixel(ox[ino], j, ysave[ino][j], owin[ino]);
          for(i=0; i<oxsize[ino]; i++) 
             if(between(i+xpos, 0, g.xres-1))
                drawpixel(i, oy[ino], xsave[ino][i], owin[ino]);
      }

      if(between(x1,0,xsize-1))
      for(j=0; j<ysize; j++) 
          if(between(j+ypos, 0, g.yres-1)) ysave[ino][j] = getpixel(x1,j,ino);

      if(between(y1,0,ysize-1))
      for(i=0; i<xsize; i++)
          if(between(i+xpos, 0, g.xres-1)) xsave[ino][i] = getpixel(i,y1,ino);

      if(x1 || y1)
      {   for(j=0; j<ysize; j++) if(between(j+ypos, 0, g.yres-1)) drawpixel(x1,j,color,win);
          for(i=0; i<xsize; i++) if(between(i+xpos, 0, g.xres-1)) drawpixel(i,y1,color,win);
      }
      ox[ino] = x1; oy[ino] = y1; owin[ino] = win; 
      oxsize[ino] = xsize; oysize[ino] = ysize;
      ody[ino] = dy; odx[ino] = dx;
      hit[ino] = 1;
  }
  g.inmenu--;
}


//--------------------------------------------------------------------//
// image_exposecb                                                     //
// Callback for image windows being exposed.                          //
//--------------------------------------------------------------------//
void image_exposecb(Widget widget, XtP client_data, XtP *call_data)
{
  client_data=client_data;  // Keep compiler quiet
  XmDrawingAreaCallbackStruct *ptr;
  int  bpp, count, ex, ey, ew, eh, ino;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  if(ptr==NULL) return;
  ino = whichimage(widget,bpp);
  if(ino<0) return;
  switch(ptr->reason)
  {  
     case XmCR_EXPOSE:
       printstatus(REDRAW);
       ex = ptr->event->xexpose.x;
       ey = ptr->event->xexpose.y;
       ew = ptr->event->xexpose.width;
       eh = ptr->event->xexpose.height;
       count = ptr->event->xexpose.count;
       XPutImage(g.display, z[ino].win, g.image_gc, z[ino].img_ximage,
            ex, ey, ex, ey, ew, eh);
       if(count==0)
       {   if(z[ino].exposefunc != NULL) z[ino].exposefunc(z[ino].exposeptr);
           if(g.want_title) draw_image_title(ino);
       }
       printstatus(g.state);
       XDefineCursor(g.display, z[ino].win, g.cursor);
       break;
     default:  
       fprintf(stderr,"Error in image_exposecb line %d\n",__LINE__);
       break;
  }
  printcoordinates(g.mouse_x, g.mouse_y, 1);    
}



//--------------------------------------------------------------------//
// warningmessagecb - handle warning messages from Xt (mainly "unable //
// to allocate colormap entry for default background").               //
//--------------------------------------------------------------------//
void warningmessagecb(String aaa)
{ 
   if(g.diagnose) printf("Xt warning: %s\n",aaa);
}


//--------------------------------------------------------------------//
//  moveimage                                                         //
//  want_redraw is optional (set to 0 to move without redrawing)      //
//  default=1                                                         //
//--------------------------------------------------------------------//
void moveimage(int ino, int xpos, int ypos, int want_redraw)
{
  int k, gotevent=0;
  XEvent event;
  gotevent = XCheckMaskEvent(g.display, g.main_mask | g.mouse_mask, &event);
  if(g.moving && gotevent) return;  // Don't return unless image is being moved
                                    // with arrow keys otherwise it will mess up
                                    // copy/paste
  if(ino>0)
  {   
     crosshairs(0, 0);
      if(z[ino].shell)
           XtMoveWidget(XtParent(XtParent(z[ino].widget)),xpos, ypos);
      else 
           XtMoveWidget(z[ino].widget, xpos, ypos);
      if(z[ino].chromakey)     
           redraw_chromakey_image(ino, want_redraw);
      if(z[ino].transparency)     
           redraw_transparent_image(ino, want_redraw);
      if(z[ino].shell)
      {     z[ino].xpos = xpos-g.main_xpos;
            z[ino].ypos = ypos-g.main_ypos; 
      }else
      {     z[ino].xpos = xpos;
            z[ino].ypos = ypos; 
      }

      ////  Dont update list after a move if image is in separate window,
      ////  because it gets a ConfigureNotify which updates list in configurecb().
      ////  If dragging image, wait until mouse button is released.
      
      if(z[ino].permanent && g.state != MOVEIMAGE)
      {   if(!z[ino].shell) for(k=0;k<g.openlistcount;k++) update_list(k);
          switchto(ino);
      }
      if(z[ino].permanent) switchto(ino);
  } 
}


//--------------------------------------------------------------------//
//  redraw_chromakey_image                                            //
//  want_redraw is optional, default=1. If 0 it speeds it up by only  //
//  redrawing the image every SPEED events (including timer or mouse  //
//  events). This is done by sending a flag to redraw().  If it is    //
//  still too slow, SPEED can be increased with no harmful effects.   //
//--------------------------------------------------------------------//
void redraw_chromakey_image(int ino, int want_redraw)
{
  const int SPEED=3;
  g.state = BUSY;
  printstatus(BUSY);
  int bpp,cf,i,j,v=0,x,y,rr,gg,bb,rmin,rmax,gmin,gmax,bmin,bmax,tmin,tmax,
      x2,y2,v1,v2;  
  if(ino<0 || !z[ino].chromakey){ g.state=NORMAL; return; }
  static int count=0;
  count++;
  if(!want_redraw && count!=SPEED) g.ignore=1; 
  if(count==SPEED)count=0;

  if(z[ino].is_zoomed == ZOOM)
  {   x2 = z[ino].zoom_x2;
      y2 = z[ino].zoom_y2;
  }else
  {   x2 = z[ino].xsize;
      y2 = z[ino].ysize;
  }
  ////  First create a good copy of img buffer  
  rebuild_display(ino);  
  

  ////  Then select fg or bg pixel and put back in img buffer
  for(j=0; j<y2; j++)
  for(i=0; i<x2; i++)
  {    cf = z[ino].cf;
       bpp = z[ino].bpp;
       x = i + z[ino].xpos;
       y = j + z[ino].ypos;
       v1 = readpixel(x, y, -2);  // foreground screen pixel
       v2 = readpixel(x, y, ino); // background screen pixel
       if(z[ino].colortype==GRAY)
       {     
             v = graypixel(v1, g.bitsperpixel);
             v = convertpixel(v, g.bitsperpixel, bpp, 0);
             tmin = z[ino].ck_graymin;
             tmax = z[ino].ck_graymax;
             if(is_opaque(v,tmin,tmax,z[ino].chromakey)) v=v1;
             else v = v2;
       }else
       {     rmax = z[ino].ck_max.red;
             rmin = z[ino].ck_min.red;
             gmin = z[ino].ck_min.green;
             gmax = z[ino].ck_max.green;
             bmin = z[ino].ck_min.blue;
             bmax = z[ino].ck_max.blue;
             valuetoRGB(v1,rr,gg,bb,bpp);
             convertRGBpixel(rr,gg,bb,bpp,24);
             if(is_opaque(rr,rmin,rmax,gg,gmin,gmax,bb,bmin,bmax,z[ino].chromakey)) 
                  v = v1;
             else v = v2;
       }
       putpixelbytes(z[ino].img[j]+i*g.off[g.bitsperpixel], v);
  }

  //// Skip redrawing if called from move() for speed
  if(g.ignore==0) 
        send_expose_event(z[ino].win, Expose, 0, 0, x2, y2);
  g.state = NORMAL;
  printstatus(g.state);
  g.ignore=0; 
} 


//--------------------------------------------------------------------//
//  redraw_transparent_image                                          //
//  want_redraw is optional, default=1. If 0 it speeds it up by only  //
//  redrawing the image every SPEED events (including timer or mouse  //
//  events). This is done by sending a flag to redraw().  If it is    //
//  still too slow, SPEED can be increased with no harmful effects.   //
//--------------------------------------------------------------------//
void redraw_transparent_image(int ino, int want_redraw)
{
  static int count=0;
  const int SPEED=3;
  int bpp,cf,i,j,x,y,rr,gg,bb,rr1,gg1,bb1,rr2,gg2,bb2,x2, y2;
  printstatus(BUSY);
  double tr1, tr2;
  if(ino<0 || !z[ino].transparency) return;
  count++;
  if(!want_redraw && count!=SPEED) g.ignore=1; 
  if(count==SPEED)count=0;
  bpp = z[ino].bpp;
  tr2 = (double)(z[ino].transparency)/100.00;
  tr1 = 1.00 - tr2;
  cf = z[ino].cf;
  if(z[ino].is_zoomed == ZOOM)
  {   x2 = z[ino].zoom_x2;
      y2 = z[ino].zoom_y2;
  }else
  {   x2 = z[ino].xsize;
      y2 = z[ino].ysize;
  }

  ////  First create a good copy of img buffer  
  rebuild_display(ino);  
  
  ////  Then mix it with background and put back in img buffer
  for(j=0; j<y2; j++)
  for(i=0; i<x2; i++)
  {  
       x = i + z[ino].xpos;
       y = j + z[ino].ypos;
       readRGB(x, y, rr1, gg1, bb1, -2);  // foreground screen pixel
       readRGB(x, y, rr2, gg2, bb2, ino); // background screen pixel
       rr = (int)(rr1*tr1 + rr2*tr2);
       gg = (int)(gg1*tr1 + gg2*tr2);
       bb = (int)(bb1*tr1 + bb2*tr2);           
       putRGBbytes(z[ino].img[j]+i*g.off[g.bitsperpixel], rr, gg, bb, g.bitsperpixel);
  }
 
  //// Skip redrawing if called from move() for speed
  if(g.ignore==0)
  {     repairimg(ino, 0, 0, x2, y2);
        send_expose_event(z[ino].win, Expose, 0, 0, x2, y2);
  }
  printstatus(g.state);
  g.moving=0;
  XFlush(g.display);
  g.ignore=0; 
  return;
} 


//--------------------------------------------------------------------//
// upcb - callbacks for arrow button to move image.                   //
//--------------------------------------------------------------------//
void upcb(Widget w, XtP client_data, XtP call_data)
{  w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
   if(ci>0)
   {  drawselectbox(OFF);
      g.moving=1;
      if(!z[ci].shell)
      {   z[ci].ypos -= g.csize;
          moveimage(ci,z[ci].xpos,z[ci].ypos);
      }
      selectimage(ci);    
   }
}
void downcb(Widget w, XtP client_data, XtP call_data)
{  w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
   if(ci>0)
   {  drawselectbox(OFF);
      g.moving=2;
      if(!z[ci].shell)
      {   z[ci].ypos += g.csize;
          moveimage(ci, z[ci].xpos, z[ci].ypos);
      }
      selectimage(ci);    
   }
}
void leftcb(Widget w, XtP client_data, XtP call_data)
{  w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
   if(ci>0)
   {  drawselectbox(OFF);
      g.moving=3;
      if(!z[ci].shell)
      {   z[ci].xpos -= g.csize;
          moveimage(ci, z[ci].xpos, z[ci].ypos);
      }
      selectimage(ci);    
   }
}
void rightcb(Widget w, XtP client_data, XtP call_data)
{  w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
   if(ci>0)
   {  drawselectbox(OFF);
      g.moving=4;
      if(!z[ci].shell)
      {   z[ci].xpos += g.csize;
          moveimage(ci, z[ci].xpos, z[ci].ypos);
      }
      selectimage(ci);    
   }
}

//--------------------------------------------------------------------//
// image_jump                                                         //
//--------------------------------------------------------------------//
void image_jump(int noofargs, char **arg, int direction)
{    
    int ino = ci;
    int distance = g.csize;
    if(noofargs >=1) distance = atoi(arg[1]); 
    if(!between(ino, 1, g.image_count-1)) return;
    int xpos = z[ino].xpos;
    int ypos = z[ino].ypos;
    switch(direction)
    {   case UP: ypos -= distance; break;
        case DOWN: ypos += distance; break;
        case LEFT: xpos -= distance; break;
        case RIGHT: xpos += distance; break;
    }
    moveimage(ino, xpos, ypos, 1);
}


//--------------------------------------------------------------------//
// disarmcb - callback for releasing a pushbutton.                    //
//--------------------------------------------------------------------//
void disarmcb(Widget w, XtP client_data, XmACB *call_data)
{  
    g.moving=0;
    w=w; call_data=call_data; client_data=client_data;  // Keep compiler quiet
    g.state=NORMAL;
    printstatus(g.state);
}


//--------------------------------------------------------------------//
//  reset_user_buttons                                                //
//  Buttons at left whose state is read from tnimage.ini.             //
//--------------------------------------------------------------------//
void reset_user_buttons(void)
{
  int k;
  for(k=0;k<g.nbuttons;k++) 
  {  if(strcmp(g.button[k]->activate_cmd,"slew")==SAME)
     {   XmToggleButtonSetState(g.button[k]->widget, g.want_slew, False);
         g.button[k]->state = g.want_slew;
     }
     if(strcmp(g.button[k]->activate_cmd,"zoom")==SAME) g.zoom_button=k;
     if(strcmp(g.button[k]->activate_cmd,"move")==SAME)
     {   XmToggleButtonSetState(g.button[k]->widget, 1-g.copy, False);
         g.button[k]->state = 1-g.copy;
     }
     if(strcmp(g.button[k]->activate_cmd,"sketch")==SAME) 
         XmToggleButtonSetState(g.button[k]->widget, g.draw_figure==SKETCH, False);
     if(ci>=0)
         XtSetKeyboardFocus(g.button[k]->widget, z[ci].widget);  
  }
}


//--------------------------------------------------------------------//
//  unpush_null_buttons                                               //
//--------------------------------------------------------------------//
void unpush_null_buttons(void)
{
  int k;
  for(k=0;k<g.nbuttons;k++) 
     if(g.button[k]->state==ON &&
        strcmp(g.button[k]->deactivate_cmd,"null")==SAME)
        unpush_main_button(k);      
  g.getout=1;
}


//--------------------------------------------------------------------//
// unpush_main_button                                                 //
//--------------------------------------------------------------------//
void unpush_main_button(int k)
{
   XmToggleButtonSetState(g.button[k]->widget, False, False);
   g.button[k]->state = OFF;
}


//--------------------------------------------------------------------//
// push_main_button                                                   //
//--------------------------------------------------------------------//
void push_main_button(int k)
{
   XmToggleButtonSetState(g.button[k]->widget, True, False);
   g.button[k]->state = ON;
}


//--------------------------------------------------------------------//
// cancelcb - callback                                                //
//--------------------------------------------------------------------//
void cancelcb(Widget w, XtP client_data, XmACB *call_data)
{
   int k;
   w=w;client_data=client_data; call_data=call_data; // keep compiler quiet
   g.getout = 1;
   g.state = NORMAL;
   printstatus(g.state);
   cancelled=1;
   if(ci>=0) for(k=0;k<20;k++) z[ci].gbutton[k]=0;
}


//--------------------------------------------------------------------//
// maincancelcb -  callback for cancel button on main screen          //
//--------------------------------------------------------------------//
void maincancelcb(Widget w, XtP client_data, XmACB *call_data)
{
   int k;
   w=w; call_data=call_data; client_data=client_data; // keep compiler quiet
   sanity_check();
   g.draw_figure = NONE;
   g.text_direction = HORIZONTAL;
   g.text_angle = 0.0;
   g.getout = 1;
   g.getboxstate = 0;
   g.getpointstate = 0;
   g.getlinestate = 0;
   g.getlabelstate = 0;
   g.spray_mode = 0;
   if(g.selectcurve) return;
   g.state = NORMAL;
   printstatus(g.state);
   if(!g.selected_is_square)
   {   selectimage(g.imageatcursor);
       g.selected_is_square=1;
   }
   for(k=0;k<g.nbuttons;k++) 
      if(strcmp(g.button[k]->deactivate_cmd, "cancel")==SAME)
          unpush_main_button(k);
   z[ci].animation = 0;
   reset_user_buttons();
   g.block = 0;
   g.busy = 0;
   setpoint = 0;
   clear_busy_flags();
}


//--------------------------------------------------------------------//
// reinitialize                                                       //
//--------------------------------------------------------------------//
void reinitialize(void)
{
   maincancelcb(0,0,0);
   readsettings(1);
   g.block = g.busy = g.waiting = 0;
   in_printimage = in_save = in_read = in_calibrate = in_warp = 0;
   in_pattern_recognition = in_camera = in_acquire = 0; 
   in_spot_dens_dialog = 0;
   in_strip_dens_dialog = 0;
}


//--------------------------------------------------------------------//
// mainbuttoncb                                                       //
//--------------------------------------------------------------------//
void mainbuttoncb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;  // keep compiler quiet
   int k,noofargs=0,button;
   char **arg;                // up to 20 arguments
   arg = new char*[20];
   for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }
   button = *(int *)client_data;
   if(g.button[button]->state==ON) g.button[button]->state=OFF; 
   else g.button[button]->state=ON;
   if(g.button[button]->state==ON)
      parsecommand(g.button[button]->activate_cmd, FORMULA, arg, noofargs, 128, 20);
   else if(strlen(g.button[button]->deactivate_cmd))
      parsecommand(g.button[button]->deactivate_cmd, FORMULA, arg, noofargs, 128, 20);
   if(strlen(arg[0])) execute(arg[0],noofargs,arg);
   for(k=0;k<20;k++) delete[] arg[k];
   
   ////  If no 'off' command, i.e. off command is "null", unpush button
   if(cancelled || strcmp(g.button[button]->deactivate_cmd,"null")==SAME) 
      unpush_main_button(button);
   delete arg;  
   cancelled=0;
}


//--------------------------------------------------------------------------//
// buttoncb                                                                 //
//--------------------------------------------------------------------------//
void buttoncb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // keep compiler quiet
  clickboxinfo *c = (clickboxinfo*)client_data;
  if(c->f1 !=NULL) c->f1(c->k);
  if(c->f2 !=NULL) c->f2(c->fanswer);
  if(c->f3 !=NULL) c->f3(c->answers);
  if(c->f4 !=NULL) c->f4(c->k, c);
  c->answer = 100; // for buttons in multiclickbox BOTTONVALSLIDER
}


//--------------------------------------------------------------------//
// helpcb - callback for getting help                                 //
// The address of int helptopic is in client_data.                    //
//--------------------------------------------------------------------//
void helpcb(Widget w, XtP client_data, XmACB *call_data)
{
    w=w; call_data=call_data;  // keep compiler quiet
    // Don't look at this line, it is an ugly hack to get around
    // the problem of passing an ordinary integer through a pointer.
    // In Intel 32 this isn't needed, but in some architectures, int
    // and void* are different sizes and it won't compile.
    int h = *(int *)client_data; 
    help(h);
} 


//--------------------------------------------------------------------//
// entercb - callback for pressing a key on a pushbutton              //
//--------------------------------------------------------------------//
void entercb(Widget w, XtP client_data, XEvent *event)
{
   w=w; client_data=client_data; 
   int key = which_key_pressed(event);
   if(key==XK_Escape){ g.getout = 1; g.state = NORMAL; }
}


//--------------------------------------------------------------------//
// okcb - callback                                                    //
//--------------------------------------------------------------------//
void okcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data; client_data=client_data; // keep compiler quiet
}


//--------------------------------------------------------------------//
// executecb - callback for main menu items.                          //
//--------------------------------------------------------------------//
void executecb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;  // Keep compiler quiet
   int k,noofargs=0;
   char **arg;                // up to 20 arguments
   arg = new char*[20];
   for(k=0;k<20;k++) arg[k] = new char[128];
   parsecommand((char*)client_data, FORMULA, arg, noofargs, 128, 20);
   execute(arg[0],noofargs,arg);
   for(k=0;k<20;k++) delete[] arg[k];
   delete arg;  
}


//--------------------------------------------------------------------//
// tearoff_activatecb                                                 //
//--------------------------------------------------------------------//
void tearoff_activatecb(Widget w, XtP client_data, XtP *call_data)
{
  int k;
  client_data=client_data;call_data=call_data;
  for(k=0; k<10; k++) if(w==g.menupane[k]) g.torn_off[k] = 1;
}


//--------------------------------------------------------------------//
// tearoff_deactivatecb                                               //
//--------------------------------------------------------------------//
void tearoff_deactivatecb(Widget w, XtP client_data, XtP *call_data)
{
  int k;
  client_data=client_data;call_data=call_data;
  for(k=0; k<10; k++) if(w==g.menupane[k]) g.torn_off[k] = 0;
}


//--------------------------------------------------------------------//
//  sig_quit                                                          //
//--------------------------------------------------------------------//
static void sig_quit(int signo)
{
  fprintf(stderr, "\nCaught signal %d\nSend bug report to %s\n",signo, g.emailaddress);
  crashing++;
  if(g.diagnose) printf("sig_quit %d\n", crashing);
  if(crashing==1) quitcb(g.main_widget,NULL,NULL); // prevent returning here
  exit(1);
}


//--------------------------------------------------------------------//
//  sig_abort                                                         //
//--------------------------------------------------------------------//
static void sig_abort(int signo)
{
  fprintf(stderr, "\nCaught signal %d (Abort)\n", signo);
  exit(1);
}


//--------------------------------------------------------------------//
//  resize_selection                                                  //
//--------------------------------------------------------------------//
void resize_selection(void)
{
   switch(g.state)
   {   case RESIZE_LEFT  : g.selected_ulx = g.mouse_x; break;
       case RESIZE_RIGHT : g.selected_lrx = g.mouse_x; break;
       case RESIZE_TOP   : g.selected_uly = g.mouse_y; break;
       case RESIZE_BOTTOM: g.selected_lry = g.mouse_y; break;
   }
   if(g.selected_ulx>g.selected_lrx) swap(g.selected_ulx,g.selected_lrx);
   if(g.selected_uly>g.selected_lry) swap(g.selected_uly,g.selected_lry);
   g.ulx = zoom_x_coordinate(g.selected_ulx, g.imageatcursor);
   g.uly = zoom_y_coordinate(g.selected_uly, g.imageatcursor);
   g.lrx = zoom_x_coordinate(g.selected_lrx, g.imageatcursor);
   g.lry = zoom_y_coordinate(g.selected_lry, g.imageatcursor);       
}



//--------------------------------------------------------------------//
// add_wm_close_callback                                              //
// prevent window manager from unceremoniously deleting our windows.  //
// not working yet.                                                   //
//--------------------------------------------------------------------//
void add_wm_close_callback(Widget w, XtCallbackProc closecb, void *arg)
{
    Display *d = XtDisplay(w);
    static Atom wm_delete_window_atom = 0;
    //// Intern atom if necessary
    if(wm_delete_window_atom == (Atom)None) 
    	wm_delete_window_atom = XmInternAtom(d, "WM_DELETE_WINDOW", True);
    if(wm_delete_window_atom == (Atom)None) return;
    XmAddWMProtocolCallback(w, wm_delete_window_atom, closecb, arg);
}

