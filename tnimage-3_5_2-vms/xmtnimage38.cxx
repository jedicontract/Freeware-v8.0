//--------------------------------------------------------------------------//
// xmtnimage38.cc                                                           //
// Latest revision: 03-17-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Objects                                                                  //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
const int DISPLAY_FILLED_REGION = 1;  // Makes it slower
XYData sdata;
int select_image_number, selectedx, selectedy;
extern int in_spot_densitometry;

//--------------------------------------------------------------------------//
// identify_object x,y are screen coordinates                               //
//--------------------------------------------------------------------------//
void identify_object(int x, int y)
{
   int ino,size,minx,miny,maxx,maxy,top,left,right,bottom,use_alpha=0;
   ino = g.imageatcursor;

   if(z[ino].is_zoomed) return;
   printstatus(BUSY);   

   if(g.selectedimage==-1)    ////  Restrict found objects to selected region
   {   minx = max(0, g.selected_ulx - z[ino].xpos);
       miny = max(0, g.selected_uly - z[ino].ypos);
       maxx = min(z[ino].xsize, g.selected_lrx - z[ino].xpos);
       maxy = min(z[ino].ysize, g.selected_lry - z[ino].ypos);
   }else                      ////  Enough for entire image
   {   minx = 0;
       miny = 0;
       maxx = z[ino].xsize;
       maxy = z[ino].ysize;
   }
   if(ino>=0)
   {      if(get_alpha_bit(ino,z[ino].cf,x-z[ino].xpos,y-z[ino].ypos))
          use_alpha=1;
   }else
   {      if(get_alpha_bit(-1,0,x,y))
          use_alpha=1;
   }

   if(g.region!=NULL) 
   {   free(g.region);
       free(g.region_1d);
       g.region=NULL;
       g.region_1d=NULL;
       g.region_ino = -1;
   }
   size = z[ino].xsize * z[ino].ysize;
   g.region_1d = (uchar*)malloc(size*sizeof(uchar));
   g.region = make_2d_alias(g.region_1d, z[ino].xsize, z[ino].ysize);
   memset(g.region_1d,0,size*sizeof(uchar));

   x -= z[ino].xpos;         ////  Make sure coordinates are relative to upper
   y -= z[ino].ypos;         ////  left of image


   //// If user clicked on a point where alpha was set, use alpha channel.
   //// If it was a rectangular region, assume it was a label, find all
   ////    pixels in region of same color.
   //// Otherwise, use flood fill to find region.

   if(use_alpha)
      find_alpha(x, y, ino, minx, miny, maxx, maxy, left, top, right, bottom);
   else if(g.selectedimage==-1)
      find_region_by_color(x, y, ino, minx, miny, maxx, maxy, left, top, right, bottom);
   else
      find_region(x, y, ino, minx, miny, maxx, maxy, left, top, right, bottom);
   
   g.selected_ulx = left + z[ino].xpos;
   g.selected_uly = top + z[ino].ypos;
   g.selected_lrx = right + z[ino].xpos;
   g.selected_lry = bottom + z[ino].ypos;

   g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
   g.uly = zoom_y_coordinate(g.selected_uly, ino);
   g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
   g.lry = zoom_y_coordinate(g.selected_lry, ino);

   g.region_ulx = g.ulx;     ////  Numbers guaranteed to be safe for accessing
   g.region_uly = g.uly;     ////  contents of g.region.
   g.region_lrx = g.lrx;
   g.region_lry = g.lry;
   g.region_ino = ino;
   sdata.n = 0;              ////  Erase any previous polygon

   g.selectedimage = -1;
   drawselectbox(OFF);
   g.selected_is_square=0;
   drawselectbox(ON);
   printstatus(NORMAL);   
}


//-------------------------------------------------------------------------//
// find_alpha    x,y are image coordinates                                 //
// mark anything in alpha channel within specified limits                  //
// called only by identify_object()                                        //
//-------------------------------------------------------------------------//
void find_alpha(int x, int y, int ino, int minx, int miny, int maxx, int maxy,
                 int &left, int &top, int &right, int &bottom)
{
   x=x; y=y;
   int f,i,j,point;  
   left = top = right = bottom = 0;
   if(!between(ino,0,g.image_count-1)) return;

   f=z[ino].cf;         

   //// Find region where alpha bit is set
   for(j=miny; j<maxy; j++)
   for(i=minx; i<maxx; i++)
   {
       if(get_alpha_bit(ino,f,i,j)) g.region[j][i]=1; 
       else g.region[j][i]=0;            
   }
   //// Mark border by setting 2nd bit.
   top = miny;
   bottom = maxy;
   left = minx;
   right = maxx;

   //// Find border
   for(j=top+1;j<bottom-1;j++)
   for(i=left+1;i<right-1;i++)
   {    point = g.region[j][i] & 1;
        if(!point)continue;
        if(point!=(g.region[j  ][i-1] & 1) || 
           point!=(g.region[j-1][i  ] & 1) ||
           point!=(g.region[j  ][i+1] & 1) ||
           point!=(g.region[j+1][i  ] & 1)) 
             g.region[j][i] = 3;
   }
}


