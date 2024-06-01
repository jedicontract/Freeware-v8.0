//--------------------------------------------------------------------------//
// xmtnimage49.cc                                                           //
// filename callbacks                                                       //
// Latest revision: 09-24-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
char lestif_filename[FILENAMELENGTH] = "nothing";
int toppos=1;
int in_fscancelcb = 0;


//--------------------------------------------------------------------//
// filenamecb                                                         //
// Calling routine must maintain a dynamic list of Widgets, then      //
//   destroy them when finished.                                      //
//--------------------------------------------------------------------//
void filenamecb(Widget widget, XtP client_data, XmACB *call_data)
{
   call_data=call_data;  // Keep compiler quiet
   Widget f,w,w3,dirmaskwidget,bb;

   XmString title, dir_mask, pathxms=NULL;
   int n, itemcount;
   Arg args[100];
   clickboxinfo *c = (clickboxinfo *)client_data;
   int wc = c->wc;
   Widget flist;

   int rx,ry,wx,wy; 
   uint keys;
   Window rwin, cwin; 

   ////  Find current position of main window & image number
   XQueryPointer(g.display,g.main_window,&rwin,&cwin,&rx,&ry,&wx,&wy,&keys);
   g.main_xpos = rx-wx;
   g.main_ypos = ry-wy;

   n = 0;
   XtSetArg(args[n], XmNdialogStyle, XmDIALOG_MODELESS); n++;
   XtSetArg(args[n], XmNdefaultPosition, False); n++;
   XtSetArg(args[n], XmNx, min(g.xres-380, g.main_xpos + 50)); n++;
   XtSetArg(args[n], XmNy, g.main_ypos - 50); n++;
   XtSetArg(args[n], XmNtitle, "File Selection"); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   w = bb = c->w[wc++] = XmCreateBulletinBoardDialog(widget,(char*)"Select File",args,n);
   c->form = bb;
   title = XmStringLtoRCreate((char*)"Filter",XmSTRING_DEFAULT_CHARSET);   
   if(c->dirmask)
        dir_mask = XmStringLtoRCreate(c->dirmask,XmSTRING_DEFAULT_CHARSET); 
   else
        dir_mask = XmStringLtoRCreate((char*)"*",XmSTRING_DEFAULT_CHARSET); 

   n = 0;
   XtSetArg(args[n], XmNhighlightThickness, 1); n++;  
   XtSetArg(args[n], XmNdirMask, dir_mask); n++;  
   XtSetArg(args[n], XmNresizable, True); n++;  
   XtSetArg(args[n], XmNfilterLabelString, title); n++; 
   if(c->path != NULL)
   {   XtSetArg(args[n], XmNdirectory, pathxms = XmStringLtoRCreate(c->path,XmSTRING_DEFAULT_CHARSET)); n++;
   }
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   f = c->w[wc++] = XmCreateFileSelectionBox(w, (char*)"FileSelector", args, n);

   Widget child;
   child = c->listwidget = XmFileSelectionBoxGetChild(f, XmDIALOG_LIST);
   if(c->type==MULTIPLE)
       XtVaSetValues(child, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
    
   XtVaGetValues(child, XmNitemCount, &itemcount, NULL);
   toppos = min(toppos, itemcount);
  
   XtVaSetValues(child, XmNtopItemPosition, toppos, NULL);

   ////   The first pushbutton is automatically placed in the wrong
   ////   position, so an unused, unmanaged pushbutton must be added
   ////   to trick Motif.
   
   n = 0;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   c->w[wc++] = XmCreatePushButton(f, (char*)"crudola", args, n);

   n = 0;
   XtSetArg(args[n], XmNchildPlacement, XmPLACE_BELOW_SELECTION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   w3 = c->w[wc++] = XmCreatePushButton(f, (char*)"Chdir", args, n);
   XtManageChild(w3);

   ////  Set c->widget[0] to the fileselectionbox widget.
   ////  c->widget[1] is the 'filename' push button in dialog box.

   c->widget[0] = w; 
   XtAddCallback(f, XmNcancelCallback, (XtCBP)fscancelcb, (XtP)c);
   XtAddCallback(f, XmNokCallback, (XtCBP)fsokcb, (XtP)c); 
   XtAddCallback(bb, XmNunmapCallback, (XtCBP)fsunmapcb, (XtP)c);

   flist =  XmFileSelectionBoxGetChild(f, XmDIALOG_LIST);
   XtAddCallback(flist, XmNsingleSelectionCallback, (XtCBP)fsscb, (XtP)flist);
   XtAddCallback(flist, XmNmultipleSelectionCallback, (XtCBP)fsscb, (XtP)flist);

   XtAddCallback(w3, XmNactivateCallback, (XtCBP)fscdcb, (XtP)f);
   dirmaskwidget = XmFileSelectionBoxGetChild(f, XmDIALOG_FILTER_TEXT);
   XtAddCallback(dirmaskwidget, XmNvalueChangedCallback, (XtCBP)fsdirmaskcb, (XtP)c); 

   if(c->path != NULL) XmStringFree(pathxms);
   XmStringFree(title);
   XmStringFree(dir_mask);
   XtManageChild(f);
   XtManageChild(w);

   //// It is necessary to manually set the colors in each subwindow.
           
   w = XmFileSelectionBoxGetChild(f, XmDIALOG_LIST);
   XtVaSetValues(w, 
#ifdef SOLARIS 
            XmNwidth, 400,
#endif
            XmNscrollBarDisplayPolicy, XmSTATIC,  // Essential for Solaris
            NULL);

   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {   
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_APPLY_BUTTON);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_CANCEL_BUTTON);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_DEFAULT_BUTTON);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_DIR_LIST);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_DIR_LIST_LABEL);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_FILTER_LABEL);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_FILTER_TEXT);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_HELP_BUTTON);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_LIST);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_LIST_LABEL);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_OK_BUTTON);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_SELECTION_LABEL);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_SEPARATOR);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_TEXT);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
       w = XmFileSelectionBoxGetChild(f, XmDIALOG_WORK_AREA);
       XtVaSetValues(w, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
   }
   c->wc = wc;
   //// Don't delete widgets here, delete at end of getfilename(s).
}


