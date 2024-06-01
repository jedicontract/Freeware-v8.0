//--------------------------------------------------------------------------//
// xmtnimage8.cc                                                            //
// curve, Bezier, linear regression                                         //
// Latest revision: 03-17-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
 
extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
extern int in_calibrate;

int X,Y;
uint bezier_mask = 
                   KeyPressMask      |
                   KeyReleaseMask    |
                   ButtonPressMask   |
                   ButtonReleaseMask |
                   Button1MotionMask |      // Click & Drag
                   PointerMotionMask;       // Mouse movement  

static int curve_width=50;
static int want_bezier_arrow = 0;

//--------------------------------------------------------------------------//
// beziercb - Normal-style event callback for Bezier curves.                //
//--------------------------------------------------------------------------//
void beziercb(Widget w, XtP client_data, XEvent *event)
{
  static int n1=0;          // The current data point
  static int ox=0,oy=0;
  g.state = CURVE;
  int bpp, ino, key, n, v;
  XYData *data = (XYData *)client_data;
  int *x = data->x;
  int *y = data->y;
  double law = g.line.arrow_width;
  double lai = g.line.arrow_inner_length;
  double lao = g.line.arrow_outer_length;
  n  = data->n;             // The total no. of data points
  g.mouse_x = event->xbutton.x;
  g.mouse_y = event->xbutton.y;
  ino = whichimage(w,bpp);  // Returns -1 or image no.
  if(ino<0) return;
  if(!in_calibrate){ g.mouse_x += z[ino].xpos; g.mouse_y += z[ino].ypos; }
  data->ino = ino;

  if(g.getout)
  {    bezier_curve_end(data);
       return;
  }
 
  g.inmenu++;
  switch(event->type)
  {  
       case Expose:     
          if(event->xexpose.count==0) printf("Exposed!\n");
          break;
       case MotionNotify:
          printcoordinates(g.mouse_x, g.mouse_y, 0); 
          if( n && n1>=0 && data->type==9 && (ox!=g.mouse_x || oy!=g.mouse_y)) 
          {   line(x[n1], y[n1], ox, oy, 0, XOR);
              line(x[n1], y[n1], g.mouse_x, g.mouse_y, 0, XOR);
          }
          ox = g.mouse_x;
          oy = g.mouse_y;
          g.ignore = 0;
          if(g.floating_magnifier) update_floating_magnifier(g.mouse_x, g.mouse_y);
          g.ignore = 1;
          if(g.mouse_button != 0)                    // Dragging the mouse 
          {  
               if(data->type != 9)
               {   draw_boxes(data);
                   draw_curve(data, XOR, OFF);                                // off
                   x[n1] = g.mouse_x;
                   y[n1] = g.mouse_y;
                   draw_curve(data, XOR, ON);                                 // on
                   draw_boxes(data);
               }
          }

          break;
       case ButtonPress: 
          //// Recalculate imageatcursor so it doesn't deselect the image
          g.imageatcursor = whichimage(g.mouse_x, g.mouse_y, bpp);
          g.mouse_button  = (int)event->xbutton.button;
          draw_boxes(data);
          draw_curve(data, XOR, OFF);
          n1 = nextpoint(data, g.mouse_x, g.mouse_y);  // Test if at existing point
          data->n = max(n,n1+1);
          n = data->n;
          x[n1] = g.mouse_x;
          y[n1] = g.mouse_y;
          draw_curve(data, XOR, ON);                                      // on
          draw_boxes(data);
          break;
       case ButtonRelease: 
          if(g.mouse_button == 3 || g.mouse_button == 2)
          {   if(data->duration !=NOEND) bezier_curve_end(data);
          }else
          {   g.mouse_button = 0;
              draw_boxes(data);
              draw_curve(data, XOR, OFF);                                     // off
              x[n1] = g.mouse_x;
              y[n1] = g.mouse_y;
              draw_curve(data, XOR, ON);                                      // on
              draw_boxes(data);
              if(n1>=data->nmax-1)
                   XSetForeground(g.display, g.image_gc, g.fcolor);
          }
          break;
       case KeyPress:
          key = which_key_pressed(event);
          switch(key)
          {   case XK_Escape:
              case XK_Linefeed:
              case XK_Return:
              case XK_KP_Space:
              case XK_space:
                 if(key==XK_Escape) g.getout = 1; // for getting out of measurement mode
                 XSetForeground(g.display, g.image_gc, g.fcolor);
                 if(data->duration !=NOEND) bezier_curve_end(data);
              
                 if(want_bezier_arrow)
                 {   n = data->n;
                     if(n>1)
                     {    v = g.line.width;
                          g.line.arrow_width = v * 6.0;
                          g.line.arrow_outer_length = v * 9.0;
                          g.line.arrow_inner_length = v * 6.0;
                          g.line.color = g.fcolor;
                          g.inmenu--;
                          draw_arrow(data->x[n-1], data->y[n-1], data->x[n-2], data->y[n-2], &g.line);
                          g.line.arrow_width=law;
                          g.line.arrow_inner_length=lai;
                          g.line.arrow_outer_length=lao;
                          g.inmenu++;
                     }
                 }
                 break;
              default: break;
          }
       default:
          break;
  }
  g.inmenu--;
}


