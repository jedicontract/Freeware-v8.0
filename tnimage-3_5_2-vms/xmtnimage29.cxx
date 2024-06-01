//--------------------------------------------------------------------------//
// xmtnimage29.cc                                                           //
// fill                                                                     //
// Latest revision: 03-28-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;
int square=0;
int remove_gradient_type = 0;
int remove_gradient_source = 0;
int ulcolor=0, urcolor=0, llcolor=0, lrcolor=0;

//-------------------------------------------------------------------------//
//  fillregion - fill a region of the screen with a color                  //
//-------------------------------------------------------------------------//
void fillregion(void)
{
   int boxtype,bpp,ino,oino,j,k,x1,y1,x2,y2;   
   static Dialog *dialog;

   oino = g.selectedimage;
   g.region_ino = ci;
   x1 = g.selected_ulx;
   x2 = g.selected_lrx;
   y1 = g.selected_uly;
   y2 = g.selected_lry;
   if(x1==x2 || y1==y2)
   { message("Please select a region\nor image to fill",ERROR); return; }

   ////  Determine bpp. This will determine whether to get 1 value
   ////  or 3 values for a rgb in multiclickbox.
   
   ino = whichimage((x1+x2)/2, (y1+y2)/2, bpp);
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   dialog = new Dialog;
   g.fill_color = min(g.fill_color, (uint)g.maxvalue[bpp]);
   g.fill_maxborder = min(g.fill_maxborder, (uint)g.maxvalue[bpp]);
   g.fill_minborder = min(g.fill_minborder, (uint)g.maxvalue[bpp]);
   g.fill_maxgrad = min(g.fill_maxgrad, (uint)g.maxvalue[bpp]);
   g.fill_mingrad = min(g.fill_mingrad, (uint)g.maxvalue[bpp]);

   //// Radios
   strcpy(dialog->title,"Fill Region");

   strcpy(dialog->radio[0][0],"Fill type");             
   strcpy(dialog->radio[0][1],"Solid");                // wantgradient=0
   strcpy(dialog->radio[0][2],"Vert.Gradient");        // wantgradient=1
   strcpy(dialog->radio[0][3],"Horiz.Gradient");       // wantgradient=2
   dialog->radioset[0] = g.fill_type + 1;
   dialog->radiono[0] = 4;

   strcpy(dialog->radio[1][0],"User Mode");             
   strcpy(dialog->radio[1][1],"Preselected area");             
   strcpy(dialog->radio[1][2],"Interactive"); 
   dialog->radioset[1] = g.fill_usermode;
   dialog->radiono[1]=3;

   dialog->radiono[2]=0;
   dialog->radiono[3]=0;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

   //// Check boxes
 
   strcpy(dialog->boxes[0],"Fill Parameters");
   strcpy(dialog->boxes[1],"Solid fill color");
   strcpy(dialog->boxes[2],"Start gradient color");
   strcpy(dialog->boxes[3],"End gradient color");
   strcpy(dialog->boxes[4],"Lower bound. color");
   strcpy(dialog->boxes[5],"Upper bound. color");

   dialog->boxtype[0]=LABEL;
   if(bpp==8) boxtype = INTCLICKBOX; else boxtype = RGBCLICKBOX;
   dialog->boxtype[1] = boxtype;
   dialog->boxtype[2] = boxtype;
   dialog->boxtype[3] = boxtype;
   dialog->boxtype[4] = boxtype;
   dialog->boxtype[5] = boxtype;

   dialog->boxmin[k] = 0; 
   for(k=1; k<=5; k++)
   switch(z[ino].colortype)
   {   case GRAY:    dialog->boxmax[k] = (int)g.maxvalue[bpp]; break;
       case INDEXED: dialog->boxmax[k] = (int)g.maxvalue[bpp]; break;
       case COLOR:   dialog->boxmax[k] = g.maxgreen[bpp]; break;
   }
 
   sprintf(dialog->answer[1][0],"%d",g.fill_color);
   sprintf(dialog->answer[2][0],"%d",g.fill_mingrad);
   sprintf(dialog->answer[3][0],"%d",g.fill_maxgrad);
   sprintf(dialog->answer[4][0],"%d",g.fill_minborder);
   sprintf(dialog->answer[5][0],"%d",g.fill_maxborder);

   dialog->bpp = bpp;
   dialog->noofradios=2;
   dialog->noofboxes=6;
   dialog->helptopic=9;  
   dialog->f1 = fillcheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = fillfinish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 440; 
   dialog->height = 0; // calculate automatically
   dialog->want_changecicb = 0;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path,".");
   printstatus(FILL);
   dialog->bpp = bpp;
   dialog->param[10] = oino;
   dialog->param[11] = x1;
   dialog->param[12] = x2;
   dialog->param[13] = y1;
   dialog->param[14] = y2;
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
}


