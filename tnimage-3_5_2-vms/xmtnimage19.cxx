//--------------------------------------------------------------------------//
// xmtnimage19.cc                                                           //
// Handling of macros                                                       //
// Latest revision: 10-12-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
extern char DECIMAL_POINT;
extern int inmath;
const int MAXARGS=50;                      // max of 50 arguments
char *gtextptr;

//// Globals for edit
clickboxinfo *ced;
int in_editdonecb = 0;
enum{ MACRO_IF, MACRO_WHILE, MACRO_UNTIL, MACRO_MATH };


//--------------------------------------------------------------------------//
// get_next_statement - extract to next ';' or '}', put in line & parse     //
// Caller must send a char* for text and not delete it, since the pointer   //
// gets reassigned. Arg can be null                                         //
//--------------------------------------------------------------------------//
void get_next_statement(char *&text, char *line, char **arg, int &argno)
{    
   static char line2[1024];
   static char *arg2[MAXARGS];
   static int argno2;
   static int hit=0;
   char *textptr2;
   int k;
   if(!hit){ hit=1; for(k=0;k<MAXARGS;k++) arg2[k]=new char[1024]; } //leave allocated
   if(arg) arg[0][0] = 0;
   find_next_statement(text, line, arg, argno);
   if(arg && !strcmp(arg[0], "if"))
   {    line2[0] = 0;
        arg2[0][0] = 0;
        textptr2 = text;
        find_next_statement(textptr2, line2, arg2, argno2);
        parsecommand(line2, FORMULA, arg2, argno2, 80, MAXARGS);
        arg2[0][4]=0;  // else statement
        if(!strcmp(arg2[0], "else")){ text=textptr2; strcat(line, line2); }
   }
}
void find_next_statement(char *&text, char *line, char **arg, int &argno)
{
  int start=0, k, brace=0, paren=0, quote=0, len=0, textlen = strlen(text);

  //// Get rid of comments
  start += first_non_comment(text+start);

  //// Find end of statement
  for(k=start; k<textlen; k++)
  {
       if(text[k]=='{') brace++;
       //// if{ ... }
       if(text[k]=='}'){ brace--; if(!brace && !paren && !quote){ len=k+1; break; }}
       if(text[k]=='(') paren++;
       if(text[k]==')') paren--;       
       if(text[k]=='\"') quote = 1-quote;
       //// stmt ;  but not for(stmt; stmt; stmt)
       if(!brace && !paren && !quote && text[k]==';'){ len=k+1; break; }
       if(text[k+1]==0){ len=k+1; break; }
  }
  if(len && len-start>0) 
  {    strncpy(line, text+start, len-start);
       line[len-start] = 0;
       text += len;
       if(text[0] == '\n') text++;
  }else
  {    line[0]=0; 
       argno = 0;
       return;
  }
  remove_trailing_space(line);
  if(arg)
  {    parsecommand(line, FORMULA, arg, argno, 80, MAXARGS);
       if(argno < MAXARGS-1) arg[argno+1][0] = 0;
  }
}


//--------------------------------------------------------------------------//
//  get_expression - extract a single expression bounded by delimiters      //
//  delimiter1 can be 0 to match any character, otherwise strips delimiters //
//--------------------------------------------------------------------------//
void get_expression(char *&text, char *line, int delimiter1, int delimiter2)
{
  int k=0, d1=0, textlen=strlen(text), start=0, end;
  end = textlen;
  start += first_non_comment(text+start);
  if(delimiter1)  // 0 = match any character
  {   for(k=start; k<textlen; k++) if(text[k]==delimiter1) break; else start++;
  }else d1=1;
  for(k=start; k<textlen; k++)
  {    if(text[k]==delimiter1) d1++;  // nested delimiter1's
       if(text[k]==delimiter2){ d1--; if(!d1){ end=k; break; } }
  }
  if(end>start) 
  {    //// skip delimiter1 if specified otherwise include it
       if(delimiter1==0)
       {    strncpy(line, text+start, end-start);
            line[end-start] = 0;
       }else
       {    strncpy(line, text+start+1, end-start-1);
            line[end-start-1] = 0;
       }
       text += end+1;
  }else return;
}


//--------------------------------------------------------------------------//
//  first_non_comment                                                       //
//--------------------------------------------------------------------------//
int first_non_comment(char *text)
{
  int start=0, k, textlen = strlen(text);
  while(text[start]=='#') 
       for(k=start+1; k<textlen; k++) if(text[k]=='\n' || k==textlen-1){ start=k+1; break;}
  return start;
}


