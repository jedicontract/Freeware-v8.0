//--------------------------------------------------------------------------//
// xmtnimage54.cc                                                           //
// clickboxes                                                               //
// Latest revision: 07-27-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int in_clickboxcancelcb = 0;

//--------------------------------------------------------------------------// 
//  clickbox                                                                //
//  answer = int& to put result in                                          //
//  fanswer = double& to put result in if getting a double                  //
//  f1 = user drags slider   - gets int                                     //
//  f2 = user drags slider   - gets double                                  //
//  f5 = user clicks ok      - gets clickboxinfo*                           //
//  f6 = user clicks cancel  - gets clickboxinfo*                           //
//  ptr = user data for f5 & f6                                             // 
//  identifier = int provided by caller, put in clickbox for f5 & f6, so    //
//     a common func can handle several types of clickboxes using a switch  //
//     statement, rather than having numerous small funcs. (e.g. cset()).   //
//     Typically f5 would be null() if cset is used. 0 = not wanted.        // 
//  cset is in xmtnimage45.cc                                               //
//--------------------------------------------------------------------------// 
void clickbox(const char* title, int identifier, int *answer, int minval, int maxval,
     void f(int answer), void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *client_data, Widget parent, int helptopic)
{
  static double fanswer = 0.0, factor = 1.0;
  clickbox(title, identifier, answer, &fanswer, factor,
           *answer, minval, maxval, f, null, f5, f6, client_data, 
           parent, 0, INTEGER, helptopic);
}

void clickbox(const char* title, int identifier, double *fanswer, 
     double fminval, double fmaxval, 
     void f(double answer), void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *client_data, Widget parent, int helptopic)
{
  static int answer=0;
  int startval, minval, maxval;
  double factor = pow(10, g.signif);
  startval = (int)(*fanswer * factor);
  minval = (int)(fminval * factor);
  maxval = (int)(fmaxval * factor);
  clickbox(title, identifier, &answer, fanswer, factor,
           startval, minval, maxval, null, f, f5, f6, client_data, 
           parent, g.signif, FLOAT, helptopic);
}
void clickbox(const char* title,  int identifier, int *answer, 
     double *fanswer, double factor,
     int startval, int minval, int maxval, 
     void f1(int answer),  void f2(double answer), 
     void f5(clickboxinfo *c), void f6(clickboxinfo *c), 
     void *client_data, Widget parent, int decimalpoints, 
     int type, int helptopic)
{
  Widget form, slider, text, okaybut, cancbut, helpbut;
  // Waiting for input; don't start, otherwise becomes orphan 
  // (see clickboxcancelcb)
  //  if(g.block) return;  
  if(g.diagnose){ printf("clickbox %s\n",title); fflush(stdout); }
  static p3d *p;
  f1=f1; f2=f2;
  p = new p3d;
  p->c = new clickboxinfo[2];
  int k, n, wc=0, xsize=340, ysize=200, ostate;
  Arg args[100];
  static char *textlabel;
  textlabel = new char[FILENAMELENGTH];
  p->parent = parent;
  p->ptr[1] = (void*)textlabel;
  p->ptr[2] = (void*)answer;
  p->ptr[3] = (void*)fanswer;
  p->client_data = client_data;
  p->c[0].f1 = f1;    p->c[1].f1 = f1;
  p->c[0].f2 = f2;    p->c[1].f2 = f2;
  p->c[0].f3 = null;  p->c[1].f3 = null;
  p->c[0].f4 = null;  p->c[1].f4 = null;
  p->c[0].f5 = f5;    p->c[1].f5 = f5;
  p->c[0].f6 = f6;    p->c[1].f6 = f6;
  p->c[0].f7 = null;  p->c[1].f7 = null;
  p->c[0].f8 = null;  p->c[1].f8 = null;
  p->c[0].identifier = identifier;
  p->c[1].identifier = identifier;
  p->c[0].ptr[1] = p->ptr[1];
  p->c[0].ptr[2] = p->ptr[2];
  p->c[0].ptr[3] = p->ptr[3];
  p->c[0].client_data = client_data;  // Generic data for f5 and f6
  p->c[1].client_data = client_data;  // Generic data for f5 and f6
  p->c[0].answers = new int[10];      // Must be 10 for multiclickbox cb
  p->c[1].answers = p->c[0].answers;  // Only allocate one answers array

  //// client_data can be a int& or dialog*. Interpretation is done by f5
  int *iii = (int*)client_data;
  if(iii == NULL) p->param[0] = 0; 
  else p->param[0] = *iii;
  
  startval = max(startval, min(startval,maxval));
  ostate = g.state;
  g.state = MESSAGE;  // Prevent graph from obscuring it
  if(type==INTEGER) itoa(startval, textlabel, 10);
  else doubletostring(startval/pow(10, g.signif), g.signif, textlabel);
  p->param[1] = ostate;
   
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
  form = p->w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"ClickboxForm", args, n);

  ////  Title for dialog box
  p->w[wc++] = addlabel(form,(char*)title,CENTER,1,2,99,15);

  ////  Text area
  n = 0;
  XtSetArg(args[n], XmNcolumns, 40); n++;
  XtSetArg(args[n], XmNrows, 1); n++;
  XtSetArg(args[n], XmNvalue, textlabel); n++;
  XtSetArg(args[n], XmNleftPosition, 5); n++;     // % of width from left
  XtSetArg(args[n], XmNrightPosition, 95); n++;   // % of height from left
  XtSetArg(args[n], XmNtopPosition, 20); n++;     // % of height from top
  XtSetArg(args[n], XmNbottomPosition, 40); n++;  // % of width from top
  XtSetArg(args[n], XmNfractionBase, 100); n++;   // Use percentages
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
  text = p->w[wc++] = XmCreateTextField(form, textlabel, args, n);
  XtManageChild(text);
   
  ////  Slider
  n = 0;
  XtSetArg(args[n], XmNwidth, max(200,xsize-20)); n++;      
  XtSetArg(args[n], XmNleftPosition, 5); n++;     // % of width from left
  XtSetArg(args[n], XmNrightPosition, 95); n++;   // % of height from left
  XtSetArg(args[n], XmNtopPosition, 50); n++;     // % of height from top
  XtSetArg(args[n], XmNbottomPosition, 75); n++;  // % of width from top
  XtSetArg(args[n], XmNfractionBase, 100); n++;   // Use percentages
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNminimum, minval); n++; 
  XtSetArg(args[n], XmNmaximum, maxval); n++; 
  XtSetArg(args[n], XmNprocessingDirection,XmMAX_ON_RIGHT); n++;      
  XtSetArg(args[n], XmNhighlightOnEnter, True); n++;      
  XtSetArg(args[n], XmNincrement, 1); n++; 
  XtSetArg(args[n], XmNvalue, startval); n++; 
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;      
  XtSetArg(args[n], XmNscaleHeight, 25); n++;      
