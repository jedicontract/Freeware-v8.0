//--------------------------------------------------------------------------//
// xmtnimage26.cc                                                           //
// Rotate,shrink                                                            //
// Latest revision: 11-28-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
int osignif;

//-------------------------------------------------------------------------//
// rotate - rotate selected part of image                                  //
//                                                                         //
//                  x2,y2                                                  //
//          +-------X--------+                                             //
//          |      /|\    original image (oxsize x oysize) before rotation //
// crop  ___|_   q/ | \  /   |                                             //
// boundary | \  /  |  \/    |                     a = oysize sin(t)       //
// after    |  3/___|__/\4  scanning boundary      b = a sin(t)            //
// rotation |  /|     / |\ / |                     c = b cos(t)            //
// (cc,dd)  |a/ |       | \  |                     d = oxsize sin(t)       //
//          |/  |b      |  \s                      e = oxsize sin(t)cos(t) //
//    x1,y1 /)c_|_      |___\ x3,y3                f = d sin(t)            //
//          \   |       |  (/                      h = oysize cos(t)cos(t) //
//          |\  |h      |  /t (angle)              p = c sin(t)            //
//          |p\ |       | /a                       q = oxsize cos(t)       //
//          |  \|_f_____|/   |  new image          s = oysize cos(t)       //
//          | /2\   |   /1   | /                                           //
//          |/  d\  |e /     |/          y|                                //
//          /     \ | /      |            |___                             //
//         /|      \|/       |              x                              //
//        / +-------X--------+ xsize,ysize                                 //
//    xpos,         x4,y4                                                  // 
//    ypos                                                                 //
// Puts image no. in param[1].                                             //
//-------------------------------------------------------------------------//
int rotate(char *title, double *angle, int noofargs, char **arg, int *param, 
    void(*user_ok_function)(dialoginfo *a, int radio, int box, int boxbutton),
    void(*user_cancel_function)(dialoginfo *a))
{
  static Dialog *dialog;
  int j,k;
  drawselectbox(OFF);
  g.getout=0; 
  osignif = g.signif;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return NOMEM; } 
  if(noofargs==0)
  { 
      dialog = new Dialog;
      if(dialog==NULL){ message(g.nomemory); return NOMEM; }
      dialog->helptopic=45;
      if(strlen(title))
         strcpy(dialog->title, title);
      else
         strcpy(dialog->title, "Rotate");

      ////--Radio buttons--////

      strcpy(dialog->radio[0][0],"Antialiasing");
      strcpy(dialog->radio[0][1],"On");
      strcpy(dialog->radio[0][2],"Off");
      if(g.rotate_anti_alias) dialog->radioset[0] = 1;
      else dialog->radioset[0] = 2;
      dialog->radiono[0]=3;
      dialog->radiono[1]=0;
      for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
      dialog->noofradios=1;

      ////-----Boxes-----////

      strcpy(dialog->boxes[0],"Rotation parameters");             
      strcpy(dialog->boxes[1],"Degrees");             
      dialog->boxtype[0]=LABEL;
      dialog->boxtype[1]=DOUBLECLICKBOX;
      dialog->boxmin[1]=0; dialog->boxmax[1] = 360;
          sprintf(dialog->answer[1][0], "%3.2f", *angle);
      dialog->noofboxes=2;
      dialog->want_changecicb = 0;
      dialog->f1 = rotatecheck;
      dialog->f2 = user_ok_function;
      dialog->f3 = rotatecancel;
      dialog->f4 = user_cancel_function;
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
      dialog->ptr[0] = param;
      dialog->ptr[3] = (void*)angle;
      strcpy(dialog->path,".");
      dialog->message[0]=0;      
      g.signif = 1;
      dialog->busy = 0;
      dialogbox(dialog);
  }else 
  {   if(noofargs>=1) *angle = atof(arg[1]);
      if(noofargs>=2) g.rotate_anti_alias = atoi(arg[2]);
      rotate_image(ci, *angle);
  }
  return OK;
}


//-------------------------------------------------------------------------//
// rotatecancel                                                            //
//-------------------------------------------------------------------------//
void rotatecancel(dialoginfo *a)
{
   a=a;
   g.getout = 1;
   if(a->radioset[0] == 1) g.rotate_anti_alias=1;
   else g.rotate_anti_alias=0;
   g.signif = osignif;
}


