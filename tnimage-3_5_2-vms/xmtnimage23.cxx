//--------------------------------------------------------------------------//
// xmtnimage23.cc                                                           //
// multiframe images                                                        //
// Latest revision: 04-01-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;


//--------------------------------------------------------------------------//
//  movie  delay is in msec                                                 //
//--------------------------------------------------------------------------//
void movie(int delay)
{
   if(ci<=0 || z[ci].frames<=1) return;
   int moving=1, minval=0, maxval=0, startval=z[ci].cf;
   static char textlabel[FILENAMELENGTH];
   strcpy(textlabel, "movie");
   maxval = z[ci].frames-1;
   
   z[ci].c.param[0] = z[ci].cf;
   z[ci].c.param[1] = ci;
   z[ci].c.param[2] = delay;
   z[ci].c.param[3] = moving;
   z[ci].c.param[4] = 0;         // No refresh
   z[ci].c.f1 = null;
   z[ci].c.f2 = null;
   z[ci].c.f3 = framenext;
   z[ci].c.f4 = null;
   z[ci].c.f5 = null;
   z[ci].c.f6 = null;
   z[ci].c.f7 = null;
   z[ci].c.f8 = null;
   z[ci].c.startval = startval;
   z[ci].c.minval = minval;
   z[ci].c.maxval = maxval;
   z[ci].c.type = PARAM_ARRAY;
   z[ci].c.wantpreview = 0;
   z[ci].c.widget[0] = NULL;
   z[ci].c.form = z[ci].widget;  
   z[ci].c.path = NULL;  
   z[ci].c.k = 0;
   z[ci].c.ino = ci;
   XtAppAddTimeOut(g.app, delay, moviecb, (XtP)&z[ci].c);
   if(delay) z[ci].fps = 1000.0/(double)z[ci].c.param[2];
}


