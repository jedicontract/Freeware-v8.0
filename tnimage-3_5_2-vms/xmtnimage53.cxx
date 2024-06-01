//--------------------------------------------------------------------------//
// xmtnimage53.cc                                                           //
// 3d graphs                                                                //
// Latest revision: 04-01-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
const int AUTOSCALE=0;
inline double cosine(double x);
inline double sine(double x);
int in_plot3ddonecb = 0;

#define SLIDERS 15     //// Change this if adding slider to 3d dialog
#define BUTTONS3D 5    //// Change this if adding button to 3d dialog

static double sinss[360], sindd[360];
static double coscc[360], cosdd[360];
static int sinhit=0;
static int coshit=0;


//--------------------------------------------------------------------------//
// sine  - x is in degrees                                                  //
//--------------------------------------------------------------------------//
inline double sine(double x)
{ 
    int k;
    int intx;
    double answer,rem;
    if(!sinhit)
    {  sinhit=1;
       for(k=0;k<360;k++) sinss[k] = sin(k*0.01745329252);
       sindd[0] = 0;
       for(k=1;k<360;k++) sindd[k] = sinss[k] - sinss[k-1];
    }
    intx = int(x);
    rem = x - (double)intx;
    answer = sinss[intx] + rem*sindd[intx];
    return(answer);
}


//--------------------------------------------------------------------------//
// cosine  - x is in degrees                                                //
//--------------------------------------------------------------------------//
inline double cosine(double x)
{ 
    int k;
    int intx;
    double answer,rem;
    if(!coshit)
    {  coshit=1;
       for(k=0;k<360;k++) coscc[k] = cos(k*0.01745329252);
       cosdd[0] = 0;
       for(k=1;k<360;k++) cosdd[k] = coscc[k] - coscc[k-1];
    }
    intx = int(x);
    rem = x - (double)intx;
    answer = coscc[intx] + rem*cosdd[intx];
    return(answer);
}



