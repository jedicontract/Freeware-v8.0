//--------------------------------------------------------------------------//
// xmtnimage2.cc                                                            //
// convert, etc.                                                            //
// Latest revision: 03-29-2003                                              //
// Copyright (c) 2003 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Any routine that uses putpixelbytes() instead of setpixelonimage()       //
// must call  rebuild_display() to update the screen copy of the image      //
// or else call putpixelbytes() again for img[ci]. It must also set         //
// i_touched[ci] to 1 to indicate it was modified.  Using rebuild_display() //
// only needs to be done if pixels are being read and set in the same loop  //
// or when a new image is created.  If setpixelonimage() is used, i_touched //
// [] is set automatically.                                                 //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"
#include "xmtnimagec.h"
extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
extern PrinterStruct printer;
int wuquantize2(int bpp, uchar *buf1, uchar *buf2, int size, int &ncolors);

//--------------------------------------------------------------------------//
// read_data                                                                //
//--------------------------------------------------------------------------//
int read_data(char *filename, double data[], int count)
{
  if(ci<0 || count<=0) return BADPARAMETERS;
  FILE *fp;
  int j;
  g.getout=0;
  getfilename(filename, NULL);
  g.getout = 0;
  if(g.getout) return(ABORT);

  if ((fp=fopen(filename,"r")) == NULL)
  {   error_message(filename, errno);
      chdir(g.currentdir); // Change back to original directory
      return(ERROR);
  }
  for(j=0;j<count;j++) fscanf(fp,"%lg",&data[j]); 
  fclose(fp);  
  return(OK);
}


//-------------------------------------------------------------------------//
// save_scan  - if mode is 1 it skips asking for filename                  //
//-------------------------------------------------------------------------//
void save_scan(char *filename, int *data, int count, int mode) 
{
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  static PromptStruct ps;
  ps.text = filename;
  ps.n = count;
  ps.data = data;
  ps.maxsize = FILENAMELENGTH-1;
  ps.helptopic = 54;
  ps.f1 = save_scan_int_part2;
  ps.f2 = null;
  if(mode!=1) getfilenameps(&ps);  
}


//-------------------------------------------------------------------------//
// save_scan_int_part2                                                     //
//-------------------------------------------------------------------------//
void save_scan_int_part2(PromptStruct *ps)
{
  if(!ps->text){ message("No text"); return; }
  ps->filename = ps->text;
  if(ps->n<=0 || !ps->data){ message("No data to save",ERROR); return; }
  ps->f1 = save_scan_int_part3;
  check_overwrite_file(ps);
}


//-------------------------------------------------------------------------//
// save_scan_int_part3                                                     //
//-------------------------------------------------------------------------//
void save_scan_int_part3(PromptStruct *ps)
{
  int k;
  char temp[FILENAMELENGTH];
  if(!ps->filename){ message("No filename"); return; }
  if(!ps->data || ps->n<=0){ message("No data"); return; }
  if(g.getout){ message("Aborted"); return; }
  FILE *fp;
  if((fp=fopen(ps->filename,"w")) == NULL)
       error_message(ps->filename, errno);
  else
  {   for(k=0; k<ps->n; k++) fprintf(fp, "%d\t%u\n", k,ps->data[k]);
      sprintf(temp,"Data saved in %s",ps->filename);
      message(temp);
      fclose(fp);
  }
} 


//-------------------------------------------------------------------------//
// save_scan  - if mode is 1 it skips asking for filename                  //
//-------------------------------------------------------------------------//
void save_scan(char *filename, double *data, int count, int mode) 
{  
  static PromptStruct ps;
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
  if(count==0){ message("No data to save",ERROR); return; }
  ps.text = filename;
  ps.n = count;
  ps.fdata = data;
  ps.maxsize = FILENAMELENGTH-1;
  ps.helptopic = 54;
  ps.f1 = save_scan_double_part2;
  ps.f2 = null;
  ps.fdata = data;
  ps.n = count;
  if(mode!=1) getfilenameps(&ps);
}


//-------------------------------------------------------------------------//
// save_scan_double_part2                                                  //
//-------------------------------------------------------------------------//
void save_scan_double_part2(PromptStruct *ps)
{
  ps->filename = ps->text;
  ps->f1 = save_scan_double_part3;
  check_overwrite_file(ps);
}


//-------------------------------------------------------------------------//
// save_scan_double_part3                                                  //
//-------------------------------------------------------------------------//
void save_scan_double_part3(PromptStruct *ps)
{
  int k;
  char temp[128];
  if(!ps->filename){ message("No filename"); return; }
  if(!ps->fdata || ps->n<=0){ message("No data"); return; }
  if(g.getout){ message("Aborted"); return; }
  FILE *fp;
  if((fp=fopen(ps->filename,"w")) == NULL)
       error_message(ps->filename, errno);
  else
  {    for(k=0; k<ps->n; k++) fprintf(fp,"%d\t%f\n", k, ps->fdata[k]);
       sprintf(temp,"Data saved in %s", ps->filename);
       message(temp);
       fclose(fp);
  }
}


//-------------------------------------------------------------------------//
// crop                                                                    //
// Erase everything but selected area on image ci                          //
//-------------------------------------------------------------------------//
void crop(int noofargs, char **arg)
{
   int ino,xx1,yy1,xx2,yy2;
   ino = ci;
   if(ino<=0){ message("Select an image first"); return; }
   if(noofargs==0)
   {    printstatus(BUSY);   
        message("Select region to crop\nPress a key when finished\nEsc to abort");
        select_region(z[ci].xpos,z[ci].ypos,z[ci].xpos+z[ci].xsize,z[ci].ypos+z[ci].ysize);
        g.key_ascii = 0;
        g.in_crop++;
        while(1)
        {   check_event();
            if(between(g.key_ascii & 0xff, 27, 94)) break; 
            usleep(10000);
        }
        g.in_crop--;
   } 
   
   if(noofargs >=1) xx1=atoi(arg[1]);
   else             xx1 = max(0, g.ulx - z[ino].xpos);
   if(noofargs >=2) yy1=atoi(arg[2]);
   else             yy1 = max(0, g.uly - z[ino].ypos);
   if(noofargs >=3) xx2=atoi(arg[3]);
   else             xx2 = min(z[ino].xsize, g.lrx - z[ino].xpos);
   if(noofargs >=4) yy2=atoi(arg[4]);
   else             yy2 = min(z[ino].ysize, g.lry - z[ino].ypos);
   drawselectbox(OFF);
   if(g.key_ascii == 27 || ino<=0) return;
   if(ci != ino) return;
   image_shift_frame(ino, -xx1, -yy1);
   if(resize_image(ino, max(1, xx2-xx1), max(1, yy2-yy1))) return;
   moveimage(ino, xx1, yy1);
   if(z[ino].backedup) backupimage(ino, 0);
}


//-------------------------------------------------------------------------//
// pointinfo   - gets information on a pixel, given its virtual x and y    //
//         coord. (more complete than whichimage() or whatbpp().           //
//         Called by changebrightness().                                   //
//         iscolor, isbw, white, and black must be initialized first.      //
//         iscolor, bpp, white, black,and isbw are passed by reference.    //
//-------------------------------------------------------------------------//
int pointinfo(int x1,int y1,int &bpp,int &iscolor,int &isbw,int &white,
               int &black)
{   int ino;
    ino=whichimage(x1,y1,bpp);
    if(ino>=0)
    {   if(z[ino].colortype!=GRAY)
        {   iscolor=1;
            white=255;
            black=0;
        }else 
        {   isbw=1; 
            white=z[ino].gray[0];
            black=z[ino].gray[1];
        }
    }else
    {   if(g.bitsperpixel==8) 
        {   isbw=1; 
            white=255;
            black=0;
        }else iscolor=1; 
    }
    return ino;
}


