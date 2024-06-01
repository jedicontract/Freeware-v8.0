//--------------------------------------------------------------------------//
// xmtnimage1.cc                                                            //
// Data conversion (itoa, doubletostring)                                   //
// Program data initialization                                              //
// Main switch statement                                                    //
// Basic graphics (box, circle, line, etc.)                                 //
// Latest revision: 03-20-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
extern PrinterStruct printer;
extern int in_printimage, in_save, in_read, in_calibrate, in_warp;
extern int in_camera, in_acquire, in_pattern_recognition;
extern int in_spot_dens_dialog;
extern int in_strip_dens_dialog;
const int LABEL_LENGTH = 128;
const int COMMAND_LENGTH = 128;
int CHARX=1800, CHARY=400;  
static char busy[MAXIMAGES];
void initialize_globals2(void);

//--------------------------------------------------------------------------//
// init_data                                                                //
// initialize global image arrays and other data                            //
// The 2d arrays are allocated as a 1d contiguous array, then variable-     //
//   aliased                                                                //
//--------------------------------------------------------------------------//
void init_data(void)
{
  if(g.diagnose){ printf("Initializing image data...");fflush(stdout); }
  int j,k, size;
  int bytesperpixel;
  z = new Image[MAXIMAGES];
  ci=0;                                // Current image
  g.image_count = 0;                   // Total no. of images

  bytesperpixel = (7+g.bitsperpixel)/8;

  ////  Allocate stringbufr - max. CHARX x CHARY

  g.stringbufr_1d = (uchar*)malloc(CHARX*CHARY*sizeof(uchar)*bytesperpixel);
  if(g.stringbufr_1d==0) { fprintf(stderr,"%s for stringbufr_1d\n",g.nomemory); exit(1); }
  g.stringbufr    = make_2d_alias(g.stringbufr_1d, CHARX*bytesperpixel, CHARY);
  g.stringbufr_ximage = createximage(CHARX,CHARY,g.bitsperpixel,g.stringbufr_1d);
  memset(g.stringbufr_1d, g.bcolor, CHARX*CHARY*g.off[g.bitsperpixel]);
  g.spixmap = createxpixmap(g.main_window,CHARX,CHARY,g.bitsperpixel);
  for(k=0;k<INFOWINDOWS;k++)
      g.mpixmap[k] = createxpixmap(g.info_window[k],CHARX,CHARY,g.bitsperpixel);

  g.region = NULL;
  g.region_1d = NULL;
  g.macrotext = new char[MACROLENGTH];
  g.macrotext[0] = 0;
  g.sketchtext = new char[MACROLENGTH];
  g.sketchtext[0] = 0;
  g.sketch_editor_widget = 0;
  g.block = 0;
  g.block_okay = 0;              // Some functions don't need to worry about blocking
  size = (int)sqrt(g.xres*g.xres + g.yres*g.yres);  // Maximum diagonal distance
  g.highest = new int[2*size];   // For Bezier curves
  g.scan = new double[2*size];   // For densitometry
  
  ////  Crash if any of the above runs out of memory
  if(g.macrotext==0){ fprintf(stderr, g.nomemory); exit(1); }
  
  ////  After create_drawing_area(), it is safe to set the palette
  ////  and create XColormaps. 

  g.state = NORMAL;
  if(g.diagnose)
  {   printf("done\n");
      printf("cwd  %s\n",g.startdir);
      printf("size %d\n",size);
  }  

  ////  Create background image (0)
  ////  Make background total x,y size to avoid extra resizing.
  newimage("Background", 0, 0, g.main_xsize, g.main_ysize, g.bitsperpixel, 
     g.colortype, 1, 0, 0, PERM, 1, 1, 0);
  ////  Put background color in the background framebuffer.

  for(j=0;j<z[0].ysize;j++)  
      putpixelbytes(z[0].image[0][j],g.bkg_image_color,z[0].xsize,g.bitsperpixel,1);

  if(g.bitsperpixel==8 && g.want_colormaps)  
  {    g.xcolormap = XCreateColormap(g.display, g.main_window, g.visual, AllocAll);  
       setpalette(g.palette);
  }


  repair(0);
  setimagetitle(0, "Background");
  if(g.autoundo) backupimage(0, 0);
#ifdef HAVE_LEX
  initialize_evaluator();
#endif
}


//--------------------------------------------------------------------------//
// initialize_globals                                                       //
// initialize global data                                                   //
//--------------------------------------------------------------------------//
void initialize_globals(void)
{

  int i,k,len;
  char *ptr=NULL;
  if(g.diagnose){ printf("Initializing globals...");fflush(stdout); }
  g.main_window=0;                         // Application's window
  for(k=0;k<INFOWINDOWS;k++)
     g.info_window[k]=0;                   // The window at left for information
  g.mainmenuheight = 20;
  g.drawarea2width = 120;

  g.tif_xsize=1;
  g.tif_ysize=1;
  strcpy(g.version, "3.5.2");
  strcpy(g.emailaddress,"tjnelson@brneurosci.org");
  strcpy(g.nomemory,"Insufficient memory");
  strcpy(g.scan_device, "/dev/scanner");  // Scanner device name
  strcpy(g.camera, "");                   // Camera driver
  g.camera_id = 0;                        // Which camera to use

  g.moving = 0;
  g.want_slew = 0;
  g.want_raise = 1;
  g.want_switching = 1;                   // 1=clicking on image switches to it
  g.want_byte_order = -1;                 // -1 = use default

  g.main_xsize = 500;
  g.main_ysize = 700;
  g.total_xsize = g.main_xsize + g.drawarea2width;
  g.total_ysize = g.main_ysize + (int)g.mainmenuheight;

  g.main_xpos  = 0;             // Upper left corner of main window
  g.main_ypos  = 0;             // Upper left corner of main window
  g.getout=0;                   // Flag for 'cancel'

  g.want_colormaps=0;           // 1=want to set palette 2=not enough colors last time
  g.bitsperpixel=0;
  g.sparse_packing=0;           // 1=X windows sparse pseudo 24bpp mode
  g.autoundo=1;
  g.imode=1;
  g.want_messages=2;
  g.draw_figure=NONE;
  g.sprayfac=12;
  g.diameter=8;
  g.cross_length=8;
  g.font = new char[256];
  strcpy(g.font,"*helvetica-bold-r-normal--12*");
  g.imagefont = new char[256];
  strcpy(g.imagefont,"*adobe-times-bold-r-normal--24-240-75-75*");
  g.window_handle_size = 2;     // Width at edge where clicking moves image

  g.line.window=0;
  g.line.type=0;
  g.line.color=g.fcolor;
  g.line.width=1;
  g.line.wantgrad=0;
  g.line.gradr=1;
  g.line.gradg=1;
  g.line.gradb=1;
  g.line.gradi=1;
  g.line.skew=0;
  g.line.perp_ends=1;
  g.line.wantarrow=0;
  g.line.outline=0;
  g.line.outline_color=0;
  g.line.arrow_width=8.0;
  g.line.arrow_outer_length=12.0;
  g.line.arrow_inner_length=8.0;
  g.line.ruler_scale=1.0;
  g.line.ruler_ticlength=5.0;
  g.line.antialias=0;

  g.mouse_button = 0;
  g.last_mouse_button = 0;
  g.getboxstate = 0;
  g.getpointstate = 0;
  g.getlinestate = 0;
  g.getlabelstate = 0;

  g.wantr=1;
  g.wantg=1;
  g.wantb=1;
  g.mouse_button=0;
  g.mouse_x=0;
  g.mouse_y=0;
  g.key_ascii=0;
  g.key_alt=0;
  g.key_ctrl=0;
  g.key_shift=0;
  g.image_count = 0;            // Total no. of images present
  g.imageatcursor = 0;          // Image no. of pixel at cursor pos.
  g.text_direction=HORIZONTAL;
  g.text_angle=0.0;
  g.want_format=-1;             // Overrides default save image format unless -1.
                                // (Allows macro to specify file format on a 
                                // separate line)

  g.read_cmyk=1;                // ==1 if image is to be interpreted as a cmyk
                                // ==0 if upper 8 bits are to be thrown away
  g.save_cmyk=0;                // ==1 if create cmyk image
  g.want_redbpp=8;
  g.want_greenbpp=8;
  g.want_bluebpp=8;
  g.want_blackbpp=0;
  g.want_noofcolors=3;
  g.usegrayscale=-1;
  g.want_quantize=1;            // ==1 if quantize,2=fit current palette 0=none
  g.jpeg_qfac=80;
  g.useheader=1;                // for custom image files
  g.xres=0;
  g.yres=0;
  g.signif = 4;                 // Significant digits to display 
  g.state=INITIALIZING;  // Program button-pressing state
  g.inmenu=0;                   // If in a menu, dialog box, sample palette,
                                // etc. prevents putting pixels in
                                // the current image buffer.
  g.false_color=0;              // no false color map
  g.tif_positive=1;
  g.tif_xoffset=0;
  g.tif_yoffset=0;
  g.tif_skipbytes=0;            // No. of extra junk bytes to skip at start
  g.bkgpalette=1;               // Palette of the background
  g.background_touched=0;       // Flag if any pixels were set on background
  g.csize=8;                    // Amount of cursor movement
  g.selected_ulx=0;
  g.selected_uly=0;
  g.selected_lrx=0;
  g.selected_lry=0;


  g.get_x1=0;
  g.get_y1=0;
  g.get_x2=0;
  g.get_y2=0;
  g.region_ulx=0;
  g.region_uly=0;
  g.region_lrx=0;
  g.region_lry=0;
  g.region_ino = -1;
  g.scancount=0;
  g.colortype=2;
  g.want_color_type=2;
  g.want_bpp=8;
  g.selectedimage = 0;          // Most recently-selected image 
  g.selected_is_square = 1;     // 1 = selected area is rectangular
  g.want_shell=0;               // If 1 = each image in separate frame
  g.window_border=1;            // If 1 = windows get complete frame   
  g.want_title=0;               // If 1 = windows get title   
  g.wm_topborderwidth=0;        // Top border provided by window manager
  g.wm_leftborderwidth=0;       // Left border provided by window manager
  g.xlib_cursor = XC_crosshair; // Default cursor type
  g.fcolor   = 0;
  g.bcolor   = 0;
  g.fc.red   = 0;               // The desired foreground and background
  g.fc.green = 0;               //  rgb values. Used in selecting a new fcolor
  g.fc.blue  = 0;               //  and bcolor if g.fcolor or g.bcolor is already
  g.bc.red   = 32;              //  taken. Otherwise, the colors are determined
  g.bc.green = 32;              //  entirely by g.fcolor, g.bcolor, and g.b_palette.
  g.bc.blue  = 32;
  g.luminr = 0.299;
  g.luming = 0.587;
  g.luminb = 0.114;

  g.window_handle_size = 2;     // Width at edge where clicking moves image
  g.nbuttons=0;
  g.ignore=0;
  g.waiting=0;
  g.spacing=4;
  g.text_spacing = 0;
  g.object_threshold.red = 50;  // 0-255
  g.object_threshold.green = 50;// 0-255
  g.object_threshold.blue = 50; // 0-255
  g.double_click_msec=400;      // milliseconds to wait for double clicking
  g.nbuttons=0;
  g.zoom_button=-1;             // Which button at left is for zoom
  g.copy=1;                     // 0=move 1=copy
  g.want_crosshairs=0;          // 1=full-screen crosshairs
  g.area_selection_mode=SINGLE; // scissors selection mode
  g.busy = 0;
  g.escape = 0;
  g.indialog = 0;
  g.fix_cmyk = 0;
  g.uselibtiff = 1;

  g.openlistcount = 0;
  g.openlist = new listinfo*[100];  // Array for pointers to up to 100 lists
                                    // that want to be dynamically updated
  g.graphcount = 0;
  g.graph = new Graph[100];         // Graphs are independent entities so must be global
  for(k=0; k<100; k++)
  {   g.graph[k].type = NONE;
      g.graph[k].pd = NULL;
  }
  if(g.openlist==NULL) { printf("Insufficient memory\n"); exit(1); }
                                       
  for (i=0; i<64; i++)              // Define grayscale colormap
  {
          g.gray_palette[i*4].red      =  i;      //black to white
          g.gray_palette[i*4].green    =  i;
          g.gray_palette[i*4].blue     =  i;

          g.gray_palette[i*4+1].red      =  i;
          g.gray_palette[i*4+1].green    =  i;
          g.gray_palette[i*4+1].blue     =  min(63,i+1);

          g.gray_palette[i*4+2].red      =  i;
          g.gray_palette[i*4+2].green    =  min(63,i+1);
          g.gray_palette[i*4+2].blue     =  i;

          g.gray_palette[i*4+3].red      =  i;
          g.gray_palette[i*4+3].green    =  min(63,i+1);
          g.gray_palette[i*4+3].blue     =  min(63,i+1);           
  }
  g.gray_palette[0].red = 0;
  g.gray_palette[0].green = 0;
  g.gray_palette[0].blue = 0;
  memcpy(g.fc_palette, g.gray_palette, 768);
  for(k=0; k<MAXIMAGES; k++) g.image_stack[k] = k;

  ////  Paths
  
  ////  Current directory
  getcwd(g.currentdir, FILENAMELENGTH-1);  
  len = strlen(g.currentdir)-1;            // eliminate trailing '/'
  if(g.currentdir[len]=='/') g.currentdir[len]=0;

  ////  Start-up dir.(return here before exit); location for print temp files
  strcpy(g.startdir, g.currentdir);        

  ////  $HOME/.tnimage for tnimage.ini, formats in Bourne
  ////  $home/.tnimage in tcsh

  ptr = getenv("HOME");
#ifdef __VMS
  ptr = vms2unix(ptr);
#endif

  if(ptr!=NULL) strcpy(g.homedir, ptr);    
  else 
  {   ptr = getenv("home");
      if(ptr!=NULL) strcpy(g.homedir, ptr);    
      else 
      {   strcpy(g.homedir, g.startdir);
          fprintf(stderr,"Unable to determine home directory in __FILE__ line __LINE__\n");
      }
  }
#ifdef __VMS
  strcat(g.homedir,"/dot_tnimage");
#else
  strcat(g.homedir,"/.tnimage");
#endif
  ////  Location of tnimage.hlp, etc.
  strcpy(g.helpdir, HELPDIR);   
  sprintf(g.formatpath,"%s/%s",g.homedir,"formats");
#ifdef __VMS
  sprintf(g.waveletpath,"./wavelets");
#else
  sprintf(g.waveletpath,"/usr/local/lib/wavelets");
#endif
  strcpy(g.fontpath, g.homedir);
 
  strcpy(g.compression,"/bin/gzip -f");       // Program to exec to compress image
  strcpy(g.decompression, "/bin/gunzip -c");  // Program to exec to decompress image
  strcpy(g.compression_ext,".gz");            // Extension added by compression program

  ////  Initialize printer defaults
   printer.copies = 1;
  printer.hsize = 4.0;
  printer.vsize = 11.0;
  printer.vpos = 0.0;
  printer.entire = 1;
  printer.hpos = 0.0;
  printer.ratio = 1.0;
  printer.intensity = 1;
  printer.paperxsize = 8.5;
  printer.paperysize = 11.0;
  printer.ifac = 1.0;
  printer.rfac = 1.0;
  printer.gfac = 1.0;
  printer.bfac = 1.0;
  printer.papertype = 0;
  printer.graybalance = 0;
  printer.depletion = 0;
  strcpy(printer.command, "lpr");
  strcpy(printer.devicename, "/dev/lp2");
  strcpy(printer.filename, "temp.print");

  printer.vertical = 1;
  printer.positive = 1;
  printer.palette = 2;         // 1-4 for B/W, RGB,CMY, or CMYK
  printer.dither = 8;          // # of printer pixels/screen pixel
  printer.quality = 0;
  printer.res = 300;
  printer.rotation = 0;
  printer.interpolate = 0;
  printer.language=0;          // 0=postscript 1=pcl
  printer.device = 1;

  //// move the rest to initialize_globals2 to avoid
  //// problems in solaris

  initialize_globals2();
}


