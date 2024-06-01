//-------------------------------------------------------------------------//
// xmtnimage51.cc                                                          //
// Spot densitometry                                                       //
// Latest revision: 01-26-2005                                             //
// See xmtnimage.h for Copyright Notice                                    //
//-------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern Widget www[4];
int SELECT_MODE = SINGLE;
const int DENS_MAX=10000;

//// Globals for densitometry
static ScanParams denssp; 
static ScanParams odenssp;
Widget fuzzyform=0;
double densresults[FUZZY_ITEMS];
int hitdefinesize=0;
extern int getpointstate;
extern int getboxstate;
static XYData spotdata;
int in_spot_dens_dialog=0;
int in_spot_densitometry=0;
int flistcount=0;
static listinfo *dlist;       
static listinfo *dfuzzyli=NULL;
static uint **savebox;
static uint *savebox_1d; 
int got_fuzzylist = 0;
int got_dlist     = 0;
int got_savebox   = 0;
int dens_count    = 0;

//-------------------------------------------------------------------------//
// densitometry                                                            //
// Entry point for spot densitometry                                       //
// All measurements have to be in the same image or on the main window     //
//-------------------------------------------------------------------------//
void densitometry(void)
{
  static int hitdens = 0;
  int j, k, yy;
  static Dialog *dialog;
  char tempstring[1024];
  g.block_okay = 1;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  if(ci<0){ message("No images selected"); return; }
  dialog       = new Dialog;
  if(in_spot_dens_dialog) return;;
  in_spot_dens_dialog=1;

  g.getout=0;
  got_fuzzylist = 0;
  g.state=DENSITOMETRY;

  if(!hitdens)                     // Default parameters
  {     denssp.autosave           = 0; // Not used
        denssp.bdensity           = 1.0;   // Default background density
        denssp.fdensity           = 0;
        denssp.bsignal            = 0;
        denssp.fsignal            = 0;
        denssp.netsignal          = 0;
        denssp.netdensity         = 0;
        denssp.leavemarked        = 0; // Flag if want to leave area marked
        denssp.invert             = 1; // 1= max signal = od of 0
        denssp.gamma_inverted     = 0; // 1=gamma table is highest for pixel=0
        denssp.scanwidth          = 1; // Not used
        denssp.selection_method   = 2; // Method for selecting coordinates
        denssp.snapthreshold      = 0; // Not used
        denssp.compensate         = 0; // Want pixel compensation (0,1, or 2)
        denssp.autobkg            = 0; // Want automatic background calculation
        denssp.wantfixedwidth     = 0; // Not used
        denssp.wantpause          = 0; // Not used
        denssp.wantplot           = 0; // Not used
        denssp.wantbox            = 0; 
        denssp.calfac             = 1; // Calibration factor
        denssp.skip               =-2; // Image to skip (not used)
        denssp.wantcolor          = 0; // Color densitometry
        denssp.diameter           = 12;
        denssp.scan               = g.scan; // Where to put the results
        denssp.color              = 0;
        for(k=0;k<256;k++) denssp.od[k] = k;

        spotdata.label = new char*[DENS_MAX];
        spotdata.label2 = new char*[DENS_MAX];
        for(k=0; k<DENS_MAX; k++){ spotdata.label[k] = new char[1024]; spotdata.label[k][0]=0; } 
        for(k=0; k<DENS_MAX; k++){ spotdata.label2[k] = new char[1024]; spotdata.label2[k][0]=0; } 
        spotdata.x = new int[DENS_MAX];
        spotdata.y = new int[DENS_MAX];
        spotdata.u = new double[DENS_MAX];
        spotdata.v = new double[DENS_MAX];
        spotdata.x1 = new int[DENS_MAX];
        spotdata.y1 = new int[DENS_MAX];
        spotdata.x2 = new int[DENS_MAX];
        spotdata.y2 = new int[DENS_MAX];
        if(g.diagnose)
        {   if(spotdata.y2==NULL) printf("spotdata.y2 is null\n");
        }
        if(spotdata.y2==NULL){ message(g.nomemory, ERROR); return; }
  }else
  {     denssp.autosave           = odenssp.autosave;
        denssp.bdensity           = odenssp.bdensity;
        denssp.bsignal            = odenssp.bsignal;
        denssp.leavemarked        = odenssp.leavemarked;
        denssp.invert             = odenssp.invert;
        denssp.gamma_inverted     = odenssp.gamma_inverted;
        denssp.scanwidth          = odenssp.scanwidth;
        denssp.selection_method   = odenssp.selection_method;
        denssp.snapthreshold      = odenssp.snapthreshold;
        denssp.compensate         = odenssp.compensate;
        denssp.autobkg            = odenssp.autobkg;
        denssp.wantfixedwidth     = odenssp.wantfixedwidth;
        denssp.wantpause          = odenssp.wantpause;
        denssp.wantbox            = odenssp.wantbox; 
        denssp.wantplot           = odenssp.wantplot;
        denssp.calfac             = odenssp.calfac;
        denssp.skip               = odenssp.skip;
        denssp.wantcolor          = odenssp.wantcolor;
        denssp.diameter           = odenssp.diameter;
        denssp.scan               = odenssp.scan;
        denssp.color              = odenssp.color;
  }
  hitdens = 1;

  strcpy(dialog->title,"   Spot densitometry");
  strcpy(dialog->radio[0][0],"Maximum signal");
  strcpy(dialog->radio[0][1],"Black");
  strcpy(dialog->radio[0][2],"White");
  
  strcpy(dialog->radio[1][0],"Area selection");
  strcpy(dialog->radio[1][1],"Automatic");
  strcpy(dialog->radio[1][2],"Manual (rectangular)");
  strcpy(dialog->radio[1][3],"Fixed size (rectangular)");
  strcpy(dialog->radio[1][4],"Manual (irregular)");
  strcpy(dialog->radio[1][5],"Manual (circle)");
  strcpy(dialog->radio[1][6],"Fixed shape (irregular)");

  strcpy(dialog->radio[2][0],"Pixel density calib.");
  strcpy(dialog->radio[2][1],"None ");
  strcpy(dialog->radio[2][2],"OD table");
  strcpy(dialog->radio[2][3], z[ci].cal_title[2]);

  strcpy(dialog->radio[3][0],"Data source");
  strcpy(dialog->radio[3][1],"Interactive");
  strcpy(dialog->radio[3][2],"Spot list");
  strcpy(dialog->radio[3][3],"Disk file");

  strcpy(dialog->radio[4][0],"Area selection mode");
  strcpy(dialog->radio[4][1],"Single");
  strcpy(dialog->radio[4][2],"Multiple");
  strcpy(dialog->radio[4][3],"Polygon");
  strcpy(dialog->radio[4][4],"Point-to-point");

  if(denssp.invert)dialog->radioset[0] = 1;
              else dialog->radioset[0] = 2;
  dialog->radioset[1] = denssp.selection_method;
  dialog->radioset[2] = denssp.compensate+1;
  dialog->radioset[3] = g.spot_dens_source;
  dialog->radioset[4] = SELECT_MODE;
 
  dialog->radiono[0] = 3;
  dialog->radiono[1] = 7;
  dialog->radiono[2] = 4;
  dialog->radiono[3] = 4;
  dialog->radiono[4] = 5;
  dialog->radiono[5] = 0;
 
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
 
  strcpy(dialog->boxes[0],"Parameters");
  strcpy(dialog->boxes[1],"Bkgd. value");
  strcpy(dialog->boxes[2],"Calib. factor");
  strcpy(dialog->boxes[3],"Leave area marked");
  strcpy(dialog->boxes[4],"Auto bkgd/fuzzy k means");
  strcpy(dialog->boxes[5],"Diameter");
 
  strcpy(dialog->boxes[6],"Bkgd. value");
  strcpy(dialog->boxes[7],"Calib.factor");
  strcpy(dialog->boxes[8],"Diameter");
  strcpy(dialog->boxes[9],"Box around area");
  strcpy(dialog->boxes[10],"Box color");
  strcpy(dialog->boxes[11],"Box color");

  dialog->boxtype[0] = LABEL;
  dialog->boxtype[1] = DOUBLECLICKBOX;
  dialog->boxtype[2] = STRING;
  dialog->boxtype[3] = TOGGLE;
  dialog->boxtype[4] = TOGGLE;
  dialog->boxtype[5] = INTCLICKBOX;
  dialog->boxtype[6] = LABEL;
  dialog->boxtype[7] = LABEL;
  dialog->boxtype[8] = LABEL;
  dialog->boxtype[9] = TOGGLE;
  dialog->boxtype[10] = INTCLICKBOX;
  dialog->boxtype[11] = LABEL;
 
  dialog->boxmin[1] = 0;  dialog->boxmax[1] = 1;
  dialog->boxmin[5] = 1;  dialog->boxmax[5] = 400;
  dialog->boxmin[10] = 0; dialog->boxmax[10] = (int) g.maxvalue[z[ci].bpp];
 
  doubletostring(denssp.bdensity, g.signif, tempstring); strcpy(dialog->answer[1][0],tempstring);
  doubletostring(denssp.calfac,   g.signif, tempstring); strcpy(dialog->answer[2][0],tempstring);
  if(denssp.leavemarked==1) dialog->boxset[3]=1; else dialog->boxset[3]=0;
  if(denssp.autobkg==1)     dialog->boxset[4]=1; else dialog->boxset[4]=0;
  sprintf(dialog->answer[5][0], "%d", denssp.diameter);
  dialog->boxset[9] = denssp.wantbox;
  sprintf(dialog->answer[10][0], "%d", denssp.color);
 
  //// Use a custom format dialog box
  //// Boxes below radios
  
  yy = 510;
  dialog->boxxy[0][0] = 6;   dialog->boxxy[0][1] = yy; 
  dialog->boxxy[0][2] = 90;  dialog->boxxy[0][3] = 20; // parameters label  
  yy+=20;

  dialog->boxxy[1][0] = 6;   dialog->boxxy[1][1] = yy; 
  dialog->boxxy[1][2] = 90;  dialog->boxxy[1][3] = 20; // bkg value
  dialog->boxxy[6][0] = 100; dialog->boxxy[6][1] = yy; 
  dialog->boxxy[6][2] = 100; dialog->boxxy[6][3] = 20;
  yy+=20;

  dialog->boxxy[2][0] = 6;   dialog->boxxy[2][1] = yy; 
  dialog->boxxy[2][2] = 90;  dialog->boxxy[2][3] = 20; // calib fac
  dialog->boxxy[7][0] = 100; dialog->boxxy[7][1] = yy; 
  dialog->boxxy[7][2] = 100; dialog->boxxy[7][3] = 20;
  yy+=20;

  dialog->boxxy[8][0] = 100; dialog->boxxy[8][1] = yy; 
  dialog->boxxy[8][2] = 100; dialog->boxxy[8][3] = 23;
  dialog->boxxy[5][0] = 6;   dialog->boxxy[5][1] = yy; 
  dialog->boxxy[5][2] = 90;  dialog->boxxy[5][3] = 20; // diameter
  yy+=24;

  dialog->boxxy[3][0] = 6;   dialog->boxxy[3][1] = yy; 
  dialog->boxxy[3][2] = 160; dialog->boxxy[3][3] = 20; // leave area marked
  yy+=20;

  dialog->boxxy[4][0] = 6;   dialog->boxxy[4][1] = yy; 
  dialog->boxxy[4][2] = 160; dialog->boxxy[4][3] = 20; // auto backgd
  yy+=20;

  dialog->boxxy[9][0] = 6;   dialog->boxxy[9][1] = yy; 
  dialog->boxxy[9][2] = 160; dialog->boxxy[9][3] = 20; // want box
  yy+=20;

  dialog->boxxy[10][0] = 6;   dialog->boxxy[10][1] = yy; 
  dialog->boxxy[10][2] = 90;  dialog->boxxy[10][3] = 20; // want box
  dialog->boxxy[11][0] = 100; dialog->boxxy[11][1] = yy; 
  dialog->boxxy[11][2] = 100; dialog->boxxy[11][3] = 23;
  yy+=20;

  dialog->radiousexy = 0;
  dialog->boxusexy   = 1;
  dialog->width      = 180;
  dialog->height     = 705;
  dialog->spacing    = 0;
  dialog->radiostart = 6;
  dialog->radiowidth = 100;
  dialog->boxstart   = 5;
  dialog->boxwidth   = 100;
  dialog->labelstart = 150;
  dialog->labelwidth = 50;        
  dialog->noofradios = 5;
  dialog->noofboxes  = 12;
  dialog->helptopic  = 6;
  for(k=0; k<dialog->noofboxes; k++) dialog->wantfunc[k]=0;
  dialog->wantfunc[1]= 1;
  dialog->want_changecicb=0;
  dialog->f1 = densitometry_check;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = null;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = densitometry_cancel;
  dialog->transient = 1;
  dialog->wantraise = 0;
  strcpy(dialog->path,".");
  dialog->message[0]=0;      
  denssp.count = 0;
  dens_count = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
// densitometry_cancel                                                      //
//--------------------------------------------------------------------------//
void densitometry_cancel(dialoginfo *a)
{
  a->busy = 0;
  g.getout = 1;
  in_spot_dens_dialog = 0;
}


//--------------------------------------------------------------------------//
//  densitometry_check                                                       //
//--------------------------------------------------------------------------//
void densitometry_check(dialoginfo *a, int radio, int box, int boxbutton)
{
//  if(!in_spot_densitometry) return;
  static int in_spot_densitometry_check = 0;
  int xsize, ysize, status=OK;
  a=a;radio=radio; box=box; boxbutton=boxbutton;
  if(a==NULL) return;
  g.getout = 0;
  if(a->radioset[0]==1) denssp.invert=1; else denssp.invert=0;
  denssp.selection_method = a->radioset[1];
  denssp.compensate       = a->radioset[2] - 1;
  g.spot_dens_source      = a->radioset[3];
  SELECT_MODE             = a->radioset[4];
  if(g.diagnose) printf("densitometry_check\n");

  //// Set to manual rectangular if reading spot list from disk
  if(g.spot_dens_source>1) denssp.selection_method=2;
  denssp.bdensity         = atof(a->answer[1][0]);
  denssp.calfac           = atof(a->answer[2][0]);
  denssp.leavemarked      = a->boxset[3];
  denssp.diameter         = atoi(a->answer[5][0]);
  denssp.wantbox          = a->boxset[9];
  denssp.color            = atoi(a->answer[10][0]);
  check_gamma_table(&denssp, denssp.ino, denssp.bpp);

  if(radio != -2) return;           // User clicks Accept button to accept their fate
  a->busy = 1;                      // Prevent window manager from closing dialog box
  if(in_spot_densitometry_check) return;  // prevent clicking twice

  //// No returns past this point
  in_spot_densitometry = 1;
  in_spot_densitometry_check = 1;
  g.getout = 0;

  if(!got_savebox)
  {   
       // Setting denssp.count to 0 here will reset counter after every
       // group of measurements (causes problem if reading spot file)
       //       denssp.count       = 0;     // No. of measurements made so far
       denssp.pixels      = 0;              // Total no. of pixels
       denssp.fsignal     = 0.0;            // Signal density value
       xsize = g.xres+2; ysize=g.yres+2;

       savebox_1d = (uint*)malloc(xsize*ysize*sizeof(uint));
       savebox = make_2d_alias(savebox_1d, xsize, ysize);  // Backup screen area
       if(savebox==NULL){ message(g.nomemory,ERROR); g.getout=1;}
       if(g.diagnose) printf("Allocating memory in spot densitometry %d %d %d\n",xsize,ysize,got_savebox);
       got_savebox = 1;
       a->ptr[13] = savebox;
       a->ptr[14] = savebox_1d;
  }

  //// Allocate list for fuzzy k means 
  //// Don't let them change this at random
  denssp.autobkg     = a->boxset[4];  
  if(denssp.autobkg && !got_fuzzylist) 
  {   
       hitdefinesize = 0;
       dfuzzyli  = new listinfo;
       a->ptr[11] = dfuzzyli;
       if(g.diagnose) printf("opening fuzzy list\n");
       got_fuzzylist=1;
       fuzzyform = open_fuzzy_list(dfuzzyli);
       dfuzzyli->busy = 1;
  }else a->ptr[11] = dfuzzyli;

  //// Allocate list for densitometry results
  if(got_dlist)
       dlist->want_finish = 0;
  else
  {
       if(g.diagnose) printf("opening dlist\n");
       dlist = new listinfo;
       dlist->title = new char[1024];
       strcpy(dlist->title, "Densitometry Results");
       dlist->info  = new char*[10000]; // Max. of 10000 densitometric measurements
       dlist->count = 0;
       dlist->transient    = 1;
       dlist->wantfunction = 0;
       dlist->autoupdate   = 0;
       dlist->clearbutton  = 1;
       dlist->highest_erased = 0;
       dlist->finished     = 0;
       dlist->allowedit    = 0;
       dlist->f1 = null;                // action to perform when item is selected
       dlist->f2 = null;                // how to update list 
       dlist->f3 = dlist_finish;        // what to delete when done
       dlist->f4 = delete_list;         // what else to delete when done
       dlist->f5 = null;                // action to perform when item is selected
       dlist->f6 = densitometry_list_close;  // what to do before deleting list
       dlist->browser     = 0;
       dlist->wc          = 0;
       dlist->itemstoshow = 10;
       dlist->firstitem   = 1;
       dlist->selection   = &g.crud;
       dlist->count       = 0;
       dlist->itemstoshow = 10;
       dlist->firstitem   = 1;
       dlist->wantsort    = 0;
       dlist->wantsave    = 1;
       dlist->helptopic   = 6;
       dlist->width       = 660;
       dlist->want_finish = 0;
       dlist->listnumber  = 1;
       list(dlist);  // this sets dlist->browser
       dlist->busy  = 1;
       got_dlist = 1;
  }
  if(g.diagnose) printf("memory allocated ok in spot dens\n");

  //// All memory alloc. above must be checked to avoid re-allocating
  //// Add to calculator
  add_variable((char*)"COMPENSATE", (double)denssp.compensate);
  add_variable((char*)"INVERT", (double)denssp.invert); 
  if(got_dlist)
  {   a->ptr[4]  = dlist; 
      status = densitometry_calc(a);
  }
  a->busy = 0;

  //// If reading from file, just leave dialog up
  if(status == ERROR)
     ;   // error handled in densitometry_calc, hopefully
  else switch(g.spot_dens_source)
  {    case 1:  message("Densitometry mode finished"); break;
       case 2:  message("Finished"); break;
       default: break;
  }
  densitometry_finish(a);
  in_spot_densitometry = 0;
  in_spot_densitometry_check = 0;
  g.getout = 0;
  return;
}


//--------------------------------------------------------------------------//
//  densitometry_start                                                      //
//--------------------------------------------------------------------------//
void densitometry_start(dialoginfo *a)
{
   a=a;
}


//--------------------------------------------------------------------------//
//  densitometry_list_close                                                 //
//--------------------------------------------------------------------------//
void densitometry_list_close(listinfo *l)
{
  if(g.diagnose) printf("densitometry_list_close\n");
  g.getout = 1;
  g.getboxstate= 0;
  g.getlinestate= 0;
  g.getpointstate= 0;
  l->busy = 0;
  if(l->listnumber == 1) 
  {    got_dlist = 0;
       denssp.count = 0;     // No. of measurements made so far
       dens_count = 0;
  }
  if(l->listnumber == 2) got_fuzzylist = 0;
}


//--------------------------------------------------------------------//
// dlist_finish                                                       //
//--------------------------------------------------------------------//
void dlist_finish(listinfo *l)
{
  l=l;
  if(g.diagnose) printf("dlist_finish\n");
}


//--------------------------------------------------------------------------//
//  densitometry_done                                                       //
//--------------------------------------------------------------------------//
void densitometry_done(dialoginfo *a)
{
  a=a;
  in_spot_dens_dialog = 0;
  odenssp.autosave          = denssp.autosave;
  odenssp.bdensity          = denssp.bdensity;
  odenssp.bsignal           = denssp.bsignal;
  odenssp.leavemarked       = denssp.leavemarked;
  odenssp.invert            = denssp.invert;
  odenssp.gamma_inverted    = denssp.gamma_inverted;
  odenssp.scanwidth         = denssp.scanwidth;
  odenssp.selection_method  = denssp.selection_method;
  odenssp.snapthreshold     = denssp.snapthreshold;
  odenssp.compensate        = denssp.compensate;
  odenssp.autobkg           = denssp.autobkg;
  odenssp.wantfixedwidth    = denssp.wantfixedwidth;
  odenssp.wantpause         = denssp.wantpause;
  odenssp.wantplot          = denssp.wantplot;
  odenssp.wantbox           = denssp.wantbox;
  odenssp.calfac            = denssp.calfac;
  odenssp.skip              = denssp.skip;
  odenssp.wantcolor         = denssp.wantcolor;
  odenssp.diameter          = denssp.diameter;
  odenssp.scan              = denssp.scan;
}


//--------------------------------------------------------------------------//
//  densitometry_finish                                                      //
//--------------------------------------------------------------------------//
void densitometry_finish(dialoginfo *a)
{
  if(g.diagnose) printf("entering densitometry_finish\n");
  g.state=NORMAL;
  g.getout = 1;
  char **savebox = (char **)a->ptr[13];
  char *savebox_1d = (char *)a->ptr[14];
  densitometry_done(a);
  if(got_savebox) 
  {  
      free(savebox); 
      free(savebox_1d);
      got_savebox = 0;
  }

  g.state=NORMAL;
  g.getout=0;
  hitdefinesize = 0;

  fuzzyform=0; 
  g.mouse_button=0;
  g.block = max(0, g.block);
  drawselectbox(OFF);
  in_spot_dens_dialog=0;
  g.getout = 1;   // force breakout of select_area()
  if(g.diagnose) printf("leaving densitometry_finish\n");
  a->busy = 0;
  return;
}


//--------------------------------------------------------------------------//
//  densitometry_calc                                                        //
//--------------------------------------------------------------------------//
int densitometry_calc(dialoginfo *a)
{
  g.getout = 0;
  if(!in_spot_densitometry) return ABORT;
  if(g.diagnose) printf("entering densitometry_calc\n");
  Widget www, scrollbar;
  Widget www2, scrollbar2;
  double background_density;
  static int hitdefinesize=0;
  static char filename[FILENAMELENGTH];
  char tempstring[1024];
  FILE *fp;
  double vvv=0.0, THRESH=0.02, ttt,
      fill1=0, fill2=0, fillcolor1, fillcolor2;
  int aino, bbb, bpp, button=0, ino=-1, invert=0, image_moved=0,
      iii, i, j, k, oldx, oldy, ppp, dx=0, dy=0, ocount, color,
      oino=-2, omainxpos=0, omainypos=0, oxmin=0, oxmax=0, oymin=0, oymax=0,
      oxpos=0, oypos=0, status=OK, sm, want_progress_bar=0,
      x, y, x1=0, x2=0, y1=0, y2=0, xhalf=0, yhalf=0, skipping=0, toosmall=0,
      xsize, ysize, xmin, xmax, ymin, ymax, xstart=0, ystart=0;
  if(!a) fprintf(stderr, "null dialog in densitometry_calc\n");
  if(g.diagnose)
  {   printf("b densitometry_calc\n");
      if(dlist==NULL) printf("dlist is null\n");
      if(savebox==NULL) printf("savebox is null\n");
  }
  if(dlist==NULL || savebox==NULL) return ABORT; 
  ino = ci;
  //// Must do densitometry on an actual image
  if(ino<1){ message("Select an image first"); return ERROR; }

  xsize = g.xres+2; ysize=g.yres+2;
  array<char> dbool(xsize+2, ysize+2);  // Boolean array for densitometry 
  if(g.diagnose)
  {   printf("d densitometry_calc xsize %d ysize %d\n",xsize,ysize);
      printf("dbool.allocated=%d\n",dbool.allocated);
  }
  if(!dbool.allocated)                  // Only need to check the last malloc.
  {   message(g.nomemory,ERROR);
      g.getout = 1; 
      return NOMEM;
  }
  if(g.spotlist==NULL)
  {   g.spotlist = new char[EDITSIZE]; // leave allocated
      g.spotlist[0] = 0; 
      if(g.diagnose) printf("g.spotlist is null\n");
  }
  if(g.spotlist==NULL) { message(g.nomemory,ERROR); return NOMEM; }
  if(g.diagnose) printf("done allocating memory in densitometry_calc\n");
  
  //// No returns past this point

  if(!denssp.selection_method) denssp.selection_method = 2;
  switch(g.spot_dens_source)
  {   case 3:          // Disk file
          strcpy(filename, getfilename(filename, NULL));
          if(!strlen(filename)) { g.getout=1; break; }
          if ((fp=fopen(filename,"rb")) == NULL){ error_message(filename, errno); g.getout=1; }
          if(g.getout) break;
          fread(g.spotlist, 1, EDITSIZE, fp);
          fclose(fp);
          for(k=1;k<25; k++) check_event(); 
          // fall through
      case 2:           // Spot list
          if(g.spotlist != NULL) parse_spotdata(&spotdata, g.spotlist);
          else{ message("Spot list is empty"); g.getout=1; }
          if(spotdata.n<1){ message("Spot list is empty"); g.getout=1; }
          break;
      case 1:           // Interactive
          switch(denssp.selection_method)
          {   case 1:
                  message("Move to center of area\npress left mouse button\nESC or Right button to stop\n");
                  break;
              case 2: message("Select areas to measure"); break;
              case 3: message("Select size to measure"); break;
              case 4: 
              case 6: if(denssp.autobkg)  
                  {  ////  Must be all on one line (Digital compiler can't handle '\')
                      sprintf(tempstring,"Warning: %cAuto background%c and %cirregular\nregion%c were both selected. Your area selections\nmust include similar numbers of foreground and\nbackground pixels.", '"', '"', '"', '"'); 
                      if(g.want_messages > 1) message(tempstring, WARNING);  
                  }
                  break;
              default:
                  break;
          }
          break;
      default: g.spot_dens_source = 1;
  }

  ////  Get the center point of the spot to measure, its bpp & image no.   
  ////  Use the image no. at center of selected area for all calculations. 

  g.busy++;     // dont delete image
  g.block_okay = 1;
  g.waiting++;  // dont close listbox
  if(want_progress_bar) progress_bar_open(www, scrollbar);
  ocount = dens_count;
  ////  Pause to make sure temporary message window is drawn
  XtAppProcessEvent(g.app, XtIMAll);
  XtAppProcessEvent(g.app, XtIMAll);
  XtAppProcessEvent(g.app, XtIMAll);
  XtAppProcessEvent(g.app, XtIMAll);


  while(!g.getout && ! (got_dlist && dlist->want_finish))
  {    
         button=0;
         check_event();
         if(g.getout || dlist->finished) break;
         if(!got_dlist) break;
         if(!in_spot_densitometry) break;
         denssp.ino = ino;
         denssp.bpp = bpp;
         z[ino].hitgray=1;    // prevent recalculating grayscalemap

         sm = denssp.selection_method;
         if(g.diagnose) printf("a in densitometry loop source=%d method=%d ci=%d\n",g.spot_dens_source,sm,ci);

         if(g.spot_dens_source == 1)               // Interactive
         {    
               g.want_switching = 0;               // prevent grabbing the area
               if(sm==2 || (sm==3 && !hitdefinesize))
               { 
                    button = getbox(x1,y1,x2,y2);  // this calls block(), must recheck state after returning
                    if(g.getout || !in_spot_densitometry) break;
                    x = (x2+x1)/2;
                    y = (y2+y1)/2;              
                    dx = (x2-x1)/2;
                    dy = (y2-y1)/2;              
                    ino = whichimage(xhalf, yhalf, bpp);
                    if(denssp.selection_method==3) 
                    {    sprintf(tempstring,
                           "Ready to begin densitometry\non %d x %d regions\nClick main Cancel button when finished",dx+dx,dy+dy);
                         message(tempstring);
                         hitdefinesize = 1;
                    }
               }
               if(sm==1 || sm==5 || (sm==3 && hitdefinesize))
               { 
                    button=getpoint(x,y);
                    ino = whichimage(x, y, bpp);
               }  
               if(sm==4)
               {    
                    if(g.getout || !in_spot_densitometry) break;
                    select_area(SELECT_MODE);  //// Waits here
                    if(g.region_ulx!=0 || g.region_uly!=0) g.getout = 0;
                    x1 = g.ulx; x2 = g.lrx; y1 = g.uly; y2 = g.lry;
                    xhalf = x = (x2+x1)/2;
                    yhalf = y = (y2+y1)/2;              
                    ino = whichimage(xhalf, yhalf, bpp);
               }
               if(sm==6)
               {    if(hitdefinesize) 
                    {   
                         g.getlabelstate = 1;
                         button = getpoint(x,y);
                         shift_selected_area(xstart,ystart,x,y);
                         g.getlabelstate = 0;
                         xstart = x; ystart = y;
                         x1 = g.ulx; x2 = g.lrx; y1 = g.uly; y2 = g.lry;
                    }else
                    {    
                         if(g.getout || !in_spot_densitometry) break;
                         select_area(SELECT_MODE);
                         if(g.region_ulx!=0 || g.region_uly!=0) g.getout = 0;
                         xstart = (g.lrx + g.ulx) / 2;
                         ystart = (g.lry + g.uly) / 2;
                         hitdefinesize = 1;
                         x1 = g.ulx; x2 = g.lrx; y1 = g.uly; y2 = g.lry;
                         x = (x2+x1)/2;
                         y = (y2+y1)/2;              
                         message("Ready to begin densitometry\non fixed shape regions\nClick main Cancel button when finished");
                    }
                    ino = whichimage(x, y, bpp);
               }
               g.want_switching = 1;
         }else
         {  
               if(dens_count >= spotdata.n + ocount) break;
               //if(want_progress_bar) progress_bar_update(scrollbar, dens_count*100/spotdata.n);
               x = spotdata.x[dens_count - ocount];
               y = spotdata.y[dens_count - ocount];
               x1 = spotdata.x1[dens_count - ocount];
               y1 = spotdata.y1[dens_count - ocount];
               x2 = spotdata.x2[dens_count - ocount];
               y2 = spotdata.y2[dens_count - ocount];
               ino = whichimage(x, y, bpp);
               if(!between(ino, 1, g.image_count-1)){ skipping++; dens_count++; continue; }
               xhalf = (x2+x1)/2;
               yhalf = (y2+y1)/2;                             
         }
         if(g.diagnose) printf("b in densitometry loop source=%d method=%d ci=%d\n",g.spot_dens_source,sm,ci);
         if(!got_savebox) g.getout=1;
         check_event();
         if(g.getout || dlist->finished || button==2) break;
         if(keyhit()) if(getcharacter()==27) break;
         if(!in_spot_densitometry) break;
         g.state=DENSITOMETRY;
         denssp.ino = ino;
         denssp.bpp = bpp;
         ////  Calculate boundaries
         if(!between(ino, 0, g.image_count-1)) break;
         if(ino>=0 && z[ino].shell) 
         {    xmin = z[ino].xpos;
              ymin = z[ino].ypos;
              xmax = z[ino].xpos + z[ino].xsize;
              ymax = z[ino].ypos + z[ino].ysize;            
              xmin=min(g.xres-g.main_xpos, max(xmin,-g.main_xpos));
              xmax=min(g.xres-g.main_xpos, max(xmax,-g.main_xpos));
              ymin=min(g.yres-g.main_ypos, max(ymin,-g.main_ypos));
              ymax=min(g.yres-g.main_ypos, max(ymax,-g.main_ypos));
         }else 
         {    xmin = 0;
              ymin = 0;
              xmax = g.main_xsize;
              ymax = g.main_ysize;
         }
         if(g.diagnose) printf("in densitometry_calc %d %d %d %d\n",x1,y1,x2,y2);

         ////  Save the background the 1st time, or if any windows were moved.   
         ////   'inmenu' must be 0 in order to get the true pixel value. This is 
         ////   especially important if image is not at screen bpp.            
         ////  Density calculations are done at the bpp of the image at the      
         ////    selected point. 
                                                         
         if(!got_savebox) g.getout = 1;
         check_event();
         if(g.getout || dlist->finished) break;
         if(!in_spot_densitometry) break;
         image_moved = 0; 
         if(between(ino, 0, g.image_count-1))
         { if(z[ino].xpos!=oxpos ||  z[ino].ypos!=oypos) image_moved=1; }
         if(xmax!=oxmax || ymax!=oymax || xmin!=oxmin || ymin!=oymin) image_moved=1;
         if(g.main_xpos!=omainxpos || g.main_ypos!=omainypos) image_moved=1;

         if(ino!=oino || image_moved)
            for(j=ymin; j<ymax; j++)
            for(k=xmin; k<xmax; k++)
                savebox[j-ymin][k-xmin] = readpixelonimage(k,j,bbb,iii);
         oino=ino;
         oxmax=xmax; oxmin=xmin; oymax=ymax; oymin=ymin;
         omainxpos=g.main_xpos; omainypos=g.main_ypos;
         if(between(ino, 0, g.image_count-1))
         { oxpos=z[ino].xpos; oypos=z[ino].ypos; }
        
         ////  Recheck the parameters in case user changed something
         ////  Any dialog entries the user could change should be
         ////  updated here

         denssp.bdensity = atof(a->answer[1][0]);  // Let user change
         if(got_fuzzylist) dfuzzyli = (listinfo *) a->ptr[11];

         denssp.selection_method = a->radioset[1];

         if(denssp.selection_method <= 0) denssp.selection_method = 2;
         denssp.compensate       = a->radioset[2] - 1;
         if(g.spot_dens_source>1) denssp.selection_method=2;
         denssp.bdensity         = atof(a->answer[1][0]);
         denssp.calfac           = atof(a->answer[2][0]);
         denssp.leavemarked      = a->boxset[3];
         denssp.diameter         = atoi(a->answer[5][0]);
         denssp.diameter = atoi(a->answer[5][0]);
         check_gamma_table(&denssp, denssp.ino, denssp.bpp);
         //// Add to calculator
         add_variable((char*)"COMPENSATE", (double)denssp.compensate);
         add_variable((char*)"INVERT", (double)denssp.invert);
         ////  Check gamma table validity                                       
         status = check_gamma_table(&denssp, ino, bpp);
         if(status!=OK) break;
         invert = denssp.invert;
         if(denssp.gamma_inverted) invert = 1-invert;
         denssp.pixels = 0; denssp.fsignal = 0;

         oldx = x; oldy = y;
         denssp.ino = ino;
         check_event();

         aino = whichimg(x, y, bpp);
         if(aino>=0 && z[aino].is_zoomed)
         {
               x = zoom_x_coordinate(x, aino);
               y = zoom_y_coordinate(y, aino);
               x1 = zoom_x_coordinate(x1, aino);
               y1 = zoom_y_coordinate(y1, aino);
               x2 = zoom_x_coordinate(x2, aino);
               y2 = zoom_y_coordinate(y2, aino);   
               dx = zoom_x_coordinate(dx, aino);
               dy = zoom_y_coordinate(dy, aino);
               xhalf = zoom_x_coordinate(xhalf, aino);
               yhalf = zoom_y_coordinate(yhalf, aino);
          }

         if(g.diagnose){ printf("in densitometry_calc %d %d aino=%d ",x,y,aino);fflush(stdout);}
         if(g.diagnose) printf(" zoomed=%d\n",z[aino].is_zoomed);
         if(g.getout || dlist->finished) break;
         if(!in_spot_densitometry) break;

         ////  Start densitometry                                               
         ////  Measure total signal and number of pixels in spot          

         switch(denssp.selection_method)
         {  case 1:  // Automatic
              if(denssp.invert)
               {  fillcolor1 = 1.0;           // Color to fill with on 1st pass
                  fillcolor2 = 0.0;           // Color to fill with on 2nd pass
                  fill1      = 0.98;          // Minimum border color
                  fill2      = denssp.bdensity;   // Maximum border color
               }else
               {  fillcolor1 = 0.0;           // Color to fill with on 1st pass
                  fillcolor2 = 1.0;           // Color to fill with on 2nd pass
                  fill1      = 0.02;          // Minimum border color
                  fill2      = denssp.bdensity;   // Maximum border color
               }
               progress_bar_open(www2, scrollbar2);
               fill(x,y,x1,y1,x2,y2,fillcolor1,fill1,fill2,ppp,ttt);
               x1 = max(0, min(x1, xsize-1));
               y1 = max(0, min(y1, ysize-1));
               x2 = max(0, min(x2, xsize-1));
               y2 = max(0, min(y2, ysize-1));
               for(j=y1;j<=y2;j++)for(i=x1;i<=x2;i++) 
                    dbool.p[j-ymin][i-xmin]=0;
               ////  Store 1 bit in array 'b' if the pixel is fillcolor1.
               progress_bar_update(scrollbar2, 25);
               for(i=x1;i<=x2;i++)
                  for(j=y1;j<=y2;j++)
                     if(fabs((double)readpixelonimage(i,j,bpp,ino)/
                             (double)g.maxvalue[bpp] - fillcolor1) < THRESH) 
                         dbool.p[j-ymin][i-xmin] = 1;                  
               x=oldx; y=oldy;
               ////  fill area again with a different color guaranteed
               ////  to overwrite the earlier fillcolor1 pixels.
               progress_bar_update(scrollbar2, 50);
               fill(x,y,x1,y1,x2,y2,fillcolor2,fill1,fill2,ppp,ttt);
               x1 = max(0, min(x1, xsize-1));
               y1 = max(0, min(y1, ysize-1));
               x2 = max(0, min(x2, xsize-1));
               y2 = max(0, min(y2, ysize-1));
               ////  If pixel didn't change, it is outside the area
               ////  because it was already the fill color, so set its 
               ////  bit to 0 if it was a 1.  There should not be any of these. 

               progress_bar_update(scrollbar2, 75);
               for(i=x1;i<=x2;i++)
               {  for(j=y1;j<=y2;j++)
                  {
                     if(dbool.p[j-ymin][i-xmin]==1)
                        if(fabs((double)readpixelonimage(i,j,bpp,ino)/
                                (double)g.maxvalue[bpp] - fillcolor2) >= THRESH)
                           dbool.p[j-ymin][i-xmin] = 0;                      
                  }
               }   
               denssp.pixels = 0;
               for(i=x1;i<=x2;i++)
               {
                  for(j=y1;j<=y2;j++)
                  {
                     if(dbool.p[j-ymin][i-xmin]==1) 
                     {    denssp.pixels++;
                          setpixelonimage(i,j,savebox[j-ymin][i-xmin],SET,bpp);
                          vvv = pixeldensity(i,j,ino,denssp.compensate,invert);
                          denssp.fsignal += vvv;
                      } 
                  }
               }
               if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
               progress_bar_close(www2);
               rebuild_display(ino);
               redraw(ino);
               break;
            case 2:       // Manual
               for(i=x1;i<x2;i++)
               for(j=y1;j<y2;j++)
               {    setpixel(i,j,g.maxcolor,XOR);
                    denssp.pixels++;
                    denssp.fsignal += pixeldensity(i,j,ino,denssp.compensate,invert);
               }
               if(!denssp.leavemarked)
               {   for(i=x1;i<x2;i++)
                   for(j=y1;j<y2;j++)
                       setpixel(i,j,g.maxcolor,XOR);
               }
               else
               {    // use a color that doesn't force grayscalemap remapping
                   color = cint(g.maxvalue[bpp] * denssp.fsignal / denssp.pixels); 
                   for(i=x1;i<x2;i++)
                   for(j=y1;j<y2;j++)
                       setpixelonimage(i,j,color,SET,bpp,0,0,0,0,0,0);
               }
               if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
               break;
            case 3:      // Fixed
               if(hitdefinesize)   
               {  x1 = x - dx;
                  x2 = x + dx;
                  y1 = y - dy;
                  y2 = y + dy;
                  for(i=x1;i<x2;i++)
                  for(j=y1;j<y2;j++)
                  if((i>xmin)&&(j>ymin)&&(i<xmax)&&(j<ymax))
                  {   setpixel(i,j,g.maxcolor,XOR);
                      denssp.pixels++;
                      denssp.fsignal += pixeldensity(i,j,ino,denssp.compensate,invert);
                  }
                  if(!denssp.leavemarked)
                  {   for(i=x1;i<x2;i++)
                      for(j=y1;j<y2;j++)
                         if((i>xmin)&&(j>ymin)&&(i<xmax)&&(j<ymax))
                                setpixel(i,j,g.maxcolor,XOR);
                  }                       
                  if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
               }
               break;
            case 4:      //  Non-rectangular     
               for(i=x1;i<x2;i++)
               for(j=y1;j<y2;j++)
               {    if(!inside_irregular_region(i,j)) continue;
                    setpixel(i,j,g.maxcolor,XOR);
                    denssp.pixels++;
                    denssp.fsignal += pixeldensity(i,j,ino,denssp.compensate,invert);
               }
#ifdef LINUX
               usleep(10000);
#endif
               if(!denssp.leavemarked)
               {    for(i=x1;i<x2;i++)
                    for(j=y1;j<y2;j++)
                    {    if(!inside_irregular_region(i,j)) continue;
                         setpixel(i,j,g.maxcolor,XOR);
                     }
               }
               else
               {  
                    // use a color that doesn't force grayscalemap remapping
                    color = cint(g.maxvalue[bpp] * denssp.fsignal / denssp.pixels); 
                    for(i=x1;i<x2;i++)
                    for(j=y1;j<y2;j++)
                    {    if(!inside_irregular_region(i,j)) continue;
                         setpixelonimage(i,j,color,SET,bpp,0,0,0,0,0,0);
                     }
             }
               if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
            break;
            case 5:       // Circle
               x1 = x-denssp.diameter/2;
               x2 = x+denssp.diameter/2;
               y1 = y-denssp.diameter/2;
               y2 = y+denssp.diameter/2;
               for(i=x1;i<x2;i++)
               for(j=y1;j<y2;j++)
               if(insidecircle(i,j,x,y,denssp.diameter))
               {   setpixel(i,j,g.maxcolor,XOR);
                   denssp.pixels++;
                   denssp.fsignal += pixeldensity(i,j,ino,denssp.compensate,invert);
               }
               if(!denssp.leavemarked)
               {   for(i=x1;i<x2;i++)
                   for(j=y1;j<y2;j++)
                         if(insidecircle(i,j,x,y,denssp.diameter))
                              setpixel(i,j,g.maxcolor,XOR);
               }
               if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
               break;
            case 6:       // Fixed shape irregular
               for(i=x1;i<x2;i++)
               for(j=y1;j<y2;j++)
               {    if(!inside_irregular_region(i,j)) continue;
                    setpixel(i,j,g.maxcolor,XOR);
                    denssp.pixels++;
                    denssp.fsignal += pixeldensity(i,j,ino,denssp.compensate,invert);
               }
#ifdef LINUX
               usleep(10000);
#endif
               if(!denssp.leavemarked)
               {    for(i=x1;i<x2;i++)
                    for(j=y1;j<y2;j++)
                    {    if(!inside_irregular_region(i,j)) continue;
                         setpixel(i,j,g.maxcolor,XOR);
                    }
               }
               if(denssp.wantbox) box(x1+1,y1,x2,y2,denssp.color,SET,0,ci);
               break;
          }
          check_event();
          if(g.getout || dlist->finished) break;

          ////  Density of the background 
          ////  Calculate results         
          background_density = 0;
          if(denssp.pixels<2)
          {    if(denssp.selection_method > 1) toosmall++;
               else message("Area too small");            
          }else if(denssp.autobkg && got_fuzzylist) 
          { 
               if(!dfuzzyli){ message("Fuzzy list is null!!!", BUG); break; }
               if(!dfuzzyli->title){ message("Fuzzy title is null!!!", BUG); break; }
               background(&denssp,x1,y1,x2,y2,invert,densresults,dfuzzyli);
               update_fuzzy_list(dfuzzyli);
               denssp.pixels = (int)densresults[18];        // Total no. of pixels
               denssp.bsignal  = denssp.bdensity*denssp.pixels; // Bkg. total signal
               denssp.fsignal  = densresults[10];           // Band total signal
               denssp.fdensity = densresults[11];           // Band density  
               denssp.netdensity = densresults[14];         // Density spot - background
               denssp.fpixels    = (int)densresults[22];    // Total pixels in spot
               denssp.netsignal  = denssp.netdensity*denssp.fpixels; // Spot density * spot pixels 
          }else 
          { 
              if(denssp.pixels==0)
              {    denssp.fdensity = 0;
                   denssp.fsignal  = 0;
                   denssp.bsignal  = 0;
              }else
                   denssp.fdensity = denssp.fsignal/denssp.pixels;
              if(bpp==8)
              {    if(got_savebox)
                      background_density = (double)(denssp.od[(int(denssp.bdensity*255.0))])/255.0;
                   else 
                      background_density = 0;   
              }else if(invert)
                   background_density = 1.0 - denssp.bdensity; 

              denssp.bsignal    = background_density * denssp.pixels;
              denssp.netsignal  = denssp.fsignal  - denssp.bsignal;
              denssp.netdensity = denssp.fdensity - background_density;
              denssp.fpixels    = denssp.pixels;       // Pixels in spot
          }
          
          background_density *= denssp.calfac;            // Bkg. density
          denssp.bsignal  *= denssp.calfac;            // Bkg. total signal
          denssp.fsignal  *= denssp.calfac;            // Band total signal
          denssp.fdensity *= denssp.calfac;            // Band density  
          denssp.netdensity *= denssp.calfac;          // Density spot - background
          denssp.netsignal  *= denssp.calfac;          // Spot density * spot pixels 

         // This has to be in a separate function because gcc can't handle    
         // long functions.                                               
         denssp.count++;
         if(denssp.count >= DENS_MAX-1)
         {   message("Data table is full",WARNING);
             g.getout=1;
         }
         spotdata.ino = ino;
         if(got_dlist)
              show_densitometry_results(x, y, dlist, &spotdata, dens_count);
         if(g.diagnose) printf("z in densitometry loop %d %d %d\n",dens_count,denssp.count,spotdata.ino);
         dens_count++;
  }

  if(want_progress_bar) progress_bar_close(www);
//  if(toosmall)
//  {  sprintf(tempstring, "%d entries had\nfewer than 2 pixels", toosmall);
//     message(tempstring, WARNING);
//  }
//  if(skipping)
//  {   sprintf(tempstring, "%d entries had data points\nnot on the image\nand were skipped", skipping);
//      message(tempstring, WARNING);
//  }

  hitdefinesize = 0;   
  g.busy = max(0,g.busy-1);
  g.block_okay = 0;
  g.waiting = max(0,g.waiting-1);
  g.want_switching = 1;
  return OK;
}


//-------------------------------------------------------------------------//
// background - return a bkg value 0..1                                    //
//-------------------------------------------------------------------------//
void background(ScanParams *sp, int x1, int y1, int x2, int y2, int invert,
   double *results, listinfo *li)
{
    int i,j,k,count,want_progress_bar=1;
    double *pixels;
    count = max(1,(y2-y1)*(x2-x1));
    pixels = new double[count];
    k=0;
    if(!between(sp->ino, 0, g.image_count-1)) return;
    for(j=y1;j<y2;j++)
    for(i=x1;i<x2;i++)
         pixels[k++] = pixeldensity(i,j,sp->ino,sp->compensate,invert);
    if(denssp.selection_method > 1) want_progress_bar=0;
    cluster(pixels, count, 2, 150, results, li, want_progress_bar);
    delete[] pixels;
    return;
}


//-------------------------------------------------------------------------//
// check_gamma_table                                                       //
//-------------------------------------------------------------------------//
int check_gamma_table(ScanParams *sp, int ino, int bpp)
{
   int highest, k, status=OK;
   if(!between(ino, 0, g.image_count-1)) return 0;
   if(bpp==8)
   {    sp->gamma_inverted = 0;
        highest=255;
        if(between(sp->ino, 0, g.image_count-1))
        {   for(k=0;k<=255;k++)
                 if(z[ino].gamma[k] > z[ino].gamma[highest]) highest=k;
            if(highest!=255 && highest!=0)
            {    message("Error: Bad OD table\nTurn pixel compensation off",ERROR);
                 status = ERROR;
            }
            if(highest != 255) sp->gamma_inverted=1;
            for(k=0;k<=255;k++)sp->od[k] = z[ino].gamma[k]; 
        }
   }
   return status;
}         


//-------------------------------------------------------------------------//
// pixeldensity - returns optical density of pixel as a                    //
//          double between 0 and 1 where 1 is the max. possible density.   //
//          Corrects for bpp of the pixel. If ino<0 it calculates it.      //
// Uses screen coordinates. See also pixeldensity_image which uses image   //
//          coordinates.  Converts to double, 0..1                         // 
//-------------------------------------------------------------------------//
double pixeldensity(int x, int y, int ino, int compensate, int invert)
{ 
   int bpp;
   uint value;
   double dval;
   char answerstring[64];
   switch(compensate)
   {   case 0:  value = readpixelonimage(x,y,bpp,ino,-2);
                if(invert) value = (uint)g.maxvalue[bpp] - value;   
                dval = (double)value / g.maxvalue[bpp];
                break;
       case 1:
                value = readpixelonimage(x,y,bpp,ino,-2);
                if(invert) value = (uint)g.maxvalue[bpp] - value;   
                if(between(ino, 0, g.image_count-1))
                {   if(bpp==8) value = z[ino].gamma[value]; }
                dval = (double)value / g.maxvalue[bpp];
                break;
       case 2:  if(ino<0) ino = whichimage(x,y,bpp);
                if(between(ino, 0, g.image_count-1))
                calibratepixel(x, y, ino, 2, dval, answerstring);
                break;
       default: message("Bad compensate type",BUG); 
    } 
    return dval;    
}


//-------------------------------------------------------------------------//
// pixeldensity_image - Same as pixeldensity except uses image coordinates.//
// Converts to double, 0..1                                                // 
//-------------------------------------------------------------------------//
double pixeldensity_image(int x, int y, int ino, int f, int compensate, 
   int invert)
{ 
   int bpp,b;
   if(!between(ino, 0, g.image_count-1)) return 0.0;
   bpp = z[ino].bpp;
   b = g.off[bpp];
   uint value;
   double dval;
   char answerstring[64];
   switch(compensate)
   {   case 0:  value = pixelat(z[ino].image[f][y]+b*x, bpp);
                if(invert) value = (uint)g.maxvalue[bpp] - value;   
                dval = (double)value / g.maxvalue[bpp];
                break;
       case 1:
                value = pixelat(z[ino].image[f][y]+b*x, bpp);
                if(invert) value = (uint)g.maxvalue[bpp] - value;   
                if(bpp==8) value = z[ino].gamma[value];                           
                dval = (double)value / g.maxvalue[bpp];
                break;
       case 2:  calibratepixel(x-z[ino].xpos, y-z[ino].ypos, ino, 2, 
                    dval, answerstring);
                break;
       default: message("Bad compensate type",BUG); 
    } 
    return dval;    
}


//-------------------------------------------------------------------------//
// pixeldensityat - returns optical density of pixel between 0 and 1.      //
// Coordinates are relative to image buffer, not the screen.               //
// Doesn't use ScanParams or calibration values. Returns raw frac 0 to 1.  //
// Do not use for densitometry, use pixeldensity() instead.                //
//-------------------------------------------------------------------------//
double pixeldensityat(int x, int y, int frame, int ino)
{ 
   if(!between(ino, 0, g.image_count-1)) return 0.0;
   int bpp = z[ino].bpp;
   int b = g.off[bpp];
   uint value;
   if(!between(y,0,z[ino].ysize-1) || !between(x,0,z[ino].xsize-1))
   {   //fprintf(stderr, "error in pdy %d %d\n",x,y); 
       return 0.0; 
   } 
   value = pixelat(z[ino].image[frame][y]+x*b, bpp);
   return ((double)value)/g.maxvalue[bpp];    // Convert to double, 0..1 
}


//-------------------------------------------------------------------------//
// colorpixeldensity - returns optical density of rgb components of pixel  //
//      as a double between 0 and 1 where 1 is the max. possible density.  //
//      Corrects for bpp of the pixel.                                     //
//-------------------------------------------------------------------------//
void colorpixeldensity(int x, int y, double &rdensity, double &gdensity, 
     double &bdensity, int invert)
{ 
   int bpp,ino,rr,gg,bb;
   uint value;
   value = readpixelonimage(x,y,bpp,ino,-2);
   valuetoRGB(value,rr,gg,bb,bpp);
   if(invert) 
   {    rr = (uint)g.maxred[bpp] - rr;   
        gg = (uint)g.maxgreen[bpp] - gg;   
        bb = (uint)g.maxblue[bpp] - bb;   
   }
   rdensity = (double)rr/(double)g.maxred[bpp];
   gdensity = (double)gg/(double)g.maxgreen[bpp];
   bdensity = (double)bb/(double)g.maxblue[bpp];
}


//-------------------------------------------------------------------------//
// show_densitometry_results                                               //
//-------------------------------------------------------------------------//
void show_densitometry_results(int x, int y, listinfo *dlist, XYData *data, int count)
{
  if(g.getout || !got_dlist || dlist==NULL || dlist->finished) return;
  if(!in_spot_densitometry) return;
  if(dlist == NULL) return;
  if(g.diagnose) printf("entering show_densitometry_results\n");
  char temp[256];
  char temp2[256];
  char tempstring[256];
  int len=0, k, ino;
  double xcalc, ycalc;
  ////  If the 1st item, set up the title and 1st item in a listinfo struct,
  ////  then open a list window with no items.

  dlist->info[dlist->count] = new char[256];   // Deleted at end of densitometry()
  if(denssp.count==1) 
  {     int lhelptopic=6;
        strcpy(dlist->info[0],  "No.x        y         Area      Tot.Signal  Spot Density  Total Bkgd. Bkgd.Density  Net density  Signal-Bkg.");
        if(g.spot_dens_source > 1) strcat(dlist->info[0], "Label  Identity");
        dlist->count  = 0;
        dlist->itemstoshow = 10;
        dlist->firstitem   = 1;
        dlist->wantsort    = 0;
        dlist->wantsave    = 1;
        dlist->helptopic   = lhelptopic;
        dlist->allowedit   = 0;
        dlist->width       = 7 * (strlen(dlist->info[0]));
        if(g.diagnose) printf("adding item %d to list\n", dlist->count);
        additemtolist(dlist->widget, dlist->browser, dlist->info[0], BOTTOM, 1);
        dlist->count++;
        dlist->info[dlist->count] = new char[256];
  }
  ////  For other results, compose a string and append it to the browser.
  ////  Use doubletostring instead of sprintf so the no. of significant
  ////  digits can be changed by the user
  ////  Count is not always same as denssp.count
    
  ino = data->ino;
  if(ino>=0 && z[ino].cal_log[0] >= 0)
  {   calibratepixel(x, y, ino, 0, xcalc, temp);
      calibratepixel(x, y, ino, 1, ycalc, temp);
  }else
  {   xcalc = (double)x;
      ycalc = (double)y;
  }

  itoa(denssp.count, temp, 10);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=3-len;k++) strcat(temp," ");

  doubletostring(xcalc, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=12-len;k++) strcat(temp," ");

  doubletostring(ycalc, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=22-len;k++) strcat(temp," ");

  ltoa(denssp.pixels, tempstring, 10);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=32-len;k++) strcat(temp," ");

  doubletostring(denssp.fsignal, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=44-len;k++) strcat(temp," ");

  doubletostring(denssp.fdensity, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=58-len;k++) strcat(temp," ");

  doubletostring(denssp.bsignal, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=70-len;k++) strcat(temp," ");

  doubletostring(denssp.bdensity, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=84-len;k++) strcat(temp," ");    

  doubletostring(denssp.netdensity, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=97-len;k++) strcat(temp," ");

  doubletostring(denssp.netsignal, g.signif, tempstring);
  strcat(temp,tempstring);
  strcat(temp,"  ");len=strlen(temp); for(k=1;k<=108-len;k++) strcat(temp," ");

  if(g.spot_dens_source>1)
  {    sprintf(temp2, "   %s   %s", data->label[count], data->label2[count]);
       strcat(temp, temp2);
  }

  additemtolist(dlist->widget, dlist->browser, temp, BOTTOM, 1);
  strcpy(dlist->info[dlist->count], temp);
  dlist->count++;
  
  denssp.pixels = 0;
  denssp.fsignal = 0.0;
  if(g.diagnose) printf("leaving show_densitometry_results\n");
}