//-------------------------------------------------------------------------//
// invertimage - inverts colors in image. Works for entire image only.     //
//               (Called by invertcolors())                                //
//-------------------------------------------------------------------------//
void invertimage(void)
{
  int f,h,j,rr,gg,bb,bpp;
  uint value;
  if(ci<0) return;
  switch_palette(ci);
  drawselectbox(OFF);
  bpp = z[ci].bpp;  
  f = z[ci].cf;
  
  if(z[ci].colortype==GRAY)
  {    for(j=0; j<z[ci].ysize; j++)
       for(h=0; h<g.off[bpp]*z[ci].xsize; h+=g.off[bpp])
       {    value = (int)(g.maxvalue[bpp] - pixelat(z[ci].image[f][j]+h,bpp));
            putpixelbytes(z[ci].image[f][j]+h,value,1,bpp);
       }
       z[ci].hitgray = 0;   // Force remapping of gray levels
  }else
  {
       if(bpp==8)
       {    for(h=0;h<=255;h++)
            {    z[ci].palette[h].red   = 63 - z[ci].palette[h].red;
                 z[ci].palette[h].green = 63 - z[ci].palette[h].green;
                 z[ci].palette[h].blue  = 63 - z[ci].palette[h].blue;
            }
            setpalette(z[ci].palette);  
       }else
       {    for(j=0; j<z[ci].ysize; j++)
            for(h=0; h<g.off[bpp]*z[ci].xsize; h+=g.off[bpp])
            { 
               RGBat(z[ci].image[f][j]+h, bpp, rr,gg, bb);
               rr = g.maxred[bpp]  - rr;
               gg = g.maxgreen[bpp]- gg;
               bb = g.maxblue[bpp] - bb;
               value = RGBvalue(rr,gg,bb,bpp);
               putRGBbytes(z[ci].image[f][j]+h, rr, gg, bb, bpp);
            }
       }
  }

  z[ci].touched = 1;
  rebuild_display(ci);
  redraw(ci);
  switchto(ci);
}


//-------------------------------------------------------------------------//
// invertcolors                                                            //
//-------------------------------------------------------------------------//
void invertcolors(int noofargs, char **arg)
{ 
  int i,ino,j,k,rr,gg,bb,bpp,x1,x2,y1,y2;
  int noscreen=0;
  uint value;
  drawselectbox(OFF);
  x1 = g.ulx;
  y1 = g.uly;
  x2 = g.lrx;
  y2 = g.lry;
  if(noofargs >=1) x1=atoi(arg[1]);
  if(noofargs >=2) y1=atoi(arg[2]);
  if(noofargs >=3) x2=atoi(arg[3]);
  if(noofargs >=4) y2=atoi(arg[4]);
  
  for(k=0; k<g.image_count; k++) z[k].hit=0;
  if(g.selectedimage>-1 && noofargs==0)   //// If entire image was selected
      invertimage(); 
  else 
  {  
     for(j=y1;j<=y2;j++)
     for(i=x1;i<=x2;i++)
     {  
            if(!g.selected_is_square && !inside_irregular_region(i,j)) continue;
            ino = whichimage(i, j, bpp);
            if(ino>=0)
            {   if(z[ino].hit==0 && z[ino].colortype==INDEXED) 
                {    z[ino].hit=2;  // Must be set before change_image_depth
                     if(change_image_depth(ino,24,TEMP)==OK) bpp = 24;
                     else
                     {   message("Insufficient image buffers available,\naborting...");
                         g.getout=1; 
                         break;
                     }
                }
            }
            if(ino>=0){ if(z[ino].hit==0) z[ino].hit=1; noscreen=1; } 
            else noscreen=0;
             
            if(ino>=0 && (!g.wantr || !g.wantg || !g.wantb) && z[ino].hit==0) 
                z[ino].hit=1;
            if(bpp==8)
            { 
                value = readpixelonimage(i,j,bpp,ino);
                if(z[ino].colortype==GRAY)
                {   value = (uint)g.maxvalue[8]-value;
                    setpixelonimage(i,j,value,SET,bpp,-1,noscreen);
                }
            }else
            { 
                readRGBonimage(i, j, bpp, ino, rr, gg, bb, -2);
                rr = g.maxred[bpp]  - rr;
                gg = g.maxgreen[bpp]- gg;
                bb = g.maxblue[bpp] - bb;
                setRGBonimage(i,j,rr,gg,bb,SET,bpp,-1,noscreen);
            }                          
     }
           
     ////  Re-palettize any color images touched if in 8-bpp screen mode.
     ////  Convert any 8-bit color images back to 8 bpp.
     ////  Note: z[].hit is attached to the image. Can't use an array to
     ////    indicate which images were changed because images are reordered
     ////    by change_image_depth() and createimage().
      
     for(k=0; k<g.image_count; k++) 
     {   if(z[k].hit==2) 
         {     z[k].hit=1;
               change_image_depth(k,8,TEMP);
               k=-1;      // Start over because image numbers may be changed.
               continue;
         }
         if(z[k].hit)
         {     rebuild_display(k); 
               redraw(k); 
               if(z[k].colortype==INDEXED) memcpy(z[k].opalette,z[k].palette,768);
         }
     }
   }
   if(ci>=0 && z[ci].colortype==INDEXED) memcpy(z[ci].opalette,z[ci].palette,768);
}             


//-------------------------------------------------------------------------//
// convertpixel - converts color depth of a pixel from bpp1 to bpp2.       //
// For monochrome pixels, it merely scales the pixel value.  'wantcolor'   //
// should be 1 for all situations except writing to a file when both       //
// and destination pixel are both monochrome.                              //
// (Default value=1)                                                       //
// If bpp1 is 8, the desired color palette must be set in advance to get   //
// the correct colors.                                                     //
// The returned pixel should appear the same as the starting pixel, except //
//   its bpp is different.                                                 //
//-------------------------------------------------------------------------//
uint convertpixel(uint color, int bpp1, int bpp2, int wantcolor)
{
    int rshift=0, gshift=0, bshift=0, ishift=0, rr, gg, bb;
    int rshift2=0, gshift2=0, bshift2=0, ishift2=0;
    if(bpp1==bpp2) return(color);

    //// Factor to convert r,g,b of pixel to 0..255 or intensity to 0..2^24
    //// Add 1 before multiplications to preserve white point
    //// Example:  0..63 -> 1..64 -> 4..256 -> 3..355
  
    if(bpp2==1) return(graypixel(color, bpp1) > g.maxgray[bpp1]/2);    

    switch(bpp1)    
    { case  8:  rshift = 2; gshift = 2; bshift = 2; ishift=16; break;
      case 15:  rshift = 3; gshift = 3; bshift = 3; ishift= 9; break;
      case 16:  rshift = 3; gshift = 2; bshift = 3; ishift= 8; break;
      case 24:  rshift = 0; gshift = 0; bshift = 0; ishift= 0; break;
      case 32:  rshift = 0; gshift = 0; bshift = 0; ishift= 0; break;
      case 48:  rshift = 0; gshift = 0; bshift = 0; ishift= 0; break;
    }

    //// factor to convert r,g,b of 0..255 or 0..2^24 to desired depth 
    
    switch(bpp2)  
    { case  8:  rshift2= 2; gshift2= 2; bshift2= 2; ishift2=16; break;
      case 15:  rshift2= 3; gshift2= 3; bshift2= 3; ishift2= 9; break;
      case 16:  rshift2= 3; gshift2= 2; bshift2= 3; ishift2= 8; break;
      case 24:  rshift2= 0; gshift2= 0; bshift2= 0; ishift2= 0; break;
      case 32:  rshift2= 0; gshift2= 0; bshift2= 0; ishift2= 0; break;
      case 48:  rshift2= 0; gshift2= 0; bshift2= 0; ishift2= 0; break;
    }

    if(wantcolor)
    {    valuetoRGB(color,rr,gg,bb,bpp1);
         rr = (((1+rr)<<rshift)>>rshift2)-1;  // Do in 2 steps to avoid negative shifts
         gg = (((1+gg)<<gshift)>>gshift2)-1;  // Do in 2 steps to avoid negative shifts
         bb = (((1+bb)<<bshift)>>bshift2)-1;  // Do in 2 steps to avoid negative shifts
         rr = max(rr,0);
         gg = max(gg,0);
         bb = max(bb,0);
         color = RGBvalue(rr,gg,bb,bpp2);
    }else
         color = (((color)<<ishift)>>ishift2);         
    return(color);
} 