//--------------------------------------------------------------------------//
// initialize_globals2                                                      //
// This must be broken into 2 shorter sections otherwise bad executable is  //
// created in Solaris.                                                      //
//--------------------------------------------------------------------------//
void initialize_globals2(void)
{

  //// Global defaults for dialog boxes
  int k;
  g.filter_type    = 1;
  g.filter_ksize   = 3;
  g.filter_maxbkg  = 0; 
  g.filter_range   = 7;
  g.filter_kmult   = 1; 
  g.filter_sharpfac = 50;
  g.filter_file[0]  = 0;
  g.filter_diffsize = 10;
  g.filter_ithresh  = 127;
  g.filter_local_scale = 10;
  g.filter_decay    = 0.8;
  g.filter_background = 0;
  g.measure_angle = 0;
  g.trace_wantsave = 0;
  g.trace_wantplot = 1;
  g.trace_wantslow = 0;
  g.trace_trackcolor = 0;
  g.circlewidth      = 1;
  g.curve_line_width = 1;
  g.circle_fill      = 0;
  g.circle_outline   = 1;
  g.circle_antialias = 0;

  g.gradient1_type  = 1;         /* gradient type 0=none, 1=radial*/
  g.gradient1_inner = 16777215;  /* inner gradient color for filled ellipses */
  g.gradient1_outer = 4608090;   /* outer gradient color for filled ellipses */
  g.gradientx1 = 0.75;           /* x angle for gradient 0-1 */
  g.gradienty1 = 0.75;           /* y angle for gradient 0-1 */
  g.gradient2_type  = 1;         /* gradient type 0=none, 1=radial*/
  g.gradient2_inner = 11847900 ; /* inner gradient color for filled ellipses */
  g.gradient2_outer = 0;         /* outer gradient color for filled ellipses */
  g.gradientx2 = 0.25;           /* x angle for gradient 0-1 */
  g.gradienty2 = 0.25;           /* y angle for gradient 0-1 */
  g.reflectivity = 0.75;         /* amount of shininess of gradient */
 


  g.mask_mode = MASK;
  g.mask_ino1 = 0;
  g.mask_ino2 = 0;
  g.mask_xoffset = 0;
  g.mask_yoffset = 0;

  g.rotate_anti_alias = 1;
  g.rotate_degrees = 0.0;
  g.rotate_label_degrees = 0.0;
  g.fill_color = g.fcolor;
  g.fill_mingrad = 0;
  g.fill_maxgrad = g.maxcolor;
  g.fill_minborder = 0;
  g.fill_maxborder = 0;
  g.fill_type = 0;
  g.fill_usermode = 2;

  g.read_raw = 0;
  g.read_grayscale = 0;
  g.read_convert = 0;
  g.read_skipswitch = 0;
  g.read_swap_bytes = 0;
  g.read_signedpixels = 0;
  g.read_split_frames = 0;

  g.paste_set0 = 3;
  g.spray_mode = FINESPRAY;

  g.create_method = 1;   
  g.create_border = 0;
  g.create_xsize = 128;
  g.create_ysize = 128;
  g.create_xpos = 0;
  g.create_ypos = 0;
  g.create_frames = 1;
  g.create_cimage = -1;
  g.create_shell = 0;
  g.create_auto_edge = 0;
  g.create_find_edge = 0;
  g.create_rows = 1;
  g.create_cols = 4;
  g.create_panel = 0;
  g.create_xspacing = 0;
  g.create_yspacing = 0;
  g.create_ulx = 0;
  g.create_uly = 0;
  g.create_lrx = 0;
  g.create_lry = 0;
  g.create_fixed_xsize = 0;
  g.create_want_fixed_xsize = 0;
  g.create_fixed_ysize = 0;
  g.create_want_fixed_ysize = 0;
  g.create_xmargin = 0;
  g.create_ymargin = 0;

  g.pattern_method  = 1;
  g.pattern_thresh = 0.5;
  g.pattern_pweight = 1.0;
  g.pattern_bweight = -0.05;
  strcpy(g.pattern_filename, "Pattern");
  g.pattern_wantlabels = 1;
  g.pattern_wantrectangles = 1;
  g.pattern_wantgraph = 1;
  g.pattern_wantdensgraph = 0;
  g.pattern_wantsave = 0;
  g.pattern_labelcolor = 255;
  g.pattern_size = 10;
  g.pattern_minsize = 1;
  g.pattern_maxsize = 10000;
  g.pattern_labelflags = 0x1;
  strcpy(g.pattern_datafile, "1.data");

  g.wavelet_type = PYRAMIDAL;
  g.wavelet_grid = 0;
  g.wavelet_maxlevels = 0;

  strcpy(g.wavelet_file, "none");
  strcpy(g.wavelet_index_range, "all");
  strcpy(g.wavelet_coeff_range, "all");
  strcpy(g.wavelet_ignore_range, "none");

  strcpy(g.wp.wave.filename, "none");       

  g.wp.gray_offset = 0;
  g.wp.want_gray_offset = 0;
  g.wp.levels = 4;
  g.wp.ntabs = 4;
  g.wp.direction = 0;
  g.wp.ino = 0;
  strcpy(g.wp.format, "pyramid");  
  g.wp.minres = 0;
  g.wp.imin = 0;
  g.wp.imax = 0;
  g.wp.cmin = 0;
  g.wp.cmax = 0;
  g.wp.ignmin = 0;
  g.wp.ignmax = 0;


  g.morph_maxsignal = 1;  // black
  g.morph_type = 2;       // grayscale
  g.morph_se = 1;         // 1=4 2=8 conn set
  g.morph_operation = -1;
  g.morph_thresh = 0.5;
  g.morph_ksize = 1;      // watershed kernel
  g.morph_contour_separation = 10;

  g.warp_gridpoints = 20;
  g.warp_cursor = 4;
  g.calib_type = 1;
  g.attributes_set0 = 1;
  g.attributes_isource = 0;
  g.attributes_idest = 0;

  g.raw_platform = PC;
  g.raw_colortype = GRAY;
  g.raw_xsize = 0;
  g.raw_ysize = 0;
  g.raw_skip = 0;
  g.raw_bpp = 8;
  g.raw_frames = 1;
  g.raw_rbpp = 0;
  g.raw_gbpp = 0;
  g.raw_bbpp = 0;
  g.raw_kbpp = 0;
  g.raw_packing = NONE;
  g.raw_want_increment = 0;
  g.raw_want_color = 0;
  g.raw_inc_inc = 1;
  g.raw_flipbits = 0;
  g.raw_signed = 0;
  strcpy(g.raw_filename2, "none");

  g.read3d_bpp = 8;
  g.read3d_frames = 15;
  g.read3d_xsize = 128;
  g.read3d_ysize = 128;
  g.read3d_skipbytes = 0;
  g.curve_type = 0;
    
  g.fft_ino2 = 1;
  g.fft_ino1 = 1;
  g.fft_direction = -1;
  g.fft_wantdisplay = -1;
   
  g.save_ascii_format = INTEGER;    

  g.rcontrast = 100;
  g.gcontrast = 100;
  g.bcontrast = 100;
  g.hcontrast = 100;
  g.scontrast = 100;
  g.vcontrast = 100;
  g.icontrast = 100;
    
  g.radjust = 0;
  g.gadjust = 0;
  g.badjust = 0;
  g.hadjust = 0;
  g.sadjust = 0;
  g.vadjust = 0;
  g.iadjust = 0;

  g.raddvalue = 0;
  g.gaddvalue = 0;
  g.baddvalue = 0;
  g.haddvalue = 0;
  g.saddvalue = 0;
  g.vaddvalue = 0;
  g.iaddvalue = 0;

  g.spot_dens_source = 1;
  g.spotlist  = NULL;
  g.zoomfac   = 1.5;  // Incremental factor to zoom by when user clicks
  g.floating_magnifier = 0;
  g.floating_magnifier_on_root = 1;
  g.in_crop = 0;
  g.helptopic = new int[1000];
  for(k=0;k<1000;k++) g.helptopic[k] = k;

  ////  Set locale-specific information (overridden by tnimage.ini)
  char *locale;
  struct lconv *lcv;
  char DECIMAL_POINT;
  char FUNC_PARAM_SEPARATOR;
  if((locale = setlocale(LC_ALL,""))==NULL)
       fprintf(stderr,"%s: cannot set locale\n",g.appname);
  if(XSetLocaleModifiers("")==NULL)
       fprintf(stderr, "Cant set locale modifiers\n");
  lcv = localeconv();
  DECIMAL_POINT = lcv->decimal_point[0];
  if(DECIMAL_POINT==',')
  {   FUNC_PARAM_SEPARATOR='.';
      fprintf(stderr, "Decimal point is %c\n",DECIMAL_POINT);
  }      
  else FUNC_PARAM_SEPARATOR=',';
  g.decimal_point = DECIMAL_POINT;
#ifdef __VMS
fprintf(stderr, "Decimal point is %c\n",DECIMAL_POINT);
#endif
  g.draw_pattern = 0;
  g.inpattern = 0;
  if(g.diagnose)
  {   printf("Done\n");
      printf(" Version %s\n",g.version);
      fflush(stdout); 
  }
}



//--------------------------------------------------------------------------//
// clear_busy_flags                                                         //
//--------------------------------------------------------------------------//
void clear_busy_flags(void)
{
  for(int k=0; k<MAXIMAGES; k++) busy[k]=0;
}