#ifdef MOTIF2
  XtSetArg(args[n], XmNshowValue, XmNEAR_BORDER); n++;      
#ifndef LESSTIF_VERSION
  XtSetArg(args[n], XmNshowArrows,  XmEACH_SIDE); n++;      
#endif
#else
  XtSetArg(args[n], XmNshowArrows, True); n++;      
#endif
  XtSetArg(args[n], XmNdecimalPoints, decimalpoints); n++;      
  XtSetArg(args[n], XmNhighlightThickness, 1); n++;      
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  slider = p->w[wc++] = XmCreateScale(form, (char*)"Value", args, n);
  XtManageChild(slider);


  ////  Now add callbacks so slider can change text area
  p->c[0].title = textlabel;
  p->c[0].answer = startval;
  p->c[0].fanswer = startval;
  p->c[0].startval = startval;
  p->c[0].minval = minval;
  p->c[0].maxval = maxval;
  p->c[0].type = type;
  p->c[0].wantpreview = 0;
  p->c[0].decimalpoints = decimalpoints;
  p->c[0].form = form;
  p->c[0].widget[0] = slider;
  p->c[0].noofbuttons = 2;
  XtAddCallback(text, XmNvalueChangedCallback, (XtCBP)clickboxcb, (XtP)&p->c[0]);

  p->c[1].title = textlabel;
  p->c[1].answer = startval;
  p->c[1].fanswer = startval;
  p->c[1].startval = startval;
  p->c[1].minval = minval;
  p->c[1].maxval = maxval;
  p->c[1].type = type;
  p->c[1].wantpreview = 0;
  p->c[1].form = form;
  p->c[1].widget[0] = text;
  p->c[1].decimalpoints = max(0,decimalpoints);
  XtAddCallback(slider, XmNdragCallback, (XtCBP)slidercb, (XtP)&p->c[1]);
  XtAddCallback(slider, XmNvalueChangedCallback,(XtCBP)slidercb, 
       (XtP)&p->c[1]);

  for(k=0;k<wc;k++)
       XtAddEventHandler(p->w[k], KeyPressMask, FALSE, (XtEH)entercb, (XtP)form);
  
  ////  Ok, Cancel, Help buttons & their callbacks
  okaybut = p->w[wc++] = add_button(form, (char*)"OK",   1, 1, 100);
  cancbut = p->w[wc++] = add_button(form, (char*)"Cancel",115, 6, 100);
  helpbut = p->w[wc++] = add_button(form, (char*)"Help",   225, 6, 100);

  XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)clickboxokcb, (XtP)p);
  XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)clickboxcancelcb, (XtP)p);
  XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(form, XmNunmapCallback, (XtCBP)clickboxunmapcb, (XtP)p);

  ////  Make Enter & Escape Keys the same as Mouse Click On The Buttons.  
  ////  Add handler so hitting Return activates Ok button
  XtAddEventHandler(text, KeyPressMask, FALSE, (XtEH)clickboxkeycb, (XtP)p);

  p->clickboxcount = 2;
  p->wc = wc;
  p->form = form;
  p->c[0].factor = factor;
  p->c[1].factor = factor;
  XtManageChild(form);
  g.waiting++;
}