//-------------------------------------------------------------------------//
// convertRGBpixel                                                         //
//-------------------------------------------------------------------------//
void convertRGBpixel(int &r, int &g, int &b, int bpp1, int bpp2)
{
    int rshift=0, gshift=0, bshift=0;
    int rshift2=0, gshift2=0, bshift2=0;
    if(bpp1==bpp2) return;

    //// Factor to convert r,g,b of pixel to 0..65535 
    //// Add 1 before multiplications to preserve white point

    switch(bpp1)    
    { case  8:  rshift = 10; gshift = 10; bshift = 10; break;
      case 15:  rshift = 11; gshift = 11; bshift = 11; break;
      case 16:  rshift = 11; gshift = 10; bshift = 11; break;
      case 24:  rshift =  8; gshift =  8; bshift =  8; break;
      case 32:  rshift =  8; gshift =  8; bshift =  8; break;
      case 48:  rshift =  0; gshift =  0; bshift =  0; break;
    }

    //// factor to convert r,g,b of 0..255 or 0..2^24 to desired depth 
    
    switch(bpp2)  
    { case  8:  rshift2 = 10; gshift2 = 10; bshift2 = 10; break;
      case 15:  rshift2 = 11; gshift2 = 11; bshift2 = 11; break;
      case 16:  rshift2 = 11; gshift2 = 10; bshift2 = 11; break;
      case 24:  rshift2 =  8; gshift2 =  8; bshift2 =  8; break;
      case 32:  rshift2 =  8; gshift2 =  8; bshift2 =  8; break;
      case 48:  rshift2 =  0; gshift2 =  0; bshift2 =  0; break;
    }

    if(r) r = (((1+r)<<rshift)>>rshift2)-1;  // Do in 2 steps to avoid negative shifts
    if(g) g = (((1+g)<<gshift)>>gshift2)-1;  // Do in 2 steps to avoid negative shifts
    if(b) b = (((1+b)<<bshift)>>bshift2)-1;  // Do in 2 steps to avoid negative shifts
} 


//--------------------------------------------------------------------------//
// repair  - repairs image                                                  //
//--------------------------------------------------------------------------//
void repair(int ino)
{
   drawselectbox(OFF);
   if(ino>=0)
   {    //z[ino].hitgray = 0;   // Force remapping of gray levels
        rebuild_display(ino);
        switch_palette(ino); 
        redraw(ino);
   }else
   {    memcpy(g.palette,g.b_palette,768);
        repairimg(-1,0,0,g.main_xsize,g.main_ysize); 
        switchto(-1); 
        redraw(-1);
   }
}


//--------------------------------------------------------------------------//
// remap_pixels                                                             //
// Converts a block of pixels with colors defined by one colormap to the    //
// closest available pixel values defined by a second colormap.             //
// Uses the colormaps associated with two images.                           //
// Slower than repairimg because it remaps all the pixels, not just the     //
// reserved ones.                                                           //
// Used for copying regions from one indexed-color image to another.        //
// `buf' is a uchar[0..h][0..w] representing an image buffer.  Assumes the  //
// pixels in buf are only 8 bits, even though ino_src may be 24 bits.       //
//--------------------------------------------------------------------------//
void remap_pixels(uchar **buf, int w, int h, int ino_src, int ino_dest)
{
   int bytesperpixel,i,i2,j;
   if(ino_src>=0 && ino_dest>=0)
   {  
       if(g.bitsperpixel!=8 && z[ino_src].bpp!=8 && z[ino_dest].bpp!=8) return;   
       if(z[ino_src].colortype==GRAY && z[ino_dest].colortype == GRAY) return;
   }

   int remap[256];
   RGB *src_palette, *dest_palette;
   if(ino_src >= 0) src_palette = z[ino_src].palette;
   else             src_palette = g.b_palette;
   if(ino_dest>= 0) dest_palette = z[ino_dest].palette;
   else             dest_palette = g.b_palette;

   remap_palette(src_palette, dest_palette, remap);

   bytesperpixel = g.off[z[ino_src].bpp];
   for(j=0;j<h;j++)
   for(i=0,i2=0; i<w; i++,i2+=bytesperpixel)
        buf[j][i2] = remap[buf[j][i2]]; 
}


//--------------------------------------------------------------------------//
// remap_palette                                                            //
// find the closest mapping between two 8-bit RGB colormaps                 //
//--------------------------------------------------------------------------//
void remap_palette(RGB *pal1, RGB *pal2, int *remap)
{
   int d,j,k,best,r0,g0,b0,rr,gg,bb;
   for(k=0;k<256;k++)
   { 
      ////  Get the desired rgb values
      r0 = pal1[k].red;
      g0 = pal1[k].green;
      b0 = pal1[k].blue;
      best = 3*63*63;

      ////  Find value with closest rgb in destination colormap
      for(j=0;j<256;j++)
      {  rr = pal2[j].red;
         gg = pal2[j].green;
         bb = pal2[j].blue;
         d = (rr-r0)*(rr-r0) + (gg-g0)*(gg-g0) + (bb-b0)*(bb-b0);
         if(d<best){ best=d; remap[k]=j; }  // j will be subsituted for k
      }
   }
}