//--------------------------------------------------------------------------//
//  menu3d                                                                  //
//--------------------------------------------------------------------------//
void menu3d(void)
{  
   if(ci<0) return;
   Widget startbutton, delayslider, slider, xrotslider, yrotslider, zrotslider, 
          form, text, okaybut, cancbut, helpbut;
   XmString xms;
   int k, n, xsize=350, ysize=270, wc=0, rw, delay, helptopic=0, minval = 0,
       maxval = 0, startval = z[ci].cf, ino = ci;
   static char *textlabel;
   static int moving = 0;
   Arg args[100];
   char tempstring[128];
   char title[100];
   textlabel = new char[FILENAMELENGTH];

   static p3d *p;
   p = new p3d;
   p->c = new clickboxinfo[3];
   textlabel = new char[FILENAMELENGTH];
 
   if(z[ino].fps) delay = (int)(1000/z[ino].fps); else delay = 1;

   //// Minval and maxval must differ by at least 1, otherwise Motif changes
   //// the scale to 0-100. This is prevented by setting XmNeditable to 0
   //// in add_slider.
      
   if(ino>=0) maxval = max(0, z[ino].frames-1);
   p->answer[0] = z[ino].cf;
   p->answer[1] = ino;
   p->answer[2] = delay;
   p->answer[3] = moving;
   p->answer[4] = 1;              // No refresh
   p->answer[5] = z[ino].xangle;  // x angle
   p->answer[6] = z[ino].yangle;  // y angle
   sprintf(title, "Image # %d Frame controls",ino);
   
   //------- Create a form dialog shell widget -----------------------//

   n=0;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, ysize); n++;
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
   form = p->w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"Menu3DForm", args, n);
   XmStringFree(xms);
       
   //--------Title for dialog box-------------------------------------//

   p->w[wc++] = addlabel(form, title, CENTER, 20, 2, 80, 10); 
   itoa(startval, textlabel, 10);

   //--------Slider indicating frame no.------------------------------//

   rw = max(200, xsize-20);
   slider = p->w[wc++] = add_slider(form,(char*)"Frame",rw,7,90,36,48,minval,maxval,startval,1);  
   p->w[wc++] = addlabel(form, (char*)"z", LEFT, 90, 40, 99, 52); 

   //--------Sliders indicating x,y,z rotation------------------------//

   xrotslider = p->w[wc++] = add_slider(form, (char*)"Xrot", rw, 7, 90, 48, 60, 0, 360, 0, 1);  
   sprintf(tempstring, "x%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, 90, 52, 99, 64); 

   yrotslider = p->w[wc++] = add_slider(form, (char*)"Yrot", rw, 7, 90, 60, 72, 0, 360, 0, 1);  
   sprintf(tempstring, "y%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, 90, 64, 99, 76); 

   zrotslider = p->w[wc++] = add_slider(form, (char*)"Zrot", rw, 7, 90, 72, 84, 0, 360, 0, 1);  
   sprintf(tempstring, "z%c",XK_degree);
   p->w[wc++] = addlabel(form, tempstring, LEFT, 90, 76, 99, 88); 

   //--------Text area------------------------------------------------//

   n = 0;
   XtSetArg(args[n], XmNcolumns, 40); n++;
   XtSetArg(args[n], XmNrows, 1); n++;
   XtSetArg(args[n], XmNvalue, textlabel); n++;
   XtSetArg(args[n], XmNleftPosition, 5); n++;     // % of width from left
   XtSetArg(args[n], XmNtopPosition, 12); n++;     // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 24); n++;  // % of width from top
   XtSetArg(args[n], XmNrightPosition, 50); n++;   // % of height from left
   XtSetArg(args[n], XmNfractionBase, 100); n++;   // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNmarginHeight, 1); n++;
   XtSetArg(args[n], XmNmarginWidth, 2); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   p->c[0].title = textlabel;
   for(k=0;k<100;k++) p->c[0].param[k] = p->answer[k];
   p->c[0].startval = startval;
   p->c[0].minval = minval;
   p->c[0].maxval = maxval;
   p->c[0].type = PARAM_ARRAY;
   p->c[0].wantpreview = 0;
   p->c[0].widget[0] = slider;
   p->c[0].form = form;  // set this to -1 for no refresh of dialog
   p->c[0].path = NULL;  
   p->c[0].k = 0;
   p->c[0].f1 = null;
   p->c[0].f2 = null;
   p->c[0].f3 = framenext;
   p->c[0].f4 = null;
   p->c[0].f5 = null;
   p->c[0].f6 = null;
   p->c[0].f7 = null;
   p->c[0].f8 = null;
   p->c[0].ino = ino;
   p->c[0].answers = NULL;
   text = p->w[wc++] = XmCreateTextField(form, textlabel, args, n);
   XtAddCallback(text, XmNvalueChangedCallback, (XtCBP)clickboxcb, (XtP)&p->c[0]);
   p->c[0].widget[1] = text;
   XtManageChild(text);
   
   //--------Now add a callback so slider can change text area--------//

   p->c[1].title = textlabel;
   for(k=0;k<100;k++) p->c[1].param[k] = p->answer[k];
   p->c[1].startval = startval;
   p->c[1].minval = minval;
   p->c[1].maxval = maxval;
   p->c[1].type = PARAM_ARRAY;
   p->c[1].wantpreview = 0;
   p->c[1].widget[0] = text;
   p->c[1].widget[1] = slider;
   p->c[1].form = form;  
   p->c[1].path = NULL;  
   p->c[1].decimalpoints = 0;  
   p->c[1].count = delay;   // time delay
   p->c[1].k = 0;
   p->c[1].f1 = null;
   p->c[1].f2 = null;
   p->c[1].f3 = framenext;
   p->c[1].ino = ino;
   p->c[1].answers = NULL;
   if(maxval > minval)
   {   XtAddCallback(slider, XmNdragCallback, (XtCBP)slidercb, (XtP)&p->c[1]);
       XtAddCallback(slider, XmNvalueChangedCallback, (XtCBP)slidercb, (XtP)&p->c[1]);
   }

   //--------Slider indicating delay----------------------------------//

   delayslider = p->w[wc++] = add_slider(form,(char*)"Delay",rw,7,50,24,36,40,2000,delay,20);  

   p->c[2].title = textlabel;
   p->c[2].answer = delay;
   for(k=0;k<100;k++) p->c[2].param[k] = p->answer[k];
   p->c[2].startval = delay;
   p->c[2].minval = 40;
   p->c[2].maxval = 2000;
   p->c[2].type = PARAM_ARRAY;
   p->c[2].wantpreview = 0;
   p->c[2].widget[0] = text;
   p->c[2].widget[1] = slider;
   p->c[2].form = form;  
   p->c[2].path = NULL;  
   p->c[2].decimalpoints = 0;  
   p->c[2].k = 0;
   p->c[2].f1 = null;
   p->c[2].f2 = null;
   p->c[2].f3 = null;
   p->c[2].ino = ino;
   p->c[2].answers = NULL;
   XtAddCallback(delayslider, XmNdragCallback, (XtCBP)delaycb, (XtP)&p->c[2]);
   XtAddCallback(delayslider, XmNvalueChangedCallback, (XtCBP)delaycb, (XtP)&p->c[2]);
   p->w[wc++] = addlabel(form,(char*)"msec",LEFT,50,24,65,36);

   //--------Ok, Cancel, Help buttons & their callbacks---------------//
   okaybut = p->w[wc++] = add_button(form, (char*)"Accept",   1, 1, 100);
   cancbut = p->w[wc++] = add_button(form, (char*)"Dismiss",115, 6, 100);
   helpbut = p->w[wc++] = add_button(form, (char*)"Help",   225, 6, 100);

   XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)menu3dokcb, (XtP)p);
   XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)menu3dcancelcb, (XtP)p);
   XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);

   //--------Start/pause button---------------------------------------//
   n=0;
   XtSetArg(args[n], XmNleftPosition, 65); n++;    // % of width from left
   XtSetArg(args[n], XmNtopPosition, 14); n++;     // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 28); n++;  // % of width from top
   XtSetArg(args[n], XmNrightPosition, 90); n++;   // % of height from left
   XtSetArg(args[n], XmNfractionBase, 100); n++;   // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   startbutton = p->w[wc++] = XmCreatePushButton(form, (char*)"Start", args, n);
   XtManageChild(startbutton);
   XtAddCallback(startbutton, XmNactivateCallback, (XtCBP)startcb, (XtP)&p->c[1]);
   p->c[1].param[2] = delay;

   ////  Movie callback
   XtAppAddTimeOut(XtWidgetToApplicationContext(form), delay, 
       moviecb, (XtP)&p->c[1]);

   ////  Callbacks for x,y,z rotation sliders
   XtAddCallback(xrotslider, XmNvalueChangedCallback, (XtCBP)xrotcb, (XtP)&p->c[1]);
   XtAddCallback(yrotslider, XmNvalueChangedCallback, (XtCBP)yrotcb, (XtP)&p->c[1]);
   XtAddCallback(zrotslider, XmNvalueChangedCallback, (XtCBP)zrotcb, (XtP)&p->c[1]);

   ////  Uncomment these if you have a really fast computer
   // XtAddCallback(xrotslider, XmNdragCallback, (XtCBP)xrotcb, (XtP)&p->c[1]);
   // XtAddCallback(yrotslider, XmNdragCallback, (XtCBP)yrotcb, (XtP)&p->c[1]);
   // XtAddCallback(zrotslider, XmNdragCallback, (XtCBP)zrotcb, (XtP)&p->c[1]);

   XtManageChild(form);

   p->ptr[1] = (void*)textlabel;
   p->wc = wc;
   p->form = form;
   p->c[0].widget[0] = delayslider;
}