//--------------------------------------------------------------------------//
// execute                                                                  //
// execute a command from menu or a macro. This method is better than       //
// having a separate callback for each menu item because it makes it        //
// easy to use macros.                                                      //
// Commands that change image depth or erase image buffers check if         //
//   image is 'busy' before proceeding (a primitive form of thread-safety). //
// command = entire command line                                            //
// arg     = the arguments in command line = command demarcated by spaces   //
// arg[0] MUST contain the command if noofargs is >0                        //
//--------------------------------------------------------------------------//
int execute(char *command, int noofargs, char **arg, int loopmode)
{
  static char **oldarg=NULL;                // up to 20 arguments
  static char oldo[256];
  static int oldnoofargs=0;
  int ino,f,j,k,status=OK;
  char o[256], commandstring[256];
  char tempstring[256];
  sanity_check();
  g.draw_figure=NONE;
  g.key_alt=0;
  g.key_ctrl=0;
  g.key_shift=0;

  g.getout = 0;
  g.state = NORMAL;
  Graph nograph;
  nograph.type = NONE;
  nograph.pd = NULL;
  nograph.ino = 0;
  if(g.zooming){ g.zooming = 0; g.getout = 1; return ABORT; }
  if(arg == NULL) noofargs=0;
  int k2, done;
  int e=0;   // flag if something was executed
  int oino = ci;

  if(oldarg==NULL)                          // remain allocated forever
  {     oldarg = new char*[20];
        for(k=0;k<20;k++){ oldarg[k] = new char[128]; oldarg[k][0]=0; }
  }
  if(g.diagnose)
  {    printf("Executing %s, %d args\n",command,noofargs); 
       if(arg != NULL)
            for(k=0; k<=noofargs; k++) printf("  arg[%d] = %s\n",k,arg[k]);
       else return ERROR;
  }

  ////  arg[0] is guaranteed to be a single word so use it if possible,
  ////  otherwise use the command line (which could contain spaces, quotes, etc).
  if(noofargs)
  {           
      if(arg != NULL) strcpy(commandstring, arg[0]);
      else commandstring[0]=0;
  }
  else  
      strcpy(commandstring, command);

  done = 0;
  for(k=0,j=0; k<(int)strlen(commandstring); k++)
  {
     switch(commandstring[k])
     {     case '(':
           case '{':
           case '}':
           case ';':
           case '\n':
           case '/': done=1; break;
           case '.':
           case '_':
           case ' ': continue;
           default : o[j] = tolower(commandstring[k]);
                     o[++j]=0;
     }
     if(done) break; 
  }
  if(!strlen(o)) return ZEROLENGTH; /// command was entirely junk

  //----------- The commands (in alphabetical order) -------------//
  //----------- (mostly)                             -------------//

  if(strcmp(o, "repeat")==SAME && strcmp(o, "undo")!=SAME && arg != NULL)
  {  
        if(strlen(oldo)) strcpy(o, oldo);
        noofargs = oldnoofargs;
        for(k=0;k<noofargs;k++) 
        strcpy(arg[k], oldarg[k]);
  }

  if(strcmp(o, "3d")==SAME){ e=1; menu3d(); }
  if(strcmp(o, "3dplot")==SAME){ e=1; plot3d(ci); }
  if(strcmp(o, "aboutthefile")==SAME){ e=1; aboutthefile(noofargs, arg); }
  if(strcmp(o, "abouttheimage")==SAME){ e=1; abouttheimage(noofargs, arg); }
  if(strcmp(o, "abouttheprogram")==SAME){ e=1; abouttheprogram(); }
  if(strcmp(o, "acquire")==SAME){ e=1; acquire(); }
  if(strcmp(o, "addborder") == SAME){ e=1; image_border(); }
  if(strcmp(o, "alternate")==SAME){ e=1; alternate(noofargs, arg); }
  if(strcmp(o, "animate")==SAME){ e=1; animate(noofargs, arg); }
  if(strcmp(o, "attributes")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; attributes(); busy[oino]--; } }
  if(strcmp(o, "automaticundo")==SAME){ e=1; g.autoundo=atoi(arg[1]); }
  if(strcmp(o, "backgroundcolor")==SAME){ e=1; setbcolor(noofargs, arg); }
  if(strcmp(o, "backgroundvalue")==SAME){ e=1; g.bcolor=atoi(arg[1]); }
  if(strcmp(o, "bcolor")==SAME){ e=1; g.bcolor=atoi(arg[1]); }
  if(strcmp(o, "backup")==SAME){ e=1; backupimage(ci,1); }
  if(strcmp(o, "beep") == SAME){ e=1; XBell(g.display,0);  }
  if(strcmp(o, "border") == SAME){ e=1; image_border(); }
  if(strcmp(o, "box")==SAME)
  {  e=1; 
     clickbox("Box width", 1, &g.line.width, 1, 100, null, cset, null, NULL, NULL, 0);
  }
  if(strcmp(o, "brightness")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; changebrightness(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "calibration")==SAME){ e=1; calibrate();  }
  if(strcmp(o, "camera")==SAME){ e=1; camera(); }
  if(strcmp(o, "cancel")==SAME)
  {  e=1; 
     maincancelcb(0,0,0); 
     unpush_null_buttons();
     return ABORT;      //// Break out so g.getout isnt reset
  }
  if(strcmp(o, "changedepth")     ==SAME ||
     strcmp(o, "changeimagedepth")==SAME ||
     strcmp(o, "changecolordepth")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=changecolordepth(noofargs,arg); busy[oino]--;} }
  if(strcmp(o, "changecolormap")==SAME){ e=1; changepalette(noofargs,arg); }
  if(strcmp(o, "changecontrast")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; changecontrast(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "changepixelvalues")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; addvalue(noofargs,arg); busy[oino]--; } } 
  if(strcmp(o, "changesizereduplicate")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=shrink(noofargs,arg,0); busy[oino]--; } }
  if(strcmp(o, "changesizeinterpolate")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=shrink(noofargs,arg,1); busy[oino]--; } }
  if(strcmp(o, "changetitle")==SAME || strcmp(o, "title")==SAME){ e=1; changetitle(noofargs,arg);}
  if(strcmp(o, "chromaticaberration")==SAME){ e=1; chromaticaberration();}
  if(strcmp(o, "circle")==SAME){  e=1; set_circle_parameters(); }
  if(strcmp(o, "clearalpha")==SAME){ e=1; clear_alpha(ci); }
  if(strcmp(o, "clearalphachannel")==SAME){ e=1; clear_alpha(ci); }
  if(strcmp(o, "color")==SAME){ e=1; changebrightness(noofargs,arg); }
  if(strcmp(o, "color->grayscale")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; converttograyscale(ci); busy[oino]--; } }
  if(strcmp(o, "colorgrayscale")==SAME || strcmp(o, "colortograyscale")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; converttograyscale(ci); busy[oino]--; } }
  if(strcmp(o, "colorbrightness")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; changebrightness(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "colormap")==SAME){ e=1; changepalette(noofargs,arg); }
  if(strcmp(o, "compositergb")==SAME){ e=1;status=combine_colors(noofargs,arg); }
  if(strcmp(o, "configure")==SAME){ e=1; configure(); }
  if(strcmp(o, "contrast")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; changecontrast2(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "convert")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=changecolordepth(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "copywavelets")==SAME){ e=1; copy_wavelets(ci); }
  if(strcmp(o, "createfileformat")==SAME){ e=1; create_custom_format(); }
  if(strcmp(o, "createimage")==SAME){ e=1; status=createimage(noofargs,arg); }
  if(strcmp(o, "createresizeimage")==SAME){ e=1; status=createimage(noofargs,arg); }
  if(strcmp(o, "crop")==SAME){ e=1; crop(noofargs,arg); }
  if(strcmp(o, "cross")==SAME)
  {   e=1;
      clickbox("Cross length", 15, &g.cross_length, 1, 400, null, cset, null, NULL, NULL, 0);
  }
  if(strcmp(o, "curve")==SAME){ e=1; curve(); } 
  if(strcmp(o, "curvedensitometry")==SAME){ e=1; curve_densitometry(); }
  if(strcmp(o, "darken")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; darken(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "deleteregion")==SAME){ e=1; paint(g.bcolor); }
  if(strcmp(o, "densitometry")==SAME){ e=1; scan_region_macro(noofargs, arg); }
  if(strcmp(o, "down") == SAME){ e=1; image_jump(noofargs, arg, DOWN); g.moving=0;}
  if(strcmp(o, "drawbox") == SAME){ e=1; drawline(noofargs, arg, BOX); }
  if(strcmp(o, "drawcircle") == SAME){ e=1; drawline(noofargs, arg, CIRCLE); }
  if(strcmp(o, "drawfilledbox") == SAME){ e=1; drawline(noofargs, arg, FILLEDRECTANGLE); }
  if(strcmp(o, "drawfilledrectangle") == SAME){ e=1; drawline(noofargs, arg, FILLEDRECTANGLE); }
  if(strcmp(o, "drawlabel") == SAME){ e=1; drawlabel(noofargs, arg); }
  if(strcmp(o, "drawline") == SAME){ e=1; drawline(noofargs, arg, LINE); }
  if(strcmp(o, "drawrectangle") == SAME){ e=1; drawline(noofargs, arg, RECTANGLE); }
  if(strcmp(o, "duplicate")==SAME){ e=1; duplicate(noofargs, arg); }
  if(strcmp(o, "erasebackground")==SAME){ e=1; erasebackground(); }
  if(strcmp(o, "erasefft") == SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; erase_fft(ci); busy[oino]--; } }
  if(strcmp(o, "executeplugin") == SAME || strcmp(o, "plugin") == SAME){ e=1; select_plugin(noofargs,arg); }
  if(strcmp(o, "exit") == SAME){ e=1; status=QUIT; exit(0); }
  if(strcmp(o, "fft")==SAME){ e=1; status=transform(noofargs,arg); }
  if(strcmp(o, "fftdisplay")==SAME)
  {    e=1; 
       if(ci>=0)
       {    z[ci].fftdisplay=atoi(arg[1]); 
            scalefft(ci);
            rebuild_display(ci);  
            redraw(ci);
       }
  }
  if(strcmp(o, "fileformat")==SAME){ e=1; g.want_format=atoi(arg[1]); }
  if(strcmp(o, "fill")==SAME){ e=1;  fillregion();  }
  if(strcmp(o, "fillregion")==SAME){ e=1;  fillregion(); }
  if(strcmp(o, "filter")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; filter_start(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "findspots")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=macro_find_spots(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "countgrains")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=macro_find_spots(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "fliphoriz")==SAME){ e=1; flip_horiz(); }     
  if(strcmp(o, "fliphorizontally")==SAME){ e=1; flip_horiz(); }     
  if(strcmp(o, "flipvertically")==SAME || strcmp(o, "flipvert")==SAME){ e=1; flip_vert(); }
  if(strcmp(o, "floatingmagnifier")==SAME)
  {    e=1; 
       g.floating_magnifier = 1 - g.floating_magnifier;
       if(g.floating_magnifier) floating_magnifier_dialog(); 
       else destroy_floating_magnifier(); 
  }
  if(strcmp(o, "font")==SAME){ e=1; changefont(); }
  if(strcmp(o, "freetypefont")==SAME){ e=1; select_freetype_font(); }
  if(strcmp(o, "fopen")==SAME){ e=1; macro_open(arg[1]); }
  if(strcmp(o, "fclose")==SAME){ e=1; macro_close(arg[1]); }
  if(strcmp(o, "foregroundcolor")==SAME){ e=1; setfcolor(noofargs, arg); }
  if(strcmp(o, "foregroundvalue")==SAME){ e=1; g.fcolor=atoi(arg[1]); }
  if(strcmp(o, "foregroundpattern")==SAME){ e=1; select_pattern(); }
  if(strcmp(o, "fcolor")==SAME){ e=1; g.fcolor=atoi(arg[1]); }
  if(strcmp(o, "frame")==SAME)
  {    e=1; 
       f=atoi(arg[1]);
       if(ci>=0 && between(f,0,z[ci].frames-1))
       {    z[ci].cf = f; 
            rebuild_display(ci); 
            redraw(ci);
       }
  }
  if(strcmp(o, "freegraincountingarrays")==SAME){ e=1; free_grain_counting_arrays(); }
  if(strcmp(o, "grabfromscreen")==SAME){ e=1; grab_window(); }
  if(strcmp(o, "gamma")==SAME){ e=1; gamma(noofargs, arg); }
  if(strcmp(o, "gradientremoval")==SAME){ e=1; remove_gradient(noofargs, arg); }
  if(strcmp(o, "graycontrast")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; changegraycontrast(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "grayscale->color")==SAME || strcmp(o, "grayscalecolor")==SAME
  || strcmp(o, "grayscaletocolor")==SAME)
  {   e=1; converttocolor(ci); }
  if(strcmp(o, "grains")==SAME) { e=1; pattern_recognition(); }
  if(strcmp(o, "grayscalemap")==SAME){ e=1; grayscalemap(); }
  if(strcmp(o, "help") == SAME){ e=1; help(1); }
  if(strcmp(o, "histogram")==SAME){ e=1; histogram(nograph); }
  if(strcmp(o, "histogramequalize")==SAME){ e=1; histogram_equalize(ci); }
  if(strcmp(o, "image") == SAME){ strcpy(o,"selectimage"); }
  if(strcmp(o, "imagenotes")==SAME ||
     strcmp(o, "notes")==SAME ){ e=1; show_notes(ci); }
  if(strcmp(o, "specialcharacter")==SAME){ e=1; specialcharacter(); }
  if(strcmp(o, "intensity")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; change_intensity(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "invertcolors")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; invertcolors(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "invertregion")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; invertcolors(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "label")==SAME){ e=1; label(); }
  if(strcmp(o, "left") == SAME){ e=1; image_jump(noofargs, arg, LEFT); g.moving=0; }
  if(strcmp(o, "lighten")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; lighten(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "line")==SAME || strcmp(o, "arrow")==SAME)
  {  e=1; 
     set_line_parameters();
  }
  if(strcmp(o, "linestyle")==SAME){ e=1; set_line_parameters(); }
  if(strcmp(o, "loadfft") == SAME){ e=1; readfft(noofargs,arg); }
  if((strcmp(o,"loadimage")==SAME)||
     (strcmp(o,"load")==SAME)||
     (strcmp(o,"open")==SAME)||
     (strcmp(o,"openimage")==SAME)||
     (strcmp(o,"read")==SAME))
  {  e=1; readstart(noofargs, arg); }
  if(strcmp(o, "localcontrast")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; localcontrast(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "mask")==SAME){ e=1; mask(noofargs,arg); } 
  if(strcmp(o, "maximize") == SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; maximize_contrast(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "maximizecontrast") == SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; maximize_contrast(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "imagemath")==SAME || 
     strcmp(o, "macro")==SAME)
  {  e=1;
     if(noofargs>0)
     {   readmacrofile(arg[1], g.macrotext);
         status=macro(g.macrotext);
     }else  
     {  
        edit(g.appname,
             "Command editor (Shift-Enter to execute command)",
             g.macrotext,80,50,0,350,MACROLENGTH,30,1,NULL,null,NULL);   
     } 
  }    
  if(strcmp(o, "magnify")==SAME){ e=1; zoom(noofargs, arg, MAGNIFY); }
  if(strcmp(o, "measure")==SAME){ e=1; measure(); }
  if(strcmp(o, "message")==SAME){ e=1; message(arg[1]); }
  if(strcmp(o, "messages")==SAME)
  {   e=1; 
      if(noofargs>0) g.want_messages = atoi(arg[1]); 
  }
  if(strcmp(o, "morphfilter")==SAME){ e=1; morphfilter(noofargs, arg); }
  if(strcmp(o, "move")==SAME){ e=1; g.copy=1-g.copy; reset_user_buttons(); }
  if(strcmp(o, "movie") == SAME){ e=1; movie(atoi(arg[1])); }
  if(strcmp(o, "moveto")==SAME)
  {   e=1;
      if(noofargs>0) z[ci].xpos = atoi(arg[1]); 
      if(noofargs>1) z[ci].ypos = atoi(arg[2]); 
      moveimage(ci, z[ci].xpos, z[ci].ypos);
  }
  if(strcmp(o, "new")==SAME){ e=1; status=createimage(noofargs,arg); }
  if(strcmp(o, "null")==SAME)
  {  e=1; 
     unpush_null_buttons(); 
     return ABORT;      //// Break out so g.getout isnt reset
  }
  if(strcmp(o, "paint")==SAME){ e=1; paint(g.fcolor); }
  if(strcmp(o, "paintregion")==SAME){ e=1; paint(g.fcolor); }
  if(strcmp(o, "palette")==SAME){ e=1; changepalette(noofargs,arg); }
  if(strcmp(o, "partition")==SAME){ e=1; partition(); }
  if(strcmp(o, "paste")==SAME){ e=1; paste(); }
  if(strcmp(o, "patternrecognition")==SAME) { e=1; pattern_recognition(); }
  if(strcmp(o, "pixelinteractmode")==SAME){ e=1; pixelmode(noofargs,arg); }
  if(strcmp(o, "printimage") == SAME){ e=1; printimage(); }
  //// 'print' is a math function if it has arguments
  if(strcmp(o, "print") == SAME && noofargs==0){ e=1; printimage(); }
  if(strcmp(o, "readfft") == SAME){ e=1; readfft(noofargs,arg); }
  if(strcmp(o, "quit") == SAME) { e=1; quitcb(0,0,0);; }          
  if(strcmp(o, "remapcolors") == SAME){ e=1; remapcolors(); }
  if(strcmp(o, "reselectarea") == SAME){ e=1; reselect_area(g.area_selection_mode); }
  if(strcmp(o, "resize")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; resize(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "resizewindow")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; resize(noofargs, arg); busy[oino]--; } }
  if(strcmp(o, "resizeimage")==SAME){ e=1; status=createimage(noofargs,arg); }
  if(strcmp(o, "redraw") == SAME){ e=1; redraw(ci); } 
  if(strcmp(o, "imageregistration") == SAME){ e=1; registration(); } 
  if(strcmp(o, "registration") == SAME){ e=1; registration(); } 
  if(strcmp(o, "reinitialize") == SAME){ e=1; reinitialize(); } 
  if(strcmp(o, "removegradient")==SAME){ e=1; remove_gradient(noofargs, arg); }
  if(strcmp(o, "repair") == SAME)
  {    e=1; 
       //z[ci].hitgray = 0;   // Force remapping of gray levels
       repair(ci); 
  } 
  if(strcmp(o, "restore") == SAME){ e=1; restoreimage(1); } 
  if(strcmp(o, "right") == SAME){ e=1; image_jump(noofargs, arg, RIGHT); g.moving=0; }
  if(strcmp(o, "root") == SAME){ e=1; root(ci, ON); } 
  if(strcmp(o, "rootpermanent") == SAME){ e=1; root(ci, PERMANENT_TILE); } 
  if(strcmp(o, "rootoff") == SAME){ e=1; root(ci, OFF); } 
  if(strcmp(o, "rotate")==SAME){ e=1; status = rotate("Rotate image", &g.rotate_degrees, noofargs, arg, NULL, null, null); } 
  if(strcmp(o, "rotateimage")==SAME){ e=1; status = rotate("Rotate image", &g.rotate_degrees, noofargs, arg, NULL, null, null); }
  if(strcmp(o, "save")==SAME){ e=1; status=saveimage(ci,noofargs,arg); }
  if(strcmp(o, "savefft") == SAME){ e=1; writefft(ci); }
  if(strcmp(o, "saveimage")==SAME){ e=1;status=saveimage(ci,noofargs,arg); }
  if(strcmp(o, "scan")==SAME){ e=1; acquire(); }
  if( ((strcmp(o, "selectaregion")) == SAME)||
      ((strcmp(o, "selectregion")) == SAME) ||
      ((strcmp(o, "select")) == SAME))
  {  e=1;
     if(noofargs==0)
        getbox(g.selected_ulx, g.selected_uly, g.selected_lrx, g.selected_lry); 
     else
        select_region(atoi(arg[1]),atoi(arg[2]),atoi(arg[3]),atoi(arg[4]));
  }
  if(strcmp(o, "selectirregulararea")==SAME ||
     strcmp(o, "selectarea")==SAME || 
     strcmp(o, "manuallyselectarea")==SAME ||
     strcmp(o, "scissors")==SAME) { e=1; select_area(g.area_selection_mode); }
  if(strcmp(o, "scanner")==SAME){ e=1; acquire(); }
  if((strcmp(o, "selectimage") == SAME) ||
     (strcmp(o, "switchto") == SAME))
  {  e=1;
     if(noofargs>=1)
     {    ino = image_number(arg[1]);
          switchto(ino); 
     }else image_list();
  }   
  if(strcmp(o, "separatergb")==SAME){ e=1; status=separate_colors(ci,noofargs,arg); }
  if(strcmp(o, "set")==SAME){ e=1; set_value(command); }
  if(strcmp(o, "setbackgroundcolor")==SAME){ e=1; setbcolor(noofargs, arg); }
  if(strcmp(o, "setbcolor")==SAME){ e=1; setbcolor(noofargs, arg); }
  if(strcmp(o, "setcolor") == SAME){ e=1; setfcolor(noofargs, arg); }
  if(strcmp(o, "setforegroundcolor") == SAME){ e=1; setfcolor(noofargs, arg); }
  if(strcmp(o, "setfcolor")==SAME){ e=1; setfcolor(noofargs, arg); }
  if(strcmp(o, "setpixel") == SAME){ e=1; drawline(noofargs, arg, PIXEL); }
  if(strcmp(o, "shiftframe") == SAME){ e=1; shift_frame(noofargs,arg); }
  if(strcmp(o, "shift") == SAME){ e=1; shift_frame(noofargs,arg); }
  if(strcmp(o, "showalpha")==SAME){ e=1; show_alpha(ci); }
  if(strcmp(o, "showalphachannel")==SAME){ e=1; show_alpha(g.selectedimage); }
  if(strcmp(o, "showodtable")==SAME){ e=1; showodtable(); }
  if(strcmp(o, "showcolorpalette")==SAME){ e=1; select_color(); } 
  if(strcmp(o, "showpalette")==SAME){ e=1; samplepalette(); } 
  if(strcmp(o, "showcolormap")==SAME){ e=1; samplepalette(); } 
  if(strcmp(o, "showselected")==SAME){ e=1; show_selected(g.selectedimage); }
  if(strcmp(o, "size")==SAME || strcmp(o, "shrink")==SAME)
  { e=1; if(!busy[oino]){ busy[oino]++; status=shrink(noofargs,arg,0); busy[oino]--; } }
  if(strcmp(o, "sketch")==SAME)
  {   e=1; 
      if(g.draw_figure==SKETCH) g.draw_figure=NONE; 
      else g.draw_figure=SKETCH; 
      reset_user_buttons();  
  }
  if(strcmp(o, "sketchmath")==SAME)
  {   e=1; 
      g.sketch_editor_widget = get_math_formulas(g.sketchtext, NORMAL);
      if(g.draw_figure==SKETCHMATH) g.draw_figure=NONE; 
      else g.draw_figure=SKETCHMATH; 
  }
  if(strcmp(o, "slew")==SAME){ e=1; g.want_slew=1-g.want_slew; reset_user_buttons(); }
  if(strcmp(o, "spotdensitometry")==SAME){ e=1; densitometry(); }
  if(strcmp(o, "spray")==SAME) { e=1; status=spray(noofargs,arg); }