//--------------------------------------------------------------------------//
// repairimg                                                                //
// remap all pixel values in z[ino].img for an image that coincide with     //
// reserved colors.                                                         //
// If ino=-1, it repairs the background.                                    //
//--------------------------------------------------------------------------//
void repairimg(int ino, int x1, int y1, int x2, int y2)
{
   int d,i,j,k,rr,gg,bb,r0,g0,b0,best;
   int remap[256];
   if(g.bitsperpixel!=8) return; 

   ////  Find the closest non-reserved color.                  
   ////  The def_colors range from 0-65535 while i_palette[] colors range 
   ////  from 0-63, so divide the former by 1024 when doing comparisons. 
   for(k=0;k<256;k++)
   {  remap[k]=k;
      if(ino>=0)
      {    r0 = z[ino].palette[k].red;
           g0 = z[ino].palette[k].green;
           b0 = z[ino].palette[k].blue;
      }else
      {    r0 = g.b_palette[k].red;
           g0 = g.b_palette[k].green;
           b0 = g.b_palette[k].blue;
      }
      
      best = 3*63*63;
      for(j=0;j<256;j++)
      { 
           //// Remove this line to include reserved colors in the remap for grayscale
           //// images as well as pseudocolor and color. This will 
           //// give more realistic gray levels but prevent changing some of them.
           //// On Solaris, where there are only a few free color cells, this line
           //// causes grayscale images to be displayed in the wrong color.
                     
           if(ino>=0 && z[ino].colortype==GRAY)
           { if(g.reserved[j])continue;}

           if(g.reserved[j])
           {    rr = g.def_colors[j].red>>10;
                gg = g.def_colors[j].green>>10;
                bb = g.def_colors[j].blue>>10;
           }else if(ino>=0)
           {    rr = z[ino].palette[j].red;
                gg = z[ino].palette[j].green;
                bb = z[ino].palette[j].blue;
           }else
           {    rr = g.b_palette[j].red;
                gg = g.b_palette[j].green;
                bb = g.b_palette[j].blue;
           }
           d = (rr-r0)*(rr-r0) + (gg-g0)*(gg-g0) + (bb-b0)*(bb-b0);
           if(d<best){ best=d; remap[k]=j; }
           if(d==0)break;
      }
   }

   //// Remap img[ino]

   if(ino>=0)
   {   x1 = max(x1,0);
       y1 = max(y1,0);
       x2 = min(x2,z[ino].xsize);
       y2 = min(y2,z[ino].ysize);
       for(j=y1;j<y2;j++)
       for(i=x1;i<x2;i++)
           z[ino].img[j][i] = remap[z[ino].img[j][i]];
   }
   return;        
}


//-------------------------------------------------------------------------//
// rebuild_display                                                         //
// Convert image[ino][f] from its current bits/pixel to g.bitsperpixel bpp //
//   and puts the result in img[ino].                                      //
// Returns error code (OK if successful).                                  //
// If image is grayscale, the grayscale factors i_gray[ino][4] are used    //
//  to map the intensity values to display colors.                         //
//  i_gray[0] = input white level (maximum white in image)                 //
//  i_gray[1] = input black level (minimum black in image)                 //
//  i_gray[2] = output white level                                         //
//  i_gray[3] = output black level                                         //
//-------------------------------------------------------------------------//
int rebuild_display(int ino)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); } 
  if(ino<0 || ino>=g.image_count) return BADPARAMETERS;
  
  int bpp1, bpp2, f, i, i1, i2, j, status=OK, val=0, 
      whitein, whiteout, blackin, blackout, ww_blackin, ww_whitein;

  double ff=0.0;
  int oldr=g.wantr, oldg=g.wantg, oldb=g.wantb;
  printstatus(BUSY);

  f = z[ino].cf;
  bpp1 = z[ino].bpp;
  bpp2 = g.bitsperpixel;
  ////  Activate all color planes temporarily
  g.wantr=1;
  g.wantg=1;
  g.wantb=1;

  ////  Assume data is actually a multiple of 8 bpp, or else 15 bpp.
  if(bpp1!=15)
    bpp1 = min(48,8*((7+bpp1)/8));  
  if(bpp2!=15)
    bpp2 = min(48,8*((7+bpp2)/8));  

  ////  Scale gray levels if it is a monochrome image  
  ////  If adjustgray is 0, user manually set gray mapping so don't change it.

  if(z[ino].colortype==GRAY)
  {  
      //// If image is an fft, calculate gray scaling using fft params.      
      //// Otherwise, if hitgray flag is 0,                                  
      //// determine the optimal gray scaling by sampling the pixel values.  
      if(z[ino].floatexists || (z[ino].waveletexists && z[ino].fftdisplay)) 
      {     ff = z[ino].fmax; 
            if(fabs(ff)<MAXINT) z[ino].gray[0] =(int)ff; else val=MAXINT*sgn(ff);
            ff = z[ino].fmin; 
            if(fabs(ff)<MAXINT) z[ino].gray[1] =(int)ff; else val=MAXINT*sgn(ff);
            z[ino].gray[2] = g.maxgray[bpp2];
            z[ino].gray[3] = 0;
            z[ino].hitgray=1;
            z[ino].gray[4] =(int)z[ino].wmax;
            z[ino].gray[5] =(int)z[ino].wmin;
      }else if(z[ino].hitgray==0)
      {     set_gray_levels(ino);
            z[ino].hitgray=1;
      }

      //// Scale the grayscale values to desired range. For 8 bit mode,`fac' 
      //// scales input to output value. For color modes,`fac' scales input  
      //// value to an r,g,and b value. In 16 bit mode, multiply g by 2.     
      
      if(z[ino].hitgray)
      {
            whitein    = z[ino].gray[0];   // It is necessary to convert these to
            blackin    = z[ino].gray[1];   // regular variables or it won't 
            whiteout   = z[ino].gray[2];   // divide properly.
            blackout   = z[ino].gray[3];   
            ww_whitein = z[ino].gray[4];   
            ww_blackin = z[ino].gray[5];   

            if(whitein!=blackin)
              z[ino].ffac = (double)(whiteout-blackout)/(double)(whitein-blackin);
            else
              z[ino].ffac = 1.0;

            if(ww_whitein!=ww_blackin)
              z[ino].wfac = (double)(whiteout-blackout)/(double)(ww_whitein-ww_blackin);
            else
              z[ino].wfac = 1.0;

            ////  Set colormap to grayscale or false-color palette
            setSVGApalette(1);
            memcpy(z[ino].palette, g.palette, 768);

            for(j=0;j<z[ino].ysize;j++)
            {  
                if(z[ino].is_zoomed && j >= z[ino].zoom_y2) break;
                for(i=0,i1=0,i2=0;i<z[ino].xsize;i++,i1+=g.off[bpp1],i2+=g.off[bpp2])
                {  
                    if(z[ino].is_zoomed && i >= z[ino].zoom_x2) break;
                    val = pixel_equivalent(ino, i, i1, j); 
                    val = false_color(val);
                    putpixelbytes(z[ino].img[j]+i2,val,1,bpp2); // Put value at destination
                }
            }
            if(g.bitsperpixel==8) repairimg(ino,0,0,z[ino].xsize,z[ino].ysize);
            printstatus(NORMAL);
      }
  }

  ////  Converting color images
  else if(!z[ino].is_zoomed)
  {
    if(bpp1==bpp2)                       // Copy w/o converting if same bpp
     {   
        for(j=0;j<z[ino].ysize;j++)
           memcpy(z[ino].img[j], z[ino].image[f][j], g.off[bpp1]*z[ino].xsize);
        if(z[ino].colortype==GRAY) if(bpp2==8) setSVGApalette(1);
     }else                                // Convert depth if different bpp's
     {                                    // Convert single frame
        change_depth(ino, 1, z[ino].colortype, z[ino].image[f], 
                 z[ino].img, bpp1, bpp2, 1, z[ino].palette);
     }
  }
  if(g.bitsperpixel==8) repairimg(ino,0,0,z[ino].xsize,z[ino].ysize);

  ////  Zoom
  if(z[ino].is_zoomed) redraw_zoom_image(ino);
  printstatus(NORMAL);

  g.wantr=oldr;
  g.wantg=oldg;
  g.wantb=oldb;
  return(status);
}        