//--------------------------------------------------------------------------// 
// menu3dokcb                                                               //
//--------------------------------------------------------------------------// 
void menu3dokcb(Widget w, XtP client_data, XmACB *call_data)
{ 
  w=w;call_data=call_data;client_data=client_data;  // Keep compiler quiet
  p3d *p = (p3d *)client_data;
  int ino = p->c[0].ino;
  if(between(ino, 0, g.image_count-1)) z[ino].animation = 1;
}


//--------------------------------------------------------------------------// 
// menu3dcancelcb                                                           //
//--------------------------------------------------------------------------// 
void menu3dcancelcb(Widget w, XtP client_data, XmACB *call_data)
{ 
   if(g.block) return;   // Waiting for input; don't deallocate
   w=w;call_data=call_data;  // Keep compiler quiet
   int delay,k;
   p3d *p = (p3d *)client_data;
   if(XtIsManaged(p->form)) XtUnmanageChild(p->form);
   int ino = p->c[0].ino;
   p->c[0].form = 0;
   if(between(ino, 0, g.image_count-1)) z[ino].animation = 0;
   XtVaGetValues(p->c[0].widget[0], XmNvalue, &delay, NULL);         
   if(between(ino, 0, g.image_count-1)) 
      if(delay) z[ino].fps = 1000.0/(double)delay;
   for(k=0; k<p->wc; k++) if(p->w[k]) XtDestroyWidget(p->w[k]);
   if(p->ptr[1]) delete[] (char*)p->ptr[1];
   if(p) delete p;
}