//-------------------------------------------------------------------------//
// find_region_by_color    x,y are image coordinates                       //
// called only by identify_object()                                        //
//-------------------------------------------------------------------------//
void find_region_by_color(int x, int y, int ino, int minx, int miny, 
     int maxx, int maxy, int &left, int &top, int &right, int &bottom)
{
   int bpp,f,i,j,pointcolor,point,v;  
   left = top = right = bottom = 0;
   if(!between(ino,0,g.image_count-1)) return;

   f = z[ino].cf;         
   bpp = z[ino].bpp;         
   if(!between(x,0,z[ino].xsize-1) ||
      !between(y,0,z[ino].ysize-1)) return;
   pointcolor = pixelat(z[ino].image[f][y]+x*g.off[bpp], bpp);

   //// Find region of same color and set 1st bit.
   for(j=miny; j<maxy; j++)
   for(i=minx; i<maxx; i++)
   {    v = pixelat(z[ino].image[f][j]+i*g.off[bpp], bpp);
        if(v == pointcolor) g.region[j][i]=1; 
        else g.region[j][i]=0;            
   }
   top = miny;
   bottom = maxy;
   left = minx;
   right = maxx;

   //// Find border
   for(j=top+1;j<bottom-1;j++)
   for(i=left+1;i<right-1;i++)
   {    point = g.region[j][i] & 1;
        if(!point)continue;
        if(point!=(g.region[j  ][i-1] & 1) || 
           point!=(g.region[j-1][i  ] & 1) ||
           point!=(g.region[j  ][i+1] & 1) ||
           point!=(g.region[j+1][i  ] & 1)) 
              g.region[j][i] = 3;
   }
}