//--------------------------------------------------------------------------//
// bezier_curve_start = Draws Bezier curve without declaring arrays.        //
// This is the main entry point for drawing a curve without using dialog box//
// To draw a Bezier curve, using dialog box, use curve().                   //
//     Draws Bezier curve using Bernstein blend method                      //
//     Type 0 = draw Bezier curve                                           //
//     Type 1 = draw B-spline curve                                         //
//     Type 2 = draw Lin.reg.line                                           //
//     Type 3 = draw Polygon                                                //
//     Type 4 = draw Trapezoid (limit must be 4).                           //
//     Type 5 = draw Trapezoid, snap sides to vertical or horizontal        //
//     Type 6 = draw Rectangle:  User selects 2 points, but 4 points        //
//                are returned with the 1st 2 and last 2 points 'width'     //
//                pixels apart.                                             //
//     Type 7 = draw Polygon except don't close figure                      //
//     Type 8 = sketch                                                      //
//     Type 9 = point to point                                              //
//     duration = TEMP  -  erase bezier before returning                    //
//     duration = PERM  -  leave bezier on screen                           //
//     duration = NOEND -  caller will call bezier_curve_end                //
//   data->x, and data->y must be allocated and data->nmax must contain     //
//   their sizes before this function is called.                            //
//--------------------------------------------------------------------------//
void bezier_curve_start(XYData *data, dialoginfo *a)
{ 
   int k;
   char tempstring[FILENAMELENGTH];
   char tempstring2[128];
   g.ignore = 1; // prevent double clicking
   if(data->nmin <=0)
      sprintf(tempstring2, "up to %d", data->nmax);
   else if(data->nmax > data->nmin)
      sprintf(tempstring2, "at least %d", data->nmin);
   else
      sprintf(tempstring2, "%d", data->nmin);
   sprintf(tempstring, "Select %s control points, \n press Esc when finished...",tempstring2);
   if(data->wantmessage)
   {   if(a==NULL)
          message(tempstring);
       else
          dialog_message(a->message_widget, tempstring);
   }
   XtRemoveEventHandler(g.drawing_area, g.mouse_mask, False, (XtEH)mousecb, (XtP)&g.image_gc);
   XtAddEventHandler(g.drawing_area, bezier_mask, False, (XtEH)beziercb, (XtP)data);
   for(k=0; k<g.image_count; k++)
   {   XtRemoveEventHandler(z[k].widget,g.mouse_mask,False,(XtEH)mousecb,(XtP)&z);
       XtAddEventHandler(z[k].widget,bezier_mask,False,(XtEH)beziercb,(XtP)data);
   }
   g.bezier_state = CURVE;
   g.state = CURVE;
}
 

//--------------------------------------------------------------------------//
// bezier_curve_end  - Resets the callback to original state. Don't forget  // 
// to deallocate the memory (data.x and y).                                 //
//--------------------------------------------------------------------------//
void bezier_curve_end(XYData *data)
{
   int k;  
   draw_boxes(data);                        // off
   draw_curve(data, XOR, OFF);              // off
   g.inmenu--;
   if(data->duration==PERM) draw_curve(data, SET, ON);
   g.inmenu++;

   // Add the old callbacks back

   XtRemoveEventHandler(g.drawing_area, bezier_mask, False, (XtEH)beziercb,(XtP)data);
   XtAddEventHandler(g.drawing_area,g.mouse_mask,False,(XtEH)mousecb,(XtP)&g.image_gc);

   for(k=0; k<g.image_count; k++)
   {   XtRemoveEventHandler(z[k].widget, bezier_mask, False, (XtEH)beziercb, (XtP)data);
       XtAddEventHandler(z[k].widget, g.mouse_mask, False, (XtEH)mousecb, (XtP)&z);
   }
   g.bezier_state=NORMAL;
   g.state=NORMAL;
   g.ignore = 0; // re-enable double clicking
}



