//--------------------------------------------------------------------------//
// xmtnimage66.cc                                                           //
// Latest revision: 08-12-2004                                              //
// Copyright (C) 2004 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
extern PrinterStruct printer;
int label_opaque = 0;
double label_angle = 0.0;
char label_text[1024];
int label_oldci = 1;
int sp_in_unmanagecb = 0;
int incolor=0;
RGB sample[20][19];
Window sample_palette_window;

#ifdef OSX
extern "C"{
char *_nl_langinfo(int item);
char *_nl_langinfo(int item)
{
 return 0;
}
char *nl_langinfo(int item);
char *nl_langinfo(int item)
{
 return 0;
}
}
#endif

//-------------------------------------------------------------------------//
// label                                                                   //
//-------------------------------------------------------------------------//
void label(void)
{
  static Dialog *dialog;
  int j,k;
  g.getout=0; 
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  dialog = new Dialog;
  if(dialog==NULL){ message(g.nomemory); return; }
  dialog->helptopic=45;
  strcpy(dialog->title, "Label");

  ////--Radio buttons--////

  strcpy(dialog->radio[0][0],"Opaque Background");
  strcpy(dialog->radio[0][1],"On");
  strcpy(dialog->radio[0][2],"Off");
  dialog->radioset[0] = label_opaque;
  dialog->radiono[0]=3;
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
  dialog->noofradios=1;

  ////-----Boxes-----////

  strcpy(dialog->boxes[0],"Label Parameters");
  strcpy(dialog->boxes[1],"Label");             
  strcpy(dialog->boxes[2],"Rotation");             
  strcpy(dialog->boxes[3]," ");
  dialog->boxtype[0]=LABEL;
  dialog->boxtype[1]=SCROLLEDSTRING;
  dialog->boxtype[2]=DOUBLECLICKBOX;
  dialog->boxtype[3]=LABEL;
  strcpy(dialog->answer[1][0], label_text);
  dialog->boxmin[2]=0; dialog->boxmax[2] = 360;
  sprintf(dialog->answer[2][0], "%3.3f", label_angle);
  dialog->noofboxes=4;
  dialog->want_changecicb = 0;
  dialog->f1 = label_check;
  dialog->f2 = null;
  dialog->f3 = label_cancel;
  dialog->f4 = null;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = null;
  dialog->f9 = null;
  dialog->width = 0;  // calculate automatically
  dialog->height = 0; // calculate automatically
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  strcpy(dialog->path,".");
  dialog->message[0]=0;      
  dialog->busy = 0;
  dialogbox(dialog);
  return;
}


//-------------------------------------------------------------------------//
// label_cancel                                                            //
//-------------------------------------------------------------------------//
void label_cancel(dialoginfo *a)
{
   a=a;
   g.getlabelstate = 0;
   g.getout = 0;
   switchto(label_oldci);
}