//-------------------------------------------------------------------------//
// find_region    x,y are image coordinates                                //
// called only by identify_object()                                        //
//-------------------------------------------------------------------------//
void find_region(int x, int y, int ino, int minx, int miny, int maxx, int maxy,
                 int &left, int &top, int &right, int &bottom)
{
   if(memorylessthan(32768)){  message(g.nomemory,3); return; } 
   static struct { int yy, xl, xr, dy; } buffer[5000];
   int bpp,f,i,j,c,dy,sp=0,point,start,x1,x2,rr,gg,bb,orr,ogg,obb;
   left = top = right = bottom = 0;
   if(!between(ino,0,g.image_count-1)) return;
   uint fc, v;
   f = z[ino].cf;
   RGB thresh;
   bpp = z[ino].bpp;
   if(!between(minx, 0, z[ino].xsize) ||
      !between(miny, 0, z[ino].ysize) ||
      !between(maxx, 0, z[ino].xsize) ||
      !between(maxy, 0, z[ino].ysize)){ fprintf(stderr, "Error in find_region\n"); return; }
   array<int> screen(z[ino].xsize+1, z[ino].ysize+1);  
   left=x; right=x; top=y; bottom=y;

   fc = pixelat(z[ino].image[f][y]+x*g.off[bpp], bpp);
   if(ino>=0 && z[ino].colortype==GRAY)
   {    thresh.red   = (g.object_threshold.red + g.object_threshold.green +
                        g.object_threshold.blue)/3 * (int)(g.maxvalue[bpp]/255.0);
        thresh.green = thresh.red;
        thresh.blue  = thresh.red;
   }else
   {    thresh.red   = g.object_threshold.red * g.maxred[bpp] / 255;
        thresh.green = g.object_threshold.green * g.maxgreen[bpp] / 255;
        thresh.blue  = g.object_threshold.blue * g.maxblue[bpp] / 255;
   }
   
   valuetoRGB(fc,orr,ogg,obb,bpp);
   for(j=miny; j<maxy; j++)
   for(i=minx; i<maxx; i++)
   {
       v = pixelat(z[ino].image[f][j]+i*g.off[bpp], bpp);
       if(ino>=0 && z[ino].colortype==GRAY)
       {    if(abs(v-fc)<thresh.red) screen.p[j][i]=0; 
            else screen.p[j][i]=1;
       }else
       {    valuetoRGB(v,rr,gg,bb,bpp);
            if(abs(rr-orr)<thresh.red &&
               abs(gg-ogg)<thresh.green &&
               abs(bb-obb)<thresh.blue) 
                 screen.p[j][i]=0; 
            else 
                screen.p[j][i]=1;          
       }
   }
   buffer[sp].yy   = y;
   buffer[sp].xl   = x; 
   buffer[sp].xr   = x;
   buffer[sp++].dy = 1;
   buffer[sp].yy   = y+1;
   buffer[sp].xl   = x;
   buffer[sp].xr   = x;
   buffer[sp++].dy = -1;

   while(sp>0)
   {      
       //// Check next lower item on stack
       dy = buffer[--sp].dy;
       if(sp<0 || sp>4999) break;
       y = buffer[sp].yy + dy;
       x1 = buffer[sp].xl;
       x2 = buffer[sp].xr;
       x = x1;
       left=min(x,left);
       right=max(x,right);
       top=min(y,top);
       bottom=max(y,bottom);
       //// Fill left until it hits a border color or y is off screen or it hits
       //// a color already filled
 
       if(between(y, miny, maxy-1))
       {  
          c = screen.p[y][x];
          while(between(x, minx, maxx-1) && c!=1)
          {    
             g.region[y][x]=1;
             screen.p[y][x]=1;
             x--;
             c = screen.p[y][x];
          }
       }
       x = max(0,x);
       left=min(x,left);
       right=max(x,right);

       //// Give user a chance to break out

       if(y >= maxy) dy = -dy; 
       if(keyhit()) if(getcharacter()==27) break;
       if(x >= x1 ) goto enter;
       start = x + 1;

       //// Check next higher item on stack

       if(start < x1)
       {    buffer[sp].yy   = y;
            buffer[sp].xl   = start;
            buffer[sp].xr   = x1 - 1;
            buffer[sp++].dy = -dy;
            if(sp<0 || sp>4999) break;
       }
       x = x1 + 1;
       left=min(x,left);
       right=max(x,right);
       top=min(y,top);
       bottom=max(y,bottom);
       do        
       {  
           //// Fill right until it hits a border color or a color already filled

           if(between(y, miny, maxy-1))
           {   
              c = screen.p[y][x];
              while(between(x, minx, maxx-1) && c!=1)
              {
                    g.region[y][x]=1;
                    screen.p[y][x]=1;
                    x++;
                    c = screen.p[y][x];
              }
           }
           left=min(x,left);
           right=max(x,right);
           buffer[sp].yy = y;
           buffer[sp].xl = start;
           buffer[sp].xr = x - 1;
           buffer[sp++].dy = dy;
           if(sp<0 || sp>4999) break;
           if (x > (x2 + 1))
           {   buffer[sp].yy = y;
               buffer[sp].xl = x2 + 1;
               buffer[sp].xr = x - 1;
               buffer[sp++].dy = -dy;
               if(sp<0 || sp>4999) break;
           }
enter:   
           //// Read right until it hits an non-border and non-filled space          
           x++;
           left=min(x,left);
           right=max(x,right);
            if(between(y, miny, maxy-1))
           {  c = screen.p[y][x];
              while(x<x2 && c==1)
              {   x++;
                  c = screen.p[y][x];
              } 
           }
           start = x;
           left=min(x,left);
           right=max(x,right);
       }while(x <= x2);
       if(sp>=4999){ message("Area too complex",ERROR); break; }       
   }

   //// Mark border by setting 2nd bit.
   top = min(maxy,max(0,top));
   bottom = min(maxy,max(0,bottom));
   left = min(maxx,max(0,left));
   right = min(maxx,max(0,right));

   //// Find border
   for(y=top+1;y<bottom-1;y++)
   for(x=left+1;x<right-1;x++)
   {    point = g.region[y][x] & 1;
        if(!point)continue;
        if(point!=(g.region[y  ][x-1] & 1) || 
           point!=(g.region[y-1][x  ] & 1) ||
           point!=(g.region[y  ][x+1] & 1) ||
           point!=(g.region[y+1][x  ] & 1)) 
             g.region[y][x] = 3;
   }
}           


//-------------------------------------------------------------------------//
// switch_selected_region                                                  //
//-------------------------------------------------------------------------//
void switch_selected_region(void)
{
   int i,j;
   int xsize = z[ci].xsize;
   int ysize = z[ci].ysize;
   int xpos = z[ci].xpos;
   int ypos = z[ci].ypos;
   int x1 = 0;
   int y1 = 0;
   int size = xsize * ysize;


   uchar *region_1d = (uchar*)malloc(size*sizeof(uchar));
   if(region_1d == NULL){ message(g.nomemory); return; }
   uchar **region = make_2d_alias(region_1d, xsize, ysize);
   if(region == NULL){ message(g.nomemory); free(region_1d); g.region_ino = -1; return; }

   drawselectbox(OFF);
   memset(region_1d,0,size*sizeof(uchar));
   
   for(j=y1; j<y1+ysize; j++)
   for(i=x1; i<x1+xsize; i++)
   {    
       region[j][i] = inside_irregular_region(i+xpos,j+ypos);
       if(region[j][i] == 1) region[j][i] = 0;
       else if(region[j][i] == 0) region[j][i] = 1;
   }

   g.selected_ulx = x1+xpos;
   g.selected_uly = y1+ypos;
   g.ulx = zoom_x_coordinate(g.selected_ulx, ci);
   g.uly = zoom_y_coordinate(g.selected_uly, ci);

   g.selected_lrx = x1+xpos+xsize-1;
   g.selected_lry = y1+ypos+ysize-1;
   g.lrx = zoom_x_coordinate(g.selected_lrx, ci);
   g.lry = zoom_y_coordinate(g.selected_lry, ci);

   g.region_ulx = g.ulx;     ////  Numbers guaranteed to be safe for accessing
   g.region_uly = g.uly;     ////  contents of g.region.
   g.region_lrx = g.lrx;
   g.region_lry = g.lry;
   g.region_ino = ci;

   g.selectedimage= -1;
   if(g.region!=NULL) 
   {   free(g.region);
       free(g.region_1d);
       g.region=NULL;
       g.region_1d=NULL;
   }
   g.region = region;
   g.region_1d = region_1d;
   message("Selected region switched");
   g.selected_is_square=0;
   drawselectbox(ON);
}

