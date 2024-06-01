//--------------------------------------------------------------------------//
// xmtnimage61.cc                                                           //
// Pixel setting                                                            //
// Latest revision: 07-09-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;

//--------------------------------------------------------------------------//
// setpixel                                                                 //
//  'mode' determines how pixel interacts with color at that location.      //
// Returns the actual color set                                             //
// Default ino is -2 => automatically calculates ino from x,y               //
//      if ino is -1 => don't draw on image                                 //
// Default win is  0 => Use main window                                     //
// win and ino are optional.                                                //
//--------------------------------------------------------------------------//
uint setpixel(int x, int y, uint color, int mode, Window win, int ino)
{
    static uint ocolor=0;
    int bpp,color1,color2;
    int owin = win;
    uint ocolor2 = color;
    GC *gc = &g.image_gc; 
    win = g.main_window;
    bpp = g.bitsperpixel;

    if(mode != SET && mode != NONE) 
    { 
        color1 = color;
        color2 = readpixel(x,y);  
        color = interact(color1,color2,g.bitsperpixel,mode);
    }

    ////  Only use patterns here if g.inpattern is set.
    if(g.inpattern && g.draw_pattern && !pixel_on_pattern(x, y)) return 0;

    ////  If drawing on drawing_area or in an image, copy the pixel value
    ////  into the appropriate buffer.
    
    if(ino==-2) ino=whichimg(x,y,bpp); 
    if(ino==-1 && !owin){ ino=whichimg(x,y,bpp); if(ino) owin=z[ino].win;}
    XSetForeground(g.display, *gc, color); 
    ocolor = color;

    if(ino>=0)                             // for temporary, permanent, and XOR
    {    x -= z[ino].xpos;
         y -= z[ino].ypos;
         if(mode != NONE && between(x,0,z[ino].xscreensize-1) && 
                            between(y,0,z[ino].yscreensize-1))
            putpixelbytes(z[ino].img[y]+x*g.off[g.bitsperpixel],color,1,g.bitsperpixel);
         win = z[ino].win;      
         XDrawPoint(g.display, win, *gc, x, y);  
    }else if(owin)
    {    if(mode==XOR) gc = &g.xor_gc;     // for drawing on graphs (non-image only)
         XSetForeground(g.display, *gc, ocolor2); 
         XDrawPoint(g.display, owin, *gc, x, y);  
    }
    return color;
}
  

//--------------------------------------------------------------------------//
//  setRGBonimage                                                           //
//--------------------------------------------------------------------------//
void setRGBonimage(int x, int y, int rr, int gg, int bb, int mode, int bpp, int skip, 
    int noscreen, Window win, int want_alpha, int ino, int zoomed)
{
   uint color;
   int cf,ii,jj,xx,yy,xx1,yy1,xx2,yy2,outbpp;
   if(bpp==0) bpp=g.bitsperpixel;   
   if(bpp>=48)
   {   if(g.inmenu){ color = RGBvalue(rr,gg,bb,bpp); setpixel(x,y,color,mode,win); return; }
       if(zoomed) ino = whichimg(x,y,outbpp);
       else       ino = whichimage(x,y,outbpp);
       if(ino < 0) return;
       cf = z[ino].cf;
       xx = x - z[ino].xpos;
       yy = y - z[ino].ypos;
       if(zoomed && z[ino].is_zoomed)  // This should be used only when drawing
       {    xx1 = int((double)xx * z[ino].zoomx_inv + z[ino].zoom_x1);
            yy1 = int((double)yy * z[ino].zoomy_inv + z[ino].zoom_y1);
            xx2 = int((double)(xx+1) * z[ino].zoomx_inv + z[ino].zoom_x1);
            yy2 = int((double)(yy+1) * z[ino].zoomx_inv + z[ino].zoom_y1);
            for(jj=yy1; jj<=yy2; jj++)
            for(ii=xx1; ii<=xx2; ii++)
            if(between(ii,0,z[ino].xsize-1) && (between(jj,0,z[ino].ysize-1)))
                    putRGBbytes(z[ino].image[cf][jj]+ii*g.off[outbpp], rr, gg, bb, outbpp);
                    
       }else
       { 
            if(z[ino].is_zoomed)
            {    xx += z[ino].zoom_x1;
                 yy += z[ino].zoom_y1;
            }
            if(between(xx,0,z[ino].xsize-1) && (between(yy,0,z[ino].ysize-1)))
                    putRGBbytes(z[ino].image[cf][yy]+xx*g.off[outbpp], rr, gg, bb, outbpp);
       }
   }else
   {    color = RGBvalue(rr,gg,bb,bpp);
        setpixelonimage(x,y,color,mode,bpp,skip,noscreen,win,want_alpha,ino,zoomed);
   }
}


