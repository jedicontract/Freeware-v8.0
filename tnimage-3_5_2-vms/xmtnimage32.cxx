//--------------------------------------------------------------------------//
// xmtnimage32.cc                                                           //
// Latest revision: 10-12-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Spreadsheet creation and handling                                        //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;

int selecting=0;
int in_spreadsheet_finish = 0;

//--------------------------------------------------------------------------//
// create_spreadsheet                                                       //
//--------------------------------------------------------------------------//
void create_spreadsheet(int ino)
{ 
#ifdef HAVE_XBAE
  if(ino<0){ message("Please select an image"); return; }
  Spreadsheet *s = z[ino].s;
  s = new_spreadsheet(ino,0);
  XtManageChild(s->ssform); 
#else
  ino=ino;
#endif
}  

//--------------------------------------------------------------------------//
// spreadsheet_unmapcb                                                      //
//--------------------------------------------------------------------------//
void spreadsheet_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!in_spreadsheet_finish) spreadsheet_finish(w, client_data, call_data);
}

//--------------------------------------------------------------------------//
// spreadsheet_finish                                                       //
//--------------------------------------------------------------------------//
void spreadsheet_finish(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data; call_data=call_data;
  in_spreadsheet_finish = 1;
  Spreadsheet *s = (Spreadsheet *)client_data;
  XtUnmanageChild(s->ssform);
  in_spreadsheet_finish = 0;
  s->visible=0;
}


//--------------------------------------------------------------------------//
// spreadsheet_ok                                                           //
//--------------------------------------------------------------------------//
void spreadsheet_ok(Widget w, XtP client_data, XmACB *call_data)
{
   w=w; client_data=client_data; call_data=call_data;
}


