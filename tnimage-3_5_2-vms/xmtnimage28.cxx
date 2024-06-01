//--------------------------------------------------------------------------//
// xmtnimage28.cc                                                           //
// dialog box                                                               //
// Latest revision: 09-04-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
int in_dialogcancelcb = 0;

//--------------------------------------------------------------------------//
// dialogbox (Motif)                                                        //
//  radiotype      method                                                   //
//  --------      -----------                                               //
//  RADIO         simple toggle radio button.                               //
//                                                                          //
//  boxtype       method                                                    //
//  -------       ---------------                                           //
//  TOGGLE        toggle button with label                                  //
//  TOGGLESTRING  toggle button with label and a string.                    //
//  INTCLICKBOX   integer clickbox() using sliders                          //
//  FILENAME      getfilename()                                             //
//  MULTIFILENAME getfilenames()                                            //
//  RGBCLICKBOX   rgbclickbox()(Assumes you want a rgb)                     //
//  TOGGLERGBCLICKBOX  toggle button with label and a multiclickbox.        //
//  NON_EDIT_LIST non-editable list                                         //
//  STRING        getstring                                                 //
//  LIST          editable list                                             //
//  LABEL         heading only                                              //
//  MULIILABEL    up to 4 columns of labels                                 //
//  NO_LABEL_LIST editable list except no label on button                   //
//  DOUBLECLICKBOX float clickbox() using sliders, uses clickboxstep[]      //
//  MULTIPUSHBUTTON row of pushbuttons                                      //
//                                                                          //
// NOTE: It is the calling routine's responsibility to restore the          //
//     original parameters if "Cancel" is selected, using the global        //
//     variable 'getout', and then to restore 'getout' to 0.                //
// All widgets and data are deleted automatically. Lists are deleted        //
//     when list window is closed. Other objects can be deleted in f2.      //
//     If nothing to delete, f2 & f3 must be set to null().                 //
//     Caller should not delete the dialogbox.                              //
// Boxes and radios can set up groups or gray out unwanted options          //
//     using a.f1 function pointer.                                         //
// All elements in a.boxwidget[][] must be set to 0 !                       //
// The callback f1() can use XtSetSensitive to gray out unwanted            //
//     options based on the radio and box settings. It must check           //
//     to make sure the widget is not 0 first, because unused widgets       //
//     are initialized to 0.                                                //
// Any flag variables to prevent multiple entry must be cleared in a.f4 .   //
//--------------------------------------------------------------------------//
void dialogbox(dialoginfo *a)
{
 if(g.block){ a->f4(a); return; } // otherwise becomes orphan
 if(a->want_changecicb) g.indialog++;
 static char dirmask[FILENAMELENGTH]="*";
 char tempstring[FILENAMELENGTH];
 clickboxinfo *clickboxdata=NULL;
 Widget *w=NULL;
 Widget bb=0, bb2, okaybutton=0;
 Widget radiopush[20];
 int n,wc=0,i,j,k,selection,value,boxcount=0,radiocount=0,startpos=0,
     multistringcount=0, noofscrolledstrings=0;
 int mpboxwidth, mplabelstart;
 int boxlabelwidth=0;
 int color[4];
 XmString xms;
 double y;
 char junk[2];
 junk[0]=32;
 junk[1]=0;

 Arg args[100];
 clickboxdata = new clickboxinfo[DCOUNT];  // Up to DCOUNT sliders
 for(k=0;k<DCOUNT;k++) 
 {   clickboxdata[k].w = 0;
     clickboxdata[k].dirmask = 0;
     clickboxdata[k].done = 0;
     clickboxdata[k].form = 0;
     clickboxdata[k].wc = 0;
     clickboxdata[k].f1 = 0;
     clickboxdata[k].f2 = 0;
     clickboxdata[k].f3 = 0;
     clickboxdata[k].f4 = 0;
     clickboxdata[k].f5 = 0;
     clickboxdata[k].f6 = 0;
     clickboxdata[k].f7 = 0;
     clickboxdata[k].f8 = 0;
     clickboxdata[k].path = 0;
     clickboxdata[k].type = SINGLE;
     clickboxdata[k].list = (char**)NULL;
     clickboxdata[k].helptopic = 0;
     for(j=0;j<20;j++){ clickboxdata[k].widget[j]=0; clickboxdata[k].ptr[j]=0; }
 }
 w = new Widget[200];
 char  temp[256], *boxstring, radiostring[256];
 Widget wid[DCOUNT][WCOUNT], radiobox[DCOUNT], radiobutton[10][DCOUNT], boxbutton[DCOUNT];
 Widget widget;
 g.getout=0;
 for(i=0;i<20;i++) radiopush[i]=0;

 //----Calculate how large it has to be-----------------------------//

 double TOPMARGIN = 6;
 double RADIOSTART;
 double RADIOWIDTH;
 double BOXSTART;
 double CHECKWIDTH = 20;
 double LABELWIDTH;
 double BOXWIDTH, LABELSTART;
 double BOXHEIGHT = 20;
 double RADIOHEIGHT = 16;
 double WIDTH = 425;
 double HEIGHT = 200;

 for(j=0;j<a->noofboxes;j++) 
 {    boxcount++;
      if(a->boxtype[j]!=TOGGLE && a->boxtype[j]!=LABEL)
          boxlabelwidth = max(boxlabelwidth, (int)strlen(a->boxes[j]));
      if(a->boxtype[j]==MULTISTRING) multistringcount++;
      if(a->boxtype[j]==SCROLLEDSTRING) noofscrolledstrings++;
 }
 for(j=0;j<a->noofradios;j++) 
 {   radiocount++;
     for(k=0;k<a->radiono[j];k++) radiocount++;
 }
 if(a->width) WIDTH = a->width; 
 if(a->height) HEIGHT = a->height; 
 else
     HEIGHT = 40 + max((2+RADIOHEIGHT)*radiocount, 
              4+(1+BOXHEIGHT)*(boxcount + 0.8*noofscrolledstrings));
 if(a->boxusexy || a->radiousexy) 
 {  RADIOSTART = a->radiostart;
    RADIOWIDTH = a->radiowidth;
    BOXSTART   = a->boxstart;
    BOXWIDTH   = a->boxwidth;
    LABELSTART = a->labelstart;
    LABELWIDTH = a->labelwidth;
 }else
 {  RADIOSTART = 1;
    RADIOWIDTH = 142;
    BOXSTART   = 160;
    BOXWIDTH   = 160;
    LABELSTART = 320;
    LABELWIDTH =  90;
 }
 if(a->noofboxes==0) 
 {  if(!a->width)
    {    RADIOWIDTH = 320;
         WIDTH      = 320;
    }
    HEIGHT    += 20;
 }
 if(a->noofradios==0)
 {  BOXSTART   = 8; 
    BOXWIDTH   = 180;
    LABELSTART = 190; 
    LABELWIDTH = 130;
    CHECKWIDTH = 32;
    HEIGHT    += 20;
 }
 if(multistringcount>0)
 {  BOXWIDTH  += 80;
    LABELSTART+= 80; 
    WIDTH     += 60;
 }

 //--------- Create a bb dialog shell widget ------------------------//

 n=0;                       
 
 ////  Block Window Manager delete button (which creates unmap event)
 ////  Needed in spot densitometry where dialog has lists that
 ////  would otherwise be orphaned.
 
 if(a->busy)
 {   XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++; 
 }
 XtSetArg(args[n], XmNx, 0); n++;    
 XtSetArg(args[n], XmNy, 0); n++;    
 if(a->boxusexy || a->radiousexy)
 {   XtSetArg(args[n], XmNwidth,  a->width); n++;
     XtSetArg(args[n], XmNheight, a->height); n++;
 }else
 {   XtSetArg(args[n], XmNwidth, WIDTH); n++;
     XtSetArg(args[n], XmNheight, HEIGHT); n++;
 }
 //// Transient makes it stay on top
 if(a->transient){  XtSetArg(args[n], XmNtransient, True); n++; }
 else           {  XtSetArg(args[n], XmNtransient, False); n++; }
 XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
 XtSetArg(args[n], XmNmarginHeight, 0); n++;
 XtSetArg(args[n], XmNmarginWidth, 0); n++;
 XtSetArg(args[n], XmNautoUnmanage, False); n++;
 ////  Stop Motif from trying to grab another color if none are available.
 if(g.want_colormaps)  
 {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
      XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
 }
 XtSetArg(args[n], XmNkeyboardFocusPolicy, XmEXPLICIT); n++;
 XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); n++;
 XtSetArg(args[n], XmNdialogTitle, xms = XmStringCreateSimple(a->title));n++;
 XtSetArg(args[n], XmNrecomputeSize, False); n++;
 bb = w[wc++] = XmCreateBulletinBoardDialog(g.main_widget, (char*)"DialogForm", args, n);
 XmStringFree(xms);


 //// In case custom size is too narrow for title
 w[wc++] = addbblabel(bb, a->title, CENTER, max(8,WIDTH/2-6*strlen(a->title)), 4+TOPMARGIN/2); 

 y = TOPMARGIN + RADIOHEIGHT + TOPMARGIN;

 //-----------Start adding radio boxes-------------------------------//

 for(i=0;i<a->noofradios;i++)
 {
       ////  'Horizontal' orientation means buttons are stacked vertically.
       if(a->radiousexy)
       {    w[wc++] = addbblabel(bb, a->radio[i][0], LEFT, a->radioxy[i][0], a->radioxy[i][1]); 
            y = a->radioxy[i][1]+RADIOHEIGHT;
       }else
       {    w[wc++] = addbblabel(bb, a->radio[i][0], LEFT, RADIOSTART, y); 
            y+=RADIOHEIGHT;
       }
       n=0;
       if(a->radiousexy)
       {   XtSetArg(args[n], XmNx,      a->radioxy[i][0]); n++;  
           RADIOSTART = a->radioxy[i][0];
           XtSetArg(args[n], XmNy,      y); n++;    
           XtSetArg(args[n], XmNwidth,  a->radioxy[i][2]); n++;  
           RADIOWIDTH = a->radioxy[i][2];
           XtSetArg(args[n], XmNheight, a->radioxy[i][3]); n++; 
           RADIOSTART = a->radioxy[i][3];
       }else
       {   XtSetArg(args[n], XmNx, RADIOSTART); n++;  
           XtSetArg(args[n], XmNy, y); n++;    
           XtSetArg(args[n], XmNwidth, RADIOWIDTH); n++;  
           XtSetArg(args[n], XmNheight, RADIOHEIGHT); n++; 
       }
       XtSetArg(args[n], XmNspacing, 0); n++;
       XtSetArg(args[n], XmNshadowThickness, 0); n++;
       XtSetArg(args[n], XmNradioBehavior, True); n++;
       XtSetArg(args[n], XmNradioAlwaysOne, True); n++;
       XtSetArg(args[n], XmNpacking,XmPACK_COLUMN); n++;
       XtSetArg(args[n], XmNnumColumns,50); n++;
       XtSetArg(args[n], XmNorientation,XmHORIZONTAL); n++;
       XtSetArg(args[n], XmNresizable, False); n++;
       XtSetArg(args[n], XmNadjustLast, False); n++;
       ////  Stop Motif from trying to grab another color if none are available.
       if(g.want_colormaps)  
       {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
            XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
       }
       radiobox[i] = w[wc++] = XmCreateRadioBox(bb, (char*)"radiobox", args, n);
       XtManageChild(radiobox[i]);
       for(j=1;j<a->radiono[i];j++)
       {  
           ////  Create main radio buttons - set shadowThickness to 1 to create
           ////  a border.

           n=0;
           XtSetArg(args[n], XmNspacing, 0); n++;
           XtSetArg(args[n], XmNresizable, False); n++;
           XtSetArg(args[n], XmNmarginWidth, 4); n++;

           XtSetArg(args[n], XmNx, RADIOSTART); n++;  
           XtSetArg(args[n], XmNy, y); n++;    
           XtSetArg(args[n], XmNwidth, RADIOWIDTH); n++;  
           XtSetArg(args[n], XmNheight, RADIOHEIGHT); n++; 
           XtSetArg(args[n], XmNpacking,XmPACK_COLUMN); n++;
           XtSetArg(args[n], XmNnumColumns,50); n++;
           XtSetArg(args[n], XmNorientation,XmHORIZONTAL); n++;
           strcpy(radiostring, a->radio[i][j]);
           for(k=strlen(radiostring);k<20;k++)strcat(radiostring,junk);
           XtSetArg(args[n], XmNindicatorOn, True); n++;
           XtSetArg(args[n], XmNindicatorType, XmONE_OF_MANY); n++;
           XtSetArg(args[n], XmNfillOnSelect, True); n++;
           ////  Stop Motif from trying to grab another color if none are available.
           if(g.want_colormaps)  
           {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
           }
           radiobutton[i][j] = w[wc++] =  
                XmCreateToggleButton(radiobox[i],radiostring,args,n);
           a->radiowidget[i][j] = radiobutton[i][j];
           XtManageChild(radiobutton[i][j]);
           if(a->radioset[i]==j)
               XmToggleButtonSetState(radiobutton[i][j], True, True); 
           else
               XmToggleButtonSetState(radiobutton[i][j], False, True);                

           ////  Set up a callback for each radio button. They must both be
           ////  disarmcallbacks (called when they unclick on it).
           ////  If armcallbacks - it will read wrong button state before calling f1.
           ////  If valuechanged - it will call f1 twice.
           ////  If different - it will only call one of them.

#ifndef LESSTIF_VERSION
           XtAddCallback(radiobutton[i][j], XmNdisarmCallback, (XtCBP)togglecb, (XtP)a);
           XtAddCallback(radiobutton[i][j], XmNdisarmCallback, (XtCBP)armboxcb, (XtP)a);
#endif
           ////  However, in lesstif the above doesn't work for some reason.
	   ////  Apparently, no disarmcb.
	   ////  So add another hack specific for lesstif.
#ifdef LESSTIF_VERSION
           XtAddCallback(radiobutton[i][j], XmNarmCallback, (XtCBP)togglecb, (XtP)a);
           XtAddCallback(radiobutton[i][j], XmNarmCallback, (XtCBP)armboxcb, (XtP)a);
#endif

           XtManageChild(radiobutton[i][j]);

           ////  If pushbutton needed too in radio button.
           ////  Put on bb Widget so it doesn't mess up size and spacing 
           ////  of the radio buttons.

           if(a->radiotype[i][j]==RADIOPUSHBUTTON) 
           {   y += 1.5*RADIOHEIGHT;
               n = 0;
               XtSetArg(args[n], XmNx, RADIOSTART); n++;  
               XtSetArg(args[n], XmNy, y); n++;    
               XtSetArg(args[n], XmNwidth, RADIOWIDTH); n++;  
               XtSetArg(args[n], XmNheight, RADIOHEIGHT); n++; 
               XtSetArg(args[n], XmNresizable, False); n++;  
#ifdef MOTIF2
               XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_NONE); n++;
#else 
               XtSetArg(args[n], XmNindicatorOn, False); n++;
#endif
               XtSetArg(args[n], XmNtraversalOn, True); n++;
               XtSetArg(args[n], XmNlabelString,
                 xms = XmStringCreateSimple(a->radiopushbuttonlabel[i]));n++; 
               ////  Stop Motif from trying to grab another color if none are available.
               if(g.want_colormaps)  
               {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                    XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
               }
               radiopush[i] = w[wc++] = XmCreatePushButton(bb,
                    a->radiopushbuttonlabel[i], args,n);
               XtAddCallback(radiopush[i], XmNactivateCallback, 
                   (XtCBP)a->radiocb[i], (XtP)(a->radiopushbuttonptr[i]));
               XmStringFree(xms);
           }
           y+=2+RADIOHEIGHT;
       }
       y+=RADIOHEIGHT;
  }

  //------Answer boxes (input boxes)-------------------------------//

  y = TOPMARGIN + RADIOHEIGHT + TOPMARGIN;

  //// For non-radio items, top position must be integer or boxes come out
  //// all different sizes.

  for(j=0;j<a->noofboxes;j++)
  {     
      boxstring = a->boxes[j];          // boxstring is a temporary string 
      boxstring[40] = 0;                // Make sure it fits on button
      a->list_visible[j] = 0;
      switch(a->boxtype[j])
      {
         case TOGGLE:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNmarginLeft, 15); n++;
              XtSetArg(args[n], XmNresizable, True); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
#ifdef MOTIF2
              XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_CROSS_BOX); n++;