//--------------------------------------------------------------------------// 
//  framenext                                                               //
//--------------------------------------------------------------------------// 
void framenext(int answers[100])
{    
   int oino = ci;
   int frame = answers[0];
   int ino = answers[1];
   if(ino>0 && ino<g.image_count)
   {   z[ino].cf = max(0, min(z[ino].frames-1, frame)); 
       frame = z[ino].cf;
       switch_palette(ino);
       rebuild_display(ino);
       redraw(ino);
       ////  This is necessary in case image is on root window
       send_expose_event(z[ino].win, Expose, 0, 0, z[ino].xsize, z[ino].ysize);
       XFlush(g.display);
   }
   switch_palette(oino);
}


//--------------------------------------------------------------------------//
// moviecb                                                                  //
//--------------------------------------------------------------------------//
void moviecb(XtP client_data, XtIntervalId *timer_id)
{  
   client_data=client_data; timer_id=timer_id;  // Keep compiler quiet
   char tempstring[100];
   XEvent event;
   clickboxinfo *c = (clickboxinfo *)client_data;
   int ino = c->param[1];
   if(!between(ino, 0, g.image_count-1)) return;
   if(!z[ino].animation) return;
   if(c->form != 0 && c->param[3])
   {    
         c->param[2] = z[ino].c.param[2];
         if(XCheckMaskEvent(g.display, g.main_mask | g.mouse_mask, &event))
              XtAppProcessEvent(g.app, XtIMAll);
         XtAppAddTimeOut(XtWidgetToApplicationContext(g.main_widget), c->param[2], 
             moviecb, (XtP)c);
         itoa(c->param[0], tempstring, 10);   
         c->param[0]++;   // frame no.  
         if(c->param[0] > c->maxval) c->param[0]=0;
         if(c->param[4])  // if want refresh
         {   XmTextSetString(c->widget[0], tempstring);   
             XmScaleSetValue(c->widget[1], c->param[0]);
             clickboxcb(c->widget[0], client_data, (XmACB*)NULL);
             slidercb(c->widget[1], client_data,(XmACB*)NULL);
         }else              // just call the callbax
         {   if(g.getout) g.getout=0; 
             slider_now_cb((Widget)0, client_data, (XmACB*)NULL);
         }
   }   
}


//--------------------------------------------------------------------------//
// delaycb                                                                  //
//--------------------------------------------------------------------------//
void delaycb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  int ino;
  clickboxinfo *c = (clickboxinfo*)client_data;
  ino = c->ino;
  XmScaleGetValue(w, &answer);
  c->param[2] = answer;
  if(between(ino, 0, g.image_count-1))
     z[ino].c.param[2] = answer;
}


//--------------------------------------------------------------------------//
// startcb                                                                  //
//--------------------------------------------------------------------------//
void startcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;  // keep compiler quiet
   clickboxinfo *c = (clickboxinfo*)client_data;
   char tempstring[20];
   int ino = c->ino;
   if(!between(ino, 0, g.image_count-1)) return;
   c->param[3] = 1 - c->param[3];
   int delay = c->param[2];
   if(delay) z[ino].fps = 1000.0/(double)delay;
   z[ino].c.param[2] = delay; 

   if(c->param[3])
   {   XtAppAddTimeOut(XtWidgetToApplicationContext(c->form), c->param[2], 
              moviecb, (XtP)c);
       strcpy(tempstring,"Pause");   
       z[ino].animation = 1;
   }else
   {
       strcpy(tempstring,"Start");   
       z[ino].animation = 0;
   }
   set_widget_label(w, tempstring);
}


