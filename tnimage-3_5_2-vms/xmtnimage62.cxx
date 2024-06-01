//--------------------------------------------------------------------------//
// xmtnimage62.cc                                                           //
// Latest revision: 01-17-2003                                              //
// Copyright (C) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;

//--------------------------------------------------------------------------//
// localcontrast                                                            //
//--------------------------------------------------------------------------//
void localcontrast(int noofargs, char **arg)
{
  filter ff;
  ff.filename[0] = 0;
  ff.ino = ci;
  ff.type = 0;
  ff.sharpfac = 100;
  ff.local_scale = 10;
  ff.do_filtering = 1;
  ff.background = 127;
  if(noofargs>=1) ff.local_scale = atoi(arg[1]);
  if(noofargs>=2) ff.sharpfac = atoi(arg[2]);
  if(z[ci].colortype == GRAY) localcontrast_gray(&ff);
  else  localcontrast_color(&ff);
}
void localcontrast(filter *ff)
{
   if(z[ci].colortype == GRAY) localcontrast_gray(ff);
   else  localcontrast_color(ff);
}



//--------------------------------------------------------------------------//
// localcontrast_gray - coordinates are relative to upper left of image     //
//--------------------------------------------------------------------------//
void localcontrast_gray(filter *ff)
{
   Widget www, scrollbar;
   int bpp,frame,i,i2,j,k,l,value, maxval, minval, b, increment,
       maxval2,minval2,maxval3,minval3,maxval4,minval4,maxval6,minval6,maxval8,
       minval8;
   double contrast, maxrange;
   double shnew = ((double)ff->sharpfac)/100.0;  // Fraction of new pixel to use
   double shold = 1.0-shnew;                     // Fraction of old pixel to keep
   int ino = ff->ino;
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   int xpos = z[ino].xpos;
   int ypos = z[ino].ypos;
   int x1 = g.ulx - xpos;
   int x2 = g.lrx - xpos;
   int y1 = g.uly - ypos;
   int y2 = g.lry - ypos;
   x1 = max(0, min(x1, xsize-1));                // ignore ff->x1, etc 
   x2 = max(0, min(x2, xsize-1));
   y1 = max(0, min(y1, ysize-1));
   y2 = max(0, min(y2, ysize-1));
   uchar *buffer_1d;
   uchar **buffer;
   uchar **add;
   bpp = z[ino].bpp;
   b = (7+bpp)/8;
   frame = z[ino].cf;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   increment = max(1, 1+ff->local_scale/5);
   printstatus(CALCULATING);
   buffer_1d  = (uchar*)malloc(xsize*ysize*b);
   buffer = make_2d_alias(buffer_1d, xsize*b, ysize);
   maxrange = g.maxvalue[bpp];
   progress_bar_open(www, scrollbar);
   add = z[ino].image[frame];
   for(j=y1;j<y2;j++)
   {
      for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
      if(g.selected_is_square || inside_irregular_region(i+xpos,j+ypos))
      {
           contrast = 1;
           maxval  = 0; minval  = (int)g.maxvalue[bpp];
           maxval2 = 0; minval2 = (int)g.maxvalue[bpp];
           maxval3 = 0; minval3 = (int)g.maxvalue[bpp];
           maxval4 = 0; minval4 = (int)g.maxvalue[bpp];
           maxval6 = 0; minval6 = (int)g.maxvalue[bpp];
           maxval8 = 0; minval8 = (int)g.maxvalue[bpp];
           progress_bar_update(scrollbar, j*100/ysize);
           if(g.image_count <= ino){ message("Hey!",ERROR); g.getout=1; break; }

           for(l=j-ff->local_scale; l<=j+ff->local_scale; l+=increment)
           for(k=i-ff->local_scale; k<=i+ff->local_scale; k+=increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval) maxval = value;
               if(value < minval) minval = value;
           }
           for(l=j-2*ff->local_scale; l<=j+2*ff->local_scale; l+=2*increment)
           for(k=i-2*ff->local_scale; k<=i+2*ff->local_scale; k+=2*increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval2) maxval2 = value;
               if(value < minval2) minval2 = value;
           }
           for(l=j-3*ff->local_scale; l<=j+3*ff->local_scale; l+=3*increment)
           for(k=i-3*ff->local_scale; k<=i+3*ff->local_scale; k+=3*increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval3) maxval3 = value;
               if(value < minval3) minval3 = value;
           }
           for(l=j-4*ff->local_scale; l<=j+4*ff->local_scale; l+=4*increment)
           for(k=i-4*ff->local_scale; k<=i+4*ff->local_scale; k+=4*increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval4) maxval4 = value;
               if(value < minval4) minval4 = value;
           }
           for(l=j-6*ff->local_scale; l<=j+6*ff->local_scale; l+=6*increment)
           for(k=i-6*ff->local_scale; k<=i+6*ff->local_scale; k+=6*increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval6) maxval6 = value;
               if(value < minval6) minval6 = value;
           }
           for(l=j-8*ff->local_scale; l<=j+8*ff->local_scale; l+=8*increment)
           for(k=i-8*ff->local_scale; k<=i+8*ff->local_scale; k+=8*increment)
           {   if(l>=ysize || k>=xsize || l<0 || k<0) continue;
               value = pixelat(add[l] + k*b, bpp);
               if(value > maxval8) maxval8 = value;
               if(value < minval8) minval8 = value;
           }

           value = pixelat(add[j] + i2, bpp);
           maxval = (6*maxval + 5*maxval2 + 4*maxval3 + 3*maxval4 + 2*maxval6 + 1*maxval8)/21;
           minval = (6*minval + 5*minval2 + 4*minval3 + 3*minval4 + 2*minval6 + 1*minval8)/21;
           if(maxval<=minval) maxval=minval+4;
           contrast = maxrange / (double)(maxval - minval);
           value = cint(shold*value + shnew*(value - minval) * contrast); 
           value = max(0, min(value, (int)maxrange));
           putpixelbytes(buffer[j]+i2, value, 1, bpp, 1);
      }else
      {    value = pixelat(add[j] + i2, bpp);
           putpixelbytes(buffer[j]+i2, value, 1, bpp, 1);
      }
      check_event();
      if(g.getout) break;
   }
   if(!g.getout)
   for(j=y1; j<y2; j++)
   for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
   {    value = pixelat(buffer[j]+i2, bpp);
        putpixelbytes(z[ino].image[frame][j]+i2, value, 1, bpp, 1);
   }
   g.getout = 0;
   progress_bar_close(www);
   repair(ino);
   rebuild_display(ino);
   redraw(ino);
   z[ino].touched = 1;
   free(buffer);
   free(buffer_1d);
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// RGBtoHSV  h=[0,360] s=[0,255] v=[0,255]                                  //
//--------------------------------------------------------------------------//
void RGBtoHSV(int rr, int gg, int bb, int &hh, int &ss, int &vv, int bpp)
{
   int fmin, fmax, delta;
   double h1=0.0;
   fmin = min(rr, min(gg, bb));
   fmax = max(rr, max(gg, bb));
   vv = fmax;
   delta = fmax - fmin;
   if(fmax) ss = g.maxgray[bpp]*delta/fmax; else ss = 0; 
   if(!delta) h1=0.0;
   else if(rr==fmax) h1 =     (double)(gg - bb)/(double)delta;  // between yellow & magenta
   else if(gg==fmax) h1 = 2 + (double)(bb - rr)/(double)delta;  // between cyan & yellow
   else              h1 = 4 + (double)(rr - gg)/(double)delta;  // between magenta & cyan
   hh = cint(h1*60);
   if(hh<0) hh+= 360;

}