#endif
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
              if(a->boxset[j])
                  XtSetArg(args[n], XmNset, True); 
              else
                  XtSetArg(args[n], XmNset, False);
              n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreateToggleButton(bb, boxstring, args, n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);

              ////  Set up callbacks for each box button
              XtAddCallback(boxbutton[j], XmNvalueChangedCallback,
                   (XtCBP)toggleboxcb, (XtP)a);
              XtAddCallback(boxbutton[j], XmNvalueChangedCallback,
                   (XtCBP)armboxcb, (XtP)a);
              break;
         case PUSHBUTTON:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++; 
              XtSetArg(args[n], XmNlabelString, 
                 xms=XmStringCreateSimple(boxstring)); n++;  

              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,(char*)"BoxPushButton",args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);

              ////  Set up callbacks for each box button
              XtAddCallback(boxbutton[j], XmNarmCallback,(XtCBP)armboxcb,(XtP)a);
              break;
         case STRING:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNrows, 1); n++;
              XtSetArg(args[n], XmNmarginHeight, 1); n++;
              XtSetArg(args[n], XmNmarginWidth, 4); n++;
              XtSetArg(args[n], XmNmaxLength, 256); n++;
              XtSetArg(args[n], XmNvalue, a->answer[j][0]);n++; 
              XtSetArg(args[n], XmNautoShowCursorPosition, True); n++;
              XtSetArg(args[n], XmNcursorPositionVisible, True); n++;
              ////  Motif ignores this for some reason
              XtSetArg(args[n], XmNcursorPosition,(int)strlen(a->answer[j][0])); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreateTextField(bb, boxstring, args, n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              XtManageChild(boxbutton[j]);
              XtAddCallback(boxbutton[j], XmNvalueChangedCallback, 
                   (XtCBP)dialogstringcb, (XtP)a);
              break;
         case SCROLLEDSTRING:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, 1.8*BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNrows, 1); n++;
              XtSetArg(args[n], XmNmarginHeight, 1); n++;
              XtSetArg(args[n], XmNmarginWidth, 4); n++;
              XtSetArg(args[n], XmNmaxLength, 256); n++;
              XtSetArg(args[n], XmNvalue, a->answer[j][0]);n++; 
              XtSetArg(args[n], XmNautoShowCursorPosition, True); n++;
              XtSetArg(args[n], XmNcursorPositionVisible, True); n++;
              ////  Motif ignores this for some reason
              XtSetArg(args[n], XmNcursorPosition,(int)strlen(a->answer[j][0])); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreateScrolledText(bb, boxstring, args, n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              XtManageChild(boxbutton[j]);
              XtAddCallback(boxbutton[j], XmNvalueChangedCallback, 
                   (XtCBP)dialogstringcb, (XtP)a);
              y+=0.8*BOXHEIGHT;
              break;
         case TOGGLESTRING:
              //// Toggle button portion - The callback f1 must unset other
              //// buttons, gray out unwanted options, etc. - it is not done
              //// automatically.

              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][0]+CHECKWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, CHECKWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNmarginLeft, 15); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
#ifdef MOTIF2
              XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_CROSS_BOX); n++;