//--------------------------------------------------------------------------//
// xrotslidercb                                                             //
//--------------------------------------------------------------------------//
void xrotcb(Widget w, XtP client_data, XmACB *call_data)
{
  int xangle, ino;
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  clickboxinfo *c = (clickboxinfo*)client_data;
  XmScaleGetValue(w, &answer);
  ino    = c->param[1];
  if(!between(ino, 0, g.image_count-1)) return;
  xangle = answer;
  c->param[4] = xangle;
  z[ino].xangle = xangle;
  rotate3d(ino, z[ino].xangle, z[ino].yangle, z[ino].zangle);  
}


//--------------------------------------------------------------------------//
// yrotslidercb                                                             //
//--------------------------------------------------------------------------//
void yrotcb(Widget w, XtP client_data, XmACB *call_data)
{
  int yangle, ino;
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  clickboxinfo *c = (clickboxinfo*)client_data;
  XmScaleGetValue(w, &answer);
  ino    = c->param[1];
  if(!between(ino, 0, g.image_count-1)) return;
  yangle = answer;
  c->param[5] = yangle;
  z[ino].yangle = yangle;
  rotate3d(ino, z[ino].xangle, z[ino].yangle, z[ino].zangle);  
}


//--------------------------------------------------------------------------//
// zrotslidercb                                                             //
//--------------------------------------------------------------------------//
void zrotcb(Widget w, XtP client_data, XmACB *call_data)
{
  int zangle, ino;
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  clickboxinfo *c = (clickboxinfo*)client_data;
  XmScaleGetValue(w, &answer);
  ino    = c->param[1];
  if(!between(ino, 0, g.image_count-1)) return;
  zangle = answer;
  c->param[6] = zangle;
  z[ino].zangle = zangle;
  rotate3d(ino, z[ino].xangle, z[ino].yangle, z[ino].zangle);  
}