//--------------------------------------------------------------------------//
// new_spreadsheet                                                          //
// If 'Widget' is non-zero, it reuses an old widget instead of creating a   //
//   new one. This is useful in converting image depths.                    //
//--------------------------------------------------------------------------//
Spreadsheet *new_spreadsheet(int ino, Widget oldwidget)
{
#ifdef HAVE_XBAE
  const int MAXWIDGETS = 30;     // max.no.of widgets in spreadsheet
  Widget okaybut, cancbut, helpbut, savebut;
  int i,j,k,n,cols,rows;
  if(ino<0) return NULL;
  Arg args[100];
  Widget form,w;  
  XmString xms;
  Spreadsheet *s = z[ino].s;
  static int helptopic = 46;
  Dimension mainmenuheight;
  int y1;
  int ystart=4;             // y position for first buttons on left
  rows=z[ino].ysize; cols=z[ino].xsize;

  if(!s->created)
  {    if(g.diagnose)
       {  printf("Creating spreadsheet %d rows %d cols...",rows,cols);fflush(stdout);}
       s->widgetcount=0;
       s->widget = new Widget[MAXWIDGETS];
       XtVaGetValues(g.menubar, XmNheight, &mainmenuheight, NULL);       
       y1 = (int)mainmenuheight;
       n=0;
       XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
       XtSetArg(args[n], XmNtopWidget, g.menubar); n++;
       XtSetArg(args[n], XmNwidth, 489); n++;
       XtSetArg(args[n], XmNheight, 489); n++;
       XtSetArg(args[n], XmNautoUnmanage, False); n++;
       XtSetArg(args[n], XmNx, 0); n++;
       XtSetArg(args[n], XmNy, y1); n++;
       XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
       XtSetArg(args[n], XmNtransient, False); n++;
       XtSetArg(args[n], XmNdialogTitle, xms = XmStringCreateSimple(z[ino].name));n++;
       if(oldwidget) 
            form = oldwidget;
       else
            form = XmCreateFormDialog(g.main_widget,(char*)"SpreadsheetForm",args,n);

       XmStringFree(xms);
       s->widget[s->widgetcount++] = form;
 
       //// Make an extra row to park cursor when refreshing spreadsheet
       s->sswidget = w = XtVaCreateManagedWidget("Spreadsheet",
                                xbaeMatrixWidgetClass,  form,
                                XmNtitle,               "Spreadsheet",
                                XmNleftAttachment,      XmATTACH_POSITION,
                                XmNautoUnmanage,        False,
                                XmNleftOffset,          110,
                                XmNcolumns,             cols,
                                XmNrows,                rows,
                                XmNrightAttachment,     XmATTACH_FORM,
                                XmNtopAttachment,       XmATTACH_FORM,
                                XmNbottomAttachment,    XmATTACH_FORM,
                                XmNtopOffset,           20,
                                XmNbottomOffset,        30,
                                XmNselectScrollVisible, False,
                                XmNallowColumnResize,   True,
                                XmNcellMarginHeight,    0,
                                XmNcellMarginWidth,     0,
                                XmNtraverseFixedCells,  True,
                                XmNgridType,            XmGRID_SHADOW_IN,
                                XmNhorizontalScrollBarDisplayPolicy, 
                                                        XmDISPLAY_STATIC,
                                XmNverticalScrollBarDisplayPolicy, 
                                                        XmDISPLAY_STATIC,                               
                                XmNspace,               2,
                                XmNbuttonLabels,        False,
                                NULL);

       s->widget[s->widgetcount++] = w;
       s->visible = 1;
       s->created = 1;
       s->rows = rows;
       s->cols = cols;
       s->number_format = INTEGER;
       s->display = ORIGINAL;
       s->ino = ino;
       s->ssform = form;
 
       //// Add radio buttons on left (this changes s->widgetcount and ystart)

       const char button_group1[][20]={ "Image","Real","Imaginary","Wavelet"};
       add_button_group(form, 4, ystart, 0, s, (void*)ssdisplaycb, (char*)"Display", button_group1);
       const char button_group2[][20]={ "Int","Hex","Float","RGB","RGB Hex"};
       add_button_group(form, 5, ystart, 0, s, (void*)ssdisplaycb, (char*)"Number format", 
           button_group2);

       //// Ok, Cancel, Help buttons & their callbacks
       okaybut = s->widget[s->widgetcount++] = add_button(form, (char*)"Accept",   1, 1, 100);
       cancbut = s->widget[s->widgetcount++] = add_button(form, (char*)"Dismiss",115, 6, 100);
       helpbut = s->widget[s->widgetcount++] = add_button(form, (char*)"Help",   225, 6, 100);
       savebut = s->widget[s->widgetcount++] = add_button(form, (char*)"Save",   335, 6, 100);

       XtAddCallback(okaybut, XmNactivateCallback, (XtCBP)spreadsheet_ok, (XtP)s);
       XtAddCallback(cancbut, XmNactivateCallback, (XtCBP)spreadsheet_finish, (XtP)s);
       XtAddCallback(helpbut, XmNactivateCallback, (XtCBP)helpcb, (XtP)&g.helptopic[helptopic]);
       XtAddCallback(savebut, XmNactivateCallback, (XtCBP)spreadsheet_savecb, (XtP)s);
    
       XtManageChild(form);
       XtMapWidget(z[ino].widget);
       XtManageChild(s->sswidget);
       XtRealizeWidget(s->sswidget);
       s->ssform_window = XtWindow(form);

       XtAddCallback(form, XmNunmapCallback, (XtCBP)spreadsheet_unmapcb, (XtP)s);
       XtAddCallback(s->sswidget,XmNmodifyVerifyCallback,(XtCBP)modifycb,(XtP)s);
       XtAddCallback(s->sswidget,XmNselectCellCallback,(XtCBP)selectcb,(XtP)s);
       XtAddCallback(s->sswidget,XmNenterCellCallback,(XtCBP)entercellcb,(XtP)s);
       XtAddCallback(s->sswidget,XmNleaveCellCallback,(XtCBP)leavecellcb,(XtP)s);
       XtAddCallback(s->sswidget,XmNprocessDragCallback,(XtCBP)dragcb,(XtP)s);
       XtAddCallback(s->sswidget,XmNselectCellCallback,(XtCBP)selectcb, (XtP)s);
       XtAddCallback(s->sswidget,XmNlabelActivateCallback,(XtCBP)columnselectcb,(XtP)s);
 
       short int *col_widths = new short int[cols];
       for(k=0;k<cols;k++) col_widths[k]=CELLWIDTH;
       XtVaSetValues(s->sswidget, XmNcolumnWidths, col_widths, NULL);
       delete[] col_widths;
 
       ////  Have to use 'beginning' because cell on cursor is always aligned left.
       uchar *col_alignments = new uchar[cols];
       for(k=0;k<cols;k++) col_alignments[k]=XmALIGNMENT_BEGINNING;
       XtVaSetValues(s->sswidget, XmNcolumnAlignments, col_alignments, NULL);
       delete[] col_alignments;
 
       Boolean *col_button_labels = new Boolean[cols];
       for(k=0;k<cols;k++) col_button_labels[k]=True;
       XtVaSetValues(s->sswidget, XmNcolumnButtonLabels, col_button_labels, NULL);
       delete[] col_button_labels;

       char **col_labels = new char*[cols];
       for(k=0;k<cols;k++)
       {    col_labels[k] = new char[20];
            sprintf(col_labels[k], "Col.%d",k);
       }
       XtVaSetValues(s->sswidget, XmNcolumnLabels, col_labels, NULL);
       for(k=0;k<cols;k++) delete[] col_labels[k];
       delete[] col_labels;
 
       ////  Text in each cell -  s->text is a char[row][col][CELLBYTES]
       s->text = new char **[rows];
       for(j=0;j<rows;j++)
       {    s->text[j] = new char *[cols];
            for(i=0;i<cols;i++)
                s->text[j][i] = new char[CELLBYTES+1];
       }
 
 
       ////  Flag if cell is selected
       s->selected_1d = new uchar[rows*cols];
       s->selected = make_2d_alias(s->selected_1d, cols, rows);
 
       ////  Length of each column
       s->maxrow = new int[cols];
       for(i=0;i<cols;i++) s->maxrow[i] = 0;
       s->maxcol = 0;
 
       ////  Flag if column is selected
       s->column_selected = new uchar[cols];
       for(i=0;i<cols;i++) s->column_selected[i] = 0;
 
       XbaeMatrixSetCell(s->sswidget,0,0,(char*)" ");  // Initialize memory
       putimageinspreadsheet(ino);
       if(g.diagnose) printf("done\n");

  }else
  {    s->visible = 1;
       putimageinspreadsheet(ino);
  }

  ////  Make sure they don't know what to do.
  if(s->widgetcount >= MAXWIDGETS) fprintf(stderr, 
"Error: spreadsheet has too many widgets.\nAsk a wizard to enlarge me.");
  return s;
#else
  ino=ino; oldwidget=oldwidget;
  return NULL;
#endif
}