//--------------------------------------------------------------------------//
//setpixelonimage - Set color of a point in the selected current image      //
//  color    = desired color to use                                         //
//  bpp      = bits/pixel of `color' - not necessarily the bpp of the image //
//  skip     = image to be skipped (default=-1)                             //
//  outcolor = output pixel value for image[]                               //
//  outbpp   = output bpp for image[]                                       //
//  zoomed   = nonzero=correct for zoomed x and y coords.                   //
//  screencolor = output pixel value for img[] for display only (correc-    //
//    ted for grayvalue, and in screen bpp).                                //
//  noscreen = 1 if writing to screen is to be bypassed (write to image     //
//    buffer only). If noscreen is 1, the calling routine must call         //
//    rebuild_display() and copybackground() to draw the entire image.      //
//    This speeds things up when the image has to be re-palettized          //
//    anyway, and avoids putting "junk" pixels temporarily on screen.       //
//  win = 0 if setting pixel on drawing_area or on an image.  The actual    //
//    window is calculated automatically. If nonzero, it sets the pixel     //
//    on the specified window.                                              //
//  If onlyino is not 0, pixel is only drawn on specified image; otherwise, //
//    image number to put pixel on is calc'd automatically.                 //
//  Returns the image no. on which the pixel was drawn.                     //
//--------------------------------------------------------------------------//
int setpixelonimage(int x, int y, uint color, int mode, int bpp, int skip, 
    int noscreen, Window win, int want_alpha, int onlyino, int zoomed)
{
   int xx=0,yy=0,xx1,yy1,xx2,yy2,ino,ii,jj,cf,outbpp,outcolor=0,screencolor,color2, ubpp;
   ////  bpp = bits/pixel of color from calling routine for pixel
   ////  ubpp = bits/pixel of existing point underneath the pixel at x,y

   if(bpp==0) bpp=g.bitsperpixel;   
   if(g.inmenu)
   {    setpixel(x,y,color,mode,win,-1);
   }else
   { 
      ////  If XOR'ing, convert underlying pixel to depth of the new 
      ////  color, then interact them at that depth.            

      if(mode!=SET && mode!=BUFFER)
      {   color2 = readpixelonimage(x,y,ubpp,ino,skip);  
          if(bpp!=ubpp) color2 = convertpixel(color2,ubpp,bpp,1);
          color = interact(color,color2,bpp,mode);
      }

      ////   Convert the interacted pixel to bpp of the topmost image. 
      if(onlyino>0) 
      {    ino = onlyino;
           outbpp = z[ino].bpp;
      }else
      {   
           if(zoomed)
              ino = whichimg(x,y,outbpp);   // needed for cut & paste on zoomed images
           else
              ino = whichimage(x,y,outbpp); // needed for filtering on zoomed images
      }
      if(mode==BUFFER)
      {   outcolor = color;
          screencolor = color;
      }else
      { 
          if(bpp==outbpp) outcolor=color;
          else if(z[ino].hitgray) outcolor = convertpixel(color,bpp,outbpp,0);
          else outcolor = convertpixel(color,bpp,outbpp,1);

          ////  Calculate color to put on screen.                      
          ////  If it is a monochrome image, remap the gray (grayvalue() also 
          ////   converts it to screen bpp).                         
          ////  If not drawing to screen, this can be skipped.    

          if(noscreen)
              screencolor=outcolor;
          else if(ino>=0 && z[ino].hitgray)
              screencolor=grayvalue(outcolor,ino,g.bitsperpixel);
          else
              screencolor=convertpixel(outcolor,outbpp,g.bitsperpixel,1);
      }


      ////  Set pixel in an image Window.                      
      ////  Put the modified pixel in the image[] or fft[] at outbpp bpp. 
      ////  Also put in spreadsheet if spreadsheet is visible.
      if(ino>=0) 
      {
          cf = z[ino].cf;
          xx = x - z[ino].xpos;
          yy = y - z[ino].ypos;
          if(g.draw_pattern && !pixel_on_pattern(xx, yy)) return 0;
          if(zoomed && z[ino].is_zoomed)  // This should be used only when drawing
          {    
               xx1 = cint((double)xx * z[ino].zoomx_inv + z[ino].zoom_x1);
               yy1 = cint((double)yy * z[ino].zoomy_inv + z[ino].zoom_y1);
               if(z[ino].zoomx < 1.0)
               {    xx2 = cint( (double)(xx+0.999) * z[ino].zoomx_inv + (double)z[ino].zoom_x1);
                    yy2 = cint( (double)(yy+0.999) * z[ino].zoomx_inv + (double)z[ino].zoom_y1);
               }else
               {    xx2 = cint( (double)(xx+z[ino].zoomx) * z[ino].zoomx_inv + (double)z[ino].zoom_x1);
                    yy2 = cint( (double)(yy+z[ino].zoomx) * z[ino].zoomx_inv + (double)z[ino].zoom_y1);
               }
               for(jj=yy1; jj<yy2; jj++)
               for(ii=xx1; ii<xx2; ii++)
               if(between(ii,0,z[ino].xsize-1) && (between(jj,0,z[ino].ysize-1)))
               {    if(z[ino].floatexists) setfloatpixel(ii,jj,outcolor,ino);
                    if(z[ino].waveletexists) setwaveletpixel(ii,jj,outcolor,ino);
                    if(noscreen==0 && z[ino].s->visible) update_cell(ino,ii,jj);
                    putpixelbytes(z[ino].image[cf][jj]+ii*g.off[outbpp],outcolor,1,outbpp);
                    if(want_alpha && mode!=BUFFER) set_alpha_bit(ino,cf,ii,jj,1); 
               }
          }else
          {  
               if(z[ino].is_zoomed)
               {    xx += z[ino].zoom_x1;
                    yy += z[ino].zoom_y1;
               }
               if(between(xx,0,z[ino].xsize-1) && (between(yy,0,z[ino].ysize-1)))
               {    
                    if(z[ino].floatexists) setfloatpixel(xx,yy,outcolor,ino);
                    if(z[ino].waveletexists) setwaveletpixel(xx,yy,outcolor,ino);
                    if(noscreen==0 && z[ino].s->visible) update_cell(ino,xx,yy);
                    putpixelbytes(z[ino].image[cf][yy]+xx*g.off[outbpp],outcolor,1,outbpp);
                    if(want_alpha && mode!=BUFFER) set_alpha_bit(ino,cf,xx,yy,1); 
               }
          }
          z[ino].touched=1;
      }else
          if(want_alpha && mode!=BUFFER) set_alpha_bit(-1,0,x,y,1); 


      ////  If it's on the screen, set the pixel too. 
      ////  Use SET because color was already interacted.
      if(noscreen==0 && mode!=BUFFER) color = setpixel(x,y,screencolor,SET,win,ino);   
   }
   return ino;
}