//--------------------------------------------------------------------------//
// nextpoint - returns closest existing point, or n if none close enough.   //
//--------------------------------------------------------------------------//
int nextpoint(XYData *data, int xtest, int ytest)
{
   int k;
   int n  = data->n;
   int *x = data->x;
   int *y = data->y;
   if(n>=0)
   {  for(k=0;k<n;k++)
      {  if(insidebox(xtest,ytest,x[k]-5,y[k]-5,x[k]+5,y[k]+5))
            return k;
      }
   }   
   return n;
}


//--------------------------------------------------------------------------//
//  curve                                                                   //
//  Draws Bezier, B-spline, or linear regression curve                      //
//  This is the main entry point for drawing a curve using a dialog box.    //
//  Use bezier_curve_start() when not using dialog box.                     //
//--------------------------------------------------------------------------//
void curve(void)
{
   static XYData *data;
   const int linreg_order=1;
   data = new XYData;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   data->type = g.curve_type;
   data->order = linreg_order;
   data->width = curve_width;
   data->n    = 0;
   data->nmin = 0;
   data->nmax = 300;
   data->duration = PERM;
   data->wantpause = 0;
   data->wantmessage = 1;
   data->win       = 0;  // Draw on main drawing_area or image
   data->x = new int[data->nmax];  
   if(data->x==NULL){ message(g.nomemory,ERROR); return; }
   data->y = new int[data->nmax];  
   if(data->y==NULL){ delete[] data->x; message(g.nomemory,ERROR); return; }

   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return; }
   int helptopic=23,j,k;
   strcpy(dialog->title,"Draw curve");
   strcpy(dialog->radio[0][0],"Curve Type");             
   strcpy(dialog->radio[0][1],"Bezier");
   strcpy(dialog->radio[0][2],"B-spline");
   strcpy(dialog->radio[0][3],"Regression line"); 
   strcpy(dialog->radio[0][4],"Polygon");             
   strcpy(dialog->radio[0][5],"Trapezoid");             
   strcpy(dialog->radio[0][6],"Snap Trapezoid");             
   strcpy(dialog->radio[0][7],"Fixed-width Rectangle");             
   strcpy(dialog->radio[0][8],"Open polygon");             
   strcpy(dialog->radio[0][9],"Sketch");             
   strcpy(dialog->radio[0][10],"Point to point polygon");             

   dialog->radioset[0] = g.curve_type+1;
   dialog->radiono[0]=11;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

   strcpy(dialog->boxes[0],"Rect.Width");
   dialog->boxtype[0]=LABEL;

   strcpy(dialog->boxes[1],"Width");             
   dialog->boxtype[1] = INTCLICKBOX;
   dialog->boxmin[1]=0; dialog->boxmax[1] = 200;
   sprintf(dialog->answer[1][0], "%d", curve_width);

   strcpy(dialog->boxes[2],"Line Width");
   dialog->boxtype[2]=LABEL;

   strcpy(dialog->boxes[3],"Width");             
   dialog->boxtype[3] = INTCLICKBOX;
   dialog->boxmin[3]=0; dialog->boxmax[3] = 200;
   sprintf(dialog->answer[3][0], "%d", g.line.width);

   strcpy(dialog->boxes[4],"Arrow");
   dialog->boxtype[4] = TOGGLE;
   dialog->boxset[4] = want_bezier_arrow;

   strcpy(dialog->boxes[5],"Antialiased");
   dialog->boxtype[5] = TOGGLE;
   dialog->boxset[5] = g.line.antialias;

   dialog->spacing = 0;
   dialog->radiostart = 2;
   dialog->radiowidth = 50;
   dialog->boxstart = 2;
   dialog->boxwidth = 50;
   dialog->labelstart = 87;
   dialog->labelwidth = 50;        
   dialog->boxxy[0][0] = 6;  dialog->boxxy[0][1] = 240;    // Parameters label
   dialog->boxxy[1][0] = 86; dialog->boxxy[1][1] = 240;    // Parameters label
   dialog->boxxy[2][0] = 6;  dialog->boxxy[2][1] = 260;    
   dialog->boxxy[3][0] = 86; dialog->boxxy[3][1] = 260;    
   dialog->boxxy[4][0] = 6;  dialog->boxxy[4][1] = 280;    
   dialog->boxxy[5][0] = 6;  dialog->boxxy[5][1] = 300;    
   dialog->width = 170;
   dialog->height = 345;

   dialog->noofradios=1;
   dialog->noofboxes=6;
   dialog->helptopic=helptopic;  
   dialog->want_changecicb = 0;
   dialog->f1 = curvecheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = curvefinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 1;
   dialog->ptr[0] = data;
   strcpy(dialog->path,".");
   dialog->message[0]=0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  curvecheck                                                              //
