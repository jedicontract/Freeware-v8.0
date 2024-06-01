//--------------------------------------------------------------------------//
// xmtnimage58.cc                                                           //
// multiclickboxes                                                          //
// Latest revision: 04-01-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int in_multiclickboxcancelcb = 0;

//--------------------------------------------------------------------------//
// multiclickbox                                                            //
// Puts integers into clickboxinfo struct by click/drag with the mouse.     //
// 'getout' must be reset to 0 by the calling routine.                      //
// Returns error code (0=OK)                                                //
// Calls ff(), which is a calling-routine-specific function, whenever value //
//     is changed or mouse is clicked. This allows passed function f() to   //
//     add new buttons.                                                     //
// c->boxtype is 0=COUNTER, 1=SLIDER, 2=VALSLIDER  3=BUTTONVALSLIDER        //
// If 'c[0].wantpreview'==1, it adds "preview" button and only calls f()    //
//     when clicked; otherwise, it calls f() continuously.                  //
// Only c->f3 is meaningful in multiclickbox. All client c->f's are ignored //
//     when ok or cancel are hit. Client c->f1 to f4 are applied to button  //
//     if type is BUTTONVALSLIDER.                                          //
// Don't call with noofboxes=1, use getinteger() or getstring() instead.    //
// Clients calling dialogbox() with MULTICLICKBOX set must set dialog->f8.  //
//--------------------------------------------------------------------------//
int multiclickbox(const char* title, int noofboxes, clickboxinfo* c,
      void ff(int values[10]), int helptopic)
{
  Widget form, okaybut, cancbut, helpbut, www;
  int k, n, wc=0, xsize, y, ypct, ysize, boxsize;
  Arg args[100];
  if(g.block) return OK;  // Waiting for input; don't start, otherwise becomes orphan

  static p3d *p;
  p = new p3d;

  static char *textlabel;
  p->clickboxdata[0] = new clickboxinfo[noofboxes];
  p->clickboxdata[1] = new clickboxinfo[noofboxes];
  textlabel = new char[FILENAMELENGTH];
  p->ptr[1] = textlabel;
  c[0].noofbuttons = noofboxes;
  c[0].k = 0;
  
  xsize = 425;
  ysize = 150 + 20*noofboxes;            // overall height in pixels
  int SCALESTART = 3;
  int LABELSTART = 64;
  int LABELEND = 85; 
  for(k=0; k<noofboxes; k++) 
     if(c[k].type == BUTTONVALSLIDER)
        { SCALESTART=18; LABELSTART=77; LABELEND=86; break; }
   
  ypct = 100/(noofboxes+2);             // % of y height for each box
  boxsize = (int)(0.9*ypct);            // height of each box in %

  ////  Create a form dialog shell widget
  n=0;
  XtSetArg(args[n], XmNwidth, xsize); n++;
  XtSetArg(args[n], XmNheight, ysize); n++;
  XtSetArg(args[n], XmNresizable, False); n++;
  XtSetArg(args[n], XmNautoUnmanage, False); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtitle, title); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  form = p->w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"MultiClickboxForm", args, n);
  p->w[wc++] = addlabel(form,(char*)title,CENTER,1,1,99,15);
  y = 15;
  for(k=0; k<noofboxes; k++)
  {  
       c[0].answers[k] = c[k].startval;

       p->w[wc++] = addlabel(form,c[k].title,LEFT,LABELSTART,y+boxsize/4,
                 LABELEND,y+boxsize+boxsize/4);
       itoa(c[k].startval, textlabel, 10);

       n = 0;
       XtSetArg(args[n], XmNwidth, 150); n++;      
       XtSetArg(args[n], XmNleftPosition, SCALESTART); n++;   // % of width from left
       XtSetArg(args[n], XmNtopPosition, y); n++;             // % of height from top
       XtSetArg(args[n], XmNbottomPosition, y+boxsize); n++;  // % of width from top
       XtSetArg(args[n], XmNrightPosition, LABELSTART); n++;// % of height from left
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNminimum, c[k].minval); n++; 
       XtSetArg(args[n], XmNmaximum, c[k].maxval); n++; 
       XtSetArg(args[n], XmNprocessingDirection,XmMAX_ON_RIGHT); n++;      
       XtSetArg(args[n], XmNhighlightOnEnter, True); n++;      
       XtSetArg(args[n], XmNincrement, 1); n++; 
       XtSetArg(args[n], XmNvalue, c[k].startval); n++; 
       XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;      
       XtSetArg(args[n], XmNshowValue, XmNONE); n++;      
       XtSetArg(args[n], XmNdecimalPoints, 0); n++;      