#ifdef HAVE_XBAE
  if(strcmp(o, "spreadsheet")==SAME) { e=1; create_spreadsheet(ci); }
#endif
  if(strcmp(o, "step") == SAME)
  {   e=1; 
      if(noofargs==0) clickbox("Step size", 0, &g.csize, 0, 1000, null, null, 
                      null, NULL, NULL,0);
      else g.csize = atoi(arg[1]);
  }
  if(strcmp(o, "stripdensitometry")==SAME){ e=1; scan_region(); }
  if(strcmp(o, "switchselected")==SAME){ e=1; switch_selected_region(); }
  if(strcmp(o, "ttt")==SAME){ e=1; test1(); }
  if(strcmp(o, "testplugin") == SAME)
  {    e=1; 
       strcpy(arg[2],"0");
       select_plugin(noofargs,arg); 
  }
  if(strcmp(o, "textdirection")==SAME)
  {  e=1;
     if(g.text_direction==HORIZONTAL) g.text_direction=VERTICAL;
     else g.text_direction=HORIZONTAL;
     message("Text direction changed");
  }
  if(strcmp(o, "textangle")==SAME)
  {  e=1;
     sprintf(tempstring, "%g", g.text_angle);
     getstring("Text angle", tempstring, 256, 0);
     g.text_angle = atof(tempstring);
  }

  if(strcmp(o, "tracecurve")==SAME){ e=1; status=trace_curve(); }
  if(strcmp(o, "chromakey")==SAME)
  {   e=1; 
      if(ci>=0) 
      {   z[ci].chromakey = 1-z[ci].chromakey; 
          rebuild_display(ci);
          redraw(ci);
          redraw_chromakey_image(ci);
      }
  }
  if(strcmp(o, "erase")==SAME)
  {   e=1;
      message("Spray erase mode\nClick Cancel when finished");
      sprintf(arg[1],"%d",ERASESPRAY);
      noofargs=1;
      spray(noofargs,arg);
  }
  if(strcmp(o, "threshold")==SAME){ e=1; threshold(ci, (uint)atoi(arg[1])); }
  if(strcmp(o, "transparency")==SAME){ e=1; set_transparency(ci,noofargs,arg); }
  if(strcmp(o, "undo") == SAME){ e=1; restoreimage(1); } 
  if(strcmp(o, "unloadimage") == SAME ||
     strcmp(o, "unload") == SAME      ||
     strcmp(o, "eraseimage") == SAME       ||
     strcmp(o, "close") == SAME)  
     { e=1;  if(!busy[oino]){ busy[oino]++; unload_image(noofargs,arg); busy[oino]--; } }
  if(strcmp(o, "unloadall") == SAME || strcmp(o, "closeallimages") == SAME) 
  {    e=1;
       k2 = g.image_count;
       //// This changes g.image_count and renumbers the images
       for(k=1; k<k2; k++)
       {  if(!busy[k]){ busy[k]++; eraseimage(1,1,0,0); busy[k]--; } }
       redrawscreen();
  }
  if(strcmp(o, "unsetpixel") == SAME){ e=1; unsetpixel(noofargs, arg); } 
  if(strcmp(o, "unzoom")==SAME){ e=1; unzoom(ci); }
  if(strcmp(o, "updatecolor") == SAME){ e=1; update_color(); } 
  if(strcmp(o, "up") == SAME){ e=1; image_jump(noofargs, arg, UP); g.moving=0; }
  if(strcmp(o, "wallpaper") == SAME){ e=1; for(k=0;k<2;k++) root(ci, PERMANENT_TILE); } 
  if(strcmp(o, "warp")==SAME){ e=1; warp(); }
  if(strcmp(o, "wavelets")==SAME){ e=1; wavelet(noofargs,arg); }
  if(strcmp(o, "whatis") == SAME || strcmp(o, "evaluate") == SAME)
  {    e=1; check_value(command);
  }
  if(strcmp(o, "windows")==SAME){ e=1; g.want_shell=atoi(arg[1]); }
  if(strcmp(o, "windowborder")==SAME){ e=1; g.window_border=atoi(arg[1]); }
  if(strcmp(o, "wantimagetitle")==SAME){ e=1; g.want_title=atoi(arg[1]); }
  if(strcmp(o, "zoom")==SAME){ e=1; zoom(noofargs, arg, ZOOM); }

  //------------------------------------------------//
  g.getout=0; 
  if(e==0) status=UNKNOWN;
  g.state=NORMAL;
  if(g.draw_figure) g.state=DRAWING;
  printstatus(g.state);
  if(g.diagnose)
  {  printf(" status=%d, %d, %d, %d\n",status,g.state,g.draw_figure,g.inmenu); 
     printf(" images=%d ci=%d\n",g.image_count,ci); 
     print_image_info();
     fflush(stdout);
  }

  if(e)
  {   for(k=0;k<g.openlistcount;k++) update_list(k);
      for(k=0;k<g.image_count;k++) 
           if(z[k].s->visible && z[k].touched) putimageinspreadsheet(k);
      update_sample_palette();
  }else if(strlen(command))
  {   //// If called from a loop, execute the command once; otherwise,
      //// iterate over all the pixels.
      if(loopmode==1)
          eval(command, NULL);
      else
          math(command);
  }
  if(g.autoundo==2 && ci>=0) backupimage(ci,0);
  strcpy(oldo, o);
  if(arg != NULL) for(k=0;k<noofargs;k++) strcpy(oldarg[k], arg[k]);
  oldnoofargs = noofargs;
  XtSetKeyboardFocus(g.main_widget, z[ci].widget);  // put focus back on image
  return status;
}