#endif
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
              if(a->boxset[j]){  XtSetArg(args[n], XmNset, True); n++; }
              else           {  XtSetArg(args[n], XmNset, False); n++; }

              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              w[wc++] = boxbutton[j] =XmCreateToggleButton(bb,(char*)" ",args,n);
              a->boxwidget[j][0] = boxbutton[j];
              XtManageChild(boxbutton[j]);

              ////  Set up callbacks for each box button
              XtAddCallback(a->boxwidget[j][0], XmNvalueChangedCallback,
                   (XtCBP)toggleboxcb, (XtP)a);  
              XtAddCallback(a->boxwidget[j][0], XmNvalueChangedCallback,(XtCBP)armboxcb, 
                   (XtP)a);
              if(a->boxset[j])
                   XmToggleButtonSetState(a->boxwidget[j][0], True, False); 

              ////  String portion
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]+CHECKWIDTH); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART+CHECKWIDTH); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH-CHECKWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNresizeWidth, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNrows, 1); n++;
              XtSetArg(args[n], XmNcolumns, 10); n++;
              XtSetArg(args[n], XmNmarginHeight, 1); n++;
              XtSetArg(args[n], XmNmarginWidth, 1); n++;
              XtSetArg(args[n], XmNmaxLength, 256); n++;
              XtSetArg(args[n], XmNvalue, a->answer[j][1]);n++; 
              XtSetArg(args[n], XmNcursorPosition,(int)strlen(a->answer[j][1])); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              widget = w[wc++] = XmCreateTextField(bb, boxstring, args, n);
              if(!a->boxusexy)
                    w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = widget;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(widget);
              XtAddCallback(widget, XmNvalueChangedCallback, 
                 (XtCBP)dialogstringcb, (XtP)a);
              break;
         case INTCLICKBOX:       // Pushbutton calling a clickbox (Slider)
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNmarginLeft, 4); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
              XtSetArg(args[n], XmNlabelString,
                 xms = XmStringCreateSimple(a->answer[j][0]));n++; 
              XtSetArg(args[n], XmNmarginWidth, 1); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);
              XmStringFree(xms);
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              clickboxdata[j].title = boxstring;
              clickboxdata[j].answer = atoi(a->answer[j][0]);
              clickboxdata[j].fanswer = 0.0;
              clickboxdata[j].startval = atoi(a->answer[j][0]);
              clickboxdata[j].minval = a->boxmin[j];
              clickboxdata[j].maxval = a->boxmax[j];
              clickboxdata[j].type = INTEGER;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].widget[0] = boxbutton[j];
              clickboxdata[j].dirmask = 0;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].wantdragcb = 1;
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              a->c[j] = &clickboxdata[j];
              XtAddCallback(boxbutton[j], XmNactivateCallback, 
                 (XtCBP)dialogclickboxcb, (XtP)a);
              XtAddCallback(boxbutton[j], XmNarmCallback, (XtCBP)armboxcb, 
                 (XtP)a);
              break;
         case FILENAME:          // Get filename
         case MULTIFILENAME:     // Get list of filenames. dialogclickboxcb()
	                         //  allocates a list & puts it in a->boxlist[][].
                                 // Puts count in a->boxcount[].
                                 // Calling routine must free this list.                                 
              if((int)a->answer[j][0][0]==0) strcpy(a->answer[j][0],"none     ");
              else strcpy(tempstring, basefilename(a->answer[j][0]));
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++; 
              XtSetArg(args[n], XmNlabelString, 
                 xms=XmStringCreateSimple(tempstring)); n++;  
              XtSetArg(args[n], XmNcursorPosition,(int)strlen(tempstring)); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              clickboxdata[j].title = a->answer[j][0];
              clickboxdata[j].widget[1] = boxbutton[j];
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              clickboxdata[j].wc = 0;
              clickboxdata[j].w  = new Widget[DCOUNT];
              clickboxdata[j].ptr[0]  = a;
              clickboxdata[j].form = 0;
              clickboxdata[j].count= 0;
              clickboxdata[j].list = NULL;
              clickboxdata[j].dirmask = new char[FILENAMELENGTH];
              clickboxdata[j].path = a->path;
              strcpy(clickboxdata[j].dirmask, dirmask);
              if(a->boxtype[j] == MULTIFILENAME)
                   clickboxdata[j].type = MULTIPLE;
              else
                   clickboxdata[j].type = SINGLE;
              clickboxdata[j].count = 0;
              a->c[j] = &clickboxdata[j];
              XtAddCallback(boxbutton[j], XmNactivateCallback, 
                 (XtCBP)dialogfilenamecb, (XtP)a);
              XtAddCallback(boxbutton[j], XmNarmCallback, (XtCBP)armboxcb, (XtP)a);
              XtManageChild(boxbutton[j]);
              XmStringFree(xms);
              startpos = strlen(tempstring);
              startpos = max(0,startpos-36);
              break;
         case RGBCLICKBOX:        // RGB multi clickbox - Display 3 boxes that all 
              n = 0;              // change the same data->
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              value = atoi(a->answer[j][0]);
              valuetoRGB(value, color[1], color[2], color[3], a->bpp);
              clickboxdata[j].title = boxstring;
              clickboxdata[j].answer = atoi(a->answer[j][0]);
              clickboxdata[j].startval = atoi(a->answer[j][0]);
              clickboxdata[j].minval = a->boxmin[j];
              clickboxdata[j].maxval = a->boxmax[j];
              clickboxdata[j].type = INTEGER;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].wantdragcb = 1;
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              a->c[j] = &clickboxdata[j];
              a->boxwidget[j][0] = 0;
              for(k=1;k<4;k++)
              {   XtSetArg(args[n], XmNx,      BOXSTART+(k-1)*43); n++;     
                  XtSetArg(args[n], XmNy,      y); n++;     
                  XtSetArg(args[n], XmNwidth,  43); n++;  
                  XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 0); n++;
                  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
                  itoa(color[k],temp,10);
                  XtSetArg(args[n], XmNlabelString,
                     xms = XmStringCreateSimple(temp));n++; 
                  wid[j][k] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtManageChild(wid[j][k]);
                  XmStringFree(xms);
                  XtAddCallback(wid[j][k], XmNactivateCallback, 
                     (XtCBP)dialogrgbclickboxcb, (XtP)a);
                  XtAddCallback(wid[j][k], XmNarmCallback,(XtCBP)armboxcb, (XtP)a);
                  if(!a->boxusexy)
                      w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              }
              for(i=0;i<4;i++) clickboxdata[j].widget[i] = wid[j][i];
              clickboxdata[j].dirmask = 0;
              break;
         case MULTICLICKBOX:      // Multiclickbox - Display 3 boxes that all 
              n = 0;              // change the same data-> (uses #1,2,3)
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              clickboxdata[j].title = boxstring;
              clickboxdata[j].minval = a->boxmin[j];
              clickboxdata[j].maxval = a->boxmax[j];
              clickboxdata[j].type = INTEGER;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].wantdragcb = 1;
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;   // Reserved by multiclickboxcb
              clickboxdata[j].f6 = null;   // Reserved by multiclickboxcb
              clickboxdata[j].f7 = null;  
              clickboxdata[j].f8 = a->f8;  
              clickboxdata[j].answers = new int[10];
              clickboxdata[j].answers[0] = atoi(a->answer[j][0]);
              clickboxdata[j].answers[1] = atoi(a->answer[j][1]);
              clickboxdata[j].answers[2] = atoi(a->answer[j][2]);
              a->c[j] = &clickboxdata[j];
              a->boxwidget[j][0] = 0;
              //// Note: widgets 1,2,3 are for answers 0,1,2
              for(k=1;k<4;k++)
              {   XtSetArg(args[n], XmNx,      BOXSTART+(k-1)*43); n++;     
                  XtSetArg(args[n], XmNy,      y); n++;     
                  XtSetArg(args[n], XmNwidth,  43); n++;  
                  XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 0); n++;
                  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
                  XtSetArg(args[n], XmNlabelString,
                     xms = XmStringCreateSimple(a->answer[j][k-1]));n++; 
                  wid[j][k] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtManageChild(wid[j][k]);
                  XmStringFree(xms);
                  XtAddCallback(wid[j][k], XmNactivateCallback, 
                     (XtCBP)dialogmulticlickboxcb, (XtP)a);
                  XtAddCallback(wid[j][k], XmNarmCallback,(XtCBP)armboxcb, (XtP)a);
                  if(!a->boxusexy)
                      w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              }
              for(i=0;i<4;i++) clickboxdata[j].widget[i] = wid[j][i];
              clickboxdata[j].dirmask = 0;
              break;
         case TOGGLERGBCLICKBOX:
              //// Toggle button portion - The callback f1 must unset other
              //// buttons, gray out unwanted options, etc. - it is not done
              //// automatically.

              n = 0;
              XtSetArg(args[n], XmNmarginLeft, 1); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNx, BOXSTART); n++;  
              XtSetArg(args[n], XmNy, y); n++;    
              XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
              XtSetArg(args[n], XmNheight, CHECKWIDTH); n++; 
#ifdef MOTIF2
              XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_CROSS_BOX); n++;