//-------------------------------------------------------------------//
//  reselect_area - mode can be SINGLE, MULTIPLE, or POLYGON         //
//-------------------------------------------------------------------//
void reselect_area(int mode)
{
  int k,x,y,ino,x1,y1,x2,y2;
  ino = select_image_number;
  int xpos = z[ino].xpos;
  int ypos = z[ino].ypos;
  int dx = xpos - selectedx;
  int dy = ypos - selectedy;
  x1 = xpos + z[ino].xsize;
  x2 = xpos ;
  y1 = ypos + z[ino].ysize;
  y2 = ypos ;
  g.ignore=1;  // ignore double clicking
  if(mode==POLYGON && sdata.n)
  {    for(k=0; k<sdata.n; k++)
       {  sdata.x[k] += dx;
          sdata.y[k] += dy;
          x1 = min(x1,sdata.x[k]);
          x2 = max(x2,sdata.x[k]);
          y1 = min(y1,sdata.y[k]);
          y2 = max(y2,sdata.y[k]);
       }   
       for(k=0; k<sdata.n-1; k++) lineselect(sdata.x[k],sdata.y[k],sdata.x[k+1],sdata.y[k+1]);
       if(sdata.n>0) lineselect(sdata.x[0],sdata.y[0],sdata.x[sdata.n-1],sdata.y[sdata.n-1]);
       message("Click on any point inside selection");
       getpoint(x,y);
       find_inside(x,y);
  }else 
  {  
       drawselectbox(ON);
       swap(x1,x2);
       swap(y1,y2);
  }

  g.ignore=0;
  selectedx = z[ino].xpos;
  selectedy = z[ino].ypos;
  g.region_ulx = x1-2;
  g.region_uly = y1-2;
  g.region_lrx = x2+2;
  g.region_lry = y2+2;
  g.selected_ulx = g.region_ulx;
  g.selected_uly = g.region_uly;
  g.selected_lrx = g.region_lrx;
  g.selected_lry = g.region_lry;

  g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
  g.uly = zoom_y_coordinate(g.selected_uly, ino);
  g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
  g.lry = zoom_y_coordinate(g.selected_lry, ino);

  repairimg(ino,0,0,z[ino].xsize,z[ino].ysize);
  if(DISPLAY_FILLED_REGION) repair(ino);
  g.getout = 0;         
  g.state=NORMAL;
  g.selectcurve = 0;
  g.selectedimage = -1;
  g.selected_is_square = 0;
  drawselectbox(OFF);
}