//-------------------------------------------------------------------------//
// pixel_equivalent                                                        //
// read color or grayscale pixel, fft, or wavelet coeff.                   //
// i,j = x,y coordinate in buffer, ioffset = g.off[bpp] * x for image bufr.//
//-------------------------------------------------------------------------//
int pixel_equivalent(int ino, int i, int ioffset, int j)
{
  int val, f=z[ino].cf, bpp = z[ino].bpp;
  double ff=0.0;
  ////  Get monochrome pixel or whatever
  if((z[ino].floatexists && z[ino].fftdisplay!=NONE) || 
      z[ino].waveletexists && z[ino].waveletstate!=NONE) 
  {   switch(z[ino].fftdisplay)
      {   case REAL: ff = z[ino].fft[j][i].real(); break;
          case IMAG: ff = z[ino].fft[j][i].imag(); break;
          case POWR: ff = (z[ino].fft[j][i].real()*z[ino].fft[j][i].imag()); break;
          case WAVE: ff = z[ino].wavelet[j][i]; break;
      }               
  }else
      ff = (double)pixelat(z[ino].image[f][j] + ioffset, bpp);
  if(fabs(ff)<MAXINT) val=(int)ff; else val=MAXINT*sgn(ff);
  ////  Scale to desired value 
  if(z[ino].waveletstate &&
     z[ino].waveletexists && 
     z[ino].wavelettype==LAPLACIAN && 
     z[ino].fftdisplay==WAVE &&
     between(j, z[ino].wavelet_ylrstart, z[ino].wavelet_ylrstart+z[ino].wavelet_yminres-1) &&
     between(i, z[ino].wavelet_xlrstart, z[ino].wavelet_xlrstart + z[ino].wavelet_xminres-1))
        ff = (((val - z[ino].gray[5]) * z[ino].wfac) + z[ino].gray[3]);
  else
       ff = (((val - z[ino].gray[1]) * z[ino].ffac) + z[ino].gray[3]); 
  if(fabs(ff)<MAXINT) val=(int)ff; else val=MAXINT*sgn(ff);
  val = max(0, min((int)g.maxcolor, val));
  return(val);
}



//-------------------------------------------------------------------------//
// false_color - create a color value based on current false/grayscale     //
// colormap.  'val' should already be scaled appropriately to              //
// 0..g.maxgray[g.bitsperpixel] and false colormap should be in g.palette. //
// g.palette is r,g,b = 0..63 for each of 0..255.                          //
//-------------------------------------------------------------------------//
int false_color(int val)
{  
   int rr, gg, bb, bpp=g.bitsperpixel, rem;
   rr = gg = bb = val;
   switch(bpp)
   {   case  8: return val;  // Handled in setSVGApalette
       case 15: if(g.false_color)
                {   rr = g.palette[rr*8].red/2;
                    gg = g.palette[gg*8].green/2;
                    bb = g.palette[bb*8].blue/2;
                }
                break;
       case 16: if(g.false_color)
                {   rr = g.palette[rr*8].red/2;
                    gg = g.palette[gg*8].green/2;
                    bb = g.palette[bb*8].blue/2;
                }
                gg *=2;
                break;
       case 24: 
       case 32: 
       case 48: rem = val&3;     ////  Add 0..3 to interpolate between palette values
                if(g.false_color && val<256 && val>=0)
                {   rr = rem + 4*g.palette[rr].red;
                    gg = rem + 4*g.palette[gg].green;
                    bb = rem + 4*g.palette[bb].blue;
                }
                break;
       default: break;
   }
   rr = min(rr,g.maxred[bpp]);
   gg = min(gg,g.maxgreen[bpp]);
   bb = min(bb,g.maxblue[bpp]);
   return RGBvalue(rr,gg,bb,bpp);     
}


//-------------------------------------------------------------------------//
// change color depth                                                      //
//-------------------------------------------------------------------------//
int changecolordepth(int noofargs, char **arg)
{   
  static int ino;
  int bytesperpixel;
  ino = ci;
  if(g.getout){ g.getout=0; return ERROR; }
  bytesperpixel = z[ino].bpp / 8;
  if(noofargs>=1) 
  {   bytesperpixel = (6+atoi(arg[1]))/8;
      changecolordepth_part2(ino, bytesperpixel);
  }else  
      clickbox("New bytes/pixel", 4, &bytesperpixel, 1, 6, null, cset, 
                 null, &ino, NULL, 47);
  return OK;
}


//-------------------------------------------------------------------------//
// change color depth part 2                                               //
//-------------------------------------------------------------------------//
void changecolordepth_part2(int ino, int bytesperpixel)
{
  int bpp1;
  int bpp2 = bytesperpixel*8; 
  bpp2 = min(48, max(bpp2, 8));
  if(!between(bpp2, 8, 48)) return;
  if(!between(ino, 1, g.image_count-1)){ message("Select an image first"); return; }
  if(ino > 0)
  {   bpp1 = z[ino].bpp;
      if(bpp2 == 0) bpp2=bpp1;
      g.getout = 0;
      if(bpp1 != bpp2)
      {   switch_palette(ino);
          setpalette(z[ino].palette);
          //// Prevent image turning black if they down-convert a 10 or
          //// 12-bit grayscale
          if(bpp2<bpp1 && z[ino].colortype == GRAY) 
              maximize_contrast(0, (char**)NULL);
          if(g.getout == 0) change_image_depth(ino, bpp2, PERM);
          z[ino].touched = 1;
          repair(ino);
      }
  }else message("No images selected", ERROR);
}