//--------------------------------------------------------------------------//
//  clickboxkeycb                                                           //
//--------------------------------------------------------------------------//
void clickboxkeycb(Widget w, XtP client_data, XEvent *event)
{
   w=w; client_data=client_data; 
   int key = which_key_pressed(event);
   if(key==XK_Return) clickboxokcb(w, client_data, NULL);
   if(key==XK_Escape) clickboxcancelcb(w, client_data, NULL);
}


//--------------------------------------------------------------------------//
//  clickboxokcb                                                            //
//--------------------------------------------------------------------------//
void clickboxokcb(Widget w, XtP client_data, XmACB *call_data)
{
   static int *aaa;
   static double *bbb;
   int j,k,box=-1;
   Widget gparent=0;
   call_data=call_data;
   p3d *p = (p3d*)client_data;   
   if(p==NULL) return;
   dialoginfo *a = (dialoginfo *)p->client_data;
   clickboxinfo *c = p->c;
   if(c==NULL) return;
   aaa = (int*)c->ptr[2];
   int answer = 0;
   if(aaa != NULL) answer = *aaa;
   bbb = (double*)p->ptr[3];
   double fanswer = 0.0;
   if(bbb != NULL) fanswer = *bbb;

   ////  Figure out which clickbox if called by dialog box
   ////  Ordinary clickboxes must set parent to NULL
  
   if(p->parent != NULL)   
   {  
       gparent = XtParent(p->parent);
       if(a!=NULL && XtIsManaged(gparent))
       {
           a->w[0] = p->parent;
           if(a->w[0]==NULL) return;
           for(k=0; k<4; k++)
           for(j=0; j<a->noofboxes; j++)
               if(p->parent == a->boxwidget[j][k] && p->parent!=0) box = j;
           a->param[0] = box;
           if(box<0) return;
       }
   }

   Widget slider = p->c[0].widget[0];
   XmScaleGetValue(slider, &answer);
   if(p->c[0].type == FLOAT && p->c[0].factor != 0.0) 
       fanswer = (double)answer / p->c[0].factor;
   c->answer = answer;
   c->fanswer = fanswer;
   //// Don't call f5 if user closed parent dialog window using window manager
   if(p->c[0].f5 != NULL && p->c != NULL)
   {   if(p->parent==NULL) p->c[0].f5(p->c);
       else if(XtIsManaged(gparent)) p->c[0].f5(p->c);
   }
   if(g.diagnose)
   {   
       if(p->c[0].type == FLOAT) printf("clickbox float answer=%g\n",fanswer); 
       else                      printf("clickbox int answer=%d\n",answer);
   }
   clickboxcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  clickboxunmapcb                                                         //
//--------------------------------------------------------------------------//
void clickboxunmapcb(Widget w, XtP client_data, XmACB *call_data)
{ 
   if(!in_clickboxcancelcb) clickboxcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  clickboxcancelcb                                                        //
//  Don't reference dialogbox in case clickbox is called by iteslf          //
//--------------------------------------------------------------------------//
void clickboxcancelcb(Widget w, XtP client_data, XmACB *call_data)
{
  int k;
//  if(g.block) return;  // Waiting for input; don't deallocate
  in_clickboxcancelcb = 1;
  w=w;call_data=call_data;
  p3d *p = (p3d*)client_data;

  p->c[0].f6(p->c);
  g.state = p->param[1];
  XtUnmanageChild(p->form);
  //// Only 1st one should ever be allocated
  if(p->clickboxcount>=1 && p->c[0].answers != NULL) delete[] p->c[0].answers;

  for(k=0; k<p->wc; k++) XtDestroyWidget(p->w[k]);
  delete[] (char*)p->ptr[1]; // textlabel
  delete[] p->c;
  delete p;   
  g.waiting = max(0, g.waiting-1);
  in_clickboxcancelcb = 0;
  if(g.diagnose) printf("clickbox done\n"); 
}


//--------------------------------------------------------------------------// 
//  getstrings - dims = no.of columns nstrings = no. of rows                //
//--------------------------------------------------------------------------// 
void getstrings(const char *title, char **label, char **headings, char ***answer, 
     int dims, int nstrings, int maxlen)
{
   Widget bb, scrollbb, **text, dialogshell, *w, scrolledwindow, okaybut, 
       cancbut,  helpbut;
   int h, k, n, ypos, wc=0, xsize, ostate, helptopic=0, want_block;
   int width, height, winheight, scrollheight;
   if(dims >= 256) { message("Too many strings"); return; }
   int stringleft[256];
   static int ingetstrings = 0;

   if(ingetstrings) return;
   ingetstrings = 1;

   Arg args[100];

   //// Increase size of this array if adding more widgets
   int MAXWIDGETS = nstrings*5 + 12 + 4; // 4 extra
   w = new Widget[MAXWIDGETS];
   text = new Widget*[dims];
   for(k=0; k<dims; k++) text[k] = new Widget[nstrings];
   xsize  = min(500, max(340, 200 + 7*strlen(label[0])));
   height = 18;         // y pixels between text areas
   width  = 50;         // x pixels between text areas

   ostate = g.state;
   g.state = MESSAGE;   // Prevent graph from obscuring it
   winheight = cint(height * nstrings + 80);  // Size of outer dialogshell
   if(headings != NULL) winheight += 20;
   scrollheight = winheight - 25;             // Size of scrolling window
   winheight = min(winheight, 300);

   //------- Create a dialog shell widget ----------------------------//
   // Can't use form widget because Motif internally 
   // converts Position arguments to ints. This causes the boxes to be 
   // all different sizes.

   dialogshell = w[wc++] = XtVaCreateManagedWidget("GetStrings",
                                xmDialogShellWidgetClass,  g.main_widget,
                                XmNx,                      g.main_xpos+100,
                                XmNy,                      g.main_ypos+100,
                                XmNwidth,                  xsize,
                                XmNheight,                 winheight,
                                XmNminWidth,               xsize+2,
                                XmNminHeight,              winheight,
                                XmNmaxHeight,              winheight,
                                XmNresizable,              False,
                                XmNtitle,                  "Enter Data",
                                XmNautoUnmanage,           False,
                                NULL);

   //------- Create a bulletin board widget --------------------------//

   n=0;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, winheight); n++;
   XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   bb = w[wc++] = XmCreateBulletinBoard(dialogshell, (char*)"GetStringsBB", args, n);

   //------- Create a scrolled window widget -------------------------//

   n=0;
   XtSetArg(args[n], XmNx, 0); n++;
   XtSetArg(args[n], XmNy, 0); n++;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, winheight-25); n++;
   XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   scrolledwindow = w[wc++] = XmCreateScrolledWindow(bb, (char*)"GetStrings", args, n);

   //------- Create a bulletin board widget on the scrolled window --//

   n=0;
   XtSetArg(args[n], XmNx, 0); n++;
   XtSetArg(args[n], XmNy, 0); n++;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, scrollheight); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   XtSetArg(args[n], XmNtitle, title); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   scrollbb = w[wc++] = XmCreateBulletinBoard(scrolledwindow, (char*)"GetStringsBB", args, n);

   //--------Title for dialog box-------------------------------------//

   w[wc++] = addbblabel(scrollbb, (char*)title, CENTER, 1.0, 5.0);

   //--------Text area------------------------------------------------//

   stringleft[0] = 4;
   for(h=1; h<=dims; h++) 
       stringleft[h] = stringleft[h-1] + width + 10;

   ypos = cint(1.5 * height);
   if(headings != NULL)
   {   for(h=0; h<dims; h++)
          w[wc++] = addbblabel(scrollbb, headings[h], LEFT, stringleft[h], 1.5*height);
       ypos += height;
   }
   for(k=0; k<nstrings; k++)
   {   //// label at right of text box
       w[wc++] = addbblabel(scrollbb, label[k], LEFT, stringleft[dims], ypos);
       if(wc >= MAXWIDGETS){ message("Too much data for mini spreadsheet"); return; }
       for(h=0; h<dims; h++)
       {   n = 0;
           XtSetArg(args[n], XmNx, stringleft[h]); n++;
           XtSetArg(args[n], XmNy, ypos); n++;    
           XtSetArg(args[n], XmNwidth, width); n++;
           XtSetArg(args[n], XmNheight, height); n++;
           XtSetArg(args[n], XmNresizable, False); n++;
           XtSetArg(args[n], XmNrows, 1); n++;
           XtSetArg(args[n], XmNcolumns, 10); n++;
           XtSetArg(args[n], XmNmarginHeight, 0); n++;
           XtSetArg(args[n], XmNmarginWidth, 0); n++;
           XtSetArg(args[n], XmNmaxLength, maxlen); n++;
           XtSetArg(args[n], XmNvalue, answer[h][k]);n++; 
           XtSetArg(args[n], XmNautoShowCursorPosition, True); n++;
           XtSetArg(args[n], XmNcursorPositionVisible, True); n++;
           ////  Stop Motif from trying to grab another color if none are available.
           if(g.want_colormaps)  
           {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
           }
           ////  Motif ignores this for some reason
           XtSetArg(args[n], XmNcursorPosition, (int)strlen(answer[h][k])); n++;
           text[h][k] = w[wc++] = XmCreateText(scrollbb, label[k], args, n);
           XtManageChild(text[h][k]);
           XtAddCallback(text[h][k], XmNvalueChangedCallback, 
               (XtCBP)getstringcb, (XtP)answer[h][k]);
       }
       ypos += height;
   }
  
   //--------Ok, Cancel, Help buttons & their callbacks---------------//

   okaybut = w[wc++] = add_bb_button(bb, (char*)"Accept",   1, cint(winheight-25), 100, 16, 1);
   cancbut = w[wc++] = add_bb_button(bb, (char*)"Dismiss",115, cint(winheight-21), 100, 16, 0);
   helpbut = w[wc++] = add_bb_button(bb, (char*)"Help",   225, cint(winheight-21), 100, 16, 0);

   XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)getstringokcb, (XtP)&want_block);
   XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)getstringcancelcb, (XtP)&want_block);
   XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  
   ////  Add callbacks so hitting Return activates Ok button
   for(k=0; k<wc; k++)
      XtAddEventHandler(w[k], KeyPressMask, FALSE, (XtEH)getstringentercb, (XtP)scrollbb);

   XtManageChild(dialogshell);
   XtManageChild(bb);
   XtManageChild(scrolledwindow);
   XtManageChild(scrollbb);

   wc = wc;
   want_block = 1;
   block(dialogshell, &want_block);  // Don't continue until they comply

   printstatus(MESSAGE, (char*)"Deleting widgets"); // Amazingly slow
   XtUnmanageChild(scrollbb);
   XtUnmanageChild(scrolledwindow);
   XtUnmanageChild(dialogshell);
   g.state = ostate;
   for(k=0; k<dims; k++) delete[] text[k];
   for(k=0; k<wc; k++)
   {   XtRemoveEventHandler(w[k],KeyPressMask,FALSE,(XtEH)entercb,(XtP)scrollbb);
       XtDestroyWidget(w[k]);   
   }
   delete[] text;  
   ingetstrings = 0;
   return;
}