//--------------------------------------------------------------------------//
void curvecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   radio=radio; box=box; boxbutton=boxbutton;
   g.curve_type = a->radioset[0]-1;
   XYData *data = (XYData*)a->ptr[0];
   data->n = 0;
   data->width = atoi(a->answer[1][0]);
   g.line.width = atoi(a->answer[3][0]);
   want_bezier_arrow  = a->boxset[4];
   g.line.antialias = a->boxset[5];
   g.getout = 0;
   if(radio == -2) 
   {   data->type = g.curve_type;
       bezier_curve_start(data, NULL); // User clicked Ok or Enter
   }
}


//--------------------------------------------------------------------------//
//  curvefinish                                                             //
//--------------------------------------------------------------------------//
void curvefinish(dialoginfo *a)
{
   a=a;
   XYData *data = (XYData*)a->ptr[0];
   delete[] data->x;
   delete[] data->y;
   delete[] data;
}


//--------------------------------------------------------------------------//
// draw_curve   -  called by beziercb                                       //
// mode is pixel interaction function, e.g. XOR.                            //
// For Bezier curves, the curve is redrawn with mode=-1 which puts the data //
// in array 'highest'.                                                      //
// 'modifiable' should be set to 0 when erasing a previously-drawn curve to //
//   prevent accidentally changing the vertices before erasing is finished. //
//--------------------------------------------------------------------------//
void draw_curve(XYData *data, int mode, int modifiable)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 

   //  Type 0 = draw Bezier curve  
   //  Type 1 = draw B-spline curve
   //  Type 2 = draw Lin.reg.line
   //  Type 3 = draw Polygon
   //  Type 4 = draw Trapezoid (limit must be 4). 
   //  Type 5 = draw Trapezoid, snap sides to vertical or horizontal
   //  Type 6 = draw Rectangle:  User selects 2 points, but 4 points
   //  Type 7 = draw Polygon except don't close figure
   //  Type 8 = sketch
   //  Type 9 = draw point to point
   int c;
   if(mode==SET) c=g.fcolor; else c=g.maxcolor;   

   switch(data->type)
   {  case 0: drawbezier(data, c, mode); drawbezier(data, c, -1); break;
      case 1: drawbspline(data, c, mode); drawbspline(data, c, -1); break;
      case 2: drawregressionline(data, c, mode); break;
      case 3: drawpolygon(data, c, mode); break;
      case 4: drawtrapezoid(data, c, mode, modifiable); break;
      case 5: drawsnaptrapezoid(data, c, mode, modifiable); break;
      case 6: drawrectangle(data, c, mode); break;
      case 7: drawopenpolygon(data, c, mode); break;
      case 8: drawsketch(data, c, mode); break;
      case 9: draw_point_to_point_curve(data, c, mode); break;
   }
} 


//--------------------------------------------------------------------------//
// drawsketch - called by draw_curve                                        //
//--------------------------------------------------------------------------//
void drawsketch(XYData *data, int color, int mode)
{
    int k;
    int n = data->n;
    int *x = data->x;
    int *y = data->y;
    for(k=1;k<n;k++)
       line(x[k-1],y[k-1],x[k],y[k],color,mode,data->win);
    for(k=1;k<n;k++)
       line(x[k-1],y[k-1],x[k],y[k],color,TRACE,data->win);
}



//--------------------------------------------------------------------------//
// drawpolygon - called by draw_curve                                       //
//--------------------------------------------------------------------------//
void drawpolygon(XYData *data, int color, int mode)
{   
    int k;
    int n = data->n;
    int *x = data->x;
    int *y = data->y;
    int a[1024];  // Fix unknown problem, something odd happening somewhere
    a[0]=0;
    if(n>1) for(k=1;k<n;k++)
       line(x[k-1],y[k-1],x[k],y[k],color,mode,data->win);
    if(n>2)
       line(x[0], y[0], x[n-1], y[n-1],color,mode,data->win);
}


//--------------------------------------------------------------------------//
// drawopenpolygon - called by draw_curve                                   //
//--------------------------------------------------------------------------//
void drawopenpolygon(XYData *data, int color, int mode)
{   int k;
    int n = data->n;
    int *x = data->x;
    int *y = data->y;
    for(k=1;k<n;k++)
       line(x[k-1],y[k-1],x[k],y[k],color,mode,data->win);
}