//--------------------------------------------------------------------------//
//  add_button_group                                                        //
//  a group of radio buttons surrounded by a frame                          //
//  Can't use va_arg because it won't compile in Irix or Solaris            //
//--------------------------------------------------------------------------//
void add_button_group(Widget parent, int count, int &ystart, int selection, 
     Spreadsheet *s, void *cb, char *title, const char label[][20])
{
  const int LENGTH = 9;      // Length of label on radio button
  const int SPACING = 20;    // Spacing between radio buttons
  Widget w, radiobox, frame, button_drawing_area;
  char tempstring[128];
  Arg args[100];
  int j,k,len,n,x=10,y=4,button=0;
  int xsize=100, ysize=(count+1)*26;
  if(ystart==4) s->noofradios=1; else s->noofradios++;

  //--------------------frame for buttons------------------------------------//
  n=0;
  XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, ystart); n++;
  XtSetArg(args[n], XmNwidth, xsize); n++;
  XtSetArg(args[n], XmNheight, ysize); n++;
  XtSetArg(args[n], XmNresizable, True); n++;
  frame = XmCreateFrame(parent, (char*)"frame", args, n);
  s->widget[s->widgetcount++] = frame;

  //--------------info area (inside frame)----------------------------------//
 
  n=0;
  XtSetArg(args[n], XmNtitle, "ButtonArea"); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNx, 0); n++;
  XtSetArg(args[n], XmNwidth, xsize); n++;
  XtSetArg(args[n], XmNheight, ysize); n++;
  button_drawing_area = w = XmCreateDrawingArea(frame,(char*)"button_drawing_area",args,n);
  s->widget[s->widgetcount++] = w;
 
  //--------------title----------------------------------------------------//
  w = addlabel(button_drawing_area,title,LEFT,x,y+2,98,y+SPACING);
  s->widget[s->widgetcount++] = w;
  XtManageChild(w);
  y+=15;
  
  ////  Create a radio box group
  n=0;
  XtSetArg(args[n], XmNspacing, 0); n++;
  XtSetArg(args[n], XmNradioBehavior, True); n++;
  XtSetArg(args[n], XmNradioAlwaysOne, True); n++;
  XtSetArg(args[n], XmNy, y); n++;
  XtSetArg(args[n], XmNtopPosition, y); n++;    
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNbottomPosition, y+SPACING); n++; 
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNpacking,XmPACK_COLUMN); n++;
  XtSetArg(args[n], XmNnumColumns,50); n++;
  XtSetArg(args[n], XmNorientation,XmHORIZONTAL); n++;
  XtSetArg(args[n], XmNresizable, False); n++;
  ////  Stop Motif from trying to grab another color if none are available.
  if(g.want_colormaps)  
  {    XtSetArg(args[n], XmNbackground, g.main_bcolor); n++;
       XtSetArg(args[n], XmNforeground, g.main_fcolor); n++;
  }
  radiobox = XmCreateRadioBox(button_drawing_area , (char*)"radiobox", args, n);
  s->widget[s->widgetcount++] = radiobox;

  XtManageChild(frame);
  XtManageChild(radiobox);
  XtManageChild(button_drawing_area);

  for(j=0;j<count;j++)
  {   
      n=0;
      XtSetArg(args[n], XmNheight, 20); n++;
      XtSetArg(args[n], XmNwidth, 55); n++;
      XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
      XtSetArg(args[n], XmNtopAttachment, XmATTACH_POSITION); n++;
      XtSetArg(args[n], XmNtraversalOn, True); n++;
      strcpy(tempstring, label[j]);
      len = LENGTH-strlen(tempstring);
      for(k=0;k<len;k++) strcat(tempstring," ");
      w = XmCreateToggleButton(radiobox,tempstring,args,n);
      s->widget[s->widgetcount++] = w;
      s->radiowidget[s->noofradios-1][button] = w;
      if(button==selection)
          XmToggleButtonSetState(w, True, True); 
      else
          XmToggleButtonSetState(w, False, True); 
      XtAddCallback(w, XmNvalueChangedCallback, (XtCBP)cb, (XtP)s);
      XtManageChild(w);      
      button++;
      if(button<10) s->radiono[s->noofradios-1]=button;
      else fprintf(stderr, "Error: too many radio buttons\n");
  }
  ystart += ysize;
}