//--------------------------------------------------------------------//
// getstringentercb                                                   //
//--------------------------------------------------------------------//
void getstringentercb(Widget w, XtP client_data, XEvent *event)
{
  int *want_block = (int*) client_data;
  *want_block = 0;
  entercb(w, client_data, event);
}


//--------------------------------------------------------------------------//
// getstringokcb                                                            //
//--------------------------------------------------------------------------//
void getstringokcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data;call_data=call_data;  // Keep compiler quiet
  int *want_block = (int*) client_data;
  *want_block = 0;
}


//--------------------------------------------------------------------------//
// getstringcancelcb                                                        //
//--------------------------------------------------------------------------//
void getstringcancelcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  int *want_block = (int*) client_data;
  *want_block = 0;
}


//--------------------------------------------------------------------------//
// getstringcb - callback for getting a string in getstring.                //
//--------------------------------------------------------------------------//
void getstringcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  char *ptr;
  char *a = (char*)client_data;
  ptr = XmTextGetString(w);
  if(ptr != NULL){ strcpy(a, ptr); XtFree(ptr); }
  return;
}


//--------------------------------------------------------------------------//
// getbox                                                                   //
// General routine for getting upper left and lower right coordinates       //
// of a screen rectangle with the mouse.                                    //
// The 4 parameters are passed by reference. They are automatically         //
// sorted before being returned. If user clicks right button, the           //
// parameters are left unchanged.                                           //
// Returns the mouse button clicked.                                        //
//--------------------------------------------------------------------------//
int getbox(int &x1,int &y1,int &x2,int &y2)
{ 
   drawselectbox(OFF);
   g.state = GETBOX;
   g.selected_is_square=1;
   g.getout = 0;
   g.getboxstate = 1;
   if(g.diagnose) printf("**blocking**\n");
   block(g.main_widget, &g.getboxstate);
   if(g.diagnose) printf("**unblocking**\n");
   x1 = g.get_x1;  
   y1 = g.get_y1;
   x2 = g.get_x2;  
   y2 = g.get_y2;
   if(x1>x2) swap(x1,x2);
   if(y1>y2) swap(y1,y2);
   drawselectbox(OFF);
   return g.mouse_button;
}
 