//--------------------------------------------------------------------------//
//  setpixelonscreen                                                        //
//--------------------------------------------------------------------------//
void setpixelonscreen(int x, int y, int color)
{
    XSetForeground(g.display, g.image_gc, color); 
    XDrawPoint(g.display, g.main_window, g.image_gc, x, y);  
}


//--------------------------------------------------------------------------//
//  set_antialiased_pixel_on_image                                          //
//  Actually sets up to 4 pixels depending on fractional x,y value.         //
//      ---------                                                           //
//      | 1*| 2 |   *Set 100% pixel 1 if fractional x,y=0.                  //
//      ---------                                                           //
//      | 3 | 4 |                                                           //
//      ---------                                                           //
//--------------------------------------------------------------------------//
int set_antialiased_pixel_on_image(double x, double y, uint color, int mode, int bpp, 
    int skip, int noscreen, Window win, int want_alpha, int onlyino, int zoomed)
{
   int ino,ix,iy,pix1,pix2,pix3,pix4,v1,v2,v3,v4;
   int rr0,rr1,rr2,rr3,rr4,gg0,gg1,gg2,gg3,gg4,bb0,bb1,bb2,bb3,bb4;
   double dx, dy, frac0, frac1, frac2, frac3, frac4;
   x += 0.45;
   y+= 0.45;
   ix = int(x);  iy = int(y);
   dx = x - (double)ix;  
   dy = y - (double)iy;

   pix1 = readpixelonimage(ix,   iy,   bpp,ino);
   pix2 = readpixelonimage(ix+1, iy,   bpp,ino);
   pix3 = readpixelonimage(ix,   iy+1, bpp,ino);
   pix4 = readpixelonimage(ix+1, iy+1, bpp,ino);

   valuetoRGB(color, rr0, gg0, bb0, bpp);
   valuetoRGB(pix1, rr1, gg1, bb1, bpp);
   valuetoRGB(pix2, rr2, gg2, bb2, bpp);
   valuetoRGB(pix3, rr3, gg3, bb3, bpp);
   valuetoRGB(pix4, rr4, gg4, bb4, bpp);
 
   frac0 = (1.0-dx) * (1.0-dy);
   frac1 = 1.0 - frac0;
   rr1 = cint(frac1*rr1 + frac0*rr0);
   gg1 = cint(frac1*gg1 + frac0*gg0);
   bb1 = cint(frac1*bb1 + frac0*bb0);

   frac0 = dx * (1.0-dy);
   frac2 = 1.0 - frac0;
   rr2 = cint(frac2*rr2 + frac0*rr0);
   gg2 = cint(frac2*gg2 + frac0*gg0);
   bb2 = cint(frac2*bb2 + frac0*bb0);

   frac0 = (1.0-dx) * dy;
   frac3 = 1.0 - frac0;
   rr3 = cint(frac3*rr3 + frac0*rr0);
   gg3 = cint(frac3*gg3 + frac0*gg0);
   bb3 = cint(frac3*bb3 + frac0*bb0);

   frac0 = dx * dy;
   frac4 = 1.0 - frac0;
   rr4 = cint(frac4*rr4 + frac0*rr0);
   gg4 = cint(frac4*gg4 + frac0*gg0);
   bb4 = cint(frac4*bb4 + frac0*bb0);

   v1 = RGBvalue(rr1,gg1,bb1,bpp);  
   v2 = RGBvalue(rr2,gg2,bb2,bpp);  
   v3 = RGBvalue(rr3,gg3,bb3,bpp);  
   v4 = RGBvalue(rr4,gg4,bb4,bpp);  

   setpixelonimage(ix,  iy,  v1,mode,bpp,skip,noscreen,win,want_alpha,onlyino,zoomed);
   setpixelonimage(ix+1,iy,  v2,mode,bpp,skip,noscreen,win,want_alpha,onlyino,zoomed);
   setpixelonimage(ix,  iy+1,v3,mode,bpp,skip,noscreen,win,want_alpha,onlyino,zoomed);
   setpixelonimage(ix+1,iy+1,v4,mode,bpp,skip,noscreen,win,want_alpha,onlyino,zoomed);
   return 0;
}