//--------------------------------------------------------------------------//
//  plot3d                                                                  //
//--------------------------------------------------------------------------//
void plot3d(int ino) 
{
   const int LLABEL=12;
   const int RLABEL=87;
   int wc=0;
   Widget okaybut, cancbut, helpbut;
   Arg args[100];
   int k, n, rw=200;
   if(ino<=0){ message("No images"); return; }
   char tempstring[1024];
   XmString xms;
   Widget form; 
   char title[100];
   int helptopic=0;
   double a,b,c, xscale,yscale,zscale,xcenter,ycenter,zcenter,xtrans,ytrans,ztrans;
   int colormode = 0;
   int granularity = 4;

   static p3d *p;
   p = new p3d;
   
   a=330;   b=10;   c=20;
   xscale  = yscale  = zscale  = 1;
   xcenter = ycenter = zcenter = 0;
   xtrans  = ytrans  = ztrans  = 0;

   if(z[ino].colortype==GRAPH)
   {   if(z[ino].gparam[0]<=0){ message("Original image is no longer present\nCan't redraw graph!"); return; }
       switchto(ino);           // Set ci to The Graph ****ino and ci change here***
       ino = z[ci].gparam[0];   // Set ino to original image being graphed
                                // Restore graph params 
       for(k=0;k<20;k++) p->answer[k] = z[ci].gparam[k];
       for(k=0;k<20;k++) p->buttonstate[k] = z[ci].gbutton[k];
   }else 
   {   
       //// Keep scrollbars off until image_resize bug is fixed.
       if(newimage("3D plot", 200, 200, cint(1.5*z[ino].xsize), cint(1.5*z[ino].ysize), 
           g.bitsperpixel, GRAPH, 1, g.want_shell, 0, PERM, 0, 1, 0)!=OK) 
       {   message(g.nomemory); 
           delete[] p;
           return;
       }
       p->answer[0] = ino;        // Original image to plot
       p->answer[1] = cint(a);    // Angle around x axis
       p->answer[2] = cint(b);    // Angle around y axis
       p->answer[3] = cint(c);    // Angle around z axis
       p->answer[4] = cint(100*xscale);
       p->answer[5] = cint(100*yscale);
       p->answer[6] = cint(100*zscale);
       p->answer[7] = cint(xcenter);
       p->answer[8] = cint(ycenter);
       p->answer[9] = cint(zcenter);
       p->answer[10] = cint(xtrans);
       p->answer[11] = cint(ytrans);
       p->answer[12] = cint(ztrans);
       p->answer[13] = granularity;
       p->answer[14] = colormode;
       p->answer[15] = ci;        // Image no. of the graph
       p->answer[16] = 0;
       p->answer[17] = 0;
       p->answer[18] = 0;
       p->answer[19] = 0;
   }
   sprintf(title, "Image # %d 3D Graph controls",ino);
   
   //------- Create a form dialog shell widget -----------------------//

   n=0;
   XtSetArg(args[n], XmNwidth, 400); n++;
   XtSetArg(args[n], XmNheight, 400); n++;
   XtSetArg(args[n], XmNtransient, False); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
   XtSetArg(args[n], XmNdialogTitle, xms = XmStringCreateSimple(title));n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   form = p->w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"Plot3DForm", args, n);
   XmStringFree(xms);
       
   //--------Title for dialog box-------------------------------------//

   p->w[wc++] = addlabel(form, title, CENTER, 20, 2, 80, 10); 

   //--------Sliders--------------------------------------------------//

   p->slider[0] = p->w[wc++] = add_slider(form, "Img#", rw, LLABEL, RLABEL, 10, 15, 1, max(1,g.image_count-1), p->answer[0], 1);  
   sprintf(tempstring, "Img#");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 10, 99, 15); 
   sprintf(tempstring, "%d", p->answer[0]);
   p->labelwidget[0] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 10, LLABEL, 15); 
                                                                                                       
   p->slider[1] = p->w[wc++] = add_slider(form, "Xrot", rw, LLABEL, RLABEL, 15, 20, 0, 360, p->answer[1], 1);  
   sprintf(tempstring, "x%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 15, 99, 20); 
   sprintf(tempstring, "%d", p->answer[1]);
   p->labelwidget[1] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 15, LLABEL, 20); 
                                                                                
   p->slider[2] = p->w[wc++] = add_slider(form, "Yrot", rw, LLABEL, RLABEL, 20, 25, 0, 360, p->answer[2], 1);  
   sprintf(tempstring, "y%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 20, 99, 25); 
   sprintf(tempstring, "%d", p->answer[2]/100);
   p->labelwidget[2] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 20, LLABEL, 25); 
                                                                                
   p->slider[3] = p->w[wc++] = add_slider(form, "Zrot", rw, LLABEL, RLABEL, 25, 30, 0, 360, p->answer[3], 1);  
   sprintf(tempstring, "z%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 25, 99, 30); 
   sprintf(tempstring, "%d", p->answer[3]/100);
   p->labelwidget[3] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 25, LLABEL, 30); 
                                                                                
   p->slider[4] = p->w[wc++] = add_slider(form, "X scale", rw, LLABEL, RLABEL, 30, 35,  1, 1000, p->answer[4], 1);  
   sprintf(tempstring, "x scale");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 30, 99, 35); 
   sprintf(tempstring, "%3.2g", (double)p->answer[4]/100);
   p->labelwidget[4] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 30, LLABEL, 35); 
                                                                                
   p->slider[5] = p->w[wc++] = add_slider(form, "Y scale", rw, LLABEL, RLABEL, 35, 40,  1, 1000, p->answer[5], 1);  
   sprintf(tempstring, "y scale");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 35, 99, 40); 
   sprintf(tempstring, "%3.2g", (double)p->answer[5]/100);
   p->labelwidget[5] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 35, LLABEL, 40); 
                                                                                
   p->slider[6] = p->w[wc++] = add_slider(form, "Z scale", rw, LLABEL, RLABEL, 40, 45,  1, 1000, p->answer[6], 1);  
   sprintf(tempstring, "z scale");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 40, 99, 45); 
   sprintf(tempstring, "%3.2g", (double)p->answer[6]/100);
   p->labelwidget[6] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 40, LLABEL, 45); 
                                                                                
   p->slider[7] = p->w[wc++] = add_slider(form, "X ctr", rw, LLABEL, RLABEL, 45, 50,  -1000, 1000, p->answer[7], 1);  
   sprintf(tempstring, "x ctr");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 45, 99, 50); 
   sprintf(tempstring, "%d", p->answer[7]/100);
   p->labelwidget[7] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 45, LLABEL, 50); 
                                                                                
   p->slider[8] = p->w[wc++] = add_slider(form, "Y ctr", rw, LLABEL, RLABEL, 50, 55,  -1000, 1000, p->answer[8], 1);  
   sprintf(tempstring, "y ctr");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 50, 99, 55); 
   sprintf(tempstring, "%d", p->answer[8]/100);
   p->labelwidget[8] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 50, LLABEL, 55); 
                                                                                
   p->slider[9] = p->w[wc++] = add_slider(form, "Z ctr", rw, LLABEL, RLABEL, 55, 60,  -1000, 1000, p->answer[9], 1);  
   sprintf(tempstring, "z ctr");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 55, 99, 60); 
   sprintf(tempstring, "%d", p->answer[9]/100);
   p->labelwidget[9] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 55, LLABEL, 60); 
                                                                                
   p->slider[10] = p->w[wc++] = add_slider(form, "X trans", rw, LLABEL, RLABEL, 60, 65,  -1000, 1000, p->answer[10], 1);  
   sprintf(tempstring, "x trans");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 60, 99, 65); 
   sprintf(tempstring, "%d", p->answer[10]/100);
   p->labelwidget[10] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 60, LLABEL, 65); 
                                                                                
   p->slider[11] = p->w[wc++] = add_slider(form, "Y trans", rw, LLABEL, RLABEL, 65, 70,  -1000, 1000, p->answer[11], 1);  
   sprintf(tempstring, "y trans");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 65, 99, 70); 
   sprintf(tempstring, "%d", p->answer[11]/100);
   p->labelwidget[11] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 65, LLABEL, 70); 
                                                                                
   p->slider[12] = p->w[wc++] = add_slider(form, "Z trans", rw, LLABEL, RLABEL, 70, 75, -1000, 1000, p->answer[12], 1);  
   sprintf(tempstring, "z trans");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 70, 99, 75); 
   sprintf(tempstring, "%d", p->answer[12]/100);
   p->labelwidget[12] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 70, LLABEL, 75); 
                                                                                
   p->slider[13] = p->w[wc++] = add_slider(form, "Gran", rw, LLABEL, RLABEL, 75, 80, 1, 100, p->answer[13], 1);  
   sprintf(tempstring, "gran");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 75, 99, 80); 
   sprintf(tempstring, "%d", p->answer[13]);
   p->labelwidget[13] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 75, LLABEL, 80); 
                                                                                
   p->slider[14] = p->w[wc++] = add_slider(form, "Color", rw, LLABEL, RLABEL, 80, 85, 0, 3, p->answer[14], 1);  
   sprintf(tempstring, "color");
   p->w[wc++] = addlabel(form, tempstring, LEFT, RLABEL, 80, 99, 85); 
   sprintf(tempstring, "%d", p->answer[14]);
   p->labelwidget[14] = p->w[wc++] = addlabel(form, tempstring, LEFT, 1, 80, LLABEL, 85); 
   //// Change SLIDERS at top if adding a new scale
                                                                                
   //// Toggle buttons

   p->buttonwidget[0] = p->w[wc++] = add_togglebutton(form, (char*)"Invert Z", 12, 86, (void*)plot3dbuttoncb, p);
   p->buttonwidget[1] = p->w[wc++] = add_togglebutton(form, (char*)"Rotate X", 27, 86, (void*)plot3dbuttoncb, p);
   p->buttonwidget[2] = p->w[wc++] = add_togglebutton(form, (char*)"Rotate Y", 42, 86, (void*)plot3dbuttoncb, p);
   p->buttonwidget[3] = p->w[wc++] = add_togglebutton(form, (char*)"Rotate Z", 57, 86, (void*)plot3dbuttoncb, p);
   p->buttonwidget[4] = p->w[wc++] = add_togglebutton(form, (char*)"Surface ", 72, 86, (void*)plot3dbuttoncb, p);

   //// Change BUTTONS3D at top if adding a new button

   for(k=0;k<BUTTONS3D;k++)
   {   p->buttonstate[k] = z[ci].gbutton[k];
       XmToggleButtonSetState(p->buttonwidget[k], p->buttonstate[k], False); 
   }

   //--------Ok, Cancel, Help buttons & their callbacks---------------//

   okaybut = p->w[wc++] = add_button(form, (char*)"Accept",    1, 1, 100);
   cancbut = p->w[wc++] = add_button(form, (char*)"Dismiss", 115, 6, 100);
   helpbut = p->w[wc++] = add_button(form, (char*)"Help",    225, 6, 100);
   XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)plot3ddonecb, (XtP)p);
   XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)plot3ddonecb, (XtP)p);
   XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
   XtAddCallback(form, XmNunmapCallback, (XtCBP)plot3dunmapcb, (XtP)p);

   ////  Callbacks for x,y,z rotation sliders
   for(k=0;k<SLIDERS;k++)
   {    XtAddCallback(p->slider[k], XmNvalueChangedCallback, (XtCBP)plot3dslidercb, p);
        //// Comment this out entirely for slow computers
        //// Remove the 'if' for really fast computers.
        // if(z[ci].xsize*z[ci].ysize<160000)
            XtAddCallback(p->slider[k], XmNdragCallback, (XtCBP)plot3dslidercb, p);     
        XtManageChild(p->slider[k]);        
   }
   XtManageChild(form);
   for(k=0;k<20;k++) z[ci].gparam[k] = p->answer[k];
   for(k=0;k<20;k++) z[ci].gbutton[k] = p->buttonstate[k];
   plot3dslidercb(p->slider[0], (XtP)p, 0);  // initial graph
   //// Params & button states are contained in graph not original image.
   for(k=0;k<20;k++) z[ci].gparam[k] = p->answer[k];
   for(k=0;k<20;k++) z[ci].gbutton[k] = p->buttonstate[k];
   if(g.autoundo) backupimage(ci,0);
   p->wc = wc;
   return;
}