//--------------------------------------------------------------------------//
// macro                                                                    //
// If Enter was clicked 'text' is entire macro. If Shift-Enter 'text' is    //
// current line in editor.                                                  //
//--------------------------------------------------------------------------//
int macro(char *text, int loopmode)
{
   char **arg;              // arguments up to 80 chars passed as strings
   g.getout = 0;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   char *textptr = text, *otextptr;
   char line[1024];
   char err[256]="Macro terminated at line ";
   int argno=0,k,status=OK,totallines=0,len;
   int lineno=0;
   time_t ntime,otime;
   time(&otime);                   // Don't use gettimeofday(), not avail. on Convex
  
   char tempstring[1024]="";                              
   arg = new char *[MAXARGS];
   for(k=0;k<MAXARGS;k++) arg[k] = new char[1024];

   ////  Parse arguments & execute. Arg[0] is the command, arg[1] to
   ////  arg[argno] inclusive are the arguments.

   if(g.getout){ g.getout=0; return(ABORT); }
   g.getout=0;
   len = (int)strlen(text);
   for(k=0;k<len;k++) if(text[k]=='\n') totallines++;
   if(len) totallines++;
   printstatus(MACRO);

   while(1)
   {

              ////  textptr points to position of current line in text
       otextptr = textptr;
       if(textptr >= text + len) break;
              ////  Get a line and parse it into args - this changes textptr
              ////  Don't access textptr after this point, may be past end
       get_next_statement(textptr, line, arg, argno);  

       if(g.diagnose) printf("%s\n",line);              
       if(!strlen(line)) break;
              ////  Give other processes a chance only once/second - otherwise it 
              ////  slows down the macros too much
       time(&ntime);
       if(ntime!=otime)
       {    XtAppProcessEvent(g.app,XtIMAll);
            otime = ntime;
       }
       if(g.getout) break; 

              ////  Commands to be executed only once. Only commands that use loops
              ////  or local variables need to be here; others should be in execute().
              
              ////  Loops are handled here instead of in calculator.y, since 
              ////  yacc is not reentrant. This allows the same macro to handle 
              ////  image math and tnimage commands. Handling it here also permits
              ////  distinguishing between explicit 'for' and 'while' loops and
              ////  implicit loops over all pixels as in  'i=image[0][0][x][y];'

              ////  Loops change textptr and may call macro() with loopmode==1.
              ////  Commands that operate on more than one line should have a
              ////  separate entry here similar to the entry for "while".

       if(!strcmp(arg[0],"loop")) { loop(line, MACRO_MATH); continue; }   

// Not yet implemented:  'if' statement outside of loops
       ////  If statement for non-pixel loops (pixel loops handled in calculator.y)
//       if(!strcmp(arg[0],"if") && loopmode==0)   { loop(line, MACRO_IF); continue; }   
       if(!strcmp(arg[0],"goto")) { lineno = atoi(arg[1])-2; continue; }   
       if(!strcmp(arg[0],"stop")) break;
       if(!strcmp(arg[0],"for"))  { for_loop(line); continue; }
       if(!strcmp(arg[0],"while")){ loop(line, MACRO_WHILE); continue; }
       if(!strcmp(arg[0],"until")){ loop(line, MACRO_UNTIL); continue; }
       ////  Evaluate any arguments before executing
       if(strcmp(arg[0],"whatis") && strcmp(arg[0],"evaluate")){ evaluate_args(argno, arg); }
       if(g.getout) return 1;
       execute(line, argno, arg, loopmode);    
       g.getout = 0;
       XFlush(g.display);
       lineno++;
   }
  
           //---Reset parameters (end of macro)---//
   printstatus(NORMAL);
   g.want_format=-1;

   sprintf(tempstring,"%s %d\nError: ",err, lineno+1);
   status_error_message(status);
   if((status!=QUIT)&&(status!=OK)) message(tempstring,ERROR);
   for(k=0;k<MAXARGS;k++) delete[] arg[k];
   delete[] arg;
   return(status);
}


//--------------------------------------------------------------------------//
// for_loop - called by macro                                               //
//--------------------------------------------------------------------------//
void for_loop(char *textptr)
{   
  char *looptext;
  char *initial;
  char *limit;
  char *increment;
  int junk;
 
  initial = new char[256];
  limit = new char[256];
  increment = new char[256];
  looptext = new char[10000];


  //// These 4 statements change textptr
  get_expression(textptr, initial, '(', ';');
  get_expression(textptr, limit, 0, ';');
  get_expression(textptr, increment, 0, ')');
  strcat(initial, ";");
  strcat(limit, ";");
  strcat(increment, ";");

  get_next_statement(textptr, looptext, NULL, junk);
  remove_outer_parentheses(looptext, '{', '}');
  for(eval(initial); eval(limit); eval(increment)) macro(looptext, 1);

  delete[] looptext;
  delete[] increment;
  delete[] limit;
  delete[] initial;
}