//--------------------------------------------------------------------------//
// readpixel                                                                //
//--------------------------------------------------------------------------//
uint readpixel(int x, int y, int skip)
{
   int ino, bpp;
   uint value=0;
   g.inmenu++;
   ino = whichimg(x,y,bpp,skip);
   bpp=g.bitsperpixel;
   if(ino>=0) 
   {    x -= z[ino].xpos;
        y -= z[ino].ypos;
        if(y >= 0 && 
           y <=  z[ino].yscreensize-1 && 
           x >= 0 && 
           x <=  z[ino].xscreensize-1)
           value = pixelat(z[ino].img[y] + x*g.off[bpp], bpp);
   }
   g.inmenu--;
   return value;
}


//--------------------------------------------------------------------------//
// readRGB                                                                  //
//--------------------------------------------------------------------------//
void readRGB(int x, int y, int &rr, int &gg, int &bb, int skip)
{
   int ino, bpp;
   g.inmenu++;
   ino = whichimg(x,y,bpp,skip);
   bpp = g.bitsperpixel;
   if(ino>=0) 
   {    x -= z[ino].xpos;
        y -= z[ino].ypos;
        if(y >= 0 && 
           y <  z[ino].yscreensize-1 && 
           x >= 0 && 
           x <  z[ino].xscreensize-1)
           RGBat(z[ino].img[y] + x*g.off[bpp], bpp, rr, gg, bb);
   }
   g.inmenu--;
   return;
}
 
 
//--------------------------------------------------------------------------//
// readpixelonimage - Read color of a point in the selected current         //
//  image or on background if not within an image.                          //
//  x and y are screen coordinates relative to upper left of main window.   //
//  Value returned is in the bits/pixel of the pixel, not necessarily       //
//  the screen. To get the screen equivalent, use readpixel().              //
// Also returns the bits/pixel of the pixel in `bpp' and the image          //
//   no. in `ino'.                                                          //
// The optional parameter `skip' allows one image to be omitted from        //
//   consideration. This allows reading an image, then XOR'ing it with      //
//   the screen. (default = -2)                                             //
// If the image no. is known with certainty, it is faster to call           //
//   pixelat() instead of this function.                                    //
//--------------------------------------------------------------------------//
uint readpixelonimage(int x, int y, int& bpp, int& ino, int skip)
{   
   uint value=0;
   ino = whichimage(x,y,bpp,skip);

   if(ino<0 || ino>=g.image_count) return 0;
   int x1 = x-z[ino].xpos;  // offset in image buffer
   int y1 = y-z[ino].ypos;  
   if(z[ino].is_zoomed)
   {  x1 += z[ino].zoom_x1;
      y1 += z[ino].zoom_y1;
   }
   if(ino>=0) 
   {  
      if(g.inmenu || z[ino].transparency) return readpixel(x,y);
      if(y1 >= 0 && 
         y1 <  z[ino].ysize && 
         x1 >= 0 && 
         x1 <  z[ino].xsize)
         {  if(z[ino].fftdisplay==NONE)
                 value = pixelat(z[ino].image[z[ino].cf][y1] + x1*g.off[bpp], bpp);
            else
                 value = pixel_equivalent(ino, x1, x1*g.off[bpp], y1); 
         }
   }
   return value;   
}