//--------------------------------------------------------------------------//
//  fillfinish                                                              //
//--------------------------------------------------------------------------//
void fillfinish(dialoginfo *a)
{
   if(g.selectedimage == -1) drawselectbox(ON);
   printstatus(NORMAL);
   int oino = a->param[10];
   int x1 = a->param[11];
   int x2 = a->param[12];
   int y1 = a->param[13];
   int y2 = a->param[14];
   if(oino<0 && g.bitsperpixel==8) repairimg(-1,x1,y1,x2,y2); 
   if(oino>=0 && (z[oino].bpp==8 || g.bitsperpixel==8)) repair(oino); 
   redrawscreen(); 
}


//--------------------------------------------------------------------------//
//  fillcheck                                                               //
//--------------------------------------------------------------------------//
void fillcheck(dialoginfo *a, int radio, int box, int boxbutton)
{
  int j,k;box=box;boxbutton=boxbutton;
  g.fill_type      = a->radioset[0] - 1;
  g.fill_usermode  = a->radioset[1];
  g.fill_color     = atoi(a->answer[1][0]);
  g.fill_mingrad   = atoi(a->answer[2][0]);
  g.fill_maxgrad   = atoi(a->answer[3][0]);
  g.fill_minborder = atoi(a->answer[4][0]);
  g.fill_maxborder = atoi(a->answer[5][0]);

  if(radio==0) 
  for(k=0;k<=3;k++)            // 3 boxes R,G,B
  {
    if(a->radioset[0]==1)      // Solid
    {   if(a->boxwidget[1][k]) XtSetSensitive(a->boxwidget[1][k],True);
         for(j=2;j<=3;j++) 
              if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
    }else                      // Horiz or vertical gradient
    {  if(a->boxwidget[1][k]) XtSetSensitive(a->boxwidget[1][k],False);
       for(j=2;j<=3;j++) 
              if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
    }
  }
  for(k=0;k<=3;k++)            // 3 boxes R,G,B
  {
    if(a->radioset[1]==1)      // Normal
    {    for(j=4;j<=5;j++) 
              if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],False);
    }else                      // Interactive
    {  
      for(j=4;j<=5;j++) 
              if(a->boxwidget[j][k]) XtSetSensitive(a->boxwidget[j][k],True);
    }
  }
 
  //////////

  if(radio != -2) return;       // Return unless user clicked OK
  
  int bpp,x,y,x1,y1,x2,y2;
  g.getout=0; 
  drawselectbox(OFF);
  square = g.selected_is_square;
  bpp = a->bpp;

  if(!between(g.region_ino, 0, g.image_count-1)) { message("Please select a region first"); return; }
  if(g.fill_usermode == 1)
  {   x1 = y1 = 0;
      x2 = z[g.region_ino].xsize;
      y2 = z[g.region_ino].ysize;
      fill_gradient(x1,y1,x2,y2,g.fill_color,g.fill_type,bpp,g.fill_mingrad,g.fill_maxgrad);
  }else 
  {   message("Click inside area to fill\nClick cancel button when finished");
      while(!g.getout)
      {   getpoint(x,y);
          if(g.getout) break;
          x1 = z[g.region_ino].xpos;
          y1 = z[g.region_ino].ypos;
          x2 = x1 + z[g.region_ino].xsize;
          y2 = y1 + z[g.region_ino].ysize;
          g.fill_color = atoi(a->answer[1][0]);
          g.fill_type = a->radioset[0] - 1;
          g.fill_minborder = atoi(a->answer[4][0]);
          g.fill_maxborder = atoi(a->answer[5][0]);
          g.fill_mingrad = atoi(a->answer[2][0]);
          g.fill_maxgrad = atoi(a->answer[3][0]);
          fill_interactive(x,y,x1,y1,x2,y2,g.fill_color,g.fill_type,bpp,
             g.fill_minborder,g.fill_maxborder,g.fill_mingrad,g.fill_maxgrad);
      }
  }
}