//--------------------------------------------------------------------------//
//  rotatecheck                                                             //
//--------------------------------------------------------------------------//
void rotatecheck(dialoginfo *dialog, int radio, int box, int boxbutton)
{
  double *angle, degrees;
  radio=radio; box=box; boxbutton=boxbutton;

  if(radio != -2) return;   //// If user clicked Ok or Enter, continue rotating
  if(g.getout) return;
  int *param = (int*)dialog->ptr[0];    // caller params from label()

  degrees = atof(dialog->answer[1][0]);
  angle = (double*)dialog->ptr[3];
  *angle = degrees;  // Set caller variable
  if(dialog->radioset[0] == 1 && degrees != 0.0) g.rotate_anti_alias=1;
  else g.rotate_anti_alias = 0;
  rotate_image(ci, degrees);
  if(param)
  {    if(g.getout) param[1] = 0; // Set image no. of label to invalid value
       else param[1] = ci;  // caller params from label()
  }
}


//--------------------------------------------------------------------------//
//  rotate_image                                                            //
//--------------------------------------------------------------------------//
void rotate_image(int ino, double degrees)
{
  int bpp,ibpp,iskip,j,k,cf,ct,oxsize,oysize,scancount=0,size,mode=0,
      ix1,ix2,iy1,iy2,j2,k2,xsize,ysize,oxpos,oypos,v,xs,ys,xe,ye,wantpalette,
      shell,obcolor,oci;
  double xx[4], yy[4], cc[5], dd[5];
  double a,b,c,d,e,f,h,q,s,dx,dy,r1,x1,x2,x3,x4,y1,y2,y3,y4;
  char tempstring[1024];
  if(ino<=0) return;
  unzoom(ino);
  ScanParams sp; 
  obcolor = g.bcolor;
  oxpos = g.selected_ulx;
  oypos = g.selected_uly;
  oxsize = g.selected_lrx-g.selected_ulx;
  oysize = g.selected_lry-g.selected_uly;
  ix1 = oxpos;
  iy1 = oypos;
  ix2 = ix1 + oxsize;
  iy2 = iy1 + oysize; 
  if(oxsize<=1 || oysize<=1 || (ino==0 && g.selectedimage==0))
  {   message("Please select an image or area first");
      return;
  }

  while(degrees>=360.0) degrees-=360.0;
  while(degrees<0.0) degrees+=360.0;
  
  r1 = degrees;
  while(r1>90.0) r1-=90.0;
  r1 = r1/DEGPERRAD;        // rotation angle in radians

  dx = sin(r1);  // increment in x direction (scan returns array spaced at 1 unit distances)
  dy = cos(r1);  // increment in y direction 

  a = (double)oysize * dx;
  b = a * dx;
  c = a * dy;
  d = (double)oxsize * dx;
  e = d * dy;
  f = d * dx;
  h = (double)oysize * dy * dy;   
  q = (double)oxsize * dy;
  s = (double)oysize * dy;

  xsize = (int)(a + q);
  ysize = (int)(d + s);
    
  if(between(degrees,90.001,180.001)) swap(xsize,ysize);
  if(between(degrees,270.001,359.999)) swap(xsize,ysize);

  if(ino>=0) 
  {   ct = z[ino].colortype;      
      bpp = z[ino].bpp;
      wantpalette = 0;
      switchto(ino);
  }else 
  {   ct = g.colortype;
      bpp = g.bitsperpixel;
      wantpalette = 1;
  }

  if(g.getlabelstate) g.bcolor = 0;
  if(g.getlabelstate) shell = 0; else shell = g.want_shell;

  //// This changes ci
  oci = ci;
  if(newimage("Image",0,0,1+xsize,1+ysize,bpp,ct,1,shell,1,PERM,wantpalette,
      g.window_border,0)!=OK)
  {   message(g.nomemory, ERROR);   
      g.bcolor = obcolor;
      return; 
  }
  for(j=0;j<1+ysize;j++) putpixelbytes(z[ci].image[0][j],g.bcolor,1+xsize,bpp,1);
  sprintf(tempstring,"rotated%3.4f", degrees);

  if(ino>=0)
  {   setimagetitle(ci,tempstring);
      memcpy(z[ci].palette, z[ino].palette, 768);
      memcpy(z[ci].opalette, z[ino].palette, 768);
      z[ci].hitgray = z[ino].hitgray;
      cf = z[ino].cf;
  }else 
  {   z[ci].hitgray=0;
      cf = 0;
  }
  iskip = ci;
  size = 100 +(int)sqrt(oxsize*oxsize + oysize*oysize);
  sp.ino         = oci;
  sp.skip        = ci;
  sp.leavemarked = 0;
  sp.scanwidth   = 0;
  sp.invert      = 0;
  sp.bpp         = bpp;
 
  if(ino>=0 && z[ino].colortype==GRAY)
     sp.wantcolor = 3;                  // Grayscale with antialiasing
  else if(g.rotate_anti_alias)
     sp.wantcolor = 1;                  // Colour with antialiasing
  else
     sp.wantcolor = 2;                  // No antialiasing
  sp.diameter    = 12;
     
  sp.scan = new double[size];
  for(k=0;k<size;k++)sp.scan[k] = (double)k;
  
  for(k=0;k<256;k++) sp.od[k] = k;

  x1 = oxpos - c;  
  y1 = oypos + h;
  x2 = oxpos + oxsize - f;
  y2 = oypos + oysize + e;
  x3 = oxpos + oxsize + c;
  y3 = oypos + b;
  x4 = oxpos + f;
  y4 = oypos - e;
  xs = z[ci].xpos;
  ys = z[ci].ypos;
  xe = xs + z[ci].xsize - 1;
  ye = ys + z[ci].ysize - 1;

  if(between(degrees,0.0, 0.001)) mode=1;
  if(between(degrees,89.999, 90.001)) mode=2;
  if(between(degrees,179.999, 180.001)) mode=3;
  if(between(degrees,269.999, 270.001)) mode=4;
  if(between(degrees,359.999, 360.0)) mode=1; 
  if(between(degrees,0.001,89.999))
  {   xx[1] = x1;     yy[1] = y1;   
      xx[2] = x2;     yy[2] = y2;
      cc[1] = xs;     dd[1] = ys+d; 
      cc[2] = xe-a;   dd[2] = ys; 
      cc[3] = xe;     dd[3] = ye-d; 
      cc[4] = xs+a;   dd[4] = ye; 
      dy*=-1;
  }

  if(between(degrees,90.001,179.999))
  {   xx[1] = x4;   yy[1] = y4;   
      xx[2] = x1;   yy[2] = y1;
      cc[1] = xs;   dd[1] = ys+a; 
      cc[2] = xe-d; dd[2] = ys; 
      cc[3] = xe;   dd[3] = ye-a; 
      cc[4] = xs+d; dd[4] = ye; 
      fswap(dx,dy);
  }                    
  if(between(degrees,180.001,269.999))
  {   xx[1] = x3;   yy[1] = y3;   
      xx[2] = x4;   yy[2] = y4;
      cc[1] = xs;   dd[1] = ys+d; 
      cc[2] = xe-a; dd[2] = ys; 
      cc[3] = xe;   dd[3] = ye-d; 
      cc[4] = xs+a; dd[4] = ye; 
      dx*=-1;
  }                    
  if(between(degrees,270.001,359.999))
  {   xx[1] = x2;   yy[1] = y2;   
      xx[2] = x3;   yy[2] = y3;
      cc[1] = xs;   dd[1] = ys+a; 
      cc[2] = xe-d; dd[2] = ys; 
      cc[3] = xe;   dd[3] = ye-a; 
      cc[4] = xs+d; dd[4] = ye; 
      dx*=-1;
      dy*=-1;
      fswap(dx,dy);
  }                    


  ////  Rotate the image
  printstatus(BUSY);
  switch(mode)
  {   case 0:     // arbitrary angle
         for(j=0;j<ysize;j++) 
         {     scan_fixed_width_area(&sp, xx, yy, scancount); 
               for(k=0;k<xsize;k++)          
               { 
                    if(!g.selected_is_square && !inside_irregular_region(k,j)) continue;
                    if(sp.scan[k]>g.maxvalue[bpp]||sp.scan[k]<0) sp.scan[k]=0;
                    putpixelbytes(z[ci].image[cf][ysize-j-1] + k*g.off[bpp], 
                        (int)sp.scan[k], 1);
               }
               yy[1] += dy;
               yy[2] += dy;
               xx[1] += dx;
               xx[2] += dx;
         }
         break;
      case 1:     //  0 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   if(!g.selected_is_square && !inside_irregular_region(k,j)) continue;
             v=readpixelonimage(k,j,ibpp,ino,iskip);
             putpixelbytes(z[ci].image[cf][j2]+k2*g.off[bpp], v, 1, bpp, 1);
         }
         break;
      case 2:    // 90 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   if(!g.selected_is_square && !inside_irregular_region(k,j)) continue;
             v=readpixelonimage(k,j,bpp,ino,ci);
             putpixelbytes(z[ci].image[cf][oxsize-k2] + j2*g.off[bpp],
                v,1,bpp,1);
         }
         break;
      case 3:     // 180 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   if(!g.selected_is_square && !inside_irregular_region(k,j)) continue;
             v=readpixelonimage(k,j,ibpp,ino,iskip);
             putpixelbytes(z[ci].image[cf][oysize-j2]+(oxsize-k2)*g.off[bpp],v,1,bpp,1);
         }
         break;
      case 4:     // 270 degrees
         for(j=iy1,j2=0;j<=iy2;j++,j2++) 
         for(k=ix1,k2=0;k<=ix2;k++,k2++)          
         {   if(!g.selected_is_square && !inside_irregular_region(k,j)) continue;
             v=readpixelonimage(k,j,bpp,ino,ci);
             putpixelbytes(z[ci].image[cf][k2]+(oysize-j2)*g.off[bpp],v,1,bpp,1);
         }
         break;
  } 
  delete[] sp.scan;
  rebuild_display(ci);
  z[ci].touched = 1;
  switchto(ci);


  ////  Crop 
  if(mode==0 && (dx!=0 || dy!=0)) crop_rectangle(xs,ys,xe,ye,cc,dd,g.bcolor);
  if(g.autoundo) backupimage(ci,0);
  printstatus(NORMAL);
  g.bcolor = obcolor;
  return;
}