#endif
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
              if(a->boxset[j]){  XtSetArg(args[n], XmNset, True); n++; }
              else           {  XtSetArg(args[n], XmNset, False); n++; }
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              w[wc++] = boxbutton[j] =XmCreateToggleButton(bb,(char*)" ",args,n);
              a->boxwidget[j][0] = boxbutton[j];
              XtManageChild(boxbutton[j]);
              ////  Set up callbacks for each box button
              XtAddCallback(boxbutton[j], XmNarmCallback,(XtCBP)armboxcb, (XtP)a);

              ////  Multiclickbox portion

              n = 0;             
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNx, BOXSTART+CHECKWIDTH); n++;  
              XtSetArg(args[n], XmNy, y); n++;    
              XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
              XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              value = atoi(a->answer[j][0]);
              valuetoRGB(value,color[1],color[2],color[3],a->bpp);
              clickboxdata[j].title = boxstring;
              clickboxdata[j].answer = atoi(a->answer[j][0]);
              clickboxdata[j].startval = atoi(a->answer[j][0]);
              clickboxdata[j].minval = a->boxmin[j];
              clickboxdata[j].maxval = a->boxmax[j];
              clickboxdata[j].type = INTEGER;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].wantdragcb = 1;
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              a->c[j] = &clickboxdata[j];
              for(k=1;k<4;k++)
              {   
                  XtSetArg(args[n], XmNx, BOXSTART+k*43); n++;  
                  XtSetArg(args[n], XmNy, y); n++;    
                  XtSetArg(args[n], XmNwidth, 43); n++;  
                  XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 0); n++;
                  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
                  itoa(color[k],temp,10);
                  XtSetArg(args[n], XmNlabelString,
                     xms = XmStringCreateSimple(temp));n++; 
                  wid[j][k] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtManageChild(wid[j][k]);
                  XmStringFree(xms);
                  XtAddCallback(wid[j][k], XmNactivateCallback, 
                     (XtCBP)dialogrgbclickboxcb, (XtP) a);
                  XtAddCallback(wid[j][k], XmNarmCallback,(XtCBP)armboxcb, (XtP)a);
                  w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              }
              for(i=0;i<4;i++) clickboxdata[j].widget[i] = wid[j][i];
              clickboxdata[j].dirmask = 0;
              break;
         case LIST:
         case NON_EDIT_LIST:
         case NO_LABEL_LIST:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNmarginLeft, 4); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
              selection = *a->l[j]->selection;
              if(selection > a->l[j]->count || selection < 0) return;
              XtSetArg(args[n], XmNlabelString,
                 xms = XmStringCreateSimple(a->l[j]->info[selection]));n++; 
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);
              XmStringFree(xms);
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              clickboxdata[j].title = a->l[j]->title;
              clickboxdata[j].answer = *a->l[j]->selection;
              clickboxdata[j].widget[0] = boxbutton[j];
              clickboxdata[j].dirmask = 0;
              clickboxdata[j].list = a->l[j]->info;
              clickboxdata[j].helptopic = a->helptopic;
              clickboxdata[j].listsize = a->boxmax[j];
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null;
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              if(a->boxtype[j]==NON_EDIT_LIST) 
                   clickboxdata[j].allowedit = 0;
              else 
              {    clickboxdata[j].allowedit = 1;  // if allowedit is 1, must allocate edittitle
                   a->l[j]->edittitle = new char[100];
                   strcpy(a->l[j]->edittitle, "Enter value");
              }
              a->l[j]->allowedit = clickboxdata[j].allowedit;
              a->l[j]->wc = 0;
              clickboxdata[j].editcol = a->startcol[j];
              clickboxdata[j].maxstringsize = 50;
              a->c[j] = &clickboxdata[j];
              XtAddCallback(boxbutton[j], XmNactivateCallback, 
                 (XtCBP)dialoglistcb, (XtP)a);
              break;
         case LABEL:      // Just a heading, don't do anything
              if(a->boxusexy)
                 w[wc++] = addbblabel(bb, boxstring, LEFT, a->boxxy[j][0], a->boxxy[j][1]);
              else
                 w[wc++] = addbblabel(bb, boxstring, LEFT, BOXSTART, y);
              a->boxwidget[j][0] = 0;
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              break;
         case DOUBLECLICKBOX:
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNmarginLeft, 4); n++;
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++; 
              XtSetArg(args[n], XmNlabelString,
                 xms = XmStringCreateSimple(a->answer[j][0]));n++; 
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,boxstring,args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);
              XmStringFree(xms);
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              clickboxdata[j].title = boxstring;
              clickboxdata[j].answer = 0;
              clickboxdata[j].fanswer = atof(a->answer[j][0]);
              clickboxdata[j].fstartval = atof(a->answer[j][0]);
              clickboxdata[j].fminval = (double)a->boxmin[j];
              clickboxdata[j].fmaxval = (double)a->boxmax[j];
              clickboxdata[j].type = FLOAT;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].widget[0] = boxbutton[j];
              clickboxdata[j].dirmask = 0;
              clickboxdata[j].wantpreview = 0;
              clickboxdata[j].wantdragcb = 1;
              clickboxdata[j].f1 = null;
              clickboxdata[j].f2 = null; 
              clickboxdata[j].f3 = null;
              clickboxdata[j].f4 = null;
              clickboxdata[j].f5 = null;
              clickboxdata[j].f6 = null;
              clickboxdata[j].f7 = null;
              clickboxdata[j].f8 = null;
              a->c[j] = &clickboxdata[j];
              XtAddCallback(boxbutton[j], XmNactivateCallback, 
                 (XtCBP)dialogdoubleclickboxcb, (XtP)a);
              XtAddCallback(boxbutton[j], XmNarmCallback,(XtCBP)armboxcb, (XtP)a);
              break;
         case MULTISTRING:  // multi string
              n = 0;              
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNnoResize, True); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNadjustLast, False); n++;
              XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
              XtSetArg(args[n], XmNshadowThickness, 0); n++;
              XtSetArg(args[n], XmNallowOverlap, True); n++;
              XtSetArg(args[n], XmNmarginHeight, 0); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              bb2 = w[wc++] = XmCreateBulletinBoard(bb, (char*)"asdform", args,  n);
              XtManageChild(bb2);

              for(k=0;k<3;k++)
              {   
                  n = 0;              
                  XtSetArg(args[n], XmNx, k*BOXWIDTH/3); n++;
                  XtSetArg(args[n], XmNy, 0); n++;    
                  XtSetArg(args[n], XmNwidth, BOXWIDTH/3-3); n++;
                  XtSetArg(args[n], XmNheight, 24); n++;
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNmarginWidth, 3); n++;
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 1); n++;
                  XtSetArg(args[n], XmNcursorPosition,(int)strlen(a->answer[j][k])); n++;
                  XtSetArg(args[n], XmNlabelString, xms = XmStringCreateSimple(a->answer[j][k]));n++; 
                  wid[j][k] = w[wc++] = XmCreateTextField(bb2, a->answer[j][k], args, n);
