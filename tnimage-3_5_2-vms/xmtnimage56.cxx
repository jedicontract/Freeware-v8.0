//--------------------------------------------------------------------------//
// xmtnimage56.cc                                                           //
// lists                                                                    //
// Latest revision: 10-18-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int in_listfinish = 0;
                                                                           

//--------------------------------------------------------------------//
// additemtolist                                                      //
// 'w' is the list widget to add the item to.                         //
// 'l' is a ptr to the listinfo struct                                //
// 'form' is the form widget the list is displayed on.                //
// 'showpos' can be TOP or BOTTOM. If TOP it displays first item.     //
// 'wantraise' should be 1 unless adding a lot of items.              //
//--------------------------------------------------------------------//
void additemtolist(Widget w, Widget form, char *s, int showpos, int wantraise)
{
  XmString xms;
                 // Last param = position; 0 means append to list
  if(strlen(s))
  {    XmListAddItemUnselected(w, xms = XmStringCreateSimple(s), 0);
       if(showpos==BOTTOM) XmListSetBottomPos(w, 0);  // Last item is last visible item.
       XmStringFree(xms);      
  }
  if(XtIsManaged(form) && wantraise) 
       XRaiseWindow(g.display, XtWindow(XtParent(form)));
}

//--------------------------------------------------------------------//
// select_list_item                                                   //
//--------------------------------------------------------------------//
void select_list_item(Widget w, int pos)
{ 
  static int opos = pos;
  if(pos>=0)
  {   XmListSelectPos(w, pos, False);  
      if(pos!=opos) opos=pos;
  }
}


//--------------------------------------------------------------------//
// set_list_top                                                       //
//--------------------------------------------------------------------//
void set_list_top(Widget w, int pos)
{ 
  if(pos>=0) XmListSetPos(w, pos+1);
}


//--------------------------------------------------------------------//
// update_list                                                        //
//--------------------------------------------------------------------//
void update_list(int listno)
{
  Widget w = g.openlist[listno]->form;
  g.openlist[listno]->f2(listno); 
  if(w && XtIsManaged(w)) XRaiseWindow(g.display, XtWindow(XtParent(w)));
}


//--------------------------------------------------------------------//
// listunmapcb                                                        //
//--------------------------------------------------------------------//
void listunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
   if(g.diagnose) printf("listunmap\n");
   if(!in_listfinish)
   {   g.block = max(0, g.block-1);
       listfinish(w, client_data, call_data);
   }
}


//--------------------------------------------------------------------//
// listfinish                                                         //
//--------------------------------------------------------------------//
void listfinish(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;
  int j,k;
  listinfo *l = (listinfo*)client_data;
  if(g.diagnose) printf("entering listfinish, block=%d lbusy=%d\n",g.block,l->busy);
  // if writing to list, signal desire to exit
  if(g.diagnose) printf("calling f6 in listfinish\n");
  l->f6(l);
  if(l->busy){ g.getout=1; return; } 
  g.getboxstate=0;
  g.getout=1;
  check_event();
  // Don't set g.getout here, it will screw up graphs
  if(g.diagnose) printf("in listfinish, block=%d\n",g.block);
  if(g.block) return;   // Waiting for input
  in_listfinish = 1;
  if(g.diagnose) printf("calling f3 in listfinish\n");
  l->f3(l);
  if(g.diagnose) printf("back from f3 in listfinish\n");
  if(l->wantsave)
  {   
     if(message("Save list contents?", YESNOQUESTION)==YES)
         savecb(l->form, l, NULL);
  }
  if(l->browser && XtIsManaged(l->browser)) XtUnmanageChild(l->browser);
  if(l->autoupdate) 
  {   for(k=0; k<g.openlistcount; k++)
      {   if(g.openlist[k] == l)
          {   for(j=k; j<g.openlistcount-1; j++)
                  g.openlist[j] = g.openlist[j+1];
              g.openlistcount--;
              break;
          } 
      }
  }
  l->finished = 1;
  if(g.diagnose) printf("calling f4 in listfinish\n");
  l->f4(l);
  if(g.diagnose) printf("back from f4 in listfinish\n");
  in_listfinish = 0;
  if(g.diagnose) printf("leaving listfinish\n");
}