//--------------------------------------------------------------------------//
// ssdisplaycb - handle radio buttons on spreadsheet                        //
//--------------------------------------------------------------------------//
void ssdisplaycb(Widget w, XtP client_data, XmACB *call_data)
{
   call_data=call_data;  // keep compiler quiet
   int i,j,group=-1,button=-1,state;
   state = XmToggleButtonGetState(w);
   Spreadsheet *s = (Spreadsheet *)client_data;

   for(i=0;i<s->noofradios;i++)
   for(j=0;j<s->radiono[i];j++)
       if(state==True && w==s->radiowidget[i][j]){ group=i; button=j; break; }

   switch(group)
   {   case 0:
            switch(button)
            {    case 0: s->display=ORIGINAL; break;
                 case 1: s->display=REAL; break;
                 case 2: s->display=IMAG; break;
                 case 3: s->display=WAVE; break;
            }
            break;
       case 1:
            switch(button)
            {    case 0: s->number_format=INTEGER; break;
                 case 1: s->number_format=HEXADECIMAL; break;
                 case 2: s->number_format=FLOAT; break;
                 case 3: s->number_format=RGBVALUE; break;
                 case 4: s->number_format=RGBHEX; break;
            }
   }
   if(state==TRUE) putimageinspreadsheet(s->ino);
}


//--------------------------------------------------------------------------//
//  putimageinspreadsheet                                                   //
//--------------------------------------------------------------------------//
void putimageinspreadsheet(int ino)
{  
#ifdef HAVE_XBAE
   int i,j,rows,cols;
   if(ino<0 || ino>=g.image_count) return;
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   rows = s->rows;
   cols = s->cols;
   for(j=0;j<rows;j++)
   for(i=0;i<cols;i++)
   {     
         pixel_text(s->text[j][i],ino,i,j);
         XbaeMatrixSetCell(s->sswidget,j,i,s->text[j][i]);
         select_cell(ino,j,i,False);
   }
   if(g.selectedimage==-1 && ino>=0)   // If a region was selected with mouse
   {    for(j=g.selected_uly-z[ino].ypos; j<g.selected_lry-z[ino].ypos; j++)
        for(i=g.selected_ulx-z[ino].xpos; i<g.selected_lrx-z[ino].xpos; i++)
             if(between(i,0,z[ino].xsize-1) && between(j,0,z[ino].ysize-1))
                 select_cell(ino,j,i,True);

   }
#else
  ino=ino;
#endif
}   