//--------------------------------------------------------------------------//
//  plot3ddonecb                                                            //
//--------------------------------------------------------------------------//
void plot3ddonecb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;call_data=call_data;
  p3d *p = (p3d*)client_data;
  p->buttonstate[1] = p->buttonstate[2] = p->buttonstate[3] = 0;
  in_plot3ddonecb = 1;
  for(int k=0; k<p->wc; k++) XtDestroyWidget(p->w[k]);
  in_plot3ddonecb = 0;
  delete p;   
}


//--------------------------------------------------------------------------//
//  plot3dunmapcb                                                           //
//--------------------------------------------------------------------------//
void plot3dunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
   if(!in_plot3ddonecb) plot3ddonecb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// plot3dslidercb                                                           //
//--------------------------------------------------------------------------//
void plot3dslidercb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  char tempstring[64];
  int k, value = 0, row=0;
  double showvalue;
  p3d *p = (p3d*)client_data;
  for(k=0; k<SLIDERS; k++) if(w == p->slider[k]){ row = k;  break;}
  XmScaleGetValue(w, &value);
  p->answer[row] = value;
  showvalue = (double)value;
  if(between(row,4,6)) showvalue/=100;
  sprintf(tempstring, "%g", showvalue);
  set_widget_label(p->labelwidget[row], tempstring);
  XFlush(g.display);
  plot3dgraph(p);
}