//                  wid[j][k] = w[wc++] = XmCreateTextField(bb2, a->answer[j][k], args, n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtManageChild(wid[j][k]);
                  XtAddCallback(wid[j][k], XmNvalueChangedCallback, (XtCBP)dialogstringcb, (XtP)a);
                  XmStringFree(xms);
              }
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              a->boxwidget[j][3] = 0;
              break;
         case MULTIPUSHBUTTON:   /// Row of pushbuttons
              n = 0;              
              mpboxwidth = cint(a->wcount[j]*BOXWIDTH/3);
              mplabelstart = cint(a->wcount[j]*LABELSTART/3);
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, mpboxwidth); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
              XtSetArg(args[n], XmNnoResize, True); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNadjustLast, False); n++;
              XtSetArg(args[n], XmNshadowThickness, 0); n++;
              XtSetArg(args[n], XmNallowOverlap, True); n++;
              XtSetArg(args[n], XmNmarginHeight, 0); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              bb2 = w[wc++] = XmCreateBulletinBoard(bb, (char*)"asdform", args, n);
              XtManageChild(bb2);

              for(k=0; k<min(3, a->wcount[j]); k++)
              {   
                  n = 0;              
                  XtSetArg(args[n], XmNx, k*BOXWIDTH/3); n++;
                  XtSetArg(args[n], XmNy, 0); n++;    
                  XtSetArg(args[n], XmNwidth, BOXWIDTH/3-3); n++;
                  XtSetArg(args[n], XmNheight, BOXHEIGHT); n++;
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 1); n++;
                  XtSetArg(args[n], XmNlabelString, xms = XmStringCreateSimple(a->answer[j][k]));n++; 
                  wid[j][k] = w[wc++] = XmCreatePushButton(bb2, (char*)"AAA", args, n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtAddCallback(wid[j][k], XmNarmCallback, (XtCBP)armboxcb, (XtP)a);
                  XtManageChild(wid[j][k]);
                  XmStringFree(xms);
              }
              if(!a->boxusexy)
                  w[wc++] = addbblabel(bb, boxstring, LEFT, mplabelstart, y);
              a->boxwidget[j][3] = 0;
              break;
         case MULTILABEL:
              n = 0;              
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
              XtSetArg(args[n], XmNnoResize, True); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNadjustLast, False); n++;
              XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
              XtSetArg(args[n], XmNshadowThickness, 0); n++;
              XtSetArg(args[n], XmNallowOverlap, True); n++;
              XtSetArg(args[n], XmNmarginHeight, 0); n++;
              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              bb2 = w[wc++] = XmCreateBulletinBoard(bb, (char*)"asdform", args,  n);
              XtManageChild(bb2);
              for(k=0;k<min(3, a->wcount[j]);k++)
              {   
                  n = 0;              
                  XtSetArg(args[n], XmNx, k*BOXWIDTH/3); n++;
                  XtSetArg(args[n], XmNy, 0); n++;    
                  XtSetArg(args[n], XmNwidth, BOXWIDTH/3); n++;
                  XtSetArg(args[n], XmNheight, BOXHEIGHT); n++;
                  XtSetArg(args[n], XmNresizable, False); n++; 
                  XtSetArg(args[n], XmNrecomputeSize, False); n++;
                  XtSetArg(args[n], XmNspacing, 1); n++;
                  XtSetArg(args[n], XmNlabelString, xms = XmStringCreateSimple(a->answer[j][k]));n++; 
                  wid[j][k] = w[wc++] = XmCreateLabel(bb2, a->answer[j][k], args, n);
                  a->boxwidget[j][k] = wid[j][k];
                  XtManageChild(wid[j][k]);
                  XmStringFree(xms);
              }
              if(!a->boxusexy)
                   w[wc++] = addbblabel(bb, boxstring, LEFT, LABELSTART, y);
              a->boxwidget[j][3] = 0;
              break;
         case MULTITOGGLE:
              strcpy(boxstring, a->boxlist[j][0]);
              n = 0;
              if(a->boxusexy)
              {  XtSetArg(args[n], XmNx,      a->boxxy[j][0]); n++;     
                 XtSetArg(args[n], XmNy,      a->boxxy[j][1]); n++;     
                 XtSetArg(args[n], XmNwidth,  a->boxxy[j][2]); n++;  
                 XtSetArg(args[n], XmNheight, a->boxxy[j][3]); n++; 
              }else
              {  XtSetArg(args[n], XmNx, BOXSTART); n++;  
                 XtSetArg(args[n], XmNy, y); n++;    
                 XtSetArg(args[n], XmNwidth, BOXWIDTH); n++;  
                 XtSetArg(args[n], XmNheight, BOXHEIGHT); n++; 
              }
              XtSetArg(args[n], XmNresizable, False); n++;
              XtSetArg(args[n], XmNrecomputeSize, False); n++;
              XtSetArg(args[n], XmNspacing, 0); n++;
              XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++; 
              XtSetArg(args[n], XmNlabelString, 
                 xms=XmStringCreateSimple(boxstring)); n++;  

              ////  Stop Motif from trying to grab another color if none are available.
              if(g.want_colormaps)  
              {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
                   XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
              }
              boxbutton[j] = w[wc++] = XmCreatePushButton(bb,(char*)"BoxPushButton",args,n);
              a->boxwidget[j][0] = boxbutton[j];
              a->boxwidget[j][1] = 0;
              a->boxwidget[j][2] = 0;
              a->boxwidget[j][3] = 0;
              XtManageChild(boxbutton[j]);

              ////  Set up callbacks for each box button
              XtAddCallback(boxbutton[j], XmNarmCallback,(XtCBP)multitoggleboxcb,(XtP)a);
              break;
         default: fprintf(stderr,"error in dialogboxes (%d=%d)\n", j,a->boxtype[j]);
      }
      y += BOXHEIGHT;
  }
  if(strlen(a->message))
  {
     n=0;
     XtSetArg(args[n], XmNx, a->message_x1); n++;   
     XtSetArg(args[n], XmNy, a->message_y1); n++; 
     XtSetArg(args[n], XmNresizable, True); n++;
     XtSetArg(args[n], XmNrecomputeSize, True); n++;
     ////  Stop Motif from trying to grab another color if none are available.
     if(g.want_colormaps)  
     {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
          XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
     }
     w[wc++] = a->message_widget = XmCreateLabel(bb, a->message, args, n);
     XtManageChild(a->message_widget);   

     n=0;
     XtSetArg(args[n], XmNx, a->message_x2); n++;   
     XtSetArg(args[n], XmNy, a->message_y2); n++; 
     XtSetArg(args[n], XmNresizable, True); n++;
     XtSetArg(args[n], XmNrecomputeSize, True); n++;
     ////  Stop Motif from trying to grab another color if none are available.
     if(g.want_colormaps)  
     {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
          XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
     }
     w[wc++] = a->message_widget2 = XmCreateLabel(bb, a->message2, args, n);
     XtManageChild(a->message_widget2);   

  }     
   
  addbbbuttons(a, bb, &w[wc], (int)(HEIGHT-20), a->helptopic, (int)a->width);
  okaybutton = w[wc];
  wc += 3;

  // This makes the Ok button the default, but also trashes the dialog box.
  //     XtVaSetValues(bb, XmNdefaultButton, okaybutton, NULL); 
  // So add an event handler to every button.
  // Make Return key activate Ok button:

  for(k=0;k<wc;k++)
      XtAddEventHandler(w[k], KeyPressMask, FALSE, (XtEH)dialogentercb, (XtP)a);

  if(a->want_changecicb)
      XtAddEventHandler(g.drawing_area,  SubstructureNotifyMask, FALSE,
           (XtEH)dialogchangecicb, (XtP)a);
  if(a->wantraise)
      XtAddCallback(bb, XmNexposeCallback, (XtCBP)dialogexposecb, (XtP)a);

   XtAddCallback(bb, XmNunmapCallback, (XtCBP)dialogunmapcb, (XtP)a);

  ////  Initialize any grayed-out items in the callback

  XtManageChild(bb);
  for(i=0;i<10;i++) if(radiopush[i]) XtManageChild(radiopush[i]);

  ////  Save all the information needed to later unmanage the dialog.    
  a->widget = w;                            // array of 200 Widgets
  a->form = bb;                             // DialogForm Widget
  a->clickboxdata = clickboxdata;           // Array of clickboxinfo structs
  a->widgetcount = wc;                      // No. of widgets on form
  armboxcb(0, a, 0);
  return;
} 


//--------------------------------------------------------------------------//
// dialogclickboxcb - callback for getting a value in dialog box.           //
// Calls clickbox(), using 'answer' in a clickboxinfo struct as the         //
// initial value, converts the integer returned by clickbox() to a          //
// string and stores it in the  'c->widget's XmNlabelString.                //
//--------------------------------------------------------------------------//
void dialogclickboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;  // Keep compiler quiet
  int box=0,j,k;
  dialoginfo *a = (dialoginfo*)client_data;

  ////  Figure out which box
  XtSetSensitive(a->form, False);
  for(k=0; k<4; k++)
  for(j=0; j<a->noofboxes; j++)
       if(w==a->boxwidget[j][k] && w!=0) box = j;
  clickboxinfo *c = a->c[box];
  a->w[0] = w;
  a->param[0] = box;
  c->answer = atoi(a->answer[box][0]);
  clickbox(c->title, 5, &c->answer, c->minval, c->maxval, 
       null, dialogclickboxcb_part2, dialogclickboxcb_cancel, (void*)a, w, 10);
}


//--------------------------------------------------------------------------//
// dialogclickboxcb_part2                                                   //
//--------------------------------------------------------------------------//
void dialogclickboxcb_part2(clickboxinfo *c)
{
  Widget w;
  int box;
  dialoginfo *a = (dialoginfo*)c->client_data;
  XtSetSensitive(a->form, True);
  char tempstring[256];
  w = a->w[0];
  box = a->param[0];
  if(g.getout){ g.getout=0; return; }
  ////  Redraw the label in the pushbutton widget that called the clickbox.
  ////  and stop Motif from trying to change size of the button
  itoa(c->answer, tempstring, 10);
  set_widget_label(w, tempstring);
  ////  Put answer back in dialoginfo
  itoa(c->answer, a->answer[box][0], 10);
}


//--------------------------------------------------------------------------//
// dialogclickboxcb_cancel                                                  //
//--------------------------------------------------------------------------//
void dialogclickboxcb_cancel(clickboxinfo *c)
{
  dialoginfo *a = (dialoginfo*)c->client_data;
  XtSetSensitive(a->form, True);
}


//--------------------------------------------------------------------------//
// dialogdoubleclickboxcb - callback for getting a value in dialog box      //
//--------------------------------------------------------------------------//
void dialogdoubleclickboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  int box=0,j,k;
  dialoginfo *a = (dialoginfo*)client_data;
  XtSetSensitive(a->form, False);

  ////  Figure out which box
  for(k=0;k<4;k++)
  for(j=0;j<a->noofboxes;j++)
       if(w==a->boxwidget[j][k] && w!=0) box = j;

  clickboxinfo *c = a->c[box];
  a->w[0] = w;
  a->param[0] = box;
  c->fanswer = atof(a->answer[box][0]);
  clickbox(c->title, 6, &c->fanswer, c->fminval, c->fmaxval, null, 
      dialogdoubleclickboxcb_part2, dialogclickboxcb_cancel, (void*)a, w, 10);
}


//--------------------------------------------------------------------------//
// dialogdoubleclickboxcb_part2                                             //
//--------------------------------------------------------------------------//
void dialogdoubleclickboxcb_part2(clickboxinfo *c)
{
  int box;
  Widget w;
  char tempstring[256];
  dialoginfo *a = (dialoginfo*)c->client_data;
  if(g.getout){ g.getout=0; return; }
  XtSetSensitive(a->form, True);

  w = a->w[0];
  ////  Redraw the label in the pushbutton widget that called the clickbox.
  ////  Stop Motif from trying to change size of the button
  doubletostring(c->fanswer, g.signif, tempstring);
  set_widget_label(w, tempstring);
  box = a->param[0];
  doubletostring(c->fanswer, g.signif, a->answer[box][0]); 
}


//--------------------------------------------------------------------------//
// dialogrgbclickboxcb - callback for getting a value in dialog box         //
// using multiclickbox().  Use widgets 1,2,and 3.                           //
//--------------------------------------------------------------------------//
void dialogrgbclickboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  uint color;
  int box=0, cmyk, j, k;

  dialoginfo *a = (dialoginfo*)client_data;
  ////  Figure out which box
  for(j=0;j<a->noofboxes;j++)
     for(k=0;k<4;k++)
        if(w==a->boxwidget[j][k] && w!=0) box = j;


  a->w[0] = w;
  a->param[0] = box;
  color = atoi(a->answer[box][0]);
  cmyk=0;
  getcolor("Color adjustment (128=normal)", &color, a->bpp, cmyk, 
     dialogrgbclickboxcb_part2, (void*)a);
}