//-------------------------------------------------------------------------//
// fill_gradient  Recursive fill is no longer needed since nonrectangular  //
//  areas can be selected.                                                 //
//-------------------------------------------------------------------------//
void fill_gradient(int minx, int miny, int maxx, int maxy, int fc, 
     int type, int bpp, int mingrad, int maxgrad)
{
   int j,k;
   int rmin, gmin, bmin, rmax, gmax, bmax, grad1, grad2;
   int ulx = g.region_ulx;
   int uly = g.region_uly;
   int lrx = g.region_lrx;
   int lry = g.region_lry;
   if(lrx==ulx || lry==uly)
   {   ulx = g.ulx;
       uly = g.uly;
       lrx = g.lrx;
       lry = g.lry;
   }
   if(type==1){ grad1=minx; grad2=maxx; } else { grad1=miny; grad2=maxy; }
   if((type!=0)&&(grad1==grad2)){  message("Bad gradient parameters"); return; }
   valuetoRGB(mingrad,rmin,gmin,bmin,bpp);
   valuetoRGB(maxgrad,rmax,gmax,bmax,bpp);
   for(j=uly;j<lry;j++)   
   for(k=ulx;k<lrx;k++)
   {      if(j<0 || k<0) continue;
          if(!g.selected_is_square && !inside_irregular_region(k-ulx+g.region_ulx,
               j-uly+g.region_uly)) continue;
          fc = gradfillcolor(fc,j,k,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax, 
               bmin,bmax,type,bpp); 
          setpixelonimage(k,j,fc,g.imode,bpp,-1,0,0,1);
   }
}


//-------------------------------------------------------------------------//
// fill_interactive - fills an area bounded by a range of designated colors//
//       when a point within the area is specified.                        //
//  x,y = start coordinates                                                //
//  fc  = fill color (if solid fill)                                       //
//  minborder, maxborder = minimum and maximum colors for border           //
//  mingrad, maxgrad = minimum and maximum colors to fill with             //
//  type :  0 = solid fill                                                 //
//          1 = horizontal gradient                                        //
//          2 = vertical gradient                                          //
//-------------------------------------------------------------------------//
void fill_interactive(int x, int y, int minx, int miny, int maxx, int maxy, 
          int fc, int type, int bpp, int minborder, int maxborder, int mingrad, 
	  int maxgrad)
{
   if(memorylessthan(32768)){  message(g.nomemory,3); return; } 
   static struct { int yy, xl, xr, dy; } buffer[5000];
   printstatus(FILL);
   int rmin, gmin, bmin, rmax, gmax, bmax, rr, gg, bb, grad1, grad2, 
       rminborder, gminborder, bminborder, rmaxborder, gmaxborder, bmaxborder,
       c, count=0, dy, ino, ibpp, sp=0, start, x1, x2, ok=1;

   ino = whichimage(x,y,bpp);
   if(type==1){ grad1=minx; grad2=maxx; } else { grad1=miny; grad2=maxy; }
   if(ino>-1 && z[ino].shell) switchto(ino);

   valuetoRGB(mingrad,rmin,gmin,bmin,bpp);
   valuetoRGB(maxgrad,rmax,gmax,bmax,bpp);
   valuetoRGB(minborder,rminborder,gminborder,bminborder,bpp);
   valuetoRGB(maxborder,rmaxborder,gmaxborder,bmaxborder,bpp);
   if((type!=0)&&(grad1==grad2)){  message("Bad gradient parameters"); return; }

   buffer[sp].yy   = y;
   buffer[sp].xl   = x; 
   buffer[sp].xr   = x;
   buffer[sp++].dy = 1;
   buffer[sp].yy   = y+1;
   buffer[sp].xl   = x;
   buffer[sp].xr   = x;
   buffer[sp++].dy = -1;
   while(sp>0 && !button_clicked())
   {      
       //// Check next lower item on stack

       dy = buffer[--sp].dy;
       if(sp<0 || sp>4999) break;
       y = buffer[sp].yy + dy;
       x1 = buffer[sp].xl;
       x2 = buffer[sp].xr;
       x = x1;
       //// Fill left until it hits a border color or y is off screen or it hits
       //// a color already filled
 
       c = (int)readpixelonimage(x,y,ibpp,ino);
       valuetoRGB(c,rr,gg,bb,bpp);
       fc = gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
            bmin,bmax,type,bpp);      
       if(between(y, miny, maxy))
       while(between(x,minx,maxx) && 
            ( !between(rr, rminborder, rmaxborder) || 
              !between(gg, gminborder, gmaxborder) || 
              !between(bb, bminborder, bmaxborder)) && 
              c!=fc)
       {    
          ok = 0;
          if(square) ok=1;
          else if(inside_irregular_region(x,y)) ok=1;
          if(ok) 
          {   setpixelonimage(x--,y,fc,SET,bpp);
              c = (int)readpixelonimage(x,y,ibpp,ino);
          }else 
          {   c=fc; x--; break; }

          valuetoRGB(c,rr,gg,bb,bpp);
          fc = gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
               bmin,bmax,type,bpp);
          check_event();
          if(!between(ino, 0, g.image_count-1)) g.getout=1;
          if(g.getout) break;
       }

       //// Give user a chance to break out

       if(!between(ino, 0, g.image_count-1)) break;
       if(type==0) if(y<=miny)break;   // fix
       if(y<miny)break; 
       if(y>maxy)dy=-dy; 
       if(keyhit()) if(getcharacter()==27) break;
       if(++count==10)
       {   count=0;
           printcoordinates(x,y,0);    	            
       }

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

           c = (int)readpixelonimage(x,y,ibpp,ino);
           valuetoRGB(c,rr,gg,bb,bpp);
           fc =  gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
                 bmin,bmax,type,bpp);
           if(between(y, miny, maxy))
           while(between(x,minx,maxx) && 
                ( !between(rr, rminborder, rmaxborder) || 
                  !between(gg, gminborder, gmaxborder) || 
                  !between(bb, bminborder, bmaxborder)) && 
                  c!=fc)
           {
                 ok = 0;
                 if(square) ok=1;
                 else if(inside_irregular_region(x,y)) ok=1;
                 if(ok) 
                 {   setpixelonimage(x++,y,fc,SET,bpp);
                     c = (int)readpixelonimage(x,y,ibpp,ino);
                 }else
                 {   c=fc; x++; break;
                 }

                 valuetoRGB(c,rr,gg,bb,bpp);
                 fc = gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
                      bmin,bmax,type,bpp);
                 check_event();
                 if(!between(ino, 0, g.image_count-1)) g.getout=1;
                 if(g.getout) break;
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
           c = (int)readpixelonimage(x,y,ibpp,ino);
           valuetoRGB(c,rr,gg,bb,bpp);
           fc =  gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
                 bmin,bmax,type,bpp);
           if(between(y, miny, maxy))
           while( x <= x2 && 
                ( between(rr, rminborder, rmaxborder) && 
                  between(gg, gminborder, gmaxborder) && 
                  between(bb, bminborder, bmaxborder)) ||
                  c==fc )
           {   x++;
               c = (int)readpixelonimage(x,y,ibpp,ino);
               if(!square && inside_irregular_region(x,y)) c=fc; 
               valuetoRGB(c,rr,gg,bb,bpp);
               fc = gradfillcolor(fc,x,y,minx,miny,maxx,maxy,rmin,rmax,gmin,gmax,
                    bmin,bmax,type,bpp);
               if(g.getout) break;
           }
           start = x;
        }
        while(x <= x2 && !button_clicked());
        if(sp>=4999){ message("Area too complex",ERROR); break; }       
   }
}           