//--------------------------------------------------------------------------//
//  pixel_text - reads current string in cell x,y                           //
//--------------------------------------------------------------------------//
void pixel_text(char *result, int ino, int x, int y)
{    
   int bpp,f,v=0,rr,gg,bb,xx,yy;
   char tempstring[64];
   double v2=0;
   if(ino<0 || ino>=g.image_count){ strcpy(result,"Bkgd"); return; }
   Spreadsheet *s = z[ino].s;
   bpp = z[ino].bpp;

   ////  xx,yy=image buffer index  x,y=screen coordinate-image x,y position
   xx=x;
   yy=y;
   if(!between(xx,0,z[ino].xsize-1)|| !between(yy,0,z[ino].ysize-1)) 
   {   strcpy(result,"Bkgd"); return; }
   switch(s->display)
   {       case ORIGINAL: 
                       f = z[ino].cf;
                       v = pixelat(z[ino].image[f][yy]+xx*g.off[bpp],bpp); 
                       v2 = double(v);
                       break;
           case REAL:  if(z[ino].floatexists) v2 = z[ino].fft[yy][xx].real(); 
                       else v2=0.0;
                       v = int(v2);
                       break;
           case IMAG:  if(z[ino].floatexists) v2 = z[ino].fft[yy][xx].imag();
                       else v2=0.0;
                       v = int(v2);
                       break;
           case WAVE:  if(z[ino].waveletexists) v2 = z[ino].wavelet[yy][xx];
                       else v2=0.0;
                       v = int(v2);
                       break;
   }
   switch(s->number_format)
   {       case INTEGER:     sprintf(tempstring, "%d", v); break;
           case FLOAT:
           case DOUBLE:      sprintf(tempstring, "%g", v2); break;
           case HEXADECIMAL: sprintf(tempstring, "%0x", v); 
                             break;
           case RGBVALUE:    valuetoRGB(v,rr,gg,bb,bpp);
                             sprintf(tempstring, "%3.1d %3.1d %3.1d", rr,gg,bb); 
                             break;
           case RGBHEX:      valuetoRGB(v,rr,gg,bb,bpp);
                             sprintf(tempstring, "%2.2x %2.2x %2.2x", rr,gg,bb); 
                             break;
   }
   strcpy(result,tempstring); 
   return;
}


//--------------------------------------------------------------------------//
//  puttextinimage                                                          //
//--------------------------------------------------------------------------//
void puttextinimage(Spreadsheet *s, char *text, int x, int y)
{ 
   int bpp,ino,v=0,rr,gg,bb,xx,yy;
   double v2=0.0;
   ino = s->ino;
   if(ino<0 || ino>=g.image_count) return;
   bpp = z[ino].bpp;
   double fmax = z[ino].fmax;
   double fmin = z[ino].fmin;
   if(fmax==fmin) fmax=fmax+1.0;

   ////  x,y=col,row   xx,yy=coordinates
   xx = zoom_x_coordinate_of_index(x,ino);
   yy = zoom_y_coordinate_of_index(y,ino);
                      
   switch(s->number_format)
   {     case INTEGER:     v = atoi(text); 
                           v2 = double(v);
                           break;
         case FLOAT:
         case DOUBLE:      v2 = atof(text); 
                           v = int(v2);
                           break;
         case HEXADECIMAL: sscanf(text, "%x", &v); 
                           v2 = double(v);
                           break;
         case RGBVALUE:    sscanf(text, "%d %d %d", &rr,&gg,&bb); 
                           v = RGBvalue(rr,gg,bb,bpp);
                           v2 = double(v);
                           break;
         case RGBHEX:      sscanf(text, "%x %x %x", &rr,&gg,&bb); 
                           v = RGBvalue(rr,gg,bb,bpp);
                           v2 = double(v);
                           break;
   }
   switch(s->display)
   {     case ORIGINAL: 
                     setpixelonimage(xx,yy,v,g.imode,bpp); 
                     break;
         case REAL:  if(fmax!=fmin && z[ino].floatexists)
                     {      z[ino].fft[y][x].real() = v2;
                            v = cint((v2-fmin)/(fmax-fmin));
                            setpixel(xx,yy,v,g.imode); 
                     }
                     break;
         case IMAG:  if(fmax!=fmin && z[ino].floatexists)
                     {      z[ino].fft[y][x].imag() = v2;
                            v = cint((v2-fmin)/(fmax-fmin));
                            setpixel(xx,yy,v,g.imode); 
                     }
                     break;
         case WAVE:  if(fmax!=fmin && z[ino].waveletexists)
                     {      z[ino].wavelet[y][x] = v2;
                            v = cint((v2-fmin)/(fmax-fmin));
                            setpixel(xx,yy,v,g.imode); 
                     }
                     break;
   }
}



