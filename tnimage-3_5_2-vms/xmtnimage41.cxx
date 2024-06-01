//--------------------------------------------------------------------------//
// xmtnimage41.cc                                                           //
// Graphs                                                                   //
// Latest revision: 12-18-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
const int AUTOSCALE=0;
inline double cosine(double x);
inline double sine(double x);
int in_graphcancelcb = 0;
extern uint bezier_mask;
int list_ok = 1;
int in_baseline = 0;
int in_manual_baseline = 0;

char buttonlabel[18][20] = {
 "Save to file ",
 "Read file    ",
 "Smooth       ",
 "Auto.Baseline", 
 "Manl.Baseline",
 "Capture image",
 "Peak areas   ",
 "BL Smoothness",
 "Scale        ",
 "Multiply     ",
 "Add value    ",
 "Change       ",
 "Bezier change",
 "Maximize     ",
 "Invert       ",
 "Reset        ",
 "Rescale      ",
 "Finished     "}; //<-Must be last!

#define GRAPH_NOOFBUTTONS 18 //// Change this if adding new button
#define GRAPH_SLIDERS 15     //// Change this if adding slider to 3d dialog
#define GRAPH_BUTTONS3D 5    //// Change this if adding button to 3d dialog
#define GRAPH_BOXSIZE 21     //// Vertical size of buttons in pixels
#define GRAPH_BOXWIDTH 100   //// Horizontal size of buttons in pixels
#define GRAPH_LABELWIDTH 30  //// Horizontal size of y axis label in pixels
#define GRAPH_LABELHEIGHT 30 //// Vertical size of y axis label in pixels
#define GRAPH_TOPPOS 1       //// Top of graph window in % of form size
#define GRAPH_BOTPOS 92      //// Bottom of graph window in % of form size
#define GRAPH_LEFTPOS 4      //// Left of graph window in % of form size