#ifdef MOTIF2
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
       p->slider[k] = p->w[wc++] = XmCreateScale(form, (char*)"MultiClickBoxScale", args, n);
       XtManageChild(p->slider[k]);
 
       ////  Text area
       n = 0;
       XtSetArg(args[n], XmNcolumns, 12); n++;
       XtSetArg(args[n], XmNrows, 1); n++;
       XtSetArg(args[n], XmNvalue, textlabel); n++;
       XtSetArg(args[n], XmNwidth, 50); n++;      
       XtSetArg(args[n], XmNleftPosition, LABELEND+1); n++;   // % of width from left
       XtSetArg(args[n], XmNtopPosition, y+boxsize/8); n++;   // % of height from top
       XtSetArg(args[n], XmNbottomPosition, y+boxsize); n++;  // % of width from top
       XtSetArg(args[n], XmNrightPosition, 98); n++;          // % of height from left
       XtSetArg(args[n], XmNfractionBase, 100); n++;          // Use percentages
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
       XtSetArg(args[n], XmNmarginHeight, 3); n++;
       XtSetArg(args[n], XmNmarginWidth, 3); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       p->clickboxdata[0][k].title = textlabel;   // copy char pointer
       p->clickboxdata[0][k].answer = c[k].startval;
           // An answers[] array has been allocated only for the first one.
           // All the data are accessible to all the other sliders, and all
           // are aliased to the same original array.
       p->clickboxdata[0][k].answers = c[0].answers; 
       p->clickboxdata[0][k].k = k;
       p->clickboxdata[0][k].startval = c[k].startval;
       p->clickboxdata[0][k].minval = c[k].minval;
       p->clickboxdata[0][k].maxval = c[k].maxval;
       p->clickboxdata[0][k].type = INTEGER;
       p->clickboxdata[0][k].wantpreview = 0;
       p->clickboxdata[0][k].widget[0] = p->slider[k];
       p->clickboxdata[0][k].f1 = null;
       p->clickboxdata[0][k].f2 = null;
       p->clickboxdata[0][k].f3 = ff;
       p->clickboxdata[0][k].f4 = NULL;
       p->clickboxdata[0][k].noofbuttons = noofboxes;
       p->text[k] = p->w[wc++] = XmCreateTextField(form, textlabel, args, n);
       XtAddCallback(p->text[k], XmNvalueChangedCallback,
          (XtCBP)clickboxcb, (XtP)&p->clickboxdata[0][k]);
       XtManageChild(p->text[k]);
 
       //// Now add a callback so slider can change text area
 
       p->clickboxdata[1][k].title = textlabel;
       p->clickboxdata[1][k].answer = c[k].startval;
       p->clickboxdata[1][k].answers = c[0].answers;
       p->clickboxdata[1][k].k = k;
       p->clickboxdata[1][k].startval = c[k].startval;
       p->clickboxdata[1][k].minval = c[k].minval;
       p->clickboxdata[1][k].maxval = c[k].maxval;
       p->clickboxdata[1][k].type = INTARRAY;
       p->clickboxdata[1][k].wantpreview = 0;
       p->clickboxdata[1][k].widget[0] = p->text[k];
       p->clickboxdata[1][k].decimalpoints = 0;
       p->clickboxdata[1][k].f1 = null;
       p->clickboxdata[1][k].f2 = null;
       p->clickboxdata[1][k].f3 = ff;
       p->clickboxdata[1][k].f4 = NULL;
       p->clickboxdata[1][k].noofbuttons = noofboxes;
       XtAddCallback(p->slider[k], XmNvalueChangedCallback,
          (XtCBP)slidercb, (XtP)&p->clickboxdata[1][k]);
       if(c[k].wantdragcb==1)
           XtAddCallback(p->slider[k], XmNdragCallback,
               (XtCBP)slidercb, (XtP)&p->clickboxdata[1][k]);
       else    //// Change label only, unless mouse released
           XtAddCallback(p->slider[k], XmNdragCallback,
               (XtCBP)slider_nof_cb, (XtP)&p->clickboxdata[1][k]);
               //// Put widget in original list for use by callback
       c[k].widget[0] = p->text[k];
       if(c[k].type == BUTTONVALSLIDER)
       {    www = p->w[wc++] = add_pushbutton(c[k].buttonlabel,form,1,y+4,SCALESTART-1,
               y+ypct-1,&c[k]);
            XtAddEventHandler(www, KeyPressMask, FALSE, (XtEH)multiclickboxkeycb, (XtP)p);
       }
       y+=ypct;
  }

  ////  Make Enter & Escape Keys the same as Mouse Click On The Buttons.  
  ////  Add handler so hitting Return activates Ok button
  for(k=0; k<noofboxes; k++)
      XtAddEventHandler(p->text[k], KeyPressMask, FALSE, (XtEH)multiclickboxkeycb, (XtP)p);

  okaybut = p->w[wc++] = add_button(form, (char*)"OK",       1, 1, 100);
  cancbut = p->w[wc++] = add_button(form, (char*)"Cancel", 115, 6, 100);
  helpbut = p->w[wc++] = add_button(form, (char*)"Help",   225, 6, 100);

  XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)multiclickboxokcb, (XtP)p);
  XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)multiclickboxcancelcb, (XtP)p);
  XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(form, XmNunmapCallback, (XtCBP)multiclickboxunmapcb, (XtP)p);

  p->clickboxcount = 2;
  p->c = c;
  p->wc = wc;
  p->form = form;
  XtManageChild(form);
  g.waiting++;                     // Don't dismiss dialog box
  return OK;
}