//--------------------------------------------------------------------------//
//  entercellcb                                                             //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void entercellcb(Widget w, XtP client_data, XBMECB *call_data)
{
   selecting = 0;
   w=w; client_data=client_data;
   int len;
   static char tempstring[FILENAMELENGTH];
   XBMECB *ptr = (XBMECB *) call_data;
   Spreadsheet *s = (Spreadsheet *)client_data;  
   int ino = s->ino;
   if(!s->created) return;
   sprintf(tempstring,"Row %d  Col %d     ",ptr->row, ptr->column);
   len = strlen(tempstring);
   XDrawImageString(g.display, s->ssform_window,g.gc,118,14,tempstring,len);
   sprintf(tempstring, "%s        ",z[ino].name);
   len = strlen(tempstring);
   XDrawImageString(g.display, s->ssform_window, g.gc, 210,14,tempstring,len);
   s->current_row = ptr->row;
   s->current_col = ptr->column;
   call_data->select_text = s->selected[ptr->row][ptr->column];
}
#endif


//--------------------------------------------------------------------------//
//  leavecellcb                                                             //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void leavecellcb(Widget w, XtP client_data, XBMLCCB *call_data)
{
   selecting = 0;
   w=w; client_data=client_data; 
   int k,oselected, owant_messages;
   Spreadsheet *s = (Spreadsheet *)client_data;  
   int ino = s->ino;
   if(!s->created) return;
   int row = s->current_row;
   int col = s->current_col;
   g.getout = 0;

   owant_messages = g.want_messages;
   if(call_data->value!=NULL && strlen(call_data->value))
   {   strcpy(s->text[row][col], call_data->value);        
       g.want_messages = 0;
       oselected = s->selected[row][col];
       select_cell(ino, row, col, False);
       if(g.getout) g.getout=0; 
       else 
       {    if(oselected) select_cell(ino, row, col, True);
            puttextinimage(s,s->text[row][col],col,row);   
            ////  Set the value of the current cell
            strcpy(call_data->value, s->text[row][col]);
            XbaeMatrixRefreshCell(s->sswidget,row,col);
       }
       g.want_messages = owant_messages;
   }

   ////  Recalculate no. of non-blank entries in column
   for(k=0;k<s->rows;k++) if(strlen(s->text[k][col])) s->maxrow[col] = 1+k;
}
#endif



//--------------------------------------------------------------------------//
//  refreshspreadsheet                                                      //
//--------------------------------------------------------------------------//
void refreshspreadsheet(int ino)
{
#ifdef HAVE_XBAE
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   putimageinspreadsheet(ino);
   XbaeMatrixRefresh(s->sswidget);
#else
   ino=ino;
#endif
}


//--------------------------------------------------------------------------//
//  update_cell - set cell to pre-evaluated number                          //
//  x,y is (screen coordinate) - (image x,y position)                       //
//--------------------------------------------------------------------------//
void update_cell(int ino, int x, int y)
{
#ifdef HAVE_XBAE
   if(ino<0) return;
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   pixel_text(s->text[y][x],ino,x,y);
   XbaeMatrixSetCell(s->sswidget,y,x,s->text[y][x]);
   XbaeMatrixRefreshCell(s->sswidget,y,x);        
#else
   ino=ino; x=x; y=y;
#endif
}


//--------------------------------------------------------------------------//
//  modifycb                                                                //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void modifycb(Widget w, XtP client_data, XBMMVCB *call_data)
{
   w=w; client_data=client_data; 
   XBMMVCB *ptr = (XBMMVCB *)call_data;
   Spreadsheet *s = (Spreadsheet *)client_data;  
   if(!s->created) return;

   XmTextVerifyCallbackStruct *xmtvcs = ptr->verify;
   XmTextBlock xmtext = xmtvcs->text;
   int insertsize = xmtext->length;          // Length of new text (1 or 0)

   int len=0, pos, row = ptr->row, col = ptr->column;
   char *newtext = xmtext->ptr;              // The new character to insert

   ////char *oldtext = (char*)ptr->prev_text;    // Previously-existing text
   //// Change suggested by  Dietmar Kunz
   char *oldtext = (char *)XbaeMatrixGetCell(w, row, col);

   char text[100];                           // Text left of insertion
   char text2[100];                          // Text right of insertion
    
   ////  There is no way to specifically detect a backspace in Xbae's
   ////  widget, so if newtext is null assume it was a backspace.
   ////  A lot of futzing around is needed to get the text while typing 
   ////  instead of after it leaves the cell.   Otherwise, if a menu is 
   ////  called while editing a cell, the most recent character will be lost.  

   if(oldtext) len = strlen(oldtext);
   pos = xmtvcs->currInsert;
   strcpy(text, oldtext);
   strcpy(text2, oldtext+pos);
   text[pos]=0;
   text2[len-pos]=0;

   if(insertsize)                            // Insert a character
   {    strcat(text,newtext);   
        strcat(text,text2);   
        pos++;
   }else                                     // Delete character at pos
   {    pos--;
        text[pos]=0;
        strcat(text,text2);
   }
   xmtvcs->currInsert = pos;
   xmtvcs->newInsert = pos;
   xmtvcs->startPos = pos-insertsize;
   xmtvcs->endPos = pos;

   puttextinimage(s,text,col,row);
   strcpy(s->text[row][col],text);
   XbaeMatrixSetCell(s->sswidget,row,col,text);
   XbaeMatrixRefreshCell(s->sswidget,row,col);        
}
#endif