//--------------------------------------------------------------------------//
// rotate3d - not finished                                                  //
//--------------------------------------------------------------------------//
void rotate3d(int ino, double xdegrees, double ydegrees, double zdegrees)
{
  xdegrees=xdegrees; ydegrees=ydegrees;
  ScanParams sp; 
  int bpp,iino,ibpp,j,k,oxsize,oysize,scancount=0,size,mode=0,
      ix1,ix2,iy1,iy2,j2,k2,xsize,ysize,oxpos,oypos,v;
  double xx[4], yy[4], zz[4];
  double degrees,a,b,c,d,e,f,h,q,s,dx,dy,r1,x1,x2,x3,x4,y1,y2,y3,y4;
  int xoffset, yoffset;
  if(!between(ino, 0, g.image_count-1)) return;

  drawselectbox(OFF);
  oxpos = g.selected_ulx;
  oypos = g.selected_uly;
  oxsize = g.selected_lrx-g.selected_ulx;
  oysize = g.selected_lry-g.selected_uly;
  ix1 = oxpos;
  iy1 = oypos;
  ix2 = ix1 + oxsize;
  iy2 = iy1 + oysize; 

  g.getout=0; 
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  degrees=zdegrees;
  while(degrees>=360.0) degrees-=360.0;
  while(degrees<0.0) degrees+=360.0;
  
  r1 = degrees;
  while(r1>90.0) r1-=90.0;
  r1 = r1/DEGPERRAD;        // rotation angle in radians

  dx = sin(r1);  // increment in x direction (scan returns array spaced at 1 unit distances)
  dy = cos(r1);  // increment in y direction 

  a = (double)oysize * dx;
  b = a * dx;
  c = a * dy;
  d = (double)oxsize * dx;
  e = d * dy;
  f = d * dx;
  h = (double)oysize * dy * dy;   
  q = (double)oxsize * dy;
  s = (double)oysize * dy;

  xsize = (int)(a + q);
  ysize = (int)(d + s);
    
  if(between(degrees,90.001,180.001)) swap(xsize,ysize);
  if(between(degrees,270.001,359.999)) swap(xsize,ysize);

  bpp = z[ino].bpp;
  switchto(ino);

  size = 100 +(int)sqrt(oxsize*oxsize + oysize*oysize);
  sp.skip        = -1;
  sp.leavemarked = 0;
  sp.scanwidth   = 0;
  sp.invert      = 0;
  sp.ino         = ino;
  sp.bpp         = z[ino].bpp;
  sp.wantcolor = 4;     // color, no antialiasing
  sp.diameter  = 12;
  
  sp.scan = new double[size];
  for(k=0;k<size;k++)sp.scan[k] = (double)k;
  
  for(k=0;k<256;k++) sp.od[k] = k;

  x1 = oxpos - c;  
  y1 = oypos + h;
  x2 = oxpos + oxsize - f;
  y2 = oypos + oysize + e;
  x3 = oxpos + oxsize + c;
  y3 = oypos + b;
  x4 = oxpos + f;
  y4 = oypos - e;

  if(between(degrees,0.0, 0.001)) mode=1;
  if(between(degrees,89.999, 90.001)) mode=2;
  if(between(degrees,179.999, 180.001)) mode=3;
  if(between(degrees,269.999, 270.001)) mode=4;
  if(between(degrees,359.999, 360.0)) mode=1; 
  if(between(degrees,0.001,89.999))   { xx[1]=x1; yy[1]=y1; xx[2]=x2; yy[2]=y2; dy*=-1; }
  if(between(degrees,90.001,179.999)) { xx[1]=x4; yy[1]=y4; xx[2]=x1; yy[2]=y1; fswap(dx,dy);}                    
  if(between(degrees,180.001,269.999)){ xx[1]=x3; yy[1]=y3; xx[2]=x4; yy[2]=y4; dx*=-1; }                    
  if(between(degrees,270.001,359.999)){ xx[1]=x2; yy[1]=y2; xx[2]=x3; yy[2]=y3; dx*=-1;
       dy*=-1; fswap(dx,dy);  }                    

  ////  Rotate the image
  printstatus(BUSY);
  xoffset = (xsize-oxsize)/2;
  yoffset = (ysize-oysize)/2;
  switch(mode)
  {   case 0:     // arbitrary angle
         for(j=0;j<ysize;j++) 
         {   
               scan_fixed_width_area3d(&sp, xx, yy, zz, scancount); 
               if(bpp!=g.bitsperpixel)
                   for(k=0;k<scancount;k++)
                        sp.scan[k] = (double)convertpixel((int)sp.scan[k], sp.bpp, g.bitsperpixel, 1);

               for(k=0;k<min(z[ino].xsize+xoffset,xsize);k++)          
               {    
                    if(between(ysize-j-yoffset-1, 0, z[ino].ysize-1) &&
                       between(k-xoffset, 0, z[ino].xsize-1))
                    {
                       putpixelbytes(z[ino].img[ysize-j-yoffset-1] + 
                        (k-xoffset)*g.off[g.bitsperpixel], (int)sp.scan[k], 1);
                    }                        
               }
               yy[1] += dy;
               yy[2] += dy;
               xx[1] += dx;
               xx[2] += dx;
         }
         break;
      case 1:     //  0 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   v = readpixelonimage(k,j,ibpp,iino,-1);
             v = convertpixel((int)v, sp.bpp, g.bitsperpixel, 1);
             putpixelbytes(z[ino].img[j2]+k2*g.off[g.bitsperpixel], v, 1, g.bitsperpixel, 1);
         }
         break;
      case 2:    // 90 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   v=readpixelonimage(k,j,bpp,iino,-1);
             v = convertpixel((int)v, sp.bpp, g.bitsperpixel, 1);
             if(between(oxsize-k2, 0, z[ino].ysize-1) &&
                between(j2,        0, z[ino].xsize-1))
             putpixelbytes(z[ino].img[oxsize-k2]+j2*g.off[g.bitsperpixel],v,1,g.bitsperpixel,1);
         }
         break;
      case 3:     // 180 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   v=readpixelonimage(k,j,ibpp,iino,-1);
             v = convertpixel((int)v, sp.bpp, g.bitsperpixel, 1);
             if(between(oysize-j2, 0, z[ino].ysize-1) &&
                between(oxsize-k2, 0, z[ino].xsize-1))
             putpixelbytes(z[ino].img[oysize-j2]+(oxsize-k2)*g.off[g.bitsperpixel],v,1,g.bitsperpixel,1);
         }
         break;
      case 4:     // 270 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   v=readpixelonimage(k,j,bpp,iino,-1);
             v = convertpixel((int)v, sp.bpp, g.bitsperpixel, 1);
             if(between(k2,        0, z[ino].ysize-1) &&
                between(oysize-j2, 0, z[ino].xsize-1))
             putpixelbytes(z[ino].img[k2]+(oysize-j2)*g.off[g.bitsperpixel],v,1,g.bitsperpixel,1);
         }
         break;
  } 
  delete[] sp.scan;
  z[ino].touched = 1;
  switchto(ino);
  printstatus(NORMAL);
  redraw(ino);
  ////  This is necessary in case image is on root window
  send_expose_event(z[ino].win,Expose,0,0,z[ino].xsize,z[ino].ysize);
  XFlush(g.display);
  return;
}