//-------------------------------------------------------------------------//
// crop_rectangle - crop outside a rectangle                               //
//-------------------------------------------------------------------------//
void crop_rectangle(int xs, int ys, int xe, int ye, double *cc, double *dd, int color)
{
    fill_triangle_fast(xs,ys,(int)cc[1],(int)dd[1],(int)cc[2],(int)dd[2],color);  
    fill_triangle_fast(xs,ye,(int)cc[1],(int)dd[1],(int)cc[4],(int)dd[4],color);  
    fill_triangle_fast(xe,ye,(int)cc[3],(int)dd[3],(int)cc[4],(int)dd[4],color);  
    fill_triangle_fast(xe,ys,(int)cc[3],(int)dd[3],(int)cc[2],(int)dd[2],color);  
}


//-------------------------------------------------------------------------//
// fill_triangle_fast - x1--x2 must be vertical, x1--x3 must be horizontal //
//-------------------------------------------------------------------------//
void fill_triangle_fast(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
    int x,y;
    x2=x2;y3=y3;
    double fac;
    fac = ((double)(x3-x1)/(double)(y2-y1));
    for(y=y1;y!=y2;y+=sgn(y2-y1))
    {    x = x3 - (int)((y-y1)*fac);
         if(x1!=x) line(x1,y,x,y,color,SET);         
    }
}