//--------------------------------------------------------------------------//
// remove_outer_parentheses                                                 //
//--------------------------------------------------------------------------//
void remove_outer_parentheses(char *s, char c1, char c2)
{   
  int k, j = 0, paren = 0, len=strlen(s);
  char *tempstring;
  tempstring = (char*)malloc(len+1);
  for(k=0; k<len; k++)
  {    if(s[k] == c2) paren--;
       if(paren){ tempstring[j++]=s[k]; tempstring[j]=0; }
       if(s[k] == c1) paren++;
  }
  strcpy(s, tempstring);
  free(tempstring);
}


//--------------------------------------------------------------------------//
// loop - called by macro                                                   //
//--------------------------------------------------------------------------//
void loop(char *textptr, int looptype)
{ 
  char *looptext;
  char *stmttext;
  looptext = (char*)malloc(10000);
  stmttext = (char*)malloc(10000);

  //// These 2 statements change textptr
  if(looptype != MACRO_MATH) 
  {   get_expression(textptr, stmttext, '(', ')');
      strcat(stmttext, ";");
  }
  get_expression(textptr, looptext, '{', '}');
  switch(looptype)
  {   case MACRO_WHILE:
          while(eval(stmttext)) macro(looptext, 1);
          break;
      case MACRO_IF:
          if(eval(stmttext)) macro(looptext, 1);
          break;
      case MACRO_UNTIL:
          while(!eval(stmttext)) macro(looptext, 1);
          break;
      case MACRO_MATH:
          math(looptext);
          break;
  }
      
  free(stmttext);
  free(looptext);
}


//--------------------------------------------------------------------------//
//  macro_open - open file in a macro                                       //
//--------------------------------------------------------------------------//
void macro_open(char *filename)
{ 
   FILE *fp;
   fp = fopen(filename, "w");
   //// Put stream ptr in a ptr variable, not numeric, so it is not
   //// substituted back in evaluate_args.
   add_pointer_variable(filename, (void*)fp); 
}