//--------------------------------------------------------------------------//
//  label_check                                                             //
//--------------------------------------------------------------------------//
void label_check(dialoginfo *dialog, int radio, int box, int boxbutton)
{
   box=box;boxbutton=boxbutton;radio=radio;
   int fc,bc,ino_label,ino_rotated=0,j,k,len,w,h,direction,ascent,descent,x,y;   
   XCharStruct overall;
   RGB ck_min, ck_max;
   int ck_max_gray, ck_min_gray;
   int i,w_rotated,h_rotated,rr=0,gg=0,bb=0,hitbkg=0,obc;   
   int oulx,ouly,olrx,olry;
 
   if(radio != -2) return;   //// If user clicked Ok or Enter, continue rotating
   if(g.getout) return;

   label_oldci = ci; 
   strcpy(label_text, dialog->answer[1][0]);
   if(!strlen(label_text)) return;
   label_angle = atof(dialog->answer[2][0]);
   label_opaque = (dialog->radioset[0]==1);

   g.getout=0;
   if(g.getout){ g.getout=0; return; }

   oulx = g.selected_ulx;
   ouly = g.selected_uly;
   olrx = g.selected_lrx;
   olry = g.selected_lry;

   bc = 0; 
   fc = g.maxcolor;
   for(k=0; k<g.image_count; k++) z[k].hit=0;
   len = (int)strlen(label_text);
   XTextExtents(g.image_font_struct,label_text,len,&direction,&ascent,&descent,&overall);
   w = 20 + len*g.text_spacing + XTextWidth(g.image_font_struct,label_text,len);
   h = g.image_font_struct->ascent + g.image_font_struct->descent;
   h *= 2;

   ////  Make a hidden temporary image at screen bpp.

   crosshairs(0,0);
   if(newimage("Label",0,0,w,h,g.bitsperpixel,g.colortype,1,0,0,PERM,1,0,0)!=OK) return;
   ino_label = ci;

   for(j=0;j<h;j++) putpixelbytes(z[ci].image[0][j],bc,w,g.bitsperpixel,1);
   selectimage(ci);
   switchto(ci);

   x = ascent/3;
   y = ascent/2;
   print(label_text,x,y,fc,bc,&g.image_gc,z[ino_label].win,label_opaque,0,0,ino_label); 

   g.getlabelstate = 1;
   g.selected_ulx = 0;
   g.selected_uly = 0; 
   g.selected_lrx = w-1; 
   g.selected_lry = h-1; 
   ////  This creates a new image and changes ci, xsize, and ysize
   rotate_image(ino_label, label_angle); 
   ino_rotated = ci;
   
   obc=g.bcolor;   
   z[ci].permanent=0;
   z[ino_rotated].permanent=0;
   selectimage(ino_rotated);
   switchto(ino_rotated);
   w_rotated = z[ino_rotated].xsize;
   h_rotated = z[ino_rotated].ysize;
   array<uchar> savebpp(w_rotated+1, h_rotated+1); 
   if(!savebpp.allocated){ eraseimage(ci,0,0,1); return; }  

   z[ino_rotated].chromakey = 0;

   valuetoRGB(g.maxcolor,rr,gg,bb,g.bitsperpixel);
   ck_max.red = rr;
   ck_max.green = gg;
   ck_max.blue = bb;
   ck_min.red = 0;
   ck_min.green = 0;
   ck_min.blue = 0;
   ck_min_gray = 0;
   ck_max_gray = g.maxcolor;

   ////  Record its original bpp. 
   ////  This causes pixel depths to be converted only if their bpp is different 
   ////  from the destination.

   for(j=0;j<h_rotated;j++) 
   for(i=0;i<w_rotated;i++) 
       savebpp.p[j][i] = g.bitsperpixel;    
   repairimg(ino_rotated, 0, 0, w_rotated, h_rotated);

   printstatus(COPY);
   drawselectbox(OFF);
   g.imageatcursor=0;

   user_position_copy(ino_rotated, 0, 0, w_rotated, h_rotated, 0, ANTI_ALIAS,
       1, ck_min_gray, ck_max_gray, ck_min, ck_max, g.bitsperpixel, savebpp.p, 
       NULL, hitbkg, label_opaque);

   eraseimage(ino_rotated,0,0,1);          // Erase the temporary image bufr
   eraseimage(ino_label,0,0,1);            // Erase the temporary image bufr
   g.bcolor = obc;
   for(k=0; k<g.image_count; k++) if(z[k].hit){ rebuild_display(k); redraw(k); }
   if(hitbkg){ repairimg(-1,0,0,w_rotated,h_rotated); redrawscreen(); }
   switchto(ino_label);

   g.selected_ulx = oulx;
   g.selected_uly = ouly; 
   g.selected_lrx = olrx; 
   g.selected_lry = olry; 
   return; 
}