//--------------------------------------------------------------------------//
// Plot      -     Motif version                                            //
// Simple bar graph - returns GOTNEW if user converted it to a new image    //
//  calx = 1 if x values are to be calculated using calibratepixel(), 0 if  //
//          some other kind of graph                                        //
//  f = y calibration factor                                                //
//  For 2D data - 1st array index is dimension, 2nd is element.             //
//  'mode' can be MEASURE, CHANGE, SUBTRACT, or BEZIER                      //
//  'title' should be char[FILENAMELENGTH] since it can be changed by user. //
//  'ytitle' is a list of 1 title for each panel on y axis.                 //
//  'draw_callback' is called once every time part of graph is redrawn.     //
//  'point_callback' is called once every time a point is changed.          //
// Returns PlotData struct. Caller can call open_graph() to put pd in global//
//  array so it can be automatically updated. User can close graph any time //
//  so only plotdonecb() should ever call close_graph().                    //
//--------------------------------------------------------------------------//
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, int *pdata, 
   int n, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v), 
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2),
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac, double ydisplayfac)
{  
   int k;
   dims=1;    
   array<double> ddd(n,1);
   for(k=0;k<n;k++) ddd.p[0][k] = (double)pdata[k];
   PlotData *pd;
   pd = plot(title,xtitle,ytitle,xdata,ddd.p,n,dims,mode,ymin,ymax,
            edata,f1cb,f2cb,f3cb,f4cb,f5cb,helptopic,xdisplayfac,ydisplayfac);
   for(k=0;k<n;k++) pdata[k]=(int)ddd.p[0][k];
   return pd;
}
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, double *pdata, 
   int n, int dims, int mode, int ymin, int ymax, 
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2),
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac, double ydisplayfac)
{  
   int k;
   dims=1;    
   array<double> ddd(n,1);
   PlotData *pd;
   for(k=0;k<n;k++) ddd.p[0][k] = pdata[k];
   pd = plot(title,xtitle,ytitle,xdata,ddd.p,n,dims,mode,ymin,ymax,
            edata,f1cb,f2cb,f3cb,f4cb,f5cb,helptopic,xdisplayfac,ydisplayfac);
   for(k=0;k<n;k++) pdata[k]=ddd.p[0][k];
   return pd;
}
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, int **pdata, 
   int n, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2),
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac, double ydisplayfac)
{  
   int j,k;
   array<double> ddd(n+10,dims+2);
   PlotData *pd;
   for(j=0;j<dims;j++) for(k=0;k<n;k++) ddd.p[j][k]=(double)pdata[j][k];
   pd = plot(title,xtitle,ytitle,xdata,ddd.p,n,dims,mode,ymin,ymax,
            edata,f1cb,f2cb,f3cb,f4cb,f5cb,helptopic,xdisplayfac,ydisplayfac);
   for(j=0;j<dims;j++) for(k=0;k<n;k++) pdata[j][k]=(int)ddd.p[j][k];
   return pd;
}
PlotData *plot(char *title, char *xtitle, char **ytitle, 
   double *xdata, 
   double **pdata, 
   int npoints, int dims, int mode, int ymin, int ymax,
   int *edata,
   void (*f1cb)(pcb_struct v),
   void (*f2cb)(void *pd, int n1, int n2),
   void (*f3cb)(void *pd, int n1, int n2),
   void (*f4cb)(void *pd, int n1, int n2),
   void (*f5cb)(void *pd, int n1, int n2),
   int helptopic, double xdisplayfac, double ydisplayfac)
{

   //////////////////////////////////////////////////////////////////////
   // To add a new button:                                             //
   // 1. New button must be second last,                               //
   // 2. Change GRAPH_NOOFBUTTONS                                      //
   // 3. Add label to buttonlabel[] (last button must be 'finished').  //
   // 4. Add new CASE statement in graphbuttoncb()                     //
   // 5. Set the button type to PUSHBUTTON or TOGGLE.                  //
   //////////////////////////////////////////////////////////////////////
   // edata is extra data used if f4cb is not null. Must be static.    //
   //////////////////////////////////////////////////////////////////////

  static Widget graph[10], ylabelwidget[10], xlabelwidget, form, 
         button[GRAPH_NOOFBUTTONS], *w; 
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return NULL; }
  int buttontype[GRAPH_NOOFBUTTONS];
  int button_initial_state[GRAPH_NOOFBUTTONS];
  XmString xms;
  Arg args[100];
  int gsize, top, xsize, ysize = 500, j, k, n, wc=0, x,y,rr,gg,bb;
  xsize = max(350, GRAPH_BOXWIDTH + GRAPH_LABELWIDTH + npoints + 10);

  static PlotData *pd;
  pd = new PlotData;
  static Window *windows, *ylabelwindow, xlabelwindow;
  static clickboxinfo *clickboxdata;
  static  int *markmin, *markmax, *markstart, *markend;
  static  double *scale, *area;
  Widget okaybut, cancbut, helpbut;
  if(helptopic==-1) helptopic=13;


  clickboxdata = new clickboxinfo[GRAPH_NOOFBUTTONS];
  for(k=0;k<GRAPH_NOOFBUTTONS;k++)
  {    clickboxdata[k].title = new char[128]; 
       clickboxdata[k].title[0] = 0; 
       clickboxdata[k].form = NULL; 
       clickboxdata[k].path = NULL; 
       clickboxdata[k].answers = NULL; 
  }


  ////  Button type and initial state (for toggle buttons only)
  for(k=0;k<GRAPH_NOOFBUTTONS;k++)
  {   buttontype[k]=PUSHBUTTON; 
      button_initial_state[k]=False; 
  }
  buttontype[11]=TOGGLEBUTTON;
  button_initial_state[11] = (mode==CHANGE);

  ////  For non-toggle, force click and depress button
  if(mode==BEZIER) clickboxdata[12].button = 12;
  buttontype[12]=TOGGLEBUTTON;

  if(dims>=10) { message("Too many dims in plot()"); return pd; }

  markmin   = new int[dims];           // small enough not to check
  markmax   = new int[dims];
  markstart = new int[dims];
  markend   = new int[dims];
  windows   = new Window[dims];
  ylabelwindow = new Window[dims];
  scale     = new double[dims];
  area      = new double[dims];
  w         = new Widget[100];

 //-------------------graph form--------------------------------------//
  n=0;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++; // Block WM delete button
  XtSetArg(args[n], XmNwidth, xsize); n++;
  XtSetArg(args[n], XmNheight, ysize); n++;
  XtSetArg(args[n], XmNmarginWidth, 1); n++;
  XtSetArg(args[n], XmNtransient, True); n++; // keep on top
  XtSetArg(args[n], XmNresizable, True); n++;
  XtSetArg(args[n], XmNautoUnmanage, False); n++;
  XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple(title)); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  form = w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"GraphForm", args, n);
  XmStringFree(xms);

 //-------------------graph area (at right)---------------------------//
  top = GRAPH_TOPPOS;
  gsize = (GRAPH_BOTPOS-GRAPH_TOPPOS)/dims;
  for(k=0;k<dims;k++)
  {    n=0;
       XtSetArg(args[n], XmNtitle, "Graph"); n++;
       XtSetArg(args[n], XmNwidth, npoints); n++;
       XtSetArg(args[n], XmNmarginHeight, 0); n++;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
       XtSetArg(args[n], XmNleftPosition, 0); n++;            // % of width fromleft
       XtSetArg(args[n], XmNleftOffset, GRAPH_LEFTPOS+GRAPH_BOXWIDTH+GRAPH_LABELWIDTH); n++; // pixels from left
       XtSetArg(args[n], XmNtopPosition, top); n++;           // % of height from top
       XtSetArg(args[n], XmNbottomPosition, top+gsize); n++;  // % of width from top
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE); n++;
       XtSetArg(args[n], XmNbackground, g.bcolor); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       graph[k] = w[wc++] = XmCreateDrawingArea(form, (char*)"graph", args, n);
       XtAddCallback(graph[k], XmNexposeCallback, (XtCBP)graphexposecb,
           (XtP)pd);
       XtAddEventHandler(graph[k], ButtonPressMask, False, (XtEH)graph_bezier_startcb, 
           (XtP)pd);
       XtAddCallback(graph[k], XmNinputCallback, (XtCBP)graphcb, (XtP)pd);
       // Need event handler to catch those wacky mouse movement events.
       XtAddEventHandler(graph[k],g.mouse_mask,False,(XtEH)graphmousecb,(XtP)pd);
       XtManageChild(graph[k]);
 
  //--------------y axis labels---------------------------------------//

       n=0;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopPosition, top); n++;           
       XtSetArg(args[n], XmNbottomPosition, top+gsize); n++;  
       XtSetArg(args[n], XmNleftPosition, 0); n++;  
       XtSetArg(args[n], XmNleftOffset, GRAPH_LEFTPOS+GRAPH_BOXWIDTH); n++;  // pixels from left
       XtSetArg(args[n], XmNrightWidget, graph[k]); n++;   
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       ylabelwidget[k] = w[wc++] = XmCreateDrawingArea(form, (char*)"YLabelWidget", args, n);
       XtAddCallback(ylabelwidget[k],XmNexposeCallback, (XtCBP)graphylabelexposecb, (XtP)pd);
       XtManageChild(ylabelwidget[k]);
       top += gsize;
  }

  //--------------x axis label----------------------------------------//

  n=0;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNmappedWhenManaged, True); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
  XtSetArg(args[n], XmNtopPosition, top); n++;           
  XtSetArg(args[n], XmNbottomPosition, 100); n++;  
  XtSetArg(args[n], XmNbottomWidget, form); n++;  
  XtSetArg(args[n], XmNbottomOffset, GRAPH_LABELHEIGHT); n++;  
  XtSetArg(args[n], XmNleftOffset, GRAPH_LEFTPOS+GRAPH_BOXWIDTH+GRAPH_LABELWIDTH); n++; // pixels from left
  XtSetArg(args[n], XmNleftPosition, 0); n++;  
  XtSetArg(args[n], XmNrightPosition, 100); n++;           
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  xlabelwidget = w[wc++] = XmCreateDrawingArea(form, (char*)"XLabelWidget", args, n);
  XtAddCallback(xlabelwidget,XmNexposeCallback, (XtCBP)graphxlabelexposecb, (XtP)pd);
  XtManageChild(xlabelwidget);

    
  //-----Initialize data----------------------------------------------//

  list_ok = 1;
  rr = g.maxred[g.bitsperpixel]/2;
  gg = g.maxgreen[g.bitsperpixel]/2;
  bb = g.maxblue[g.bitsperpixel]/2;

  //// Make local copy of all arrays in case original goes out of scope
  pd->title   = new char[256]; strcpy(pd->title, title);     // Title of graph
  pd->xtitle  = new char[256]; strcpy(pd->xtitle, xtitle);   // X axis label
  pd->data    = new double*[dims];                           // 'dims' sets of y values
  pd->ytitle  = new char*[dims];                             // 'dims' Y axis labels
  pd->graph   = new Widget[dims];                            // Widget for each graph
  for(k=0;k<dims;k++)           
  {    pd->data[k] = new double[npoints+1];
       for(j=0;j<npoints;j++) pd->data[k][j] = pdata[k][j];
       pd->ytitle[k] = new char[256]; 
       strcpy(pd->ytitle[k], ytitle[k]);                     
       pd->graph[k]  = graph[k];
  }
  pd->xdata   = new double[npoints+1];                       // Calibrated x values
  for(j=0;j<npoints;j++) pd->xdata[j] = xdata[j];

  //// Copy scalar variables 
  pd->gcolor  = RGBvalue(rr,gg,bb, g.bitsperpixel);
  pd->form    = form;              // Form widget
  pd->w       = w;                 // All widgets
  pd->win     = windows;           // Xlib windows for each dimension - must wait until 
                                   //   form widget is managed before filling array.
  pd->ylabelwindow = ylabelwindow; // Xlib windows for y axis label
  pd->dims    = dims;              // No. of graphs in the plot 
  pd->xsize   = xsize;             // Width of entire plot including buttons
                                   // Height of plot
  pd->ysize   = ysize*(GRAPH_BOTPOS-GRAPH_TOPPOS)/100/dims;     
  pd->n       = npoints;           // Current no. of data points
  pd->nmax    = npoints;           // Max. no. of data points
  pd->focus   = 0;                 // Which graph has mouse focus
  pd->xcalib  = 1.0;               // User-specified x axis calibration factor
  pd->ycalib  = 1.0;               // User-specified y axis calibration factor
  pd->scale   = scale;             // Automatic scaling factor
  pd->xscale  = 1.0;               // Not used
  pd->yscale  = 1.0;               // Button-selected y axis scaling factor
  pd->markmin = markmin;           // Min value of selected area
  pd->markmax = markmax;           // Max value of selected area
  pd->markstart = markstart;       // Actual start of mouse dragging
  pd->markend   = markend;         // Actual end of mouse dragging
  pd->helptopic = helptopic;       // Help topic for the graph
  pd->type      = mode;            // 0=get areas, 1=change values when dragging
  pd->otype     = mode;            // original type
  pd->f         = f1cb;            // Callback: what to do when mouse is dragged
  pd->f2        = f2cb;            // Callback: what to do when graph is redrawn
  pd->f3        = f3cb;            // Callback: what to do when click on maximize button
  pd->f4        = f4cb;            // Callback: what to do when click on save
  pd->f5        = f5cb;            // Callback: finished button is clicked
  pd->area      = area;            // Most recently-selected area
  pd->button    = 0;               // Mouse button currently pressed
  pd->ymin      = (double)ymin;    // Minimum observed value 
  pd->ymax      = (double)ymax;    // Maximum observed value 
  pd->llimit    = (double)ymin;    // Minimum allowable value (0=don't care)
  pd->ulimit    = (double)ymax;    // Maximum allowable value (0=don't care)
  pd->reason    = NORMAL;          // Why draw_graph was called
  pd->xdisplayfac = xdisplayfac;   // Factor to multiply text x value by
  pd->ydisplayfac = ydisplayfac;   // Factor to multiply text y value by
  pd->c           = clickboxdata;  // Array of clickbox structs
  pd->gotlist     = 0;             // No list yet
  pd->l           = 0;             // No list
  pd->edata       = edata;         // Extra data, must be NULL unless f4cb specified
  pd->hit         = 0;             // No buttons clicked yet
  pd->but         = button;        // Button widgets
  // pd->formwin is assigned below  
  
  for(k=0;k<dims;k++)
  {    pd->markmin[k]=0;  
       pd->markmax[k]=0; 
       pd->markstart[k]=0; 
       pd->markend[k]=0; 
       pd->scale[k] = 0.0;
       pd->area[k] = 0.0;
  }

  ////  Add buttons on left
  ////  The last button ('finished') is bigger than the others and is 
  ////  left unmanaged. 
  x = 5;
  y = 5;
  for(j=0;j<GRAPH_NOOFBUTTONS;j++)
  {     n = 0;
        XtSetArg(args[n], XmNx, x); n++;
        XtSetArg(args[n], XmNy, y); n++;
        XtSetArg(args[n], XmNwidth, GRAPH_BOXWIDTH-2); n++;  
        XtSetArg(args[n], XmNresizable, False); n++;  
#ifdef MOTIF2
        XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_NONE); n++;