//--------------------------------------------------------------------------//
//  readRGBonimage                                                          //
//  skip=-2 = default, ignore                                               //
//--------------------------------------------------------------------------//
void readRGBonimage(int x, int y, int& bpp, int& ino, int &rr, int &gg, 
   int &bb, int skip)
{
   ino = whichimage(x,y,bpp,skip);
   int x1 = x-z[ino].xpos;  // offset in image buffer
   int y1 = y-z[ino].ypos;  
   if(z[ino].is_zoomed)
   {  x1 += z[ino].zoom_x1;
      y1 += z[ino].zoom_y1;
   }
   if(ino>=0) 
   {  
      if(g.inmenu || z[ino].transparency) readRGB(x,y,rr,gg,bb,skip);
      else if(y1 >= 0 && 
              y1 <  z[ino].ysize && 
              x1 >= 0 && 
              x1 <  z[ino].xsize)
              RGBat(z[ino].image[z[ino].cf][y1] + x1*g.off[bpp], bpp, rr, gg, bb);
   }
   return;   
}


//--------------------------------------------------------------------------//
//  erase_pixel                                                             //
//--------------------------------------------------------------------------//
int erase_pixel(int x, int y, int *repairflags)
{
   int ii,iii,jj,ino,bpp,f,v,ct,done=0,needrepair=0,needremap=0;
   ino = whichimage(x,y,bpp);                     
   uchar *wadd, *radd;
   if(z[ino].backedup)
   {    f = z[ino].cf;
        ii = x - z[ino].xpos;
        iii = ii * g.off[bpp];
        jj = y - z[ino].ypos;
        if(between(ii,0,z[ino].xsize-1) && between(jj,0,z[ino].ysize-1))
        {    radd = z[ino].backup[f][jj]+iii;
             v = pixelat(radd, bpp);
             ct = z[ino].colortype;
             if(bpp>8 && ct==GRAY)    
                  v = grayvalue(v,ino,g.bitsperpixel);
             else if(bpp==8 && bpp!=g.bitsperpixel)
             {    needremap=1;
                  needrepair=1;
             }
             else if(bpp>8 && bpp!=g.bitsperpixel)
                  v = convertpixel(v,bpp,g.bitsperpixel,1);
             else if(g.bitsperpixel==8) 
             {    needremap=1;
                  needrepair=1;
             }
             wadd = z[ino].image[f][jj]+iii;
             putpixelbytes(wadd,v,1,bpp,1);
             if(needrepair==0 && needremap==0)
                  setpixelonimage(x,y,v,g.imode);
             else
             {   
                  v = convertpixel(v,bpp,g.bitsperpixel,1);
                  setpixel(x,y,v,SET);
             }
             set_alpha_bit(ino,f,x-z[ino].xpos,y-z[ino].ypos,0);
             z[ino].hit=1;
        }
   }else
   {    if(g.image_count>0) 
             message("Image was not backed up");
        else message("No images");
        done=1;
   } 
   if(done) repairflags[0] |= 4;
   if(needrepair) repairflags[ino] |= 2;
   if(needremap) repairflags[ino] |= 1;
   return ino;
}


//--------------------------------------------------------------------------//
//  set_alpha_bit                                                           //
//  x,y are indices in image buffer relative to upper left of image.        //
//--------------------------------------------------------------------------//
void set_alpha_bit(int ino, int frame, int x, int y, int value)
{
   int xindex, bit;
   xindex = x>>3;
   bit = 1<<(x-(xindex<<3));
   if(between(xindex,0,(z[ino].xsize>>3)-1) && between(y,0,z[ino].ysize-1))
   {   if(value) z[ino].alpha[frame][y][xindex] |= bit;
       else      z[ino].alpha[frame][y][xindex] &= ~bit;
   }
}


//--------------------------------------------------------------------------//
//  get_alpha_bit                                                           //
//  x,y are indices in image buffer relative to upper left of image.        //
//  f = frame                                                               //
//--------------------------------------------------------------------------//
int get_alpha_bit(int ino, int f, int x, int y)
{
   int xindex, bit, answer=0;
   xindex = x>>3;
   bit = 1<<(x-(xindex<<3));
   if(between(xindex,0,z[ino].xsize>>3-1) && between(y,0,z[ino].ysize-1))
      if(z[ino].alpha[f][y][xindex] & bit) answer=1;

   return answer;
}


//--------------------------------------------------------------------------//
//  zoom_x_index                                                            //
//  Returns image buffer x index of a zoomed point given the zoomed x coord.//
//--------------------------------------------------------------------------//
int zoom_x_index(int x, int ino)
{ 
  if(ino>=0 && z[ino].is_zoomed) 
  {   x -= z[ino].xpos;
      x = ((int)((double)x*z[ino].zoomx_inv) + z[ino].zoom_x1); 
  }
  return x;
}     