//-------------------------------------------------------------------------//
// select_pattern                                                          //
//-------------------------------------------------------------------------//
void select_pattern(void)
{
  int n;
  Arg args[100];
  Widget sp_drawing_area, sp_button, sp_widget;
  XmString xms;

  n=0;
  XtSetArg(args[n], XmNtransient, False); n++;
  XtSetArg(args[n], XmNresizable, False); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
  XtSetArg(args[n], XmNautoUnmanage, False); n++;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, 2); n++;
  XtSetArg(args[n], XmNwidth, 306); n++;
  XtSetArg(args[n], XmNheight, 280); n++;
  XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple((char*)"Select Pattern")); n++;
  sp_widget =  XmCreateFormDialog(g.main_widget, (char*)"SelectPatternWidget", args, n);
  XmStringFree(xms);


  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, 2); n++;
  XtSetArg(args[n], XmNwidth, 302); n++;
  XtSetArg(args[n], XmNheight, 252); n++;
  sp_drawing_area = XmCreateDrawingArea(sp_widget, (char*)"SelectPatternDrawWidget", args, n);

  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 100); n++;
  XtSetArg(args[n], XmNy, 256); n++;
  XtSetArg(args[n], XmNwidth, 100); n++;
  XtSetArg(args[n], XmNheight, 20); n++;
  sp_button = XmCreatePushButton(sp_widget, (char*)"Cancel", args, n);

  XtManageChild(sp_drawing_area);
  XtManageChild(sp_button);
  XtManageChild(sp_widget);
  XtAddCallback(sp_button, XmNactivateCallback, (XtCBP)select_pattern_unmanagecb, (XtP)NULL);
  XtAddCallback(sp_drawing_area, XmNexposeCallback, (XtCBP)select_patterncb, 0);
  XtAddEventHandler(sp_drawing_area, ButtonPressMask, False, 
      (XtEH)select_pattern_answercb, (XtP)NULL);
}


//--------------------------------------------------------------------//
//  select_patterncb                                                  //
//--------------------------------------------------------------------//
void select_patterncb(Widget widget, XtP client_data, XtP *call_data)
{
  client_data = client_data; call_data=call_data; widget=widget;
  int i,j,k,row,col,opattern;
  Window win = XtWindow(widget);
  opattern = g.draw_pattern;
  g.draw_pattern = 0;
  XSetForeground(g.display, g.image_gc, BlackPixel(g.display, g.screen));
  XSetBackground(g.display, g.image_gc, WhitePixel(g.display, g.screen));
  for(j=0;j<=250;j+=50)
      XDrawLine(g.display, win, g.image_gc, 0, j, 300, j);
  for(k=0;k<=300;k+=50)
      XDrawLine(g.display, win, g.image_gc, k, 0, k, 250);

  g.inmenu++;
  g.inpattern++;
  for(k=0;k<30;k++)
  {   row = (k * 5)/ 30;
      col = k % 6;
      g.draw_pattern = k;
      for(j=0;j<50;j++)
      for(i=0;i<50;i++)
         setpixel(i+col*50,j+row*50,0,SET,win,-3);
  }  
  g.inpattern--;
  g.inmenu--;
  g.draw_pattern = opattern;
}


//--------------------------------------------------------------------------//
// select_pattern_unmapcb                                                   //
//--------------------------------------------------------------------------//
void select_pattern_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  if(!sp_in_unmanagecb) select_pattern_unmanagecb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// select_pattern_unmanagecb - callback for "cancel" button                 //
//--------------------------------------------------------------------------//
void select_pattern_unmanagecb(Widget w, XtP client_data, XmACB *call_data)
{
   call_data=call_data; client_data=client_data; // keep compiler quiet
   sp_in_unmanagecb = 1;
   XtUnmanageChild(XtParent(w));
   sp_in_unmanagecb = 0;
}


//--------------------------------------------------------------------//
//  select_pattern_answercb                                           //
//--------------------------------------------------------------------//
void select_pattern_answercb(Widget w, XtP client_data, XEvent *event)
{
   w=w;client_data=client_data;
   int x,y,row,col;
   x = event->xmotion.x;
   y = event->xmotion.y;
   row = y / 50;
   col = x / 50;
   g.draw_pattern = row * 6 + col;
}