//-------------------------------------------------------------------------//
//  select_area - mode can be SINGLE, MULTIPLE, POLYGON, or POINTTOPOINT   //
//-------------------------------------------------------------------------//
void select_area(int mode)
{
  static int in_select_area = 0;
  static int hit = 0;
  static Widget www;
  int bpp,clicked=0,ino=ci,k,rx,ry,size=0,x,y,ox=0,oy=0,top,left,right,bottom,
      xstart=0, ystart=0, bad=0, drawing=0;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  if(in_select_area) return;
  if(g.getlabelstate) return;
  int nmax;
  nmax=2000; 
  Window rwin, cwin;
  uint mask;
  //// Leave allocated permanently so user can restore selected area
  if(!hit)
  {   sdata.x = new int[nmax]; if(sdata.x==NULL){ message(g.nomemory,ERROR); return; }
      sdata.y = new int[nmax]; 
           if(sdata.y==NULL){ delete[] sdata.x; message(g.nomemory,ERROR); return; }
      hit = 1;
  }
  sdata.v = NULL;
  sdata.n = 0;
  sdata.dims = 1;
  sdata.nmin = 0;
  sdata.nmax = nmax;
  sdata.duration = TEMP;
  sdata.wantpause = 0;
  sdata.wantmessage = 0;
  sdata.type = 3;
  sdata.win  = 0; // calculate window automatically on drawing area

  tempmessageopen(www);
  if(g.want_messages > 1)
  {
     switch(mode)
     {  case MULTIPLE:
           tempmessageupdate(www, (char*)"Select region(s) with mouse\nClick Cancel when finished"); 
           break;
        case SINGLE:
           tempmessageupdate(www, (char*)"Select region with mouse\nClick Main Cancel button to abort");
           break;
        case POLYGON:
           tempmessageupdate(www, (char*)"Select vertices\nSpace when finished\nClick Main Cancel button to abort");
           break;
        case POINTTOPOINT:
           tempmessageupdate(www, (char*)"Click on first point\nClick Main Cancel button to abort");
           break;
     }
  }

  //// Find out which image they want
  ino = whichimg(g.mouse_x, g.mouse_y, bpp);
  switchto(ino);
  if(!between(ino, 0, g.image_count)){ tempmessageupdate(www, (char*)"No image selected"); return; } 
  if(z[ino].is_zoomed) { tempmessageupdate(www, (char*)"Can\'t select on zoomed images"); return; }

  //// Put in globals in case reselect_area is called
  select_image_number = ino;
  selectedx = z[ino].xpos;
  selectedy = z[ino].ypos;

  ////  Reset region buffer

  size = z[ino].xsize * z[ino].ysize;
  if(g.region_1d != NULL) free(g.region_1d); 
  if(g.region!=NULL) free(g.region); 
  g.region=NULL;
  g.region_1d=NULL;

  g.region_1d = (uchar*)malloc(size*sizeof(uchar));
  if(g.region_1d == NULL){ message(g.nomemory); return; }
  g.region = make_2d_alias(g.region_1d, z[ci].xsize, z[ci].ysize);
  if(g.region == NULL){ message(g.nomemory); free(g.region_1d); g.region_ino = -1; return; }
  drawselectbox(OFF);
  memset(g.region_1d, 0, size*sizeof(uchar));
  g.state=SELECTCURVE;

  //// no returns past this point
  in_select_area = 1;
  left = z[ino].xsize + z[ino].xpos;
  right = 0;
  top = z[ino].ysize + z[ino].ypos;
  bottom = 0;
  g.getout=0;
  g.selectcurve=1;
  g.region_ino = ino;

  ////  Region is defined in screen coordinates but array is filled from 
  ////  upper left of image

  ////  Initial values
  g.region_ulx = z[ino].xpos;
  g.region_uly = z[ino].ypos;
  g.region_lrx = z[ino].xpos + z[ino].xsize;
  g.region_lry = z[ino].ypos + z[ino].ysize;
  g.ignore=1;  // ignore double clicking

  ////  Start drawing
  drawing = 1;
  while(drawing)
  {  
     switch(mode)
     {  case POINTTOPOINT:
               sdata.type = 9;
        case POLYGON:
               sdata.n = 0;    
               tempmessageupdate(www, (char*)"Select vertices\nSpace when finished\nClick Main Cancel button to abort");
               bezier_curve_start(&sdata, NULL);
               //// This is necessary in case user presses Esc on a torn-off menu
               g.block++;
               while(g.bezier_state==CURVE)
                    XtAppProcessEvent(XtWidgetToApplicationContext(g.drawing_area),XtIMAll);
               g.block = max(0, g.block-1);
               for(k=0; k<sdata.n-1; k++) lineselect(sdata.x[k],sdata.y[k],sdata.x[k+1],sdata.y[k+1]);
               if(sdata.n>0) lineselect(sdata.x[0],sdata.y[0],sdata.x[sdata.n-1],sdata.y[sdata.n-1]);
               tempmessageupdate(www, (char*)"Click on any point inside selection");
               XQueryPointer(g.display, g.main_window, &rwin, &cwin, &rx, &ry, &x, &y, &mask);
               check_event();
               usleep(10000);
               if(sdata.n>0) getpoint(x,y);
               usleep(10000);
               if(ci != ino) 
               {    bad=1; 
                    tempmessageupdate(www, (char*)"Error: current image\nunexpectedly changed"); 
                    drawing=0; 
                    break; 
               }
               find_inside(x, y);
               g.selected_ulx = g.region_lrx;
               g.selected_uly = g.region_lry;
               g.selected_lrx = g.region_ulx;
               g.selected_lry = g.region_uly;
               for(k=0; k<sdata.n; k++)
               {    g.selected_ulx = min(sdata.x[k], g.selected_ulx);
                    g.selected_uly = min(sdata.y[k], g.selected_uly);
                    g.selected_lrx = max(sdata.x[k], g.selected_lrx);
                    g.selected_lry = max(sdata.y[k], g.selected_lry);
               }
               drawing=0; 
               break;
        case SINGLE:
        case MULTIPLE:
               g.block++;
               XtAppProcessEvent(g.app, XtIMAll);
               g.state=SELECTCURVE;
               ////  Get screen coordinates
               ////  Pause so it gets correct information
               usleep(1000);
               XQueryPointer(g.display, g.main_window, &rwin, &cwin, &rx, &ry, &x, &y, &mask);
               usleep(1000);
               g.block--;
               if(g.getout){ drawing=0; break; }
               if(rwin && cwin)  //// cwin is 0 if mouse is off image, e.g. dragging the dialog box
               {  
                    if(mask && Button1Mask) 
                    {   if(!clicked){ xstart=x; ystart=y; }
                        clicked = 1;
                        if(whichimage(x,y,bpp) != ino) continue;
                        if(ox||oy) lineselect(x,y,ox,oy); 
                        left = min(x,left);
                        right = max(x,right);
                        top = min(y,top);
                        bottom = max(y,bottom);
                        ox=x; oy=y;
                    }else if(clicked)
                    {   g.region_ulx = left-2;
                        g.region_uly = top-2;
                        g.region_lrx = right+2;
                        g.region_lry = bottom+2;
                        lineselect(x,y,xstart,ystart); // Close polygon
                        if(g.want_messages > 1)
                            tempmessageupdate(www, (char*)"Click on any point\ninside selection");
                        usleep(10000);
                        XtAppProcessEvent(g.app, XtIMAll);
	                getpoint(x,y); // changes g.selected_ulx, etc.            
                        usleep(10000);
                        XtAppProcessEvent(g.app, XtIMAll);
                        g.selected_ulx = left-2;
                        g.selected_uly = top-2;
                        g.selected_lrx = right+2;
                        g.selected_lry = bottom+2;
                        if(ci != ino) 
                        {    bad=1; 
                             tempmessageupdate(www, (char*)"Error: current image\nunexpectedly changed"); 
                             drawing=0; 
                             break; 
                        }
                        find_inside(x,y);
                        clicked=0;
                        ox=0; oy=0;
                        if(mode == MULTIPLE) 
                             tempmessageupdate(www, (char*)"Select next region\nClick Cancel when finished"); 
                        else { drawing=0; break; }
                    }
               }else clicked=0; 
               break;
          default: 
               break;
     }
     if(g.getout) break;
  }

  g.ignore=0;  // re-enable double clicking

  if(bad)
  {   free(g.region_1d); 
      free(g.region); 
      g.region=NULL;
      g.region_1d=NULL;
  }else
  {   XtSetKeyboardFocus(g.main_widget, g.drawing_area);
      g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
      g.uly = zoom_y_coordinate(g.selected_uly, ino);
      g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
      g.lry = zoom_y_coordinate(g.selected_lry, ino);

      g.selectedimage = -1;
      g.selected_is_square = 0;
  }
  repairimg(ino,0,0,z[ino].xsize,z[ino].ysize);
  if(DISPLAY_FILLED_REGION) repair(ino);
  g.state=NORMAL;
  g.selectcurve = 0;
  tempmessageclose(www);
  in_select_area = 0;
  return;
}