//--------------------------------------------------------------------------//
//  zoom_y_index                                                            //
//  Returns image buffer y index of a zoomed point given the zoomed x coord.//
//--------------------------------------------------------------------------//
int zoom_y_index(int y, int ino)
{ 
  if(ino>=0 && z[ino].is_zoomed) 
  {   y -= z[ino].ypos;
      y = ((int)((double)y*z[ino].zoomy_inv) + z[ino].zoom_y1); 
  }
  return y;
}     


//--------------------------------------------------------------------------//
//  zoom_x_coordinate                                                       //
//  Returns image x coordinate of a zoomed screen x coordinate              //
//--------------------------------------------------------------------------//
int zoom_x_coordinate(int x, int ino)
{ 
  if(ino>=0 && z[ino].is_zoomed) 
  {    x = (int)((double)(x - z[ino].xpos)*z[ino].zoomx_inv);
       x = max(0, min(x, z[ino].xsize-1));
       x += z[ino].xpos;
  }
  return x;
}     

//--------------------------------------------------------------------------//
//  zoom_y_coordinate                                                       //
//  Returns image y coordinate of a zoomed screen y coordinate              //
//--------------------------------------------------------------------------//
int zoom_y_coordinate(int y, int ino)
{ 
  if(ino>=0 && z[ino].is_zoomed) 
  {    y = (int)((double)(y-z[ino].ypos)*z[ino].zoomy_inv);
       y = max(0, min(y, z[ino].ysize-1));
       y += z[ino].ypos;
  }
  return y;
}     


//--------------------------------------------------------------------------//
//  zoom_x_coordinate_of_index                                              //
//  Returns screen coordinate given the image buffer x index                //
//--------------------------------------------------------------------------//
int zoom_x_coordinate_of_index(int x, int ino)
{
   if(ino<0) return x;
   if(z[ino].is_zoomed) 
      return (int)((double)x*z[ino].zoomx) + z[ino].zoom_x1 + z[ino].xpos;  
   return x+z[ino].xpos;
}


//--------------------------------------------------------------------------//
//  zoom_y_coordinate_of_index                                              //
//  Returns screen coordinate given the image buffer y index                //
//--------------------------------------------------------------------------//
int zoom_y_coordinate_of_index(int y, int ino)
{
   if(ino<0) return y;
   if(z[ino].is_zoomed) 
      return (int)((double)y*z[ino].zoomy) + z[ino].zoom_y1 + z[ino].ypos;  
   return y+z[ino].ypos;
}


//--------------------------------------------------------------------------//
// interact - calculates the interaction of 2 pixels. They must have the    //
//  same bits/pixel.                                                        //
//--------------------------------------------------------------------------//
uint interact(int color1, int color2, int bpp, int mode)
{
     int rr,gg,bb,r2,g2,b2;
     if(bpp!=8 && bpp!=15 && bpp!=16 && bpp!=24 && bpp!=32) return 0;
     if(bpp==8)
     {  switch(mode)
        {   case MAXIMUM: color1=max(color1,color2); break;
            case MINIMUM: color1=min(color1,color2); break;     
            case ADD: color1=min((int)g.maxcolor,color1+color2); break;
            case SUB12: color1=max(0,color1-color2);     break;
            case SUB21: color1=max(0,color2-color1);     break;
            case XOR:   color1^=color2; break;
            case AVE: color1+=color2; color1>>=1; break;       
            case SUP: if(color1==0) return(color2);
                      else return(color1);
        }
        color1=min(255,max(0,color1));
     }else
     {  valuetoRGB(color1,rr,gg,bb,bpp);
        valuetoRGB(color2,r2,g2,b2,bpp);
        switch(mode)
        {   case MAXIMUM:   rr=max(rr,r2); gg=max(gg,g2); bb=max(bb,b2); break;
            case MINIMUM:   rr=min(rr,r2); gg=min(gg,g2); bb=min(bb,b2); break;
            case ADD:   rr=min(g.maxred[bpp]  ,rr+r2);
                        gg=min(g.maxgreen[bpp],gg+g2);
                        bb=min(g.maxblue[bpp] ,bb+b2);
                        break;
            case SUB12: rr=max(0,rr-r2);
                        gg=max(0,gg-g2);
                        bb=max(0,bb-b2);
                        break;
            case SUB21: rr=max(0,r2-rr);
                        gg=max(0,g2-gg);
                        bb=max(0,b2-bb);
                        break;
            case XOR:  
                        if(!rr) rr=255;
                        if(!gg) gg=255;
                        if(!bb) bb=255;
                        rr^=r2; gg^=g2; bb^=b2; 
                        break;
            case AVE:   rr+=r2; rr>>=1;
                        gg+=g2; gg>>=1;
                        bb+=b2; bb>>=1;
                        break;
            case SUP:   if(color1==0) return(color2); 
                        else return(color1);
        }
        color1 = RGBvalue(rr,gg,bb,bpp);
        color1 = min((uint)g.maxvalue[bpp],(uint)max(0,color1));
     }   
     return((uint)color1);
}