//-------------------------------------------------------------------------//
// change_depth                                                            //
// Convert image frame from its current bits/pixel to bpp2 using data in   //
//   buf1 and put the result in buf2.  Rescales pixel values for grayscale //
//   images and rescales separate RGB values for color images.             //
// Assumes output buffer buf2 is already allocated to the right size.      //
// Buf2 can be same as buf1 if bpp2 >= bpp1.                               //
// Set want_palette to 1 for normal image conversions, 0 if converting     //
//   a backup or some other buffer not involved in an image.               //
// 2 forms for converting a single frame or an entire image. No. of frames,//
//   and size of buf1 and buf2 must be the same.                           //
//-------------------------------------------------------------------------//
int change_depth(int ino, int frames, int colortype, uchar **buf1, uchar **buf2, 
    int bpp1, int bpp2, int want_palette, RGB palette[256])
{
   frames = frames;  
   return change_depth(ino, 1, colortype, &buf1, &buf2, bpp1, bpp2, want_palette, palette);
}
int change_depth(int ino, int frames, int colortype, uchar ***buf1, uchar ***buf2, 
    int bpp1, int bpp2, int want_palette, RGB palette[256])
{ 
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); } 
   if(ino<0 || ino>=g.image_count){ message("Error changing image depth",ERROR); return(ERROR);}
   int f,i,j,i1,i2,status=OK, rr, gg, bb, value, 
       rshift=0, gshift=0, bshift=0, ishift=0, rshift2=0, gshift2=0, bshift2=0,
       ishift2=0, size=0;
   if(bpp1==bpp2) return OK;
   if(!between(ino, 0, g.image_count-1)) return BADPARAMETERS;
   if(frames<=0){ fprintf(stderr, "Frames=%d in __FILE__\n",frames); return BADPARAMETERS; }
   uchar *buf;
   size = max(g.off[bpp1], g.off[bpp2]) * z[ino].xsize + 10;
   buf = new uchar[size];
   if(buf==NULL){  message(g.nomemory,ERROR); printstatus(NORMAL); return(NOMEM); }
   calculate_shifts(bpp1, rshift, gshift, bshift, ishift);
   calculate_shifts(bpp2, rshift2, gshift2, bshift2, ishift2);
   if(colortype==GRAY)
   {    
       for(f=0; f<frames; f++)
       for(j=0; j<z[ino].ysize; j++)
       {    for(i=0,i1=0,i2=0; i<z[ino].xsize; i++,i1+=g.off[bpp1],i2+=g.off[bpp2])
            {   
                 value = pixelat(buf1[f][j]+i1, bpp1);        // Get monochrome pixel
                 value = (value << ishift) >> ishift2; 
                 putpixelbytes(buf+i2, value, 1, bpp2);       // Put value in bufr
            }
            memcpy(buf2[f][j], buf, z[ino].xsize*g.off[bpp2]);
       }
   }else
   {    
       if(bpp1>8 && bpp2==8)            // Down convert to 8 bpp
       {                                // Fit to current palette if down-converting
                                        // a multi-frame image
           if(g.want_quantize==2 || z[ino].frames>1)
              status = fitpalette(bpp1, buf1, buf2, palette, z[ino].xsize, 
                       z[ino].ysize, frames, z[ino].ncolors);  
           else if(g.want_quantize==1)        
              status = wuquantize(bpp1, buf1, buf2, z[ino].xsize, z[ino].ysize,
                       frames, z[ino].ncolors);  
                                        // Attach the new, quantized palette 
                                        // to the image.
           memcpy(palette, g.palette, 768);  
       }else if(bpp1<=32)               // Up or down convert true/high color image
       {                                // Use the image's palette
           if(want_palette) switch_palette(ino);
           for(f=0; f<frames; f++)
           for(j=0; j<z[ino].ysize; j++)
           {  i1=0;i2=0;
              for(i=0,i1=0,i2=0; i<z[ino].xsize; i++,i1+=g.off[bpp1],i2+=g.off[bpp2])
              {   
                  RGBat(buf1[f][j]+i1, bpp1, rr, gg, bb);
                  rr = (rr<<rshift)>>rshift2;  // Do in 2 steps to avoid 
                  gg = (gg<<gshift)>>gshift2;  //   negative shifts
                  bb = (bb<<bshift)>>bshift2; 
                  putRGBbytes(buf+i2, rr, gg, bb, bpp2);
              }
              memcpy(buf2[f][j], buf, z[ino].xsize*g.off[bpp2]);
           }
       }else                            // 48 bpp 
       {                                // Use the image's palette
           if(want_palette) switch_palette(ino);
           for(f=0; f<frames; f++)
           for(j=0; j<z[ino].ysize; j++)
           {  i1=0;i2=0;
              for(i=0,i1=0,i2=0; i<z[ino].xsize; i++,i1+=g.off[bpp1],i2+=g.off[bpp2])
              {   
                  RGBat(buf1[f][j]+i1, bpp1, rr, gg, bb); 
                  rr = (rr<<rshift)>>rshift2;  // Do in 2 steps to avoid 
                  gg = (gg<<gshift)>>gshift2;  //   negative shifts
                  bb = (bb<<bshift)>>bshift2; 
                  if(bpp2>=48)
                  {   putRGBbytes(buf+i2, rr, gg, bb, bpp2);
                  }else
                  {   value = RGBvalue(rr,gg,bb,bpp2);
                      putpixelbytes(buf+i2, value, 1, bpp2, 1);  
                  }
              }
              memcpy(buf2[f][j], buf, z[ino].xsize*g.off[bpp2]);
           }           
       }

   }
   delete[] buf;
   return status;
}


//-------------------------------------------------------------------------//
// calculate_shifts                                                        //
// Calculate factor to convert r,g,b,i of pixel to 0..2^24 or back         //
//-------------------------------------------------------------------------//
void calculate_shifts(int bpp, int &rr, int &gg, int &bb, int &ii)
{
   switch(bpp)   
   {   case  8:  rr = 18; gg = 18; bb = 18; ii = 16; break; // 6 bits/color
       case 15:  rr = 19; gg = 19; bb = 19; ii =  9; break; // 5 bits/color
       case 16:  rr = 19; gg = 18; bb = 19; ii =  8; break; // 5 or 6 bits/color
       case 24:  rr = 16; gg = 16; bb = 16; ii =  0; break; // 8 bits/color
       case 32:  rr = 16; gg = 16; bb = 16; ii =  0; break; // 8 bits/color
       case 48:  rr =  8; gg =  8; bb =  8; ii =  0; break; // 16 bits/color
       default: message("Error calculating shifts", BUG);
   }
}



//-------------------------------------------------------------------------//
// converttograyscale                                                      //
// Convert image[ino] from color to gray scale.                            //
//-------------------------------------------------------------------------//
void converttograyscale(int ino)
{ 
  int i,i2,j;
  if(z[ino].colortype == GRAY) return;
  uint value;
  if(ino<0){ message("Select an image to convert"); return; }
  int bpp=z[ino].bpp;
  printstatus(BUSY);
  drawselectbox(OFF);

  int cf = z[ino].cf;
  for(j=0;j<z[ino].ysize;j++)
  for(i=0,i2=0; i<z[ino].xsize; i++,i2+=g.off[bpp])
  {  
       value=pixelat(z[ino].image[cf][j]+i2,bpp);
       value=graypixel(value,bpp);
       putpixelbytes(z[ino].image[cf][j]+i2,(uint)value,1,bpp);
  }
  setSVGApalette(1);
  memcpy(z[ino].palette, g.palette, 768);
  memcpy(z[ino].opalette, g.palette, 768);
  setpalette(z[ino].palette);
  z[ino].colortype=GRAY;
  z[ino].touched=1;
  z[ino].hitgray=0;
  rebuild_display(ino);
  switch_palette(ino); 
  redraw(ino);
  printstatus(NORMAL);
}  


//-------------------------------------------------------------------------//
// converttocolor                                                          //
//-------------------------------------------------------------------------//
void converttocolor(int ino)
{
  if(ino<0) return;
  int cf,i,j,i2,bpp,value,rr,gg,bb;
  double fac = 1.0;
  if(z[ino].bpp==16)
  {    message("This operation will result in poor image quality.\nRecommend converting image to 24 bits first",WARNING,56);
       if(g.getout){ g.getout=0; return; }
  }
  printstatus(BUSY);
  bpp=z[ino].bpp;
  cf = z[ino].cf;
  switchto(ino);
  z[ino].hitgray=0;       
  if(z[ino].bpp==8) 
  {    z[ino].colortype=INDEXED; 
       memcpy(z[ino].palette,z[ino].opalette,768);  
       setpalette(z[ino].palette);
  }else if(z[ino].colortype==GRAY)
  {    z[ino].colortype=COLOR;
       fac = (double)g.maxgray[bpp] / (double)(z[ino].gray[0] - z[ino].gray[1]);
       for(j=0;j<z[ino].ysize;j++)
       for(i=0,i2=0; i<z[ino].xsize; i++,i2+=g.off[bpp])
       {  
           value=pixelat(z[ino].image[cf][j]+i2,bpp);
           value = cint((value - z[ino].gray[1]) * fac);
           rr=gg=bb=value;
           if(z[ino].bpp==16) gg+=value;
           rr = max(0, min(rr, g.maxred[bpp]));
           gg = max(0, min(gg, g.maxgreen[bpp]));
           bb = max(0, min(bb, g.maxblue[bpp]));
           value = RGBvalue(rr,gg,bb,bpp);
           putpixelbytes(z[ino].image[cf][j]+i2,(uint)value,1,bpp);
       }
  }else
       z[ino].colortype=COLOR;
     
  repair(ino);
  printstatus(NORMAL);
}