#else
        XtSetArg(args[n], XmNindicatorOn, False); n++;
#endif
        XtSetArg(args[n], XmNtraversalOn, True); n++;
        if(j==GRAPH_NOOFBUTTONS-1)
        {   XtSetArg(args[n], XmNheight, GRAPH_BOXSIZE*2); n++; }
        else
        {   XtSetArg(args[n], XmNheight, GRAPH_BOXSIZE); n++;  }
        XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
        ////  Stop Motif from trying to grab another color if none are available.
        if(g.want_colormaps)  
        {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
             XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
        }
        clickboxdata[j].button = j;
        clickboxdata[j].helptopic = helptopic;
        clickboxdata[j].ptr[10] = pd;
        clickboxdata[j].f1 = null;
        clickboxdata[j].f2 = null;
        clickboxdata[j].f3 = null;     
        clickboxdata[j].f4 = null;     
        clickboxdata[j].f5 = null;     
        clickboxdata[j].f6 = null;     
        clickboxdata[j].f7 = null;     
        clickboxdata[j].f8 = null;     
        clickboxdata[j].maxval = GRAPH_NOOFBUTTONS-1;
        clickboxdata[j].list = (char**)buttonlabel;
        if(buttontype[j]==PUSHBUTTON)
        {  button[j] = w[wc++] = XmCreatePushButton(form,buttonlabel[j],args,n);
           XtManageChild(button[j]);
           XtAddCallback(button[j], XmNactivateCallback, 
                (XtCBP)graphbuttoncb, (XtP)(&clickboxdata[j]));
        }else if(buttontype[j]==TOGGLEBUTTON)
        {  button[j] = w[wc++] = XmCreateToggleButton(form,buttonlabel[j],args,n);
           XtManageChild(button[j]);
           XtAddCallback(button[j], XmNarmCallback, 
                (XtCBP)graphbuttoncb, (XtP)(&clickboxdata[j]));
           XmToggleButtonSetState(button[j],button_initial_state[j],False);
        }
        y += GRAPH_BOXSIZE;
  }

  ////  Let manual baseline button map & unmap the last button
  for(j=0;j<GRAPH_NOOFBUTTONS;j++)
        clickboxdata[j].widget[0] = button[GRAPH_NOOFBUTTONS-1];

  ////  Ok, Cancel, Help buttons & their callbacks
  okaybut = w[wc++] = add_button(form, (char*)"Accept",   1, 1, 100);
  cancbut = w[wc++] = add_button(form, (char*)"Dismiss",115, 6, 100);
  helpbut = w[wc++] = add_button(form, (char*)"Help",   225, 6, 100);

  XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)graphcancelcb, (XtP)pd);
  XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)graphcancelcb, (XtP)pd);
  XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(form, XmNunmapCallback, (XtCBP)graphunmapcb, (XtP)pd);


  XtManageChild(form);
  for(k=0;k<dims;k++)  
  {    windows[k] = XtWindow(graph[k]);        // Window to draw in
       ylabelwindow[k] = XtWindow(ylabelwidget[k]);  // Window for y axis labels
  }
  xlabelwindow = XtWindow(xlabelwidget);       // Window for y axis labels
  pd->xlabelwindow = xlabelwindow;             // Xlib windows for x axis label
  pd->formwin = XtWindow(form);                //  Window for labels
  XtUnmapWidget(button[GRAPH_NOOFBUTTONS-1]);
  pd->wc = wc;
  return pd;
}


//--------------------------------------------------------------------------//
//  graphunmapcb                                                            //
//--------------------------------------------------------------------------//
void graphunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
   g.waiting=max(0,g.waiting-1);
   if(!in_graphcancelcb) graphcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  graphcancelcb                                                           //
//--------------------------------------------------------------------------//
void graphcancelcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;call_data=call_data;
  in_graphcancelcb = 1;
  PlotData *pd = (PlotData*)client_data;

  //// In case user clicks Cancel without clicking Finish
  XYData *xydata;
  if(in_manual_baseline) 
  {   xydata = pd->xydata;  
      manual_baseline_finish(pd, xydata);
  }

  int k;
  for(k=0; k<pd->dims; k++)
       XtRemoveEventHandler(pd->graph[k], g.mouse_mask, False, 
       (XtEH)graphmousecb, (XtP)pd);
  XtUnmanageChild(pd->form);
  for(k=0; k<GRAPH_NOOFBUTTONS; k++) delete[] pd->c[k].title; 
  close_graph(pd);  // Remove graph from auto-update list
  pd->c->done = 1;
  pd->n = 0;
  pd->gotlist = 0;
  delete[] pd->c;
  delete[] pd->area;
  delete[] pd->scale;
  delete[] pd->markstart;
  delete[] pd->markend;
  delete[] pd->markmax;
  delete[] pd->markmin;
  delete[] pd->win;
  delete[] pd->ylabelwindow;
  for(k=0; k<pd->wc; k++) XtDestroyWidget(pd->w[k]);
  delete[] pd->w;
  pd->title[0] = 0;
  delete[] pd->title;
  delete[] pd->xtitle;
  for(k=0;k<pd->dims;k++) delete[] pd->ytitle[k];
  delete[] pd->ytitle;
  for(k=0;k<pd->dims;k++) delete[] pd->data[k];
  delete[] pd->data;
  delete[] pd->xdata;
  delete pd;
  in_graphcancelcb = 0;
  g.getout = 1;  // needed in case a prompt box is open
  g.ignore = 0;  // re-enable area selection
}