//-------------------------------------------------------------------------//
// fill - bpp-independent fill for densitometry                            //
// fc, maxfc, and minfc are {0..1}                                         //
// Solid fill only                                                         //
// Use this for filling images, use fillbuf() for filling in an int array. //
//-------------------------------------------------------------------------//
void fill(int x, int y, int &xmin, int &ymin, int &xmax, int &ymax,
       double fc, double maxfc, double minfc, int &totalpixels, double &totalsignal)
{
   if(memorylessthan(32768)){  message(g.nomemory,3); return; } 
   printstatus(FILL);
   double THRESH=0.002;
   int count=0, color, dy, ino, bpp, sp=0, start, x1, x2;
   double c;
   int minimumx, minimumy, maximumx, maximumy;

   totalpixels = 0;
   totalsignal = 0.0;
   if(minfc>maxfc) fswap(minfc,maxfc)
   static struct 
   {   int yy, xl, xr, dy;
   }buffer[5000];

   ino = whichimage(x,y,bpp);

   ////  Limits at which filling should stop.
   
   minimumx=g.selected_ulx;
   minimumy=g.selected_uly;
   maximumx=g.selected_lrx;
   maximumy=g.selected_lry;
   if(z[ino].shell)
   {    switchto(ino);
        minimumx = min(g.xres-g.main_xpos, max(minimumx,-g.main_xpos));
        maximumx = min(g.xres-g.main_xpos, max(maximumx,-g.main_xpos));
        minimumy = min(g.yres-g.main_ypos, max(minimumy,-g.main_ypos));
        maximumy = min(g.yres-g.main_ypos, max(maximumy,-g.main_ypos));
   }


   xmin = maximumx-1; xmax = minimumx;
   ymin = maximumy-1; ymax = minimumy;
   buffer[sp].yy = y;
   buffer[sp].xl = x; 
   buffer[sp].xr = x;
   buffer[sp++].dy = 1;
   buffer[sp].yy = y + 1;
   buffer[sp].xl = x;
   buffer[sp].xr = x;
   buffer[sp++].dy = -1;

   while (sp > 0)
   {      
       //// Check next lower item on stack

       dy = buffer[--sp].dy;
       if(sp<0 || sp>4999) break;
       y = buffer[sp].yy + dy;
       ymin = min(y,ymin);
       ymax = max(y,ymax);
       x1 = buffer[sp].xl;
       x2 = buffer[sp].xr;
       x = x1;

       //// Fill left until it hits a border color or y is off screen or it hits
       //// a color already filled
 
       c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
       if(between(y, minimumy, maximumy))
       while(x>=minimumx && between(c,minfc,maxfc)==0 && fabs(c-fc)>THRESH)
       {    
          color = (int)(fc*g.maxvalue[bpp]);
          setpixelonimage(x--,y,color,SET,bpp,-1,0,0,1);
          totalpixels++;
          totalsignal += c;
          c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
       }

       //// Give user a chance to break out

       if(y<=minimumy)break;   // fix
       if(keyhit()) if(getcharacter()==27) break;
       if(++count==10)
       {   count=0;
           printcoordinates(x,y,0);    	            
       }

       xmin = min(x,xmin);
       xmax = max(x,xmax);
       if (x >= x1) goto enter;
       start = x + 1;

       //// Check next higher item on stack

       if (start < x1)
       {    buffer[sp].yy = y;
            buffer[sp].xl = start;
            buffer[sp].xr = x1 - 1;
            buffer[sp++].dy = -dy;
            if(sp<0 || sp>4999) break;
       }
       x = x1 + 1;
       xmin = min(x,xmin);
       xmax = max(x,xmax);
       do        
       {  
           //// Fill right until it hits a border color or a color already filled

           c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
           if(between(y, minimumy, maximumy))
           while(x<maximumx && between(c,minfc,maxfc)==0 && fabs(c-fc)>THRESH)
           {
                 color = (int)(fc*g.maxvalue[bpp]);
                 setpixelonimage(x++,y,color,SET,bpp,-1,0,0,1);
                 totalpixels++;
                 totalsignal += c;
                 c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
           }
           xmin = min(x,xmin);
           xmax = max(x,xmax);  
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
           c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
           if(between(y, minimumy, maximumy))
           while( x <= x2 && ( between(c, minfc, maxfc)!=0 || fabs(c-fc)<=THRESH ))
           {   x++;
               c = (double)readpixelonimage(x,y,bpp,ino)/(double)g.maxvalue[bpp];
           }
           xmin = min(x,xmin);
           xmax = max(x,xmax);
           start = x;
        }while (x <= x2);

        if(sp>=4999){ message("Area too complex",ERROR); break; }       
   }
   if(xmin<minimumx) xmin=minimumx;
   if(xmax>maximumx) xmax=maximumx;
   if(ymin<minimumy) ymin=minimumy;
   if(ymax>maximumy) ymax=maximumy;    
   return;
}           