//--------------------------------------------------------------------//
//  fsscb                                                             //
//--------------------------------------------------------------------//
void fsscb(Widget w, XtP client_data, XmLCB *call_data)
{ 
   w=w; client_data=client_data;
   XmListCallbackStruct *ptr = call_data;
   int count = ptr->selected_item_count;
   char **tempstring=NULL;
   tempstring = new char *[count+1];
   XmStringGetLtoR(ptr->item, XmFONTLIST_DEFAULT_TAG, tempstring);
   if(strlen(tempstring[0])) strcpy(lestif_filename, tempstring[0]);
   delete[] tempstring;
}


//--------------------------------------------------------------------//
// fsokcb - callback for "ok" button in file selection box.           //
// Caller passes a clickboxinfo struct in client_data.                //
// Widget2 must be set to a pushbutton Widget or to NULL.             // 
// This is where memory is allocated for multifilename selector.      //
// The list array must be freed with delete[].                        //
// The filename strings must be freed with XtFree().                  //
//--------------------------------------------------------------------//
void fsokcb(Widget w, XtP client_data, XmFSBCB *call_data)
{
   call_data=call_data;
   Widget selectiontext;  // XmText for user to type in a filename 
   XmStringTable xmstrings;
   int k;
   clickboxinfo *c = (clickboxinfo *)client_data;
   char *filename; 
   char *fname;
   Widget button;

   //// Should never happen - clickbox already deleted
   if(c->done)
   {    message("Error in fsokcb", BUG); 
        fscancelcb(w, client_data, call_data);
        return;
   }

   filename = new char[FILENAMELENGTH];
   filename[0]=0;
   
   if(c->type == MULTIPLE)
   {   
       c->count=0;
       XtVaGetValues(c->listwidget, XmNselectedItemCount, &c->count, NULL); 

       ////  This returns the XmList items themselves, not a copy. The 
       ////  XmStringTable is automatically created by FileSelectionBox.
       ////  The application must not free the XmStrings (see OSF p. 1-859).
       
       XtVaGetValues(c->listwidget, XmNselectedItems, &xmstrings, NULL);

       c->list = new char*[c->count+1];
       for(k=0;k<c->count;k++)
       {
            ////  This allocates memory for items in list, use XtFree to free it.
            XmStringGetLtoR(xmstrings[k], XmFONTLIST_DEFAULT_TAG, &c->list[k]);
       }
       if(c->count>0) 
       {    strcpy(c->title, c->list[0]);
            strcpy(filename, c->list[0]);
       }else 
       {       //// A single click selects the file, then a double click deselects
               //// it and then comes here with nothing in the list. So retaliate 
               //// by copying selected filename (set in fsscb) out of a global.
               //// Also, with lestif, the fileselector doesn't recognize the 'Enter" key.
            selectiontext = XmFileSelectionBoxGetChild(w, XmDIALOG_TEXT);
               //// First check if they typed something in the text widget.
            XtVaGetValues(selectiontext, XmNvalue, &fname, NULL);
            if(strlen(fname)) strcpy(filename, fname);
               //// If it got nothing or a directory, substitute previous selection.
               //// Lestif might have unselected it so copy from the global.
            if(!strlen(filename) || filename[(int)strlen(filename)-1]=='/') 
                  strcpy(filename, lestif_filename);
            c->list[0] = XtMalloc(FILENAMELENGTH);  // Free this with XtFree()
            remove_trailing_space(filename);
            if(strlen(filename)) strcpy(c->title, filename);
            if(strlen(filename)) strcpy(c->list[0], filename);
       }
   }

   ////  For single file mode, Motif puts filename in an editable text widget. 
   if(c->type == SINGLE)
   {    selectiontext = XmFileSelectionBoxGetChild(w, XmDIALOG_TEXT);
        XtVaGetValues(selectiontext, XmNvalue, &fname, NULL);
        if(strlen(fname)) strcpy(filename, fname);
        //// In case user clicks Ok without selecting a file
        if(!strlen(filename) || filename[(int)strlen(filename)-1]=='/') 
             strcpy(filename, lestif_filename);
        remove_trailing_space(filename);
        if(strlen(filename)) strcpy(c->title, filename);
        c->count=1;
   }
   XtVaGetValues(c->listwidget, XmNtopItemPosition, &toppos, NULL);
   XFlush(g.display);

   ////  Put the new filename on the pushbutton. 
   ////  The pushbutton's XmNresizable resource must be False, otherwise
   ////  Motif will use this as an excuse to resize the entire dialog box.
   ////  Unmanage c->calling widget, which is the bulletin board - not w,
   ////  which is the filenamewidget on top.

   ////  c->widget[1] is the 'filename' pushbutton in dialogbox.
   ////  If calling from somewhere else, this should be set to NULL.
     
   ////  Focus goes to OK button when file is selected, so pressing Enter
   ////  twice will load the image.
   
   if(c->form != NULL) XmProcessTraversal(c->form, XmTRAVERSE_NEXT_TAB_GROUP); 

   //// Put the name in the pushbutton widget if it exists
   button = c->widget[1];
   if(button && strlen(c->title) && XtIsManaged(button)) 
        set_widget_label(button, basefilename(filename));

   dialoginfo *a = (dialoginfo*)c->ptr[0];
   if(a && strlen(c->path)) strcpy(a->path, c->path);

   c->done = 1;                            // Set flag indicating done
   delete[] filename;
   fscancelcb(w, client_data, call_data);
   g.escape = 0;
}