//--------------------------------------------------------------------------//
//  draw_graph                                                              //
//  Draw segment of graph from data point n1 to data point n2               //
//  Also calculates area of selected region of graph                        //
//--------------------------------------------------------------------------//
double draw_graph(PlotData *pd, int n1, int n2)
{ 
  if(g.getout) return 0;
  g.inmenu++;
  int focus,k,k2,mark,mmstart,mmend,n,y,ye,win;
  double answer=0.0, scale=0.0, largest;
  n      = pd->n;
  y      = pd->ysize;
  focus  = pd->focus;
  win    = pd->win[focus];
  mmstart= pd->markstart[focus];
  mmend  = pd->markend[focus];
  if(mmstart>mmend) swap(mmstart,mmend);
  if(n1>n2) swap(n1,n2);
  n1     = max(0,min(n,n1     ));
  n2     = max(0,min(n,n2     ));
  mmstart= max(0,min(n,mmstart));
  mmend  = max(0,min(n,mmend  ));
  scale  = pd->scale[focus];

  ////  Automatically rescale graph
  if(!scale || AUTOSCALE)
  {
      largest= findbiggest(pd->data[focus], n);
      if(largest!=0) scale = (double)(pd->ysize)/largest;
      if(scale==0.0) scale = 1e6; 
  }
  pd->scale[focus] = scale;
  
  pd->gcolor = WhitePixel(g.display, g.screen);
  blackbox(n1,0,n2-1,pd->ysize,pd->gcolor,win);
  XSetForeground(g.display, g.image_gc, BlackPixel(g.display, g.screen));
  XSetBackground(g.display, g.image_gc, WhitePixel(g.display, g.screen));

  if(focus < pd->dims) XDrawLine(g.display, win, g.image_gc, n1, y-1, n2, y-1);
  mark=0;
  for(k=n1;k<n2;k++) 
  {   if(k>=mmstart && pd->type==MEASURE) mark=1;
      if(k>=mmend) mark=0;
      ye = pd->ysize - (int)(pd->data[focus][k]*scale);
      k2 = k;
      // if(pd->xdisplayfac != 0) k2 = cint(k / pd->xdisplayfac);
      if(mark)
      {    answer += pd->data[focus][k];
           XDrawLine(g.display, win, g.image_gc, k2, 0, k2, ye);
      }else
      {    XDrawLine(g.display, win, g.image_gc, k2, y, k2, ye);
      }       
  }

  if(pd->type != MEASURE) answer = 0.0;
  pd->area[focus] = answer;        
  ////  Don't call callback if just redrawing graph after expose event
  if(pd->reason==NORMAL) pd->f2(pd,n1,n2); 
  g.inmenu--;
  return answer;
}


//--------------------------------------------------------------------------//
//  raisecb                                                                 //
//--------------------------------------------------------------------------//
void raisecb(Widget w, XtP client_data, XEvent *event)
{
  client_data=client_data; event=event;
  if(g.state != MESSAGE) XRaiseWindow(g.display, XtWindow(w));
}


//--------------------------------------------------------------------------//
//  put_area_in_list                                                        //
//--------------------------------------------------------------------------//
void put_area_in_list(PlotData *pd)
{
  if(!list_ok) return;
  if(!pd) return;
  static char **graphlist_info;
  static char *graphlist_title;
  char temp[256];
  int helptopic = 74;
  listinfo *l = pd->l;
  int focus = pd->focus;
  if(!pd->gotlist)
  {   
      graphlist_title = new char[100];
      strcpy(graphlist_title, "Measured Areas");
      graphlist_info = new char*[10000];
      l = new listinfo;
      l->info   = graphlist_info;
      l->title  = graphlist_title;
      l->count  = 0;
      l->itemstoshow = 2;
      l->firstitem   = 1;
      l->wantsort    = 0;
      l->wantsave    = 1;
      l->helptopic   = helptopic;
      l->allowedit   = 0;
      l->selection   = &g.crud;
      l->width       = 0;
      l->wantfunction = 0;
      l->autoupdate   = 0;
      l->clearbutton  = 1;
      l->highest_erased = 0;
      l->f1 = null;
      l->f2 = null;
      l->f3 = graph_list_finish;
      l->f4 = delete_list;
      l->f5 = null;
      l->f6 = null;
      l->listnumber = 0;
      l->finished = 0;
      list(l);
      pd->gotlist = 1;
      pd->l = l;
      l->ptr[10] = pd;
  }else l = pd->l;
  if(!l) return;
  if(l->finished) return;
  if(in_graphcancelcb) return;
  if(!pd->gotlist) return;
  l->info[l->count] = new char[128];        
  sprintf(temp, "%d %g", l->count+1, pd->area[focus]);       
  strcpy(l->info[l->count], temp);
  l->count++;
  if(l->count < 10000)
     additemtolist(l->widget, l->browser, temp, BOTTOM, 1);         
  else
     message("List is full");
}


//--------------------------------------------------------------------//
// graph_list_finish                                                  //
//--------------------------------------------------------------------//
void graph_list_finish(listinfo *l)
{
   l=l;
   list_ok = 0;
   PlotData *pd = (PlotData *)l->ptr[10];
   if(in_graphcancelcb || !pd || !pd->n) return;
   g.getout = 0;
}

//--------------------------------------------------------------------//
// graphexposecb - callback for graph expose                          //
//--------------------------------------------------------------------//
void graphexposecb(Widget w, XtP client_data, XmACB *call_data)
{
  g.inmenu++;
  w=w;call_data=call_data;
  PlotData *pd = (PlotData*)client_data;
  call_data=call_data;  // keep compiler quiet
  int k,ofocus;
  Window win;
  ofocus = pd->focus;
  win = XtWindow(w);
  for(k=0;k<pd->dims;k++) if(win==pd->win[k]){ pd->focus = k; break; }
  pd->reason=EXPOSE;
  draw_graph(pd, 0, pd->n);
  pd->reason=NORMAL;
  pd->focus = ofocus;
  g.inmenu--;
}  


//--------------------------------------------------------------------//
// graphxlabelexposecb - callback for graph expose x axis label       //
//--------------------------------------------------------------------//
void graphxlabelexposecb(Widget w, XtP client_data, XEvent *event)
{
  g.inmenu++;
  w=w;client_data=client_data, event=event;
  PlotData *pd = (PlotData *)client_data;
  print(pd->xtitle, 15, 10, g.main_fcolor, g.main_bcolor, &g.gc,
      pd->xlabelwindow, 0, 0);
  g.inmenu--;
}


//--------------------------------------------------------------------//
// graphylabelexposecb - callback for graph expose y axis labels      //
// This must be in a callback because it is impossible to draw in a   //
// window until it receives its first expose event.                   //
//--------------------------------------------------------------------//
void graphylabelexposecb(Widget w, XtP client_data, XEvent *event)
{
  g.inmenu++;
  client_data=client_data, event=event;
  PlotData *pd = (PlotData *)client_data;
  int k, panel = 0;
  for(k=0;k<pd->dims;k++) if(XtWindow(w) == pd->ylabelwindow[k]) panel=k;
  print(pd->ytitle[panel], 15, pd->ysize-16, g.main_fcolor, 255, &g.gc,
      pd->ylabelwindow[panel], 0, 1);
  g.inmenu--;
}


//--------------------------------------------------------------------//
// graphmousecb - callback for graph mouse movements                  //
//--------------------------------------------------------------------//
void graphmousecb(Widget w, XtP client_data, XEvent *event)
{  
  w=w;  // keep compiler quiet
  static int obutton = 0, ofocus=-1;
  double yvalue;
  int focus, mode, k, x, y; 
  PlotData *pd = (PlotData *)client_data;

  focus = pd->focus; 
  Window   win = XtWindow(w);
  for(k=0;k<pd->dims;k++) if(win==pd->win[k]){ focus = k; break; }
  pd->focus = focus;

  mode = pd->type;
  x = event->xmotion.x;
  y = event->xmotion.y;
  if(x<0 || x>=pd->n) return;
  static pcb_struct pcb;

  g.inmenu++;
  pd->reason=EXPOSE;  // Stop draw_graph from calling f2
  XSetForeground(g.display, g.gc, g.main_fcolor);
  XSetBackground(g.display, g.gc, g.main_bcolor);

  if(pd->button)
  { 
       if(mode==MEASURE)
       {    pd->markend[focus] = x;
            pd->markmin[focus] = min(pd->markmin[focus],x);
            pd->markmax[focus] = max(pd->markmax[focus],x);
            draw_graph(pd, pd->markmin[focus], pd->markmax[focus]); 
       }
       if(mode==CHANGE && y>=0)
       {    if(pd->scale[focus] > 0.0)
                 yvalue = (double)(pd->ysize - y) / pd->scale[focus];
            else yvalue = (double)(pd->ysize - y); 
            yvalue = min(yvalue, (double)MAXINT/2);
            x = max(0, min(pd->nmax, x));
            pd->data[focus][x] = yvalue;
            pd->data[focus][max(0,x-1)] = yvalue; // make mouse drag 3 pixels wide
            pd->data[focus][min(pd->n-1,x+1)] = yvalue;
            draw_graph(pd, x-1, x+1); 
            pcb.x = x;
            pcb.y = (int)yvalue;
            pcb.focus = focus;
            pd->f(pcb);  // Callback for single point
       }
  }
  obutton = pd->button;
  ofocus = focus;
  print_graph_coordinates(pd,x,y);
  g.inmenu--;
  return;              
} 