//-------------------------------------------------------------------------//
// fillbuf - bpp-independent fill - same as fill() except uses int array.  // 
// Solid fill only                                                         //
// Use fill() instead for flood filling an image.                          //
// minimumx, minimumy, maximumx, maximumy are the coordinates beyond which //
//   filling should not occur.                                             //
// minfc and maxfc are the range of values that constitute the boundary.   //
//   Usually minfc=maxfc.  fc must be outside this range.                  //
// If signalfromimage is 1, totalsignal is measured from the corresponding //
//   coordinates in image no. ino instead of buf. This allows using an     //
//   array to determine the boundaries while reading signal from image.    //
// This changes the contents of buf, and returns the total signal and pixel//
//   count.                                                                //
//-------------------------------------------------------------------------//
void fillbuf(int **buf, int x, int y, int minimumx, int minimumy, int maximumx,
     int maximumy, int &xmin, int &ymin, int &xmax, int &ymax, int fc, 
     int maxfc, int minfc, int &totalpixels, double &totalsignal,
     int signalfromimage, int ino)
{
   int c=0, dy, f=0, sp=0, start, x1, x2;
   totalpixels = 0;
   totalsignal = 0.0;
   double pixeldensity = 0.0, totalpixelsignal=0.0;
   if(minfc>maxfc) swap(minfc,maxfc)
   static struct{ int yy, xl, xr, dy; } buffer[5000];
   if(signalfromimage) f = z[ino].cf;

   xmin = maximumx-1; xmax = minimumx;
   ymin = maximumy-1; ymax = minimumy;
   buffer[sp].yy = y;
   buffer[sp].xl = x; 
   buffer[sp].xr = x;
   buffer[sp++].dy = 1;
   buffer[sp].yy = y + 1;
   buffer[sp].xl = x;
   buffer[sp].xr = x;
   buffer[sp++].dy = -1;

   while(sp > 0)
   {   dy = buffer[--sp].dy;
       if(sp<0 || sp>4999) break;
       y = buffer[sp].yy + dy;
       ymin = min(y,ymin);
       ymax = max(y,ymax);
       x1 = buffer[sp].xl;
       x2 = buffer[sp].xr;
       x = x1;

       if(between(x, minimumx, maximumx-1))
           c = buf[y][x];
       if(signalfromimage) pixeldensity = pixeldensityat(x,y,f,ino);
       if(between(y, minimumy, maximumy-1))
       while(x>=1+minimumx && between(c,minfc,maxfc)==0 && c!=fc)
       {    
          buf[y][x--] = fc;
          totalpixels++;
          if(signalfromimage) 
          {   pixeldensity = pixeldensityat(x,y,f,ino);
              totalsignal += pixeldensity;
          }else totalsignal += (double)c;
          if(between(x, minimumx, maximumx-1))
              c = buf[y][x];
          if(signalfromimage) 
          {   totalpixelsignal += pixeldensity;
              pixeldensity = pixeldensityat(x,y,f,ino);
          }
       }
       if(y<=minimumy) break;
       if(y>=maximumy) break;
       if(keyhit()) if(getcharacter()==27) break;

       xmin = min(x,xmin);
       xmax = max(x,xmax);
       if (x >= x1) goto fillbufenter;
       start = x + 1;

       if (start < x1)
       {    buffer[sp].yy = y;
            buffer[sp].xl = start;
            buffer[sp].xr = x1 - 1;
            buffer[sp++].dy = -dy;
            if(sp<0 || sp>4999) break;
       }
       x = x1 + 1;
       xmin = min(x,xmin);
       xmax = max(x,xmax);
       do        
       {   if(between(x, minimumx, maximumx-1))
                 c = buf[y][x];
           if(signalfromimage) pixeldensity = pixeldensityat(x,y,f,ino);
           if(between(y, minimumy, maximumy-1))
           while(x<maximumx && between(c,minfc,maxfc)==0 && c!=fc)
           {     buf[y][x++] = fc;
                 totalpixels++;
                 if(signalfromimage)
                 {    pixeldensity = pixeldensityat(x,y,f,ino);
                      totalsignal += pixeldensity;
                 }else totalsignal += (double)c;
                 if(between(x, minimumx, maximumx-1))
                     c = buf[y][x];
                 if(signalfromimage) 
                 {   totalpixelsignal += pixeldensity;
                     pixeldensity = pixeldensityat(x,y,f,ino);
                 }
           }
           xmin = min(x,xmin);
           xmax = max(x,xmax);  
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
fillbufenter:   
           x++;
           if(between(x, minimumx, maximumx-1))
               c = buf[y][x];
           if(between(y, minimumy, maximumy-1))
           while( x <= x2 && ( between(c, minfc, maxfc)!=0 || c==fc ))
           {   x++;
               c = buf[y][x];
           }
           xmin = min(x,xmin);
           xmax = max(x,xmax);
           start = x;
        }while (x <= x2);

        if(sp>=4999){ message("Area too complex",ERROR); break; }       
   }
   if(xmin<minimumx) xmin=minimumx;
   if(xmax>maximumx) xmax=maximumx;
   if(ymin<minimumy) ymin=minimumy;
   if(ymax>maximumy) ymax=maximumy;    
   return;
}           