//--------------------------------------------------------------------------//
//  dragcb                                                                  //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void dragcb(Widget w, XtP client_data, XBMECB *call_data)
{
   Spreadsheet *s = (Spreadsheet *)client_data;  
   int ino = s->ino;
   w=w; client_data=client_data;
   toggle_cell(ino, call_data->row, call_data->column);
   z[ino].s->column_selected[call_data->column] = 0; 
}
#endif

//--------------------------------------------------------------------------//
//  selectcb                                                                //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void selectcb(Widget w, XtP client_data, XBMSCCB *call_data)
{
   w=w; client_data=client_data;
   Spreadsheet *s = (Spreadsheet *)client_data;  
   int ino = s->ino;
   if(!z[ino].s->created) return;
   int row, col;
   row = call_data->row;
   col = call_data->column;
   toggle_cell(ino, row, col);
   z[ino].s->column_selected[col]=0; 
}
#endif


//--------------------------------------------------------------------------//
//  columnselectcb                                                          //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void columnselectcb(Widget w, XtP client_data, XBMLACB *call_data)
{
   w=w; client_data=client_data;
   Spreadsheet *s = (Spreadsheet *)client_data;  
   int ino = s->ino;
   if(!z[ino].s->created) return;
   select_column(ino, call_data->column, 1-z[ino].s->column_selected[call_data->column]);
}
#endif


//--------------------------------------------------------------------------//
//  select_column                                                           //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void select_column(int ino, int col, int value)
{
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   if(col<0 || s->maxrow[col]<=0) return;
   s->column_selected[col] = value; 
   int k;
   for(k=0; k<s->maxrow[col]; k++) select_cell(ino, k, col, value);  
}
#endif

//--------------------------------------------------------------------------//
//  select_cell                                                             //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void select_cell(int ino, int row, int col, int value)
{
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   if(value==True)
        XbaeMatrixSelectCell(s->sswidget, row, col);
   else         
        XbaeMatrixDeselectCell(s->sswidget, row, col);
   s->selected[row][col]=value;
}
#endif

//--------------------------------------------------------------------------//
//  toggle_cell                                                             //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void toggle_cell(int ino, int row, int col)
{
   static int orow=-1, ocol=-1;
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   if(selecting==0) deselectallcells(ino);
   selecting = 1;
   if (row!=orow || col!=ocol)
   {     if(s->selected[row][col]==True)
         {   XbaeMatrixDeselectCell(s->sswidget, row, col);
             s->selected[row][col] = False;
         }else
         {   XbaeMatrixSelectCell(s->sswidget, row, col);
             s->selected[row][col] = True;
         }
   }
   orow=row;
   ocol=col;
}
#endif

//--------------------------------------------------------------------------//
//  deselectallcells                                                        //
//--------------------------------------------------------------------------//
#ifdef HAVE_XBAE
void deselectallcells(int ino)
{ 
   int i,j;
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   for(j=0;j<s->cols;j++)    
   for(i=0;i<s->rows;i++)
   {     if(s->selected[i][j]==True)
         {   XbaeMatrixDeselectCell(s->sswidget, i, j);
             s->selected[i][j] = False;
         }
   }
}
#endif

//--------------------------------------------------------------------------//
//  delete_spreadsheet - deallocate all dynamic memory for the spread-      //
//  sheet. Don't delete the spreadsheet itself.                             //
//--------------------------------------------------------------------------//
void delete_spreadsheet(int ino, Widget savewidget)
{  
#ifdef HAVE_XBAE
   int j,k;
   if(g.diagnose){ printf("Deleting spreadsheet %d...",ino); fflush(stdout); }
   Spreadsheet *s = z[ino].s;
   if(!s->created) return;
   XtUnmanageChild(s->ssform);
   for(k=0;k<s->widgetcount;k++)
         if(s->widget[k]!=savewidget) XtDestroyWidget(s->widget[k]);
   delete s->widget;
   for(j=0; j<s->rows; j++)
   {    for(k=0; k<s->cols; k++) delete[] s->text[j][k];
        delete[] s->text[j];
   }
   delete[] s->text; 
         
   delete s->selected;
   delete s->selected_1d;
   delete s->column_selected;
   delete s->maxrow;
   s->visible = 0;
   s->created = 0;
   if(g.diagnose) printf("done\n");
#else
   ino=ino; savewidget=savewidget;
#endif
}