//-------------------------------------------------------------------------//
// fill_triangle                                                           //
//-------------------------------------------------------------------------//
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
   lineinfo li;
   li.color = color;
   li.ino = ci; // If 0 it can put it on any image
   li.type = 0;
   li.width = 1;
   li.wantgrad = 0;
   li.gradr = 0;
   li.gradg = 0;
   li.gradb = 0;
   li.gradi = 0;
   li.skew = 0;
   li.perp_ends = 0;
   li.wantarrow = 0;
   li.arrow_width = 0.0;
   li.arrow_inner_length = 0.0;
   li.arrow_outer_length = 0.0;
   li.ruler_scale = 1.0;
   li.ruler_ticlength = 5.0;
   li.window = z[ci].win;
   li.wantprint = 0;
   li.antialias = 0;
   fill_triangle(x1, y1, x2, y2, x3, y3, &li);
}
void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, lineinfo *li)
{
   double denom,ys,ye;
   int x;

   // Don't remove this line - it is needed because compiler optimizes
   // incorrectly. Needs a delay to get the correct input values.   
   check_event();  
   
   if(x1>x2){ swap(x1,x2); swap(y1,y2); }
   if(x1>x3){ swap(x1,x3); swap(y1,y3); }
   if(x2>x3){ swap(x2,x3); swap(y2,y3); }

   for(x=x1;x<=x2;x++)
   {  
      denom=(double)(x3-x1); if(denom==0) denom=1.0;
      ys = y1 + ((double)(x-x1))*((double)(y3-y1))/denom;
      denom = (double)(x2-x1); if(denom==0) denom=1.0;
      ye = y1 + ((double)(x-x1))*((double)(y2-y1))/denom;
      line(x,cint(ys),x,cint(ye),g.imode,li);
   }
   for(x=x2;x<x3;x++)
   {  denom=(double)(x1-x3); if(denom==0) denom=1.0;
      ys = y3 + ((double)(x-x3))*((double)(y1-y3))/denom;
      denom=(double)(x2-x3); if(denom==0) denom=1.0;
      ye = y3 + ((double)(x-x3))*((double)(y2-y3))/denom;
      line(x,cint(ys),x,cint(ye),g.imode,li);
   }

}