//--------------------------------------------------------------------------//
// drawrectangle - called by draw_curve                                     //
// Fixed-width rectangle                                                    //
//--------------------------------------------------------------------------//
void drawrectangle(XYData *data, int color, int mode)
{
                                  // Rectangle for fixed-width scan
                                  // Temporarily change the coordinates
                                  // to show the region. Then change
                                  // back to the 2 original coordinates.
  int idx,idy;
  int ox1,oy1,ox2,oy2;
  double m,dx,dy,w;

  w = (double)data->width;         // Width
  if(data->n < 2) return;     
  data->n = 2;                     // Ensure only 2 points 

  ox1=data->x[0];  oy1=data->y[0];
  ox2=data->x[1];  oy2=data->y[1];

  if(data->y[1]!=data->y[0]) 
       m = (double)(data->x[1]-data->x[0])/(double)(data->y[1]-data->y[0]);
  else m=1e6;

  dx = sqrt( (double)(w*w)/(double)(1+m*m) );   
  idx=(int)(0.5+dx);              // Integer must be used here
  if(idx<0) idx=min(idx,-1);      // y coords must be different
  else  idx=max(idx,1);
  dy = -dx*m;
  idy=(int)dy;

  data->x[0] = ox1-idx;  data->y[0] = oy1-idy;
  data->x[1] = ox2-idx;  data->y[1] = oy2-idy;
  data->x[3] = ox1+idx;  data->y[3] = oy1+idy;
  data->x[2] = ox2+idx;  data->y[2] = oy2+idy;
  data->n = 4;

  drawpolygon(data, color, mode);

  data->x[0]=ox1; data->y[0]=oy1;   // Restore original coordinates
  data->x[1]=ox2; data->y[1]=oy2;
  data->n = 2;

}


//--------------------------------------------------------------------------//
// draw_boxes                                                               //
//--------------------------------------------------------------------------//
void draw_boxes(XYData *data)
{  
   int k;
   int dx = z[ci].xpos;
   int dy = z[ci].ypos;
   int color = cint(0.9*g.maxcolor-1);
   for(k=0;k<data->n;k++)               
   {  if(in_calibrate)
            box(data->x[k]-5+dx, data->y[k]-5+dy, data->x[k]+5+dx, data->y[k]+5+dy,
                 color, XOR, data->win);  
      else
      {    if(data->type == 9)
                box(data->x[k], data->y[k], data->x[k], data->y[k],
                    color, XOR, data->win);  
           else
                box(data->x[k]-5, data->y[k]-5, data->x[k]+5, data->y[k]+5,
                    color, XOR, data->win);  
      }
   }
}


//--------------------------------------------------------------------------//
// drawtrapezoid  -  called by draw_curve()                                 //
// Can change data->x[] and data->y[] if 'modifiable' is nonzero. This is   //
// because the coordinates might not fit a trapezoid. Set 'modifiable' to   //
// 0 when only erasing an old curve.                                        //
//--------------------------------------------------------------------------//
void drawtrapezoid(XYData *data, int color, int mode, int modifiable)
{
   // Force slope of segment 3--4 parallel to 1--2  otherwise some pixels   
   // will be oversampled. This is done by calculating the intersection of  
   // 2--4 and the line from 3 parallel to 1--2 and setting 4 to this value.
  
  int x1,x2,x3,x4,y1,y2,y3,y4;
  double b24,b35,m12=1000.0,m24=1000.0;
  x1 = data->x[0]; y1 = data->y[0];
  x2 = data->x[1]; y2 = data->y[1];
  x3 = data->x[2]; y3 = data->y[2];
  x4 = data->x[3]; y4 = data->y[3];
  if(data->n >= 4 && modifiable)
  {   data->n = 4;                              // Ensure only 4 sides
      if(x4!=x2) m24 = ((double)y4-(double)y2) / ((double)x4-(double)x2); 
      if(x1!=x2) m12 = ((double)y2-(double)y1) / ((double)x2-(double)x1);
      b24 = (double)y2 - m24*(double)x2;
      b35 = (double)y3 - m12*(double)x3;
      if(m24!=m12) x4 = (int)(0.5000001 + (b24-b35)/(m12-m24));
      y4 = (int)(m12*(double)x4 + b35 + 0.5000001);
      data->x[3] = x4;
      data->y[3] = y4;
  }
  drawpolygon(data, color, mode);
}
   