//--------------------------------------------------------------------------//
//  macro_close - close file in a macro                                     //
//--------------------------------------------------------------------------//
void macro_close(char *filename)
{ 
   FILE *fp;
   void *ptr;
   ptr = read_ptr(filename);
   if(ptr)
   {   fp = (FILE*)ptr;
       fclose(fp);
   }
}

 
//--------------------------------------------------------------------------//
// edit                                                                     //
// simple editor                                                            //
// count = total no. of rows in editor                                      //
// want_execute_button determines what buttons appear on bottom.            //
//    0 = ok cancel help                                                    //
//    1 = dismiss cancel help execute                                       //
// Calling routine must not deallocate text until edit window is closed.    //
//--------------------------------------------------------------------------//
Widget edit(const char *title, const char *subtitle, char *&text, int rows, int columns, 
     int width, int height, int maxlength, int helptopic, int want_execute_button, 
     char *editfilename, void (*f9)(void *ptr, char *text), void *client_data)
{
   int xpos;
   if (height==0) height=200;
   if (width==0) width=580;
   static Widget *w, editor; 
   Widget form, loadbut, savebut, execbut=0, labelwidget;
   Widget okaybut, cancbut, helpbut;
   ced = new clickboxinfo;
   XmString xms;
   int n, wc=0, button_width, button_sep;
   Arg args[100];
   char *filename = NULL;
   if(filename==NULL){ filename = new char[FILENAMELENGTH]; filename[0]=0; }
   if(editfilename!=NULL) strcpy(filename, editfilename);
   gtextptr = text;
   
   w = new Widget[100];     // List of widgets to destroy

   //------- ` a form dialog shell widget -----------------------//
   n=0;
   //XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++; // Block WM delete button
   XtSetArg(args[n], XmNresizable, True); n++;
   XtSetArg(args[n], XmNautoUnmanage, False); n++;
   XtSetArg(args[n], XmNtransient, True); n++; 
   XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple((char*)title)); n++;
   form = w[wc++] = XmCreateFormDialog(g.main_widget, (char*)"EditForm", args, n);
   XmStringFree(xms);
 
   ////  Add label
   XtSetArg(args[n], XmNleftPosition, 2); n++;     // % of width from left
   XtSetArg(args[n], XmNrightPosition, 100); n++;  
   XtSetArg(args[n], XmNtopPosition, 0); n++;      // % of height from top
   XtSetArg(args[n], XmNtopOffset, 1); n++;     
   XtSetArg(args[n], XmNheight, 20); n++;     
   XtSetArg(args[n], XmNresizable, False); n++;
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
   labelwidget = XmCreateLabel(form, (char*)subtitle, args, n);
   XtManageChild(labelwidget);   

   n = 0;
   XtSetArg(args[n], XmNwidth, width); n++;
   XtSetArg(args[n], XmNheight, height); n++;
   XtSetArg(args[n], XmNeditable, True); n++;
   XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
   XtSetArg(args[n], XmNmaxLength, maxlength); n++;
   XtSetArg(args[n], XmNcolumns, columns); n++;
   XtSetArg(args[n], XmNrows, rows); n++;
   XtSetArg(args[n], XmNleftPosition, 1); n++;       // % of width from left
   XtSetArg(args[n], XmNmarginWidth, 1); n++;
   XtSetArg(args[n], XmNmarginHeight, 0); n++;
   XtSetArg(args[n], XmNtopPosition, 0); n++;      // % of height from top
   XtSetArg(args[n], XmNbottomPosition, 98); n++;  // % of width from top
   XtSetArg(args[n], XmNrightPosition, 99.8); n++;   // % of height from left
   XtSetArg(args[n], XmNtopOffset, 22); n++;       
   XtSetArg(args[n], XmNbottomOffset, 24); n++;       // % of height from top
   XtSetArg(args[n], XmNfractionBase, 100); n++;     // Use percentages
   XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
   XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
   ////  Stop Motif from trying to grab another color if none are available.
   if(g.want_colormaps)  
   {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
        XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
   }
   editor = w[wc++] = XmCreateScrolledText(form, (char*)"Editor", args, n);
   XmTextSetString(editor, text);

   ced->ptr[1]    = (char *)text;  // Starting text         
   ced->widget[0] = form;          // Widget to put answer on
   ced->widget[1] = editor;        // Widget to read string from
   ced->widget[2] = labelwidget;   // Label to put filename in
   ced->count = rows;              // Max. no. of rows (not enforced)
   ced->maxval    = maxlength;     // Max. no. of characters
   ced->title = new char[FILENAMELENGTH];
   strcpy(ced->title, title);      // Title to display
   ced->filename  = filename;      // Filename
   ced->ask       = 1;             // Use getfilename() to get file name
   ced->allowedit = 2;             // Each line is a command
   ced->form      = form;          // Parent form
   ced->path      = NULL;
   ced->maxstringsize = maxlength; // Maximum string size
   ced->w         = w;
   ced->f9        = f9;            // Client callback when Accept is clicked
   ced->client_data = client_data;
   ced->done      = 0;
   ced->answers   = NULL;
   
   XtAddCallback(editor, XmNmotionVerifyCallback, (XtCBP)editcursorcb, (XtP)ced);
   XtAddCallback(editor, XmNmodifyVerifyCallback, (XtCBP)editmodifycb, (XtP)ced);
   XtManageChild(editor);
   XtManageChild(form);
   if(want_execute_button)
   {   button_width = 75;
       button_sep = 1;
   }else 
   {   button_width = 100;
       button_sep = 10;
   }

   ////  Ok, Cancel, Help buttons & their callbacks
   xpos = 1;
   okaybut = w[wc++] = add_button(form, (char*)"Accept", xpos, 1, button_width);
   xpos += 8 + button_width + button_sep;
   cancbut = w[wc++] = add_button(form, (char*)"Dismiss",xpos, 6, button_width);
   xpos += button_width + button_sep;
   helpbut = w[wc++] = add_button(form, (char*)"Help",   xpos, 6, button_width);
   xpos += button_width + button_sep;
   loadbut = w[wc++] = add_button(form, (char*)"Load",   xpos, 6, button_width);
   xpos += button_width + button_sep;
   savebut = w[wc++] = add_button(form, (char*)"Save",   xpos, 6, button_width);
   xpos += button_width + button_sep;
   if(want_execute_button)
      execbut = w[wc++] = add_button(form, (char*)"Execute",   xpos, 6, button_width);

   XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)editokcb,   (XtP)ced);
   XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb,     (XtP)&g.helptopic[helptopic]);
   XtAddCallback(savebut, XmNactivateCallback, (XtCBP)savetextcb, (XtP)ced);
   XtAddCallback(loadbut, XmNactivateCallback, (XtCBP)loadtextcb, (XtP)ced);
   if(want_execute_button)
      XtAddCallback(execbut, XmNactivateCallback, (XtCBP)executetextcb, (XtP)ced);
   XtAddCallback(form, XmNunmapCallback, (XtCBP)editunmapcb, (XtP)ced);
   XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)editdonecb, (XtP)ced);

   XtVaSetValues(loadbut, XmNbackground, g.main_bcolor,
         XmNforeground, g.main_fcolor, NULL);
   XtVaSetValues(savebut, XmNbackground, g.main_bcolor,
         XmNforeground, g.main_fcolor, NULL);
   if(want_execute_button)
       XtVaSetValues(execbut, XmNbackground, g.main_bcolor,
           XmNforeground, g.main_fcolor, NULL);

   // causes problems if 2 editors on top of each other
   //   XtAddEventHandler(XtParent((form)), 
   //        FocusChangeMask | VisibilityChangeMask, 
   //        False, (XtEH)raisecb, (XtP)NULL);

   g.getout=0; 
   ced->wc = wc;
   return editor;
}


