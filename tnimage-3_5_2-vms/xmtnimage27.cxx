//--------------------------------------------------------------------------//
// xmtnimage27.cc                                                           //
// Draw lines                                                               //
// Latest revision: 03-02-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
static int circle_type = 1;
int *ellipse_x1;
int *ellipse_x2;

//-------------------------------------------------------------------//
// set_line_parameters                                               //
//-------------------------------------------------------------------//
void set_line_parameters(void)
{
  static int hit=0;
  int j,k;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  static Dialog *dialog;
  dialog = new Dialog;
  g.getout=0;
  strcpy(dialog->title,"Line drawing parameters");
  if(!hit) g.line.color = g.fcolor;
  hit=1;

  ////  Radios

  strcpy(dialog->radio[0][0],"Transverse gradient");
  strcpy(dialog->radio[0][1],"None");
  strcpy(dialog->radio[0][2],"Single");             
  strcpy(dialog->radio[0][3],"Double");
  strcpy(dialog->radio[1][0],"Line end");
  strcpy(dialog->radio[1][1],"None");
  strcpy(dialog->radio[1][2],"Square");             
  strcpy(dialog->radio[2][0],"Line type");
  strcpy(dialog->radio[2][1],"Line");
  strcpy(dialog->radio[2][2],"Ruler");             
  strcpy(dialog->radio[2][3],"Ruler+numbers");             
  dialog->radioset[0] = g.line.wantgrad+1;
  dialog->radioset[1] = g.line.perp_ends+1;
  dialog->radioset[2] = g.line.type+1;

  ////  Boxes
  
  for(k=0;k<DCOUNT;k++)dialog->boxset[k]=0;
  strcpy(dialog->boxes[0],"Line properties");
  strcpy(dialog->boxes[1],"Line width");
  strcpy(dialog->boxes[2],"Skew");
  strcpy(dialog->boxes[3],"Gradient");
  strcpy(dialog->boxes[4],"Center color");
  strcpy(dialog->boxes[5],"Outline color");
  strcpy(dialog->boxes[6],"Arrowhead");
  strcpy(dialog->boxes[7],"Outline");
  strcpy(dialog->boxes[8],"Antialiased");
  strcpy(dialog->boxes[9],"Arrow width");
  strcpy(dialog->boxes[10],"Outer length");
  strcpy(dialog->boxes[11],"Inner length");
  strcpy(dialog->boxes[12],"Ruler scale");
  strcpy(dialog->boxes[13],"Ruler tic length");

  dialog->boxtype[0]=LABEL; 
  dialog->boxtype[1]=STRING; 
  dialog->boxtype[2]=STRING;
  dialog->boxtype[3]=MULTICLICKBOX;
  dialog->boxtype[4]=RGBCLICKBOX;
  dialog->boxtype[5]=RGBCLICKBOX;
  dialog->boxtype[6]=TOGGLE;
  dialog->boxtype[7]=TOGGLE;
  dialog->boxtype[8]=TOGGLE;
  dialog->boxtype[9]=STRING; 
  dialog->boxtype[10]=STRING; 
  dialog->boxtype[11]=STRING; 
  dialog->boxtype[12]=STRING; 
  dialog->boxtype[13]=STRING; 

  sprintf(dialog->answer[1][0],"%d", g.line.width);
  sprintf(dialog->answer[2][0],"%d", g.line.skew);

  dialog->boxmin[3] = -100;
  dialog->boxmax[3] = 100;
  strcpy(dialog->answer[3][0],"Answer");
  sprintf(dialog->answer[3][0],"%d", g.line.gradr);
  sprintf(dialog->answer[3][1],"%d", g.line.gradg);
  sprintf(dialog->answer[3][2],"%d", g.line.gradb);
  strcpy(dialog->boxpushbuttonlabel[3][0],"Red");
  strcpy(dialog->boxpushbuttonlabel[3][1],"Green");
  strcpy(dialog->boxpushbuttonlabel[3][2],"Blue");

  sprintf(dialog->answer[4][0],"%d", g.line.color);
  sprintf(dialog->answer[5][0],"%d", g.line.outline_color);
  dialog->boxset[6] = g.line.wantarrow;
  dialog->boxset[7] = g.line.outline;
  dialog->boxset[8] = g.line.antialias;
  sprintf(dialog->answer[9][0],"%g", g.line.arrow_width);
  sprintf(dialog->answer[10][0],"%g", g.line.arrow_outer_length);
  sprintf(dialog->answer[11][0],"%g", g.line.arrow_inner_length);
  sprintf(dialog->answer[12][0],"%g", g.line.ruler_scale);
  sprintf(dialog->answer[13][0],"%g", g.line.ruler_ticlength);

  dialog->bpp = 24;
  dialog->radiono[0]=4;
  dialog->radiono[1]=3;
  dialog->radiono[2]=4;
  dialog->noofradios=3;
  dialog->noofboxes=14;
  dialog->helptopic=0;  
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
  dialog->want_changecicb = 0;
  dialog->f1 = lineparamcheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = lineparamfinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = (XtCBP)armboxcb; // calls f1 indirectly
  dialog->f9 = null;

  dialog->width = 0;     // calculate automatically
  dialog->height = 0;    // calculate automatically
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  strcpy(dialog->path,".");
  g.getout=0;
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  lineparamfinish                                                         //
//--------------------------------------------------------------------------//
void lineparamfinish(dialoginfo *a)
{
  int k;
  a=a;
  g.draw_figure = NONE;
  draw(END);
  g.getlinestate = 0;
  g.state = NORMAL;
  for(k=0;k<g.nbuttons;k++) 
  {   if(strcmp(g.button[k]->activate_cmd,"line")==SAME)
      unpush_main_button(k); 
  }
}


//--------------------------------------------------------------------------//
//  lineparamcheck                                                          //
//--------------------------------------------------------------------------//
void lineparamcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  static int ov = 1;
  g.line.wantgrad  = a->radioset[0]-1;
  g.line.perp_ends = a->radioset[1]-1;
  g.line.type      = a->radioset[2]-1;
  g.line.width = atoi(a->answer[1][0]);
  g.line.skew  = atoi(a->answer[2][0]); 
  g.line.gradr = atoi(a->answer[3][0]);
  g.line.gradg = atoi(a->answer[3][1]);
  g.line.gradb = atoi(a->answer[3][2]);
  g.line.color = atoi(a->answer[4][0]);
  g.line.outline_color = atoi(a->answer[5][0]);

  g.line.wantarrow = a->boxset[6];
  g.line.outline   = a->boxset[7];
  g.line.antialias = a->boxset[8];
  if(g.line.wantarrow)
      g.draw_figure=ARROW;
  else
      g.draw_figure=LINE; 

  g.line.arrow_width = atof(a->answer[9][0]);
  g.line.arrow_outer_length = atof(a->answer[10][0]);
  g.line.arrow_inner_length = atof(a->answer[11][0]);
  g.line.ruler_scale        = atof(a->answer[12][0]);
  g.line.ruler_ticlength    = atof(a->answer[13][0]);
  boxbutton=boxbutton;
  int j,k,v;
  double v6,v7,v8;
  double ratio=1;

  if(radio==0)                          // Transverse gradient radio group
  {    if(a->radioset[0]==1)            // No gradient
       {    for(k=0;k<4;k++) 
                for(j=3;j<=4;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
       }else
       {    for(k=0;k<4;k++) 
                for(j=3;j<=4;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
       }
  }
  if(box==6)                            // Arrow head
  {    if(a->boxset[6])
       {    for(k=0;k<4;k++)                // Arrow width, etc.
                for(j=9;j<=11;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
       }else
       {    for(k=0;k<4;k++) 
                for(j=9;j<=11;j++) 
                    if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
       }
  }
  if(box==7)                            // Outline    
  {    if(a->boxset[7])
       {  
            for(k=0;k<4;k++)                // Arrow width, etc.
                    if(a->boxwidget[5][k]) XtSetSensitive(a->boxwidget[5][k],True);
       }else
       {    for(k=0;k<4;k++) 
                    if(a->boxwidget[5][k]) XtSetSensitive(a->boxwidget[5][k],False);
       }
   }


  ////  Scale the arrow dimensions to linewidth so shape is the same
  if(box==1)
  {  
       v = cint(0.75*(double)atoi(a->answer[1][0]));
       if(v!=0 && ov!=0) ratio = (double)v/(double)ov; 
       else { ratio=v; strcpy(a->lastanswer[box],"1"); }
       v6 = max(6.0, ratio*atof(a->answer[9][0]));
       v7 = max(9.0, ratio*atof(a->answer[10][0]));
       v8 = max(6.0, ratio*atof(a->answer[11][0]));
       sprintf(a->answer[9][0],"%g",v6);
       sprintf(a->answer[10][0],"%g",v7);
       sprintf(a->answer[11][0],"%g",v8);
       XmTextSetString(a->boxwidget[9][0], a->answer[9][0]);
       XmTextSetString(a->boxwidget[10][0], a->answer[10][0]);
       XmTextSetString(a->boxwidget[11][0], a->answer[11][0]);
       g.line.arrow_width = atof(a->answer[9][0]);
       g.line.arrow_outer_length = atof(a->answer[10][0]);
       g.line.arrow_inner_length = atof(a->answer[11][0]);
       ov = v;
  }
}


//--------------------------------------------------------------------------//
// draw_arrow - draws arrow head with point at x1,y1 and tail in direction  //
// of x2,y2                                                                 //
//--------------------------------------------------------------------------//
void draw_arrow(int x1, int y1, int x2, int y2, lineinfo *li)
{
//               x1,y1---> x  csize    -
//                        /|\/         |
//                       / |/\         | 
//                      /  |  \        | asize
//                     /  _Xc  \       |
//                    /__/ | \__\      |
//                   //    |    \\     |
//                  Xt1--- Xb-----Xt2  -
//                      
//                  |----width----|
   double dx,dy,frac,m;
   int length,w2,xc,yc,xt1,yt1,xt2,yt2,xb,yb,oldoutline;
   double csize, asize, width;
   int color;
   csize = li->arrow_inner_length;
   asize = li->arrow_outer_length;
   width = li->arrow_width;
   color = li->color;

   if(x1==x2) x2=x1+1;
   length = (int)(sqrt( (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1)));
   if(length) frac = (double)csize/(double)length;
   else frac=0.0;

   xc = x1 + (int)(frac * (x2-x1));
   yc = y1 + (int)(frac * (y2-y1));

   frac = (double)asize/(double)length;
   xb = x1 + (int)(frac * (x2-x1));
   yb = y1 + (int)(frac * (y2-y1));

   if(y1!=y2) m = -(double)(x2-x1)/(double)(y2-y1);
   else m=-100000;
   w2 = cint(width/2);
   dx = sqrt((w2*w2)/(m*m+1));
   dy = dx*m;
   xt1 = xb - (int)dx;
   xt2 = xb + (int)dx;
   yt1 = yb - (int)dy;
   yt2 = yb + (int)dy;

   fill_triangle(x1,y1,xc,yc,xt1,yt1,li);
   fill_triangle(x1,y1,xc,yc,xt2,yt2,li);
   if(li->outline)
   {   oldoutline = li->outline;
       li->outline = 0;
       fill_triangle(x1,y1,xc,yc,xt1,yt1,li);
       fill_triangle(x1,y1,xc,yc,xt2,yt2,li);
       li->outline = oldoutline;
   }
}


//-------------------------------------------------------------------//
// cross                                                             //
// draw a cross                                                      //
//-------------------------------------------------------------------//
void cross(int x, int y, int d, uint color, int mode, int win)
{
   line(x-d/2, y, x+d/2, y, color,mode,win);
   line(x, y-d/2, x, y+d/2, color,mode,win);
}



//-------------------------------------------------------------------//
// set_circle_parameters                                             //
//-------------------------------------------------------------------//
void set_circle_parameters(void)
{
  static int hit=0;
  int j,k;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
  static Dialog *dialog;
  dialog = new Dialog;
  g.getout=0;
  strcpy(dialog->title,"Circle drawing parameters");
  if(!hit) g.line.color = g.fcolor;
  hit=1;

  ////  Radios

  strcpy(dialog->radio[0][0],"Shape");
  strcpy(dialog->radio[0][1],"Circle (fixed size)");
  strcpy(dialog->radio[0][2],"Ellipse/Circle");
  strcpy(dialog->radio[0][3],"Oval");
  strcpy(dialog->radio[0][4],"Rounded Rect.");
  strcpy(dialog->radio[0][5],"Rectangle");

  strcpy(dialog->radio[1][0],"Fill 1 Type");
  strcpy(dialog->radio[1][1],"None");
  strcpy(dialog->radio[1][2],"Solid color");
  strcpy(dialog->radio[1][3],"Radial grad.");
  strcpy(dialog->radio[1][4],"Linear grad.");

  strcpy(dialog->radio[2][0],"Fill 2 Type");
  strcpy(dialog->radio[2][1],"None");
  strcpy(dialog->radio[2][2],"Solid color");
  strcpy(dialog->radio[2][3],"Radial grad.");
  strcpy(dialog->radio[2][4],"Linear grad.");

  dialog->radioset[0] = circle_type;
  dialog->radiono[0] = 6;

  dialog->radioset[1] = g.gradient1_type;
  dialog->radiono[1] = 5;

  dialog->radioset[2] = g.gradient2_type;
  dialog->radiono[2] = 5;

  ////  Boxes
  
  for(k=0;k<DCOUNT;k++)dialog->boxset[k]=0;
  strcpy(dialog->boxes[0],"Line properties");
  strcpy(dialog->boxes[1],"Diameter");
  strcpy(dialog->boxes[2],"Line width");
  strcpy(dialog->boxes[3],"Outline");
  strcpy(dialog->boxes[4],"Grad.1 x angle");
  strcpy(dialog->boxes[5],"Grad.1 y angle");
  strcpy(dialog->boxes[6],"Outer color 1");
  strcpy(dialog->boxes[7],"Inner color 1");
  strcpy(dialog->boxes[8],"Grad2 x angle");
  strcpy(dialog->boxes[9],"Grad2 y angle");
  strcpy(dialog->boxes[10],"Outer color 2");
  strcpy(dialog->boxes[11],"Inner color 2");
  strcpy(dialog->boxes[12],"Reflectivity");
  strcpy(dialog->boxes[13],"Antialiased");

  dialog->boxtype[0]=LABEL; 
  dialog->boxtype[1]=INTCLICKBOX; 
  dialog->boxtype[2]=INTCLICKBOX; 
  dialog->boxtype[3]=TOGGLE; 

  dialog->boxtype[4]=DOUBLECLICKBOX;
  dialog->boxtype[5]=DOUBLECLICKBOX;
  dialog->boxtype[6]=RGBCLICKBOX;
  dialog->boxtype[7]=RGBCLICKBOX; 

  dialog->boxtype[8]=DOUBLECLICKBOX;
  dialog->boxtype[9]=DOUBLECLICKBOX;
  dialog->boxtype[10]=RGBCLICKBOX;
  dialog->boxtype[11]=RGBCLICKBOX; 
  dialog->boxtype[12]=DOUBLECLICKBOX;
  dialog->boxtype[13]=TOGGLE; 

  sprintf(dialog->answer[1][0],"%d", g.diameter);
  sprintf(dialog->answer[2][0],"%d", g.circlewidth);
  dialog->boxset[3] = g.circle_outline;
  sprintf(dialog->answer[4][0],"%g", g.gradientx1);
  sprintf(dialog->answer[5][0],"%g", g.gradienty1);
  sprintf(dialog->answer[6][0],"%d", g.gradient1_outer);
  sprintf(dialog->answer[7][0],"%d", g.gradient1_inner);

  sprintf(dialog->answer[8][0],"%g", g.gradientx2);
  sprintf(dialog->answer[9][0],"%g", g.gradienty2);
  sprintf(dialog->answer[10][0],"%d", g.gradient2_outer);
  sprintf(dialog->answer[11][0],"%d", g.gradient2_inner);
  sprintf(dialog->answer[12][0],"%g", g.reflectivity);
  dialog->boxset[13] = g.circle_antialias;

  dialog->boxmin[1] = 1;
  dialog->boxmax[1] = 450;
  dialog->boxmin[2] = 1;
  dialog->boxmax[2] = 40;
  dialog->boxmin[4] = 0;
  dialog->boxmax[4] = 1;
  dialog->boxmin[5] = 0;
  dialog->boxmax[5] = 1;

  dialog->boxmin[8] = 0;
  dialog->boxmax[8] = 1;
  dialog->boxmin[9] = 0;
  dialog->boxmax[9] = 1;
  dialog->boxmin[12] = 0;
  dialog->boxmax[12] = 1;

  dialog->bpp = 24;
  dialog->noofradios=3;
  dialog->noofboxes=14;
  dialog->helptopic=0;  
  for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
  dialog->want_changecicb = 0;
  dialog->f1 = circlecheck;
  dialog->f2 = null;
  dialog->f3 = null;
  dialog->f4 = circlefinish;
  dialog->f5 = null;
  dialog->f6 = null;
  dialog->f7 = null;
  dialog->f8 = (XtCBP)armboxcb; // calls f1 indirectly
  dialog->f9 = null;

  dialog->width = 0;     // calculate automatically
  dialog->height = 0;    // calculate automatically
  dialog->transient = 1;
  dialog->wantraise = 0;
  dialog->radiousexy = 0;
  dialog->boxusexy = 0;
  strcpy(dialog->path,".");
  g.getout=0;
  dialog->message[0] = 0;      
  dialog->busy = 0;
  dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  circlefinish                                                            //
//--------------------------------------------------------------------------//
void circlefinish(dialoginfo *a)
{
  int k;
  a=a;
  g.draw_figure = NONE;
  draw(END);
  g.getlinestate = 0;
  g.state = NORMAL;
  for(k=0;k<g.nbuttons;k++) 
  {   if(strcmp(g.button[k]->activate_cmd,"line")==SAME)
      unpush_main_button(k); 
  }
}


//--------------------------------------------------------------------------//
//  circlecheck                                                             //
//--------------------------------------------------------------------------//
void circlecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  box=box; boxbutton=boxbutton;
  g.diameter = atoi(a->answer[1][0]);
  g.circlewidth = atoi(a->answer[2][0]);
  circle_type = a->radioset[0];
  g.gradient1_type =  a->radioset[1];
  g.gradient2_type =  a->radioset[2];
  switch(circle_type)
  {    case 1:  g.draw_figure = CIRCLE; break;
       case 2:  g.draw_figure = ELLIPSE; break;
       case 3:  g.draw_figure = OVAL; break;
       case 4:  g.draw_figure = ROUNDED_RECTANGLE; break;
       case 5:  g.draw_figure = RECTANGLE; break;
  }
  g.diameter        = atoi(a->answer[1][0]);
  g.circlewidth     = atoi(a->answer[2][0]);
  g.circle_fill     = (a->radioset[1]>1) || (a->radioset[2]>1);  // 0 or 1
  g.circle_outline  = a->boxset[3];
  g.gradientx1      = atof(a->answer[4][0]);
  g.gradienty1      = atof(a->answer[5][0]);
  g.gradient1_outer = atoi(a->answer[6][0]);
  g.gradient1_inner = atoi(a->answer[7][0]);
  g.gradientx2      = atof(a->answer[8][0]);
  g.gradienty2      = atof(a->answer[9][0]);
  g.gradient2_outer = atoi(a->answer[10][0]);
  g.gradient2_inner = atoi(a->answer[11][0]);
  g.reflectivity    = atof(a->answer[12][0]);
  g.circle_antialias = a->boxset[13];

  // Return unless user clicked OK
  if(radio == -2) g.state = DRAWING;
}


//-------------------------------------------------------------------//
// circle                                                            //
// draw a circle                                                     //
// The optional 'win' causes box to be drawn on some window other    //
// than the drawing_area or an image (default=0).                    //
//-------------------------------------------------------------------//
void circle(int x, int y, int d, int width, uint color, int mode, int win)
{
  int r = d/2;
  int r2 = (int) (((double)r+ 0.5) * 0.707106781186547388734);
  int row=0, col, px, py, h, i, j, k, w, w2, bpp, ino;
  int sum, rr,gg,bb,rr1,gg1,bb1,rr2,gg2,bb2;
  int distance2, radius2;
  double dx, dy, frac, xfrac, yfrac;
  double kk=0, xx, yy;

  kk=kk;row=row;r2=r2;
  py = r<<1;
  px = 0;
  sum = -(r<<1);
  w = cint(width / 2);
  w2=w;
  if(width %2) w2++;
  if(g.circle_fill && mode==SET)
  {
      valuetoRGB(g.gradient1_outer, rr1, gg1, bb1, g.bitsperpixel);
      valuetoRGB(g.gradient1_inner, rr2, gg2, bb2, g.bitsperpixel);
      radius2 = d*d/4 - width;
      for(j=-d/2; j<d/2; j++)
      for(k=-d/2; k<d/2; k++)
      {
          switch(g.gradient1_type)
          {  case 2:   // none
                distance2 = j*j + k*k;
                if(distance2 < radius2)
                   setpixelonimage(x+k, y+j, g.gradient1_outer, SET,0,-1,0,win,1,0,1);
                break;
             case 3:   // radial
                distance2 = j*j + k*k;
                if(distance2 < radius2) // if inside circle
                {
                      xfrac = (double)k/(double)d + 0.5;  //0 to 1
                      yfrac = (double)j/(double)d + 0.5; 
                      dx = xfrac - g.gradientx1;
                      dy = yfrac + g.gradienty1 - 1;
                      frac = pow(dx*dx + dy*dy, 1 - g.reflectivity);
                      rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                      gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                      bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                      col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                      setpixelonimage(x+k, y+j, col, SET,0,-1,0,win,1,0,1);
                }                   
                break;
          }
      }
      valuetoRGB(g.gradient2_inner, rr2, gg2, bb2, g.bitsperpixel);
      for(j=-d/2; j<d/2; j++)
      for(k=-d/2; k<d/2; k++)
      {
          switch(g.gradient2_type)
          {  case 2:   // none
                distance2 = j*j + k*k;
                if(distance2 < radius2)
                   setpixelonimage(x+k, y+j, g.gradient2_outer, SET,0,-1,0,win,1,0,1);
                break;
             case 3:   // radial
                distance2 = j*j + k*k;
                if(distance2 < radius2) // if inside circle
                {
                      readRGBonimage(k+x, j+y, bpp, ino, rr1, gg1, bb1,-2);
                      xfrac = (double)k/(double)d + 0.5;  //0 to 1
                      yfrac = (double)j/(double)d + 0.5; 
                      dx = xfrac - g.gradientx2;
                      dy = yfrac + g.gradienty2 - 1;
                      frac = pow(dx*dx + dy*dy, 1 - g.reflectivity);
                      rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                      gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                      bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                      col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                      setpixelonimage(x+k, y+j, col, SET,0,-1,0,win,1,0,1);
                }                   
                break;
          }
      }
  }

  if(!g.circle_outline) return;
  if(g.circle_antialias)
  {
     radius2 = d*d/4 - width;
     if(mode==SET)
     for(j=cint(0.707*r); j>0; j--)
     {
        xx = sqrt((double)radius2 - (double)(j*j));
        yy = (double)j;
        for(h=-w; h<w2; h++)
        for(i=-w; i<w2; i++)
        {     set_antialiased_pixel_on_image(double(x+i)+xx, double(y+h)+yy,color,mode,0,-1,0,win,1,0,1);
              set_antialiased_pixel_on_image(double(x+i)-xx, double(y+h)+yy,color,mode,0,-1,0,win,1,0,1);
              set_antialiased_pixel_on_image(double(x+i)+xx, double(y+h)-yy,color,mode,0,-1,0,win,1,0,1);
              set_antialiased_pixel_on_image(double(x+i)-xx, double(y+h)-yy,color,mode,0,-1,0,win,1,0,1);
        }
     }
     if(mode==SET)
     {  
         set_antialiased_pixel_on_image(double(x), double(y-r),color,mode,0,-1,0,win,1,0,1);       
         set_antialiased_pixel_on_image(double(x), double(y+r),color,mode,0,-1,0,win,1,0,1);       
         set_antialiased_pixel_on_image(double(x-r), double(y),color,mode,0,-1,0,win,1,0,1);       
         set_antialiased_pixel_on_image(double(x+r), double(y),color,mode,0,-1,0,win,1,0,1);       
         for(k=cint(0.707*r); k>0; k--)
         {
              yy = sqrt((double)radius2 - (double)(k*k));
              xx = (double)k;
              for(h=-w; h<w2; h++)
              for(i=-w; i<w2; i++)
              {     set_antialiased_pixel_on_image(double(x+i)+xx, double(y+h)+yy,color,mode,0,-1,0,win,1,0,1);
                    set_antialiased_pixel_on_image(double(x+i)-xx, double(y+h)+yy,color,mode,0,-1,0,win,1,0,1);
                    set_antialiased_pixel_on_image(double(x+i)+xx, double(y+h)-yy,color,mode,0,-1,0,win,1,0,1);
                    set_antialiased_pixel_on_image(double(x+i)-xx, double(y+h)-yy,color,mode,0,-1,0,win,1,0,1);
             }
         }
     }
  }else
  {  if(g.circle_outline)
     while (px <= py)
     {   if ( !(px & 1))
         {  
            for(j=-w; j<w2; j++)
            for(k=-w; k<w2; k++)
            {
                col = x + k + (px>>1);
                row = y + j + (py>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                row = y + j - (py>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                col = x + k - (px>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                row = y + j + (py>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                col = x + k + (py>>1);
                row = y + j + (px>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                row = y + j - (px>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                col = x + k - (py>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
                row = y + j + (px>>1);
                setpixelonimage(col,row,color,mode,0,-1,0,win,1,0,1);
            }
         }
         sum +=px++;
         sum += px;
         if (sum >= 0)
         {  sum-=py--;
            sum -=py;
         }
     }
  }
}



//-------------------------------------------------------------------//
// box                                                               //
// Draw a box - if called from mouse loop, set inmenu non-zero first.//
// The optional 'win' causes box to be drawn on some window other    //
// than the drawing_area or an image (default=0).                    //
//-------------------------------------------------------------------//
void box(int x1, int y1, int x2, int y2, uint color, int mode, int win, int ino)
{
   lineinfo li;
   li.ino=ino;
   li.type=g.line.type;
   li.color=color;
   li.width=1;
   li.wantgrad=0;
   li.gradr=0;
   li.gradg=0;
   li.gradb=0;
   li.gradi=0;
   li.skew=0;
   li.perp_ends=0;
   li.wantarrow=0;
   li.outline=0;
   li.arrow_width=0.0;
   li.arrow_inner_length=0.0;
   li.arrow_outer_length=0.0;
   li.ruler_scale = 1.0;
   li.ruler_ticlength = 5.0;
   li.window=win;
   li.wantprint=0;
   li.antialias=0;
   box(x1,y1,x2,y2,mode,&li);
}
void box(int x1, int y1, int x2, int y2, int mode, lineinfo *li)
{
  int col, j, k, bpp, ino;
  int rr,gg,bb,rr1,gg1,bb1,rr2,gg2,bb2,width,height;
  double dx, dy, frac, xfrac, yfrac;
  if(!(g.circle_fill && mode==SET) || g.circle_outline) 
  {   line(x1-1,y1,x1-1,y2,mode,li);
      line(x2,y1,x2,y2,mode,li);
      line(cint(x1-li->width/2),y1,cint(x2+li->width/2),y1,mode,li);
      line(cint(x1-li->width/2),y2,cint(x2+li->width/2),y2,mode,li);
  } 
  if(x1>x2) swap(x1,x2); 
  if(y1>y2) swap(y1,y2);
  width = x2 - x1;
  height = y2 - y1;
  if(g.circle_fill && mode==SET)
  {
      valuetoRGB(g.gradient1_outer, rr1, gg1, bb1, g.bitsperpixel);
      valuetoRGB(g.gradient1_inner, rr2, gg2, bb2, g.bitsperpixel);
      for(j=y1; j<y2; j++)
      for(k=x1; k<x2; k++)
      {
          switch(g.gradient1_type)
          {  case 2:   // none
                setpixelonimage(k, j, g.gradient1_outer, SET,0,-1,0,0,1,0,1);
                break;
             case 3:   // radial
                xfrac = (double)(k-x1)/(double)width;  //0 to 1
                yfrac = (double)(j-y1)/(double)height; 
                dx = xfrac - g.gradientx1;
                dy = yfrac + g.gradienty1 - 1;
                frac = pow(dx*dx + dy*dy, 1 - g.reflectivity);
                rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                setpixelonimage(k, j, col, SET,0,-1,0,0,1,0,1);                              
                break;
          }
      }
      valuetoRGB(g.gradient2_inner, rr2, gg2, bb2, g.bitsperpixel);
      for(j=y1; j<y2; j++)
      for(k=x1; k<x2; k++)
      {
          switch(g.gradient2_type)
          {  case 2:   // none
                setpixelonimage(k, j, g.gradient2_outer, SET,0,-1,0,0,1,0,1);
                break;
             case 3:   // radial
                readRGBonimage(k, j, bpp, ino, rr1, gg1, bb1,-2);
                xfrac = (double)(k-x1)/(double)width;  //0 to 1
                yfrac = (double)(j-y1)/(double)height; 
                dx = xfrac - g.gradientx2;
                dy = yfrac + g.gradienty2 - 1;
                frac = pow(dx*dx + dy*dy, 1 - g.reflectivity);
                rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                setpixelonimage(k, j, col, SET,0,-1,0,0,1,0,1);                              
                break;
          }
      }
  }
}
void xor_box(int x1, int y1, int x2, int y2)
{
   g.inmenu++;
   box(x1,y1,x2,y2,g.maxcolor,XOR);
   XFlush(g.display);
   g.inmenu--;
}


//-------------------------------------------------------------------//
// dashed_box                                                        //
//-------------------------------------------------------------------//
void dashed_box(int x1, int y1, int x2, int y2, uint color1, uint color2,
     int spacing, int mode)
{
   int i,j,s2=2*spacing;
   mode = NONE;
   if(x1>x2)swap(x1,x2);
   if(y1>y2)swap(y1,y2);
   for(i=x1;i<=x2;i+=s2)
       for(j=0;j<spacing;j++)
       {   if(i+j>x2) continue;
           setpixel(i+j,y1,color1,mode);
           setpixel(i+j,y2,color1,mode);
           if(i+j+spacing>x2) continue;
           setpixel(i+j+spacing,y1,color2,mode);
           setpixel(i+j+spacing,y2,color2,mode);
       }
   for(i=y1;i<=y2;i+=s2)
       for(j=0;j<spacing;j++)
       {   if(i+j>y2) continue;
           setpixel(x1,i+j,color1,mode);
           setpixel(x2,i+j,color1,mode);
           if(i+j+spacing>y2) continue;
           setpixel(x1,i+j+spacing,color2,mode);
           setpixel(x2,i+j+spacing,color2,mode);
       }
}


//-------------------------------------------------------------------//
// line - draws a line                                               //
// Mode can be:  XOR or SET. If "SET", it uses the currently-selected//
//   pixel interaction mode (imode).                                 //
// Returns the number of pixels set.                                 //
// The optional 'win' causes box to be drawn on some window other    //
// than the drawing_area or an image (default=0).                    //
// Based on the Bresenham algorithm.                                 //
//                                                                   //
//   <45 degrees                                                     //
//   Fill direction ----->                                           //
//      __________________         ____________1_____2               //
//     4     /- _         |       4           _\     |               //
//     |    / 3   - _     |       |       _ -   \    |               //
//     5- _/          - _ |       |   _ -        \   |               //
//     |  /s - _          |       | -           e_\  |               //
//     | /       - _      /       |\         _ -   \ |               //
//     |/            - _e/|       | \ s  _ -       _\|               //
//     | - _            / |       | _\ -       _ -   |               //
//     |     - _       /  |       5-  \    _ -       |               // 
//     |__________- _/____|       |____\_-___________|               //
//                  1     2       4     3                            // 
//                                                                   //
//   >45 degrees                                                     //
//         4 -_----5-----      ------5-----_ 4       Fill direction  //
//          /   - /_   /       \     _\_ -  \            |           //
//         /     /s  -/         \_ -  s\     \           |           //
//        /     /    / 3       3 \      \     \          |           // 
//      1/_    /    /             \      \ e  _\1       \|/          //
//      /   - /e   /               \    _ \-    \        V           //
//     /_____/__-_/               2 \-_____\_____\                   //
//                 2                                                 //
//-------------------------------------------------------------------//
int line(int xs, int ys, int xe, int ye, uint color, int mode, int win, int ino)
{
   lineinfo li;
   li.ino=ino; // If 0 it can put it on any image
   li.type=g.line.type;
   li.color=color;
   li.width=g.line.width;
   li.wantgrad=0;
   li.gradr=0;
   li.gradg=0;
   li.gradb=0;
   li.gradi=0;
   li.skew=0;
   li.perp_ends=0;
   li.wantarrow=0;
   li.arrow_width=0.0;
   li.arrow_inner_length=0.0;
   li.arrow_outer_length=0.0;
   li.ruler_scale = 1.0;
   li.ruler_ticlength = 5.0;
   li.window=win;
   if(g.imode == BUFFER)
     li.wantprint=0;
   else
     li.wantprint=1;
   li.antialias=g.line.antialias;
   return line(xs,ys,xe,ye,mode,&li);
}

int line(int xs, int ys, int xe, int ye, int mode, lineinfo *li)
{
    int k, n, color, outlinecolor, xtext, ytext;
    double angle, len, dx, dy, x, y, seglength, ticfac = 1.0;
    char tempstring[256];

    //// Ruler
    //// Don't draw ruler if user selects 'label'

    if(g.draw_figure && (li->type==1 || li->type==2) && xe!=xs)
    {
       seglength = 5 * li->ruler_scale;
       angle = atan((double)(ye-ys)/(double)(xe-xs));
       len = sqrt((double)(xe-xs)*(xe-xs) + (double)(ye-ys)*(ye-ys));
       n = int(len / seglength);      // no. of tic marks
       for(k=0; k<=n; k++)
       {   ticfac=1.0; 
           if(!(k%5)) ticfac=2.0; 
           if(!(k%10)) ticfac=3.0;            
           dx = ticfac * li->ruler_ticlength * sin(angle);
           dy = ticfac * li->ruler_ticlength * cos(angle);
           x = (double)xs + (double)k * (double)(xe-xs) / (double)n; 
           y = (double)ys + (double)k * (double)(ye-ys) / (double)n; 
           do_line(cint(x), cint(y), cint(x+dx), cint(y-dy), mode, li);
           ////  Ruler with numbers
           if(!g.mouse_button && !(k%10) && li->type==2 && mode!=XOR)   
           {    sprintf(tempstring, "%d", k/10);
                xtext = cint(x + 1.5*dx);
                ytext = cint(y - 1.5*dy);
                print_rotated(xtext, ytext, tempstring, angle*DEGPERRAD);
           }
       }
    }
    
    if(li->outline==1)
    {
        outlinecolor = li->outline_color;
        color = li->color;
        li->color = outlinecolor;
        do_line(xs-1, ys, xe-1, ye, mode, li);
        do_line(xs+1, ys, xe+1, ye, mode, li);
        do_line(xs, ys-1, xe, ye-1, mode, li);
        do_line(xs, ys+1, xe, ye+1, mode, li);
        li->color = color;
    }
    return do_line(xs, ys, xe, ye, mode, li);
}


//-------------------------------------------------------------------//
// do_line - don't call directly                                     //
//-------------------------------------------------------------------//
int do_line(int xs, int ys, int xe, int ye, int mode, lineinfo *li)
{
     if(li->antialias && xs!=xe && ys!=ye && mode==SET) 
         return do_line_antialias(xs, ys, xe, ye, mode, li);
     char tempstring[256];
     double angle=0;
     int ino,bpp=0,b,c,cc=0,dx,dy,even=0,x_sign,y_sign,j,pixels=0,x,y,decision,total=0,
         jj,rr,gg,bb,rr1,gg1,bb1,ok=1,x1=0,y1=0,x2=0,y2=0,x3=0,y3=0,x4=0,
         x5=0,y5=0,e,sign=0,skew=0,sskew=0,ibpp,ii,jstart;     
     double len,m=0,m1=0,m3=0,value;
     int color = li->color;
     dx = abs(xe - xs);
     dy = abs(ye - ys);
     if(dx==0 && dy==0) return OK;   // Needed in Solaris
     if(li->perp_ends || li->width>1 || li->wantprint)
     {
        if(xe!=xs) angle = atan2(ye-ys,xe-xs); else angle = 1.570796327*sgn(ye-ys);    
     }
     if(li->wantgrad) 
     {   rr1=g.line.gradr; gg1=g.line.gradg; bb1=g.line.gradb;
     }else
     {   rr1=0; gg1=0; bb1=0; 
     }
     int fc,bc;
     Window win = li->window;
     ino = li->ino;
     if(ino) bpp = z[ino].bpp;

     if(li->wantprint)
     {   ////  Read the pixel value for information_area Widget.
         XtVaGetValues(g.info_area[0], XmNforeground, &fc, XmNbackground, &bc, NULL);         
         len = sqrt((ye-ys)*(ye-ys)+(xe-xs)*(xe-xs));
         sprintf(tempstring,"L=%4.1f %2.1f deg    ",len,-DEGPERRAD*angle);
         print(tempstring,8,84,fc,bc,&g.gc,g.info_window[3],BACKGROUND,HORIZONTAL);
     }
     if(li->skew) sskew = li->skew * sgn(xe-xs) * sgn(ye-ys);
     if(sskew==0 || dx<3 || dy<3) sskew=li->skew;
     
     if(dx > dy)         // Less than 45 degrees
     {  if(li->width>1)
        {   if(!(li->width & 1)) even=1;   
            pixels = abs(int((0.4999+(double)li->width/2)/cos(angle)));
        }
        if(xs > xe){ swap(xs,xe); swap(ys,ye); }
        if(ye - ys < 0) y_sign = -1; else y_sign = 1;        
        if(li->perp_ends)
        {   c = abs(cint((li->width/2)*sin(angle)));
            b = abs(cint((li->width/2)*cos(angle)));
            sign = sgn(ye-ys);
            x1 = xe - c;
            y1 = ye + b*sign;
            x2 = xe + c;
            y2 = y1;
            if(xe!=x1)
               m1 = -(double)(ye-y1)/(double)(xe-x1);
            else m1 = 1.0;
 
            x3 = xs + c;
            y3 = ys - b*sign;
            x4 = xs - c;
            if(xs!=x3)
               m3 = -(double)(ys-y3)/(double)(xs-x3);
            else m3 = 1.0;
            x5 = x4;
            y5 = cint(ys - c*tan(angle));
        }else
        {   x5 = xs; 
            y5 = ys;
            x2 = xe;
        }
        jstart = -pixels + even;
        for (j=-pixels+even; j<=pixels; j++)
        {   for (x=x5,y=y5,decision=0; x<=x2; x++,decision+=dy)
            {     if(decision>=dx){ decision -= dx; y+= y_sign; }
                  if(li->wantgrad && mode==SET)
                  {    valuetoRGB(color,rr,gg,bb,g.bitsperpixel);
                       if(li->wantgrad==2) jj=abs(j); else jj=j;
                       rr+=jj*rr1; gg+=jj*gg1; bb+=jj*bb1;
                       cc=RGBvalue(rr,gg,bb,g.bitsperpixel);
                  }else cc=color;
                  if(li->skew) skew = j*sskew;                 
                  if(li->perp_ends)
                  {    ok=1;
                       if(x<x3)
                       {   m = -(double)(y+j-y3)/(double)(x-x3);
                           if(sign != sgn(m-m3)) ok=0;
                       }
                       if(x>x1)
                       {   m = -(double)(y+j-y1)/(double)(x-x1);
                           if(sign != sgn(m-m1)) ok=0;
                       }
                  }
                  switch(mode)
                  {  case NONE:
                     case SET:   //1
                       total++;
                       if(ok) setpixelonimage(x+skew,y+j,cc,g.imode,bpp,-1,0,win,1,ino,1);
                       break;
                     case XOR:   //7
                       total++;
                       bpp = whatbpp(x,y+j);
                       if(ok) setpixelonimage(x+skew,y+j,cc,XOR,bpp,-1,0,win,1,ino);
                       break;
                     case BUFFER: 
                       total++;
                       bpp = z[ino].bpp;
                       if(ok) setpixelonimage(x+skew,y+j,cc,BUFFER,bpp,-1,1,win,0,ino);
                       break;
                     case SCAN:  
                       total++;
                       value = (double)readpixelonimage(x,y,ibpp,ii);
                       if(ok) g.scan[g.scancount++] = value / g.maxvalue[ibpp];
                       break;
                     case TRACE:  
                       total++;
                       if(ok && between(x,0,g.xres-1)) g.highest[x] = y;
                       break;
                     default:  
                       fprintf(stderr, "Error in line %d\n",mode);
                       break;
                  }
                  
            }
        }    
     }else    // more than 45 degrees
     {        
        if(li->width>1)
        {   if(!(li->width & 1)) even=1;   
            pixels = abs(int((0.4999+(double)li->width/2)/sin(angle)));
        }
        if(ys > ye){ swap(ys,ye); swap(xs,xe);  }
        if(xe - xs < 0) x_sign = -1; else x_sign = 1;                 
        if(li->perp_ends)
        {   sign = sgn(xs-xe);
            b = abs(cint((li->width/2)*cos(angle)));
            c = abs(cint(b/tan(angle)));
            e = abs(cint((li->width/2)*sin(angle)));

            x1 = xe - sign*e;
            y1 = ye - b;
            x2 = xe + sign*e;
            y2 = ye + b;
            if(xe!=x1)
               m1 = (double)(ye-y1)/(double)(xe-x1);
            else m1=1;

            x3 = xs + sign*e;
            y3 = ys + b;
            x4 = xs - sign*e;
            if(xs!=x3)
               m3 = (double)(ys-y3)/(double)(xs-x3);
            else m3=1;
            x5 = cint(xs + sign*c);
            y5 = ys - b;
        }else
        {   x5 = xs; 
            y5 = ys;
            y2 = ye;
        }
        for (j=-pixels+even; j<=pixels; j++)
        {   for(x=x5,y=y5,decision=0; y<=y2; y++,decision+=dx)
            {     if (decision>=dy){ decision -= dy; x+=x_sign; }
                  if(li->wantgrad && mode==SET)
                  {    valuetoRGB(color,rr,gg,bb,g.bitsperpixel);
                       if(li->wantgrad==2) jj=abs(j); else jj=j;
                       rr+=jj*rr1; gg+=jj*gg1; bb+=jj*bb1;
                       cc=RGBvalue(rr,gg,bb,g.bitsperpixel);
                  }else cc=color;
                  if(li->skew) skew = j*sskew;                 
                  if(li->perp_ends)
                  {    ok=1;
                       if(y<y3)
                       {   
                           if(x+j-x3 != 0) m = (double)(y-y3)/(double)(x+j-x3); else m=1.0;
                           if(sign != sgn(m3-m)) ok=0;
                           if((sign>0 && x+j>=x3) || (sign<0 && x+j<x3)) ok=0;
                       }
                       if(y>y1)
                       {   if(x+j-x1 != 0) m = (double)(y-y1)/(double)(x+j-x1); else m=1.0;
                           if(sign != sgn(m1-m)) ok=0;
                           if((sign>0 && x+j<x1) || (sign<0 && x+j>=x1)) ok=0;
                       }
                  }
                  switch(mode)
                  {  
                     case NONE:
                     case SET:   //1
                       total++;
                       if(ok)setpixelonimage(x+j,y+skew,cc,g.imode,bpp,-1,0,win,1,ino,1);
                       break;
                     case XOR:   // 7
                       total++;
                       bpp = whatbpp(x+j,y);
                       if(ok)setpixelonimage(x+j,y+skew,cc,XOR,bpp,-1,0,win,1,ino);
                       break;
                     case BUFFER: 
                       total++;
                       bpp = z[ino].bpp;
                       if(ok)setpixelonimage(x+j,y+skew,cc,BUFFER,bpp,-1,1,win,0,ino);
                       break;
                     case SCAN:  
                       total++;
                       value = (double)readpixelonimage(x,y,ibpp,ii);
                       if(ok)g.scan[g.scancount++] = value / g.maxvalue[ibpp];
                       break;
                     case TRACE:  
                       total++;
                       if(ok && between(x,0,g.xres-1)) g.highest[x] = y;
                       break;
                     default: 
                       fprintf(stderr, "Error in line %d\n",mode);
                       break; 
                  }
            }
        }
     }
     return(total);
}


//--------------------------------------------------------------------------//
//  sketch                                                                  //
//--------------------------------------------------------------------------//
int sketch(int xs, int ys, int xe, int ye, uint color, int mode, lineinfo *line, 
    int inmath)
{
     int cc=0,dx,dy,x_sign,y_sign,j,pixels=0,x,y,decision,total=0,h,
         jj,rr,gg,bb,rr1,gg1,bb1,skew=0,sskew=0;     
     dx = abs(xe - xs);
     dy = abs(ye - ys);
     if(dx==0 && dy==0) return OK;   // Needed in Solaris
     if(dx>100000 || dy>100000) return BADPARAMETERS;
     if(line->wantgrad) { rr1 = g.line.gradr; gg1 = g.line.gradg; bb1 = g.line.gradb;}
     else { rr1 = gg1 = bb1 = 0; }
     Window win = line->window;
     if(line->width>1) pixels = line->width;
     sskew = line->skew;    
     int sk0 = pixels*sskew;

     if(dx > dy)         // Less than 45 degrees
     {
        if(xs > xe){ swap(xs,xe); swap(ys,ye); }
        if(ye - ys < 0) y_sign = -1; else y_sign = 1;        
        for (j=-pixels,skew=-sk0; j<=pixels; j++,skew+=sskew)
        {   for (x=xs,y=ys,decision=0; x<=xe; x++,decision+=dy)
            {     if(decision>=dx){ decision -= dx; y+= y_sign; }
                  if(inmath)
                  { 
                       do_pixel_math(x, y, x+1, y+1, g.sketchtext, 0);
                  }else 
                  {    if(line->wantgrad && mode==SET)
                       {    valuetoRGB(color,rr,gg,bb,g.bitsperpixel);
                            if(line->wantgrad==2) jj=abs(j); else jj=j;
                            rr+=jj*rr1; gg+=jj*gg1; bb+=jj*bb1;
                            cc=RGBvalue(rr,gg,bb,g.bitsperpixel);
                       }else cc=color;
                       for(h=skew-sskew;h<=skew+sskew;h++)
                            setpixelonimage(x+h,y+j,cc,g.imode,0,-1,0,win,1,0,ZOOMED);
                  }
            }
        }     
     }else    // more than 45 degrees
     {        
        if(ys > ye){ swap(ys,ye); swap(xs,xe);  }
        if(xe - xs < 0) x_sign = -1; else x_sign = 1;                 
        for (j=-pixels,skew=-sk0; j<=pixels; j++,skew+=sskew)
        {   for(x=xs,y=ys,decision=0; y<=ye; y++,decision+=dx)
            {     if (decision>=dy){ decision -= dy; x+=x_sign; }
                  if(inmath)
                  {
                       do_pixel_math(x, y, x+1, y+1, g.sketchtext, 0);
                  }else 
                  {    if(line->wantgrad && mode==SET)
                       {    valuetoRGB(color,rr,gg,bb,g.bitsperpixel);
                            if(line->wantgrad==2) jj=abs(j); else jj=j;
                            rr+=jj*rr1; gg+=jj*gg1; bb+=jj*bb1;
                            cc=RGBvalue(rr,gg,bb,g.bitsperpixel);
                       }else cc=color;
                       for(h=skew-sskew;h<=skew+sskew;h++)
                            setpixelonimage(x+h,y+j,cc,g.imode,0,-1,0,line->window,1,0,ZOOMED);
                  }
            }
        }
     }
     return(total);
}


//--------------------------------------------------------------------------//
//  drawline - for macros                                                   //
//--------------------------------------------------------------------------//
void drawline(int noofargs, char **arg, int whatever)
{
   int i,j, ino, bpp, width=1;
   int x1,x2,y1,y2,d=10, mode=SET;
   int color = g.fcolor;
   x1=y1=x2=y2=0;
   if(noofargs >=1) x1=atoi(arg[1]);
   if(noofargs >=2) y1=atoi(arg[2]);
   if(noofargs >=3) x2=atoi(arg[3]);
   if(noofargs >=4) y2=atoi(arg[4]);
   if(noofargs >=5) color=atoi(arg[5]);
   if(noofargs >=6) mode=atoi(arg[6]);
   if(ci>0) color = convertpixel(color,z[ci].bpp,g.bitsperpixel,1);

   switch(whatever)
   {   case LINE:   line(x1,y1,x2,y2,color,mode); break;
       case BOX:    box(x1,y1,x2,y2,color,mode); break;
       case CIRCLE: if(noofargs >=3) d = atoi(arg[3]);
                    if(noofargs >=4) color = atoi(arg[4]);
                    if(noofargs >=5) width = atoi(arg[5]);
                    if(noofargs >=6) g.circle_antialias = atoi(arg[6]);
                    if(noofargs >=7) mode  = atoi(arg[7]);
                    circle(x1, y1,d, width, color, mode, -2); 
                    break;
       case RECTANGLE: box(x1,y1,x2,y2,color,mode); break;
       case FILLEDRECTANGLE: 
             for(j=y1; j<=y2; j++)
             for(i=x1; i<=x2; i++)
                setpixelonimage(i,j,color,mode,0,-1,0,0,1);
             break;   
       case PIXEL: 
             if(noofargs >=3) color = atoi(arg[3]);
             if(noofargs >=4) mode = atoi(arg[4]);
             setpixelonimage(x1,y1,color,mode,0,-1,0,0,1); 
             break;
   }
   ino = whichimage(x1,y1,bpp);
   //if(whatever != PIXEL && ino>=0) repair(ino);
}



//--------------------------------------------------------------------------//
//  drawlabel - for macros                                                  //
//--------------------------------------------------------------------------//
void drawlabel(int noofargs, char **arg)
{
   int fcol=g.fcolor, bcol=g.bcolor, wantbkg=0, vertical=0;
   int x,y,ino,bpp;
   x=y=0;
   ino = whichimage(x,y,bpp);
   if(noofargs <1) return;
   if(noofargs >=2) x = atoi(arg[2]);
   if(noofargs >=3) y = atoi(arg[3]);
   if(noofargs >=4) vertical = atoi(arg[4]);
   if(noofargs >=5) wantbkg = atoi(arg[5]);
   if(noofargs >=6) fcol = atoi(arg[6]);
   if(noofargs >=7) bcol = atoi(arg[7]);
   print(arg[1], x, y, fcol, bcol, &g.image_gc, z[ino].win, wantbkg, vertical,0);
}


//--------------------------------------------------------------------------//
//  unsetpixel - for macros                                                 //
//--------------------------------------------------------------------------//
void unsetpixel(int noofargs, char **arg)
{
   int b,f,ino,bpp,x1,y1,pix;
   x1=y1=0;
   if(noofargs >=1) x1=atoi(arg[1]);
   if(noofargs >=2) y1=atoi(arg[2]);
   ino = whichimage(x1,y1,bpp);
   if(!between(ino, 0, g.image_count-1)) return;
   switchto(ino);
   b = g.off[bpp];
   f = z[ino].cf;
   if(!between(x1, 0, z[ino].xsize-1)) return;
   if(!between(y1, 0, z[ino].ysize-1)) return;
   if(z[ci].backedup)
   {    pix = pixelat(z[ci].backup[f][y1] + b*x1, bpp);
        putpixelbytes(z[ino].image[f][y1] + b*x1, pix, 1, bpp, 1);   
   }
}


//--------------------------------------------------------------------------//
//  do_line_antialias                                                       //
//--------------------------------------------------------------------------//
int do_line_antialias(int xs, int ys, int xe, int ye, int mode, lineinfo *li) 
{
   mode=mode;
   double  x,y,dx,dy,dydx,dxdy,dydx2,dxdy2,distance,angle;
   int j,fc,ino,bpp,value,opix,width,x1,y1,gatevalue,gg,oino=ci,
       ok=1,p,miny,maxy,temp;
   if(xs>xe)
   {  temp = xe; xe = xs; xs = temp; // swap doesn't work here for
      temp = ye; ye = ys; ys = temp; // unknown reason
   }
   dx = (double)(xe - xs);
   dy = (double)(ye - ys);
   if(fabs(dx)<0.01) dx=0.01;
   if(fabs(dy)<0.01) dy=0.01;
   int d = cint(sqrt(dy*dy + dx*dx));  
   dydx = dy/dx;
   dxdy = dx/dy;
   dydx2 = dydx / 2.0;
   dxdy2 = dxdy / 2.0;
   if(xe!=xs) angle = atan2(ye-ys, xe-xs); else angle = 90*RADPERDEG;
   width = 1 + li->width / 2;
   fc = li->color;
   miny = min(ys, ye);
   maxy = max(ys, ye);

   if(fabs(dx)>fabs(dy))
   { 
      width = int((double)width / cos(angle));
      x = (double)xs - width;
      y = (double)ys - width*dydx;
      while(1)
      {    x++;
           if(x > xe+width) break;
           y += dydx;
           for(j=-width-1; j<width+1; j++) // use vertical segments
           {   x1 = cint(x);
               y1 = cint(y + (double)j);       
               opix = readpixel(x1, y1);
               ino = whichimg(x1, y1, bpp);
               if(ino>=0 && z[ino].bpp==8){ switch_palette(ino); oino=ino; }
               distance = max(0.0, (double)width - fabs(y-(double)y1)); 
               gg = min(255, cint(255.0 * distance));
               gatevalue = RGBvalue(gg,gg,gg,g.bitsperpixel);
               value = anti_aliased_value(gatevalue, opix, fc, COLOR, g.bitsperpixel);
               p = cint(x + (double)j*dydx2);
               if(p<xs || p>xe) ok=0; else ok=1;  // square edge
               if(ok) setpixelonimage(x1,y1,value,g.imode,g.bitsperpixel,-1,0,0,1,0,1);
           }
      }
   }else
   {  
      width = int((double)width / fabs(sin(angle)));    
      x = (double)xs - width*dxdy*sgn(dy);
      y = (double)ys - width*sgn(dy);
      while(1)
      {    x += dxdy*(double)sgn(dy);
           if(x>=xe+width) break;
           y += (double)sgn(dy);
           for(j=-width-1; j<width+1; j++) // use horizontal segments
           {   y1 = cint(y);
               x1 = cint(x + (double)j);       
               opix = readpixel(x1,y1);
               ino = whichimg(x1,y1,bpp);
               if(ino>=0 && z[ino].bpp==8){ switch_palette(ino); oino=ino; }
               distance = max(0.0, (double)width - (double)fabs(x-(double)x1));
               gg = min(255, cint(255.0 * distance));
               gatevalue = RGBvalue(gg,gg,gg,g.bitsperpixel);
               value = anti_aliased_value(gatevalue, opix, fc, COLOR, g.bitsperpixel);
               p = cint(y + (double)j*dxdy2);
               if(p<miny || p>maxy) ok=0; else ok=1; // square edge
               if(ok) setpixelonimage(x1,y1,value,g.imode,g.bitsperpixel,-1,0,0,1,0,1);
           }
      }
   }   
   return d;

}


//-------------------------------------------------------------------//
// ellipse                                                           //
// draw an ellipse                                                   //
// Adapted from Tim Hogan's DDJ algorithm as modified by J.D.        //
// McDonald.                                                         //
//-------------------------------------------------------------------//
void ellipse(int xx1, int yy1, int xx2, int yy2, int width, int color,
             int mode, int figure_type)
{
   int alpha, beta, alpha2, alpha4, beta2, beta4, d, j, k,
       ddx, ddy, alphadx, betady, dy, dx, irx, iry, xx, yy,
       ymin, ino, bpp;
   int xdiam, ydiam, col, rr, gg, bb, rr1,gg1,bb1,rr2,gg2,bb2;
   double fracdx, fracdy, frac, xfrac, yfrac;
 
   //// irx = width   iry = height   
   //// xx1,yy1 xx2,yy2 = start and end coordinates supplied by user

   if(figure_type == ROUNDED_RECTANGLE)
   {    rounded_rectangle(xx1,yy1,xx2,yy2,width,color,mode); 
        return;
   }
   if(figure_type == RECTANGLE)
   {     box(xx1, yy1, xx2, yy2, color, mode);
        return;
   }

   if(xx1 > xx2) swap(xx1,xx2);
   if(yy1 > yy2) swap(yy1,yy2);
   if(yy1==yy2) return;
   if(mode == SET && g.circle_fill)
   {    ellipse_x1 = new int[2*(yy2-yy1+1)];
        ellipse_x2 = new int[2*(yy2-yy1+1)];
        for(j=0; j<2*(yy2-yy1); j++) ellipse_x1[j] = ellipse_x2[j] = 0;
   }


   irx = xx2 - xx1;
   iry = yy2 - yy1;
   beta = irx * irx;
   alpha = iry * iry;

   if (alpha == 0) alpha = 1;
   if (beta == 0) beta = 1;
   dy = 0;
   dx = irx;
   alpha2 = alpha << 1;
   alpha4 = alpha2 << 1;
   beta2 = beta << 1;
   beta4 = beta2 << 1;
   alphadx = alpha * dx;
   betady = 0;
   ddx = alpha4 * (1 - dx);
   ddy = beta2 * 3;

   d = alpha2 * ((dx - 1) * dx) + alpha + beta2 * (1 - alpha);
   xx = xx1; 
   yy = yy1;
   setpixelonimage(xx2, yy-iry, color, mode,0,-1,0,0,1,0,1);
   setpixelonimage(xx2, yy+iry, color, mode,0,-1,0,0,1,0,1);
   do {
        if (d >= 0) 
        {   d += ddx;
            dx--;
            alphadx -= alpha;
            ddx += alpha4;
            set_ellipse_pixels(xx++,yy--,xx1,yy1,xx2,yy2,width, color, mode);
	} else
            set_ellipse_pixels(xx,yy--,xx1,yy1,xx2,yy2,width, color, mode);
        d += ddy;
        dy++;
        betady += beta;
        ddy += beta4;
   } while (alphadx > betady);

   d = beta2 * (dy * (dy + 1)) + alpha2 * (dx * (dx - 2) + 1) 
       + beta * (1 - alpha2);
   ddx = alpha2 * (3 - (dx << 1));
   ddy = beta4 * (1 + dy);

   do {
	if (d <= 0) 
        {   d += ddy;
	    ddy += beta4;
	    dy++;
            set_ellipse_pixels(xx++,yy--,xx1,yy1,xx2,yy2,width, color, mode);
        } else
            set_ellipse_pixels(xx++,yy,xx1,yy1,xx2,yy2,width, color, mode);
        d += ddx;
        ddx += alpha4;
        dx--;
   } while (dx > 0);

   if(mode==SET && g.circle_fill)
   {    ymin = yy1 - (yy2-yy1);
        for(j=0; j<2*(yy2-yy1); j++)
        {   switch(g.gradient1_type)
            {     case 2:   // no gradient
                      for(k=ellipse_x1[j]; k<ellipse_x2[j]; k++)
                          setpixelonimage(k+xx1, j+ymin,  g.gradient1_outer, mode,0,-1,0,0,1,0,1);
                      break;
                  case 3:
                      valuetoRGB(g.gradient1_outer, rr1, gg1, bb1, g.bitsperpixel);
                      valuetoRGB(g.gradient1_inner, rr2, gg2, bb2, g.bitsperpixel);
                      xdiam = 2*(xx2-xx1-width);
                      ydiam = 2*(yy2-yy1-width);
                      for(k=ellipse_x1[j]; k<ellipse_x2[j]; k++)
                      {     xfrac = (double)k/(double)xdiam;  //0 to 1
                            yfrac = (double)j/(double)ydiam; 
                            fracdx = xfrac - g.gradientx1;
                            fracdy = yfrac + g.gradienty1 - 1;
                            frac = pow(fracdx*fracdx +fracdy*fracdy, 1.0 - g.reflectivity);
                            rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                            gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                            bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                            col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                            setpixelonimage(k+xx1, j+ymin,  col, mode,0,-1,0,0,1,0,1);
                      }                   
                      break;
              }
              switch(g.gradient2_type)
              {   
                  case 2:   // no gradient
                      for(k=ellipse_x1[j]; k<ellipse_x2[j]; k++)
                          setpixelonimage(k+xx1, j+ymin,  g.gradient2_outer, mode,0,-1,0,0,1,0,1);
                      break;
                  case 3:
                      valuetoRGB(g.gradient2_inner, rr2, gg2, bb2, g.bitsperpixel);
                      xdiam = 2*(xx2-xx1-width);
                      ydiam = 2*(yy2-yy1-width);
                      for(k=ellipse_x1[j]; k<ellipse_x2[j]; k++)
                      {    
                            readRGBonimage(k+xx1, j+ymin, bpp, ino, rr1, gg1, bb1,-2);
                            xfrac = (double)k/(double)xdiam;  //0 to 1
                            yfrac = (double)j/(double)ydiam; 
                            fracdx = xfrac - g.gradientx2;
                            fracdy = yfrac + g.gradienty2 - 1;
                            frac = pow(fracdx*fracdx +fracdy*fracdy, 1.0 - g.reflectivity);
                            rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                            gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                            bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                            col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                            setpixelonimage(k+xx1, j+ymin,  col, mode,0,-1,0,0,1,0,1);
                      }                   
                      break;
              }
        }

        delete[] ellipse_x1;
        delete[] ellipse_x2;

   }
}


//-------------------------------------------------------------------//
//  set_ellipse_pixels                                               //
//-------------------------------------------------------------------//
void set_ellipse_pixels(int xx, int yy, int xx1, int yy1, int xx2, int yy2,
     int width, int color, int mode)
{
   xx1 = xx1;
   int j, k, w1, w2, ymin;
   w1 = cint(width / 2);
   w2 = w1;
   if(width %2) w2++;

   if(mode!=SET || g.circle_outline)
   for(j=-w1; j<w2; j++)
   for(k=-w1; k<w2; k++)
   { 
   if(mode==SET && g.circle_antialias)
   {
       set_antialiased_pixel_on_image(double(xx+k),         double(yy+j),         color, mode,0,-1,0,0,1,0,1);
       set_antialiased_pixel_on_image(double(xx2+xx2-xx+k), double(yy+j),         color, mode,0,-1,0,0,1,0,1);
       set_antialiased_pixel_on_image(double(xx+k),         double(yy1+yy1-yy+j), color, mode,0,-1,0,0,1,0,1);
       set_antialiased_pixel_on_image(double(xx2+xx2-xx+k), double(yy1+yy1-yy+j), color, mode,0,-1,0,0,1,0,1);
   }else
   {
       setpixelonimage(xx+k,         yy+j,         color, mode,0,-1,0,0,1,0,1);
       setpixelonimage(xx2+xx2-xx+k, yy+j,         color, mode,0,-1,0,0,1,0,1);
       setpixelonimage(xx+k,         yy1+yy1-yy+j, color, mode,0,-1,0,0,1,0,1);
       setpixelonimage(xx2+xx2-xx+k, yy1+yy1-yy+j, color, mode,0,-1,0,0,1,0,1);
   }
   }
   if(mode==SET && g.circle_fill)
   {   ymin = yy1 - (yy2-yy1);
       ellipse_x1[yy-ymin] = xx-xx1;
       ellipse_x2[yy-ymin] = xx2+xx2-xx-xx1;
       ellipse_x1[yy1+yy1-yy-ymin] = xx-xx1;
       ellipse_x2[yy1+yy1-yy-ymin] = xx2+xx2-xx-xx1;
   }
}



//-------------------------------------------------------------------//
// rounded_rectangle                                                 //
//-------------------------------------------------------------------//
void rounded_rectangle(int xx1, int yy1, int xx2, int yy2, int width, int color, int mode)
{  
  int d = g.diameter;
  int j, k, xdiam, ydiam, col, rr, gg,bb, rr1,gg1,bb1,rr2,gg2,bb2;
  double fracdx, fracdy, frac, xfrac, yfrac;

  if(xx2<xx1) swap(xx1,xx2);
  if(yy2<yy1) swap(yy1,yy2);
  if(yy1==yy2) return;

  //// Allocate arrays and put straight portion start & end points in array
  if(mode == SET && g.circle_fill)
  {    ellipse_x1 = new int[yy2-yy1+2*width+d+d+1];
       ellipse_x2 = new int[yy2-yy1+2*width+d+d+1];
       for(j=0; j<yy2-yy1+d-width; j++)
       {     ellipse_x1[j] = width/2;
             ellipse_x2[j] = xx2-xx1+d-width/2;
       }          
  }

  //// These 4 lines put curved parts in arrays ellipse_x1 and ellipse_x2
////*********FIX**********
  draw_arc(xx1-d/2+width, yy1-d/2+width, xx1, yy1, ULEFT,  width,color,mode);
  draw_arc(xx1-d/2+width, yy1-d/2+width, xx1, yy2, LLEFT,  width,color,mode);
  draw_arc(xx1-d/2+width, yy1-d/2+width, xx2, yy1, URIGHT, width,color,mode);
  draw_arc(xx1-d/2+width, yy1-d/2+width, xx2, yy2, LRIGHT, width,color,mode);
  g.line.width = width;
  int dx = 0;
  if(width>=2) dx=1;


  line(xx1,        yy1-d/2-dx, xx2,        yy1-d/2-dx, color, mode, 0); //top
  line(xx1,        yy2+d/2,    xx2,        yy2+d/2,    color, mode, 0); //bottom
  line(xx1-d/2-dx, yy1,        xx1-d/2-dx, yy2,        color, mode, 0); //left
  line(xx2+d/2-dx/2, yy1,      xx2+d/2-dx/2, yy2,      color, mode, 0); //right
  if(mode==SET && g.circle_fill)
  {    for(j=0; j<yy2-yy1+d-width; j++)
       switch(g.gradient1_type)
       {   case 2:   // no gradient
               for(k=ellipse_x1[j]-d/2; k<ellipse_x2[j]-d/2; k++)
                   setpixelonimage(k+xx1, j+yy1-d/2+width/2,  g.gradient1_outer, mode,0,-1,0,0,1,0,1);
               break;
           case 3:
               valuetoRGB(g.gradient1_outer, rr1, gg1, bb1, g.bitsperpixel);
               valuetoRGB(g.gradient1_inner, rr2, gg2, bb2, g.bitsperpixel);
               xdiam = 2*(xx2-xx1-width);
               ydiam = 2*(yy2-yy1-width);
               for(k=ellipse_x1[j]-d/2; k<ellipse_x2[j]-d/2; k++)
               {     xfrac = (double)k/(double)xdiam;  //0 to 1
                     yfrac = (double)j/(double)ydiam; 
                     fracdx = xfrac - g.gradientx1;
                     fracdy = yfrac + g.gradienty1 - 1;
                     frac = sqrt(fracdx*fracdx +fracdy*fracdy);
                     rr = max(0, min(255, cint(frac*rr1 + (1-frac)*rr2)));
                     gg = max(0, min(255, cint(frac*gg1 + (1-frac)*gg2)));
                     bb = max(0, min(255, cint(frac*bb1 + (1-frac)*bb2)));
                     col = RGBvalue(rr,gg,bb,g.bitsperpixel);
                     setpixelonimage(k+xx1, j+yy1-d/2+width/2,  col, mode,0,-1,0,0,1,0,1);
               }                   
               break;
       }
       delete[] ellipse_x1;
       delete[] ellipse_x2;
  }
}


//-------------------------------------------------------------------//
// draw_arc                                                          //
// xx1 and yy1 are upper left of rounded rectangle                   //
// x,y is enter of arc                                               //
//-------------------------------------------------------------------//
void draw_arc(int xx1, int yy1, int x, int y, int orientation, int width, 
              int color, int mode)
{
  int d = g.diameter;
  int r = d/2;
  int xx, yy, px, py, j=0,k=0, w,w2;
  int sum;

  py = r<<1;
  px = 0;
  sum = -(r<<1);
  w = cint(width / 2);
  w2=w;
  if(width %2) w2++;
  while (px <= py)
  {   if ( !(px & 1))
      {  
         for(j=-w; j<w2; j++)
         for(k=-w; k<w2; k++)
         {
             xx = x + k + (px>>1);
             yy = y + j + (py>>1);
             if(orientation==LRIGHT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x2[yy-yy1] = xx-xx1;
             }
             yy = y + j - (py>>1);
             if(orientation==URIGHT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x2[yy-yy1] = xx-xx1;
             }
             xx = x + k - (px>>1);
             if(orientation==ULEFT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x1[yy-yy1] = xx-xx1;
             }
             yy = y + j + (py>>1);
             if(orientation==LLEFT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x1[yy-yy1] = xx-xx1;
             }


             xx = x + k + (py>>1);
             yy = y + j + (px>>1);
             if(orientation==LRIGHT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x2[yy-yy1] = xx-xx1;
             }
             yy = y + j - (px>>1);
             if(orientation==URIGHT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x2[yy-yy1] = xx-xx1;
             }
             xx = x + k - (py>>1);
             if(orientation==ULEFT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x1[yy-yy1] = xx-xx1;
             }
             yy = y + j + (px>>1);
             if(orientation==LLEFT)
             {    setpixelonimage(xx,yy,color,mode,0,-1,0,0,1,0,1);
                  if(mode==SET && g.circle_fill && yy>=yy1) ellipse_x1[yy-yy1] = xx-xx1;
             }
         }
      }
      sum +=px++;
      sum += px;
      if (sum >= 0)
      {  sum-=py--;
         sum -=py;
      }
  }
}