//--------------------------------------------------------------------------//
// drawsnaptrapezoid  - called by draw_curve()                              //
// Can change data->x[] and data->y[] if 'modifiable' is nonzero. This is   //
// because the coordinates might not fit a trapezoid. Set 'modifiable' to   //
// 0 when only erasing an old curve.                                        //
//--------------------------------------------------------------------------//
void drawsnaptrapezoid(XYData *data, int color, int mode, int modifiable)
{ 
                      // Trapezoid with vertical or horizontal sizes
                      // Snap into vertical or horizontal
   int x1,x2,x3,x4,y1,y2,y3,y4,lefttoright,toptobottom;
   if(data->n >= 4 && modifiable)
   {   data->n=4;     // Ensure only 4 sides
       x1 = data->x[0]; y1 = data->y[0];
       x2 = data->x[1]; y2 = data->y[1];
       x3 = data->x[2]; y3 = data->y[2];
       x4 = data->x[3]; y4 = data->y[3];

       if((x1+x2)/2 < (x3+x4)/2) lefttoright=1; else lefttoright=0;
       if((y1+y2)/2 < (y3+y4)/2) toptobottom=1; else toptobottom=0;
       if(abs(y2-y1)>abs(x2-x1))
       {   if(lefttoright)
           { x2=min(x1,x2); x1=x2;
             x4=max(x3,x4); x3=x4;
           }else  
           { x2=max(x1,x2); x1=x2;
             x4=min(x3,x4); x3=x4;
           }
           if(y2<y1) swap(y2,y1);
           if(y4<y3) swap(y4,y3);
       }else 
       {   if(toptobottom)
           { y2=min(y1,y2); y1=y2;
             y4=max(y3,y4); y3=y4; 
           }else
           { y2=max(y1,y2); y1=y2;
             y4=min(y3,y4); y3=y4; 
           }
           if(x2>x1) swap(x2,x1);
           if(x4>x3) swap(x4,x3);
       }
       data->x[1]=x1; data->y[1]=y1;
       data->x[0]=x2; data->y[0]=y2;
       data->x[2]=x3; data->y[2]=y3;
       data->x[3]=x4; data->y[3]=y4;
   }
   drawpolygon(data, color, mode);
}



//--------------------------------------------------------------------------//
//   drawbezier() = Draws Bezier curve using Bernstein polynomials.         //
//   Called by draw_curve().                                                //
//--------------------------------------------------------------------------//
void drawbezier(XYData *data, uint color, int mode)
{
    int i,j,k,count;
    double x,y,b,t;
    int oantialias = g.line.antialias;
    int ocf = g.circle_fill;
    int ogt = g.gradient1_type;
    int ogo = g.gradient1_outer;
    int oco = g.circle_outline;
    double increment;
    
    if(g.line.antialias) increment = 0.03;
    else increment = 0.001;
    count = data->n - 1;
    X = data->x[0];
    Y = data->y[0];
    if(count<1) return;

    //// if XOR, temporarily turn of antialiasing
    if(g.line.antialias && mode!=SET)
    {     oantialias = g.line.antialias;
          g.line.antialias = 0;
    }
    for (t=0.0; t<=1.0; t+=increment)
    {   x = 0;
        y = 0;
        for (j=0; j<=count; j++)
        {   b = 1.0;
            for (k=count; k>j; k--)   b*=k;
            for (k=count-j; k>1; k--) b/=k;
            for (i=1; i<=j; i++)  b*=t;
            for (i=1; i<=count-j; i++) b*=(1-t);
            x += data->x[j] * b;
            y += data->y[j] * b;
        }
        if(mode==SET && !g.line.antialias) 
        {   g.circle_fill=1;
            g.circle_outline=0;
            g.gradient1_type= 2;
            g.gradient1_outer = color;
            circle((int)x, (int)y, g.line.width, 1, color, mode);
            g.circle_fill=ocf;
            g.gradient1_type=ogt;
            g.gradient1_outer=ogo;
            g.circle_outline=oco;
        }else if(mode==SET) 
        {   lineto((int)x, (int)y, color, mode, data->win);
            g.circle_fill=1;
            g.circle_outline=0;
            g.gradient1_type= 2;
            g.gradient1_outer = color;
            g.circle_antialias++;
            if(t!=1.0) circle((int)x, (int)y, cint(g.line.width), 1, color, mode);
            g.circle_antialias--;
            g.circle_fill=ocf;
            g.gradient1_type=ogt;
            g.gradient1_outer=ogo;
            g.circle_outline=oco;
        }else
            lineto((int)x, (int)y, color, mode, data->win);

    }
    g.line.antialias = oantialias;
}