//--------------------------------------------------------------------------//
// editentercb                                                              //
//--------------------------------------------------------------------------//
void editentercb(Widget w, XtP client_data, XEvent *event)
{   
   w=w; client_data=client_data;   
   int key = which_key_pressed(event);
   //// This may or may not do anything.
   if(key==XK_Return) editokcb(w, client_data, NULL);  
   if(key==XK_Escape) editdonecb(w, client_data, NULL);  
}


//--------------------------------------------------------------------------//
// which_key_pressed                                                        //
//--------------------------------------------------------------------------//
int which_key_pressed(XEvent *event)
{
   if(event->type != KeyPress) return 0;
   char buffer[20];
   XComposeStatus compose;
   XKeyEvent keyevent = event->xkey;
   KeySym keysym;
   int key = 0;  
   int bufsize = 20;
   XLookupString(&keyevent,buffer,bufsize,&keysym,&compose);
   key = keysym & 0xffff;
   return key;
}


//--------------------------------------------------------------------------//
//  editokcb                                                                //
//--------------------------------------------------------------------------//
void editokcb(Widget w, XtP client_data, XmACB *call_data)
{

  char *s, *text;
  w=w;call_data=call_data;
  clickboxinfo *c = (clickboxinfo*)client_data;

  Widget editor = c->widget[1];
  text = (char*)c->ptr[1];
  strcpy(text, s=XmTextGetString(editor));  // This allocates memory
  if(c->f9 != NULL) c->f9(c->client_data, text);
  XtFree(s);
  editdonecb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
//  editunmapcb                                                             //
//--------------------------------------------------------------------------//
void editunmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!in_editdonecb) editdonecb(w, client_data, call_data);
}

//--------------------------------------------------------------------------//
//  editdonecb                                                              //
//--------------------------------------------------------------------------//
void editdonecb(Widget w, XtP client_data, XmACB *call_data)
{
  int len,k;
  w=w;call_data=call_data;
  char *otext, *text;
  if(inmath) return;
  g.getout = 1;
  if(g.busy) return;
  clickboxinfo *c = (clickboxinfo*)client_data;
  in_editdonecb = 1;

  if(c && c->ptr[1] && c->allowedit && c->widget[1] && XtIsManaged(c->widget[1])) 
  {    otext = (char*)c->ptr[1];
       text = XmTextGetString(c->widget[1]);      // This allocates memory
       len = strlen(text);
       if(len>=0) strcpy(otext, text);
       XtFree(text);
   }
  XtRemoveEventHandler(XtParent(c->form), StructureNotifyMask, False, 
      (XtEH)raisecb, (XtP)NULL);
  if(c && c->form && XtIsManaged(c->form)) XtUnmanageChild(c->form);

  c->done = 1;

  for(k=0; k<c->wc; k++) XtDestroyWidget(c->w[k]);
  delete[] c->w;
  delete[] c->title;
  delete[] c;
  c = 0;
  in_editdonecb = 0;
  g.sketch_editor_widget = 0; // in case in sketch math
}  