//-------------------------------------------------------------------------//
// RGBtoint - convert r,g,and b to 30 bit integers so they can be          //
//            manipulated independently of their bpp depth.                //
//            The input values are passed by ref. and get changed.         //
//-------------------------------------------------------------------------//
void RGBtoint(int &rr, int &gg, int &bb, int bpp)
{
   int drbpp,dgbpp,dbbpp;
   int rr2=rr,gg2=gg,bb2=bb;
   drbpp = 30-g.redbpp[bpp];              // Shift r,g,b to 0..2^^32
   if(drbpp>0) rr2 = rr << ( drbpp);        // (30 bits for max. precision)
   if(drbpp<0) rr2 = rr >> (-drbpp);
   dgbpp = 30-g.greenbpp[bpp];
   if(dgbpp>0) gg2 = gg << ( dgbpp);
   if(dgbpp<0) gg2 = gg >> (-dgbpp);
   dbbpp = 30-g.bluebpp[bpp];
   if(dbbpp>0) bb2 = bb << ( dbbpp);
   if(dbbpp<0) bb2 = bb >> (-dbbpp);
   rr=rr2; gg=gg2; bb=bb2;
}


//-------------------------------------------------------------------------//
// inttoRGB - convert 30 bit integers back to r,g,b values                 //
//            at the specified bpp depth. The input values are             //
//           passed by reference and get changed.                          //
//-------------------------------------------------------------------------//
void inttoRGB(int &rr, int &gg, int &bb, int bpp)
{
   int drbpp,dgbpp,dbbpp,rr2=rr,gg2=gg,bb2=bb;
   drbpp = 30-g.redbpp[bpp];              // Shift r,g,b to 0..2^^32
   dgbpp = 30-g.greenbpp[bpp];
   dbbpp = 30-g.bluebpp[bpp];
   if(drbpp>0) rr2 = rr >> ( drbpp);      // Use copies in case someone passes
   if(drbpp<0) rr2 = rr << (-drbpp);      // the same variable twice
   if(dgbpp>0) gg2 = gg >> ( dgbpp);
   if(dgbpp<0) gg2 = gg << (-dgbpp);
   if(dbbpp>0) bb2 = bb >> ( dbbpp);
   if(dbbpp<0) bb2 = bb << (-dbbpp);
   rr=rr2; gg=gg2; bb=bb2;
}
 

//-------------------------------------------------------------------------//
// zoom                                                                    //
//-------------------------------------------------------------------------//
void zoom(int noofargs, char **arg, int mode)
{
  int bpp,ino=0,x,y,button;
  double zfac=1.0;
  if(noofargs >=1){ ino=atoi(arg[1]); if(ino<0) return; }
  if(noofargs >=2) zfac=atof(arg[2]);
  if(g.zoomfac<1.0) g.zoomfac = 1.1; 
  g.getout=0;
  drawselectbox(OFF);
  if(noofargs < 2)
  {    g.zooming = 1;
       while(g.getout==0 && g.zooming)
       {   g.state = ZOOMING+mode-1;
           getpoint(x,y);
           ino = whichimg(x,y,bpp);
           if(ino<0) return;
           if(g.getout) {g.getout=0; break;}
           button = g.last_mouse_button;      
           switch(button)
           {   case 1: zfac = g.zoomfac; break;
               case 2: unzoom(ino); return;
               case 3: zfac = 1/g.zoomfac; break;  
           }
           zoom_image(ino, zfac, x, y, mode);
       }  
      // message("Zooming mode off"); 
       g.zooming = 0;
       g.getout = 0;
       g.state = NORMAL;             
  }else zoom_image(ino, zfac, 0, 0, mode);
}


//-------------------------------------------------------------------------//
// zoom_image                                                              //
//-------------------------------------------------------------------------//
void zoom_image(int ino, double zfac, int x, int y, int mode)
{
  int x1,y1,x2,y2,dx=0,dy=0;
  int ox1, oy1;
  int xsize, ysize;
  double zoomx, zoomy;
  if(ino<=0) return;
  xsize = min(z[ino].xsize, g.main_xsize);
  ysize = min(z[ino].ysize, g.main_ysize);
  zoomx = z[ino].zoomx;
  zoomy = z[ino].zoomy;
  ox1 = z[ino].zoom_x1;
  oy1 = z[ino].zoom_y1;
  x1 = y1 = 0;
  x2 = cint(z[ino].xsize*z[ino].zoomx*zfac);
  y2 = cint(z[ino].ysize*z[ino].zoomy*zfac);
  z[ino].zoomx *= zfac;
  z[ino].zoomy *= zfac;

  if(zfac>=0.0 && zfac<1.0) 
  {   z[ino].zoomx_inv = 1.0/z[ino].zoomx;
      z[ino].zoomy_inv = 1.0/z[ino].zoomy;
  }else
  {   z[ino].zoomx_inv = 1.0/z[ino].zoomx;
      z[ino].zoomy_inv = 1.0/z[ino].zoomy;
  }

  z[ino].zoom_x1 = x1;
  z[ino].zoom_y1 = y1;
  z[ino].zoom_x2 = x2;
  z[ino].zoom_y2 = y2;

  g.selected_ulx = x1 + z[ino].xpos;
  g.selected_uly = y1 + z[ino].ypos;
  g.selected_lrx = x2 + z[ino].xpos - 1;
  g.selected_lry = y2 + z[ino].ypos - 1;

  if(z[ino].is_zoomed && z[ino].is_zoomed != mode)
      resize_img(ino, z[ino].xsize, z[ino].ysize); 
  
  if(between(z[ino].zoomx, 0.99, 1.01) &&
     between(z[ino].zoomy, 0.99, 1.01))
     z[ino].is_zoomed = 0;
  else
     z[ino].is_zoomed = mode;
  g.ulx = zoom_x_coordinate(g.selected_ulx, ino);
  g.uly = zoom_y_coordinate(g.selected_uly, ino);
  g.lrx = zoom_x_coordinate(g.selected_lrx, ino);
  g.lry = zoom_y_coordinate(g.selected_lry, ino); 

  ////  ZOOM = enlarge/shrink window and image
  ////  MAGNIFY = enlarge/shrink image in constant window

  if(mode == ZOOM)
  {   z[ino].xscreensize = int(z[ino].xsize * z[ino].zoomx);
      z[ino].yscreensize = int(z[ino].ysize * z[ino].zoomy);
      resize_img(ino, x2, y2);
  }else
  {   z[ino].xscreensize = z[ino].xsize;
      z[ino].yscreensize = z[ino].ysize;
      dx = x - cint((double)xsize * z[ino].zoomx_inv / 2);
      dy = y - cint((double)ysize * z[ino].zoomy_inv / 2);
      dx -= z[ino].xpos;
      dy -= z[ino].ypos;
      dx = max(0,dx);
      dy = max(0,dy);
      z[ino].zoom_x1 = dx;
      z[ino].zoom_y1 = dy;
      z[ino].zoom_x2 = z[ino].xsize;
      z[ino].zoom_y2 = z[ino].ysize;
  }
  rebuild_display(ino);
  if(z[ino].s->visible) putimageinspreadsheet(ino);   
  redraw(ino);
}