//--------------------------------------------------------------------//
//  update_color                                                      //
//--------------------------------------------------------------------//
void update_color(void)
{
   int x, y, c;
   getpoint(x,y);
   c = readpixel(x,y);
   if(g.last_mouse_button==1) g.fcolor = c; else g.bcolor = c;
}


//--------------------------------------------------------------------//
//  select_color                                                      //
//--------------------------------------------------------------------//
void select_color(void)
{
  int n;
  Arg args[100];
  Widget sc_drawing_area, sc_button, sc_widget;
  XmString xms;

  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, 2); n++;
  XtSetArg(args[n], XmNwidth, 300); n++;
  XtSetArg(args[n], XmNheight, 348); n++;
  XtSetArg(args[n], XmNdialogTitle, xms=XmStringCreateSimple((char*)"Select Color")); n++;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
  sc_widget =  XmCreateFormDialog(g.main_widget, (char*)"SelectColorWidget", args, n);
  XmStringFree(xms);


  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 2); n++;
  XtSetArg(args[n], XmNy, 2); n++;
  XtSetArg(args[n], XmNwidth, 288); n++;
  XtSetArg(args[n], XmNheight, 304); n++;
  sc_drawing_area = XmCreateDrawingArea(sc_widget, (char*)"SelectColorDrawWidget", args, n);

  n=0;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNx, 90); n++;
  XtSetArg(args[n], XmNy, 313); n++;
  XtSetArg(args[n], XmNwidth, 100); n++;
  XtSetArg(args[n], XmNheight, 20); n++;
  sc_button = XmCreatePushButton(sc_widget, (char*)"Cancel", args, n);

  XtManageChild(sc_drawing_area);
  XtManageChild(sc_button);
  XtManageChild(sc_widget);
  XtAddCallback(sc_button, XmNactivateCallback, (XtCBP)select_color_unmanagecb, (XtP)NULL);
  XtAddCallback(sc_drawing_area, XmNexposeCallback, (XtCBP)select_colorcb, 0);
  XtAddEventHandler(sc_drawing_area, ButtonPressMask | PointerMotionMask, False, 
      (XtEH)select_color_answercb, (XtP)NULL);

}


//--------------------------------------------------------------------//
//  select_colorcb                                                    //
//--------------------------------------------------------------------//
void select_colorcb(Widget widget, XtP client_data, XtP *call_data)
{
  client_data = client_data; call_data=call_data; widget=widget;
  XmDrawingAreaCallbackStruct *ptr;
  XEvent event;
  XExposeEvent expose_event;
  ptr = (XmDrawingAreaCallbackStruct*) call_data;
  static int hit = 0;
  int i,j,y;
  int ex, ey, ew, eh;
  int x1=0,y1=0,x2=18,y2=19;
  Window win = XtWindow(widget);
  g.inmenu++;
  if(incolor) return;
  incolor++;

  y = 0;
  if(ptr==NULL){ incolor--; return; }
  switch(ptr->reason)
  {  
     case XmCR_EXPOSE:
       event = *(ptr->event);
       expose_event = event.xexpose;
       ex = ptr->event->xexpose.x;
       ey = ptr->event->xexpose.y;
       ew = ptr->event->xexpose.width;
       eh = ptr->event->xexpose.height;
       break;
     default: incolor--; return;
  }
  x1 = max(0,ex/16);
  y1 = max(0,ey/16);
  x2 = min(18, ew/16 + x1 + 1);
  y2 = min(19, eh/16 + y1 + 1);
  if(!hit)
  {
     //// gray
     for(i=0;i<=18;i++) 
     {   sample[0][i].red   = min(255, i*16);  
         sample[0][i].green = min(255, i*16);  
         sample[0][i].blue  = min(255, i*16); 
     }
     //// red to blue
     for(j=1;j<=9;j++)
     for(i=0;i<=8;i++)  
     {   sample[j][i].red   = min(255, (j-1)*32);  
         sample[j][i].green = min(255, i*32);  
         sample[j][i].blue  = min(255, i*32); 
     }

     //// green to magenta
     for(j=10;j<=19;j++)
     for(i=0; i<=8;i++)  
     {   sample[j][i].red   = min(255, i*32);  
         sample[j][i].green = min(255, (j-10)*32);  
         sample[j][i].blue  = min(255, i*32); 
     }

     //// blue to yellow
     for(j=1; j<=9;j++)
     for(i=9; i<=18;i++) 
     {   sample[j][i].red   = min(255, (i-9)*32);  
         sample[j][i].green = min(255, (i-9)*32);  
         sample[j][i].blue  = min(255, (j-1)*32);
     }

     //// 
     for(j=10;j<=19;j++)
     for(i=9; i<=18;i++) 
     {  
         sample[j][i].red   = (i-9)*24;  
         sample[j][i].green = (i-9)*24;  
         sample[j][i].blue  = (i-9)*24;  

         sample[j][i].red   += (j-10)*7;  
         sample[j][i].green += (18-i)*7;  
         sample[j][i].blue  += (19-j)*7;  

         //sample[j][i].red   = min(255, sample[j][i].red);
         //sample[j][i].green = min(255, sample[j][i].green);
         //sample[j][i].blue  = min(255, sample[j][i].blue);
     }

//     for(i=8;i<16;i++) { sample[j][i].red = 64+(j-8)*8;  sample[j][i].green = 64+(i-8)*8;  sample[j][i].blue = (j-8)*8; }
//     for(j=10;j<=19;j++)
//     for(i=9; i<=18;i++) { sample[j][i].red   = random_color();
//                           sample[j][i].green = random_color();
//                           sample[j][i].blue  = random_color(); }
     hit = 1;
     sample_palette_window = win;
  }
  for(j=y1;j<=y2;j++)
  for(i=x1;i<=x2;i++)
      draw_sample_color(win, sample[j][i].red, sample[j][i].green, sample[j][i].blue, i*16, j*16);
  incolor--;
  g.inmenu--;
}


