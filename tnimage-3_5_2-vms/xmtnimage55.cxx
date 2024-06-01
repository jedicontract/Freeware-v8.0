//--------------------------------------------------------------------------//
// xmtnimage55.cc                                                           //
// messages                                                                 //
// Latest revision: 01-26-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern int torn_off;
int in_message=0;
int in_destroywidget = 0;

//--------------------------------------------------------------------//
// message (Motif version)                                            //
// valid modes                                                        //
// -----------                                                        //
// NONE                                                               //
// INFO                                                               //
// PROMPT                                                             //
// QUESTION                                                           //
// YESNOQUESTION                                                      //
// WARNING                                                            //
// ERROR                                                              //
// BUG                                                                //
// Space for the returnstring must be already allocated.              //
// 'maxstringsize' must be set to the buffer size of `returnstring' if//
//    mode is PROMPT.  The string will be truncated to this size.     //
// Returns one of:                                                    //
//   YES = user clicked Yes to yes/no question or Ok                  //
//   NO  = user clicked No button                                     //
//   CANCEL = user clicked Cancel button                              // 
//--------------------------------------------------------------------//
int message(const char *heading){  return message(heading, (char*)NULL, INFO, 0, 0); }

int message(const char *heading, int mode, int helptopic)
{  
   if(mode==PROMPT)
   {   fprintf(stderr,"Error in message, line %d\n",__LINE__);
       return BUG;
   }
   return message(heading, (char*)NULL, mode, 50, helptopic); 
}
int message(const char *heading, char *returnstring, int mode, int maxstringsize,
   int helptopic)
{ 
   // Default is simplemessage, which doesn't block so is safe to call
   // in callbacks. If callback calls a function that blocks, motif for 
   // unknown reason calls the callback repeatedly.
   if(!g.want_messages) return YES;
   switch(mode)
   {  case INFO: case NONE: case ERROR: case WARNING: case BUG:
         simplemessage(heading, mode, helptopic); 
         return YES; 
   }
   XFlush(g.display);
   XSync(g.display, 0);
   int want_block;
   Widget www=0;
   clickboxinfo *c;
   int answer=0;
   int alloc1=0, alloc3=0, n, ostate;
   XmString xms, xms2, xms3=NULL;
   Arg args[100];
   Widget w2=0, w, helpwidget, menuwidget;
   ostate = g.state;
   g.state = MESSAGE;
   c = new clickboxinfo;
   if(c == NULL) return CANCEL;

   c->title = new char[FILENAMELENGTH];
   if(c->title == NULL) return CANCEL;
   in_message = 1;

   if(mode==PROMPT && returnstring!=NULL && strlen(returnstring)) 
        strcpy(c->title, returnstring);
   else c->title[0] = 0;
   
   if(!helptopic) c->helptopic = 34; else c->helptopic = helptopic;
   c->answer = 0;
   c->form = NULL; c->path = NULL;
   c->f1 = null;   c->f2 = null;
   c->f3 = null;   c->f4 = null;
   c->f5 = null;   c->f6 = null;
   c->f7 = null;   c->f8 = null;
   c->answers = NULL; 
   c->maxstringsize = maxstringsize;

   xms = XmStringCreateLtoR((char*)heading, XmFONTLIST_DEFAULT_TAG);
   n = 0;
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNnoResize, False); n++;
   XtSetArg(args[n], XmNselectionLabelString, xms) ;n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
 
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }

   switch(mode)  
   {  case PROMPT:
         XtSetArg(args[n], XmNtitle, "Information needed"); n++;
         if(returnstring[0] && returnstring!=NULL)
         {   alloc3=1;
             XtSetArg(args[n], XmNtextString,xms3=XmStringCreateSimple(returnstring));n++;
         }
         www = XmCreatePromptDialog(g.main_widget, (char*)"Prompt", args, n);
         if(g.want_colormaps)
         {    w = XtNameToWidget(www, "Text");
              XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
         }
         if(alloc3) XmStringFree(xms3); 
         break;
      case QUESTION:         // Ok, cancel, help
         XtSetArg(args[n], XmNtitle, "Question"); n++;
         www = XmCreateQuestionDialog(g.main_widget, (char*)"Question", args, n);
         break;
      case YESNOQUESTION:    // Yes, no, cancel, help
         XtSetArg(args[n], XmNtitle, "Question"); n++;
         XtSetArg(args[n], XmNokLabelString, xms2=XmStringCreateSimple((char*)"Yes")); n++;
         www = XmCreateQuestionDialog(g.main_widget, (char*)"YesNoQuestion", args, n);
         n = 0;
         ////  Stop Motif from trying to grab another color if none are available.
         if(g.want_colormaps)  
         {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
              XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
         }
         w2 = XmCreatePushButton(www, (char*)"No", args, n);
         XtManageChild(w2);
         alloc1 = 1;
         XtAddCallback(w2, XmNactivateCallback, (XtCBP)nomsgcb, (XtP)c);
         XmStringFree(xms2);
        break;
   }

   if(mode==PROMPT)
       XtAddCallback(www, XmNokCallback, (XtCBP)promptmsgcb, (XtP)c);
   else
       XtAddCallback(www, XmNokCallback, (XtCBP)okmsgcb, (XtP)c);
   XtAddCallback(www, XmNcancelCallback, (XtCBP)cancelmsgcb, (XtP)c);

   helpwidget = XtNameToWidget(www, "Help");
   XtAddCallback(helpwidget, XmNactivateCallback, (XtCBP)helpmsgcb, (XtP)c);
   XtAddCallback(www, XmNunmapCallback, (XtCBP)messageunmapcb, (XtP)c);

   XtManageChild(www);

   want_block = 1;
   g.waiting++;
   c->ptr[7] = &want_block;
   menuwidget = desensitize_tearoff();
   block(www, &want_block);
   resensitize_tearoff(menuwidget);

   if(mode==PROMPT && c->title!=NULL && c->title[0] && returnstring!=NULL)
   { 
       c->title[FILENAMELENGTH-2] = 0;
       if(strlen(c->title)) strcpy(returnstring, c->title); 
   }
   g.state = ostate;
   if(alloc1) XtDestroyWidget(w2);
   if(c==NULL){ g.waiting=max(0, g.waiting-1); in_message = 0; return CANCEL; }
   answer = c->answer;

   if(mode==PROMPT) XtRemoveCallback(www, XmNokCallback, (XtCBP)promptmsgcb, (XtP)c);
   else XtRemoveCallback(www, XmNokCallback, (XtCBP)okmsgcb, (XtP)c);
   XtRemoveCallback(www, XmNcancelCallback, (XtCBP)cancelmsgcb, (XtP)c);
   XtRemoveCallback(helpwidget, XmNactivateCallback, (XtCBP)helpmsgcb, (XtP)c);

   if(c->title != NULL) delete[] c->title;
   c->title = NULL;
   if(c != NULL) delete c;
   c = NULL;
   if(xms!=NULL) XmStringFree(xms);
   in_destroywidget = 1;
   if(www) XtDestroyWidget(www);
   in_destroywidget = 0;
   in_message = 0;
   return answer;
}