//-------------------------------------------------------------------//
// lineselect  x,y are screen coordinates  define boundary by setting//
//             g.region[][] to 3                                     //
//-------------------------------------------------------------------//
void lineselect(int xs, int ys, int xe, int ye)
{
   int decision, dx, dy, x_sign, y_sign, x, y;
   int x1 = z[g.region_ino].xpos;
   int y1 = z[g.region_ino].ypos;
   int x2 = x1 + z[g.region_ino].xsize;
   int y2 = y1 + z[g.region_ino].ysize;
   dx = abs(xe - xs);
   dy = abs(ye - ys);
 
   if(dx > dy)
   {  if(xs > xe){ swap(xs,xe); swap(ys,ye); }
      if(ye - ys < 0) y_sign = -1; else y_sign = 1;
      for (x=xs,y=ys,decision=0; x<=xe; x++,decision+=dy)
      {   if(decision>=dx){ decision -= dx; y+=y_sign; }
          if(between(x, x1, x2-1) &&
             between(y, y1, y2-1)) 
                g.region[y-y1][x-x1] = 3;
          setpixel(x,y,g.maxcolor,SET);
      } 
   }else
   {  if(ys > ye){ swap(ys,ye); swap(xs,xe); }
      if(xe - xs < 0) x_sign = -1; else x_sign = 1;
      for (x=xs,y=ys,decision=0; y<=ye; y++,decision+=dx)
      {   if(decision>=dy){ decision -= dy; x+=x_sign;  }
          if(between(x, x1, x2-1) &&
             between(y, y1, y2-1)) 
              g.region[y-y1][x-x1] = 3;
          setpixel(x,y,g.maxcolor,SET);
      }
   }
   return;
}