//-------------------------------------------------------------------------//
// cint                                                                    //
//-------------------------------------------------------------------------//
int cint(double a)
{   return((int)(0.49999*sgn(a) + a)); }


//-------------------------------------------------------------------------//
// shrink                                                                  //
// Shrink or enlarge an image.  Returns error code. Creates new image.     //
// Use shrink_image_interpolate_in_place() to enlarge without creating     //
//    a new image.                                                         //
// mode=0 reduplicate pixels mode=1 interpolate pixels                     //
//-------------------------------------------------------------------------//
int shrink(int noofargs, char **arg, int mode)
{
   static char cxfactor[128]="1.0 ";
   static char cyfactor[128]="1.0 ";
   static char heading[128]="Shrink/Expand x factor:";
   static double sxfactor=1;
   static double syfactor=1;
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   char temp[128];
   g.getout = 0;
   char *ptr=NULL;
   heading[14]='x';

   if(noofargs==0) 
      message(heading,cxfactor,PROMPT,79,63);
   else   
      strcpy(cxfactor,arg[1]);
   sxfactor = strtod(cxfactor,&ptr);
   if(g.getout){ g.getout=0; return ABORT; }
   
   heading[14]='y';
   if(noofargs==0) 
      message(heading,cyfactor,PROMPT,79,63);
   else   
      strcpy(cyfactor,arg[2]);
   syfactor = strtod(cyfactor,&ptr);
   if(g.getout){ g.getout=0; return ABORT; }

   if(ci==0)
   {   message("Select an image first");
       return BADPARAMETERS;
   }
   if((sxfactor==0)||(syfactor==0)) return(BADPARAMETERS);
   if( ((sxfactor<1)&&(syfactor>1)) || ((syfactor<1)&&(sxfactor>1)) )
   {   message("Error - mixed expand and shrink operations",ERROR); 
       return(BADPARAMETERS);
   }
   sprintf(temp,"Creating new image\n%d bits/pixel",z[ci].bpp);
   message(temp);
   if(mode==0)
       shrink_image(ci, sxfactor, syfactor);
   else
       shrink_image_interpolate(ci, sxfactor, syfactor);
   if(g.autoundo) backupimage(ci,0);
   g.state = NORMAL;
   return OK;
}