//--------------------------------------------------------------------//
//  print_graph_coordinates                                           //
//--------------------------------------------------------------------//
void print_graph_coordinates(PlotData *pd, int x, int y)
{
  y=y;
  int x2 = x;
  double yvalue;
  const int SPACING=14;
  char tempstring2[100];
  char tempstring[256];
  int ystart = 390;
  int focus = pd->focus;
  Window win = pd->formwin;
  g.inmenu++;
  
                  // Print x value
  doubletostring(pd->xdisplayfac * pd->xcalib * (double)x, g.signif, tempstring2);
  sprintf(tempstring,"x=%s                ",tempstring2);
  XDrawImageString(g.display, win, g.gc, 5, ystart ,tempstring,13);
  ystart+=SPACING;
        
                 // Print y value
  if(pd->xdisplayfac != 0) x2 = cint(x * pd->xdisplayfac);
  yvalue = pd->ydisplayfac * pd->ycalib * (double)pd->data[focus][x];
  if(fabs(yvalue) < 1e-100) yvalue = 0.0;
  doubletostring(yvalue, g.signif, tempstring2);
  sprintf(tempstring,"y=%s               ",tempstring2);
  XDrawImageString(g.display, win, g.gc, 5, ystart, tempstring,16);
  ystart+=SPACING;

                 // Print area
  doubletostring(pd->area[focus], g.signif, tempstring2);
  sprintf(tempstring,"Area=%s                           ",tempstring2);
  XDrawImageString(g.display, win, g.gc, 5, ystart, tempstring,26);
  ystart+=SPACING;

                 // Print scale
  doubletostring(pd->scale[focus], g.signif, tempstring2);
  sprintf(tempstring,"Scale=%s            ",tempstring2);
  XDrawImageString(g.display, win, g.gc, 5, ystart, tempstring,17);
  ystart+=SPACING;

                // Focus is also printed in graphcb
  sprintf(tempstring,"Graph=%d           ",pd->focus+1);
  XDrawImageString(g.display, win, g.gc, 5, ystart, tempstring,17);
  ystart+=SPACING;
  g.inmenu--;
  return;              
} 


//--------------------------------------------------------------------//
// graph_bezier_startcb - mouse click on graph for type==bezier       //
//--------------------------------------------------------------------//
void graph_bezier_startcb(Widget w, XtP client_data, XmACB *call_data)
{
  ////  Press the button for them if in bezier mode
  ////  Find current position of main window & image number
  call_data=call_data;
  int k, focus=0;
  Window win;
  PlotData *pd = (PlotData *)client_data;
  win = XtWindow(w);
  int rx,ry,wx,wy; 
  uint keys;
  Window rwin, cwin; 

  if(!in_baseline)
  {   for(k=0;k<pd->dims;k++) if(win==pd->win[k]){ focus = k; break; }
      pd->focus = focus;
      if(pd->otype == BEZIER) 
      {
           //// Press Bezier mode button
           send_buttonpress_event(XtWindow(pd->but[12]), ButtonPress, 
                      10,10, Button1Mask, Button1);
           ////  Find current position of main window & image number
           win = XtWindow(pd->graph[focus]);
           XQueryPointer(g.display,win,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);

           //// Set first control point so mouse click is not lost
           send_buttonpress_event(win, ButtonPress, rx, ry,
                      Button1Mask, Button1);
      }
  }
}


//--------------------------------------------------------------------//
// graphcb - callback for graph input                                 //
//--------------------------------------------------------------------//
void graphcb(Widget w, XtP client_data, XmACB *call_data)
{  
  w=w;
  static int ofocus=-1,ox=-1;
  XmDrawingAreaCallbackStruct *ptr;
  XEvent event;  

  int focus,x,y;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  if(ptr==NULL) return;
  PlotData *pd = (PlotData *)client_data;

  g.inmenu++;
  event = *(ptr->event);
  focus = pd->focus;
  x = (int)event.xbutton.x;
  y = (int)event.xbutton.y;

   // All X events except MotionNotify in the graph window are handled here.

  switch(ptr->event->type)
  {    case ButtonPress: 
            pd->button = (int)event.xbutton.button;
            print_graph_coordinates(pd,x,y);
            pd->markstart[focus] = x;
            pd->markend[focus]   = x;

            // Unmark old marked area
            draw_graph(pd, pd->markmin[focus], pd->markmax[focus]); 
            pd->markmin[focus] = x;
            pd->markmax[focus] = x;
            ox=x;
            break;
       case ButtonRelease: 
            pd->button = 0;
            pd->markmin[focus] = pd->n;
            pd->markmax[focus] = 0;
            ////  If only changing focus, don't call callback
            if(focus==ofocus || x!=ox) pd->f2(pd,0,pd->n);   
            ofocus = focus;
            if(pd->type==MEASURE) put_area_in_list(pd);
            break;
  }  
  g.inmenu--;
  return;
}  