//--------------------------------------------------------------------//
// fscancelcb - callback for "cancel" button in file selection box.   //
//--------------------------------------------------------------------//
void fscancelcb(Widget w, XtP client_data, XmFSBCB *call_data)
{
   int k;
   w=w; call_data=call_data;  // Keep compiler quiet
   in_fscancelcb = 1;
   clickboxinfo *c = (clickboxinfo *)client_data;
   if(c->widget[0] && XtIsManaged(c->widget[0])) XtUnmanageChild(c->widget[0]);
   for(k=0;k<c->wc;k++) if(!c->done && c->w[k]) XtDestroyWidget(c->w[k]);
   in_fscancelcb = 0;
   c->form = 0;
   c->done = 1;                            // Set flag indicating done
   g.escape = 1;
}


//--------------------------------------------------------------------//
// fsunmapcb - callback in case user deletes file selection box window//
//--------------------------------------------------------------------//
void fsunmapcb(Widget w, XtP client_data, XmFSBCB *call_data)
{
  g.block = 0; g.waiting=0;g.busy=0;
  if(!in_fscancelcb) fscancelcb(w, client_data, call_data);
}


//--------------------------------------------------------------------//
// fscdcb - callback for "chdir" button in file selection box.        //
//--------------------------------------------------------------------//
void fscdcb(Widget w, XtP client_data, XmFSBCB *call_data)
{
   w=w; call_data=call_data;  // Keep compiler quiet
   Widget f = (Widget)client_data;
   char *dir;
   int len;
   dir = new char[1024];
   XmString xms = XmStringCreateLtoR(dir, XmSTRING_DEFAULT_CHARSET);
   XtVaGetValues(f, XmNdirectory,  &xms, NULL);       
   XmStringGetLtoR(xms, XmFONTLIST_DEFAULT_TAG, &dir);
   chdir(dir);
   len = strlen(dir)-1;            // eliminate trailing /
   if(dir[len]=='/') dir[len]=0;
   strcpy(g.currentdir,dir);
   XmStringFree(xms);
   delete[] dir;
}