//-------------------------------------------------------------------------//
// unzoom                                                                  //
//-------------------------------------------------------------------------//
void unzoom(int ino)
{
   int hit=0;
   if(ino<=0) return;
   if(z[ino].is_zoomed) 
   {   hit=1;
       resize_img(ino, z[ino].xsize, z[ino].ysize);
   }
   z[ino].is_zoomed = 0;
   z[ino].zoomx = 1.0;
   z[ino].zoomy = 1.0;
   z[ino].zoomx_inv = 1.0;
   z[ino].zoomy_inv = 1.0;
   z[ino].zoom_x1 = 0;
   z[ino].zoom_x2 = 0;
   z[ino].zoom_y1 = 0;
   z[ino].zoom_y2 = 0;
   z[ino].xscreensize = z[ino].xsize;
   z[ino].yscreensize = z[ino].ysize;
   if(hit)
   {   selectimage(ino);
       repair(ino);
   }
}


//-------------------------------------------------------------------------//
// redraw_zoom_image                                                       //
//-------------------------------------------------------------------------//
void redraw_zoom_image(int ino)
{
   int size, bpp, wantcolor, bytesperpixel;
   double zoomx, zoomy, zoom1=1.0, zoom2=1.0;
   int f,i,i2,j,x1,y1,x2,y2,xsize,ysize,ogray0=0,ogray1=0;
   if(ino<=0) return;
   if(!z[ino].is_zoomed){ fprintf(stderr, "Error in redraw_zoom_image\n"); return; }
   uint v;
   uchar **zoombuf;
   uchar *zoombuf_1d;

   if(z[ino].colortype==GRAY) wantcolor=0; else wantcolor=1;
   f = z[ino].cf;
   bpp = z[ino].bpp;
   size = z[ino].xsize * z[ino].ysize * g.bitsperpixel;
   bytesperpixel = (7+g.bitsperpixel)/8;
   if(memorylessthan(size)){  message(g.nomemory,ERROR); return; } 
   if((zoombuf_1d = new uchar[size])==NULL){ message(g.nomemory, ERROR); return; }   

   zoombuf = make_2d_alias(zoombuf_1d, z[ino].xsize*bytesperpixel, z[ino].ysize);
   zoomx = z[ino].zoomx;
   zoomy = z[ino].zoomy;

   if(zoomx != 0.0) zoom1 = 1.0 / zoomx;
   if(zoomy != 0.0) zoom2 = 1.0 / zoomy;
   xsize = z[ino].xsize;
   ysize = z[ino].ysize;
   x1 = z[ino].zoom_x1;
   y1 = z[ino].zoom_y1;
   x2 = z[ino].xscreensize;
   y2 = z[ino].yscreensize;

   //// Convert zoombuf to g.bitsperpixel if necessary

   if(bpp==g.bitsperpixel)
   {   for(j=0; j<z[ino].ysize; j++)
           memcpy(zoombuf[j], z[ino].image[f][j], g.off[bpp]*z[ino].xsize);
       if(z[ino].colortype==GRAY) if(g.bitsperpixel==8) setSVGApalette(1);
   }else
   {   change_depth(ino, 1, z[ino].colortype, z[ino].image[f], 
          zoombuf, bpp, g.bitsperpixel, 1, z[ino].palette);
   }

   //// Scale grayscale mapping temporarily in case image is gray
   if(z[ino].colortype==GRAY)
   {
       ogray0 = z[ino].gray[0]; 
       ogray1 = z[ino].gray[1]; 
       z[ino].gray[0] = cint((double)z[ino].gray[0] * g.maxvalue[g.bitsperpixel] / g.maxvalue[bpp]); 
       z[ino].gray[1] = cint((double)z[ino].gray[1] * g.maxvalue[g.bitsperpixel] / g.maxvalue[bpp]);   
   }


   for(j=y1; j<y2; j++)
   {  for(i=x1,i2=0; i<x2; i++,i2+=g.off[g.bitsperpixel])
      {    v = pixelat(zoombuf[int(j*zoom2)] + 
                       int(i*zoom1)*g.off[g.bitsperpixel], 
                       g.bitsperpixel);
           if(z[ino].colortype==GRAY) v = grayvalue(v, ino, g.bitsperpixel);
           putpixelbytes(z[ino].img[j]+i2, v, 1, g.bitsperpixel, 1);
      }
   }

   if(z[ino].colortype==GRAY)
   {
      z[ino].gray[0] = ogray0;
      z[ino].gray[1] = ogray1;
   }
   free(zoombuf);
   delete[] zoombuf_1d;
}


//-------------------------------------------------------------------------//
// clear_alpha                                                             //
//-------------------------------------------------------------------------//
void clear_alpha(int ino)
{
   int bpp,i,ii,j,v,f;
   if(ino<0){ message("Please select an image"); return; }
   if(!z[ino].backedup){ message("Image was not backed up"); return; }
   
   f = z[ino].cf;
   bpp = z[ino].bpp;
   for(j=0;j<z[ino].ysize;j++)
   for(i=0, ii=0; i<z[ino].xsize; i++, ii+=g.off[bpp])
       if(get_alpha_bit(ino,0,i,j))
       {    v = pixelat(z[ino].backup[f][j]+ii,bpp);  
            putpixelbytes(z[ino].image[f][j]+ii,v,1,bpp,1);
            set_alpha_bit(ino,0,i,j,0);
       } 
    repair(ino);   
}


//-------------------------------------------------------------------------//
// show_alpha                                                              //
//-------------------------------------------------------------------------//
void show_alpha(int ino)
{
   int i,j;
   check_event();
   if(ino>=0)
   {   for(j=0;j<z[ino].ysize;j++)
       {   if(keyhit()) if(getcharacter()==27) break;
           for(i=0;i<z[ino].xsize;i++)
           {   if(get_alpha_bit(ino,0,i,j)) 
                 setpixel(i+z[ino].xpos,j+z[ino].ypos,g.maxcolor,SET);
               else setpixel(i+z[ino].xpos,j+z[ino].ypos,0,SET);
           }
       }
   }else message("Error: bad image number");
   sleep(2);
   repair(ino);
}


//-------------------------------------------------------------------------//
// set_gray_levels - calculate grayscale map for image                     //
//-------------------------------------------------------------------------//
void set_gray_levels(int ino)
{
    int i,j,i1,f,minval=1000000,maxval=0,val;
    int bpp1 = z[ino].bpp;
    int bpp2 = g.bitsperpixel;
    if(!z[ino].adjustgray) return;
    f = z[ino].cf;
    if(bpp1==8 && bpp2==8)
    {   z[ino].gray[0] = 255; 
        z[ino].gray[1] = 0;        
        z[ino].gray[2] = 255;
        z[ino].gray[3] = 0;
    }else
    {   if(bpp1==8)
        {    z[ino].gray[0] = 255; 
             z[ino].gray[1] = 0;        
        }else
        {   for(j=0;j<z[ino].ysize;j+=5)
            for(i=0,i1=0; i<z[ino].xsize; i+=4,i1+=g.off[bpp1]) // every 4th pixel
            {    val = pixelat(z[ino].image[f][j]+i1,bpp1);
                 if(val>maxval) maxval=val;
                 if(val<minval) minval=val;
            }
            z[ino].gray[0] = maxval; 
            z[ino].gray[1] = minval;        
        }
        z[ino].gray[2] = g.maxgray[bpp2];
        z[ino].gray[3] = 0;
    }
}