//--------------------------------------------------------------------//
// graphbuttoncb - callback for buttons at left of graph              //
//--------------------------------------------------------------------//
void graphbuttoncb(Widget w, XtP client_data, XmACB *call_data)
{  
  static double fvalue=1.0, addvalue=0;
  static int blcount = 200;
  static XYData data;    // Data for manual baseline
  static int prevbutton=-1;
  call_data=call_data;   // keep compiler quiet
  int ino, status=OK;
  Window win = XtWindow(w);
  clickboxinfo *c = (clickboxinfo *)client_data;
  PlotData *pd = (PlotData *)c->ptr[10];
  static pcb_struct v;
  int button = c->button;
  int nn = pd->n;
  int focus = pd->focus;
  int j, k, y, yy;
  int oignore = g.ignore;
  g.ignore = 0;

  g.inmenu++;
  double biggest=0, datamax, scale=1.0;
  v.focus=focus;
  pd->xydata = &data;
  pd->v = &v;
  pd->reason=NORMAL;
  for(k=0;k<nn;k++) if(pd->data[focus][k] > biggest) biggest=pd->data[focus][k];
  if(pd->f2 != (void (*)(void *, int, int)) null) pd->usef2=1; else pd->usef2=0;
  if(g.diagnose) printf("graph button %d clicked\n",button);
  switch(button)
  {    case 0:  
                if(pd->f4 == (void (*)(void *, int, int)) null ||
                   pd->f4 == NULL)
                   save_scan(pd->title,pd->data[focus],nn,0); // Save to disk
                else
                   pd->f4(pd,nn,focus);                       // Caller-supplied function
                break;
       case 1:  read_data(pd->title,pd->data[focus],nn);      // Read from disk
                if(g.getout) break;
                draw_graph(pd,0,nn);
                break;
       case 2:  smooth(pd->data[focus],nn,15);    // Smooth
                if(g.getout) break;
                draw_graph(pd,0,nn);
                if(pd->usef2) pd->f2(pd,0,nn);   
                else for(k=0;k<nn;k++)
                {  v.x=k;
                   v.y=(int)pd->data[focus][k];
                   pd->f(v);
                }
                pd->f5(pd,0,nn);
                break;
       case 3:                                    // Auto baseline
                baseline(pd, blcount);
                if(g.getout) break;
                draw_graph(pd,0,nn);
                if(pd->usef2) pd->f2(pd,0,nn);   
                else for(k=0;k<nn;k++)
                {  v.x=k; 
                   v.y=(int)pd->data[focus][k];
                   pd->f(v);
                }
                break;
       case 4:                                    // Manual baseline
                XtMapWidget(c->widget[0]);
                pd->type = SUBTRACT;
                manual_baseline(pd, &data);
                pd->type = pd->otype;
                break;
       case 5:                                    // Capture image
                if(newimage("Image", 0, 0, nn, pd->ysize, 8, GRAY, 1,
                  g.want_shell, 1, PERM, 1, g.window_border,0)==OK) 
                {    status=GOTNEW;
                     ino = ci;
                     win = z[ino].win;
                     g.inmenu--;
                     scale  = pd->scale[focus];
                     for(k=0;k<nn;k++)
                     {   y = cint(scale * (double)(pd->data[focus][k]));
                         for(j=0;j<y;j++)
                         {
                             yy = min(pd->ysize-1, max(0, pd->ysize-j));
                             z[ino].image[0][yy][k]= 0;
                         }
                         for(j=y;j<pd->ysize;j++)
                         {
                             yy = min(pd->ysize-1, max(0, pd->ysize-j));
                             z[ino].image[0][yy][k]= 255;
                         }
                     }
                     g.inmenu++;
                     z[ino].touched=1;
                     rebuild_display(ino);
                     g.getout = 0;
                }
                break;
       case 6:  findpeaks(pd);
                break;
       case 7:  clickbox("Baseline smoothness",0, &blcount, 1, 1000, null, 
                          null, null, NULL, NULL, 0);
                break;
       case 8:  clickbox("Y Scale", 13, &pd->yscale, 0.0, 10.0, null, 
                          cset, null, pd, NULL, 0);
                break;
       case 9:  clickbox("Factor to multiply by", 12, &fvalue, 0.0, 10.0, null, 
                          cset, null, pd, NULL, 0);
                pd->f5(pd,0,nn);
                break;                                     
       case 10:                                   // Add
                clickbox("Value to add", 11, &addvalue, -biggest, biggest, null, 
                          cset, null, pd, NULL, 0);
                pd->f5(pd,0,nn);
                break;                                     
       case 11: if(pd->type == MEASURE) pd->type = CHANGE; 
                else pd->type = MEASURE;
                break;
       case 12:                                   // Bezier change
                data.duration = NOEND;  // don't allow keypress to end bezier, user must click Finish
                XtMapWidget(c->widget[0]);
                pd->type = CHANGE;
                manual_baseline(pd, &data);
                if(g.getout) break;
                draw_graph(pd,0,nn);
                if(pd->usef2) pd->f2(pd,0,nn);   
                else for(k=0;k<nn;k++)
                {  v.x = k;
                   v.y = (int)pd->data[focus][k];
                   pd->f(v);
                }   
                pd->type = pd->otype;
                break;
       case 13:                                   // Maximize
                pd->f3(pd,0,nn);
                if(pd->ymax != pd->ymin)
                   scale = (pd->ulimit - pd->llimit) / (pd->ymax - pd->ymin);
                else scale = 1.0;
                if(pd->usef2) pd->f2(pd,0,nn);   
                else for(k=0;k<nn;k++)
                {  v.x = k;
                   pd->data[focus][k] = scale * (pd->data[focus][k] - pd->ymin);
                   v.y = (int)(pd->data[focus][k]);
                   pd->f(v);
                }
                draw_graph(pd,0,nn);
                break;
       case 14:                                   // Invert
                pd->f3(pd,0,nn);
                datamax=0.0;
                if(pd->llimit == pd->ulimit) datamax = biggest;
                else datamax = pd->ulimit;
                for(k=0;k<nn;k++)
                {  v.x = k;
                   pd->data[focus][k] = max(pd->llimit,min(pd->ulimit,(datamax - pd->data[focus][k])));
                   v.y = (int)(pd->data[focus][k]);
                   if(!pd->usef2) pd->f(v);
                }   
                pd->f5(pd,0,nn);
                draw_graph(pd,0,nn);   
                break;
       case 15:                                   // Reset
                for(k=0;k<nn;k++) pd->data[pd->focus][k] =k;
                pd->reason=EXPOSE;
                draw_graph(pd, 0, pd->n);
                pd->reason=NORMAL;
                break;
       case 16:                                   // Rescale
                pd->scale[focus]=0;
                draw_graph(pd,0,nn);              // Reset graph
                break;
       default: if(g.bezier_state != NORMAL)      // Last button ("Finished")
                {
                    if(prevbutton==4) pd->type = SUBTRACT;
                    if(prevbutton==12) pd->type = CHANGE;
                    manual_baseline_finish(pd, &data);
                }
                XtUnmapWidget(w);                 // Get rid of the finish button
                g.bezier_state = CURVE;           // Set bezier indicator on, still may be in densitometry
                pd->type = pd->otype;
                break;
                 
  }
  pd->status = status;
  prevbutton = button;
  g.getout=0;
  g.ignore = oignore;
  g.inmenu--;
  return;
}  


//--------------------------------------------------------------------------//
// graph_adjust - called by clickbox in graphbuttoncb                       //
//--------------------------------------------------------------------------//
void graph_adjust(clickboxinfo *c)
{  
  int k;
  PlotData *pd = (PlotData*)c->client_data;
  draw_graph(pd, 0, pd->n);
  int oignore = g.ignore;
  g.ignore = 0;
  if(pd->usef2) 
      pd->f2(pd, 0, pd->n);   
  else for(k=0; k<pd->n; k++)
  {   pd->v->x = k;
      pd->v->y = (int)pd->data[pd->focus][k];
      pd->f(*pd->v);
  }   
  g.ignore = oignore;
}



//--------------------------------------------------------------------------//
// findbiggest                                                              //
// Return the largest element in a 1-dim array of length n                  //
//--------------------------------------------------------------------------//
double findbiggest(double* pdata, int n)
{ 
   int k;
   double biggest=0;
   for(k=0;k<n;k++) if(pdata[k] > biggest) biggest=pdata[k]; 
   if(biggest<=0.0) biggest=1.0;
   return(biggest);
}


//--------------------------------------------------------------------------//
//  baseline                                                                //
//  Automatically subtracts baseline from data set of count points          //
//--------------------------------------------------------------------------//
void baseline(PlotData *pd, int iterations)
{  
   int j, k, count;
   count       = pd->n;
   int ysize   = pd->ysize;
   int focus   = pd->focus;
   Window win  = pd->win[focus];
   double scale = pd->scale[focus];
   g.inmenu++;
   printstatus(CALCULATING);
   double *tdata;
   double *pdata = pd->data[pd->focus];
   tdata = new double[count];
   for(k=0;k<count;k++) tdata[k] = pdata[k];
   for(j=0;j<iterations;j++)
   {  smooth(tdata,count,21);
      for(k=0;k<count;k++)
      {   if(tdata[k]>pdata[k]) tdata[k]=pdata[k];
          if(tdata[k]<0) tdata[k]=0;
      }    
   }
   smooth(tdata,count,15);
   printstatus(NORMAL);
   for(k=0;k<count;k++)
   { 
       setpixel(k, (int)(  ysize-tdata[k]*scale), 0, SET, win, -1);
       setpixel(k, (int)(1+ysize-tdata[k]*scale), 0, SET, win, -1);
       setpixel(k, (int)(2+ysize-tdata[k]*scale), 0, SET, win, -1);    
   }
   sleep(1);
   for(k=1;k<count;k++)
   {   pdata[k] -= tdata[k];
       if(pdata[k]<0) pdata[k]=0;
   }
   pdata[0]=pdata[1];
   delete[] tdata;
   g.inmenu--;
}


