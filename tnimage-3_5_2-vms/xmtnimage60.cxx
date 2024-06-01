//--------------------------------------------------------------------------//
// xmtnimage60.cc                                                           //
// Latest revision: 11-16-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;
extern int **xbias, **ybias;
extern int vector_map_created;

//--------------------------------------------------------------------//
// check_overwrite_file                                               //
//--------------------------------------------------------------------//
void check_overwrite_file(PromptStruct *ps)
{
  char temp[2048];
  if(g.want_messages && !access(ps->filename, F_OK))
  {   sprintf(temp,"Overwrite existing file\n%s (y/n)?", ps->filename);
      prompt(temp, ps, 0);
  }else prompt_yes(0, ps, NULL);
}

//--------------------------------------------------------------------//
// prompt                                                             //
//--------------------------------------------------------------------//
void prompt(char *heading, PromptStruct *ps, int helptopic)
{    
  Widget www, helpwidget;
  Arg args[100];
  XmString xms;
  int n=0;
  g.state = MESSAGE;
  xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
  n = 0;
  XtSetArg(args[n], XmNmessageString, xms); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  XtSetArg(args[n], XmNtitle, (char*)"Question"); n++;
  www = XmCreateQuestionDialog(g.main_widget, (char*)"Question", args, n);
  helpwidget = XtNameToWidget(www, (char*)"Help");
  XtAddCallback(www, XmNokCallback, (XtCBP)prompt_yes, (XtP)ps);
  XtAddCallback(www, XmNcancelCallback, (XtCBP)prompt_no, (XtP)ps);
  XtAddCallback(helpwidget, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(www, XmNunmapCallback, (XtCBP)prompt_unmap, (XtP)NULL);
  XtManageChild(www);
  XmStringFree(xms);
}


//--------------------------------------------------------------------//
// prompt_yes                                                         //
//--------------------------------------------------------------------//
void prompt_yes(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;
  PromptStruct *ps = (PromptStruct*)client_data;
  ps->f1(ps);
  ps->f2(ps);
  g.state = NORMAL;
}


//--------------------------------------------------------------------//
// prompt_no                                                          //
//--------------------------------------------------------------------//
void prompt_no(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data; client_data=client_data;
  PromptStruct *ps = (PromptStruct*)client_data;
  message("Aborted");
  ps->f2(ps);
  g.state = NORMAL;
}


//--------------------------------------------------------------------//
// prompt_unmap                                                       //
//--------------------------------------------------------------------//
void prompt_unmap(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;client_data=client_data; 
}


//--------------------------------------------------------------------//
// click_prompt                                                       //
//--------------------------------------------------------------------//
void click_prompt(char *heading, PromptStruct *ps, int helptopic)
{    
  Widget www, helpwidget;
  Arg args[100];
  XmString xms;
  int n=0;
  g.state = MESSAGE;
  xms = XmStringCreateLtoR(heading, XmFONTLIST_DEFAULT_TAG);
  n = 0;
  XtSetArg(args[n], XmNmessageString, xms); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  XtSetArg(args[n], XmNtitle, (char*)"Message"); n++;
  www = XmCreateMessageDialog(g.main_widget, (char*)"Message", args, n);
  helpwidget = XtNameToWidget(www, "Help");
  XtAddCallback(www, XmNokCallback, (XtCBP)prompt_yes, (XtP)ps);
  XtAddCallback(www, XmNcancelCallback, (XtCBP)prompt_no, (XtP)ps);
  XtAddCallback(helpwidget, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
  XtAddCallback(www, XmNunmapCallback, (XtCBP)prompt_unmap, (XtP)NULL);
  XtManageChild(www);
  XmStringFree(xms);
}


//--------------------------------------------------------------------------// 
//  getstring                                                               //
//--------------------------------------------------------------------------// 
int getstring(const char *heading, char *returnstring, int maxstringsize, int helptopic)
{
   return message(heading, returnstring, PROMPT, maxstringsize, helptopic);
}

//--------------------------------------------------------------------------// 
//  getstring                                                               //
//--------------------------------------------------------------------------// 
void getstring(const char *heading, PromptStruct *ps)
{
  Widget textwidget=0, www, helpwidget;
  Arg args[100];
  XmString xms, xms3;
  int n=0;
  g.state = MESSAGE;
  xms = XmStringCreateLtoR((char*)heading, XmFONTLIST_DEFAULT_TAG);
  n = 0;
  XtSetArg(args[n], XmNmessageString, xms); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  XtSetArg(args[n], XmNtitle, "Information needed"); n++;
  XtSetArg(args[n], XmNtextString, xms3=XmStringCreateSimple(ps->text));n++;
  www = XmCreatePromptDialog(g.main_widget, (char*)"Prompt", args, n);
  textwidget = XtNameToWidget(www, "Text");
  if(g.want_colormaps)
       XtVaSetValues(textwidget, XmNbackground, g.main_bcolor, XmNforeground, g.main_fcolor,NULL);
  helpwidget = XtNameToWidget(www, "Help");
  XtAddCallback(www, XmNokCallback, (XtCBP)getstring_yes, (XtP)ps);
  XtAddCallback(www, XmNcancelCallback, (XtCBP)getstring_no, (XtP)ps);
  XtAddCallback(helpwidget, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[ps->helptopic]);
  XtAddCallback(www, XmNunmapCallback, (XtCBP)getstring_unmap, (XtP)NULL);
  ps->widget[0] = textwidget;
  XmStringFree(xms3); 
  XtManageChild(www);
}


//--------------------------------------------------------------------------// 
//  getfilenameps                                                           //
//--------------------------------------------------------------------------// 
void getfilenameps(PromptStruct *ps)
{
  g.getout = 0;
  g.escape = 0;
  getfilename(ps->text, NULL);
  if(!g.escape) ps->f1(ps);
  ps->f2(ps);
}


//--------------------------------------------------------------------//
// getstring_yes                                                      //
//--------------------------------------------------------------------//
void getstring_yes(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;
  PromptStruct *ps = (PromptStruct*)client_data;
  char *s=NULL;
  if(ps->widget[0]) s = XmTextGetString(ps->widget[0]);   // This allocates memory
  if(s) 
  {    s[min((int)strlen(s), ps->maxsize)] = 0;
       strcpy(ps->text, s);
       XtFree(s);
  }       
  ps->f1(ps);
  ps->f2(ps);
  g.state = NORMAL;
  XtDestroyWidget(XtParent(w));
}


//--------------------------------------------------------------------//
// getstring_no                                                       //
//--------------------------------------------------------------------//
void getstring_no(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;client_data=client_data; 
  PromptStruct *ps = (PromptStruct*)client_data;
  message("Aborted");
  ps->f2(ps);
  g.state = NORMAL;
}


//--------------------------------------------------------------------//
// getstring_unmap                                                    //
//--------------------------------------------------------------------//
void getstring_unmap(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; call_data=call_data;client_data=client_data; 
  // figure out what to do here
}


//--------------------------------------------------------------------//
// X selection                                                        //
//--------------------------------------------------------------------//
void getxselection(void)
{
  XtP client_data=0;
  Time timestamp=0;
  XtGetSelectionValue(g.main_widget, XA_PRIMARY, XA_STRING, 
      (XtSelectionCallbackProc) selectioncb, client_data,
      timestamp);
}

//--------------------------------------------------------------------//
// print_selectioncb                                                  //
// paste text with middle button                                      //
//--------------------------------------------------------------------//
void selectioncb(Widget widget, XtP client_data, Atom *selection,
    Atom *type, XtP value, ulong *value_length, int *format)
{
  char *string, *ostring, *crpos;
  widget=widget; client_data=client_data; selection=selection; format=format;
  type=type; value=value; value_length = value_length;
  int len=0,linelen,h,x,y,direction,ascent,descent;   
  XCharStruct overall;
  x = g.mouse_x;
  y = g.mouse_y;
  if(value!=NULL) len = strlen((char*)value);
  if(len<=0) return;
  string = new char[len+1];
  ostring = string;
  strcpy(string, (char*)value);

  crpos = string;
  while(crpos)
  { 
      crpos = strchr(string, '\n');
      if(crpos!=NULL) linelen = crpos - string;
      else linelen = strlen(string);
      string[linelen] =0;
      XTextExtents(g.image_font_struct,string,len,&direction,&ascent,&descent,&overall);
      h = g.image_font_struct->ascent + g.image_font_struct->descent;
      remove_terminal_cr(string);
      print(string,x,y,g.fcolor,g.bcolor,&g.image_gc,0,0,0,0,0); 
      y += h;
      string += linelen+1;
      if(!crpos) break;
  }
  delete[] ostring;
}    