//--------------------------------------------------------------------//
// delete_list - Dialog lists call delete_list automatically when     //
// dialog closes and should not call delete_list again. Regular lists //
// specify f4 = delete_list, and f4 is called in listfinish.          //
//--------------------------------------------------------------------//
void delete_list(listinfo *l)
{
  int k;
  if(g.diagnose)
  {   printf("entering delete_list\n");
      if(l==NULL) printf("l is null\n");
  }      
  if(l==NULL) return;
  for(k=0; k<l->count; k++) delete[] l->info[k];              
  if(l->allowedit && l->edittitle!=NULL) delete[] l->edittitle;
  for(k=0; k<l->wc; k++) if(l->widgetlist[k]) XtDestroyWidget(l->widgetlist[k]);
  l->browser  = 0;
  if(l->title) delete[] l->title;
  if(l->info) delete[] l->info;
  delete l;
  l = 0;
  if(g.diagnose) printf("leaving delete_list\n");
}


//--------------------------------------------------------------------//
// list                                                               //
// Returns the list Widget in 'listwidget' so caller can add to list. //
// 'listwidget' is needed for XmListAddItemUnselected.                //
// f1 = called after an item is selected in list                      //
// f2 = called after list is updated, every time image is changed.    //
// f3 = called before list is unmanaged after clicking Cancel button  //
// f4 = called after list is unmanaged after clicking Cancel button   //
// f5 = called after list is cleared                                  //
//--------------------------------------------------------------------//
Widget list(listinfo *l)
{
   int k, width=0, xsize, ysize, showtop=TOP;
   Widget list, form, label, savebutton, clearbutton=0;
   Widget okaybut, cancbut, helpbut;
   Arg args[100];
   XmString xms;
   Cardinal n;
   int LISTTOP = 8;
   if(g.diagnose) printf("entering list\n");
   l->wc = 0;
   if(l->itemstoshow<7) LISTTOP=16;
   xsize = 440;   // Minimum size
   ysize = l->itemstoshow*16 + 100;
   if(l->firstitem>1) showtop=BOTTOM;
   l->busy = 0;   // Ok to delete the list
   l->want_finish = 0;

   //------- Calculate width -----------------------------------------//

   for(k=0; k<l->count; k++) width=max(width,(int)strlen(l->info[k]));
   xsize = max(xsize,min(640,width*8));
   xsize = max(l->width, xsize);

   //------- Create a form dialog shell widget -----------------------//

   n=0;
   XtSetArg(args[n], XmNwidth, xsize); n++;
   XtSetArg(args[n], XmNheight, ysize); n++;
   if(l->transient)
   {  XtSetArg(args[n], XmNtransient, True); n++;}
   else
   {  XtSetArg(args[n], XmNtransient, False); n++;}
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
  
   ////  This statement prevents user from closing window with window
   ////  manager. Functions calling block() to wait for events should
   ////  capture "cancel" clicks and deallocate everything before
   ////  allowing WM to close listbox or it will cause a crash.
   
   XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++; // Block WM delete button
   XtSetArg(args[n], XmNkeyboardFocusPolicy, XmEXPLICIT); n++;
   XtSetArg(args[n], XmNnavigationType, XmEXCLUSIVE_TAB_GROUP); n++;
   XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple(l->title)); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   form = l->widgetlist[l->wc++] = XmCreateFormDialog(g.main_widget, l->title, args, n);
   l->browser = form;
   XmStringFree(xms);
   label = l->widgetlist[l->wc++] = addlabel(form, l->title, CENTER, 20, 1, 80, LISTTOP); 

   //--------Title and 'save' button----------------------------------//
   okaybut = l->widgetlist[l->wc++] = add_button(form, (char*)"Accept",   1, 1, 80);
   cancbut = l->widgetlist[l->wc++] = add_button(form, (char*)"Dismiss", 85, 6, 80);
   helpbut = l->widgetlist[l->wc++] = add_button(form, (char*)"Help",   170, 6, 80);

   XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)listokcb, (XtP)l);
   XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)listfinish, (XtP)l);
   XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[l->helptopic]);
   XtAddCallback(form, XmNunmapCallback, (XtCBP)listunmapcb, (XtP)l);

   savebutton = l->widgetlist[l->wc++] = XtVaCreateManagedWidget("Save",
         xmPushButtonWidgetClass,    form,
         XmNtopAttachment,           XmATTACH_NONE,
         XmNbottomAttachment,        XmATTACH_FORM,
         XmNleftAttachment,          XmATTACH_FORM,
         XmNrightAttachment,         XmATTACH_NONE,
         XmNwidth,                   80,
         XmNleftOffset,              255,
         XmNbottomOffset,            6,
         XmNdefaultButton,           False,
         NULL);

   if(l->clearbutton)
     clearbutton = l->widgetlist[l->wc++] = XtVaCreateManagedWidget("Clear",
         xmPushButtonWidgetClass,    form,
         XmNtopAttachment,           XmATTACH_NONE,
         XmNbottomAttachment,        XmATTACH_FORM,
         XmNleftAttachment,          XmATTACH_FORM,
         XmNrightAttachment,         XmATTACH_NONE,
         XmNwidth,                   80,
         XmNleftOffset,              340,
         XmNbottomOffset,            6,
         XmNdefaultButton,           False,
         NULL);

   if(g.want_colormaps)  
   {
       XtVaSetValues(savebutton, XmNbackground, g.main_bcolor,
               XmNforeground, g.main_fcolor,NULL); 
       if(l->clearbutton)
         XtVaSetValues(clearbutton, XmNbackground, g.main_bcolor,
               XmNforeground, g.main_fcolor,NULL); 
   }

   //------- Put scrolled list on the form ---------------------------//

   XtSetArg(args[n], XmNhighlightThickness, 1); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
   XtSetArg(args[n], XmNbottomOffset, 34); n++;
   XtSetArg(args[n], XmNfractionBase, 100); n++;   // Use percentages
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
   XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
   XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++; 
   XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++; 
   XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