//--------------------------------------------------------------------------//
//  manual_baseline                                                         //
//--------------------------------------------------------------------------//
void manual_baseline(PlotData *pd, XYData *data)
{  
  const int POINTS = 1000;   // No. of control points user can select
  int k;    
  Widget widget;
  int focus = pd->focus;
  data->x = new int[POINTS];
  data->y = new int[POINTS];
  data->v = NULL;
  data->n = 0;
  data->nmin      = 0;
  data->nmax      = POINTS;
  data->width     = 0;
  data->type      = 0;
  if(pd->otype == BEZIER)
     data->duration  = NOEND;
  else
     data->duration  = TEMP;
  data->wantpause = 0;
  data->wantmessage = 1;
  data->order     = 0;
  data->win       = pd->win[focus]; 
  g.inmenu++;
  in_baseline = 1;

  //// Replace graphcb and graphmousecb callbacks for graph windows 
  //// with beziercb

  for(k=1;k<pd->n;k++) g.highest[k+g.main_xpos] = pd->ysize;
  
  for(k=0;k<pd->dims;k++)
  {     widget = XtWindowToWidget(g.display, pd->win[k]);
        XtRemoveEventHandler(widget, g.mouse_mask,False,(XtEH)graphmousecb,(XtP)pd);
        XtAddEventHandler(widget,bezier_mask,False,(XtEH)beziercb,(XtP)data); 
        XtRemoveCallback(widget, XmNinputCallback,(XtCBP)graphcb, (XtP)pd);
        XtAddCallback(widget, XmNinputCallback,(XtCBP)beziercb, (XtP)data);
  }

  //// Wait around for something to happen
  
  XSetForeground(g.display, g.image_gc, g.maxcolor);
  g.bezier_state = CURVE;
  in_manual_baseline = 1;
}


//--------------------------------------------------------------------------//
//  manual_baseline_finish                                                  //
//--------------------------------------------------------------------------//
void manual_baseline_finish(PlotData *pd, XYData *data)
{
  int k;
  int focus = pd->focus;
  double dy=0.0, y=0.0;   
  int nn = pd->n, x1,x2;
  Widget widget;
  pcb_struct *v = pd->v;
  in_manual_baseline = 0;

  if(g.getout){ delete[]data->x; delete[]data->y; g.inmenu--; return; }

  ////  Turn off Bezier button 
  XmToggleButtonSetState(pd->but[12], 0, False);

  //// Restore original callbacks

  for(k=0;k<pd->dims;k++)
  {     widget = XtWindowToWidget(g.display, pd->win[k]);
        XtRemoveEventHandler(widget,bezier_mask,False,(XtEH)beziercb,(XtP)data);
        XtAddEventHandler(widget, g.mouse_mask,False,(XtEH)graphmousecb,(XtP)pd);
        XtRemoveCallback(widget, XmNinputCallback, (XtCBP)beziercb, (XtP)data);
        XtAddCallback(widget, XmNinputCallback, (XtCBP)graphcb, (XtP)pd);
  }
  g.bezier_state = NORMAL;

  //// Subtract the baseline or substitute the new number

  x1 = max(0, min(pd->nmax-1, data->x[0]));
  x2 = max(0, min(pd->nmax-1, data->x[data->n-1]));

  if(pd->type==SUBTRACT)
  {   
      for(k=1;k<pd->n;k++)
      {   if(pd->scale[focus] != 0.0) 
              dy = (double)(pd->ysize - g.highest[k+g.main_xpos]) / pd->scale[focus];
          pd->data[focus][k] -= dy;
          if(pd->data[focus][k] < 0.0) pd->data[focus][k] = 0.0;

      }
  }else if(pd->type==CHANGE)
  {  
     for(k=x1; k<=x2; k++)
     {    y = (double)(pd->ysize - g.highest[k+g.main_xpos]) / pd->scale[focus];
          pd->data[focus][k] = y;
     }
  }
  draw_graph(pd,0,data->n);
  delete[] data->y;
  delete[] data->x;
  g.inmenu--;

  draw_graph(pd,0,nn);

  int oignore = g.ignore;
  g.ignore = 0;
  if(pd->usef2) 
        pd->f2(pd,0,nn);   
  else for(k=0;k<nn;k++)
  {     v->x=k;
        v->y=(int)pd->data[focus][k];
        pd->f(*v);
  }
  pd->f5(pd,0,nn);   
  g.ignore = oignore;
  pd->type = MEASURE;
  in_baseline = 0;
  g.state = NORMAL;
}


//--------------------------------------------------------------------------//
// findpeaks                                                                //
// Calculate & display peaks                                                //
//--------------------------------------------------------------------------//
void findpeaks(PlotData *pd)
{
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }

   static listinfo *l;
   static char *listtitle;
   static char **info;                                    

   int count, end, focus, j, k, osign=0, peak=0, sign=0, size;
   double *tdata, *dy, *d2y, *area, *yy;
   int *xafter, *xx;
   double dx=0.0;
   char tempstring[100]; 
   focus = pd->focus;
   count = pd->n;   
   tdata = new double[count+10];              // Temporary smoothed data
   if(tdata==NULL){ message(g.nomemory,ERROR); return; }
   dy  = new double[count+10];                // 1st deriv               
   d2y = new double[count+10];                // 2nd deriv
   xx  = new int[count+10];
   yy  = new double[count+10];

   xx[0]=0;
   yy[0]=pd->data[focus][0];
   
   for(k=0;k<count;k++) tdata[k] = pd->data[focus][k];
   smooth(tdata, count, 3);

   ////  Count the number of peaks.
                                             // Take deriv.
   for(k=1;k<count;k++) dy[k] = tdata[k] - tdata[k-1];
   for(k=1;k<count;k++) d2y[k] = dy[k] - dy[k-1];
   smooth(dy, count, 5);
   smooth(d2y, count, 5);
   for(k=2;k<count-2;k++)
   {                                         // If a maximum
       if(sgn(dy[k+1])== 1) sign=1;  
       if(sgn(dy[k+1])==-1) sign=-1;  
       if( sign!=osign && sgn(d2y[k])==-1 && k>3 )
       {   peak++;
           if(peak>=200){ message("Too many peaks",ERROR); break; }
           xx[peak] = k;
           yy[peak] = pd->data[focus][k];    // Use original data for y at max.
           k++;
       }    
       osign = sign;
   }
               
   ////  Find start, end, maximum, and area of each peak
 
   area = new double[peak+10];               // Peak areas
   xafter  = new int[peak+10];               // x position of end of peak

   xafter[0] = 0;
   for(k=1; k<=peak; k++)
   {   area[k]=0;
       osign=1;

       ////  Measure left half of peak, stop at end of previous peak

       for(j=xx[k]; j>xx[k-1]; j--)
       {   sign = sgn(dy[j]);  
           dx = fabs(pd->xdata[j+1] - pd->xdata[j]);
           area[k] += dx * tdata[j];
           if(xx[k]==0)break;    
           if(sign!=osign && j<xx[k]-3)break;
       }                                     // At this point start of peak=j    
       osign=-1;
       if(k<peak) end=xx[k+1]; else end=count;

       ////  Measure right half of peak, stop at start of next peak

       for(j=xx[k]+1; j<end; j++)
       {   sign = sgn(dy[j]); 
           dx = fabs(pd->xdata[j+1] - pd->xdata[j]);
           area[k] += dx * tdata[j];
           xafter[k] = j;
           if(xx[k]==0) break;    
           if(sign!=osign && j>xx[k]+3)break;
       }                                    // At this point end of peak=xafter[k]
   }   

   ////  Put results in list box

   info = new char*[peak+10];
   info[0] = new char[100];                       
   strcpy(info[0],"Peak#  x      y");
   for(k=0;k<13;k++) strcat(info[0]," ");
   strcat(info[0], "Area");
   if(peak)
   { for(k=1; k<=peak; k++)                                
     {   info[k] = new char[100];                       
         itoa(k,tempstring,10);
         strcpy(info[k],tempstring);
         for(j=strlen(info[k]);j<6;j++)strcat(info[k]," ");
 
         itoa(xx[k],tempstring,10);
         strcat(info[k],tempstring);
         for(j=strlen(info[k]);j<14;j++)strcat(info[k]," ");
        
         doubletostring(yy[k], g.signif, tempstring);
         strcat(info[k],tempstring);       
         for(j=strlen(info[k]); j<22+g.signif; j++) strcat(info[k]," ");

         doubletostring(area[k], g.signif, tempstring);
         strcat(info[k], tempstring);
     }                                                  
   }
   size=max(5,min(peak+1,30)); 
   
   listtitle = new char[100];
   strcpy(listtitle, "Peaks");                  
   l = new listinfo;
   l->title      = listtitle;
   l->info       = info;
   l->count      = peak+1;
   l->itemstoshow= size;
   l->firstitem  = 0;
   l->wantsort   = 0;
   l->wantsave   = 1;
   l->helptopic  = 0;
   l->selection  = &g.crud;
   l->allowedit  = 1;
   l->edittitle  = new char[100];
   strcpy(l->edittitle, "Edit item");
   l->editrow    = 0;
   l->editcol    = 0;
   l->maxstringsize = 99;
   l->width      = 0;
   l->transient   = 1;
   l->maxstringsize = 99;
   l->wantfunction = 0;
   l->autoupdate   = 0;
   l->clearbutton  = 0;
   l->highest_erased = 0;
   l->iptr[0] = xafter;
   l->iptr[1] = xx;
   l->dptr[0] = area;
   l->dptr[1] = tdata;
   l->dptr[2] = dy;     
   l->dptr[3] = d2y;
   l->dptr[4] = yy;
   l->f1 = null;
   l->f2 = null;
   l->f3 = findpeaksfinish;
   l->f4 = delete_list;
   l->f5 = null;
   l->f6 = null;
   l->listnumber = 0;
   list(l);
}   