//--------------------------------------------------------------------//
// fsdirmaskcb - callback for changing dir mask in file selection box.//
//--------------------------------------------------------------------//
void fsdirmaskcb(Widget w, XtP client_data, XmFSBCB *call_data)
{
   w=w; call_data=call_data;  // Keep compiler quiet
   clickboxinfo *c = (clickboxinfo *)client_data;  
   XmString xmstring;
   XtVaGetValues(XtParent(w), XmNdirMask,  &xmstring, NULL);       
   if(c->dirmask != NULL)
        XmStringGetLtoR(xmstring, XmFONTLIST_DEFAULT_TAG, &c->dirmask);

   XtVaGetValues(XtParent(w), XmNdirectory,  &xmstring, NULL);       
   if(c->path != NULL)
        XmStringGetLtoR(xmstring, XmFONTLIST_DEFAULT_TAG, &c->path);
   XmStringFree(xmstring);
} 


//--------------------------------------------------------------------//
// getfilename                                                        //
//--------------------------------------------------------------------//
char *getfilename(char *oldtitle, char *path)
{  
   return getfilename(oldtitle, path, NULL);
}
char *getfilename(char *oldtitle, char *path, Widget widget)
{ 
   static char dirmask[FILENAMELENGTH]="*";
   int ostate = g.state;
   g.state = MESSAGE;
   int k;
   clickboxinfo c;
   c.title = oldtitle;
   c.helptopic = 0;
   c.done      = 0;
   c.wc        = 0;
   c.w         = new Widget[20];
   c.f1 = null;
   c.f2 = null;
   c.f3 = null;
   c.f4 = null;
   c.f5 = null;
   c.f6 = null;
   c.f7 = null;
   c.f8 = null;
   c.wc = 0;
   c.form = NULL;
   c.path = path;
   c.type = SINGLE;
   c.list = (char**)NULL;
   c.dirmask = new char[FILENAMELENGTH];
   strcpy(c.dirmask, dirmask);
   for(k=0; k<20; k++){ c.widget[k]=0; c.ptr[k]=0; }
   c.widget[1] = widget;

   ////  This changes c.wc
   filenamecb(g.main_widget, &c, (XmACB*)NULL);  
   g.waiting++;
   g.block++;
   while(!c.done) XtAppProcessEvent (g.app, XtIMAll);
   g.block = max(0, g.block-1);
   g.waiting = max(0, g.waiting-1);
   for(k=0;k<c.wc;k++) XtDestroyWidget(c.w[k]);

   XFlush(g.display);
   strcpy(dirmask, c.dirmask);
   delete[] c.w;
   delete[] c.dirmask;
   c.wc = 0;
   g.state = ostate;
   if(strlen(c.title)>0) return c.title;
   else return oldtitle;
}


//--------------------------------------------------------------------//
// getfilenames                                                       //
//--------------------------------------------------------------------//
void getfilenames(clickboxinfo *c)
{  
   static int ingfn=0;
   if(c==NULL) return;
   int k;
   c->done      = 0;
   if(ingfn){ c->done=1; return; }
   c->wc        = 0;
   ingfn=1;
   static char dirmask[FILENAMELENGTH]="*";
   c->type      = MULTIPLE;
   c->dirmask   = new char[FILENAMELENGTH];

   ////  Delete any previous list in case user clicks twice
   ////  Calling routine must still delete final list when finished.
   if(c->list!=NULL && c->count)
   {    for(k=0; k<c->count; k++) XtFree(c->list[k]);
        delete[] c->list;
   }
   c->count     = 0;
   strcpy(c->dirmask, dirmask);
   ////  This changes c->wc, allocates and fills c->list, and sets c->count.
   ////  Calling routine must delete c->list.
   filenamecb(g.main_widget, c, (XmACB*)NULL);   
   strcpy(dirmask, c->dirmask);
   ingfn=0;
}