//--------------------------------------------------------------------------//
// setfloatpixel - Converts an integer pixel value back to a floating       //
//  point number. This is done after manipulating a FFT-transformed         //
//  image. All manipulations are done on the integer value which is then    //
//  converted back to a float using scaling factors i_fmin[image]           //
//  and i_fmax[image].                                                      //
//  The calling routine has to know which image it is.                      //
//  x and y are screen coordinates of the pixel.                            //
//  If the fft'd image is showing "original image", it does nothing.        //
// Wavelet pixels are not changed here, but in setwaveletpixel().           //
//--------------------------------------------------------------------------//
void setfloatpixel(int x, int y, uint color,int ino)
{
  complex value(0,0);
  double dreal=0.0,dimag=0.0;
  double dfac;
  if(ino>=0)
  { if( z[ino].floatexists && 
        x < z[ino].xsize && 
        y < z[ino].ysize && 
        z[ino].fftdisplay!=WAVE &&
        z[ino].fftdisplay!=NONE)
    { 
        dfac = (double)(z[ino].fmax-z[ino].fmin)/(double)g.maxvalue[z[ino].bpp]; 
        if(x>=0 && y>=0)
        {  
           switch(z[ino].fftdisplay)
           {  case REAL:  dimag=z[ino].fft[y][x].imag(); 
                          dreal = (double)color*dfac;
                          break;
              case IMAG:  dreal=real(z[ino].fft[y][x]); 
                          dimag = (double)color*dfac;
                          break;
              case POWR:  dreal = (double)color*dfac;
                          dimag = (double)color*dfac;
                          break;
              default:    fprintf(stderr,"error in setfloatpixel\n");
                          break;
           }   
           value.real()=dreal;
           value.imag()=dimag;
           z[ino].fft[y][x] = value;  // complex
        }
    }   
  }  
}


//--------------------------------------------------------------------------//
// readbyte                                                                 //
// reads 8 pixels from screen, assumes B&W, compress into 1 byte            //
//--------------------------------------------------------------------------//
uchar readbyte(int x,int y)
{  
   uchar b;
   int k;
   uint threshold = g.maxcolor/2;
   b=0;
   for(k=0;k<8;k++) if(readpixel(x+k,y)>threshold) b |= 1<<(7-k);
   return(b);
}


//--------------------------------------------------------------------------//
// graypixel - Convert a color pixel value at specified bpp to a grayscale  //
//            value, regardless of its bits/pixel. You can't just set       //
//            the r,g,and b the same because there may be different         //
//            bits for each color.                                          //
//            Used to convert a pixel value to a "gray" color before        //
//            displaying it, thus bpp is usually = bitsperpixel.            //
//                                                                          //
//            Not to be confused with grayvalue(), which remaps the         //
//            mapping of grayscale pixels to the screen.                    //
//--------------------------------------------------------------------------//
int graypixel(int value, int bpp)
{
   int rr,gg,bb;
   valuetoRGB(value,rr,gg,bb,bpp);
   switch(bpp)
   {   case 8: 
               return (int)((rr*4 + gg*4 + bb*4)/3);
       case 16:          
               bb=(rr+(gg>>1)+bb)/3;
               rr=bb;
               gg=bb*2;
               break;
       default:
               bb=(int)((double)rr*g.luminr +
                        (double)gg*g.luming +
                        (double)bb*g.luminb);
               bb = min(bb,g.maxblue[bpp]-1);
               rr=bb;
               gg=bb;
               break;
   }
   value = RGBvalue(rr,gg,bb,bpp);
   return value;
}     


//--------------------------------------------------------------------------//
//  graytocolor - convert a grayscale pixel to RGB values                   //
//--------------------------------------------------------------------------//
int graytocolor(int ino, int value, int bpp)
{
   int rr,gg,bb;
   double fac = (double)g.maxred[bpp] / (double)(z[ino].gray[0] - z[ino].gray[1]);
   value = cint((value - z[ino].gray[1]) * fac);
   rr=gg=bb=value;
   if(bpp==16) gg+=value;
   rr = max(0, min(rr, g.maxred[bpp]));
   gg = max(0, min(gg, g.maxgreen[bpp]));
   bb = max(0, min(bb, g.maxblue[bpp]));
   return RGBvalue(rr,gg,bb,bpp);
}