//--------------------------------------------------------------------------//
// editmodifycb  - called when text in editor changes                       //
//--------------------------------------------------------------------------//
void editmodifycb(Widget w, XtP client_data, XmACB *call_data)
{
   const int CMDLEN=FILENAMELENGTH;
   w=w;  //keep compiler quiet
   int k,len,start=0,end=0,pos=0;
   char *text=NULL;
   char command[FILENAMELENGTH];
   char newtext=0;
   char **arg;                // up to 20 arguments
   if(inmath) return;
   g.getout = 0;
   clickboxinfo *c = (clickboxinfo *)client_data;
   XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct *)call_data;
   if(tvcs->text->length && tvcs->text->ptr != NULL) 
       newtext = tvcs->text->ptr[0];

   ////  It is impossible to access the text without making a copy of it.
   ////  Getting XmNvalue also makes a copy.
   ////  Execute current line as a command. Change c.allowedit to 1 if you
   ////  don't want this.  

   ////  Local memory allocation and deallocation must be outside of Xm memory
   ////  functions for unknown reason.
   
   arg = new char*[20];
   for(k=0;k<20;k++) arg[k] = new char[CMDLEN];

   if(c->allowedit==2 && newtext == '\r') 
   {  
       text = XmTextGetString(c->widget[1]);      // This allocates memory
       if(text==NULL){ message("Error getting text string",ERROR); return; }
       if(tvcs->text->ptr) tvcs->text->length=0;  // Get motif to ignore the \r
       start = end = pos = tvcs->startPos;
       len = strlen(text);
       for(k=pos-1;k>=0;k--)                      // Find most recent \n
       {    if(text[k]=='\n'){ start=k+1; break; }
            if(k==0){ start=0; break; }
       }
       for(k=pos;k<=len;k++)                      // Find next \n
       {    if(text[k]=='\n'){ end=k; break; }
            if(k==len){ end=len; break; }
       }
       command[0]=0;
       if(start<end)
       {    strncpy(command,text+start,end-start);
            command[end-start]=0;
            for(k=strlen(command);k;k--)if(command[k]<=' ')command[k]=0; else break;
            if(g.diagnose) printf("Macro command %s\n",command);
            if(g.getout) return;
            if(strlen(command)) macro(command);
            if(g.getout) return;
       }
       if(!g.getout) 
       {   strcpy((char*)c->ptr[1], text);  // Starting text         
           XtFree(text);
       }
   }
   if(!g.getout) 
   {   XSetInputFocus(g.display, XtWindow(w), RevertToParent, CurrentTime);
       for(k=0;k<20;k++) delete[] arg[k];
       delete arg;  
   }
}
 


//--------------------------------------------------------------------------//
// editcursorcb  - called when cursor moves in editor                       //
//--------------------------------------------------------------------------//
void editcursorcb(Widget w, XtP client_data, XmACB *call_data)
{
    w=w;  //keep compiler quiet
    int k,pos=0,row=1,col=1;
    Window win;
    char *s;
    char rowcol[40];
    clickboxinfo *c = (clickboxinfo *)client_data;
    XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct *)call_data;
    pos = tvcs->newInsert;
    s = XmTextGetString(c->widget[1]);   // This allocates memory
    for(k=0;k<pos;k++) if(s[k]=='\n'){col=1; row++;} else col++;
    sprintf(rowcol,"%d   %d   ",row,col);
    win = XtWindow(c->widget[0]);

    //  Motif doesn't care if no. of rows increases so leave this commented out.
    //    if(row>c->count)                // Truncate entry if too many rows
    //    {   s[pos]=0;
    //        XmTextSetString(c->widget[1], s);
    //    }

    XDrawImageString(g.display, win, g.gc, 200, 20, rowcol, strlen(rowcol));
    XtFree(s);
}


//--------------------------------------------------------------------//
// savetextcb - callback for saving text data from text widget        //
//--------------------------------------------------------------------//
void savetextcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data; // keep compiler quiet
   clickboxinfo *c = (clickboxinfo *)client_data;
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
   int answer = CANCEL;
   static char *text;
   static char *filename;
   static PromptStruct ps;
   filename = c->filename;
   Widget editor = c->widget[1];
   text = XmTextGetString(editor);  // This allocates memory
   answer = message("New filename:",filename,PROMPT,FILENAMELENGTH-1,54);
   if(answer != YES) return;

   ps.filename = filename;
   ps.f1 = savetext_part2;
   ps.f2 = savetext_finish;
   ps.text = text;
   check_overwrite_file(&ps);
}


//--------------------------------------------------------------------//
// savetext_part2                                                     //
//--------------------------------------------------------------------//
void savetext_part2(PromptStruct *ps)
{
   FILE *fp;
   char tempstring[100];
   char *text = (char *)ps->text;
   if ((fp=fopen(ps->filename,"w")) == NULL)
        error_message(ps->filename, errno);
   else
   {    fprintf(fp,"%s\n",text);
        sprintf(tempstring,"Text saved in %s",ps->filename);
        message(tempstring);
        fclose(fp);
   }
}


//--------------------------------------------------------------------//
// savetext_finish                                                    //
//--------------------------------------------------------------------//
void savetext_finish(PromptStruct *ps)
{
   char *text = (char *)ps->text;
   XtFree(text);
}