//-------------------------------------------------------------------------//
// gradfillcolor                                                           //
// Called by fill() during gradient fills to calculate the appropriate     //
// fill color.                                                             //
//-------------------------------------------------------------------------//
int gradfillcolor(int startcolor, int x, int y, 
                  int x1, int y1, int x2, int y2,
                  int rmin, int rmax, int gmin, 
                  int gmax, int bmin, int bmax, 
                  int type, int bpp)
{
  int f=startcolor;
  int dx=0,dy=0,fr,fg,fb;
  switch(type)
  { 
    case 0:        // no gradient
        break; 
    case 1:        // horizontal gradient
        dx = x2-x1;
        if(dx==0) return 0;
        fr = rmin + (x-x1)*(rmax - rmin)/dx;
        fg = gmin + (x-x1)*(gmax - gmin)/dx;
        fb = bmin + (x-x1)*(bmax - bmin)/dx;
        fr = max(0,min(fr,g.maxred[bpp]));
        fg = max(0,min(fg,g.maxgreen[bpp]));
        fb = max(0,min(fb,g.maxblue[bpp]));
        f = RGBvalue(fr,fg,fb,bpp);
        break;
    case 2:         // vertical gradient
        dy = y2-y1;
        fr = rmin + (y-y1)*(rmax - rmin)/dy;
        fg = gmin + (y-y1)*(gmax - gmin)/dy;
        fb = bmin + (y-y1)*(bmax - bmin)/dy;
        fr = max(0,min(fr,g.maxred[bpp]));
        fg = max(0,min(fg,g.maxgreen[bpp]));
        fb = max(0,min(fb,g.maxblue[bpp]));
        f = RGBvalue(fr,fg,fb,bpp);
        break;       
    default: break;
  }    
  return f;
}