//--------------------------------------------------------------------------//
// getpoint                                                                 //
// General routine for getting a x,y point with the mouse.                  //
// Returns the mouse button clicked, so can also wait for a button.         //
//--------------------------------------------------------------------------//
int getpoint(int &x,int &y)
{ 
   g.getpointstate = 1;
   block(g.main_widget, &g.getpointstate);
   x = g.get_x1;  
   y = g.get_y1;
   return g.mouse_button;
}


//--------------------------------------------------------------------------//
// getline                                                                  //
//--------------------------------------------------------------------------//
void getline(int &x1, int &y1, int &x2, int &y2)
{
   drawselectbox(OFF);
   g.state = GETLINE;
   g.draw_figure = TEMPLINE;   
   g.getlinestate = 1;
   block(g.main_widget, &g.getlinestate);
   g.draw_figure = NONE;   
   x1 = g.get_x1;  
   y1 = g.get_y1;
   x2 = g.get_x2;  
   y2 = g.get_y2;
   drawselectbox(OFF);
}


//--------------------------------------------------------------------------//
// getboxposition                                                           //
// Same as getpoint except XOR's a box of width w and height h.             //
// Returns the mouse button clicked, so can also wait for a button.         //
// Don't confuse with getbox which gets four corners of a box.              //
//--------------------------------------------------------------------------//
int getboxposition(int &x,int &y, int w, int h)
{ 
   g.getpointstate = 1;
   g.state = GETBOXPOSITION;
   g.selected_is_square=1;
   g.getout = 0;
   g.getpointstate = 1;
   g.width = w;
   g.height = h;
   g.draw_figure = BOX;
   draw(START);
   block(g.main_widget, &g.getpointstate);
   g.state = NORMAL; 
   g.draw_figure = NONE;
   x = g.get_x1;  
   y = g.get_y1;
   return g.mouse_button;
}