//--------------------------------------------------------------------------//
// HSVtoRGB h=[0,360] s=[0,255] v=[0,255]                                   //
//--------------------------------------------------------------------------//
void HSVtoRGB(int hh, int ss, int vv, int &rr, int &gg, int &bb, int bpp)
{ 
  double f, p, q, t;
  double r1,g1,b1,h1,s1,v1;
  int i;
  ss = min(g.maxgray[bpp], max(ss, 1));
  vv = min((int)g.maxgray[bpp], max(vv, 1));
  h1 = (double)hh;
  s1 = (double)ss/g.maxgray[bpp];
  v1 = (double)vv/g.maxgray[bpp];
  if(s1==0){ rr=gg=bb=vv; return; }
  h1 /= 60;
  i = (int)floor(h1);
  f = h1 - i;
  p = v1 * (1 - s1);
  q = v1 * (1 - s1 * f);
  t = v1 * (1 - s1 *(1 - f));
  switch(i)
  {    case 0:  r1 = v1; g1 = t ; b1 = p ; break;
       case 1:  r1 = q ; g1 = v1; b1 = p ; break;
       case 2:  r1 = p ; g1 = v1; b1 = t ; break;
       case 3:  r1 = p ; g1 = q ; b1 = v1; break;
       case 4:  r1 = t ; g1 = p ; b1 = v1; break;
       default: r1 = v1; g1 = p ; b1 = q ; break;
  }           
  rr = cint(r1 * g.maxred[bpp]);
  gg = cint(g1 * g.maxgreen[bpp]);
  bb = cint(b1 * g.maxblue[bpp]);
  rr = max(0,min(rr, g.maxred[bpp]));
  gg = max(0,min(gg, g.maxgreen[bpp]));
  bb = max(0,min(bb, g.maxblue[bpp]));
}