//-------------------------------------------------------------------//
// calculate_selected_region                                         //
// crude way of estimating inside of selected area                   //
// Fast and compact but doesn't work as well as find_inside().       //
//-------------------------------------------------------------------//
void calculate_selected_region(void)
{
   int i,j,inside=0, border=0;
   int x1 = z[g.region_ino].xpos;
   int y1 = z[g.region_ino].ypos;
   for(j=g.region_uly; j<g.region_lry; j++)
   {  inside=0;
      border=0;
      for(i=g.region_ulx; i<g.region_lrx; i++)
      {  
         if(g.region[j-y1][i-x1]==3)
         {   if(!border) inside = 1-inside;
             border = 1;
         }
         else
         {   //// Make sure region has boundary
             if(j>y1 && i>x1)
             {    if(g.region[j-y1-1][i-x1]==1 || g.region[j-y1][i-x1-1]==1) inside=1;  
                  if(g.region[j-y1-1][i-x1]==0 || g.region[j-y1][i-x1-1]==0) inside=0; 
             }
             if(inside) g.region[j-y1][i-x1]=1; 
             else g.region[j-y1][i-x1]=0; 
             border = 0;
         }
      }
      ////  If it gets to end of line & still inside, go back
      if(inside) for(i=g.region_lrx; i>g.region_ulx; i--) 
      {   if(g.region[j-y1][i-x1]==3) break;
          g.region[j-y1][i-x1]=0;       
      }
   }
}


//-------------------------------------------------------------------------//
// inside_irregular_region x,y are screen coordinates                      //
//-------------------------------------------------------------------------//
int inside_irregular_region(int x, int y)
{
   int ii=0,dx,dy,xsize,ysize;
   dx = z[g.region_ino].xpos;
   dy = z[g.region_ino].ypos;
   xsize = z[g.region_ino].xsize;
   ysize = z[g.region_ino].ysize;
   if(g.region==NULL) return 0;
   ////  g.region[y][x] is 3 if border, 1 if selected, else 0
   if(between(y-dy, 0, ysize-1) &&
      between(x-dx, 0, xsize-1) &&
      between(x, g.region_ulx, g.region_lrx-1) &&  
      between(y, g.region_uly, g.region_lry-1) && g.region[y-dy][x-dx]) 
        ii = g.region[y-dy][x-dx];
   return(ii);
}


//-------------------------------------------------------------------------//
// show_selected                                                           //
//-------------------------------------------------------------------------//
void show_selected(int ino)
{
   int i,j;
   if(g.selected_is_square) 
   {   for(j=g.uly; j<=g.lry; j++)
       for(i=g.ulx; i<=g.lrx; i++)
          setpixel(i+z[ino].xpos,j+z[ino].ypos,g.maxcolor/4,SET);
   }
   else
   {   for(j=g.region_uly; j<g.region_lry; j++)
       {   for(i=g.region_ulx; i<g.region_lrx; i++)
           {   if(inside_irregular_region(i,j))
                 setpixel(i,j,g.maxcolor,SET);
               else setpixel(i,j,0,SET);
           }
       }
   }
   sleep(2);
   redraw(ino);
}


//-------------------------------------------------------------------------//
// shift_selected_area  - move selected area around in buffer              //
//-------------------------------------------------------------------------//
void shift_selected_area(int oldx, int oldy, int &newx, int &newy)
{
  int i,j,i2,j2,ixsize,iysize,isize,sxsize,sysize,xmax,ymax,ino,x,y;
  ino = g.region_ino;
  int xsize = z[ino].xsize;
  int ysize = z[ino].ysize;
  int xpos = z[ino].xpos;
  int ypos = z[ino].ypos;
  int dx, dy;
  ixsize = g.region_lrx - g.region_ulx + 1;
  iysize = g.region_lry - g.region_uly + 1;
  isize = ixsize * iysize; 
  sxsize = g.selected_lrx - g.selected_ulx + 1;
  sysize = g.selected_lry - g.selected_uly + 1;

  newx = max(newx, sxsize/2+1);
  newy = max(newy, sysize/2+1);
  newx = min(newx, xsize-sxsize/2-1);
  newy = min(newy, ysize-sysize/2-1);

  dx = newx - oldx;
  dy = newy - oldy;

  uchar *temp_1d = (uchar*)malloc(isize*sizeof(uchar));
  if(temp_1d == NULL){ message(g.nomemory); return; }
  uchar **temp = make_2d_alias(temp_1d, ixsize, iysize);
  if(temp == NULL){ message(g.nomemory); free(temp_1d); return; }

  ////  Make a copy of only the selected region & erase the original.
  ////  region[][] is 0..size; g.region_ulx, etc are z[ino].xpos..xpos+xsize.
  for(j=0; j<iysize; j++)
  for(i=0; i<ixsize; i++)
  {
      x = i+g.region_ulx-xpos;
      y = j+g.region_uly-ypos;
      if(x<0 || y<0) continue;
      if(x>=xsize || y>=ysize) continue;
      temp[j][i] = g.region[y][x];
      g.region[y][x] = 0;
  }
     
  ////  Paste the copy to new location
  for(j=0; j<iysize; j++)
  for(i=0; i<ixsize; i++)
  {   j2 = j + dy + g.region_uly-ypos;
      i2 = i + dx + g.region_ulx-xpos;
      if(between(i2,0,z[ino].xsize-1) && between(j2,0,z[ino].ysize-1)) 
          g.region[j2][i2] = temp[j][i];
  }

  xmax = z[ino].xpos + z[ino].xsize;
  ymax = z[ino].ypos + z[ino].ysize;
  g.region_ulx += dx;   
  g.region_uly += dy;   
  g.region_lrx += dx;   
  g.region_lry += dy;   
  g.selected_ulx += dx;
  g.selected_uly += dy;
  g.selected_lrx += dx;
  g.selected_lry += dy;
  g.ulx += dx;
  g.uly += dy;
  g.lrx += dx;
  g.lry += dy;
  free(temp); 
  free(temp_1d); 
}