#ifdef MOTIF2
   XtSetArg(args[n], XmNselectionMode, XmNORMAL_MODE); n++;  
   XtSetArg(args[n], XmNmatchBehavior, XmQUICK_NAVIGATE); n++;  
#endif
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
   XtSetArg(args[n], XmNtopWidget, label); n++;    
   XtSetArg(args[n], XmNvisibleItemCount, l->itemstoshow); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   list = l->widgetlist[l->wc++] = XmCreateScrolledList(form, (char*)"list", args, n);
   XtAddCallback(list, XmNsingleSelectionCallback, (XtCBP)listcb, (XtP*)l);
   l->widget = list;
   XtAddCallback(savebutton, XmNactivateCallback, (XtCBP)savecb, (XtP)l);
   if(l->clearbutton)
      XtAddCallback(clearbutton, XmNactivateCallback, (XtCBP)clearcb, (XtP)l);

  // Make Return key activate Ok button:

  for(k=0; k<l->wc; k++)
      XtAddEventHandler(l->widgetlist[k], KeyPressMask, FALSE, (XtEH)listentercb, (XtP)l);

   for(k=0;k<l->count;k++) additemtolist(list, form, l->info[k], showtop,1);
   XtManageChild(list);
   XtManageChild(form);
   XtRealizeWidget(list);
   XtRealizeWidget(form);

   if(*l->selection > 0) select_list_item(list, *l->selection+1);
   if(l->firstitem > 0) set_list_top(list, l->firstitem+1);

   if(l->autoupdate)
   {    l->form = form;  //// Setting form causes window to autoraise
        g.openlist[g.openlistcount] = l; 
        update_list(g.openlistcount);
        g.openlistcount = min(99, 1+g.openlistcount);
   } else l->form=0;
   if(g.diagnose) printf("leaving list\n");
   return form;
} 


//--------------------------------------------------------------------//
// message_window_open - opens a message window that stays up until a //
// task is finished.  Caller must periodically call check_event() and //  
// call message_window_close() to close the window.                   //
//--------------------------------------------------------------------//
Widget message_window_open(char *heading)
{
   int k,n;
   XmString xms=NULL;
   Arg args[100];
   Widget w,www=0;
   n = 0;
   XtSetArg(args[n], XmNtitle, "Information"); n++;
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNnoResize, False); n++;
   XtSetArg(args[n], XmNtransient, True); n++;
   xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNselectionLabelString, xms) ;n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   www = XmCreateWorkingDialog(g.main_widget, (char*)"Wait", args, n);
   w = XmMessageBoxGetChild(www,XmDIALOG_OK_BUTTON);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www,XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(w);
   XtManageChild(www);
   XmStringFree(xms);
   XFlush(g.display);
   
   ////  Flush events to make sure window gets drawn
   send_expose_event(XtWindow(www),Expose,0,0,100,100);
   send_expose_event(XtWindow(XtParent(www)),Expose,0,0,100,100);
   send_expose_event(XtWindow(XtParent(XtParent(www))),Expose,0,0,100,100);
   for(k=0;k<100;k++) check_event();
   XFlush(g.display);
   return www;
}