//--------------------------------------------------------------------------//
// setimagetitle                                                            //
//--------------------------------------------------------------------------//
void setimagetitle(int ino, const char *title)
{   
  if(ino>=0 && strlen(title))
  {    strcpy(z[ino].name, title);    
       if(z[ino].shell)
           XtVaSetValues(XtParent(XtParent(z[ino].widget)), 
               XmNtitle, z[ino].name,  
               XmNiconName, z[ino].name, 
               (char *)0);
  }
}


//--------------------------------------------------------------------------//
// set_transparency                                                         //
//--------------------------------------------------------------------------//
void set_transparency(int ino, int noofargs, char **arg)
{   
  if(ino<0) return;
  int t = z[ino].transparency;
  if(noofargs==0) clickbox("Transparency", 3, &t, 0, 100, null, cset, 
                  null, &ino, NULL, 43);
  else transparency_set_image(ino, atoi(arg[1]));
}


//--------------------------------------------------------------------------//
// transparency_set_image                                                   //
//--------------------------------------------------------------------------//
void transparency_set_image(int ino, int t)
{
  z[ino].transparency = t;
  rebuild_display(ino);
  redraw(ino);
  redraw_transparent_image(ino);
}


//--------------------------------------------------------------------------//
//copybackground  Copy parts of the image in corresponding                  //
//  screen locations to the screen.                                         //
//--------------------------------------------------------------------------//
void copybackground(int x1, int y1, int x2, int y2, int skipimage)
{
    int k,L,ytest,ino;
    int ix1,ix2,iy1,iy2,startx,endx,starty,endy;

    if(x1<0) x1=0;               // Fix the parameters  
    if(x2<0) x2=0;
    if(x1>g.main_xsize-1) x1=g.main_xsize-1;
    if(x2>g.main_xsize-1) x2=g.main_xsize-1;
    if(x2<x1)swap(x1,x2);
    ytest=0;
    if(y1<ytest) y1=ytest;
    if(y2<ytest) y2=ytest;
    if(y1>g.main_ysize-16) y1=g.main_ysize-16;
    if(y2>g.main_ysize-16) y2=g.main_ysize-16;
    if(y2<y1)swap(y1,y2);
                                 // Copy portions of any image in region
                                 // in order of back to front.
                                 // Only the portion between startx,starty
                                 // and endx, endy is copied.
    for(k=g.image_count-1; k>=0; k--)
    {   
          ino = z[k].overlap;
          if((ino>=0)&&(ino!=skipimage))
          {     ix1 = 0;         // Edges of image (can be off screen)
                ix2 = z[ino].xsize - 1; 
                iy1 = 0;
                iy2 = z[ino].ysize - 1;
                starty = max(iy1, y1-z[ino].ypos);
                endy   = min(iy2, y2-z[ino].ypos);
                if(endy>starty)
                {       startx = max(ix1, x1-z[ino].xpos-1); //Start, end x pos.on image
                        endx   = min(ix2, x2-z[ino].xpos-1);
                        L = endx - startx;                 // no.of x pixels to copy 
                        if(L>0)              
                          copyimage(startx,starty,endx,endy,startx-ix1,starty-iy1,
                              PUT, z[ino].img_ximage, z[ino].win);         
                }   
          }
    }       
}



//--------------------------------------------------------------------------//
// erasefromscreen - replace image with background on screen                //
//--------------------------------------------------------------------------//
void erasefromscreen(int ino)
{   
    int x1,x2,y1,y2;
    if(ino>=0)
    { x1 = z[ino].xpos;          // Edges of image (can be off screen)
      x2 = z[ino].xpos+z[ino].xsize;
      y1 = z[ino].ypos;
      y2 = z[ino].ypos+z[ino].ysize;
      copybackground(x1,y1,x2,y2,ino);
    }  

   // Temporarily move image off edge of the screen to prevent XOR from
   // reading its pixels. 

   z[ino].xpos = g.main_xsize + 1;
   z[ino].ypos = g.main_ysize + 1;

}

//--------------------------------------------------------------------------//
// switchto - switch screen to image no. ino                                //
//--------------------------------------------------------------------------//
void switchto(int ino)
{ 
  int j,k;
  listinfo *l;
  if(!g.want_switching) return;

  if(ino>=0 && ino<g.image_count)
  {  
      if(z[ino].floating) return;
      drawselectbox(OFF);
      if(ino>0)
      {  if(g.want_raise)
         {   if(z[ino].shell) 
                XRaiseWindow(g.display,XtWindow(XtParent(XtParent(z[ino].widget))));
             else if(ino>0)
                XRaiseWindow(g.display,z[ino].win);
         }
         //// Re-sort images so ino is on top
         for(j=1; j<g.image_count; j++)
             if(z[j].overlap < z[ino].overlap) z[j].overlap++;
         z[ino].overlap = 0;
         for(j=1; j<g.image_count; j++)
             g.image_stack[z[j].overlap] = j;
      }
      if(z[ino].palette!=NULL && g.bitsperpixel<=8)
      { 
         setpalette(z[ino].palette);
         g.currentpalette = -2;
      }
      selectimage(ino);
      for(k=0;k<g.openlistcount;k++) 
      {   l = g.openlist[k];
          if(l->autoupdate && ci!=ino)
          {     ci = ino;
                update_list(k);
          }
      }
      update_sample_palette();

     // Needed to make sure dialogchangecicb actually gets the SubstructureNotify
     // event. For some reason, about 1/100 of the time the event is missed.
     
     if(g.indialog)
     {   XtMoveWidget(z[0].widget, 1, 0);
         XtMoveWidget(z[0].widget,-1, 0);
     }
     ci = ino;                                
  }
  g.imageatcursor = ci;
  g.create_xsize = z[ino].xsize;
  g.create_ysize = z[ino].ysize;
  print_image_title_at_bottom(ci);
}      

 
//--------------------------------------------------------------------------//
// selectimage - select image # ino                                         //
//--------------------------------------------------------------------------//
void selectimage(int ino)
{
  drawselectbox(OFF);
  if(!between(ino,0,g.image_count-1)) return;
  if(g.getlabelstate) return;
  g.selectedimage = ino; 
  g.selected_is_square=1;
  g.selected_ulx = z[ino].xpos;
  g.selected_lrx = z[ino].xpos + z[ino].xscreensize - 1;
  g.selected_uly = z[ino].ypos;
  g.selected_lry = z[ino].ypos + z[ino].yscreensize - 1;    
  g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
  g.uly = zoom_y_coordinate(g.selected_uly, ino);
  g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
  g.lry = zoom_y_coordinate(g.selected_lry, ino); 
}


//--------------------------------------------------------------------------//
// select_region                                                            //
//--------------------------------------------------------------------------//
void select_region(int x1, int y1, int x2, int y2)
{
  int ino,bpp;
  if(g.getlabelstate) return;
  g.selected_ulx = x1;
  g.selected_uly = y1;
  g.selected_lrx = x2;
  g.selected_lry = y2;
  g.selectedimage=-1;
  ino = whichimage(x1,y1,bpp);
  g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
  g.uly = zoom_y_coordinate(g.selected_uly, ino);
  g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
  g.lry = zoom_y_coordinate(g.selected_lry, ino); 
  g.selected_is_square=1;
  drawselectbox(ON);
}


//--------------------------------------------------------------------------//
//whichimage - determines which image x,y is on.   x,y = screen coords.     //
//  Returns:      -1 = main window (not on an image)                        //
//                 0 to 511 = image number.                                 //
//  Also returns the bits/pixel of the image in ref. variable bpp.          //
//  The optional parameter `skip' allows one image to be skipped from       //
//     consideration for reading images in XOR mode.                        //
//  Images with the 'permanent' flag set to 0 are skipped.                  //
//  x and y are screen coordinates relative to (main_xpos, main_ypos).      //
//  Repeats until it finds an opaque pixel.                                 //
//--------------------------------------------------------------------------//
int whichimg(int x, int y, int& bpp, int skip){ return which_image_number(x,y,bpp,skip,1); }
int whichimage(int x, int y, int& bpp, int skip){ return which_image_number(x,y,bpp,skip,0); }
int which_image_number(int x, int y, int& bpp, int skip, int from_screen)
{
   int k, ino, inside;
   for(k=0; k<g.image_count; k++)  
   {   ino = min(g.image_count-1, g.image_stack[k]);     // Start with foreground image
       if(z[ino].permanent==TEMP) continue;
       if(ino>=0 && ino!=skip)
       {    
            inside = 0;
            if(from_screen)
            {    if(insidebox(x, y, z[ino].xpos, z[ino].ypos, 
                     z[ino].xpos + z[ino].xscreensize - 1,
                     z[ino].ypos + z[ino].yscreensize - 1) )
                     inside = 1;
            }else
            {
                 if(insidebox(x, y, z[ino].xpos, z[ino].ypos, 
                     z[ino].xpos + z[ino].xsize - 1,
                     z[ino].ypos + z[ino].ysize - 1) )
                     inside = 1;
            }
            if(inside)
            {    
                 bpp=z[ino].bpp;
                 return(ino); 
            }   
       }
   }  

   bpp = g.bitsperpixel;
   return 0;
}



//--------------------------------------------------------------------------//
//  whichimage - if image is in shell, w could be the image's widget or its //
//  3x parent. If not in a shell, w is the image's widget.                  // 
//--------------------------------------------------------------------------//
int whichimage(Widget w, int& bpp)
{
   int k;
   for(k=0; k<g.image_count; k++)  
   {  
         if(w==z[k].widget)  // may be in shell or on main window
         {   bpp=z[k].bpp;
             return(k); 
         }
         if(z[k].shell)
         {   if(z[k].scrollbars)      // image in shell with scroll bars
             {     if(w==XtParent(XtParent(XtParent(XtParent(z[k].widget)))))
                   {  bpp=z[k].bpp;
                      return(k); 
                   }
             }
             else                    // image in shell, no scroll bars
             {     if(w==XtParent(XtParent(XtParent(z[k].widget))))
                   {  bpp=z[k].bpp;
                      return(k); 
                   }
             }
         }
   }  
   bpp=g.bitsperpixel;
   return 0;
}
int whichimage(Window w, int& bpp)
{
   int k;
   for(k=0; k<g.image_count; k++)  
   {     if(w==z[k].win)
         {  bpp=z[k].bpp;
            return(k); 
         }
   }  
   bpp=g.bitsperpixel;
   return -1;
}



//--------------------------------------------------------------------------// 
// whatbpp - Returns the bits/pixel of whatever image is at x,y.            //
//--------------------------------------------------------------------------//
int whatbpp(int x, int y)
{
   int bpp;
   whichimage(x,y,bpp);
   return(bpp);
}


//--------------------------------------------------------------------------//
// image_number - obtain image no. from title                               //
// Strip off the "" if given.                                               //
//--------------------------------------------------------------------------//
int image_number(char *s)
{
   int total=0,answer=0,k,start=0,end;
   char tempstring[128];
   char tempstring2[FILENAMELENGTH];
   if(s[0]=='\"') start=1;
   if(start) 
   {    end=start;
        for(k=start; k<=(int)strlen(s); k++) if(s[k]=='\"') end=k;
   }else  end=strlen(s);
   strcpy(tempstring, s+start);
   tempstring[end]=0;
   for(k=0; k<g.image_count; k++)  
   {   if(!strcmp(basefilename(z[k].name), tempstring)) 
       {   total++;
           answer = k;
       }
   }

   //// If it's not a title, maybe the string was the image number.
   if(!total)
   {   answer = atoi(tempstring);
       if(!between(answer, 0, g.image_count-1))
       {     message("Invalid image name",ERROR); 
             g.getout=1; 
             answer = 0; 
       }
   }
   if(total>1) 
   {   sprintf(tempstring2, "%d images had same title\n(%s)\nClosing last one only", total, s);
       message(tempstring2, WARNING);
   }        
   return answer;
}


//--------------------------------------------------------------------------//
// double_image_number - obtain image no. as double for calculator.y        //
//--------------------------------------------------------------------------//
double double_image_number(char *s)
{
   double ii = (double)image_number(s);
   return ii;
}


//--------------------------------------------------------------------------//
// insidebox - returns 1 if x,y is inside box x1,y1,x2,y2                   //
//--------------------------------------------------------------------------//
int insidebox(int x, int y, int x1, int y1, int x2, int y2)
{
   if((x>=x1)&&(x<=x2)&&(y>=y1)&&(y<=y2)) return (1);
   else return(0);
}