//-------------------------------------------------------------------------//
// shrink_image                                                            //
//-------------------------------------------------------------------------//
int shrink_image(int ino, double sxfactor, double syfactor)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   int bpp,ibpp,ct,f,i,j,frames=1,v,xx,yy,yy2;
   int oino,oxsize,oysize,xstart,ystart,xsize,ysize;
   double invxfac, invyfac;
   uint *buffer;
   g.getout = 0;
   if(ino < 0) return BADPARAMETERS;
   bpp = z[ino].bpp; 

   oxsize = 1 + g.lrx - g.ulx;
   oysize = 1 + g.lry - g.uly;
   xstart = g.ulx;
   ystart = g.uly;
         
   xsize=(int)(sxfactor*(oxsize));
   ysize=(int)(syfactor*(oysize));
   if((xsize<2)||(ysize<2)) return(BADPARAMETERS);

   if(g.selectedimage>=0) ct = z[ino].colortype;
   else { if(bpp<=8) ct=INDEXED; else ct=COLOR; }
   if(newimage("Image",0,0,xsize,ysize,bpp,ct,frames,g.want_shell,1,PERM,0,g.window_border,0)!=OK)
       return(NOMEM);    

   oino = ino;
   if(oino>=0) frames = z[oino].frames;

   g.state = BUSY;
   drawselectbox(OFF);

   if(ci>=0)
   {   z[ci].touched = 1;
       setimagetitle(ci,"Untitled");
   }
   if(ci>=0 & oino>=0) 
   {   memcpy(z[ci].palette, z[oino].palette, 768);
       memcpy(z[ci].opalette, z[oino].palette, 768);
   }

   ////  Split read & write by using an intermediate buffer in case image  
   ////  is in virtual memory.                                             

   invxfac = 1.0 / sxfactor;
   invyfac = 1.0 / syfactor;
   buffer = new uint[g.off[bpp]*(xsize+50)]; // big safety margin
   for(f=0;f<frames;f++) 
   for(j=0;j<ysize;j++) 
     {  yy2 = cint(j * invyfac);
        yy = ystart + yy2;        
        if(g.selectedimage==-1)    // screen region
        {  for(i=0;i<xsize;i++)          
           {    xx = xstart + cint(i * invxfac); 
                v = readpixelonimage(xx,yy,ibpp,ino,ci);            
                buffer[i] = convertpixel(v, ibpp, bpp, 1);
           }
        }else                      // entire image (can get frames)
        {  for(i=0; i<xsize; i++)          
           {    xx = cint(i * invxfac); 
                if(between(xx,0,z[oino].xsize-1) && between(yy2,0,z[oino].ysize-1))
                   buffer[i] = pixelat(z[oino].image[f][yy2]+xx*g.off[bpp], bpp);
           }
        } 
        for(i=0;i<xsize;i++)       // Put data in image buffer
             putpixelbytes(z[ci].image[f][j]+i*g.off[bpp], buffer[i], 1, bpp, 1);     
   }
   delete[] buffer;
   switchto(ci);
   repair(ci);
   return OK;
}