//--------------------------------------------------------------------------//
// listentercb                                                              //
//--------------------------------------------------------------------------//
void listentercb(Widget w, XtP client_data, XEvent *event)
{
   w=w; client_data=client_data;   
   int key = which_key_pressed(event);
   if(key==XK_Return) listokcb(w, client_data, NULL);  
   if(key==XK_Escape) listfinish(w, client_data, NULL);  
}


//--------------------------------------------------------------------//
// listokcb                                                           //
//--------------------------------------------------------------------//
void listokcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;
   listinfo *l = (listinfo*)client_data;
   l->f1(l);
}


//--------------------------------------------------------------------//
// message_window_close                                               //
//--------------------------------------------------------------------//
void message_window_close(Widget w)
{
   XtUnmanageChild(w);
   XtDestroyWidget(w);
   XFlush(g.display);
}


//--------------------------------------------------------------------//
// message_window_update                                              //
//--------------------------------------------------------------------//
void message_window_update(Widget www, char *string)
{
   int k;
   XmString xms=NULL;
   xms = XmStringCreateLtoR(string, XmFONTLIST_DEFAULT_TAG);
   XtVaSetValues(www, XmNmessageString, xms, NULL);
   XmStringFree(xms);
   for(k=0;k<10;k++) check_event();
   XFlush(g.display);
}


//--------------------------------------------------------------------//
// progress_bar_open                                                  //
//--------------------------------------------------------------------//
void progress_bar_open(Widget &www, Widget &scrollbar)
{
   int n;
   XmString xms=NULL;
   char heading[128] = "Processing";
   Arg args[100];
   Widget w;
   n = 0;
   XtSetArg(args[n], XmNtitle, heading); n++;
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNnoResize, True); n++;
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNtransient, True); n++;
   xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
   XtSetArg(args[n], XmNmessageString, xms); n++;
   XtSetArg(args[n], XmNselectionLabelString, xms) ;n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   www = XmCreateWorkingDialog(g.main_widget, (char*)"Wait", args, n);
   w = XmMessageBoxGetChild(www, XmDIALOG_OK_BUTTON);
   XtUnmanageChild(w);
   w = XmMessageBoxGetChild(www, XmDIALOG_HELP_BUTTON);
   XtUnmanageChild(w);
   w = XtNameToWidget(www, "Cancel");
   XtAddCallback(w, XmNactivateCallback, (XtCBP)minimal_cancelcb, (XtP)NULL);
   XtAddCallback(www, XmNunmapCallback, (XtCBP)progress_bar_unmapcb, (XtP)NULL);

   XFlush(g.display);

   n = 0;
   XtSetArg(args[n], XmNshowArrows, False); n++;
   XtSetArg(args[n], XmNminimum, 0); n++;
   XtSetArg(args[n], XmNmaximum, 100); n++;
   XtSetArg(args[n], XmNvalue, 0); n++;
   XtSetArg(args[n], XmNheight, 20); n++;
   XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
   XtSetArg(args[n], XmNeditable, False); n++;
   XtSetArg(args[n], XmNsliderSize, 1); n++;
   scrollbar = XmCreateScrollBar(www, (char*)"ProgressBar", args, n);
   XtManageChild(scrollbar);
   XtManageChild(www);

   XtAddEventHandler(XtParent(www), 
        FocusChangeMask | VisibilityChangeMask, 
        False, (XtEH)raisecb, (XtP)NULL);

   XmStringFree(xms);
   XFlush(g.display);  
   g.waiting++;
}



//--------------------------------------------------------------------------//
// progress_bar_unmapcb - called when user closes a message box using window//
// manager instead of the cancel button.                                    //
//--------------------------------------------------------------------------//
void progress_bar_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{  
  w=w; call_data=call_data;client_data=client_data;
  g.getout = 1;
}


//--------------------------------------------------------------------//
// progress_bar_update                                                //
//--------------------------------------------------------------------//
void progress_bar_update(Widget scrollbar, int percent)
{
   static int opercent = 0;
   XmString xms=NULL;
   char heading[128];
   sprintf(heading,"%d%c",percent,'%');
   percent = max(1, min(percent,100));
   if(percent<opercent) opercent = 0;
   xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
   XtVaSetValues(scrollbar, XmNsliderSize, percent, NULL);
   send_expose_event(XtWindow(scrollbar),Expose,0,0,100,100);
  
   //// This doesn't work in lesstif for some reason

#ifndef LESSTIF_VERSION
   if(abs(percent - opercent)>=5)
   {
       XtVaSetValues(XtParent(scrollbar), XmNmessageString, xms, NULL);
       opercent = percent;
       check_event();
   }
#endif
   XmStringFree(xms);
   XFlush(g.display);
}