//--------------------------------------------------------------------------//
//  multiclickboxunmapcb                                                    //
//--------------------------------------------------------------------------//
void multiclickboxunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!in_multiclickboxcancelcb) multiclickboxcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  multiclickboxkeycb                                                      //
//--------------------------------------------------------------------------//
void multiclickboxkeycb(Widget w, XtP client_data, XEvent *event)
{
   w=w; client_data=client_data; 
   int key = which_key_pressed(event);
   if(key==XK_Return) multiclickboxokcb(w, client_data, NULL);
   if(key==XK_Escape) multiclickboxcancelcb(w, client_data, NULL);
}


//--------------------------------------------------------------------------//
//  multiclickboxokcb                                                       //
//--------------------------------------------------------------------------//
void multiclickboxokcb(Widget w, XtP client_data, XmACB *call_data)
{
  int k, butno;
  w=w;call_data=call_data;
  p3d *p = (p3d*)client_data;
  clickboxinfo *c = p->c;
  clickboxinfo **clickboxdata = p->clickboxdata;
  butno = 0;
  for(k=0; k<c[0].noofbuttons; k++) 
  {   //// Read value of slider widget (#1) & put in c.answer 
      XmScaleGetValue(p->slider[k], &clickboxdata[1][butno].answer);
      c[k].answer = clickboxdata[1][butno].answer;
      c[0].answers[k] = c[k].answer;
  }      
  c[0].f5(c);                      // This should normally call part 2 
  c[0].f7(c);                      // Extra client function
  ////  Call dialogbox OK function cb (c->ptr[0] = dialoginfo*) 
  ////  if called from a dialog box
  if(c->ptr[0] != NULL) c[0].f8(w, c->ptr[0], NULL);  
  ////  Auto unmanage
  multiclickboxcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  multiclickboxcancelcb                                                   //
//--------------------------------------------------------------------------//
void multiclickboxcancelcb(Widget w, XtP client_data, XmACB *call_data)
{
  int k;
  w=w;call_data=call_data;
  p3d *p = (p3d*)client_data;
  clickboxinfo *c = p->c;
  if(g.block) return;  // Waiting for input; don't deallocate everything yet
  in_multiclickboxcancelcb = 1;
  c[0].f6(c);
  XtUnmanageChild(p->form);
  delete[] (char*)p->ptr[1]; // textlabel
  for(k=0; k<p->wc; k++) XtDestroyWidget(p->w[k]);
  for(k=0;k<p->clickboxcount; k++) delete[] p->clickboxdata[k];
  delete p;   
  g.waiting = max(0, g.waiting-1);
  in_multiclickboxcancelcb = 0;
}