//--------------------------------------------------------------------------//
//  setslider                                                               //
//--------------------------------------------------------------------------//
void setslider(p3d *p, int sliderno, int value)
{
   char tempstring[64];
   XmScaleSetValue(p->slider[sliderno], value); 
   sprintf(tempstring, "%d", value);
   set_widget_label(p->labelwidget[sliderno], tempstring);
}


//--------------------------------------------------------------------------//
// plot3buttoncb                                                            //
//--------------------------------------------------------------------------//
void plot3dbuttoncb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  if(z[ci].colortype != GRAPH) return;
  int ino, k, but=0, state;
  p3d *p = (p3d*)client_data;
  for(k=0; k<BUTTONS3D; k++) { if(w == p->buttonwidget[k]) { but = k; break; }}
  state = XmToggleButtonGetState(w); 
  p->buttonstate[but] = state;
  ino = ci;
  do{
      check_event();
      if(p->buttonstate[1]){ p->answer[1] += 2; setslider(p, 1, p->answer[1]);}
      if(p->buttonstate[2]){ p->answer[2] += 2; setslider(p, 2, p->answer[2]);}
      if(p->buttonstate[3]){ p->answer[3] += 2; setslider(p, 3, p->answer[3]);}
      if(p->answer[1]>=360) p->answer[1]-=360;
      if(p->answer[2]>=360) p->answer[2]-=360;
      if(p->answer[3]>=360) p->answer[3]-=360;
      plot3dgraph(p); 
      //// In case they unload the graph or image while it's rotating
      if(z[ino].gparam[0] == 0)
      {   message("Error",ERROR); 
          for(k=0;k<20;k++) z[ino].gbutton[k]=0;
          z[ino].gparam[0] = -1;
      }
      if(g.getout || z[ino].gparam[0] <= 0 || ino>g.image_count-1)
      {  p->buttonstate[1] = p->buttonstate[2] = p->buttonstate[3] = 0; g.getout=0; break; }
   }while (p->buttonstate[1] || p->buttonstate[2] || p->buttonstate[3]);
   XFlush(g.display);
}