//-------------------------------------------------------------------------//
// remove_gradient - adds or subtracts gradient                            //      
//-------------------------------------------------------------------------//
int remove_gradient(int noofargs, char **arg)
{
   noofargs=noofargs; arg=arg;
   int boxtype,bpp,ino,oino,j,k,x1,y1,x2,y2;   
   static Dialog *dialog;

   oino = g.selectedimage;
   g.region_ino = ci;
   x1 = g.selected_ulx;
   x2 = g.selected_lrx;
   y1 = g.selected_uly;
   y2 = g.selected_lry;
   if(x1==x2 || y1==y2)
   { message("No image selected",ERROR); return ERROR; }

   ////  Determine bpp. This will determine whether to get 1 value
   ////  or 3 values for a rgb in multiclickbox.
   
   ino = whichimage((x1+x2)/2, (y1+y2)/2, bpp);
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return ERROR; } 
   dialog = new Dialog;
   g.fill_color = min(g.fill_color, (uint)g.maxvalue[bpp]);
   g.fill_maxborder = min(g.fill_maxborder, (uint)g.maxvalue[bpp]);
   g.fill_minborder = min(g.fill_minborder, (uint)g.maxvalue[bpp]);
   g.fill_maxgrad = min(g.fill_maxgrad, (uint)g.maxvalue[bpp]);
   g.fill_mingrad = min(g.fill_mingrad, (uint)g.maxvalue[bpp]);

   //// Radios
   strcpy(dialog->title,"Gradient Removal");

   strcpy(dialog->radio[0][0],"Method");             
   strcpy(dialog->radio[0][1],"Addition");      
   strcpy(dialog->radio[0][2],"Subtraction");   
   strcpy(dialog->radio[0][3],"Multiplication");
   dialog->radioset[0] = remove_gradient_type + 1;
   dialog->radiono[0] = 4;

   strcpy(dialog->radio[1][0],"Coordinates scope");             
   strcpy(dialog->radio[1][1],"Entire Image");      
   strcpy(dialog->radio[1][2],"Selected area");   
   dialog->radioset[1] = remove_gradient_source + 1;
   dialog->radiono[1] = 3;

   dialog->radiono[2]=0;
   for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;

   //// Check boxes
 
   strcpy(dialog->boxes[0],"Values of gradient");
   strcpy(dialog->boxes[1],"Upper left");
   strcpy(dialog->boxes[2],"Upper right");
   strcpy(dialog->boxes[3],"Lower left");
   strcpy(dialog->boxes[4],"Lower right");

   dialog->boxtype[0]=LABEL;
   if(z[ino].colortype==GRAY || bpp==8) boxtype = INTCLICKBOX; else boxtype = RGBCLICKBOX;
   dialog->boxtype[1] = boxtype;
   dialog->boxtype[2] = boxtype;
   dialog->boxtype[3] = boxtype;
   dialog->boxtype[4] = boxtype;

   for(k=1; k<=4; k++)
   switch(z[ino].colortype)
   {   case GRAY:  
       case INDEXED:
                   dialog->boxmin[k] = 0; 
                   dialog->boxmax[k] = (int)g.maxvalue[bpp];
                   break;
       case COLOR:		
                   dialog->boxmin[k] = 0; 
                   dialog->boxmax[k] = g.maxgreen[bpp];
   }
 
   sprintf(dialog->answer[1][0],"%d",ulcolor);
   sprintf(dialog->answer[2][0],"%d",urcolor);
   sprintf(dialog->answer[3][0],"%d",llcolor);
   sprintf(dialog->answer[4][0],"%d",lrcolor);

   dialog->bpp = bpp;
   dialog->noofradios=2;
   dialog->noofboxes=5;
   dialog->helptopic=73;  
   dialog->f1 = remove_gradient_check;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = remove_gradient_finish;
   dialog->f5 = null;
   dialog->f6 = null;
   dialog->f7 = null;
   dialog->f8 = null;
   dialog->f9 = null;
   dialog->width = 440; 
   dialog->height = 0; // calculate automatically
   dialog->want_changecicb = 0;
   dialog->transient = 1;
   dialog->wantraise = 0;
   dialog->radiousexy = 0;
   dialog->boxusexy = 0;
   strcpy(dialog->path,".");
   dialog->bpp = bpp;
   dialog->param[10] = oino;
   dialog->param[11] = x1;
   dialog->param[12] = x2;
   dialog->param[13] = y1;
   dialog->param[14] = y2;
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog);
   return OK;
}