//--------------------------------------------------------------------//
// desensitize_tearoff - desensitize most recent menu so user can't   //
// press Esc on it, and return its identity so it can be reactivated. //
//--------------------------------------------------------------------//
Widget desensitize_tearoff(void)
{
   static Widget widget, history;
   int k;
   XtVaGetValues(g.menubar, XmNmenuHistory, &history, NULL);
   if(history == 0) return 0;
   widget = XtParent(history);
   for(k=0; k<10; k++) 
   {  if(g.torn_off[k] && widget==g.menupane[k] && widget!=0) 
      {    XtSetSensitive(g.menupane[k], False);
           return widget;
      }
   }
   return 0;
}


//--------------------------------------------------------------------//
// resensitize_tearoff                                                //
//--------------------------------------------------------------------//
void resensitize_tearoff(Widget w)
{
   if(w) XtSetSensitive(w, True);
}


//--------------------------------------------------------------------//
// okmsgcb - callback for message box ok button                       //
//--------------------------------------------------------------------//
void okmsgcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;client_data=client_data;call_data=call_data; // keep compiler quiet
  clickboxinfo *c = (clickboxinfo *)client_data;
  if(c==NULL) return;
  c->answer = YES;  
  int *want_block = (int *)c->ptr[7]; // unset want_block
  if(want_block) *want_block = 0;
  g.waiting = max(0, g.waiting-1);
}