//--------------------------------------------------------------------------//
//   plot3dgraph                                                            //
//--------------------------------------------------------------------------//
void plot3dgraph(p3d *p)
{
#ifdef DIGITAL
#define ZNORMAL 0
#define ZGRAY 1
#define ZINV 2
#define ZORIG 3
#else
   const int ZNORMAL=2;
   const int ZGRAY  =1;
   const int ZINV   =0;
   const int ZORIG  =3;
#endif
   int colormode = ZINV, ino, imno, hit=0, i2, imax, xmax, 
       j, k, value=0, i, ii, jj, granularity, ovalue, oxx=0, oyy=0, 
       xtemp=0, ytemp=0, wantcolor, want_surface=0, xcorner[5], ycorner[5];
   int jj1, jj2, jj3, ogimode, bkgcolor, fgcolor=0;
   int color = (int)g.maxcolor; 

   ino = p->answer[15];    // graph image no
   imno  = p->answer[0];   // image no of image being plotted
   if(z[ino].colortype != GRAPH || ino>g.image_count-1) return;
   if(z[ino].gparam[0] == 0)
   {   message("Glrrk!!!!",ERROR); 
       for(k=0;k<20;k++) z[ino].gbutton[k]=0;
       z[ino].gparam[0] = -1;
   }
   if(z[ino].gparam[0] == -1) return;

   int bpp = z[ino].bpp;
   int ibpp = z[imno].bpp;
   imax = max(2*z[imno].xsize, 2*z[imno].ysize);

   int *xlastline,*ylastline, *prevx, *prevy;
   xlastline = new int[imax+1];
   ylastline = new int[imax+1];
   prevx = new int[imax+1];
   prevy = new int[imax+1];

   int xcenter, ycenter, zcenter, xtrans, ytrans, ztrans, xhalf, yhalf;
   int xgraphcenter = z[ino].xpos + z[ino].xsize/2;
   int ygraphcenter = z[ino].ypos + z[ino].ysize/2;
   int granoff,total,count,rtot,gtot,btot;
   //int rr,gg,bb,v,j2,i3,i4;
   double mat[3][3];
   double a,b,c;                  // x,y,x angle
   double xx,yy,zz;               // coordinates
   double xx1,yy1;                // new coordinates
   double xscale, yscale, zscale =  0.5;
   double zhalf;        
   double yymat01, yymat11;

   lineinfo li;

   li.ino=ino;    // constrain drawing to ino
   li.type=0;
   li.color=0;
   li.width=1;
   li.wantgrad=0;
   li.gradr=0;
   li.gradg=0;
   li.gradb=0;
   li.gradi=0;
   li.skew=0;
   li.perp_ends=0;
   li.wantarrow=0;
   li.arrow_width=0.0;
   li.arrow_inner_length=0.0;
   li.arrow_outer_length=0.0;
   li.ruler_scale = 1.0;
   li.ruler_ticlength = 5.0;
   li.window=z[ino].win;
   li.wantprint=0;
   li.antialias=0;
  
   a = (double)p->answer[1];
   b = (double)p->answer[2];
   c = (double)p->answer[3];
   xscale  = (double)p->answer[4] / 100;
   yscale  = (double)p->answer[5] / 100;
   zscale  = (double)p->answer[6] / 100;
   xcenter = p->answer[7];
   ycenter = p->answer[8];
   zcenter = p->answer[9];
   xtrans  = p->answer[10];
   ytrans  = p->answer[11];
   ztrans  = p->answer[12];
   granularity = p->answer[13];
   colormode   = p->answer[14];
   bkgcolor = 0;
   if(colormode == ZINV) bkgcolor = color;

   zscale *= 256/g.maxvalue[ibpp];
   xhalf = cint(z[ino].xsize*0.3) + xcenter;  //// These 3 ensure rotation is about the
   yhalf = cint(z[ino].ysize*0.3) + ycenter;  //// center of graph
   zhalf  = 127 * zscale;
   xmax = z[imno].xsize * g.off[ibpp];
   wantcolor = (z[imno].colortype != GRAY);

   hit = 0;
   while(a<0.0) a+=360.0;
   while(b<0.0) b+=360.0;
   while(c<0.0) c+=360.0;
   while(a>=360) a-=360.0;
   while(b>=360) b-=360.0;
   while(c>=360) c-=360.0;

   for(k=0; k<z[ino].ysize; k++) 
       memset(z[ino].image[0][k], bkgcolor, z[ino].xsize*g.off[z[ino].bpp]);

   granoff = granularity * g.off[ibpp];
   mat[0][0] = cosine(b)*cosine(c);
   mat[0][1] = sine(a)*sine(b)*cosine(c) - cosine(a)*sine(c);
   mat[0][2] = cosine(a)*sine(b)*cosine(c) + sine(a)*sine(c);
   mat[1][0] = cosine(b)*sine(c);
   mat[1][1] = sine(a)*sine(b)*sine(c) + cosine(a)*cosine(c);
   mat[1][2] = cosine(a)*sine(b)*sine(c) - sine(a)*cosine(c);             
   mat[2][0] = -sine(a);
   mat[2][1] = sine(a)*cosine(b);
   mat[2][2] = cosine(a)*cosine(b);              
   jj1 = 0;
   jj2 = z[imno].ysize;
   jj3 = granularity;
   ogimode = g.imode;
   g.imode = BUFFER;  //// This makes surface plotting much faster

   int xgraphcenterplusxtrans = xgraphcenter + xtrans;
   int ygraphcenterplusytrans = ygraphcenter + ytrans;
   if(p->buttonstate[4]) want_surface=1;
   
   for(j=jj1; j<jj2; j+=jj3)
   { 
     jj = j - yhalf;
     if(yscale !=1.0) yy = (double) jj * yscale; else yy = (double)jj;
     yymat11 = yy*mat[1][1]; 
     yymat01 = yy*mat[0][1];
     for(i=0,i2=0; i<z[imno].xsize; i+=granularity, i2+=granoff)
     {  
       count = total = rtot = gtot = btot = 0;

       /*   This section averages the pixels in each rectangle but makes it
            50% slower
       for(j2=j; j2<j+granularity; j2++)
       for(i4=i,i3=i2; i4<i+granularity; i4++,i3+=g.off[ibpp])
       {    if(j2 >= z[imno].ysize) continue;
            if(i3 >= xmax) continue;
            v = pixel_equivalent(imno, i4, i3, j);
            if(!wantcolor)
                total += v;
            else 
            {   valuetoRGB(v,rr,gg,bb,ibpp);
                rtot += rr;
                gtot += gg;
                btot += bb;
            }
            count++;
       }
       if(count) 
       {   if(!wantcolor)
                value = total / count;
           else
           {    rr = rtot / count;
                gg = gtot / count;
                bb = btot / count;
                value = RGBvalue(rr,gg,bb,ibpp);
           }
       }else value = 0;
       */
       value = pixel_equivalent(imno, i, i2, j);
       ovalue = value;
       ii = i - xhalf;
       if(xscale !=1.0) xx = (double) ii * xscale; else xx = (double)ii;
       if(p->buttonstate[0])  // invert z
           zz = (double) (value+ztrans) * zscale - zhalf + zcenter;
       else
           zz = (double) (-value+ztrans) * zscale + zhalf + zcenter; 
       xx1 = xx*mat[0][0] + yymat01 + zz*mat[0][2];
       yy1 = xx*mat[1][0] + yymat11 + zz*mat[1][2];
       //// zz1 = xx*mat[2][0] + yy*mat[2][1] + zz*mat[2][2];
       xx = xx1;
       yy = yy1;
       ////  zz = zz1 + ztrans;
       switch(colormode)
       {   case ZGRAY:   fgcolor = int(max(30, min(255, zhalf - zz)));
                         fgcolor = RGBvalue(color, color, color, bpp);
                         break;
           case ZORIG:   fgcolor = convertpixel(ovalue, ibpp, bpp, 1); 
                         break;
           case ZINV:    fgcolor = (int)g.maxvalue[bpp] - color;
                         break;
           case ZNORMAL: fgcolor = color;
                         break;
       }
       xtemp = cint(xx) + xgraphcenterplusxtrans;
       ytemp = cint(yy) + ygraphcenterplusytrans;
       if(want_surface)
       {  if(i>0 && j>0)
          {   
              xcorner[1] = prevx[i-granularity];
	      ycorner[1] = prevy[i-granularity];
	      xcorner[2] = prevx[i];
	      ycorner[2] = prevy[i];
	      xcorner[3] = oxx;
	      ycorner[3] = oyy;
	      xcorner[4] = xtemp;
	      ycorner[4] = ytemp;
              fill_polygon(xcorner, ycorner, fgcolor);
          }
       }else
       {  li.color = fgcolor;
          if(j>0 && hit && between(i,0,imax)) 
              line(xlastline[i], ylastline[i], xtemp, ytemp, BUFFER, &li);
          if(i>0 && hit) 
              line(oxx, oyy, xtemp, ytemp, BUFFER, &li);
       }
       xlastline[i] = xtemp;
       ylastline[i] = ytemp;
       oxx = xtemp;
       oyy = ytemp;
       hit = 1;
     }
     if(want_surface)
     {  for(i=0; i<z[imno].xsize; i+=granularity)
        {  prevx[i] = xlastline[i];
           prevy[i] = ylastline[i];
        } 
     }
   }
   g.imode = ogimode;
   rebuild_display(ino);  
   redraw(ino);

   delete[] prevy;
   delete[] prevx;
   delete[] ylastline;
   delete[] xlastline;
}