//--------------------------------------------------------------------------//
//  random_color - create browns and mixed colors                           //
//--------------------------------------------------------------------------//
int random_color(void)
{
  int i,k,j=16;
  k  = j*cint((rand()/(RAND_MAX+1.0)));
  for(i=0;i<=128/j;i++)
     k += j*cint((rand()/(RAND_MAX+1.0)));
  k = cint((double)k*1.5);
  return k;
}


//--------------------------------------------------------------------------//
//  draw_sample_color                                                       //
//--------------------------------------------------------------------------//
void draw_sample_color(Window win, int rr, int gg, int bb, int x, int y)
{
  int i,j,color;
  color = RGBvalue(rr,gg,bb,g.bitsperpixel);
  for(j=y;j<y+16;j++)
  for(i=x;i<x+16;i++)
         setpixel(i,j,color,SET,win,-3);
}


//--------------------------------------------------------------------------//
// select_color_unmapcb                                                     //
//--------------------------------------------------------------------------//
void select_color_unmapcb(Widget w, XtP client_data, XmACB *call_data)
{
  select_color_unmanagecb(w, client_data, call_data);
}


//--------------------------------------------------------------------------//
// select_color_unmanagecb - callback for "cancel" button                   //
//--------------------------------------------------------------------------//
void select_color_unmanagecb(Widget w, XtP client_data, XmACB *call_data)
{
   call_data=call_data; client_data=client_data; // keep compiler quiet
   XtUnmanageChild(XtParent(w));
}


//--------------------------------------------------------------------//
//  select_color_answercb                                             //
//--------------------------------------------------------------------//
void select_color_answercb(Widget w, XtP client_data, XEvent *event)
{
   w=w;client_data=client_data;
   int x,y,button,row,col,color;
   x = event->xmotion.x;
   y = event->xmotion.y;
   button  = event->xbutton.button;
   row = y / 16;
   col = x / 16;
   color = RGBvalue(sample[row][col].red, sample[row][col].green, sample[row][col].blue, g.bitsperpixel);
   if(button == 1) g.fcolor = color;
   if(button == 2) g.bcolor = color;
   if(button == 3) g.bcolor = color;
   printcoordinates(col, row, 2);   
}