//--------------------------------------------------------------------------//
//  findpeakfinish                                                          //
//--------------------------------------------------------------------------//
void findpeaksfinish(listinfo *l)
{
  delete[] l->iptr[0]; // xafter
  delete[] l->iptr[1]; // xx
  delete[] l->dptr[0]; // area
  delete[] l->dptr[1]; // tdata
  delete[] l->dptr[2]; // dy
  delete[] l->dptr[3]; // d2y
  delete[] l->dptr[4]; // yy
}


//--------------------------------------------------------------------------//
//  smooth                                                                  //
//  Smooth dataset with 'width'-point smoothing                             //
//--------------------------------------------------------------------------//
void smooth(int* pdata, int count, int width)
{  int k;
   double* ddd;
   ddd = new double[count+10]; 
   if(ddd==NULL){ message(g.nomemory,ERROR); return; }
   for(k=0;k<count;k++) ddd[k] = (double)pdata[k];
   smooth(ddd,count,width);
   for(k=0;k<count;k++) pdata[k] = (int)ddd[k];
   delete[] ddd;
}
void smooth(double* pdata,int count,int width)
{
    int j,k,tot,w;
    static int gau3[ 4] = {  1, 2,  1                     };
    static int gau5[ 6] = {  1, 4,  6,  4,   1            };
    static int gau7[ 8] = {  1, 6, 15, 20,  15,   6,   1  };
    static int gau9[10] = {  1, 8, 28, 56,  70,  56,  28,   8,   1         };
    static int gau11[12]= {  1,10, 45,120, 210, 252, 210, 120,  45,  10, 1 };
    static int gau15[16]= {  1,14, 91,364,1001,2002,3003,3432,3003,2002,1001,
                           364,91, 14,  1 }; 
    static int gau19[20]= {  1,18,153,816,3060,8568,18564,31824,43758,48620,
                           43758,31824,18564,8568,3060,816,153,18,1 };
    static int gau21[22]= {  1,20,190, 1140, 4845, 15504,  38760, 77520, 
                            125970, 167960, 184756, 167960, 125970, 77520,
                             38760,  15504,   4845,   1140, 190, 20, 1 }; 
    double testval;
    double *tdata;
    tdata = new double[count+10];
    if(tdata==NULL){ message(g.nomemory,ERROR); return; }
    for(k=0;k<count;k++) tdata[k]=pdata[k];
    switch(width)
    {    case 3: { w=1; tot=4;  break;  }
         case 5: { w=2; tot=16; break;  }
         case 7: { w=3; tot=64; break;  }
         case 9:  { w=4; tot=256;     break; }
         case 11: { w=5; tot=1024;    break; }
         case 15: { w=7; tot=16384;   break; }
         case 19: { w=9; tot=262144;  break; }
         case 21: { w=10;tot=1048576; break; }
         default: { message("Error", ERROR); delete[] tdata; return; }
    }
    for(k=0;k<count;k++)
    {  tdata[k]=0;
       for(j=-w;j<=w;j++)
       {  if(k+j<0) testval = pdata[0];
          else if(k+j>=count) testval = pdata[count-1];
          else testval = pdata[k+j];
          switch(width)
          {    case 3: { tdata[k] += testval*gau3[w+j]; break; }
               case 5: { tdata[k] += testval*gau5[w+j]; break; }
               case 7: { tdata[k] += testval*gau7[w+j]; break; }
               case 9: { tdata[k] += testval*gau9[w+j]; break; }
               case 11:{ tdata[k] += testval*gau11[w+j]; break; }
               case 15:{ tdata[k] += testval*gau15[w+j]; break; }
               case 19:{ tdata[k] += testval*gau19[w+j]; break; }
               case 21:{ tdata[k] += testval*gau21[w+j]; break; }
          }
       }
       if(tot!=0) tdata[k] /= tot;
    }      
    for(k=0;k<count;k++) pdata[k]=tdata[k];
    delete[] tdata;
    return;
}


//--------------------------------------------------------------------------//
//  redraw_graph                                                            //
//--------------------------------------------------------------------------//
void redraw_graph(PlotData *pd)
{
  int k;
  if(pd==NULL) return; 
  int xsize = max(350, GRAPH_BOXWIDTH + GRAPH_LABELWIDTH + pd->n + 10);
  pd->xsize = xsize;
  ////  Resize graph widget
  for(k=0; k<pd->dims; k++) XtVaSetValues(pd->graph[k], XmNwidth, pd->n, NULL); 
  draw_graph(pd, 0, pd->n);
}


//--------------------------------------------------------------------------//
// update_graph                                                             //
//--------------------------------------------------------------------------//
void update_graph(int graphno)
{
  if(g.graph[graphno].type == NONE) return;
  Widget form = g.graph[graphno].pd->form;
  if(form && XtIsManaged(form)) XRaiseWindow(g.display, XtWindow(XtParent(form)));
  switch(g.graph[graphno].type)
  {   case HISTOGRAM_GRAPH: histogram(g.graph[graphno]); break;
      default: break;// Functionality to be added here someday
  }
}


//--------------------------------------------------------------------------//
// close_graph - remove graph from global update list                       //
//--------------------------------------------------------------------------//
void close_graph(PlotData *pd)
{ 
  int j,k;
  for(k=0; k<g.graphcount; k++)
  {   if(g.graph[k].pd == pd)
      {   g.graph[k].type = NONE;
          g.graph[k].pd = NULL;
          for(j=k; j<g.graphcount-1; j++)
             g.graph[j] = g.graph[j+1];
          g.graphcount--;
          break;
      }
  }
}


//--------------------------------------------------------------------------//
// open_graph                                                               //
//--------------------------------------------------------------------------//
Graph open_graph(PlotData *pd, int ino, int type)
{ 
   g.graph[g.graphcount].type = type;
   g.graph[g.graphcount].pd = pd;
   g.graph[g.graphcount].ino = ino;
   g.graphcount++;
   return g.graph[g.graphcount];
}


//--------------------------------------------------------------------------//
// graph_is_open                                                            //
//--------------------------------------------------------------------------//
int graph_is_open(PlotData *pd)
{ 
  int k;
  for(k=0; k<g.graphcount; k++) if(g.graph[k].pd == pd) return 1;
  return 0;
}