//--------------------------------------------------------------------------//
//  copy_spreadsheet                                                        //
//  z[ino1].xsize and z[ino1].ysize must be set to new values first.        //
//  ino2 = source ino1 = destination                                        //
//  Reuses source (ino2's) spreadsheet form widget.                         //
//--------------------------------------------------------------------------//
void copy_spreadsheet(int ino1, int ino2)
{
#ifdef HAVE_XBAE
   int i,j;
   if(ino1<0 || ino2<0) return;
   if(z[ino1].xsize!=z[ino2].xsize || z[ino1].ysize!=z[ino2].ysize) 
   {   fprintf(stderr,"Error at %s line %d\n",__FILE__,__LINE__); return; }
   Spreadsheet *s, *os;
   if(g.diagnose){ printf("Copy spreadsheet %d %d...",ino1,ino2); fflush(stdout); }
   os = z[ino2].s;  
   if(!os->created) return;

   if(z[ino1].s->created) delete_spreadsheet(ino1, z[ino2].s->ssform);
   s = new_spreadsheet(ino1, z[ino2].s->ssform);  
 
   for(j=0;j<s->rows;j++)
   for(i=0;i<s->cols;i++) 
   {   strcpy(s->text[j][i], os->text[j][i]);
       s->selected[j][i] = os->selected[j][i];
   }
   for(i=0;i<s->cols;i++) 
   {   s->maxrow[i] = os->maxrow[i];
       s->column_selected[i] = os->column_selected[i];
   }
   s->maxcol = os->maxcol;
   s->visible = os->visible;
   s->number_format = os->number_format;
   s->current_col = os->current_col;
   s->current_row = os->current_row;
   z[ino1].s = s;
   if(g.diagnose){ printf("...done..."); fflush(stdout); }
#else
   ino1=ino1; ino2=ino2;
#endif
}


//--------------------------------------------------------------------------//
// spreadsheet_savecb                                                       //
//--------------------------------------------------------------------------//
void spreadsheet_savecb(Widget w, XtP client_data, XmACB *call_data)
{
  w=w; client_data=client_data; call_data=call_data;
  static PromptStruct ps;
  static char ofilename[FILENAMELENGTH] = "spreadsheet.ascii";
  static char filename[FILENAMELENGTH];
  static Spreadsheet *s;
  s = (Spreadsheet *)client_data;
  strcpy(filename, ofilename);
  if(getstring("New filename:",filename,FILENAMELENGTH-1,69) != OK) return;

  ps.ptr1 = (void*)s;
  ps.filename = filename;
  ps.f1 = spreadsheet_savecb_part2;
  ps.f2 = null;
  check_overwrite_file(&ps);
}

//--------------------------------------------------------------------------//
// spreadsheet_savecb_part2                                                 //
//--------------------------------------------------------------------------//
void spreadsheet_savecb_part2(PromptStruct *ps)
{
  static int compress = 0;  // change to 1 for gzip
  char tempstring[FILENAMELENGTH];
  Spreadsheet *s = (Spreadsheet *)ps->ptr1;
  char *filename = ps->filename;
  int i, j;
  uint size;
  char answerstring[64];
  FILE *fp;
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
  {
      message("Can't create file"); 
      return; 
  }
  fprintf(fp,"Image name: %s\n",z[s->ino].name);
  fprintf(fp,"x size: %d\n",z[s->ino].xsize);
  fprintf(fp,"y size: %d\n",z[s->ino].ysize);

  for(j=0; j<s->rows; j++)
  {   for(i=0; i<s->cols; i++)
      { 
         pixel_text(answerstring, s->ino, i, j);
           fprintf(fp, "%s  ",answerstring);
      }
      fprintf(fp,"\n");
  }
  close_file(fp, compress); 
  error_message("File error", errno);
  size = filesize(filename);            // Check to ensure not an empty file
  if(size && !access(filename, F_OK))   // access = 0 if file exists
  {    sprintf(tempstring,"%d x %d spreadsheet saved\nin %s\n(size=%d bytes)",
           s->rows, s->cols, filename, size);
       message(tempstring);
  }else message("File error");
  return;
}
