//-------------------------------------------------------------------------//
// xmtnimage52.cc                                                          //
// Strip densitometry                                                      //
// Latest revision: 05-02-2005                                             //
// Copyright (C) 2003 by Thomas J. Nelson                                  //
// See xmtnimage.h for Copyright Notice                                    //
//-------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image       *z;
extern int         ci;
extern Widget www[4];
const int SELECT_MODE = SINGLE;

//// Globals for scan_region
XYData *srdata; 
ScanParams srsp;                      
ScanParams osrsp;
int srhit = 0;
int in_strip_dens_dialog=0;

//-------------------------------------------------------------------------//
// scan_region - Entry point for strip densitometry.                       //
//-------------------------------------------------------------------------//
void scan_region(void)
{  
   if(in_strip_dens_dialog) return;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   srdata = new XYData;
   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   in_strip_dens_dialog = 1;
   int j, k;
   char tempstring[128];    
   if(!srhit)                          // Defaults
   {    sprintf(srsp.filename,"%s%s",g.startdir,"/1.scn");
        srsp.ino                = ci;
        srsp.bpp                = z[ci].bpp;
        srsp.autosave           = 0;   // Automatically save scan data
        srsp.bdensity           = 0;   // Not used
        srsp.count              = 0;   // No. of measurements made so far
        srsp.leavemarked        = 0;   // Flag if want to leave area marked
        srsp.invert             = 1;   // =1 if max value==black
        srsp.gamma_inverted     = 0;   // 1=gamma table is highest for pixel=0
        srsp.selection_method   = 3;   // Which method for getting coordinates
        srsp.scanwidth          = 10;
        srsp.snapthreshold      = 3;   // 
        srsp.compensate         = 0;   // Want pixel compensation
        srsp.autobkg            = 0;   // Automatically calculate background
        srsp.wantfixedwidth     = 0;   // Flag if rectangle is fixed width
        srsp.wantrepeat         = 0;
        srsp.wantpause          = 0;   // 
        srsp.wantplot           = 1;   // 1=plot scan in graph
        srsp.calfac             = 1.0; // Calibration factor
        srsp.skip               = -2;
        srsp.wantcolor          = 0;   // 1=color scan 0=grayscale
        srsp.diameter           = 12;
        srsp.scan               = g.scan; // where to put the results
        srsp.graph.type         = NONE;
        srsp.graph.pd           = NULL;
   }else
   {    strcpy(srsp.filename, osrsp.filename);
        srsp.ino                = osrsp.ino;
        srsp.bpp                = osrsp.bpp;
        srsp.autosave           = osrsp.autosave;
        srsp.bdensity           = osrsp.bdensity;
        srsp.count              = osrsp.count;
        srsp.leavemarked        = osrsp.leavemarked;
        srsp.invert             = osrsp.invert;
        srsp.gamma_inverted     = osrsp.gamma_inverted;
        srsp.selection_method   = osrsp.selection_method;
        srsp.scanwidth          = osrsp.scanwidth;
        srsp.snapthreshold      = osrsp.snapthreshold;
        srsp.compensate         = osrsp.compensate;
        srsp.autobkg            = osrsp.autobkg;
        srsp.wantfixedwidth     = osrsp.wantfixedwidth;
        srsp.wantrepeat         = osrsp.wantrepeat;
        srsp.wantpause          = osrsp.wantpause;
        srsp.wantplot           = osrsp.wantplot;
        srsp.calfac             = osrsp.calfac;
        srsp.skip               = osrsp.skip;
        srsp.wantcolor          = osrsp.wantcolor;
        srsp.diameter           = osrsp.diameter;
        srsp.scan               = osrsp.scan;
        srsp.graph              = osrsp.graph;
   }
   srsp.pixels      = 0;                   // Total no. of pixels
   srsp.bdensity    = 0.0;                 // Background density value
   srsp.fsignal     = 0.0;                 // Total signal density value

   srdata->x = new int[300]; if(srdata->x==NULL)
                             {    message(g.nomemory,ERROR);
                                  in_strip_dens_dialog = 0;
                                  return; 
                             }
   srdata->y = new int[300]; if(srdata->y==NULL)
                             {     delete[] srdata->x; 
                                   message(g.nomemory,ERROR);  
                                   in_strip_dens_dialog = 0;
                                   return;
                             }  
   for(k=0;k<300;k++){ srdata->x[k]=0; srdata->y[k]=0; }
   srdata->n=0;
   
   strcpy(dialog->title,"Strip densitometry");
   strcpy(dialog->radio[0][0],"Coordinates");             
   strcpy(dialog->radio[0][1],"Select coordinates");
   strcpy(dialog->radio[0][2],"Repeat prev. scan");
   strcpy(dialog->radio[1][0],"Maximum signal");             
   strcpy(dialog->radio[1][1],"Black");
   strcpy(dialog->radio[1][2],"White");
   strcpy(dialog->radio[2][0],"Scan type");             
   strcpy(dialog->radio[2][1],"Rhomboid, 90°(4 pts.)");
   strcpy(dialog->radio[2][2],"Rhomboid (4 points)");
   strcpy(dialog->radio[2][3],"Fixed width (2 pts.)");
   strcpy(dialog->radio[3][0],"Pixel density calib.");
   strcpy(dialog->radio[3][1],"None ");
   strcpy(dialog->radio[3][2],"OD table");
   strcpy(dialog->radio[3][3], z[ci].cal_title[2]);
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
  
   strcpy(dialog->boxes[0],"Options");
   strcpy(dialog->boxes[1],"Auto save");
   strcpy(dialog->boxes[2],"Data file");
   strcpy(dialog->answer[2][0], srsp.filename);
   strcpy(dialog->boxes[3],"Plot result");
   strcpy(dialog->boxes[4],"Pause");
   strcpy(dialog->boxes[5],"Leave marked");
   strcpy(dialog->boxes[6],"Calib.fac");
   strcpy(dialog->boxes[7],"Calib.fac");
   strcpy(dialog->boxes[8],"Fixed width");
   strcpy(dialog->boxes[9],"Filename");
   strcpy(dialog->boxes[10],"Width");
   dialog->boxmin[8]=1; 
   dialog->boxmax[8]=200;
   itoa(srsp.scanwidth, dialog->answer[8][0], 10);
 
   dialog->boxtype[0]=LABEL;
   dialog->boxtype[1]=TOGGLE;
   dialog->boxtype[2]=STRING;
   dialog->boxtype[3]=TOGGLE;
   dialog->boxtype[4]=TOGGLE;
   dialog->boxtype[5]=TOGGLE;
   dialog->boxtype[6]=STRING;
   dialog->boxtype[7]=LABEL;
   dialog->boxtype[8]=INTCLICKBOX;
   dialog->boxtype[9]=LABEL;
   dialog->boxtype[10]=LABEL;

   if(srsp.wantrepeat) dialog->radioset[0]=2;
                  else dialog->radioset[0]=1;
   if(srsp.invert)     dialog->radioset[1]=1;
                  else dialog->radioset[1]=2; 
   dialog->radioset[2] = srsp.selection_method;
   dialog->radioset[3] = srsp.compensate+1;

   if(srsp.autosave) dialog->boxset[1]=1; else dialog->boxset[1]=0;
   if(srsp.wantplot) dialog->boxset[3]=1; else dialog->boxset[3]=0;
   if(srsp.wantpause)dialog->boxset[4]=1; else dialog->boxset[4]=0;
   if(srsp.leavemarked) dialog->boxset[5]=1; else dialog->boxset[5]=0;

   doubletostring(srsp.calfac, g.signif, tempstring);
   strcpy(dialog->answer[6][0], tempstring);

   dialog->radiono[0]=3;
   dialog->radiono[1]=3;
   dialog->radiono[2]=4;
   dialog->radiono[3]=4;
   dialog->radiono[4]=0;
   dialog->noofradios=4;
   dialog->noofboxes=11;
   dialog->helptopic=8;  
   dialog->width=400;
   dialog->height=0; // calculate automatically
   dialog->want_changecicb = 0;
   dialog->f1 = scanregioncheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = scanregionfinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   strcpy(dialog->path,".");
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;

   //// Use a custom format dialog box
   dialog->radiousexy = 0;
   dialog->boxusexy = 1;
   dialog->width = 170;
   dialog->height = 550;

   int y=325, dx=100, dy=20;
   //// threshold
   dialog->boxxy[0][0] = 6;
   dialog->boxxy[0][1] = y;
   dialog->boxxy[0][2] = dx;
   dialog->boxxy[0][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[1][0] = 6;
   dialog->boxxy[1][1] = y;
   dialog->boxxy[1][2] = dx;
   dialog->boxxy[1][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[2][0] = 6;
   dialog->boxxy[2][1] = y;
   dialog->boxxy[2][2] = dx;
   dialog->boxxy[2][3] = dy;

   dialog->boxxy[9][0] = 106;
   dialog->boxxy[9][1] = y;
   dialog->boxxy[9][2] = dx;
   dialog->boxxy[9][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[3][0] = 6;
   dialog->boxxy[3][1] = y;
   dialog->boxxy[3][2] = dx;
   dialog->boxxy[3][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[4][0] = 6;
   dialog->boxxy[4][1] = y;
   dialog->boxxy[4][2] = dx;
   dialog->boxxy[4][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[5][0] = 6;
   dialog->boxxy[5][1] = y;
   dialog->boxxy[5][2] = dx;
   dialog->boxxy[5][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[6][0] = 6;
   dialog->boxxy[6][1] = y;
   dialog->boxxy[6][2] = dx;
   dialog->boxxy[6][3] = dy;

   dialog->boxxy[7][0] = 106;
   dialog->boxxy[7][1] = y;
   dialog->boxxy[7][2] = dx;
   dialog->boxxy[7][3] = dy;

   y += cint(1.2*dy);
   dialog->boxxy[8][0] = 6;
   dialog->boxxy[8][1] = y;
   dialog->boxxy[8][2] = dx;
   dialog->boxxy[8][3] = dy;

   dialog->boxxy[10][0] = 106;
   dialog->boxxy[10][1] = y;
   dialog->boxxy[10][2] = dx;
   dialog->boxxy[10][3] = dy;


 
   dialog->spacing = 0;
   dialog->radiostart = 2;
   dialog->radiowidth = 150;
   dialog->boxstart = 2;
   dialog->boxwidth = 50;
   dialog->labelstart = 87;
   dialog->labelwidth = 50;        

   dialogbox(dialog);
   return;
}


//--------------------------------------------------------------------------//
//  scan_region_macro                                                       //
//  densitometry(x1,y1,x2,y2); in image math                                //
//--------------------------------------------------------------------------//
void scan_region_macro(int noofargs, char **arg)
{
   static int hit=0;
   int x1=0,x2=0,x3=0,x4=0,y1=0,y2=0,y3=0,y4=0,compensate=0;
   int k;
   double calfac = 1;
   if(noofargs<8){ message("Densitometry needs at least 8 arguments:\nx1, y1, x2, y2, x3, y3, x4, y4"); return; }

   if(noofargs >=1) x1 = atoi(arg[1]);
   if(noofargs >=2) y1 = atoi(arg[2]);

   if(noofargs >=3) x2 = atoi(arg[3]);
   if(noofargs >=4) y2 = atoi(arg[4]);

   if(noofargs >=5) x3 = atoi(arg[5]);
   if(noofargs >=6) y3 = atoi(arg[6]);

   if(noofargs >=7) x4 = atoi(arg[7]);
   if(noofargs >=8) y4 = atoi(arg[8]);

   if(noofargs >=9) compensate = atoi(arg[9]);
   if(noofargs >=10) calfac = atof(arg[10]);

   srdata = new XYData;
   sprintf(srsp.filename,"%s%s",g.startdir,"/1.scn");
   srsp.ino                = ci;
   srsp.bpp                = z[ci].bpp;
   srsp.autosave           = 0;         // Automatically save scan data
   srsp.bdensity           = 0;         // Not used
   srsp.count              = 0;         // No. of measurements made so far
   srsp.leavemarked        = 1;         // Flag if want to leave area marked
   srsp.invert             = 1;         // =1 if max value==black
   srsp.gamma_inverted     = 0;         // 1=gamma table is highest for pixel=0
   srsp.selection_method   = 2;         // Which method for getting coordinates
   srsp.scanwidth          = 10;
   srsp.snapthreshold      = 3;         // 
   srsp.compensate         = compensate;// Want pixel compensation
   srsp.autobkg            = 0;         // Automatically calculate background
   srsp.wantfixedwidth     = 0;         // Flag if rectangle is fixed width
   srsp.wantrepeat         = 0;
   srsp.wantpause          = 0;         // 
   srsp.wantplot           = 1;         // 1=plot scan in graph
   srsp.calfac             = calfac;    // Calibration factor
   srsp.skip               = -2;
   srsp.wantcolor          = 0;         // 1=color scan 0=grayscale
   srsp.diameter           = 12;
   srsp.scan               = g.scan;    // where to put the results
   if(!hit)
   {   srsp.graph.type     = NONE;
       srsp.graph.pd       = NULL;
   }
   srsp.pixels             = 0;         // Total no. of pixels
   srsp.bdensity           = 0.0;       // Background density value
   srsp.fsignal            = 0.0;       // Total signal density value
   srdata->x = new int[300];
   srdata->y = new int[300];  
   if(srdata->y==NULL)
   {     delete[] srdata->x; 
         message(g.nomemory,ERROR);  
         return;
   }  
   for(k=0;k<300;k++){ srdata->x[k]=0; srdata->y[k]=0; }
   srdata->x[0] = x1;
   srdata->x[1] = x2;
   srdata->x[2] = x3;
   srdata->x[3] = x4;
   srdata->y[0] = y1;
   srdata->y[1] = y2;
   srdata->y[2] = y3;
   srdata->y[3] = y4;
   srdata->n    = 4;
   srdata->nmin = 4;
   srdata->nmax = 4;
   srdata->type = 4;
   scan_region_calc(srdata, &srsp, NULL);
   delete[] srdata->y;
   delete[] srdata->x;  
   hit = 1;
}  

   
//--------------------------------------------------------------------------//
//  scanregioncheck                                                         //
//--------------------------------------------------------------------------//
void scanregioncheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   static int in_scanregioncheck = 0;
   static Widget www;
   int invert=0, aborted=0;
   radio=radio; box=box; boxbutton=boxbutton;
   if(g.getout) return; 
   if(g.diagnose) printf("scanregioncheck\n");
   if(in_scanregioncheck) return;

   if(a->radioset[0]==1) srsp.wantrepeat=0; else srsp.wantrepeat=1;    
   if(a->radioset[1]==1) srsp.invert=1;     else srsp.invert=0;
   srsp.compensate = a->radioset[3] - 1;
   if(!srhit) srsp.wantrepeat=0;             // can't repeat if 1st time
            
   srsp.selection_method = a->radioset[2];
   if(a->boxset[1]) srsp.autosave=1; else srsp.autosave=0;
   strcpy(srsp.filename, a->answer[2][0]);
   if(a->boxset[3]) srsp.wantplot=1; else srsp.wantplot=0;
   if(a->boxset[4]) srsp.wantpause=1; else srsp.wantpause=0;
   if(a->boxset[5]) srsp.leavemarked=1; else srsp.leavemarked=0;
   srsp.calfac    = atof(a->answer[6][0]);
   srsp.scanwidth = atoi(a->answer[8][0]);
 
   if(a->radioset[2] == 3)
         XtSetSensitive(a->boxwidget[8][0],True);
   else
         XtSetSensitive(a->boxwidget[8][0],False);

   //// Add to calculator
   add_variable((char*)"COMPENSATE", (double)srsp.compensate);
   add_variable((char*)"INVERT", (double)srsp.invert);

   srdata->n         = 0;
   srdata->dims      = 1;
   srdata->nmin      = 0;
   if(!in_scanregioncheck)
       srdata->nmax  = 300;
   srdata->v         = NULL;
   srdata->width     = srsp.scanwidth;
   srdata->type      = 4;
   srdata->duration  = TEMP;
   srdata->wantpause = 0;
   srdata->wantmessage = 1;
   srdata->order     = 0;
   srdata->win       = 0;      // calculate window automatically on drawing area
   

   if(radio != -2) return;     // Click ok or Enter to get past here
   a->busy = 1;                // Prevent window manager from closing dialog box
   in_scanregioncheck = 1;
   g.getout = 0;
   invert = srsp.invert;

   tempmessageopen(www);
   switch(srsp.selection_method)
   {
          case 1:
          case 2:
          tempmessageupdate(www, (char*)"Select 4 points, then press space bar. \n\
Press Esc or click 'cancel' when finished with  densitometry.");
          break;
          case 3:
          tempmessageupdate(www, (char*)"Select 2 points, then press space bar.\n\
Press Esc or click 'cancel' when finished with  densitometry.");
          break;
   }

   while(!g.getout)
   {   srsp.invert = invert;   // srsp.invert may get changed in scan_region_calc()
       check_event();
       scan_region_start(srdata, &srsp, a);
       if(g.getout){ g.getout=0; aborted=1; break; }
       g.block++;

       if(g.want_messages >=2)

       while(g.bezier_state==CURVE && !g.getout)
           XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
       g.block = max(0, g.block-1);  // Could already be 0 if maincancelbutton was clicked
       check_event();
       if(g.getout){ aborted=1; break; }
       scan_region_calc(srdata, &srsp, a);
   }
  tempmessageclose(www);

   //// Don't write to a->message_widget, a may have been deleted
   //// Don't reset g.getout, needed to break out of dialogokcb
   //// Don't unmap dialog here, it will hang

   a->busy = 0;
   message("Densitometry\nmode finished");
   strcpy(osrsp.filename, srsp.filename);
   osrsp.autosave           = srsp.autosave;
   osrsp.bdensity           = srsp.bdensity;
   osrsp.count              = srsp.count;
   osrsp.leavemarked        = srsp.leavemarked;
   osrsp.invert             = invert;
   osrsp.gamma_inverted     = srsp.gamma_inverted;
   osrsp.selection_method   = srsp.selection_method;
   osrsp.scanwidth          = srsp.scanwidth;
   osrsp.snapthreshold      = srsp.snapthreshold;
   osrsp.compensate         = srsp.compensate;
   osrsp.autobkg            = srsp.autobkg;
   osrsp.wantfixedwidth     = srsp.wantfixedwidth;
   osrsp.wantrepeat         = srsp.wantrepeat;
   osrsp.wantpause          = srsp.wantpause;
   osrsp.wantplot           = srsp.wantplot;
   osrsp.calfac             = srsp.calfac;
   osrsp.skip               = srsp.skip;
   osrsp.wantcolor          = srsp.wantcolor; 
   osrsp.diameter           = srsp.diameter;
   osrsp.scan               = srsp.scan;
   osrsp.graph              = srsp.graph;

   ////  Try to clean up if user hits main Cancel button
   ////  Don't call dialogunmap, it will crash is user closes window with
   ////  window manager
   
   if(aborted)
   {   
       bezier_curve_end(srdata);
       g.bezier_state=NORMAL;
       g.state=NORMAL;  
   }
   srhit = 1;
   in_scanregioncheck = 0;
   return;
}


//--------------------------------------------------------------------------//
//  scanregionfinish                                                        //
//--------------------------------------------------------------------------//
void scanregionfinish(dialoginfo *a)
{
   a=a;
   delete[] srdata->y;
   delete[] srdata->x;
   in_strip_dens_dialog = 0;
   a->busy = 0;
}


//-------------------------------------------------------------------------//
// scan_region_start - Continue scanning with parameters from scan_region  //
//               or end scanning                                           //
//-------------------------------------------------------------------------//
void scan_region_start(XYData *data, ScanParams *sp, dialoginfo *a)
{
   a=a;
   switchto(g.imageatcursor);       
   if(g.getout)
   {    g.getout=0;
        message("Densitometry mode finished"); 
        a->busy = 0;
   }else if(!sp->wantrepeat)
   {    sp->count = 0;
        data->n = 0;
        switch(sp->selection_method)
        { case 1: 
              data->type = 5;
              data->nmin = 4;
              data->nmax = 4;
              bezier_curve_start(data, a); 
              break;
          case 2: 
              data->type = 4;
              data->nmin = 4;
              data->nmax = 4;
              bezier_curve_start(data, a); 
              break;
          case 3: 
              data->type = 6;
              data->nmin = 2;
              data->nmax = 2;
              bezier_curve_start(data, a); 
              break;
          default:
              g.getout=1;
              break;
        }
   }
}


//-------------------------------------------------------------------------//
// scan_region_calc - measure density of a region bounded by trapezoid     //
//-------------------------------------------------------------------------//
void scan_region_calc(XYData *data, ScanParams *sp, dialoginfo *a)
{
    a=a;
    char tempstring[FILENAMELENGTH];    
    char extension[FILENAMELENGTH]="";
    int allnumbers, bad=0, bpp, dot, helptopic=8, i, ino, aino, j, k, 
        nmax, number=0, oldci=0, size, status=OK, usediag=0;
    int x[6], y[6];
    double total=0.0, xx[6], yy[6];
    double *xdata;
    char **ytitle;
    char *ptr;
    PlotData *pd;

    size = 2*(int)sqrt(g.xres*g.xres + g.yres*g.yres);  // Size of scan[] array
    ytitle = new char*[1];
    ytitle[0] = new char[256];

    xdata = new double[size];     // Largest possible diag. distance
    if(xdata == NULL){ message(g.nomemory,ERROR); delete[] ytitle[0]; return; }

    x[1] = data->x[0];
    x[2] = data->x[1];
    x[3] = data->x[2];
    x[4] = data->x[3];
    y[1] = data->y[0];
    y[2] = data->y[1];
    y[3] = data->y[2];
    y[4] = data->y[3];
    x[5] = (data->x[0] + data->x[1] + data->x[2] + data->x[3])/4;
    y[5] = (data->y[0] + data->y[1] + data->y[2] + data->y[3])/4;

    if(sgn(x[2]-x[1])!=sgn(x[4]-x[3])){ swap(x[3],x[4]); swap(y[3],y[4]); }
    if(sgn(y[2]-y[1])!=sgn(y[4]-y[3])){ swap(x[3],x[4]); swap(y[3],y[4]); }
 
    for(k=1;k<5;k++)
    {    aino = whichimg(x[k],y[k],bpp);
         if(z[aino].is_zoomed)
         {     x[k] = zoom_x_coordinate(x[k], aino);
               y[k] = zoom_y_coordinate(y[k], aino);
         }
    }

    xx[1] = (double)x[1];
    xx[2] = (double)x[2];
    xx[3] = (double)x[3];
    xx[4] = (double)x[4];
    xx[5] = (double)x[5];
    yy[1] = (double)y[1];
    yy[2] = (double)y[2];
    yy[3] = (double)y[3];
    yy[4] = (double)y[4];
    yy[5] = (double)y[5];

    //// Validate gamma tables & test if need to invert gamma.
    //// Do center point last. Center determines whether inverting is needed. 
    nmax = data->nmax;
    if(nmax-1 < 6)
       ino = whichimage(x[nmax-1],y[nmax-1],bpp);
    else ino = ci;
    sp->ino = ino;
    sp->bpp = bpp;

    if(ino<0){ delete[] xdata; delete[] ytitle[0]; delete[] ytitle; return; }
    // no grayscale map for color modes
    if(z[ino].bpp>8 && sp->compensate==1) sp->compensate=0;   
    status = check_gamma_table(sp, ino, bpp);
    if(status!=OK)
    {     g.getout=1;
          delete[] xdata;
          message("Densitometry mode finished"); 
          return;
    }
    if(sp->gamma_inverted) sp->invert = 1 - sp->invert;

    ////  Measure signal in selected region
    for(i=0;i<size;i++){ sp->scan[i]=0; xdata[i]=(double)i; }
 
    switch(sp->selection_method)
    {   case 1: usediag=0; break;
        case 2:          // If diagonal scan was selected but points were 
                         // vertical or horizontal
           if((abs(x[2]-x[1]) < sp->snapthreshold)||
              (abs(y[2]-y[1]) < sp->snapthreshold))
              usediag=0;
           else 
              usediag=1;
           break;
        case 3: usediag=2; break;
    }        
 
    switch(usediag)
    {   case 0: total = scan_normal_polygon(sp,x,y,g.scancount); break;
        case 1: total = scan_arbitrary_polygon(sp,xx,yy,g.scancount); break;
        case 2: total = scan_fixed_width_area(sp,xx,yy,g.scancount); break;
    }

    for(i=0;i<size;i++) sp->scan[i] *= sp->calfac; 
    total *= sp->calfac; 
    sprintf(tempstring,"Total signal scanned %g", total);
    //dialog_message(a->message_widget2, tempstring);
    if(g.diagnose) printf("scanregioncalc %d   %d %d %d %d %d   %d %d %d %d %d\n",
                            ino, x[1],x[2],x[3],x[4],x[5], y[1],y[2],y[3],y[4],y[5]);
  
    ////  Plot scan results
    strcpy(ytitle[0],"Signal");

    if(sp->wantplot) 
    { 
       if(!graph_is_open(sp->graph.pd))
       {   pd = plot(sp->filename, (char*)"Data Point", ytitle, xdata, 
                sp->scan, g.scancount, 1, MEASURE, 0, 0, NULL, null, null,
                null, null, null, helptopic);
           sp->graph = open_graph(pd, ci, STRIP_DENSITOMETRY_GRAPH);
           sp->graph.pd = pd;
           sp->graph.ino = ci;
           sp->graph.type = STRIP_DENSITOMETRY_GRAPH;
       }else 
       {   pd = sp->graph.pd;
           pd->n = g.scancount;
           delete[] pd->data[0];  
           pd->data[0] = new double[pd->n+1];       
           for(k=0; k<pd->n; k++) pd->data[0][k] = sp->scan[k];
           redraw_graph(pd);
       }
    }
    g.getout = 0;        // In case user clicks 'cancel' button
    if((sp->autosave)&&(!bad))
    { 
       save_scan(sp->filename, sp->scan, g.scancount, 1);
                         // Find last dot and slash in filename
       ptr = 1 + strrchr(sp->filename,'/');
       strcpy(tempstring, ptr);
       dot = (int)strlen(tempstring);
       for(j=0;j<(int)strlen(tempstring);j++) if(tempstring[j]=='.') dot=j;
                         // If text to left of dot is all digits, increment
                         // the filename.
       allnumbers=1;
       for(k=0;k<dot;k++) if(!isdigit(tempstring[k])) allnumbers=0;
       if(allnumbers)         
       {    memset(extension,0,strlen(extension));
            strncpy(extension,tempstring+dot+1,4);
            number = atoi(tempstring);
            tempstring[dot]=0;
            number++;
            itoa(number,tempstring,10);
            sprintf(sp->filename,"%s/%s.%s",g.startdir,tempstring,extension);
       }
    }
    if(status==GOTNEW)      // If plot created an image, put it in background
    {   switchto(oldci);      
        status=OK;
    }
    delete[] ytitle[0];
    delete[] ytitle;
    delete[] xdata;
}


//-------------------------------------------------------------------------//
// scan_normal_polygon  (called by scan_region_calc)                       //
// 4-sided polygon, vertical/horizontal starting edge, all points are at   //
// the center of a pixel.                                                  //
//-------------------------------------------------------------------------//
double scan_normal_polygon(ScanParams *sp, int x[], int y[], int &scancount) 
{
     int i,k,count,direction;
     double total=0.0, fvalue;
     int mode[3];
     mode[0]=XOR;
     mode[1]=XOR;
     mode[2]=NONE;

           //-------------Scan vertically or horizontally-------------//     
     if(abs(y[1]-y[2]) < sp->snapthreshold) direction=VERTICAL;
     else direction=HORIZONTAL;

     if(direction==VERTICAL)
     { 
       if((y[3]!=y[1])&&(y[4]!=y[2]))
       {
           /*-You have to have (int) before both terms here or it-----*/
           /*-doesn't divide correctly for some unfathomable reason---*/
           //-----------------Scan top to bottom----------------------//
             
          scancount = abs((int)y[3]-(int)y[1]);
          for(k=0;k<=2;k++)
          {  count=0;
             for(i=0;count<abs(y[3]-y[1]);i+=sgn(y[3]-y[1]))      
             {   if(k==2)
                 {  fvalue = scanline(sp, x[1]+(int)(i*(x[3]-x[1]))/(int)(y[3]-y[1]), 
                             y[1]+i, x[2]+(int)(i*(x[4]-x[2]))/(int)(y[4]-y[2]), 
                             y[2]+i);
                    sp->scan[count] = fvalue; 
                    total+=fvalue;
                 }else
                 {  g.inmenu++;
                    line(x[1]+(int)(i*(x[3]-x[1]))/(int)(y[3]-y[1]), y[1]+i,
                             x[2]+(int)(i*(x[4]-x[2]))/(int)(y[4]-y[2]), y[2]+i,
                             g.maxcolor,mode[k]);                  
                    g.inmenu--;
                 }
                 count++;      
             } 
          }                    
       }
     }else 
     { 
       if((x[3]!=x[1])&&(x[4]!=x[2]))   
       {
           //-----------------Scan left to right----------------------//   
          scancount = abs(x[3]-x[1]); 
          for(k=0;k<=2;k++)
          {  count=0;
             for(i=0;count<abs(x[3]-x[1]);i+=sgn(x[3]-x[1]))
             {  
                if(k==2)
                {   fvalue = scanline(sp, x[1]+i,
                             y[1]+(int)(i*(y[3]-y[1]))/(int)(x[3]-x[1]),
                             x[2]+i, y[2]+(int)(i*(y[4]-y[2]))/(int)(x[4]-x[2]));
                    sp->scan[count] = fvalue;       
                    total += fvalue;
                }else
                {   g.inmenu++;
                    line(x[1]+i,y[1]+(int)(i*(y[3]-y[1]))/(int)(x[3]-x[1]),
                             x[2]+i,y[2]+(int)(i*(y[4]-y[2]))/(int)(x[4]-x[2]),
                             g.maxcolor,mode[k]); 
                    g.inmenu--;
                }
                count++;
             }                                               
          }    
       }   
     }  
     return total;
}  


//-------------------------------------------------------------------------//
// scan_arbitrary_polygon   (called by scan_region_calc)                   //
// xx and yy range from 1 to 4, not 0 to 3                                 //
//-------------------------------------------------------------------------//
double scan_arbitrary_polygon(ScanParams *sp, double xx[],double yy[], 
   int &scancount) 
{
                           // Arbitrary 4-sided polygon. Points do not
                           // necessarily coincide with centers of pixels.
     int j,k,x,y;
     int parallelogram;
                                              // dx = x increment
                                              // dy = y increment
                                              // m  = slope
                                              // b  = y intercept
                                              // numbers = line segments
     double m,m12,m13,m14,m23,m24,m34;
     double b,b12,b13,b14,b23,b24,b34;
     double dx,dx12,dx31,dx41,dx32,dx42,dx43;
     double denom,total=0.0;
  
                        //-------Reorder the segments from left to right----   
                        //----After finding the starting edge. This makes---
                        //----calculations easier.--------------------------

     dx = (double)(xx[2]-xx[1]); if(dx==0) dx=1e-6;
     m = (yy[2] - yy[1])/dx; if(m==0) m=1e-6;     //--Slope of starting edge
     b = yy[1] - xx[1]*m;                         //--Intercept of starting edge
     scancount = 0;                               // No. points in scan perp.to 1--2
     if((m!=0)&&(m+1/m !=0)) denom = 1/(m + 1/m); else denom=1;          


     for(j=1;j<=4;j++)
     for(k=1;k<=4;k++)
        if(xx[k] > xx[j]){ fswap(xx[k],xx[j]); fswap(yy[k],yy[j]); }

     //-------Try to fix any bad parameters--------------
     //-------Prevent divide by 0 errors-----------------
     //---Slope & y-intercept based on original start
     //---and end points of segment 1--2 

     dx12 = (double)(xx[2]-xx[1]); if(dx12==0) dx12=1e-6;
     dx31 = (double)(xx[3]-xx[1]); if(dx31==0) dx31=1e-6;
     dx41 = (double)(xx[4]-xx[1]); if(dx41==0) dx41=1e-6;
     dx32 = (double)(xx[3]-xx[2]); if(dx32==0) dx32=1e-6;
     dx42 = (double)(xx[4]-xx[2]); if(dx42==0) dx42=1e-6;
     dx43 = (double)(xx[4]-xx[3]); if(dx43==0) dx43=1e-6;

     m12 = (yy[2] - yy[1])/dx12; if(m12==0) m12=1e-6;
     m13 = (yy[3] - yy[1])/dx31; if(m13==0) m12=1e-6;
     m14 = (yy[4] - yy[1])/dx41; if(m14==0) m12=1e-6;
     m23 = (yy[3] - yy[2])/dx32; if(m23==0) m12=1e-6;
     m24 = (yy[4] - yy[2])/dx42; if(m24==0) m12=1e-6;
     m34 = (yy[4] - yy[3])/dx43; if(m34==0) m12=1e-6;

     b12 = yy[1] - xx[1]*m12;
     b13 = yy[1] - xx[1]*m13;
     b14 = yy[1] - xx[1]*m14;
     b23 = yy[2] - xx[2]*m23;
     b24 = yy[2] - xx[2]*m24;
     b34 = yy[3] - xx[3]*m34;

                                    
     // The global 'scancount' is incremented in diagonalscan()      
     // which contains common code. For trapezoids, the area is      
     // split into 3 sections and 3 of these loops are executed.    
     // For a rectangle, only loop 'e' is executed.		     
     // If the edges are exactly vertical or horizontal, this method 
     // doesn't work. This case is handled by the rectilinear scan   
     // method above, which is faster.  			     
     // 							     
     // In the section below, the pixels are always scanned in the   
     // same order (y's changing fastest). The pixel values are      
     // automatically placed in the correct element of the array.    
     // The 6 cases are for the 6 possible shapes.		     

     //------Show the region being scanned---------------
     //------then scan diagonally------------------------
     //---If 2--3 is parallel to 1--4  
     // (difference in slope is less than 
     //  the diff. in slope between 1--2 and
     //  3--4 and 1--3 and 2--4)
    
     parallelogram=0;
     if( (fabs(m23-m14)<fabs(m12-m34)) && (fabs(m23-m14)<fabs(m13-m24)) )
        parallelogram=1;
     if(parallelogram)   
     {  
        for(k=1;k<=3;k++)           // Loop 'a' - change k loop to 1..3 if you
        {                           // want the area temporarily inverted.
           if(fabs(xx[1]-xx[2])>1.0)
           {    for(x=(int)xx[1]; x<(int)xx[2]; x++)
                for(y=(int)(m12*x+b12); y!=(int)(m14*x+b14); y+=sgn(m14-m12))
                {   if(k<3) setpixel(x,y,g.maxcolor,XOR);
                    else total+=diagonalscan(sp,x,y,m,b,denom);
                    if((y<=0)||(y>g.main_ysize-1))break;
                }   
           }
           if(fabs(xx[2]-xx[3])>2.0) // Loop 'b'
           {    for(x=(int)xx[2]; x<(int)xx[3]; x++)
                for(y=(int)(m23*x+b23); y!=(int)(m14*x+b14); y+=sgn(m14-m12))
                {   if(k<3) setpixel(x,y,g.maxcolor,XOR);
                    else total+=diagonalscan(sp,x,y,m,b,denom);
                    if((y<=0)||(y>g.main_ysize-1))break;
                }   
           }
           if(fabs(xx[3]-xx[4])>2.0) // Loop 'c'
           {   for(x=(int)xx[3]; x<(int)xx[4]; x+=sgn(xx[4]-xx[3]))
               for(y=(int)(m34*x+b34); y!=(int)(m14*x+b14); y+=sgn(m14-m12))
               {   if(k<3) setpixel(x,y,g.maxcolor,XOR);
                   else total+=diagonalscan(sp,x,y,m,b,denom);
                   if((y<=0)||(y>g.main_ysize-1))break;
               }   
           }  
        }
     }
     else                           //---If 1--2 is parallel to 3--4
     {                              // Change k loop to 1..3 if you
                                    // want the area temporarily inverted.
        for(k=1;k<=3;k++)              
        {  if(fabs(xx[1]-xx[2])>1.0)  // Loop 'd'
           { 
             for(x=(int)xx[1]; x<(int)xx[2]; x++)
             for(y=(int)(m13*x+b13); y!=(int)(m12*x+b12); y+=sgn(m12-m13))
             {     if(k<3) setpixel(x,y,g.maxcolor,XOR);
                   else total+= diagonalscan(sp,x,y,m,b,denom);
                   if((y<=0)||(y>g.main_ysize-1))break;
             }
           }     
           if(fabs(xx[2]-xx[3])>1.0)  // Loop 'e'
           {   for(x=(int)xx[2]; x<(int)xx[3]; x++)
               {   for(y=(int)(m13*x+b13); y!=(int)(m24*x+b24); y+=sgn(m12-m13))
                   {  if(k<3) setpixel(x,y,g.maxcolor,XOR);
                      else total+=diagonalscan(sp,x,y,m,b,denom);
                      if((y<=0)||(y>g.main_ysize-1))break;
                   }   
               }
           }
           if(fabs(xx[3]-xx[4])>1.0) // Loop 'f'
           { 
             for(x=(int)xx[3]; x<(int)xx[4]; x+=sgn(xx[4]-xx[3]))
             for(y=(int)(m34*x+b34); y!=(int)(m24*x+b24); y+=sgn(m12-m13))
             {     if(k<3) setpixel(x,y,g.maxcolor,XOR);
                   else total+=diagonalscan(sp,x,y,m,b,denom);
                   if((y<=0)||(y>g.main_ysize-1))break;
             }   
           }  
        }   
     }
     return total;
}
   

//-------------------------------------------------------------------------//
// scan_fixed_width_area    (called by scan_region_calc)                   //
// xx and yy range from 1 to 4, not 0 to 3                                 //
// onlyino = image to read from  (0=calculate automatically)               //
//-------------------------------------------------------------------------//
double scan_fixed_width_area(ScanParams *sp, double xx[], double yy[], 
  int &scancount) 
{
                                              // dx = x increment
                                              // dy = y increment
                                              // m  = slope
                                              // b  = y intercept
                                              // numbers = line segments
  double A,B,C,D,a,m12,dx12,fvalue,total=0.0,fr,fg,fb;
  double d=0.0,dp,dx,dy,ddx,ddy,fx,fy,mp,rx,ry,w,xs,ys;
  int h,hmax,j,k,ix,iy,bpp,ino,ia,ra,ga,ba,ib,rb,gb,ic,rc,gc,bc,
      id,rd,gd,bd,vv,rr,gg,bb;
  int obpp=-1;

    //------------------------------------------------------------------//
    // Fixed width scan. Analyze by brute force.                        //
    // The starting and ending coordinates are the only points guaran-  //
    // teed to coincide with a pixel. Calculate all other points at     //
    // multiples of 1 pixel from each other, read the surrounding 4     //
    // pixels, and calculate the value for that point based on a        //
    // value proportional to how far away the pixel is.                 //
    //------------------------------------------------------------------//

    //---Calculate line between the starting and ending points---//

  dx12 = (double)(xx[2]-xx[1]); if(dx12==0) dx12=1e-6;
  m12 = (yy[2] - yy[1])/dx12; if(m12==0) m12=1e-6;

    //---Length of line between x1,y1 and x2,y2------------------//
  a = (xx[2]-xx[1]) * (xx[2]-xx[1]) + (yy[2]-yy[1]) * (yy[2]-yy[1]);
  d = sqrt(a);
  scancount = (int)d;

    //---X & Y incremental distances along line parallel to 1--2 //
  ddx = sqrt(1.0/(1.0+m12*m12));
  if(xx[1]>xx[2]) ddx *= -1.0;
  ddy = ddx*m12;

    //---Calculate line perpendicular to 1--2, through x1,y1 ----//
  if(fabs(m12)>1e-10)
    mp = -1.0/m12;
  else
    mp = -100000000.0;

  //---Initialize everything-------------------------------------//
  //   For k=-width to +width, calculate the starting and        //
  //   ending coordinates on the perpendicular line.             //
  //   Read the values of the 4 closest pixels to this point.    //
  //   Take a fraction of these pixel values proportional to     //
  //   how close they are.                                       //
  //                                                             //
  //   Pixel boundary is at upper left of pixel A.               //
  //   X,Y coordinate is a non-integer remainder into            //
  //   pixel A = rx,ry. This ensures that the pixels             //
  //   to use are always to the right and below pixel A.         //
  //                                                             //
  //    Contributing pixel   Use                                 //
  //        A                A * (1-rx) * (1-ry)                 //
  //        B               +B * rx * (1-ry)                     // 
  //        C               +C * (1-rx) * ry                     //
  //        D               +D * rx * ry                         //
  //                   Total should be 1.                        //
  //-------------------------------------------------------------//

  double fracx,fracy;
  for(k=0;k<scancount;k++) sp->scan[k]=0;
  ino = sp->ino;
  total = 0;
  if(sp->leavemarked) hmax=1; else hmax=0;
  for(h=0;h<=hmax;h++)
  {
     for(k=0; k<=sp->scanwidth; k++)
     {  w = (double)sp->scanwidth;
        dp = (double)k - floor(w/2.0-0.5);
                                         // 1 2 3 4 5 -> -2 -1 0 -1 2, etc.
                                         // 1 2 3 4   -> -1  0 1  2
        dx = sqrt((dp*dp)/(1.0+mp*mp));  // Incremental distance along perp.
        dx *= sgn(dp);
        dy = mp * dx;       
        xs = xx[1] + dx;                 // Starting coordinates
        ys = yy[1] + dy;
       
       //---Calculate the coords. of each point on this line.-------//
        fx = xs; fy=ys;                  // floating-point real coordinates
  
        for(j=0;j<scancount;j++)
        {   
           rx = modf(fx, &fracx);
           ry = modf(fy, &fracy);
           ix = int(fracx);
           iy = int(fracy);

           if(h==1)
                setpixel(ix,iy,g.maxcolor,SET);
           else switch(sp->wantcolor){
           case 0:          // grayscale mode 0..1 for scanning
                A = pixeldensity(ix,  iy  ,ino, sp->compensate, sp->invert);
                B = pixeldensity(ix+1,iy  ,ino, sp->compensate, sp->invert);
                C = pixeldensity(ix  ,iy+1,ino, sp->compensate, sp->invert);
                D = pixeldensity(ix+1,iy+1,ino, sp->compensate, sp->invert);
                fvalue = A * (1.0-rx) * (1.0-ry)
                       + B * rx * (1.0-ry)
                       + C * (1.0-rx) * ry
                       + D * rx * ry;
     
                total += fvalue;
                sp->scan[j] += fvalue; 
                break;
           case 1:          // color mode for rotating images
                            // w/antialiasing
                ia = readpixelonimage(ix,  iy,  bpp, ino);
                valuetoRGB(ia, ra, ga, ba, bpp);    
                ib = readpixelonimage(ix+1,iy,  bpp, ino);
                valuetoRGB(ib, rb, gb, bb, bpp);    
                ic = readpixelonimage(ix,  iy+1,bpp, ino);
                valuetoRGB(ic, rc, gc, bc, bpp);    
                id = readpixelonimage(ix+1,iy+1,bpp, ino);
                valuetoRGB(id, rd, gd, bd, bpp);    
                fr = (double)(
                       ra * (1.0-rx) * (1.0-ry)
                     + rb * rx * (1.0-ry)
                     + rc * (1.0-rx) * ry
                     + rd * rx * ry );
                fg = (double)(
                       ga * (1.0-rx) * (1.0-ry)
                     + gb * rx * (1.0-ry)
                     + gc * (1.0-rx) * ry
                     + gd * rx * ry );
                fb = (double)(
                       ba * (1.0-rx) * (1.0-ry)
                     + bb * rx * (1.0-ry)
                     + bc * (1.0-rx) * ry
                     + bd * rx * ry );
              
                ////  Source pixel is on a different image, switch to new colormap 
                if(bpp!=obpp && ino>=0 && z[ino].colortype!=GRAY)
                {    memcpy(g.palette, z[ino].palette, 768);
                     obpp = bpp;
                }
                vv = (int)RGBvalue((int)fr,(int)fg,(int)fb,bpp);

                ////  Source pixel is grayscale, convert to color 
                if(ino>=0 && z[ino].colortype==GRAY)
                {    vv=convertpixel(vv, bpp, sp->bpp, 1);
                     vv=graypixel(vv,sp->bpp);
                     bpp=sp->bpp;
                }
                sp->scan[j] = convertpixel(vv, bpp, sp->bpp, 1);
                break;
           case 2:          // color mode for rotating images w/o 
                            // antialiasing
                sp->scan[j] = (double)readpixelonimage(ix,iy,bpp,ino);
                break;
           case 3:          // grayscale mode 0..maxcolor for rotating
                A = (double)readpixelonimage(ix,  iy,  bpp, ino);
                B = (double)readpixelonimage(ix+1,iy,  bpp, ino);
                C = (double)readpixelonimage(ix,  iy+1,bpp, ino);
                D = (double)readpixelonimage(ix+1,iy+1,bpp, ino);
                fvalue = A * (1.0-rx) * (1.0-ry)
                       + B * rx * (1.0-ry)
                       + C * (1.0-rx) * ry
                       + D * rx * ry;
                vv = (int)fvalue; 
                   
                ////  Source pixel is color or different bpp, convert to gray 
                if(bpp!=sp->bpp)
                {     fvalue /= g.maxgray[sp->bpp];
                      rr = (int)fvalue*g.maxred[sp->bpp];
                      gg = (int)fvalue*g.maxgreen[sp->bpp];
                      bb = (int)fvalue*g.maxblue[sp->bpp];
                     
                      ////  Source pixel is on a different image, switch to new colormap 
                      if(bpp!=obpp && ino>=0)
                      {    memcpy(g.palette, z[ino].palette, 768);
                           obpp = bpp;
                      }
                      vv = RGBvalue(rr,gg,bb,sp->bpp);
                }
                sp->scan[j] = vv;
                break;
           case 4:          // color mode for rotating 1 image w/o 
                            // antialiasing
               ino = ci;
               bpp = z[ino].bpp;
               if(between(ix,0,z[ino].xsize-1) && between(iy,0,z[ino].ysize-1))
                   sp->scan[j]=pixelat(z[ino].image[0][iy]+ix*g.off[bpp], bpp);
               break;
           }
           fx+=ddx;
           fy+=ddy; 
        } 
    }
  }
  return total;
}



//-------------------------------------------------------------------------//
// diagonalscan - calculate distance from x,y to line given by slope m     //
//      and y intercept b, read the pixel at x,y, and put its value into   //
//      array 'scan[]', using anti-aliasing to partition portions of the   //
//      pixel value into the appropriate element depending how close it    //
//      is to the calculated position.                                     //
//      This does not take into account pixels at edge of the figure.      //
//      'denom' is a pre-calculated factor 1/(m+1/m) for speed.            //
//-------------------------------------------------------------------------//
double diagonalscan(ScanParams *sp, double x, double y, double m, double b, 
                    double denom)
{ 
        double bperp, d, dx, dy, fval, r, xint, yint;
        int n;
        
        if(m==0) 
           bperp = y + x*1e6;         // Y-intercept of line perpendicular 
        else                          // to y=mx+b, through x,y.
           bperp = y + x/m;
        xint = (bperp - b)*denom;     // Intersection coordinates
        yint = m*xint + b;
        dx = x-xint;                  // X distance point to line
        dy = y-yint;                  // Y distance point to line
        d = sqrt(dx*dx + dy*dy);      // Distance from point to line
        n = int(d);                   // Integer distance point to line
        r = d - (double)n;             // Remainder (determines partitioning)
        g.scancount = max(g.scancount,n);
        fval = pixeldensity((int)x,(int)y,sp->ino,sp->compensate,sp->invert);
                                      // Partition the pixel value
        if(r >= 0.5)
        {   sp->scan[n+1] += (r - 0.5)*fval;
            sp->scan[n]   += (1.5 - r)*fval; 
        }else
        {   sp->scan[n]   += (r + 0.5)*fval;
            if(n>0) sp->scan[n-1] += (0.5 - r)*fval;
        }        
        return(fval);
}        


//-------------------------------------------------------------------//
// scanline - use Bresenham algorithm to measure the pixel value     //
// along the length of a line.                                       //
//-------------------------------------------------------------------//
double scanline(ScanParams *sp, int xs, int ys, int xe, int ye)
{
     int decision, dx, dy, x_sign, y_sign, x, y;
     double total=0.0;
     dx = abs(xe - xs);
     dy = abs(ye - ys);
     if(dx > dy)
     {  if(xs > xe){ swap(xs,xe); swap(ys,ye); }
        if(ye - ys < 0) y_sign = -1; else y_sign = 1;
        for (x=xs,y=ys,decision=0; x<=xe; x++,decision+=dy)
        {   if(decision>=dx){ decision -= dx; y+= y_sign; }
            total += pixeldensity(x,y,sp->ino,sp->compensate,sp->invert);
        } 
     }else
     {  if(ys > ye){ swap(ys,ye); swap(xs,xe); }
        if(xe - xs < 0) x_sign = -1; else x_sign = 1;
        for (x=xs,y=ys,decision=0; y<=ye; y++,decision+=dx)
        {   if(decision>=dy){ decision -= dy; x+=x_sign;  }
            total +=  pixeldensity(x,y,sp->ino,sp->compensate,sp->invert);
        }
     }
     return total;
}


//-------------------------------------------------------------------//
// insidecircle                                                      //
//-------------------------------------------------------------------//
int insidecircle(int i, int j, int x, int y, int d)
{
   double r = (double)(d)/2;
   double r2 = r*r;
   double dist2;
   dist2 = (i-x)*(i-x) + (j-y)*(j-y);
   if(dist2<=r2) return 1;
   return 0;
}


//------------------------------------------------------------------//
// fill_polygon (4 sides only)                                      //
// xx and yy range from 1 to 4, not 0 to 3                          //
//------------------------------------------------------------------//
void fill_polygon(int *xx, int *yy, int color) 
{
   fill_triangle(xx[1], yy[1], xx[4], yy[4], xx[3], yy[3], color);
   fill_triangle(xx[1], yy[1], xx[4], yy[4], xx[2], yy[2], color);
}


//------------------------------------------------------------------//
// curve_densitometry                                               //
//------------------------------------------------------------------//
void curve_densitometry(void)
{
  static int in_curve_dens=0;
  if(in_curve_dens) return;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  static PlotData *pd;
  XYData *data;
  data = new XYData;
  int k, maxpix = 0, aborted=0;
  int helptopic = 66;
  char **ytitle;
  ytitle = new char*[1];
  ytitle[0] = new char[256];
  strcpy(ytitle[0], "Pixel values");
  array<double> xdata(300);
  
  g.getout=0;
  data->x = new int[300];
  data->y = new int[300];
  if(data->x == NULL |data->y == NULL){  message(g.nomemory,ERROR); return; } 
  data->n = 0;
  data->dims      = 2;
  data->ino       = ci;
  data->nmin      = 0;
  data->nmax      = 300;
  data->width     = 0;
  data->type      = 0;  // Bezier
  data->duration  = TEMP;
  data->wantpause = 0;
  data->wantmessage = 1;
  data->win       = 0;  // Calculate automatically
  
  in_curve_dens = 1;
  while(g.getout==0)
  { 
     g.scancount = 0;
     data->n = 0;
     for(k=0;k<300;k++){ data->x[k]=1;  data->y[k]=1; }
     g.getout = 0;
     bezier_curve_start(data, NULL);
     if(!g.getout) 
     {    g.block++;
          while(g.bezier_state==CURVE)
             XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
          g.block = max(0, g.block-1);
          if(g.getout) aborted=1;
     }

     //// bezier_curve_end is called automatically when key is pressed.

     bezier_curve_end(data);
     draw_curve(data, SCAN, ON);               // put in g.scan
     g.bezier_state=NORMAL;
     g.state=NORMAL;

     for(k=0; k<g.scancount; k++)
     {   if(g.scan[k] > maxpix) maxpix = (int)g.scan[k];
     }

     //// To open an updatable graph, declare a PlotData*, call open_graph.
     //// Don't allocate or delete PlotData.
     //// Then call graph_is_open, if !=0 reset pd->n, resize pd->data, copy
     //// the new data, and call redraw_graph. Graph is automatically closed
     //// and all internal data are automatically deleted when user clicks cancel.
     //// Do not access any component of pd until after calling plot().
     
     if(!graph_is_open(pd))
     {   
         pd = plot((char*)"Curve_Densitometry_Results", (char*)"Point Number", 
              ytitle, 
              xdata.data, g.scan, g.scancount, 1, MEASURE, 0, maxpix, NULL, 
              null, null, null, null, null, helptopic, 1.0, 1.0);
         open_graph(pd, ci, CURVE_DENSITOMETRY_GRAPH);
     }else if(!g.getout)
     {   pd->n = g.scancount;
         delete[] pd->data[0];  
         pd->data[0] = new double[pd->n+1];       
         for(k=0; k<pd->n; k++) pd->data[0][k] = g.scan[k];
         redraw_graph(pd);
     }


  }
  g.scancount = 0;
  bezier_curve_end(data);
  delete[] ytitle[0];
  delete[] ytitle;
  delete[] data->y;
  delete[] data->x;
  in_curve_dens = 0;
}