//--------------------------------------------------------------------//
// loadtextcb - callback for reading text data into text widget       //
//--------------------------------------------------------------------//
void loadtextcb(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; call_data=call_data; // keep compiler quiet
   clickboxinfo *c = (clickboxinfo *)client_data;
   char *text = (char*)c->ptr[1];
   char *filename = c->filename;
   char *oldfilename;
   oldfilename = new char[FILENAMELENGTH];
   strcpy(oldfilename, filename);
   
   Widget editor = c->widget[1];
   if(memorylessthan(4096)){  message(g.nomemory,ERROR); return; } 
   int size=0;
   FILE *fp;

   if(c->ask) strcpy(filename, getfilename(oldfilename, NULL));
   if(!c || c->done){ message("Missing editor",ERROR); delete[]oldfilename; return; }
   if(g.getout){ g.getout=0; return; }
   text[0] = 0;
   if(!access(filename,F_OK))            //// access = 0 if file exists  
        size = filesize(filename);       //// Check to ensure not an empty file
   else 
        error_message(filename, errno);
   if(size >= c->maxval)                 //// Check to ensure enough memory
        message("Macro file is too large!",ERROR);
   else if ((fp=fopen(filename,"r")) == NULL)
        error_message(filename, errno);
   else                                 
   {    fread((void*)text,size,1,fp);    //// Read the entire file 
        text[size]=0;
        if(text[size-1]==10) text[size-1]=0;
        fclose(fp);
        if(c->ask && editor) XmTextSetString(editor, text);
   }
   if(strlen(filename) && c->widget[2]) set_widget_label(c->widget[2], filename);
   delete[] oldfilename;
   return;
}


//--------------------------------------------------------------------//
// readmacrofile                                                      //
//--------------------------------------------------------------------//
int readmacrofile(char *filename, char *text)
{
    clickboxinfo c;
    c.title = new char[FILENAMELENGTH];
    c.title[0] = 0;
    c.ask = 0;
    c.done = 0;
    c.allowedit = 1;
    c.ptr[1]    = text;
    c.maxval    = MACROLENGTH;
    c.filename  = filename;

    c.widget[0] = 0;  
    c.widget[1] = 0;  
    c.widget[2] = 0;   
    c.f1 = null;
    c.f2 = null;
    c.f3 = null;
    c.f4 = null;
    c.f5 = null;
    c.f6 = null;
    c.f7 = null;
    c.f8 = null;
    loadtextcb(g.main_widget, &c, NULL);
    text = (char*)c.ptr[1];
    if(text[0]!=0) strcpy(text, (char *)c.ptr[1]);
    delete[] c.title;
    return OK;
}


//--------------------------------------------------------------------//
// parsecommand                                                       //
// input: a single command with arguments  e.g. load(test,1,1,2,3)    //
// Each word k is placed in arg[k][]. arg[0][] is the command, the    //
// rest are arguments.                                                //
// The no. of arguments, not counting arg[0], is placed in `noofargs'.//
// 'maxlen' is allocated size of the string in arg[][].               //
// arg must be already allocated.                                     //
// 'type' can be FORMULA or TEX                                       //
//--------------------------------------------------------------------//
void parsecommand(char *cmd, int type, char **arg, int &noofargs, int maxlen, int maxargs)
{   
   int count=0,start=0;
   char *buf;
   char word[FILENAMELENGTH]="";
   buf = new char[FILENAMELENGTH];
   noofargs = -1;
   count = 0;
   do
   {    
        if(type==FORMULA) get_next_word(cmd+count, word, maxlen, start);
        if(type==TEX) get_next_tex_word(cmd+count, word, maxlen, start);
        if(word==NULL) break;
        noofargs++;
        strcpy(buf, word);
        count += 1+strlen(buf) + start;
        count += strspn(cmd+count, "(,;)");
        strncpy(arg[noofargs], buf, maxlen-1);
        remove_trailing_junk(arg[noofargs]);
        if(!strlen(arg[noofargs])) noofargs--;
        if(count >= (int)strlen(cmd)) break;
        if(noofargs >= maxargs) break;
   }while(1);
   delete[] buf;
}


//--------------------------------------------------------------------//
// get_next_word - returns first word demarcated by a space in buffer //
// (or, if word starts with a quotation mark, demarcated by quotes).  //
//--------------------------------------------------------------------//
void get_next_word(char *buffer, char *word, int maxlen, int &start)
{
    int space;
    start = 0;
    start += strspn(buffer+start, " ,;({\n");       // Position of 1st non space
    start += first_non_comment(buffer+start);
    if(buffer[start]=='\"')                         // If in a quote
         space = strcspn(buffer+ ++start,"\"");     // Position of 2nd quote
    else
         space = strcspn(buffer+start, "{(,)}\n");  // Start of next arg
    if(space==0) return;
    space = min(maxlen, space);
    strncpy(word, buffer+start, space);   
    word[space]=0;
}    