//--------------------------------------------------------------------//
// progress_bar_close                                                 //
//--------------------------------------------------------------------//
void progress_bar_close(Widget w)
{
   XtUnmanageChild(w);
   XtDestroyWidget(w);
   XFlush(g.display);
   g.waiting = max(0, g.waiting-1);
}


//--------------------------------------------------------------------//
// listcb - callback for list box.                                    //
// Item text is placed in 'string', l->selection points to answer no. //
//--------------------------------------------------------------------//
void listcb(Widget w, XtP client_data, XmACB *call_data)
{
   int k;
   if(g.diagnose) printf("entering listcb\n");
   listinfo *l = (listinfo *)client_data;
   char *string;
   XmString xms;
   XmListCallbackStruct *ptr;
   ptr = (XmListCallbackStruct*) call_data;
   XmStringGetLtoR(ptr->item, XmFONTLIST_DEFAULT_TAG, &string);
 
   if(l->selection!=NULL) *l->selection = ptr->item_position;
 
   *l->selection = *l->selection - 1;
   k =  *l->selection;
   if(l->allowedit && k >= l->editrow)
   {     message(l->edittitle, l->info[k] + l->editcol, PROMPT, l->maxstringsize, l->helptopic);
         xms = XmStringCreateSimple(l->info[k]);
         XmListReplaceItemsPosUnselected(w, &xms, 1, k+1);
         XmStringFree(xms);
   }
   if(g.diagnose) printf("calling f1\n");
   if(l->wantfunction) l->f1(l);
   if(g.diagnose) printf("leaving listcb\n");
}


//--------------------------------------------------------------------//
// savecb - callback for saving list data                             //
//--------------------------------------------------------------------//
void savecb(Widget w, XtP client_data, XmACB *call_data)
{
   int k, status=YES;
   static PromptStruct ps;
   w=w; call_data=call_data; // keep compiler quiet
   static char filename[FILENAMELENGTH]="";
   static listinfo *l2;
   listinfo *l = (listinfo *)client_data;
   int count = l->count;
   l2 = new listinfo;

   //// Maka a copy since no way to stop list from being deleted
   l2->title = new char[256];
   strcpy(l2->title, l->title);
   l2->info  = new char*[10000]; // Max. of 10000 densitometric measurements
   l2->count = l->count;
   l2->highest_erased = l->highest_erased;
   for(k=0; k<count; k++) 
   {   l2->info[k] = new char[1024];
       strcpy(l2->info[k], l->info[k]);
   }
   if(count==0){ message("No data to save",ERROR); return; }
   getfilename(filename, NULL);
   g.getout = 0;
   if(status != YES) return;
   if(filename[0]==0 || filename==NULL) return;
   ps.filename = filename;
   ps.f1 = savecb_part2;
   ps.f2 = null;
   ps.ptr = l2;
   ps.n = count;
   check_overwrite_file(&ps);
}


//--------------------------------------------------------------------//
// savecb_part2                                                       //
//--------------------------------------------------------------------//
void savecb_part2(PromptStruct *ps)
{
   char temp[FILENAMELENGTH];
   char *filename = ps->filename;
   listinfo *l = (listinfo *)ps->ptr;
   int k;
   int count = ps->n;

   FILE *fp;
   if ((fp=fopen(filename,"w")) == NULL)
        error_message(filename, errno);
   else
   {    fprintf(fp,"%s\n",l->title);
        for(k=l->highest_erased; k<count; k++)
            fprintf(fp,"%s\n",l->info[k]);
        sprintf(temp,"%s saved in %s",l->title,filename);
        message(temp);
        fclose(fp);
   }
   for(k=0;k<count; k++) delete[] l->info[k];
   delete[] l->info;
   delete[] l->title;
}


//--------------------------------------------------------------------//
// clearcb - callback for erasing list data                           //
//--------------------------------------------------------------------//
void clearcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data;          // keep compiler quiet
   listinfo *l = (listinfo *)client_data;
   XmListDeleteAllItems(l->widget);  // Caller must still free listinfo data
   l->highest_erased = l->count;
   l->f5(l);
}


//--------------------------------------------------------------------//
// minimal_cancelcb                                                   //
//--------------------------------------------------------------------//
void minimal_cancelcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data; client_data=client_data; // Keep compiler quiet
   if(g.diagnose) printf("minimal_cancelcb\n");
   g.getout=1;
}