//--------------------------------------------------------------------------//
// dialogrgbclickboxcb_part2                                                //
//--------------------------------------------------------------------------//
void dialogrgbclickboxcb_part2(clickboxinfo *c)
{
  int col[4];
  int k, bpp = 24;
  char tempstring[256];
  dialoginfo *a = (dialoginfo*)c[0].client_data;
  int box = a->param[0];
  valuetoRGB(c[0].answer,col[1],col[2],col[3],bpp); 
  
  //// Redraw the labels in the pushbutton widgets that called the clickbox
  //// (not the multiclickbox itself)
  for(k=1;k<4;k++)
  {    itoa(col[k], tempstring, 10);
       set_widget_label(a->boxwidget[box][k], tempstring);
  }
  itoa(c->answer, a->answer[box][0], 10);
  c->startval = c->answer;
} 


//--------------------------------------------------------------------------//
// dialogmulticlickboxcb - callback for getting a value in dialog box       //
// using multiclickbox().  Use widgets 0,1,and 2.                           //
//--------------------------------------------------------------------------//
void dialogmulticlickboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  int box=0, j, k;

  dialoginfo *a = (dialoginfo*)client_data;
  clickboxinfo *c;

  ////  Figure out which box
  for(j=0;j<a->noofboxes;j++)
     for(k=0;k<4;k++)
        if(w==a->boxwidget[j][k] && w!=0) box = j;

  c = new clickboxinfo[3];
  c[0].answers = new int[10];
  c[0].answers[0] = a->c[box]->answers[0];
  c[0].answers[1] = a->c[box]->answers[1];
  c[0].answers[2] = a->c[box]->answers[2];
  for(k=0; k<3; k++)
  {   c[k].title = new char[128];        
      c[k].type = VALSLIDER;
      c[k].buttonlabel = new char[128];        
      strcpy(c[k].title, a->boxpushbuttonlabel[box][k]);
      c[k].buttonlabel[0] = 0;
      c[k].startval = atoi(a->answer[box][k]);
      c[k].minval = a->c[box]->minval;
      c[k].maxval = a->c[box]->maxval;
      c[k].answers = c[0].answers;
      c[k].f1 = a->c[box]->f1;
      c[k].f2 = a->c[box]->f2;
      c[k].f3 = a->c[box]->f3;
      c[k].f4 = a->c[box]->f4;
      c[k].f5 = dialogmulticlickboxcb_part2;
      c[k].f6 = dialogmulticlickboxfinish;
      c[k].f7 = a->c[box]->f7;
      c[k].f8 = a->c[box]->f8;
      c[k].noofbuttons = 3;
      c[k].ptr[0] = a;
      c[k].param[0] = box;
  }
  multiclickbox(a->c[box]->title, 3, c, null, a->helptopic);
}


//--------------------------------------------------------------------------//
// dialogmulticlickboxcb_part2                                              //
//--------------------------------------------------------------------------//
void dialogmulticlickboxcb_part2(clickboxinfo *c)
{
  int k, box;
  char tempstring[256];
  dialoginfo *a = (dialoginfo*)c[0].ptr[0];
  box = c[0].param[0];
  sprintf(a->answer[box][0], "%d", c[0].answers[0]);
  sprintf(a->answer[box][1], "%d", c[0].answers[1]);
  sprintf(a->answer[box][2], "%d", c[0].answers[2]);
  //// Redraw the labels in the pushbutton widgets that called the clickbox
  //// (not the multiclickbox itself)
  for(k=0;k<3;k++)
  {    itoa(c[0].answers[k], tempstring, 10);
       set_widget_label(a->boxwidget[box][k+1], tempstring);
  }
}


//--------------------------------------------------------------------------//
// dialogmulticlickboxfinish                                                //
//--------------------------------------------------------------------------//
void dialogmulticlickboxfinish(clickboxinfo *c)
{
  int k;
  for(k=0; k<c[0].noofbuttons; k++)
  {     delete[] c[k].buttonlabel;
        delete[] c[k].title;
  }
  delete[] c[0].answers;
  delete[] c;          
}


//--------------------------------------------------------------------------//
// dialogstringcb - callback for getting a string in dialog box.            //
//--------------------------------------------------------------------------//
void dialogstringcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  int box=0,boxbutton=0,j,k,column=0;
  char *ptr;
  dialoginfo *a = (dialoginfo*)client_data;

  ////  Figure out which box
  for(k=0;k<4;k++)
  for(j=0;j<a->noofboxes;j++)
       if(w==a->boxwidget[j][k] && w!=0){ box = j; boxbutton=k; }
  column = boxbutton;

  ////  Put answer back in dialoginfo in case dialog remains on screen

  ptr = XmTextGetString(w);
  remove_trailing_space(ptr);
  if(ptr != NULL){  strcpy(a->answer[box][column], ptr); XtFree(ptr); }
  if(!strlen(a->lastanswer[box])) strcpy(a->lastanswer[box],"1");
  //// use radio = 0 since this must come from a box not a radio
  a->f1(a,0,box,boxbutton);
  if(strlen(a->answer[box][column])) 
      strncpy(a->lastanswer[box], a->answer[box][column], 99);
  return;
}


//--------------------------------------------------------------------------//
// dialogcursorcb - set cursor. Should not be necessary but Motif           //
// ignores cursor settings at initialization.                               //
//--------------------------------------------------------------------------//
void dialogcursorcb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data; w=w; client_data=client_data;

  int box=0,j,k,boxbutton=0,column=0;
  ////  Figure out which box
  dialoginfo *a = (dialoginfo*)client_data;
  for(k=0;k<4;k++)
  for(j=0;j<a->noofboxes;j++)
       if(w==a->boxwidget[j][k] && w!=0){ box = j; boxbutton=k; }
  column = boxbutton;
  int len =(int)strlen(a->answer[box][0]);
  ////  Set string and position at same time so it takes it.
  if(len)
     XtVaSetValues(w, XmNvalue, a->answer[box][column], XmNcursorPosition, len, NULL); 
}


//--------------------------------------------------------------------------//
// dialoglistcb - callback for getting item from list in dialog box.        //
// Calls list(), stores the selected string in the  'c->widget's            //
// XmNlabelString. The list data must be already allocated.                 //
// Don't allocate anything here, since this func may never be called.       //
//                                                                          //
// Caller must allocate and set title, and set f1, f2, f4, info, count,     //
// selection (which must point to a static int), and wantfunction.          // 
// l->ptr[0] and param[0] and [1] are reserved for use by dialog.           //
// Caller should not delete the list or info, this is done automatically.   //
//--------------------------------------------------------------------------//
void dialoglistcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(g.diagnose) printf("entering dialoglistcb\n");
  w=w; call_data=call_data;  // Keep compiler quiet
  int box=0,boxbutton=0,j,k;
  dialoginfo *a = (dialoginfo*)client_data;
  for(k=0; k<4; k++)
  for(j=0; j<a->noofboxes; j++)
       if(w==a->boxwidget[j][k] && w!=0){ box = j; boxbutton=k; }
  clickboxinfo *c = a->c[box];
  if(a->list_visible[box]) return; // Don't open list if list already up
  a->list_visible[box] = 1;
  listinfo *l = a->l[box];  

  //// Add values common to all dialog lists. List should already
  //// have a title, info, and count. 
  l->itemstoshow = 11;
  l->firstitem   = 0;
  l->wantsort    = 0;
  l->wantsave    = 0;
  l->allowedit   = c->allowedit;
  l->editcol     = c->editcol;
  l->editrow     = 0;
  l->helptopic   = c->helptopic;
  l->maxstringsize  = c->maxstringsize;
  l->width          = 0;
  l->transient      = 1;
  l->autoupdate     = 0;
  l->clearbutton    = 0;
  l->highest_erased = 0;
  l->f1 = dialoglistokcb;
  l->f2 = null;
  l->f3 = null;
  l->f4 = dialoglistcancelcb;
  l->f5 = null;
  l->f6 = null;
  l->c = c;
  l->ptr[0]   = a;
  l->param[0] = box;
  l->param[1] = boxbutton;
  l->finished = 0;
  list(l);
  if(g.diagnose) printf("leaving dialoglistcb\n");
}  


//--------------------------------------------------------------------------//
// dialoglistokcb                                                           //
// Don't deallocate anything here, since this func may never be called, or  //
// may be called twice.                                                     //
//--------------------------------------------------------------------------//
void dialoglistokcb(listinfo *l)
{
  int choice;
  clickboxinfo *c = l->c;
  dialoginfo *a = (dialoginfo *)l->ptr[0];
  c->answer = *l->selection; 
  int box = l->param[0];
  int boxbutton = l->param[1];
  if(!a->list_visible[box]) return;  // Don't accept value if canceled
  ////  Redraw the label in the pushbutton widget that called the clickbox.
  set_widget_label(c->widget[0], c->list[c->answer]);
  choice = c->answer;
  strcpy(a->answer[box][0], a->l[box]->info[choice]);
  *a->l[box]->selection = choice;
  a->f1(a, 0, box, boxbutton);
  g.getout=0;
  dialoglistcancelcb(l);
}


//--------------------------------------------------------------------------//
// dialoglistcancelcb                                                       //
// Don't deallocate anything here, since this func may never be called, or  //
// may be called twice.                                                     //
//--------------------------------------------------------------------------//
void dialoglistcancelcb(listinfo *l)
{
  dialoginfo *a = (dialoginfo *)l->ptr[0];
  int box = l->param[0];
  a->list_visible[box] = 0;
  if(a->l[box]->browser && XtIsManaged(a->l[box]->browser)) 
      XtUnmanageChild(a->l[box]->browser);
}


//--------------------------------------------------------------------------//
// dialogchangecicb                                                         //
//--------------------------------------------------------------------------//
void dialogchangecicb(Widget w, XtP client_data, XmACB *call_data)
{
    w=w;call_data=call_data;      // keep compiler quiet
    dialoginfo *a = (dialoginfo *)client_data;
    g.want_switching =0;
    armboxcb(0, a, 0);
    g.want_switching =1;
}