//--------------------------------------------------------------------//
// cancelmsgcb -  callback for message box cancel button              //
//--------------------------------------------------------------------//
void cancelmsgcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;client_data=client_data;call_data=call_data; // keep compiler quiet
  clickboxinfo *c = (clickboxinfo *)client_data;
  if(c==NULL) return;
  c->answer = CANCEL;
  g.getout = 1;
  int *want_block = (int *)c->ptr[7]; // unset want_block
  if(want_block) *want_block = 0;
  g.waiting = max(0, g.waiting-1);
  //// Don't delete clickbox here, it will crash if user closes window
  //// from WM
}


//--------------------------------------------------------------------//
// helpmsgcb -  callback for message help button                      //
//--------------------------------------------------------------------//
void helpmsgcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;client_data=client_data;call_data=call_data; // keep compiler quiet
  clickboxinfo *c = (clickboxinfo *)client_data;
  if(c==NULL) return;
  help(c->helptopic);
  int *want_block = (int *)c->ptr[7]; // unset want_block
  if(want_block) *want_block = 0;
  g.waiting = max(0, g.waiting-1);
}


//--------------------------------------------------------------------//
// nomsgcb -  callback for message box 'no' button                    //
//--------------------------------------------------------------------//
void nomsgcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;client_data=client_data;call_data=call_data; // keep compiler quiet
  clickboxinfo *c = (clickboxinfo *)client_data;
  if(c==NULL) return;
  c->answer = NO;       // This is not zero!
  int *want_block = (int *)c->ptr[7]; // unset want_block
  if(want_block) *want_block = 0;
  g.waiting = max(0, g.waiting-1);
}


//--------------------------------------------------------------------//
// promptmsgcb - callback for message box ok button to get string     //
//--------------------------------------------------------------------//
void promptmsgcb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w;client_data=client_data;call_data=call_data; // keep compiler quiet
  clickboxinfo *c = (clickboxinfo *)client_data;
  if(c==NULL) return;
  char *string;         
  XmSelectionBoxCallbackStruct *ptr;
  ptr = (XmSelectionBoxCallbackStruct *)call_data;
  ////  This function allocates space so strcpy it then free it.
  XmStringGetLtoR(ptr->value, XmFONTLIST_DEFAULT_TAG, &string);
  ////  Make sure it is short enough
  if(string!=NULL && string[0] && c->title != NULL)
  {  
      strcpy(c->title, string);
      c->title[c->maxstringsize-1] = 0;
      XtFree(string);
  }
  c->answer = YES;
  int *want_block = (int *)c->ptr[7]; // unset want_block
  if(want_block) *want_block = 0;
  g.waiting = max(0, g.waiting-1);
}
 
 
//--------------------------------------------------------------------//
//  simplemessage                                                     //
//--------------------------------------------------------------------//
void simplemessage(const char *heading, int mode, int helptopic)
{
  Widget www, helpwidget;
  Arg args[100];
  XmString xms;
  int n=0;
  if(mode==NONE || !g.want_messages) return;
  g.state = MESSAGE;
  xms = XmStringCreateLtoR((char*)heading, XmFONTLIST_DEFAULT_TAG);
  n = 0;
  XtSetArg(args[n], XmNmessageString, xms); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  switch(mode)
  {   case ERROR:
         XtSetArg(args[n], XmNtitle, (char*)"Error"); n++;
         www = XmCreateErrorDialog(g.main_widget, (char*)"Error", args, n);
         break;
      case BUG:
         XtSetArg(args[n], XmNtitle, (char*)"Error"); n++;
         www = XmCreateErrorDialog(g.main_widget, (char*)"Bug", args, n);
         break;
      case WARNING:
         XtSetArg(args[n], XmNtitle, (char*)"Warning"); n++;
         www = XmCreateWarningDialog(g.main_widget, (char*)"Warning", args, n);
         break;
      default:
         XtSetArg(args[n], XmNtitle, (char*)"Information"); n++;
         www = XmCreateInformationDialog(g.main_widget, (char*)"Information", args, n);
         break;
  }
  helpwidget = XtNameToWidget(www, (char*)"Help");
  XtAddCallback(www, XmNokCallback, (XtCBP)simplemessagecb, (XtP)NULL);
  XtAddCallback(www, XmNcancelCallback, (XtCBP)simplemessagecb, (XtP)NULL);
  XtAddCallback(helpwidget, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(www, XmNunmapCallback, (XtCBP)simplemessageunmapcb, (XtP)NULL);
  XtManageChild(www);
  XmStringFree(xms);
  check_event();
}


//--------------------------------------------------------------------//
// simplemessagecb                                                    //
//--------------------------------------------------------------------//
void simplemessagecb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;client_data=client_data;
  g.state = NORMAL;
}