//-------------------------------------------------------------------------//
// shrink_image_interpolate                                                //
//-------------------------------------------------------------------------//
int shrink_image_interpolate(int ino, double sxfactor, double syfactor)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   int bpp,ct,f,i,j,frames=1;
   int oino,oxsize,oysize,xstart,ystart,xsize,ysize;
   double invxfac, invyfac;
   double xx,yy,yy2;
   uint *buffer;
   g.getout = 0;
   if(ino < 0) return BADPARAMETERS;
   bpp = z[ino].bpp; 

   oxsize = 1 + g.lrx - g.ulx;
   oysize = 1 + g.lry - g.uly;
   xstart = g.ulx;
   ystart = g.uly;
         
   xsize=(int)(sxfactor*(oxsize));
   ysize=(int)(syfactor*(oysize));
   if((xsize<2)||(ysize<2)) return(BADPARAMETERS);

   if(g.selectedimage>=0) ct = z[ino].colortype;
   else { if(bpp<=8) ct=INDEXED; else ct=COLOR; }
   if(newimage("Image",0,0,xsize,ysize,bpp,ct,frames,g.want_shell,1,PERM,0,g.window_border,0)!=OK)
       return(NOMEM);    

   oino = ino;
   if(oino>=0) frames = z[oino].frames;

   g.state = BUSY;
   drawselectbox(OFF);

   if(ci>=0)
   {   z[ci].touched = 1;
       setimagetitle(ci,"Untitled");
   }
   if(ci>=0 & oino>=0) 
   {   memcpy(z[ci].palette, z[oino].palette, 768);
       memcpy(z[ci].opalette, z[oino].palette, 768);
   }

   ////  Split read & write by using an intermediate buffer in case image  
   ////  is in virtual memory.                                             

   invxfac = 1.0 / sxfactor;
   invyfac = 1.0 / syfactor;
   buffer = new uint[g.off[bpp]*(xsize+50)]; // big safety margin
   for(f=0;f<frames;f++) 
   for(j=0;j<ysize;j++) 
     {  yy2 = (double)j * invyfac;
        yy = (double)ystart + yy2;        
        for(i=0; i<xsize; i++)          
        {    xx = (double)i * invxfac; 
             buffer[i] = read_anti_aliased_pixel(xx, yy2, oino);
        } 
        for(i=0;i<xsize;i++)       // Put data in image buffer
             putpixelbytes(z[ci].image[f][j]+i*g.off[bpp], buffer[i], 1, bpp, 1);     
   }
   delete[] buffer;
   switchto(ci);
   repair(ci);
   return OK;
}


//-------------------------------------------------------------------------//
// shrink_image_interpolate_in_place                                       //
// center point is unchanged                                               //
//-------------------------------------------------------------------------//
int shrink_image_interpolate_in_place(int ino, double sxfactor, double syfactor)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   int bpp,f,i,ii,j,jj,frames=1;
   int xsize,ysize,xoffset,yoffset;
   double invxfac, invyfac;
   double xx,yy;
   uint *buffer;
   g.getout = 0;
   if(ino < 0) return BADPARAMETERS;
   bpp = z[ino].bpp; 

   xsize=(int)(sxfactor*z[ino].xsize);
   ysize=(int)(syfactor*z[ino].ysize);
   g.state = BUSY;
   drawselectbox(OFF);

   ////  Split read & write by using an intermediate buffer in case image  
   ////  is in virtual memory.                                             

   invxfac = 1.0 / sxfactor;
   invyfac = 1.0 / syfactor;
   xoffset = (xsize - z[ino].xsize) / 2; // pixels to shift left to keep centered
   yoffset = (ysize - z[ino].ysize) / 2; // pixels to shift left to keep centered

   ////  Shrink x size first
   if(fabs(sxfactor - 1.0) > 0.001)
   {
       buffer = new uint[g.off[bpp]*(xsize+50)]; // big safety margin
       for(f=0;f<frames;f++) 
       for(j=0;j<ysize;j++) 
       {    yy = (double)j * invyfac;
            for(i=0; i<xsize; i++)          
            {    xx = (double)i * invxfac; 
                 buffer[i] = read_anti_aliased_pixel(xx, yy, ino);
            } 
            for(i=0;i<xsize;i++)       // Put data in image buffer
            {    ii = max(0, min((i-xoffset), z[ino].xsize-1));
                 putpixelbytes(z[ino].image[f][j] + ii*g.off[bpp], buffer[i], 1, bpp, 1);     
            }
       }
       delete[] buffer;
   }

   ////  Shrink y size second
   if(fabs(syfactor - 1.0) > 0.001)
   {
       buffer = new uint[g.off[bpp]*(ysize+50)]; // big safety margin
       for(f=0;f<frames;f++) 
       for(i=0;i<xsize;i++) 
       {    xx = (double)i * invxfac;
            for(j=0; j<ysize; j++)          
            {    yy = (double)j * invyfac; 
                 buffer[j] = read_anti_aliased_pixel(xx, yy, ino);
            } 
            for(j=0;j<ysize;j++)       // Put data in image buffer
            {    jj = max(0, min((j-yoffset), z[ino].ysize-1));
                 putpixelbytes(z[ino].image[f][jj] + i*g.off[bpp], buffer[j], 1, bpp, 1);     
            }
       }
       delete[] buffer;
   }

   g.state = OK;
   repair(ci);
   return OK;
}