//--------------------------------------------------------------------------//
// boxoverlap - returns 1 if 2 boxes overlap at any point                   //
//--------------------------------------------------------------------------//
int boxoverlap(int x1,int y1,int x2,int y2,int x3,int y3,int x4,int y4)
{  int y;
   for(y=y1;y<=y2;y++)
   {  if(between(y,y3,y4) && 
       ((between(x4,x1,x2)||between(x3,x1,x2))||
        (between(x1,x3,x4)||between(x2,x3,x4))))
      return(1);
   }
   return(0);
}


//------------------------------------------------------------------------//
// itoa                                                                   //
// Convert integer to string.                                             //
//------------------------------------------------------------------------//
char *itoa(int value, char *str, int radix)
{
   switch(radix)
   {  
#ifdef MSDOS
      case 2:  sprintf(str, "%b",value); break;
#endif
      case 8:  sprintf(str, "%o",value); break; 
      case 10: sprintf(str, "%d",value); break; 
      case 16: sprintf(str, "%x",value); break; 
      default: perror("Error converting string"); exit(1);
   } 
   return(str);
}


//------------------------------------------------------------------------//
// ltoa                                                                   //
// Convert long integer to string.                                        //
//------------------------------------------------------------------------//
char *ltoa(long int value, char *str, int radix)
{
   return itoa((int)value, str, radix); 
}


//-----------------------------------------------------------------------//
//  doubletostring                                                       //
//  convert double to a string with signif significant digits.           //
//  no longer needed since linux now has a gcvt.                         //
//  In OSX, 'configure' incorrectly says fcvt is present.                //
//-----------------------------------------------------------------------//
char *doubletostring(double val, int signif, char *buf)
{  
   buf=buf; val=val;signif=signif;
   char gtemp[128];
#ifdef OSX
   sprintf(buf, "%f", val);
   strcpy(gtemp, buf);
   return buf;
#else
#ifndef HAVE_FCVT
   sprintf(buf, "%f", val);
   strcpy(gtemp, buf);
   return buf;
#else
#ifdef HAVE_ISINF
   if(isinf(val)){ strcpy(buf,"Infinity");  return buf; }
#endif
#ifdef HAVE_ISNAN
   if(isnan(val)){ strcpy(buf,"Not a number"); return buf; }
#endif
   char *bufr;
   const int plussign=0;
   int b=0,j,k=0,position,sign,dotpos;

   bufr = (char*)fcvt(val,signif,&position,&sign);
   if(sign){ gtemp[0]='-'; k++; } 
   else if(plussign) {  gtemp[0]='+'; k++; }

   if(position>0)     // number is >= 1
   {   for(j=1;j<=position;j++,b++){ gtemp[k]=bufr[b]; k++; }
       gtemp[k]='.';
       k++;
   }    
   else               // number is < 1
   {  gtemp[k]='0';   // leading zero
      k++;
      gtemp[k]='.';
      k++;
      for(j=1;j<=-position;j++){ gtemp[k]='0'; k++; }
   }
   for(j=1;j<=signif;j++,b++){ gtemp[k]=bufr[b]; k++; }
  
   gtemp[k]=0;
         // double check to make sure excess zeros are not added
   dotpos = strlen(gtemp);
   for(k=0;k<(int)strlen(gtemp);k++) if(gtemp[k]=='.'){ dotpos=k;break; }
   if(signif>0)
     gtemp[min(128-1,dotpos+signif+1)]=0;
   else
     gtemp[min(128-1,dotpos+signif)]=0;
   strcpy(buf,gtemp);
   return buf;
#endif
#endif 
}


//-----------------------------------------------------------------------//
// maxgamma - find largest value in gamma table for an image             //
//-----------------------------------------------------------------------//
int maxgamma(int ino)
{
    static int oldino=0,oldresult=0;
    int k, highest=0;
    if(ino==oldino) return(oldresult);
    if(ino>=0)
    { 
      for(k=0;k<=255;k++) if(z[ino].gamma[k]>z[ino].gamma[highest]) highest=k;
      oldresult=z[ino].gamma[highest];
      oldino=ino;
      return(oldresult);
    }
    oldresult = (uint)g.maxvalue[g.bitsperpixel];
    return(oldresult);    
}


//--------------------------------------------------------------------------//
// intensity                                                                //
// Calculates the brightness of a pixel                                     //
//--------------------------------------------------------------------------//
int intensity(int pixelvalue, int bpp)
{
     int red, green, blue;
     valuetoRGB(pixelvalue, red, green, blue, bpp);
     return (int)(red*g.luminr + green*g.luming + blue*g.luminb);
}


//--------------------------------------------------------------------------//
// cls - Clears the screen.                                                 //  
//--------------------------------------------------------------------------//
void cls(uint color)
{
   blackbox(1,0,g.main_xsize-2,g.main_ysize-2,color);
}


//--------------------------------------------------------------------------//
// samplecolor - print sample of color in clickbox                          //
//--------------------------------------------------------------------------//
void samplecolor(int c)
{
  c=c;  // need more than this
}
void samplecolor(int c[10])
{
  c[0]=c[0];  // need more than this
}


//--------------------------------------------------------------------------//
// between    - returns 1 if a is between b and c                           //
//--------------------------------------------------------------------------//
int between(int a,int b,int c)
{   if((a>=b)&&(a<=c))return(1);
    else return(0);
}

//--------------------------------------------------------------------------//
// between    - returns 1 if a is between b and c                           //
//--------------------------------------------------------------------------//
int between(double a, double b, double c)
{   if((a>=b)&&(a<=c))return(1);
    else return(0);
}

//--------------------------------------------------------------------------//
// fbetween   - returns 1 if a is between b and c                           //
//--------------------------------------------------------------------------//
double fbetween(double a, double b, double c)
{   if((a>=b)&&(a<=c))return(1.0);
    else return(0.0);
}


//--------------------------------------------------------------------------//
// onedge - returns 1 if x,y is on edge of box x1,y1,x2,y2                  //
//--------------------------------------------------------------------------//
int onedge(int x, int y, int x1, int y1, int x2, int y2)
{
  const int w1 = 0;                     // width of handle outside box
  const int w2 = g.window_handle_size;  // width of handle inside box
  if( between(y, y1-w1, y2+w1))
  {  if( between(x, x1-w1, x1+w2)) return(1);
     if( between(x, x2-w2, x2+w1)) return(1);
  }
  if( between(x, x1-w1, x2+w1))
  {  if( between(y, y1-w1, y1+w2)) return(1);
     if( between(y, y2-w2, y2+w1)) return(1);
  }   
  return(0);
}


//--------------------------------------------------------------------------//
// stringtohex                                                              //
//  converts a string to integer assuming the string is a hex value.        //
//--------------------------------------------------------------------------//
int stringtohex(char* string)
{
     int k;
     char c;
     int answer = 0;
     for(k=0;k<(int)strlen(string);k++)
     {   answer <<= 4;
         c = string[k];
         if((c>='0')&&(c<='9')) answer += c-'0'; 
         if((c>='A')&&(c<='F')) answer += c-'A'+10; 
         if((c>='a')&&(c<='f')) answer += c-'a'+10; 
     }    
     return answer;
}