//--------------------------------------------------------------------------//
// armboxcb - callback for dialog-specific functions when button is         //
// clicked in dialog. Togglecb must be called before armboxcb in            //
// radioboxes.                                                              //
//--------------------------------------------------------------------------//
void armboxcb(Widget w, XtP client_data, XmACB *call_data)
{
    call_data=call_data;      // keep compiler quiet
    int i,j,k,radio=-1,box=-1,boxbutton=-1;
    dialoginfo *a = (dialoginfo *)client_data;
    if(w == NULL) return;

    for(i=0; i<a->noofradios; i++)
    for(j=0; j<a->radiono[i]; j++)
       if(w == a->radiowidget[i][j]) radio=i;

    ////  Set the cursor position on any string boxes. This should be done
    ////  at initialization but Motif ignores it.
    
    for(k=0;k<4;k++)
    for(j=0;j<a->noofboxes;j++)
    {   if(w == a->boxwidget[j][k] && w!=0)
        {   boxbutton=k;  box=j; }        
    }
    a->f1(a,radio,box,boxbutton);

    for(j=0;j<a->noofboxes;j++)
       if(a->boxtype[j]==STRING || a->boxtype[j]==SCROLLEDSTRING)
          dialogcursorcb(a->boxwidget[j][0],client_data,call_data);
}


//--------------------------------------------------------------------------//
// toggleboxcb - callback to toggle a box button in dialog                  //
//--------------------------------------------------------------------------//
void toggleboxcb(Widget w, XtP client_data, XmACB *call_data)
{
    call_data=call_data;      // keep compiler quiet
    int j, state;
    state = XmToggleButtonGetState(w);
    dialoginfo *a = (dialoginfo *)client_data;

    for(j=0;j<a->noofboxes;j++)
       if(w==a->boxwidget[j][0] && w!=0)  // Toggle button is always no.0
       {  if(state==True) a->boxset[j]=1; else a->boxset[j]=0;
          XmToggleButtonSetState(w, state, False);
       }
}


//--------------------------------------------------------------------------//
// togglecb - callback to toggle a list button in dialog                    //
//--------------------------------------------------------------------------//
void togglecb(Widget w, XtP client_data, XmACB *call_data)
{
    call_data=call_data;      // keep compiler quiet
    int i,j,state;
    state = XmToggleButtonGetState(w);
    dialoginfo *a = (dialoginfo *)client_data;

    ////  Lesstif hack
#ifdef LESSTIF_VERSION
    for(i=0;i<a->noofradios;i++)
    for(j=0;j<a->radiono[i];j++)
       if(w==a->radiowidget[i][j]) 
       {   if(state==True)
           {   state=False;
               XmToggleButtonSetState(w, False, True);
           }else
           {   XmToggleButtonSetState(w, True, True);
	       state=True;
	   }
       }
#endif

    for(i=0;i<a->noofradios;i++)
    for(j=0;j<a->radiono[i];j++)
       if(state==True && w==a->radiowidget[i][j]) a->radioset[i]=j;
}


//--------------------------------------------------------------------------//
// addbbbuttons                                                             //
//--------------------------------------------------------------------------//
void addbbbuttons(dialoginfo *a, Widget bb, Widget *buttons, int y, 
  int helptopic, int width) 
{
  int x1, x2, x3, buttonwidth;
  if(width >= 330 || width==0)  // 0 = don't care
  {   x1 = 1; 
      x2 = 115; 
      x3 = 225; 
      buttonwidth = 100;
  }else 
  {   buttonwidth = (width-14)/3;
      x1 = 1;
      x2 = 4 + buttonwidth + 9;
      x3 = 4 + 2 * buttonwidth + 10;
  }
  add_all_bbbuttons(a, bb, buttons, y,
      (char*)"Accept", x1, 
      (char*)"Dismiss", x2, 
      (char*)"Help", x3, buttonwidth, helptopic);
}


//--------------------------------------------------------------------------//
// add_all_bbbuttons                                                        //
//--------------------------------------------------------------------------//
void add_all_bbbuttons(dialoginfo *a, Widget bb, Widget *button, int y,
   char *s1, int x1, 
   char *s2, int x2, 
   char *s3, int x3, int buttonwidth, int helptopic)
{  
   Widget okbutton;
   button[0] = add_bb_button(bb, s1, x1, y-5, buttonwidth, 16, True);
   okbutton = button[0];
   button[1] = add_bb_button(bb, s2, x2, y,   buttonwidth, 16, False);
   button[2] = add_bb_button(bb, s3, x3, y,   buttonwidth, 16, False);

   XtAddCallback(button[0], XmNactivateCallback, (XtCBP)dialogokcb, (XtP)a);
   XtAddCallback(button[1], XmNactivateCallback, (XtCBP)dialogcancelcb, (XtP)a);
   XtAddCallback(button[2], XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);

   ////  Make Enter & Escape Keys the same as Mouse Click On The Buttons.  
   XtAddEventHandler(button[0], KeyPressMask, FALSE, (XtEH)dialogentercb, (XtP)a);
   XtAddEventHandler(button[1], KeyPressMask, FALSE, (XtEH)dialogentercb, (XtP)a);
   XtAddEventHandler(button[2], KeyPressMask, FALSE, (XtEH)dialogentercb, (XtP)a);
}


//--------------------------------------------------------------------------//
// add_bb_button  - generic button for bulletin board widget                //
//--------------------------------------------------------------------------//
Widget add_bb_button(Widget parent, char *s, int x, int y, int w, int h, 
  int want_default)
{
   Arg args[100];
   Cardinal n;
   Widget button;
   n=0;
   XtSetArg(args[n],XmNresizable,       False); n++;
   XtSetArg(args[n],XmNx,               x); n++;
   XtSetArg(args[n],XmNy,               y); n++;
   XtSetArg(args[n],XmNheight,          h); n++;
   XtSetArg(args[n],XmNwidth,           w); n++;
   XtSetArg(args[n],XmNdefaultButton,   want_default); n++;
   XtSetArg(args[n],XmNshowAsDefault,   want_default); n++;
   XtSetArg(args[n],XmNsensitive,       True); n++;
   XtSetArg(args[n],XmNnavigationType,  XmEXCLUSIVE_TAB_GROUP); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   button = XmCreatePushButton(parent, s, args, n);  // OK button
   XtManageChild(button);
   return button;
}


//--------------------------------------------------------------------------//
// dialogentercb                                                            //
//--------------------------------------------------------------------------//
void dialogentercb(Widget w, XtP client_data, XEvent *event)
{
   w=w; client_data=client_data;   
   int key = which_key_pressed(event);
   //// This may or may not do anything.
   if(key==XK_Return) dialogokcb(w, client_data, (XmACB*)NULL);  
   if(key==XK_Escape) dialogcancelcb(w, client_data, NULL);  
}


//--------------------------------------------------------------------------//
// dialogokcb                                                               //
//--------------------------------------------------------------------------//
void dialogokcb(Widget w, XtP client_data, XmACB *call_data)
{
  int j, k;
  w=w;client_data=client_data;call_data=call_data;   
  dialoginfo *a = (dialoginfo *)client_data;

  ////  Put result in answer boxes.   Only needed for widgets that   
  ////  don't have callbacks.

  for(j=0;j<a->noofboxes;j++)
  {  switch(a->boxtype[j])
     {       case FILENAME:                                         // Filename 
                strcpy(a->answer[j][0], a->clickboxdata[j].title);  // Single 
                break;
             case MULTIFILENAME:                                    // Multiple

                ////  Caller must XtFree a->clickboxdata[j].list[k]'s 
                ////  and delete[] clickboxdata[j].list

                a->boxcount[j] = a->clickboxdata[j].count;

                ////  User did not select any files so no list was created
                ////  in fsokcb().  Create a list and use the old filename.

                if(a->boxcount[j]==0)
                {   a->clickboxdata[j].list = new char*[1];          
                    a->clickboxdata[j].list[0] = XtMalloc(FILENAMELENGTH); 
                    strcpy(a->clickboxdata[j].list[0], a->clickboxdata[j].title); 
                    a->boxcount[j]=1;
                }
                a->boxlist[j] = a->clickboxdata[j].list;
                strcpy(a->answer[j][0], a->boxlist[j][0]);
                break;
     }
  }

  //// ENTER_KEY = -2 = f1 was called by Ok button

  //// If f1 or f2 go into an event loop or check events, they must
  //// increment g.waiting and decrement it when finished, to prevent
  //// the dialog box from being deallocated.
  
  a->f1(a,ENTER_KEY,0,0);  
  //if(!g.getout) a->f2(a,ENTER_KEY,0,0);  
  for(k=0;k<g.openlistcount;k++) update_list(k);
  for(k=0;k<g.image_count;k++) 
        if(z[k].s->visible && z[k].touched) putimageinspreadsheet(k);
  update_sample_palette();
}


//--------------------------------------------------------------------------//
// dialogunmapcb - called when user closes a dialog box using window        //
// manager instead of the dismiss button like they are supposed to.         //
//--------------------------------------------------------------------------//
void dialogunmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
   g.waiting = 0;  // Can't block WM from deleting our window
   g.block = 0;
   g.busy = 0;
   if(!in_dialogcancelcb) dialogcancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// dialogcancelcb                                                           //