//--------------------------------------------------------------------------//
// drawbspline() = Draws a B-Spline curve with multi-point inputs.          //
//   Called by draw_curve().                                                //
//--------------------------------------------------------------------------//
void drawbspline(XYData *data, uint color, int mode)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   int i,k,count;
   int *xpoint, *ypoint;
   double x,y;
   double t,nc1,nc2,nc3,nc4;
   int oantialias = g.line.antialias;
   count = data->n + 1;
   if(count<1)return;
   if(count>=data->nmax-4) count=data->nmax-4;
   xpoint = new int[data->nmax];
   ypoint = new int[data->nmax];
   for(k=0;k<data->nmax-1;k++){  xpoint[k+1]=data->x[k]; ypoint[k+1]=data->y[k];  }
   for(k=count+1;k<data->nmax;k++){  xpoint[k]=0; ypoint[k]=0;  }

   xpoint[0]=xpoint[1];
   ypoint[0]=ypoint[1];
   X = xpoint[0];
   Y = ypoint[0];
   xpoint[count]=xpoint[count-1];
   ypoint[count]=ypoint[count-1];
   xpoint[count+1]=xpoint[count];
   ypoint[count+1]=ypoint[count];
   xpoint[count+2]=xpoint[count];
   ypoint[count+2]=ypoint[count];

   //// if XOR, temporarily turn of antialiasing
   if(g.line.antialias && mode==XOR)
   {
          oantialias = g.line.antialias;
          g.line.antialias = 0;
   }
   for(i=0; i<count; i++)
   {   for (t=0.0; t<=1.0; t+=0.01)
       {   nc1=-(t*t*t/6)+t*t/2-t/2+1.0/6;
           nc2=t*t*t/2-t*t+2.0/3;
           nc3=(-t*t*t+t*t+t)/2 + 1.0/6;
           nc4=t*t*t/6;
           x = (nc1*xpoint[i]+nc2*xpoint[i+1]+nc3*xpoint[i+2]+nc4*xpoint[i+3]);
           y = (nc1*ypoint[i]+nc2*ypoint[i+1]+nc3*ypoint[i+2]+nc4*ypoint[i+3]);
           if((x!=X)||(y!=Y)) lineto((int)x,(int)y,color,mode,data->win);
       }
   }
   g.line.antialias = oantialias;

   delete[] ypoint;
   delete[] xpoint;
}

//--------------------------------------------------------------------------//
//   drawregressionline() = Draws nth order Linear regression line          //
//   Called by draw_curve().                                                //
//--------------------------------------------------------------------------//
void drawregressionline(XYData *data, uint color, int mode)
{
    int k;
    if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
    int n = data->n;
    int order = data->order; 
    double *x;
    double *y;
    double q[20];
    double f,r2;
    int x1,x2,y1,y2;
    if(n<=1) return;
    
    x = new double[n+1];
    y = new double[n+1];

    for(k=0;k<n;k++)
    {  x[k] = data->x[k];
       y[k] = data->y[k];
    }   

    x1 = data->x[0];
    x2 = data->x[n-1];
    if(n <= data->order+1)
    {   y1 = data->y[0];
        y2 = data->y[n-1];
    }else
    {   linreg(x,y,n,order,q,f,r2);
        y1 = (int)(q[0]+q[1]*x1);
        y2 = (int)(q[0]+q[1]*x2);
    }    
    line(x1,y1,x2,y2,color,mode,data->win);
    delete[] y;
    delete[] x;

}

//--------------------------------------------------------------------------//
// lineto = Draws a line starting from end of last line                     //
//   Called by bezier() and bspline().                                      //
// highest[] differs from most other pixel coordinates in this program in   //
//   that 0 is the left edge of the screen, not main_xpos.                  //
//--------------------------------------------------------------------------//
void lineto(int xe, int ye, uint color, int mode, Window win)
{
     int  k, xs, yy, ys;
     xs = X;
     ys = Y;
     X = xe;
     Y = ye;
     if(mode!=-1) 
        line(xs,ys,xe,ye,color,mode,win);
     else
     {  if(xs>xe){  swap(xe,xs); swap(ye,ys); }
        for(k=xs;k<=xe;k++)
        {    yy=0;
             if(xe!=xs) 
             {   yy = ys + (ye-ys)*(k-xs)/(xe-xs);
                 if(k+g.main_xpos<g.xres) g.highest[k+g.main_xpos] = yy;             
             }
        }     
     }   
}