//--------------------------------------------------------------------//
// get_next_tex_word                                                  //
//--------------------------------------------------------------------//
void get_next_tex_word(char *buffer, char *word, int maxlen, int &start)
{
    int space;
    start = strspn(buffer, "\\");                 // Position of 1st backslash
    if(buffer[start]=='\"')                       // If in a quote
         space = strcspn(buffer+ ++start,"\"");   // Position of 2nd quote
    else
         space = strcspn(buffer+start, " ");      // Position of next space
    if(space==0) return;
    space = min(maxlen,space);
    strncpy(word, buffer+start, space);   
    word[space]=0;
}    


//--------------------------------------------------------------------//
// executetextcb                                                      //
//--------------------------------------------------------------------//
void executetextcb(Widget w, XtP client_data, XmACB *call_data)
{
   char *text;
   int k;
   XmTextPosition cursorpos, linestartpos=0;
   w=w; call_data=call_data; // keep compiler quiet
   if(inmath) return;
   clickboxinfo *c = (clickboxinfo *)client_data;
   Widget editor = c->widget[1];
   text = XmTextGetString(editor);  // This allocates memory
   XtVaGetValues(editor, XmNcursorPosition, &cursorpos,  NULL);         
   for(k=cursorpos; k; k--) if(text[k]=='\n'){ linestartpos = k+1; break; }
   macro(text+linestartpos);
   XtFree(text);
   //// This doesn't work because the @#$% button grabs the focus after return
   if(!g.getout) XSetInputFocus(g.display, XtWindow(editor), RevertToParent, CurrentTime);
   return;
}


//-------------------------------------------------------------------------//
// pixels  -  macro function, called by calculator                         //
//-------------------------------------------------------------------------//
double pixels(double image_number)
{
  int ino = cint(image_number);
  if(between(ino, 0, g.image_count-1)) return(double)(z[ino].xsize * z[ino].ysize);
  return 0;
}


//-------------------------------------------------------------------------//
// rdensity  - density of rectangular region, called by calculator         //
//-------------------------------------------------------------------------//
double rdensity(double xx1, double yy1, double xx2, double yy2)
{
  double d;
  int i,j,ino,bpp, datatype;
  int x1 = (int)xx1;
  int y1 = (int)yy1;
  int x2 = (int)xx2;
  int y2 = (int)yy2;
  int comp = (int)read_variable((char*)"COMPENSATE", NULL, datatype);
  int invert = (int)read_variable((char*)"INVERT", NULL, datatype);
  d = 0;
  for(j=y1; j<=y2; j++)
  for(i=x1; i<=x2; i++)
  {   ino = whichimage(i,j,bpp);
      d += pixeldensity(i,j,ino,comp,invert);
  }
  return d;
}


//-------------------------------------------------------------------------//
// cdensity  - density of circular region, called by calculator            //
//-------------------------------------------------------------------------//
double cdensity(double xx, double yy, double diam)
{
  double d;
  int i,j,ino,bpp, datatype;
  int x = (int)xx;
  int y = (int)yy;
  int diameter = (int)diam;
  int x1 = x - cint(diameter/2);
  int y1 = y - cint(diameter/2);
  int x2 = x + cint(diameter/2);
  int y2 = y + cint(diameter/2);
  int comp = (int)read_variable((char*)"COMPENSATE", NULL, datatype);
  int invert = (int)read_variable((char*)"INVERT", NULL, datatype);
  d = 0;
  for(j=y1; j<=y2; j++)
  for(i=x1; i<=x2; i++)
  {   ino = whichimage(i,j,bpp);
      if(insidecircle(i,j,x,y,diameter))
         d += pixeldensity(i,j,ino,comp,invert);
  }
  return d;
}


//-------------------------------------------------------------------------//
// macro_double_input  -  macro function, called by calculator             //
//-------------------------------------------------------------------------//
double macro_double_input(char *prompt)
{
  char tempstring[FILENAMELENGTH];
  tempstring[0]=0;
  message(prompt, tempstring, PROMPT, FILENAMELENGTH, 30);
  return atof(tempstring);
}


//-------------------------------------------------------------------------//
// macro_string_input  -  macro function, called by calculator             //
//-------------------------------------------------------------------------//
void macro_string_input(char *prompt, char *returnstring)
{
  returnstring[0]=0;
  message(prompt, returnstring, PROMPT, FILENAMELENGTH, 30);
}