//--------------------------------------------------------------------------//
// getinteger                                                               //
// Puts integer in 'answer' when user clicks OK. Doesn't block. Caller      //
// supplies f5 and f6 which process result when user clicks OK or Cancel.   //
// NOTE: use f5 and f6 to obtain the result.                                //
//--------------------------------------------------------------------------//
void getinteger(const char *title, int *answer, int minval, int maxval, 
    void f5(clickboxinfo *c), 
    void f6(clickboxinfo *c), 
    void *client_data, 
    int helptopic)
{   
   clickbox(title, 0, answer, minval, maxval, null, f5, f6, client_data, 
       NULL, helptopic); 
}


//--------------------------------------------------------------------------// 
// getcolor - enter a color value or RGB values if in color mode.           //
//           Returns answer in a ref so that starting value is shown.       //
//           The optional parameter bpp is bits/pixel to use. If 0, it      //
//             uses current screen bitsperpixel.                            //
//           The optional parameter cmyk changes the printed strings        //
//             only, for getting cmy values; can be 0 or 1.                 //
// Answer is changed via the pointer '*answer' = c->ptr[2].                 //
// Caller data in 'client_data' is passed to callbacks via c->client_data.  //
//--------------------------------------------------------------------------//
void getcolor(const char* title, uint *answer, int bpp, int cmyk, 
     void(*f7)(clickboxinfo *c), void *client_data)
{
   int color = *answer;
   int c=0,rr,gg,bb;
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   static clickboxinfo* item;
   if(bpp==0) bpp=g.bitsperpixel;
   //   if(bpp==8)
   //   {   getinteger("Pixel value", (int*)answer, 0, 255, f7, null, client_data, 0);
   //   }else{
   item = new clickboxinfo[3];
   item[0].title = new char[128];
   valuetoRGB(color,rr,gg,bb,bpp);
   if(cmyk)
      strcpy(item[0].title,"Cyan");
   else
      strcpy(item[0].title,"Red");
   item[0].startval = rr;
   item[0].minval = 0;
   item[0].maxval = g.maxred[bpp];
   item[0].type = VALSLIDER;
   item[0].answer = *answer;
   item[0].answers = new int[10]; // Must be 10 for multiclickbox cb
   item[0].wantdragcb = 1;
   item[0].wantpreview = 0;

   item[1].title = new char[128];
   if(cmyk)
      strcpy(item[1].title,"Magenta");
   else
      strcpy(item[1].title,"Green");
   item[1].startval = gg;
   item[1].minval = 0;
   item[1].maxval = g.maxgreen[bpp];
   item[1].type = VALSLIDER;
   item[1].wantdragcb = 1;

   item[2].title = new char[128];
   if(cmyk)
      strcpy(item[2].title,"Yellow");
   else
      strcpy(item[2].title,"Blue");
   item[2].startval = bb;
   item[2].minval = 0;
   item[2].maxval = g.maxblue[bpp];
   item[2].type = VALSLIDER;
   item[2].wantdragcb = 1;

   item[0].ino = ci;
   item[0].bpp = bpp;
   item[0].client_data = client_data; // dialoginfo *
   item[0].ptr[0] = client_data;      // dialoginfo *
   item[0].ptr[2] = answer;
   item[0].noofbuttons = 1;
   item[0].f1 = null;
   item[0].f2 = null;
   item[0].f3 = null;
   item[0].f4 = null;
   item[0].f5 = getcolorok;
   item[0].f6 = getcolorfinish;
   item[0].f7 = f7;
   item[0].f8 = (XtCBP)armboxcb;
   if(cmyk) multiclickbox(title, 3, item, null, 0);
   else     multiclickbox(title, 3, item, samplecolor, 0);
   if(g.getout){ c=color; g.getout=0; }
   return;
} 