//--------------------------------------------------------------------------//
// savesettings                                                             //
//--------------------------------------------------------------------------//
void savesettings(void)
{
  FILE* fp;
  int k,rr,gg,bb;
  char *ifile;
  ifile = new char[FILENAMELENGTH];
  sprintf(ifile,"%s/tnimage.ini",g.homedir);

  setSVGApalette(-4, 0);     // Reset background palette
  valuetoRGB(g.fcolor,rr,gg,bb,g.bitsperpixel);
  g.fc.red=rr; g.fc.green=gg; g.fc.blue=bb;
  valuetoRGB(g.bcolor,rr,gg,bb,g.bitsperpixel);
  g.bc.red=rr; g.bc.green=gg; g.bc.blue=bb;

  if((fp=fopen(ifile, "w"))!=NULL )
  { 
       fprintf(fp,"version %s\n",g.version);
       fprintf(fp,"area_selection_mode %d\n",g.area_selection_mode);
       fprintf(fp,"auto_undo %d\n",g.autoundo);
       fprintf(fp,"background_color %d\n",g.bcolor);
       fprintf(fp,"background_image_color %d\n",g.bkg_image_color);
       fprintf(fp,"background_palette %d\n",g.bkgpalette);
       fprintf(fp,"background_red %d\n",g.bc.red);
       fprintf(fp,"background_green %d\n",g.bc.green);
       fprintf(fp,"background_blue %d\n",g.bc.blue);
       fprintf(fp,"camera %s\n",g.camera);
       fprintf(fp,"camera_id %d\n",g.camera_id);
       fprintf(fp,"copy %d\n",g.copy);
       fprintf(fp,"compression %s\n",g.compression);
       fprintf(fp,"compression_ext %s\n",g.compression_ext);
       fprintf(fp,"csize %d\n",g.csize);
       fprintf(fp,"decimal_point %c\n",g.decimal_point);
       fprintf(fp,"decompression %s\n",g.decompression);

       fprintf(fp,"default_font %s\n",g.font);
       fprintf(fp,"double_click_msec %d\n",g.double_click_msec);
       fprintf(fp,"floating_magnifier_on_root %d\n",g.floating_magnifier_on_root);
       fprintf(fp,"font_selection %d\n",g.font_selection);
       fprintf(fp,"image_font %s\n",g.imagefont);
       fprintf(fp,"information_width %d\n",g.drawarea2width);
       fprintf(fp,"foreground_color %d\n",g.fcolor);
       fprintf(fp,"foreground_red %d\n",g.fc.red);
       fprintf(fp,"foreground_green %d\n",g.fc.green);
       fprintf(fp,"foreground_blue %d\n",g.fc.blue);
       fprintf(fp,"format_path %s\n",g.formatpath);
       fprintf(fp,"help_directory %s\n",g.helpdir);
       fprintf(fp,"jpeg_q_factor %d\n",g.jpeg_qfac);
       fprintf(fp,"luminosity_red %g\n",g.luminr);
       fprintf(fp,"luminosity_green %g\n",g.luming);
       fprintf(fp,"luminosity_blue %g\n",g.luminb);
       fprintf(fp,"object_threshold_red %d\n",g.object_threshold.red);
       fprintf(fp,"object_threshold_green %d\n",g.object_threshold.green);
       fprintf(fp,"object_threshold_blue %d\n",g.object_threshold.blue);

       fprintf(fp,"print_paper_xsize %g\n",printer.paperxsize);
       fprintf(fp,"print_paper_ysize %g\n",printer.paperysize);
       fprintf(fp,"print_blue_factor %g\n",printer.bfac); 
       fprintf(fp,"print_depletion %d\n",printer.depletion);
       fprintf(fp,"print_dither %d\n",printer.dither);
       fprintf(fp,"print_graybalance %d\n",printer.graybalance);
       fprintf(fp,"print_green_factor %g\n",printer.gfac);
       fprintf(fp,"print_intensity_factor %g\n",printer.ifac);
       fprintf(fp,"print_palette %d\n",printer.palette);
       fprintf(fp,"print_paper %d\n",printer.papertype);
       fprintf(fp,"print_language %d\n",printer.language);
       fprintf(fp,"print_positive %d\n",printer.positive);
       fprintf(fp,"print_quality %d\n",printer.quality);
       fprintf(fp,"print_red_factor %g\n",printer.rfac);
       fprintf(fp,"print_resolution %d\n",printer.res);
       fprintf(fp,"print_device %d\n",printer.device);
       fprintf(fp,"print_vertical %d\n",printer.vertical);
       fprintf(fp,"print_hsize %g\n",printer.hsize);
       fprintf(fp,"print_ratio %g\n",printer.ratio);
       fprintf(fp,"print_rotation %d\n",printer.rotation);
       fprintf(fp,"print_interpolate %d\n",printer.interpolate);
       fprintf(fp,"print_command %s\n",printer.command);
       fprintf(fp,"print_devicename %s\n",printer.devicename);
       fprintf(fp,"print_hpos %g\n",printer.hpos);
       fprintf(fp,"print_vpos %g\n",printer.vpos);

       fprintf(fp,"read_cmyk %d\n",g.read_cmyk);
       fprintf(fp,"save_cmyk %d\n",g.save_cmyk);
       fprintf(fp,"scanner %s\n",g.scan_device);
       fprintf(fp,"scannerid %d\n",g.scannerid);
       fprintf(fp,"signif_digits %d\n",g.signif);
       fprintf(fp,"spacing %d\n",g.spacing);
       fprintf(fp,"split_frame_cols %d\n",g.create_cols);
       fprintf(fp,"text_spacing %d\n",g.text_spacing);
       fprintf(fp,"total_xsize %d\n",g.total_xsize);
       fprintf(fp,"total_ysize %d\n",g.total_ysize);
       fprintf(fp,"use_custom_header %d\n",g.useheader);
       fprintf(fp,"want_colormaps %d\n",g.want_colormaps);
       fprintf(fp,"want_crosshairs %d\n",g.want_crosshairs);
       fprintf(fp,"want_messages %d\n",g.want_messages);
       fprintf(fp,"want_quantize %d\n",g.want_quantize);
       fprintf(fp,"want_raise %d\n",g.want_raise);
       fprintf(fp,"want_shell %d\n",g.want_shell);
       fprintf(fp,"want_slew %d\n",g.want_slew);
       fprintf(fp,"want_byte_order %d\n",g.want_byte_order);
       fprintf(fp,"want_title %d\n",g.want_title);
       fprintf(fp,"wavelet_path %s\n",g.waveletpath);
       fprintf(fp,"window_border %d\n",g.window_border);
       fprintf(fp,"window_handle_size %d\n",g.window_handle_size);
       fprintf(fp,"xlib_cursor %d\n",g.xlib_cursor);
       fprintf(fp,"zoom_factor %g\n",g.zoomfac);
       fprintf(fp,"nbuttons %d\n",g.nbuttons);
       for(k=0;k<g.nbuttons;k++)
          fprintf(fp,"button %s %s:%s\n",
                  g.button[k]->label, g.button[k]->activate_cmd,
                  g.button[k]->deactivate_cmd);
       fclose(fp);

  }else
  {    fprintf(stderr,"Can't save settings %s\n",ifile); //CHAPG
       error_message(ifile, errno);
  }
  
  //// Make sure user can still read ini file even if tnimage was run as root.
  chmod(ifile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  delete[] ifile;
}


//--------------------------------------------------------------------------//
// readsettings                                                             //
//--------------------------------------------------------------------------//
void readsettings(int rereading)
{  
  if(g.diagnose){ printf("Reading tnimage.ini...");fflush(stdout); }
  FILE* fp;
  int gotconfig=1, status=0;
  int space=0, ret=0, but=0,k, length;
  char name[FILENAMELENGTH]="";
  char value[FILENAMELENGTH]="";
  char initfile[FILENAMELENGTH]="";
  char initversion[128]="";
  char tempstring[FILENAMELENGTH]="";
  int colon=0;

  sprintf(initfile,"%s/tnimage.ini",g.homedir);
  if((fp=fopen(initfile,"rb"))==NULL ) gotconfig=0;
  else fclose(fp);

  initversion[0]=0;
  if(gotconfig)

  {  fp=fopen(initfile,"r");
     while(!feof(fp))
     { 
       fgets(name, 80, fp);
       if(feof(fp))break;
       space = strcspn(name," ");                // Position of 1st space

       // Can't have this in intel 64 bit  
       //ret = (int)index(name, '\n') - (int)name; // Position of final \n
       ret = strlen(name);
       for(k=0; k<=(int)strlen(name); k++) if(name[k]=='\n') ret = k; // Position of final \n

       name[space]=0;
       name[ret]=0;
       strcpy(value, 1+name+space);
       if(strcmp(name,"version")==SAME) strcpy(initversion,value); 
       if(strcmp(initversion, g.version)!=SAME)
       {   fprintf(stderr,"Version mismatch in tnimage.ini - ignoring old settings\n");
           gotconfig=0;
           break;
       }

       ///-----parameters-----///
       if(strcmp(name,"area_selection_mode")==SAME) g.area_selection_mode=atoi(value);
       if(strcmp(name,"background_palette")==SAME) g.bkgpalette=atoi(value);
       if(g.bkgpalette<=0) g.bkgpalette=1;
       if(strcmp(name,"information_width")==SAME) g.drawarea2width=atoi(value);
       if(strcmp(name,"total_xsize")==SAME)
       {   g.total_xsize = atoi(value);
          // g.total_xsize = g.total_xsize + g.drawarea2width;   
           g.main_xsize = g.total_xsize - g.drawarea2width;
       }
       if(strcmp(name,"total_ysize")==SAME) 
       {   g.total_ysize = atoi(value);
            g.main_ysize = g.total_ysize - (int)g.mainmenuheight;
       }

       if(strcmp(name,"button")==SAME && !rereading) 
       {   if(but>=g.nbuttons)
               fprintf(stderr,"tnimage.ini: Error: button count mismatch, ignoring user button\n");
           else
           {   
               space = strcspn(value," ");           // Position of 1st space
               colon = strcspn(value,":");           // Position of 1st colon
               strncpy(g.button[but]->label, value, space);
               g.button[but]->label[space]=0;
               strncpy(g.button[but]->activate_cmd, value+space+1, colon-space-1);
               g.button[but]->activate_cmd[colon-space-1]=0;
               if(colon<(int)strlen(value))
                  strncpy(g.button[but]->deactivate_cmd, value+colon+1, COMMAND_LENGTH);
               else
                  strcpy(g.button[but]->deactivate_cmd, "null");
               but++;
           }
       }

       if(strcmp(name,"camera")==SAME) strcpy(g.camera, value);
       if(strcmp(name,"camera_id")==SAME) g.camera_id=atoi(value);
       if(strcmp(name,"compression")==SAME) strcpy(g.compression, value);
       if(strcmp(name,"csize")==SAME) g.csize=atoi(value);
       if(strcmp(name,"decompression")==SAME) strcpy(g.decompression, value);
       if(strcmp(name,"compression_ext")==SAME) strcpy(g.compression_ext, value);
       if(strcmp(name,"decimal_point")==SAME) g.decimal_point = value[0];
       if(strcmp(name,"default_font")==SAME) strcpy(g.font, value);
       if(strcmp(name,"image_font")==SAME) strcpy(g.imagefont, value);
       if(strcmp(name,"foreground_color")==SAME) g.fcolor=atoi(value);
       if(strcmp(name,"background_color")==SAME) g.bcolor=atoi(value);
       if(strcmp(name,"background_image_color")==SAME) g.bkg_image_color=atoi(value);
       if(strcmp(name,"double_click_msec")==SAME) g.double_click_msec=atoi(value);
       if(strcmp(name,"font_selection")==SAME) g.font_selection=atoi(value);
       if(strcmp(name,"foreground_red")==SAME) g.fc.red=atoi(value);
       if(strcmp(name,"foreground_green")==SAME) g.fc.green=atoi(value);
       if(strcmp(name,"foreground_blue")==SAME) g.fc.blue=atoi(value);
       if(strcmp(name,"background_red")==SAME) g.bc.red=atoi(value);
       if(strcmp(name,"background_green")==SAME) g.bc.green=atoi(value);
       if(strcmp(name,"background_blue")==SAME) g.bc.blue=atoi(value);
       if(strcmp(name,"format_path")==SAME)
       {   strcpy(g.formatpath, value);
           length = strlen(g.formatpath);
           if(length && g.formatpath[length] =='/') g.formatpath[length]=0;
       }
       if(strcmp(name,"help_directory")==SAME) strcpy(g.helpdir, value);
       if(strcmp(name,"luminosity_red")==SAME) g.luminr=atof(value);
       if(strcmp(name,"luminosity_green")==SAME) g.luming=atof(value);
       if(strcmp(name,"luminosity_blue")==SAME) g.luminb=atof(value);
       if(strcmp(name,"copy")==SAME) g.copy=atoi(value);
       if(strcmp(name,"floating_magnifier_on_root")==SAME) g.floating_magnifier_on_root=atoi(value);
       if(strcmp(name,"nbuttons")==SAME && !rereading) 
       {   g.nbuttons = max(0,atoi(value));
           g.button = new ButtonStruct*[g.nbuttons];
           for(k=0;k<g.nbuttons;k++)
           {   g.button[k] = new ButtonStruct;
               g.button[k]->button = 0;
               g.button[k]->state  = OFF;
               g.button[k]->widget = 0;
               g.button[k]->label  = new char[LABEL_LENGTH];
               g.button[k]->activate_cmd = new char[COMMAND_LENGTH];
               g.button[k]->deactivate_cmd = new char[COMMAND_LENGTH];
           }
       }
       if(strcmp(name,"nosparse")==SAME) g.nosparse=atoi(value);
       if(strcmp(name,"object_threshold_red")==SAME) g.object_threshold.red=atoi(value);
       if(strcmp(name,"object_threshold_green")==SAME) g.object_threshold.green=atoi(value);
       if(strcmp(name,"object_threshold_blue")==SAME) g.object_threshold.blue=atoi(value);

       if(strcmp(name,"print_quality")==SAME)   printer.quality=atoi(value);
       if(strcmp(name,"print_paper_xsize")==SAME)   printer.paperxsize=atof(value);
       if(strcmp(name,"print_paper_ysize")==SAME)   printer.paperysize=atof(value);
       if(strcmp(name,"print_device")==SAME)    printer.device=atoi(value);
       if(strcmp(name,"print_dither")==SAME)    printer.dither=atoi(value);
       if(strcmp(name,"print_resolution")==SAME)printer.res=atoi(value);
       if(strcmp(name,"print_vertical")==SAME)  printer.vertical=atoi(value);
       if(strcmp(name,"print_positive")==SAME)  printer.positive=atoi(value);
       if(strcmp(name,"print_palette")==SAME)   printer.palette=atoi(value);
       if(strcmp(name,"print_language")==SAME)  printer.language=atoi(value);
       if(strcmp(name,"print_intensity_factor")==SAME) printer.ifac=atof(value);
       if(strcmp(name,"print_red_factor")==SAME) printer.rfac=atof(value);
       if(strcmp(name,"print_green_factor")==SAME) printer.gfac=atof(value);
       if(strcmp(name,"print_blue_factor")==SAME) printer.bfac=atof(value);
       if(strcmp(name,"print_paper")==SAME) printer.papertype=atoi(value);
       if(strcmp(name,"print_hsize")==SAME) printer.hsize=atof(value);
       if(strcmp(name,"print_ratio")==SAME) printer.ratio=atof(value);
       if(strcmp(name,"print_rotation")==SAME) printer.rotation=atoi(value);
       if(strcmp(name,"print_interpolate")==SAME) printer.interpolate=atoi(value);
       if(strcmp(name,"print_hpos")==SAME) printer.hpos=atof(value);
       if(strcmp(name,"print_vpos")==SAME) printer.vpos=atof(value);

       if(strcmp(name,"read_cmyk")==SAME) g.read_cmyk=atoi(value);
       if(strcmp(name,"auto_undo")==SAME) g.autoundo=atoi(value);
       if(strcmp(name,"use_custom_header")==SAME) g.useheader=atoi(value);
       if(strcmp(name,"save_cmyk")==SAME) g.save_cmyk=atoi(value);
       if(strcmp(name,"signif_digits")==SAME) g.signif=min(8,atoi(value));
       if(strcmp(name,"sparse")==SAME) g.sparse=atoi(value);
       if(strcmp(name,"print_graybalance")==SAME) printer.graybalance=atoi(value);
       if(strcmp(name,"print_depletion")==SAME) printer.depletion=atoi(value);
       if(strcmp(name,"jpeg_q_factor")==SAME) g.jpeg_qfac=atoi(value);
       if(strcmp(name,"print_command")==SAME) strcpy(printer.command,value);
       if(strcmp(name,"print_devicename")==SAME) strcpy(printer.devicename,value);
       if(strcmp(name,"scanner")==SAME) strcpy(g.scan_device,value);
       if(strcmp(name,"scannerid")==SAME) g.scannerid=atoi(value);
       if(strcmp(name,"spacing")==SAME) g.spacing=atoi(value);
       if(strcmp(name,"split_frame_cols")==SAME) g.create_cols=atoi(value);
       if(strcmp(name,"text_spacing")==SAME) g.text_spacing=atoi(value);
       if(strcmp(name,"want_colormaps")==SAME)
       {      g.want_colormaps=atoi(value);
              if(g.want_colormaps==2) g.want_colormaps=0; // < 32 colors ast time
       }
       if(strcmp(name,"want_crosshairs")==SAME) g.want_crosshairs=atoi(value);
       if(strcmp(name,"want_title")==SAME) g.want_title=atoi(value);
       if(strcmp(name,"want_messages")==SAME) g.want_messages=atoi(value);
       if(strcmp(name,"want_quantize")==SAME) g.want_quantize=atoi(value);
       if(strcmp(name,"want_slew")==SAME) g.want_slew=atoi(value);
       if(strcmp(name,"want_raise")==SAME) g.want_raise=atoi(value);
       if(strcmp(name,"want_shell")==SAME) g.want_shell=atoi(value);
       if(strcmp(name,"want_byte_order")==SAME) g.want_byte_order=atoi(value);
       if(strcmp(name,"wavelet_path")==SAME)
       {   strcpy(g.waveletpath, value);
           length = strlen(g.waveletpath);
           if(length && g.waveletpath[length] =='/') g.waveletpath[length]=0;
       }
       if(strcmp(name,"window_border")==SAME) g.window_border=atoi(value);
       if(strcmp(name,"window_handle_size")==SAME) g.window_handle_size=atoi(value);
       if(strcmp(name,"xlib_cursor")==SAME) g.xlib_cursor=atoi(value);
       if(strcmp(name,"zoom_factor")==SAME) g.zoomfac=atof(value);
     }
     fclose(fp);
     
  }
  if(!gotconfig && !rereading)
  {
       if(g.diagnose) printf("access homedir %d\n",access(g.homedir,F_OK));
       if(access(g.homedir,F_OK)) 
       {    printf("\nCreating directory %s\n", g.homedir);        
            status = mkdir(g.homedir, 0755);
            if(status) printf("Can't create directory:\n%s",g.homedir);

            sprintf(tempstring,"%s/formats",g.homedir);
            status = mkdir(tempstring, 0755);
            if(status) printf("Can't create directory:\n%s",tempstring);

            sprintf(tempstring,"%s/notes",g.homedir);
            status = mkdir(tempstring, 0755);
            if(status) printf("Can't create directory:\n%s",tempstring);
       }
       if(g.diagnose) printf("status %d\n",status);       
       if(g.diagnose){ printf("%s/tnimage.ini not found, setting default buttons\n",
             g.homedir);fflush(stdout);}
#ifdef DIGITAL
#define NBUTTONS 20
#else
       const int NBUTTONS=20;
#endif
       char buttonname[NBUTTONS][20] ={ 
                                        "Cancel",           "Slew",
                                        "Open",             "Close",
                                        "New",              "Save", 
                                        "Sketch",           "Line", 
                                        "Move",             "Erase", 
                                        "Scan",             "Print", 
                                        "Labels",           "Zoom",
                                        "Fcolor",           "Bcolor",
                                        "Prop",             "Config",
                                        "Undo",             "Quit"
                                      };
       char activatecmd[NBUTTONS][20]  ={ 
                                        "cancel",           "slew",
                                        "open",             "close",
                                        "new",              "save", 
                                        "sketch",           "line", 
                                        "move",             "erase", 
                                        "scan",             "print", 
                                        "label",            "zoom",
                                        "foregroundcolor",  "backgroundcolor",
                                        "attributes",       "configure",
                                        "undo",             "quit", 
                                      };
       char deactivatecmd[NBUTTONS][20]  ={ 
                                        "null",             "slew",
                                        "null",             "null",
                                        "null",             "null", 
                                        "cancel",           "cancel", 
                                        "move",             "null", 
                                        "null",             "null", 
                                        "null",             "null",
                                        "null",             "null",
                                        "null",             "null",
                                        "null",             "null"
                                      };
       g.nbuttons = NBUTTONS;
       g.button = new ButtonStruct*[NBUTTONS];
       for(k=0;k<NBUTTONS;k++) 
       {   g.button[k] = new ButtonStruct;
           g.button[k]->button = 0;
           g.button[k]->state  = OFF;
           g.button[k]->widget = 0;
           g.button[k]->label = new char[LABEL_LENGTH];
           g.button[k]->activate_cmd = new char[COMMAND_LENGTH];
           g.button[k]->deactivate_cmd = new char[COMMAND_LENGTH];
           strcpy(g.button[k]->label, buttonname[k]);
           strcpy(g.button[k]->activate_cmd, activatecmd[k]);
           strcpy(g.button[k]->deactivate_cmd, deactivatecmd[k]);
       }
  }
  g.currentpalette=g.bkgpalette;
  if(g.diagnose)
  {    printf("done\n nbuttons=%d\n",g.nbuttons);
       for(k=0;k<g.nbuttons;k++)
           printf(" button %d %s %s\n",k,g.button[k]->label,
               g.button[k]->activate_cmd);
       print_configuration();
  }
}        



//--------------------------------------------------------------------------//
// basefilename                                                             //
// strip path from filename & returns file name w/o path                    //
//--------------------------------------------------------------------------//
char* basefilename(char* pathname)
{  int k;
   int slashpos=0;
   static char temp[FILENAMELENGTH]="";  // must be static so name can be used
   if(strlen(pathname)) strcpy(temp,pathname);

   for(k=0;k<(int)strlen(temp);k++)          // find last slash or colon
       if((temp[k]=='/')||(temp[k]=='\\')||(temp[k]==':')) slashpos=k+1;
   return(temp+slashpos);
}


//--------------------------------------------------------------------------//
// strip_relative_path                                                      //
// strip path from filename only if it is a relative path                   //
//--------------------------------------------------------------------------//
char* strip_relative_path(char* pathname)
{  int k;
   int slashpos=0;
   char temp[FILENAMELENGTH]="";
   if(strlen(pathname)) strcpy(temp,pathname);

   if(temp[0]=='.' && (temp[1]=='/') || (temp[1]=='.' && temp[2]=='/'))
   {   for(k=0;k<(int)strlen(temp);k++)          // find last slash or colon
           if((temp[k]=='/')||(temp[k]=='\\')||(temp[k]==':')) slashpos=k+1;
   }
   return(temp+slashpos);
}



//--------------------------------------------------------------------------//
// print_configuration                                                      //
//--------------------------------------------------------------------------//
void print_configuration(void)
{   int k;
    printf("Configuration\n"); 
    printf(" cursor       %d\n",g.xlib_cursor); 
    printf(" quantize     %d\n",g.want_quantize); 
    printf(" autoundo     %d\n",g.autoundo); 
    if(g.bitsperpixel==8) 
    {   printf(" reserved colors\n"); 
        for(k=0;k<256;k++)if(g.reserved[k])printf("%d ",k); 
        printf("\n");
    }
    printf(" luminosity   %g %g %g\n",g.luminr,g.luming,g.luminb); 
    printf(" colormaps    %d\n",g.want_colormaps); 
    printf(" signif       %d\n",g.signif); 
    printf(" csize        %d\n",g.csize); 
    printf(" sprayfac     %d\n",g.sprayfac); 
    printf(" total xsize  %d\n",g.total_xsize);
    printf(" total ysize  %d\n",g.total_ysize);
    printf(" main xsize   %d\n",g.main_xsize);
    printf(" main ysize   %d\n",g.main_ysize);
    printf(" xlib cursor  %d\n",g.xlib_cursor);
    printf(" fcolor       %d\n",g.fcolor);
    printf(" bcolor       %d\n",g.bcolor);
    printf(" default font %s\n",g.font);
    printf(" image font   %s\n",g.imagefont);
    printf(" infor. width %d\n",g.drawarea2width);
    printf(" mainmenuheight %d\n",(int)g.mainmenuheight);
    printf(" slew         %d\n",g.want_slew);
    printf(" raise        %d\n",g.want_raise);
    printf(" shell        %d\n",g.want_shell);
    printf(" colortype    %d\n",g.colortype);
    printf(" draw_figure  %d\n",g.draw_figure);
    printf(" window_border %d\n",g.window_border);
    printf(" window_handle size %d\n",g.window_handle_size);
    printf(" home dir     %s\n",g.homedir);
    printf(" start dir    %s\n",g.startdir);
    printf(" current dir  %s\n",g.currentdir);
    printf(" help dir     %s\n",g.helpdir);
    fflush(stdout);
}


//--------------------------------------------------------------------------//
// print_image_info                                                         //
//--------------------------------------------------------------------------//
void print_image_info(void)
{   int k;
    printf("Image information\n"); 
    for(k=0;k<g.image_count;k++)
    {  
       printf("ino %d x=%d y=%d bpp=%d\n",k,z[k].xsize,z[k].ysize,z[k].bpp);
       printf("   xpos  %d ypos %d frames %d %d\n",z[k].xpos,z[k].ypos,z[k].frames,z[k].cf);
       printf("   transparency %d chromakey %d %d %d %d %d %d\n",
          z[k].transparency,
          z[k].chromakey, 
          z[k].ck_graymin,
          z[k].ck_graymax,
          z[k].ck_min.red,
          z[k].ck_min.green, 
          z[k].ck_min.blue);
       printf("   xpos  %d ypos %d \n",z[k].xpos,z[k].ypos);
       printf("   backed up %d \n",z[k].backedup);
       printf("   overlap   %d \n",z[k].overlap);
       printf("   colortype %d \n",z[k].colortype);
       printf("   fft %d %d %g %g %d %d %d\n",
          z[k].floatexists,
          z[k].fftdisplay,
          z[k].fmin,
          z[k].fmax,
          z[k].fftxsize,
          z[k].fftysize,
          z[k].fftstate);
       printf("   shell %d %d %d \n",z[k].shell,z[k].permanent,z[k].window_border);
    }
    printf("Other information\n"); 
    if(memorylessthan(16384)) printf("***Memory is less than 16384***\n");
    else if(memorylessthan(100000)) printf("***Memory less than 100000***\n");
    else printf("Memory is okay\n");
    printf("busy %d block %d waiting %d inmenu %d\n",g.busy, g.block,g.waiting, g.inmenu);
    if(in_printimage) printf("still in printimage\n");
    if(in_save) printf("still in save\n");
    if(in_read) printf("still in read\n");
    if(in_calibrate) printf("still in calibrate\n");
    if(in_warp) printf("still in warp\n");
    if(in_pattern_recognition) printf("still in pattern_recognition\n");
    if(in_spot_dens_dialog) printf("still in spot_dens_dialog\n");
    if(in_acquire) printf("still in acquire\n");
    if(in_strip_dens_dialog) printf("still in strip_dens_dialog\n");

}    



//--------------------------------------------------------------------------//
// is_opaque                                                                //
//--------------------------------------------------------------------------//
int is_opaque(int v, int tmin, int tmax, int chromakey)
{
      switch(chromakey)
      {    case CHROMAKEY_NONE:   return 1;
           case CHROMAKEY_EQUAL:  if(abs(v-tmin)>5) return 1;
                                  else return 0;
           case CHROMAKEY_NOTEQUAL:if(abs(v-tmin)<5) return 1;
                                  else return 0;
           case CHROMAKEY_NORMAL: if(between(v,tmin,tmax)) return 1;
                                  else return 0;
           case CHROMAKEY_INVERT: if(!between(v,tmin,tmax)) return 1;
                                  else return 0;
           default: return 1;
      }
}
int is_opaque(int rr, int rmin, int rmax,
              int gg, int gmin, int gmax,
              int bb, int bmin, int bmax, int chromakey)
{
      switch(chromakey)
      {    case CHROMAKEY_NONE:  return 1;
           case CHROMAKEY_EQUAL: if(abs(rr-rmin)>5 ||
                                    abs(gg-gmin)>5 ||
                                    abs(bb-bmin)>5) return 1;
                                 else return 0;
           case CHROMAKEY_NOTEQUAL: if(abs(rr-rmin)<5 ||
                                    abs(gg-gmin)<5 ||
                                    abs(bb-bmin)<5) return 1;
                                 else return 0;
           case CHROMAKEY_NORMAL:if(between(rr,rmin,rmax) &&
                                    between(gg,gmin,gmax) &&
                                    between(bb,bmin,bmax)) return 1;
                                 else return 0;
           case CHROMAKEY_INVERT:if(!between(rr,rmin,rmax) ||
                                    !between(gg,gmin,gmax) ||
                                    !between(bb,bmin,bmax)) return 1;
                                 else return 0;
           default: return 1;
      }
}


//--------------------------------------------------------------------------//
// root - put image on root window                                          //
// mode can be ON OFF PERMANENT_TILE                                        //
//--------------------------------------------------------------------------//
void root(int ino, int mode)
{
   int j,k,bpp,width,height;
   if(ino<=0){ message("Please select an image"); return; }
   Window owin = z[ino].win;
   Pixmap pixmap;
   int imgxsize = z[ino].xsize;
   int imgysize = z[ino].ysize;
   int xcount = (int)(0.9999 + (double)g.xres / (double)imgxsize);
   int ycount = (int)(0.9999 + (double)g.yres / (double)imgysize);
   XImage *image;
   switch(mode)
   {   case ON:  
           z[ino].win = g.root_window; 
           break;
       case PERMANENT_TILE:  
           ////  Don't let X11 tile it automatically, it will screw up by spacing out
           ////  the image to fit screen evenly, leaving black areas.
           bpp = g.bitsperpixel; 
           if(bpp<8) bpp=8;
           if(g.sparse_packing && bpp==32) bpp=24;
           pixmap = XCreatePixmap(g.display, g.root_window, g.xres, g.yres, bpp);
           image = createximage(imgxsize, imgysize, bpp, z[ino].img_1d);
           for(j=0;j<ycount;j++)
           for(k=0;k<xcount;k++)
           {    width = min(imgxsize, g.xres-k*imgxsize);
                height = min(imgysize, g.yres-j*imgysize);
                XPutImage(g.display, pixmap, g.image_gc, image, 0, 0, 
                     k*imgxsize, j*imgysize, width, height); 
                copyimage(0,0,width,height,k*imgxsize,j*imgysize,PUT,image,g.root_window);
           }
           XSetWindowBackgroundPixmap(g.display, g.root_window, pixmap);
           XFreePixmap(g.display, pixmap);
           send_expose_event(g.root_window,Expose,0,0,g.xres,g.yres);
           break;
       case OFF: 
           z[ino].win = XtWindow(z[ino].widget); 
           break;
   }
   redraw(ino);  
   redrawscreen();
   copybackground(0,0,z[ino].xsize,z[ino].ysize,-1);
   send_expose_event(z[ino].win,Expose,0,0,z[ino].xsize,z[ino].ysize);
   send_expose_event(z[ino].win,NoExpose,0,0,z[ino].xsize,z[ino].ysize);
   send_expose_event(owin,Expose,0,0,z[ino].xsize,z[ino].ysize);
}


//--------------------------------------------------------------------------//
// changetitle                                                              //
//--------------------------------------------------------------------------//
void changetitle(int noofargs, char **arg)
{ 
   int ino=ci;
   if(g.getout){ g.getout=0; return; }
   if(!between(ino,0,g.image_count-1)) { message("Bad image number",ERROR); return; }
   if(noofargs>=1) strncpy(z[ino].name, arg[1], FILENAMELENGTH-1);
   else message("New image name:",z[ino].name,PROMPT,FILENAMELENGTH-1,54);
   setimagetitle(ino, z[ino].name);
}


//--------------------------------------------------------------------------//
// string_index - does what index() should be doing: returns the position   //
// of the 1st or lastt occurrence of c in text.   If not found, returns 0.  //
//--------------------------------------------------------------------------//
int string_index(char *text, int direction, int c)
{
    int k;
    int len=strlen(text);
    if(direction>0)
    {    for(k=0;k<len;k++) if(text[k]==c) return k; }
    else
    {    for(k=len-1;k>=0;k--) if(text[k]==c) return k; }
    return 0;
}

t                                                                                                                                                                                                      