//--------------------------------------------------------------------------//
// linreg  linear regression by least squares                               //
// If points are vertical,slope is set to 10000                             //
// Returns error code (OK if successful).                                   //
//--------------------------------------------------------------------------//
int linreg(double* x, double* y, int n, int regorder, double* q, double &f,
    double &r2)
{
   int i,i2,j,k,s,t,alloc1=0, alloc2=0;
   double bb,cc,nn,xx,yy,ss,tt;
   int ok,okx,oky;
   double *xraa = NULL;
   double *e = NULL;
   double firstx,firsty;
   double xj2=0,xk2=0,xj3=0,see;
      
   firstx=x[0]; firsty=y[0]; okx=0; oky=0;   
   for(k=0;k<n;k++)
   {  if(x[k]==0) x[k] = 1e-25;
      if(y[k]==0) y[k] = 1e-25;
      if(x[k]!=firstx) okx=1;
      if(y[k]!=firsty) oky=1;
   }
   if(!okx){ x[0]+=0.1; x[n]+=0.1; }  // prevent all x's from being the same
   if(!oky){ y[0]+=0.1; y[n]+=0.1; }  // prevent all y's from being the same

   nn = (double)n;
   if((regorder > n-1)||(regorder < 0))
   {  message("Bad regression order",ERROR);
      return(ERROR);
   }   

   i = 2 * regorder + 3;
   xraa = new double[i];
   e = new double[regorder + 3];
   array<double> r(regorder+3,regorder+3); 
   if(!r.allocated) goto linregend;

   for(i=2;i<=2*regorder+1;i++) xraa[i] = 0;
   for(i=1;i<=regorder+2;i++) e[i] = 0;
   xraa[1] = nn;
   for(i=1;i<=n;i++)
   {  yy = y[i-1];
      xx = x[i-1];
      for(j=2;j<=2*regorder+1;j++) xraa[j] += pow(xx,(double)j - 1.0);
      for(j=1;j<=regorder+1;j++)
      {  e[j] += yy * pow(xx,(double)j - 1.0);
         r.p[j][regorder+2] = e[j];
      }
      e[regorder + 2] += yy*yy;
   }
   for(i=1;i<=regorder+1;i++)
   {  for(j=1;j<=regorder+1;j++)
        r.p[i][j] = xraa[i+j-1];
   }     
   ok=0;
   for(s=1;s<=regorder+1;s++)
   {
      for(t=s;t<=regorder+1;t++) if(r.p[t][s] != 0.0){ ok=1; break; } 
      if(!ok){ message("No unique solution",ERROR); goto linregend; }
      for(j=1;j<=regorder+2;j++)
      {   bb = r.p[s][j];
          r.p[s][j] = r.p[t][j];
          r.p[t][j] = bb;
      }
   
      if(fabs(r.p[s][s])>1e-30)  cc=1.0/r.p[s][s];      //check for underflow
      else 
      {  message("Division by zero",ERROR); 
         goto linregend; 
      }
      for(j=1;j<=regorder+2;j++) r.p[s][j] = cc * r.p[s][j];
   
      for(t=1;t<=regorder+1;t++)
      {  if(t!=s) 
         {  cc = -r.p[t][s];
            for(j=1;j<=regorder+2;j++) r.p[t][j] += cc * r.p[s][j];
         }
      }   
   }                                       

    //---------Part 2--------//

   ss = 0.0;                               // sum of squares of regression

   for(i=2;i<=regorder+1;i++) ss += r.p[i][regorder+2]*(e[i]-xraa[i]*e[1]/nn);
   tt = e[regorder+2] - e[1]*e[1]/nn;      // total sum of squares
   cc = tt - ss;                           // sum of squares of residual
   i2 = n - regorder - 1;                  // deg.freedom of residual
   if(regorder!= 0) xj2 = ss/(double)regorder; // mean square of regression
   xk2 = cc / (double)i2;                  // mean square of residual
   if(xk2!=0.0) f=xj2/xk2;                 // f-value
   if(tt!=0.0) xj3=ss/tt;                  // coeff. of determination
   for(i=1;i<=regorder+1;i++) q[i-1]=r.p[i][regorder+2]; // k-degree coefficient
   r2 = sqrt(xj3);                         // coeff.of correlation
   if(i2) see=sqrt(cc / i2);               // std.error of estimate

linregend:
    if(alloc1) delete[] xraa;
    if(alloc2) delete[] e;
    return(OK);
}


//--------------------------------------------------------------------------//
// draw_point_to_point_curve                                                //
//--------------------------------------------------------------------------//
void draw_point_to_point_curve(XYData *data, uint color, int mode)
{  
   data=data; color=color; mode=mode;
   // Don't do nothin.
}