//--------------------------------------------------------------------------//
void dialogcancelcb(Widget w, XtP client_data, XmACB *call_data)
{  
   //// Don't close dialog box if some operation called by it is still
   //// running. This will cause a crash even if you don't access the
   //// widgets, because X11 still accesses them internally. Example:
   //// start a 15x15 filter -> cancel dialog -> crash.
  
   if(g.busy) return;
   if(g.diagnose) printf("entering dialogcancelcb\n");
   w=w;client_data=client_data;call_data=call_data;   
   int j,k;
   dialoginfo *a = (dialoginfo *)client_data;
   a->f9(a);                // Preliminary stuff to do when cancel button is clicked
                            // for example, setting a->busy to 0
   if(a->busy) return;
   g.getout = 1;
   XFlush(g.display);
   for(k=0;k<DCOUNT;k++) a->boxcount[k]=0;

  //// If f3 goes into an event loop or check events, it must
  //// increment g.waiting and decrement it when finished, to 
  //// prevent the dialog box from being deallocated. 

   a->f3(a);                // Put any extra stuff to do in f3
   g.block = max(0, g.block);
   if(g.block) return;      // Waiting for input, don't deallocate
   if(g.diagnose) printf("dialogcancelcb calling f4\n");
   a->f4(a);                // Put any extra stuff to delete in f4
   if(g.diagnose) printf("dialogcancelcb back from f4\n");
   //// No returns past this point

   //// Unmap all the kids too if they are filename widgets
   for(j=0;j<a->noofboxes;j++)
   {    switch(a->boxtype[j])
        {   case FILENAME:
            case MULTIFILENAME:
                 if(a->c[j]->form) fsunmapcb(0, a->c[j], 0);
                 break;
        }
   }
   in_dialogcancelcb = 1;
   if(!a) return;
   char dirmask[FILENAMELENGTH]="*";
   for(j=0;j<a->noofboxes;j++)
   {  
        switch(a->boxtype[j])
        {   case FILENAME:
            case MULTIFILENAME:
               if(a->clickboxdata[j].dirmask) 
               { 
                    strcpy(dirmask,a->clickboxdata[j].dirmask);
                    delete[] a->clickboxdata[j].dirmask;
               }
               ////  The widgets were already destroyed in getfilenames().
               if(a->clickboxdata[j].w) delete[] a->clickboxdata[j].w;
               break;
            case LIST:
            case NON_EDIT_LIST:
            case NO_LABEL_LIST:
                delete_list(a->l[j]);
               break;
        }
   }
   if(a->want_changecicb)
        XtRemoveEventHandler(g.drawing_area, SubstructureNotifyMask, 
            FALSE, (XtEH)dialogchangecicb, (XtP)a);
   XtUnmanageChild(a->form);
   a->clickboxdata[1].done=1;
   if(a->clickboxdata!=NULL) delete[] a->clickboxdata;
   a->clickboxdata = 0;
   for(k=0;k<a->widgetcount;k++) 
   {   if(a->widget[k]!=NULL)
       {   XtDestroyWidget(a->widget[k]);
           a->widget[k]=0;
       }
   }
   if(a->widget!=NULL) delete[] a->widget;
   a->widget=0;
   a->form=0;
   delete a;
   a = NULL;
   in_dialogcancelcb = 0;
   //// g.getout should be 1 at this point in case caller is in a loop.
   g.indialog = max(0, g.indialog-1);
   if(g.diagnose) printf("leaving dialogcancelcb\n");
}



//--------------------------------------------------------------------------//
// dialogfilenamecb - callback for getting filename or list of              //
// filenames in dialog box.                                                 //
//--------------------------------------------------------------------------//
void dialogfilenamecb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  int box=0, j, k;
  dialoginfo *a = (dialoginfo*)client_data;
  for(k=0;k<4;k++)
  for(j=0;j<a->noofboxes;j++)
       if(w==a->boxwidget[j][k] && w!=0) box = j;
  clickboxinfo *c = a->c[box];
  c->ptr[0] = a;
  c->widget[0] = w;
  //// If c->type is SINGLE, one filename is returned in c->title.
  //// Otherwise, it allocates a list of filenames and puts it in array c->list[].

  if(c->type == SINGLE)
      getfilename(c->title, c->path, c->widget[0]);
  else
      getfilenames(c);
}


//--------------------------------------------------------------------------//
// dialogexposecb - callback if dialog box is exposed                       //
//--------------------------------------------------------------------------//
void dialogexposecb(Widget w, XtP client_data, XtP *call_data)
{
  w=w; call_data=call_data;  // Keep compiler quiet
  dialoginfo *a = (dialoginfo*)client_data;
  a=a;
}


//--------------------------------------------------------------------------//
// multitoggleboxokcb                                                       //
//--------------------------------------------------------------------------//
void multitoggleboxokcb(Widget w, XtP client_data, XmACB *call_data)
{
  call_data=call_data;      // keep compiler quiet
  int togglebox=0, box=0, k, state;
  p3d* p = (p3d*)client_data;
  dialoginfo *a = (dialoginfo *)p->ptr[0];
  Widget *tb = (Widget*) p->ptr[2];

  for(k=0; k<p->param[0]; k++) if(w==tb[k] && w!=0) togglebox = k;
  box = p->param[1];
  state = XmToggleButtonGetState(w);
  if(state!=True) a->boxset[box] |= (1<<(togglebox-1)); 
  else            a->boxset[box] &= ~(1<<(togglebox-1));
}


//--------------------------------------------------------------------------//
// multitoggleboxcancelcb                                                   //
//--------------------------------------------------------------------------//
void multitoggleboxcancelcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data; call_data=call_data;
  int k;
  p3d* p = (p3d*)client_data;
  Widget *ww = (Widget*) p->ptr[1];
  XtUnmanageChild(p->form);
  for(k=0; k<p->wc; k++) if(ww[k] != NULL) XtDestroyWidget(ww[k]);
  if(ww != NULL) delete[] ww;
  delete p;
}


//--------------------------------------------------------------------------//
// multitoggleboxcb                                                         //
//--------------------------------------------------------------------------//
void multitoggleboxcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data; call_data=call_data;
  static p3d *p;
  p = new p3d;
  Arg args[100];
  Widget *ww, *tb, dialogshell, bb, okaybut, cancbut, helpbut;  
  ww = new Widget[50];
  tb = new Widget[50];
  int box=0, j, k, n, xsize, ysize, x, y, wc=0, togglebox=0;
  int helptopic = 72;
  dialoginfo *a = (dialoginfo*)client_data;
  for(j=0; j<a->noofboxes; j++)
  for(k=0; k<a->boxcount[j]; j++)
      if(w==a->boxwidget[j][k] && w!=0){ box=j; togglebox = k; }
  xsize = 170;
  ysize = 50 + 20*a->boxcount[box];

  //------- Create a dialog shell widget ----------------------------//
  dialogshell = ww[wc++] = XtVaCreateManagedWidget("GetStrings",
                                xmDialogShellWidgetClass,  g.main_widget,
                                XmNx,                      g.main_xpos+100,
                                XmNy,                      g.main_ypos+100,
                                XmNwidth,                  xsize,
                                XmNheight,                 ysize,
                                XmNminWidth,               xsize+2,
                                XmNresizable,              False,
                                XmNtitle,                  "Select Options",
                                XmNautoUnmanage,           False,
                                NULL);

  //------- Create a bulletin board widget --------------------------//
  n=0;
  XtSetArg(args[n], XmNwidth, xsize); n++;
  XtSetArg(args[n], XmNheight, ysize); n++;
  XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
  XtSetArg(args[n], XmNresizable, False); n++;
  XtSetArg(args[n], XmNautoUnmanage, False); n++;
  bb = ww[wc++] = XmCreateBulletinBoard(dialogshell, (char*)"MultiToggleBB", args, n);

  okaybut = ww[wc++] = add_bb_button(bb, (char*)"Accept",   1, ysize-25, 55, 16, 1);
  cancbut = ww[wc++] = add_bb_button(bb, (char*)"Dismiss", 65, ysize-21, 55, 16, 0);
  helpbut = ww[wc++] = add_bb_button(bb, (char*)"Help",   117, ysize-21, 55, 16, 0);

  XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)multitoggleboxcancelcb, (XtP)p);
  XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)multitoggleboxcancelcb, (XtP)p);
  XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);

  ////  Add callbacks so hitting Return activates Ok button
  for(k=0; k<wc; k++)
     XtAddEventHandler(ww[k], KeyPressMask, FALSE, (XtEH)getstringentercb, (XtP)bb);

  XtManageChild(dialogshell);
  XtManageChild(bb);

  //------- Add a bunch of toggle boxes -----------------------------//
  n=0;
  XtSetArg(args[n], XmNresizable, False); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNnoResize, True); n++;
  XtSetArg(args[n], XmNspacing, 0); n++;
  XtSetArg(args[n], XmNadjustLast, False); n++;
  XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
  XtSetArg(args[n], XmNshadowThickness, 0); n++;
  XtSetArg(args[n], XmNallowOverlap, True); n++;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
#ifdef MOTIF2
  XtSetArg(args[n], XmNindicatorOn, XmINDICATOR_CROSS_BOX); n++;
#endif
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }

  x = 5; 
  y = 20;
  for(k=0; k<a->boxcount[box]; k++)
  {   n = 0;              
      XtSetArg(args[n], XmNx, x); n++;
      XtSetArg(args[n], XmNy, y); n++;    
      XtSetArg(args[n], XmNwidth, 150); n++;
      XtSetArg(args[n], XmNheight, 20); n++;
      XtSetArg(args[n], XmNresizable, False); n++; 
      XtSetArg(args[n], XmNspacing, 1); n++;
      if(k==0)
      {    tb[k] = ww[wc++] = XmCreateLabel(bb, a->boxlist[box][k], args, n);
      }else
      {   if(a->boxset[box] & (1<<(k-1)))
              XtSetArg(args[n], XmNset, True); 
          else
              XtSetArg(args[n], XmNset, False);
          n++;
          tb[k] = ww[wc++] = XmCreateToggleButton(bb, a->boxlist[box][k], args, n);
      }
      XtManageChild(tb[k]);
      XtAddCallback(tb[k], XmNarmCallback, (XtCBP)multitoggleboxokcb,(XtP)p);
      y += 20;
  }      
  p->wc = wc;
  p->ptr[0] = a;
  p->ptr[1] = ww;
  p->ptr[2] = tb;
  p->form = dialogshell;
  p->param[0] = a->boxcount[box];
  p->param[1] = box;
}