//--------------------------------------------------------------------------//
// scan_fixed_width_area3d - only works for z rotation so far               //
//--------------------------------------------------------------------------//
void scan_fixed_width_area3d(ScanParams *sp, double xx[], double yy[], double zz[], 
  int &scancount) 
{
  double a,m12,dx12;
  double d=0.0,dp,dx,dy,ddx,ddy,fx,fy,mp,xs,ys;
  int j,ix,iy,bpp,ino;
  zz=zz;
  ino=sp->ino;
  if(!between(ino, 0, g.image_count-1)) return;

  bpp=z[ino].bpp;
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
  if(fabs(m12)>1e-10) mp = -1.0/m12;  else mp = -100000000.0;
  dp = - 1;
  dx = sqrt((dp*dp)/(1.0+mp*mp));  // Incremental distance along perp.
  dx *= sgn(dp);
  dy = mp * dx;       
  xs = xx[1] + dx;                 // Starting coordinates
  ys = yy[1] + dy;
  //---Calculate the line parallel to 1--2.--------------------//
  //---Calculate the coords. of each point on this line.-------//
  fx = xs - z[ino].xpos; 
  fy = ys - z[ino].ypos;    // floating-point real coordinates
  for(j=0;j<scancount;j++)
  {    ix = int(fx);
       iy = int(fy);
       if(between(ix,0,z[ino].xsize-1) && between(iy,0,z[ino].ysize-1))
             sp->scan[j] = pixelat(z[ino].image[0][iy]+ix*g.off[bpp], bpp);
       else sp->scan[j] = g.bcolor;
       fx+=ddx;
       fy+=ddy; 
  } 
}


//--------------------------------------------------------------------------//
//  add_slider                                                              //
//--------------------------------------------------------------------------//
Widget add_slider(Widget parent, const char *name, int width, int left, int right, 
   int top, int bottom, int minval, int maxval, int startval, int increment)
{
   int n;
   Arg args[100];
   Widget w;
     
   n = 0;
   XtSetArg(args[n], XmNwidth, width); n++;      
   XtSetArg(args[n], XmNleftPosition, left); n++;      // % of width from left
   XtSetArg(args[n], XmNtopPosition, top); n++;        // % of height from top
   XtSetArg(args[n], XmNbottomPosition, bottom); n++;  // % of width from top
   XtSetArg(args[n], XmNrightPosition, right); n++;    // % of height from left
   XtSetArg(args[n], XmNfractionBase, 100); n++;       // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNminimum, minval); n++; 
   XtSetArg(args[n], XmNmaximum, maxval); n++; 
   XtSetArg(args[n], XmNprocessingDirection,XmMAX_ON_RIGHT); n++;      
   XtSetArg(args[n], XmNhighlightOnEnter, True); n++;      
   XtSetArg(args[n], XmNincrement, increment); n++; 
   XtSetArg(args[n], XmNvalue, startval); n++; 
   XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;      
#ifdef MOTIF2
   XtSetArg(args[n], XmNshowValue, XmNEAR_BORDER); n++;      
   XtSetArg(args[n], XmNsliderMark, XmTHUMB_MARK); n++;      
#ifndef LESSTIF_VERSION
   XtSetArg(args[n], XmNshowArrows, XmEACH_SIDE); n++;      
#endif
#else
   XtSetArg(args[n], XmNshowArrows, True); n++;      
#endif
   XtSetArg(args[n], XmNhighlightThickness, 1); n++;      
   ////  Stop Motif from trying to grab another color if none are available.   
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   w = XmCreateScale(parent, (char*)name, args, n);   
   XtManageChild(w);
   if(maxval == minval) XtVaSetValues(w, XmNeditable, 0, NULL);

   return w;
}