//-------------------------------------------------------------------------//
// getcolorok                                                              //
//-------------------------------------------------------------------------//
void getcolorok(clickboxinfo *c)
{
  int rr,gg,bb;
  if(c[0].noofbuttons == 3)
  {    rr = c[0].answer;
       gg = c[1].answer;
       bb = c[2].answer;
       c[0].answer = RGBvalue(rr,gg,bb,24);
       c[0].answers[0] = rr;
       c[0].answers[1] = gg;
       c[0].answers[2] = bb;
  }
  int *answer = (int *)c[0].ptr[2];
  *answer = c[2].answer;
}



//-------------------------------------------------------------------------//
// getcolorfinish                                                          //
//-------------------------------------------------------------------------//
void getcolorfinish(clickboxinfo *c)
{
  delete[] c[0].answers;
  delete[] c[0].title;
  if(c->noofbuttons == 3)
  {   delete[] c[1].title;
      delete[] c[2].title;
  }
  delete[] c;                                   
}


//--------------------------------------------------------------------------//
// clickboxcb - callback for getting a value in dialog box.  Called         //
// by a text widget. Reads the Widget's string, converts it to an           //
// integer, checks it against the values in the clickboxinfo struct,        //
// and sets the slider value in the 'c->widget' (which must be a            //
// XmScale).                                                                // 
//--------------------------------------------------------------------------//
void clickboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  double fanswer=0;
  char *string = NULL;
  clickboxinfo *c = (clickboxinfo*)client_data;
  string = XmTextGetString(w);  // This allocates memory for string
  if(string != NULL && c != NULL) 
  {   if(c->type == INTEGER) 
      {    answer = atoi(string);
           answer = max(min(answer,c->maxval),c->minval);
           XmScaleSetValue(c->widget[0], answer);
           if(c->answers != NULL)
           {
                if(between(c->k, 0, 20)) c->answers[c->k] = answer;
           }
      }
      if(c->type == FLOAT) 
      {    fanswer = atof(string) * 1.00000001 * pow(10, g.signif);
           c->fanswer = fanswer;
           fanswer = max(min(fanswer,c->maxval),c->minval);
           XmScaleSetValue(c->widget[0], (int)(fanswer));
      }
      XtFree(string);
  }
}