//-------------------------------------------------------------------------//
// find_inside  all values are screen coordinates                          //
//-------------------------------------------------------------------------//
void find_inside(int x, int y)
{
   static struct { int yy, xl, xr, dy; } buffer[5000];
   int c, dy, sp=0, start, x1, x2;
   buffer[sp].yy   = y;
   buffer[sp].xl   = x; 
   buffer[sp].xr   = x;
   buffer[sp++].dy = 1;
   buffer[sp].yy   = y+1;
   buffer[sp].xl   = x;
   buffer[sp].xr   = x;
   buffer[sp++].dy = -1;

   int xpos = z[g.region_ino].xpos;
   int ypos = z[g.region_ino].ypos;
   //int xsize = z[g.region_ino].xsize;
   int ysize = z[g.region_ino].ysize;
   int minx = g.region_ulx+2;
   int miny = g.region_uly+1;
   int maxx = g.region_lrx-1;
   int maxy = g.region_lry-1;

   while(sp>0)
   {      
       //// Check next lower item on stack

       dy = buffer[--sp].dy;
       if(sp<0 || sp>4999) break;
       y = buffer[sp].yy + dy;
       y = max(ypos+1,y);
       y = min(ysize+ypos-1,y);
       x1 = buffer[sp].xl;
       x2 = buffer[sp].xr;
       x = x1;
       //// Fill left until it hits a border color or y is off screen or it hits
       //// a color already filled
 
       c = g.region[y-ypos][x-xpos];
       if(between(y, miny, maxy-1))
       while(between(x,minx,maxx-1) && c!=3 && c!=1)
       {  
              if(DISPLAY_FILLED_REGION) setpixel(x,y,255,SET);
              g.region[y-ypos][x-xpos] = 1;  
              x--;
              c = g.region[y-ypos][x-xpos]; 
       }
       if(y<=miny)break;   // fix
       if(y>maxy)dy=-dy; 
       if(x >= x1) goto enter;
       start = x + 1;

       //// Check next higher item on stack

       if(start < x1)
       {    buffer[sp].yy   = y;
            buffer[sp].xl   = start;
            buffer[sp].xr   = x1 - 1;
            buffer[sp++].dy = -dy;
            if(sp<0 || sp>4999) break;
       }
       x = x1 + 1;
       do        
       {  
           //// Fill right until it hits a border color or a color already filled

           c = g.region[y-ypos][x-xpos];
           if(between(y, miny, maxy-1))
           while(between(x,minx,maxx-1) && c!=3 && c!=1)
           {  
               if(DISPLAY_FILLED_REGION) setpixel(x,y,255,SET);
               g.region[y-ypos][x-xpos] = 1;
               x++;
               c = g.region[y-ypos][x-xpos];
           }
           buffer[sp].yy = y;
           buffer[sp].xl = start;
           buffer[sp].xr = x - 1;
           buffer[sp++].dy = dy;
           if(sp<0 || sp>4999) break;
           if (x > (x2 + 1))
           {   buffer[sp].yy = y;
               buffer[sp].xl = x2 + 1;
               buffer[sp].xr = x - 1;
               buffer[sp++].dy = -dy;
               if(sp<0 || sp>4999) break;
           }
enter:   
           //// Read right until it hits an non-border and non-filled space
           
           x++;
           c = g.region[y-ypos][x-xpos];
           if(between(y, miny, maxy-1))
           while( x <= x2 && (c==3 || c==1) )
           {   x++;
               c = g.region[y-ypos][x-xpos];
           }
           start = x;
        }
        while(x <= x2);
        if(sp>=4999){ message("Area too complex",ERROR); break; }       
   }
}           