//--------------------------------------------------------------------------//
// simplemessageunmapcb - called when user closes a message box using window//
// manager instead of the cancel button.                                    //
//--------------------------------------------------------------------------//
void simplemessageunmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
  w=w; call_data=call_data;client_data=client_data;
//   simplemessagecb(w, client_data, call_data);
//  if(!in_destroywidget) cancelmsgcb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// messageunmapcb - called when user closes a message box using window      //
// manager instead of the cancel button.                                    //
//--------------------------------------------------------------------------//
void messageunmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
  w=w; call_data=call_data;client_data=client_data;
//  if(!in_destroywidget) cancelmsgcb(w, client_data, call_data);
}



//--------------------------------------------------------------------//
//  tempmessageopen                                                   //
//--------------------------------------------------------------------//
void tempmessageopen(Widget &www)
{
   int n;
   XmString xms=NULL;
   char heading[128] = "Message";
   Arg args[100];
   Widget w;
   if(!g.want_messages) return;
   n = 0;
   XtSetArg(args[n], XmNtitle, heading); n++;
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNwidth, 220); n++;
   XtSetArg(args[n], XmNheight, 50); n++;
//   XtSetArg(args[n], XmNnoResize, True); n++;
//   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNtransient, False); n++;
   xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNselectionLabelString, xms) ;n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   www = XmCreateWorkingDialog(g.main_widget, heading, args, n);
   w = XmMessageBoxGetChild(www, XmDIALOG_OK_BUTTON);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www, XmDIALOG_CANCEL_BUTTON);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www, XmDIALOG_SEPARATOR);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www, XmDIALOG_SYMBOL_LABEL);
   XtUnmanageChild(w);

   XtManageChild(www);
   XtAddCallback(www, XmNunmapCallback, (XtCBP)tempmessage_unmapcb, (XtP)NULL);
   XFlush(g.display);

   n = 0;
   XtAddEventHandler(XtParent(www), 
        FocusChangeMask | VisibilityChangeMask, 
        False, (XtEH)raisecb, (XtP)NULL);
   XtAddEventHandler(XtParent(www), StructureNotifyMask, False, 
       (XtEH)tempmessage_configurecb, (XtP)NULL);
   XtVaSetValues(www, XmNdeleteResponse, XmDO_NOTHING, NULL); // Prevent Window manager from closing it
   XmStringFree(xms);
   XFlush(g.display);  
   XtVaSetValues(www, XmNx, g.main_xpos-120, XmNy, g.main_ypos-16, NULL); 
}


//--------------------------------------------------------------------//
// tempmessageupdate                                                  //
//--------------------------------------------------------------------//
void tempmessageupdate(Widget www, char *heading)
{  

   if(!g.want_messages) return;
   XmString xms=NULL;
   xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
   XtVaSetValues(www, XmNmessageString, xms, NULL);
   XFlush(g.display);

}


//--------------------------------------------------------------------------//
// tempmessageclose                                                         //
//--------------------------------------------------------------------------//
void tempmessageclose(Widget www)
{  
   if(!g.want_messages) return;
   XtUnmanageChild(www);
   XtDestroyWidget(www);
   XFlush(g.display);
}


//--------------------------------------------------------------------------//
// tempmessage_unmapcb - called when user closes a message box using window //
// manager instead of the cancel button.                                    //
//--------------------------------------------------------------------------//
void tempmessage_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
  if(!g.want_messages) return;
  w=w; call_data=call_data;client_data=client_data;
}


//--------------------------------------------------------------------------//
// tempmessageconfigurecb                                                   //
//--------------------------------------------------------------------------//
void tempmessage_configurecb(Widget w, XtP client_data, XEvent *event)
{
   client_data = client_data; event=event;
   int rx,ry,wx,wy; 
   uint keys;
   Window rwin, cwin;
  
   Window win = XtWindow(w);
   XQueryPointer(g.display, win, &rwin, &cwin, &rx, &ry, &wx, &wy, &keys);

}