//--------------------------------------------------------------------------//
// slidercb - Called by a slider widget, reads the slider's value,          //
// Widget's string, converts it to a string, and sets the text in the       //
// 'c->widget' (which must be a XmText).                                    //
//--------------------------------------------------------------------------//
void slidercb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  double fanswer=0.0;
  char *string = new char[256];
  strcpy(string," ");
  clickboxinfo *c = (clickboxinfo*)client_data;
  XmScaleGetValue(w, &answer);
  g.getout = 0;

  switch(c->type)
  {     case INTEGER: 
             c->f1(answer);  
             itoa(answer, string, 10); 
             break;
        case FLOAT:
             fanswer = ((double)answer)/pow(10, g.signif);  
             c->fanswer = fanswer;
             c->f2(fanswer); 
             sprintf(string, "%g", fanswer); 
             break;
        case INTARRAY:
             c->answers[c->k] = answer;
             c->f3(c->answers);
             itoa(c->answers[c->k], string, 10); 
             break;
        case PARAM_ARRAY:   // Int array using c->param[] which is static
             c->param[c->k] = answer;
             c->f3(c->param);
             itoa(c->param[c->k], string, 10); 
             break;
  }
  c->answer = answer;
  XmTextSetString(c->widget[0], string);
  delete[] string;
}


//--------------------------------------------------------------------------//
// slider_nof_cb - Same as slidercb except doesn't call function.           //
//--------------------------------------------------------------------------//
void slider_nof_cb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  double fanswer=0.0;
  char *string = new char[256];
  strcpy(string," ");
  clickboxinfo *c = (clickboxinfo*)client_data;
  XmScaleGetValue(w, &answer);

  switch(c->type)
  {     case INTEGER: 
             itoa(answer, string, 10); 
             break;
        case FLOAT:
             fanswer = ((double)answer)/pow(10, g.signif);  
             c->fanswer = fanswer;
             doubletostring(fanswer, g.signif, string); 
             break;
        case INTARRAY:
             c->answers[c->k] = answer;
             c->answer = answer;
             itoa(c->answers[c->k], string, 10); 
             break;
        case PARAM_ARRAY:   // Int array using c->param[] which is static
             c->param[c->k] = answer;
             itoa(c->param[c->k], string, 10); 
             break;
  }
  c->answer = answer;
  XmTextSetString(c->widget[0], string);
  delete[] string;
}


//--------------------------------------------------------------------------//
// slider_now_cb - Same as slidercb except doesn't call widget.             //
//--------------------------------------------------------------------------//
void slider_now_cb(Widget w, XtP client_data, XmACB *call_data)
{ 
  w=w;
  call_data=call_data;  // Keep compiler quiet
  int answer=0;
  double fanswer=0.0;
  clickboxinfo *c = (clickboxinfo*)client_data;
  switch(c->type)
  {     case INTEGER: c->f1(answer); break;
        case FLOAT:   c->f2(fanswer); break;
        case INTARRAY: c->f3(c->param); break;
        case PARAM_ARRAY: c->f3(c->param); break;
  }
  c->answer = answer;
}