//--------------------------------------------------------------------------//
//  remove_gradient_finish                                                  //
//--------------------------------------------------------------------------//
void remove_gradient_finish(dialoginfo *a)
{
   a=a;
   if(g.selectedimage == -1) drawselectbox(ON);
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
//  remove_gradient_check                                                   //
//--------------------------------------------------------------------------//
void remove_gradient_check(dialoginfo *a, int radio, int box, int boxbutton)
{
  a=a;box=box;boxbutton=boxbutton;
  remove_gradient_type = a->radioset[0] - 1;
  remove_gradient_source = a->radioset[1] - 1;
  ulcolor = atoi(a->answer[1][0]);
  urcolor = atoi(a->answer[2][0]);
  llcolor = atoi(a->answer[3][0]);
  lrcolor = atoi(a->answer[4][0]);

  if(radio != -2) return;       // Return unless user clicked OK

  int b,ct,v,dx,dy,dr,dg,db,f,ino,bpp,x,y,x1,y1,x2,y2,oino,
      rr,gg,bb,rr1,gg1,bb1,rr2,gg2,bb2,rr3,gg3,bb3,rr4,gg4,bb4,
      rr5,gg5,bb5,rr6,gg6,bb6,di,ii1,ii2,ii3,ii4,ii5,ii6;
  double fx, fy;

  oino = ino = ci;
  bpp = z[ino].bpp;
  b = g.off[bpp];
  f = z[ino].cf;
  ct = z[ino].colortype;
  x1 = g.selected_ulx;
  x2 = g.selected_lrx;
  y1 = g.selected_uly;
  y2 = g.selected_lry;
  g.getout=0; 
  drawselectbox(OFF);

  valuetoRGB(ulcolor, rr1, gg1, bb1, bpp);
  valuetoRGB(urcolor, rr2, gg2, bb2, bpp);
  valuetoRGB(llcolor, rr3, gg3, bb3, bpp);
  valuetoRGB(lrcolor, rr4, gg4, bb4, bpp);
  dx = x2 - x1;
  dy = y2 - y1;
  ii1 = ulcolor;
  ii2 = urcolor;
  ii3 = llcolor;
  ii4 = lrcolor;
  if(dx<=0 || dy<=0) return;

  if(ct==GRAY)
  {  for(y=y1; y<=y2; y++)  
     {  if(g.getout){ g.getout=0; break; }
        for(x=x1; x<=x2; x++)  
        {  
           if(!g.selected_is_square && !inside_irregular_region(x,y)) continue;
           v = (int)readpixelonimage(x,y,bpp,ino);
           fx = (double)x / (double)dx;
           fy = (double)y / (double)dy;
           ii6 = cint(fy*(ii4-ii2) + ii2);
           ii5 = cint(fy*(ii3-ii1) + ii1);
           di  = cint(fx*(ii6-ii5) + ii5); 
           switch(remove_gradient_type)
           {   case 0: v += di;  break;
               case 1: v -= di;  break;
               case 2: v = cint(v*0.01*di);break;
           }
           v = max(0,min(v, int(g.maxvalue[bpp])));
           setpixelonimage(x,y,v,g.imode,bpp);
       }
     }
  }else
  { 
     for(y=y1; y<=y2; y++)  
     {  if(g.getout){ g.getout=0; break; }
        for(x=x1; x<=x2; x++)  
        { 
           if(!g.selected_is_square && !inside_irregular_region(x,y)) continue;
           v = (int)readpixelonimage(x,y,bpp,ino);
           valuetoRGB(v,rr,gg,bb,bpp);

           if(remove_gradient_source == 0 && ino>=0)
           {   fx = (double)x / (double)z[ino].xsize;
               fy = (double)y / (double)z[ino].ysize;
           }else
           {   fx = (double)(x-x1) / (double)dx;
               fy = (double)(y-y1) / (double)dy;
           }

           rr6 = cint(fy*(rr4-rr2) + rr2);
           rr5 = cint(fy*(rr3-rr1) + rr1);
           dr  = cint(fx*(rr6-rr5) + rr5); 

           gg6 = cint(fy*(gg4-gg2) + gg2);
           gg5 = cint(fy*(gg3-gg1) + gg1);
           dg  = cint(fx*(gg6-gg5) + gg5); 

           bb6 = cint(fy*(bb4-bb2) + bb2);
           bb5 = cint(fy*(bb3-bb1) + bb1);
           db  = cint(fx*(bb6-bb5) + bb5); 

           switch(remove_gradient_type)
           {   case 0: rr += dr; gg += dg; bb += db;  // add
                       break;
               case 1: rr -= dr; gg -= dg; bb -= db;  // subtract
                       break;
               case 2: rr = cint(rr*0.01*dr);         // multiply
                       gg = cint(gg*0.01*dg); 
                       bb = cint(bb*0.01*db);
           }
           rr = max(0,min(rr, g.maxred[bpp]));
           gg = max(0,min(gg, g.maxgreen[bpp]));
           bb = max(0,min(bb, g.maxblue[bpp]));
           v = RGBvalue(rr,gg,bb,bpp);
           setpixelonimage(x,y,v,g.imode,bpp);
      }
    }
 }
 repair(oino); 
}