//--------------------------------------------------------------------------//
// grayvalue                                                                //
//   calculates the gray-mapped value for a pixel if the image              //
//   is grayscale. If image is color, it has no effect.                     //
//   Each image has a different gray scale mapping, thus the                //
//   image no. is required.                                                 //
//   (Converts from real image value to screen-mapped value.)               //
//   `bpp' is desired bits/pixel of output, normally same as                //
//    screen bitsperpixel.                                                  //
//    Not to be confused with graypixel, which turns the value              //
//    into a "gray" color.                                                  //
//--------------------------------------------------------------------------//
int grayvalue(int value, int ino, int bpp)
{
  double fac=1.0;
  int rr,gg,bb;
  if(ino>=0)
  { 
     int whitein =   z[ino].gray[0];  // It is necessary to convert these to
     int blackin =   z[ino].gray[1];  // regular variables or it won't 
     int whiteout=   z[ino].gray[2];  // divide properly.
     int blackout=   z[ino].gray[3];         
     if(z[ino].hitgray)
     {  
        if((whitein!=whiteout)||(blackin!=blackout))
          if(whitein!=blackin)
            fac=(double)(whiteout-blackout)/(double)(whitein-blackin);
        value = (int)((value-blackin)*fac+blackout);
        value = max(0,min(whiteout,value));
   
        // Gray scale is mapped to the brightest single color in color modes,
        // so it is necessary to convert back to a real gray pixel value.

        if(bpp>8)                     
        {    value=max(0,min(value,g.maxred[bpp]));
             rr = value;
             gg = value; if(bpp==16) gg<<=1;  // 16bit mode has 6 bits of green
             bb = value;
             value = RGBvalue(rr,gg,bb,bpp);     
        }
     }   
  }   
  return(value);
}



//--------------------------------------------------------------------------//
// pixel_on_pattern                                                         //
// returns 1 if x,y is a foreground pixel in g.draw_pattern                 //
// x,y should be relative to upper left of image                            //
//--------------------------------------------------------------------------//
int pixel_on_pattern(int x, int y)
{
  switch(g.draw_pattern)
  {
      case 0: return 1; break;
      case 1: if(x%5) return 0; break;                  // vertical lines
      case 2: if(y%5) return 0; break;                  // horizontal lines
      case 3: if((x+y)%5) return 0; break;              // up diagonal
      case 4: if((x-y)%5) return 0; break;              // down diagonal
      case 5: if((x+y)%5 && (x-y)%5) return 0; break;   // small XXX
      case 6: if((x+y)%10 && (x-y)%10) return 0; break; // big XXX
      case 7: if((x*y)%5 && (x*y)%5) return 0; break;   //
      case 8: if((x%5 != y%5)) return 0; break;         // inverse of 4
      case 9: if(x%2 && y%2) return 0; break;           // fine XXX
      case 10: if(x%3 && y%3) return 0; break;          // medium XXX
      case 11: if((x+y)%5 && (x-y)%5) return 0;         // dots
               if((x*y)%5 && (x*y)%5) return 0;
               break;
      case 12: if((x+y)%3 && (x-y)%3) return 0;         // dots
               if((x*y)%3 && (x*y)%3) return 0;
               break;
      case 13: if((x*x+y*y)%5) return 0; break;         // small circles
      case 14: if((x*x+y*y)%10) return 0; break;        // medium circles
      case 15: if((x*x+y*y)%20) return 0; break;        // big circles
      case 16: if((x*y+x*y)%4 || (x*x+y*y)%5) return 0; break;  // big circles
      case 17: if((x%8 +y%6) % 4) return 0; break;      // hatch
      case 18: if((x%7 +y%5) % 3) return 0; break;      // hatch
      case 19: if(((x%8 - y%6) % 4) ||
                   (x%6 + y%4) % 4) return 0;
               break;                                   // zigzag
      case 20: if(((x%8 - y%6) +
                   (x%6 + y%4)) % 8) return 0;
               break;                                   // zigzag
      case 21: if(((x%5 - y%4) +
                   (x%4 + y%5)) % 6) return 0;
               break;                                   // zigzag
      case 22: if(((x%9 - y%8) +
                   (x%8 + y%9)) % 4) return 0;
               break;                                   // medium squares
      case 23: if(((x%12 - y%11) -
                   (x%11 + y%12)) % 4) return 0;
               break;                                   // big squares
      case 24: if(((x%9 - y%7) +
                   (x%7 + y%9)) % 4) return 0;
               break;                                   // complex circle
      case 25: if(((x%11 - y%9) -
                   (x%9 + y%11)) % 4) return 0;
               break;                                   // big complex circles
      case 26: if((x*y%11 - y%9 - x*9) % 4) return 0;
               break;                                   // pattern
      case 27: if(((x*y)%22 - y%2 - x%2) % 7) return 0;
               break;                                   // complex squares
      case 28: if(((x*y)%3 - y%2 - x%2) % 11) return 0;
               break;                                   // knit
      case 29: if(((x*y)%4 - y%3 - x%3) % 22) return 0;
               break;                                   // knit
  }
  return 1;
}