//--------------------------------------------------------------------------//
// localcontrast_color                                                      //
//--------------------------------------------------------------------------//
void localcontrast_color(filter *ff)
{
   Widget www, scrollbar;
   int bpp,frame,e,f,h,i,i2,j,k,kk,l,ll,value,b,increment;
   int ino = ff->ino;
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   int xpos = z[ino].xpos;
   int ypos = z[ino].ypos;
   int x1 = g.ulx - xpos;
   int x2 = g.lrx - xpos;
   int y1 = g.uly - ypos;
   int y2 = g.lry - ypos;
   x1 = max(0, min(x1, xsize-1));
   x2 = max(0, min(x2, xsize-1));
   y1 = max(0, min(y1, ysize-1));
   y2 = max(0, min(y2, ysize-1));
   uchar *buffer_1d;
   uchar **buffer;
   uchar **add;
   double shnew = ((double)ff->sharpfac)/100.0;  // Fraction of new pixel to use
   double shold = 1.0-shnew;                     // Fraction of old pixel to keep

   bpp = z[ino].bpp;
   b = (7+bpp)/8;
   frame = z[ino].cf;
   increment = max(1, 1+ff->local_scale/5);
   printstatus(CALCULATING);

   buffer_1d  = (uchar*)malloc(xsize*ysize*b);
   buffer = make_2d_alias(buffer_1d, xsize*b, ysize);
   progress_bar_open(www, scrollbar);
   add = z[ino].image[frame];

   int maxval[6][3], minval[6][3];
   int fac[6] = { 1, 2, 3, 4, 6, 8 };
   int r[3], maxr[3], mincolor[3], maxcolor[3];
   double contrast[3];
   maxr[0] = g.maxred[bpp];
   maxr[1] = g.maxgreen[bpp];
   maxr[2] = g.maxblue[bpp];
    
   for(j=y1;j<y2;j++)
   {   check_event();
       if(g.getout) break;
       progress_bar_update(scrollbar, j*100/ysize);        
       for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
       {   if(g.selected_is_square || inside_irregular_region(i+xpos,j+ypos))
           {
               //// Initialize maxval and minval arrays
               for(e=0; e<3; e++)
               {   contrast[e] = 1.0;
                   for(k=0; k<6; k++)
                   {   maxval[k][e] = 0; 
                       minval[k][e] = maxr[e]; 
                   }
               }
               //// Find max & min rgb at 6 distances from the pixel, set by
               //// array fac[] x ff->local_scale (scale factor)
               for(h=0; h<6; h++)
               {   f = fac[h];
                   for(l=j-f*ff->local_scale; l<=j+f*ff->local_scale; l+=f*increment)
                   for(k=i-f*ff->local_scale; k<=i+f*ff->local_scale; k+=f*increment)
                   {   kk=k; ll=l;
                       if(ll<=0) ll = 1;
                       if(kk<=0) kk = 1;
                       if(l>=ysize-2) ll = ysize-2;
                       if(k>=xsize-2) kk = xsize-2;                 
                       value = pixelat(add[ll] + kk*b, bpp);
                       valuetoRGB(value, r[0], r[1], r[2], bpp);
                       for(e=0; e<3; e++)
                       {   if(r[e] > maxval[h][e]) maxval[h][e] = r[e];
                           if(r[e] < minval[h][e]) minval[h][e] = r[e];
                       }
                   }
               }
               
               //// Read the center pixel
               value = pixelat(add[j] + i2, bpp);
               valuetoRGB(value, r[0], r[1], r[2], bpp);

               //// Calculate contrast factor to multiply pixel by for each color e
               for(e=0; e<3; e++)
               {   maxcolor[e] = (6*maxval[0][e] + 5*maxval[1][e] + 4*maxval[2][e] + 
                                  3*maxval[3][e] + 2*maxval[4][e] + 1*maxval[5][e])/21;
                   mincolor[e] = (6*minval[0][e] + 5*minval[1][e] + 4*minval[2][e] + 
                                  3*minval[3][e] + 2*minval[4][e] + 1*minval[5][e])/21;
                   if(maxcolor[e] <= mincolor[e]) maxcolor[e] = mincolor[e] + 4;
                   if(maxcolor[e] != mincolor[e])
                       contrast[e] = maxr[e] / (double)(maxcolor[e] - mincolor[e]);
                   r[e] = cint(shold*r[e] + shnew*(r[e] - mincolor[e]) * contrast[e]); 
                   r[e] = max(0, min(r[e], maxr[e]));
              }
              value = RGBvalue(r[0], r[1], r[2], bpp); 
           }else
              value = pixelat(add[j] + i2, bpp);
          putpixelbytes(buffer[j]+i2, value, 1, bpp, 1);
       }
   }
   if(!g.getout)
   for(j=y1;j<y2;j++)
   for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
   {    value = pixelat(buffer[j]+i2, bpp);
        putpixelbytes(z[ino].image[frame][j]+i2, value, 1, bpp, 1);
   }
   g.getout = 0;
   progress_bar_close(www);
   repair(ino);
   rebuild_display(ino);
   redraw(ino);
   z[ino].touched = 1;
   free(buffer);
   free(buffer_1d);
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// localcontrast_adaptive                                                   //
//--------------------------------------------------------------------------//
void localcontrast_adaptive(filter *ff)
{
   Widget www, scrollbar;
   int bpp,frame,i,i2,j, value, b;
   double contrast, maxrange, maxval, minval, difference, value2, value3;
   double shnew = ((double)ff->sharpfac)/100.0;  // Fraction of new pixel to use
   double shold = 1.0-shnew;                     // Fraction of old pixel to keep
   double decay = ff->decay;
   int ino = ff->ino;
   int xsize = z[ino].xsize;
   int ysize = z[ino].ysize;
   int xpos = z[ino].xpos;
   int ypos = z[ino].ypos;
   int x1 = g.ulx - xpos;
   int x2 = g.lrx - xpos;
   int y1 = g.uly - ypos;
   int y2 = g.lry - ypos;
   x1 = max(0, min(x1, xsize-1));                // ignore ff->x1, etc 
   x2 = max(0, min(x2, xsize-1));
   y1 = max(0, min(y1, ysize-1));
   y2 = max(0, min(y2, ysize-1));
   uchar *buffer_1d;
   uchar **buffer;
   uchar **add;
   bpp = z[ino].bpp;
   b = (7+bpp)/8;
   frame = z[ino].cf;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   printstatus(CALCULATING);
   buffer_1d = (uchar*)malloc(xsize*ysize*b);
   buffer    = make_2d_alias(buffer_1d, xsize*b, ysize);
   maxrange  = g.maxvalue[bpp];
   maxval    = maxrange;
   minval    = 0.0;
   progress_bar_open(www, scrollbar);
   add = z[ino].image[frame];
    
   for(j=y1;j<y2;j++)
   {
      progress_bar_update(scrollbar, j*100/ysize);        
      for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
      if(g.selected_is_square || inside_irregular_region(i+xpos,j+ypos))
      {
           value  = pixelat(add[j] + i2, bpp);  // Read center pixel
           value2 = read_surrounding_pixels(i,j,x1,y1,x2,y2,add,bpp,b,5); 
           value3 = read_surrounding_pixels(i,j,x1,y1,x2,y2,add,bpp,b,10); 
           maxval = decay * maxval + (1.0-decay) * max(value2, value3);
           minval = decay * minval + (1.0-decay) * min(value2, value3);
           difference = max(1.0, fabs(maxval - minval));
           contrast = 0.1 * maxrange / difference;
           value = cint(shold*value + shnew*(value - minval) * contrast); 
           value = max(0, min(value, (int)maxrange));
           putpixelbytes(buffer[j]+i2, value, 1, bpp, 1);
      }else
      {    value = pixelat(add[j] + i2, bpp);
           putpixelbytes(buffer[j]+i2, value, 1, bpp, 1);
      }
      check_event();
      if(g.getout) break;
   }
   if(!g.getout)
   for(j=y1; j<y2; j++)
   for(i=x1, i2=x1*b; i<x2; i++,i2+=b)
   {    value = pixelat(buffer[j]+i2, bpp);
        putpixelbytes(z[ino].image[frame][j]+i2, value, 1, bpp, 1);
   }
   g.getout = 0;
   progress_bar_close(www);
   repair(ino);
   rebuild_display(ino);
   redraw(ino);
   z[ino].touched = 1;
   free(buffer);
   free(buffer_1d);
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
// read_surrounding_pixels                                                  //
//--------------------------------------------------------------------------//
double read_surrounding_pixels(int i, int j, int x1, int y1, int x2, int y2,
    uchar **add, int bpp, int b, int dist)
{
   int yread1, yread2, xread1, xread2;
   double value;
   xread1 = max(x1  , i-dist);
   yread1 = max(y1  , j-dist);
   xread2 = max(x2-1, i+dist);
   yread2 = min(y2-1, j+dist);
   value = 0.25 * (double)(
            (pixelat(add[yread1] + xread1*b, bpp)) +
            (pixelat(add[yread2] + xread1*b, bpp)) +
            (pixelat(add[yread1] + xread2*b, bpp)) +
            (pixelat(add[yread2] + xread2*b, bpp)));
   return value;
}


//--------------------------------------------------------------------------//
//  force_background                                                        //
//--------------------------------------------------------------------------//
void force_background(filter *ff)
{
   printstatus(BUSY);
   int b,bpp,f,i,i2,j,k,obusy,sci, noofargs=0, value, smoothed_value;
   char **arg;                // up to 20 arguments
   arg = new char*[20];
   for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }
   int oci = ci;
   double factor;
   bpp = z[ci].bpp;
   f = z[ci].cf;
   bpp = z[ci].bpp;
   int target_value = ff->background;
   b = g.off[bpp];

   //// shrink=1.0 = old method
   double shrink = max(0.01, ff->sharpfac / 100.0);

   //// Duplicate image
   duplicate_image(ci,0,0,0,0);
   int nci = ci;

   //// Shrink the copy so smoothing covers wider area
   shrink_image_interpolate(nci, shrink, shrink); // This changes ci

   obusy = g.busy;
   g.busy = 0;
   eraseimage(nci,0,0,0);
   g.busy = obusy;

   sci = ci;

   filter ff2;
   ff2.ino = nci;
   ff2.x1 = 0;
   ff2.y1 = 0;
   ff2.x2 = z[nci].xsize-1;
   ff2.y2 = z[nci].ysize-1;
   ff2.type = 0;
   ff2.ksize = ff->ksize;
   ff2.sharpfac = 100;
   ff2.range = 1;
   ff2.kmult = 1; 
   ff2.maxbkg = 1;
   ff2.entireimage = 1;
   ff2.diffsize    = 10;
   ff2.ithresh     = 127;
   ff2.entireimage = 1;
   ff2.filename[0] = 0;
   ff2.want_progress_bar = 1;
   ff2.local_scale  = 10;
   ff2.decay        = 0.8;
   ff2.background   = target_value;
   ff2.do_filtering = 1;
   //// Low pass filter new one with ksize
   for(k=1; k<=ff->kmult; k++) filter_region(&ff2); 

   //// Return smoothed image to normal size
   shrink_image_interpolate(sci, 1.0/shrink, 1.0/shrink); // This changes ci

   obusy = g.busy;
   g.busy = 0;
   eraseimage(sci,0,0,0);
   g.busy = obusy;

   nci = ci;

   //// Multiply each pixel in oci by factor to make the corresponding
   //// pixel in nci equal to 127

   switchto(oci);

   //// In case 1.0/shrink is not an integer
   int xbound = min(z[oci].xsize, z[nci].xsize);
   int ybound = min(z[oci].ysize, z[nci].ysize);
   for(j=0; j<ybound; j++)
   for(i=0,i2=0; i<xbound; i++,i2+=b)
   {
       value = pixelat(z[oci].image[f][j]+i2, bpp);
       smoothed_value = pixelat(z[nci].image[f][j]+i2, bpp);
       if(smoothed_value) factor = target_value / (double) smoothed_value;
       else factor = 1.0;
       value = cint((double)value * factor);
       value = max(0, min(value, (int)g.maxvalue[bpp]));
       putpixelbytes(z[oci].image[f][j]+i2, value, 1, bpp, 1);  
   }

   //// Rescale
   sprintf(arg[1], "%d", 5);
   noofargs = 1;
   maximize_contrast(noofargs, arg);

   obusy = g.busy;
   g.busy = 0;
   eraseimage(nci,0,0,0);
   g.busy = obusy;
   if(oci != nci) switchto(oci);

   for(k=0;k<20;k++) delete[] arg[k];
   delete arg;  
   g.state = NORMAL;
   printstatus(NORMAL);
}


//--------------------------------------------------------------------------//
//  subtract_background                                                    //
//--------------------------------------------------------------------------//
void subtract_background(filter *ff)
{
   //// Duplicate image
   printstatus(BUSY);
   int b,bpp,f,i,i2,j,k, noofargs=0, value, smoothed_value;
   char **arg;                // up to 20 arguments
   arg = new char*[20];
   for(k=0;k<20;k++){ arg[k] = new char[128]; arg[k][0]=0; }
   int oci = ci;
   double factor=1.0;
   bpp = z[ci].bpp;
   f = z[ci].cf;
   bpp = z[ci].bpp;
   int target_value = ff->background;
   b = g.off[bpp];
   duplicate_image(ci,0,0,0,0);
   int nci = ci;

   filter ff2;
   ff2.ino = ci;
   ff2.x1 = 0;
   ff2.y1 = 0;
   ff2.x2 = z[ci].xsize-1;
   ff2.y2 = z[ci].ysize-1;
   ff2.type = 0;
   ff2.ksize = ff->ksize;
   ff2.sharpfac = 100;
   ff2.range = 1;
   ff2.kmult = 1; 
   ff2.maxbkg = 1;
   ff2.entireimage = 1;
   ff2.diffsize    = 10;
   ff2.ithresh     = 127;
   ff2.entireimage = 1;
   ff2.filename[0] = 0;
   ff2.want_progress_bar = 1;
   ff2.local_scale  = 10;
   ff2.decay        = 0.8;
   ff2.background   = target_value;
   ff2.do_filtering = 1;
   //// Low pass filter new one with ksize
   for(k=1; k<=ff->kmult; k++) filter_region(&ff2); 

   switchto(oci);
   factor = (double)ff->sharpfac / 100.0;
   for(j=0; j<z[oci].ysize; j++)
   for(i=0,i2=0; i<z[oci].xsize; i++,i2+=b)
   {
       value = pixelat(z[oci].image[f][j]+i2, bpp);
       smoothed_value = pixelat(z[nci].image[f][j]+i2,bpp);
       value += ff->background - cint(factor * (double)smoothed_value);
       value = max(0, min(value, (int)g.maxvalue[bpp]));
       putpixelbytes(z[oci].image[f][j]+i2, value, 1, bpp, 1);  
   }

   //// Rescale
   sprintf(arg[1], "%d", 5);
   noofargs = 1;
   maximize_contrast(noofargs, arg);

   int obusy = g.busy;
   g.busy = 0;
   eraseimage(nci,0,0,0);
   g.busy = obusy;
   if(oci !=ci) switchto(oci);

   for(k=0;k<20;k++) delete[] arg[k];
   delete arg;  
   printstatus(NORMAL);
}

