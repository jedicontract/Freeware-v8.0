//--------------------------------------------------------------------------//
// xmtnimage40.cc                                                           //
// PCX file reading, writing                                                //
// Latest revision: 05-28-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//
 
#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;


//--------------------------------------------------------------------------//
// readpcxfile                                                              //
// Transfer PCX  file from disk to screen.                                  //
// Assumes 32-bit integers and 16-bit short integers.                       //
// Only reads image vertically.  1,2,4, and 8-bpp PCX only.                 //
//--------------------------------------------------------------------------//
int readpcxfile(char *filename)
{  
   FILE *fp;
   uchar crudbuf[9], *buffer;
   int colortype, i, j, junk, k, k1, kxfac, kyfac, noofcolors=1, 
       oldy2=0, paletteatend = 0, paletteindicator=0, status=OK, 
       x, x2, xsize, y2, ysize;
   char pcx_id        =0;
   char pcx_version   =0;
   char pcx_encoding  =0;
   char pcx_bitsperpixel =0;
   char pcx_bitplanes    =0;
   short int pcx_xleft   =0;
   short int pcx_ytop    =0;
   short int pcx_xright  =0;
   short int pcx_ybottom =0;
   short int test;
   short int pcx_bitsperscanline = 0;
   double xfac, yfac=1;
   uchar count = 0;        /* run-length count, 0-255   */
   uchar color = 0;        /* pixel value */      
   uchar nextbyte=0;
   uchar value;

   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       status=NOTFOUND;
       return(status);
   }

   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   xfac=g.tif_xsize;
   kxfac=(int)(xfac*64);    // integer to speed up multiplication
                            // so  xfac*k  ==  ((kxfac*k)>>6).

   yfac=g.tif_ysize;        // yfac=size in y dimension, 0 to 1, 1=full size
   kyfac=(int)(yfac*64);    // integer to speed up multiplication
                            // so  yfac*k  ==  ((kyfac*k)>>6).
                       
   pcx_id = getbyte(fp); 
   pcx_version = getbyte(fp);
   pcx_encoding = getbyte(fp);
   pcx_bitsperpixel = getbyte(fp);
   pcx_xleft = getword(fp);       // x screen position
   pcx_ytop = getword(fp);        // y screen position
   pcx_xright = getword(fp);
   pcx_ybottom = getword(fp);
   getword(fp);                   // hres dpi   
   getword(fp);                   // yres dpi   
   for(k=16;k<=63;k++) getbyte(fp);
   getbyte(fp);                   // byte 64 should be 0 
   pcx_bitplanes = getbyte(fp);
   pcx_bitsperscanline = getword(fp);
   getword(fp);                   // paletteinfo
   for(k=70;k<=127;k++) getbyte(fp);
  
   //// Check validity of PCX parameters
   
   if(pcx_id != 0x0a ||
      pcx_version >5 ||
      pcx_encoding != 1 ||
      pcx_bitsperpixel >8 ||
      pcx_bitplanes > 4 )
      {  message("Invalid PCX file",ERROR); return ERROR; }

   xsize = pcx_xright - pcx_xleft;
   ysize = pcx_ybottom - pcx_ytop;
   noofcolors = pcx_bitsperpixel/8;
   if(noofcolors<=8) colortype=INDEXED; else colortype=COLOR;

   buffer = new uchar[g.off[g.bitsperpixel]*(xsize+4)];  //check if null later
   if(buffer==NULL) return(NOMEM);
   
   if(newimage(basefilename(filename),pcx_xleft+g.tif_xoffset, pcx_ytop+g.tif_yoffset, 
               (int)(0.99999 + xsize*g.tif_xsize), 
               (int)(0.99999 + ysize*g.tif_ysize), 8, colortype, 1, 
               g.want_shell,1,PERM,1,g.window_border,0) != OK) 
   { delete[] buffer; return(NOMEM); }

   z[ci].originalbpp=pcx_bitsperpixel;
                                    // Figure out what palette  
   if(pcx_bitsperpixel==8) paletteatend=1;
   if(pcx_bitsperpixel==2) paletteatend=1;
   if((pcx_bitsperpixel==4)&&(pcx_version>=5)) paletteatend=1;
   if(pcx_bitplanes>1) paletteatend = 0;
   noofcolors = 1<<(pcx_bitsperpixel * pcx_bitplanes);
             /* two exceptions to above rule */
   if((pcx_bitsperpixel==2)&&(pcx_bitplanes==4)) noofcolors=16;          
   if((pcx_bitsperpixel==8)&&(pcx_bitplanes==3)) noofcolors=16777216;          
 
   ////  Read the palette - this must be done after newimage().

   if(paletteatend)
   {   fseek(fp,-769,SEEK_END);         /* palette at end of file */
       paletteindicator = getbyte(fp);
       if(paletteindicator==12)
       {  for (i=0; i<=255; i++)
          {    color = getbyte(fp);
               g.palette[i].red    = color>>2;
               color = getbyte(fp);
               g.palette[i].green  = color>>2;
               color = getbyte(fp);
               g.palette[i].blue   = color>>2;
          }    
          setpalette(g.palette);
       }
    }   
    else if((pcx_bitplanes==4)&&(noofcolors<=16))
    {  fseek(fp,g.tif_skipbytes+16,SEEK_SET);          /* header palette */
       for (i=0; i<16; i++)
          {    j=i*16;
               color = getbyte(fp);
               g.palette[j].red    = color>>2;
               color = getbyte(fp);
               g.palette[j].green  = color>>2;
               color = getbyte(fp);
               g.palette[j].blue   = color>>2;
#ifdef MSDOS
               reg.x.ax = 0x1010;
               reg.x.bx = j;
               reg.h.ch = g.palette[j].green;
               reg.h.cl = g.palette[j].blue;
               reg.h.dh = g.palette[j].red;  
               int86(0x10,&reg,&reg);  
#endif
          }    
    }

   ////  Read the PCX image    

   fseek(fp,g.tif_skipbytes+128,SEEK_SET);

   if((pcx_bitplanes==1)&&(pcx_bitsperpixel==8))
   {   for(i=0;i<ysize;i++)
       {   if(keyhit()) if (getcharacter()==27)break;
           x=0;
           count=0;
           while(x<=xsize)  
           {
               nextbyte = getbyte(fp);
               y2 = (kyfac*i)>>6;                
               if(nextbyte >= 0xc0)             
               {  count = nextbyte & 0x3F;   
                  color = getbyte(fp);
                  if(!g.tif_positive)color=255-color;
                  for(j=0;j<count;j++)
                    if((xfac!=1) || (yfac!=1))
                       z[ci].image[z[ci].cf][y2][(kxfac*x++)>>6] = color;
                    else
                       z[ci].image[z[ci].cf][i][x++] = color;                             
               }
               else
               {  if(!g.tif_positive)nextbyte=255-nextbyte;
                  if((xfac!=1) || (yfac!=1))
                     z[ci].image[z[ci].cf][y2][(kxfac*x++)>>6] = nextbyte;
                  else    
                     z[ci].image[z[ci].cf][i][x++] = nextbyte;
               }      
        
            }
       }  
   } else if((pcx_bitsperpixel==1))  /* VGA or monochrome PCX file */
   {   for(i=0;i<ysize;i++)
       {   if(keyhit()) if (getcharacter()==27)break;
           x=0;
           test=pcx_bitsperscanline*pcx_bitplanes;
           while(x<test)  
           {   nextbyte = getbyte(fp);
               if(nextbyte >= 0xc0)             
               {  count = nextbyte & 0x3F;   
                  color = getbyte(fp);
                  for(j=0;j<count;j++) buffer[x++]=color;
               }
               else
                  buffer[x++]=nextbyte;
           }
           y2 = ((kyfac*i)>>6);
           if(y2!=oldy2)
           {
             for(k1=0;k1<pcx_bitsperscanline;k1++)
             {  
                 for(k=0;k<8;k++) crudbuf[k]=0;
                 for(k=0;k<pcx_bitplanes;k++)
                 {  value=1<<k;
                    color=buffer[k1+k*pcx_bitsperscanline];
                    if(color&0x80) crudbuf[0]+=value;
                    if(color&0x40) crudbuf[1]+=value;
                    if(color&0x20) crudbuf[2]+=value;
                    if(color&0x10) crudbuf[3]+=value; 
                    if(color&0x08) crudbuf[4]+=value; 
                    if(color&0x04) crudbuf[5]+=value; 
                    if(color&0x02) crudbuf[6]+=value; 
                    if(color&0x01) crudbuf[7]+=value;                 
                 }
                 if(pcx_bitplanes==4) 
                 {   for(k=0;k<8;k++)
                     {    crudbuf[k]<<=4;
                          if(!g.tif_positive) crudbuf[k]=255-crudbuf[k];
                     }     
                 }
                 if(pcx_bitplanes==1) 
                 {   for(k=0;k<8;k++)
                     {    if(crudbuf[k]) crudbuf[k]=255;
                          if(!g.tif_positive) crudbuf[k]=255-crudbuf[k];
                     }     
                 }
             
                 if((xfac!=1) || (yfac!=1))
                     for(k=0;k<8;k++)
                       z[ci].image[z[ci].cf][y2][(kxfac*((k1<<3)+k))>>6] = crudbuf[k];
                 else
                 {   x2 = k1<<3;
                     for(k=0;k<8;k++)
                        z[ci].image[z[ci].cf][i][x2+k] = crudbuf[k];               
                 }
             } 
           }
           oldy2=y2;
       }       
    } else 
    {  message("File is an unsupported type of PCX file.");
       status=ERROR;
    }
    fclose(fp); 
    delete[] buffer;
    return(status);
}


//--------------------------------------------------------------------------//
// aboutpcxfile                                                             //
// Prints information about PCX file on screen.                             //
//--------------------------------------------------------------------------//
void aboutpcxfile(char *filename)
{  
   static listinfo *l;
   static char *listtitle;
   listtitle = new char[100];
   strcpy(listtitle, "About the File");
   static char **info;                                    
   FILE *fp;
   int k;
   int crud;
   int pcx_id        =0;
   int pcx_version   =0;
   int pcx_encoding  =0;
   int pcx_bitsperpixel =0;
   int pcx_xleft   =0;
   int pcx_ytop    =0;
   int pcx_xright  =0;
   int pcx_ybottom =0;
   int pcx_hres    =0;
   int pcx_vres    =0;
   int pcx_bitplanes    =0;
   int pcx_bitsperscanline = 0;
   int pcx_paletteinfo = 0;
   int paletteatend = 0;
   int noofcolors=0;
   int junk;

   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return;
   }

   //-------------Put information in list box-------------------------//

   int lc=-1;
   info = new char*[100];   // max. 100 lines - Change if you add more items
   l = new listinfo;

   info[++lc] = new char[100];                       
   sprintf(info[lc],"Filename:  %s",basefilename(filename));

   //-------program name-------------------------//

   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   pcx_id = getbyte(fp);
   pcx_version = getbyte(fp);
   pcx_encoding = getbyte(fp);
   pcx_bitsperpixel = getbyte(fp);
   pcx_xleft = getword(fp);
   pcx_ytop = getword(fp);
   pcx_xright = getword(fp);
   pcx_ybottom = getword(fp);
   pcx_hres = getword(fp);   // dpi 
   pcx_vres = getword(fp);   // dpi 
   for(k=16;k<=63;k++) crud = getbyte(fp);
   crud = getbyte(fp);       // byte 64 should be 0 
   pcx_bitplanes = getbyte(fp);
   pcx_bitsperscanline = getword(fp);
   pcx_paletteinfo = getword(fp);
   for(k=70;k<=127;k++) crud = getbyte(fp);
  
   if(pcx_bitsperpixel==8) paletteatend=1;
   if(pcx_bitsperpixel==2) paletteatend=1;
   if((pcx_bitsperpixel==4)&&(pcx_version>=5)) paletteatend=1;
   if(pcx_bitplanes>1) paletteatend = 0;
   noofcolors =1<<(pcx_bitsperpixel * pcx_bitplanes);
             // two exceptions to above rule 
   if((pcx_bitsperpixel==2)&&(pcx_bitplanes==4)) noofcolors=16;          
   if((pcx_bitsperpixel==8)&&(pcx_bitplanes==3)) noofcolors=16777216;          


   info[++lc]=new char[100]; sprintf(info[lc],"File format: PCX");
   info[++lc]=new char[100]; sprintf(info[lc],"PCX ID       %d",pcx_id);
   info[++lc]=new char[100]; sprintf(info[lc],"pcx version  %d",pcx_version);
   info[++lc]=new char[100]; sprintf(info[lc],"pcx encoding %d",pcx_encoding);
   info[++lc]=new char[100]; sprintf(info[lc],"bits/pixel   %d",pcx_bitsperpixel);
   info[++lc]=new char[100]; sprintf(info[lc],"x left       %d",pcx_xleft);
   info[++lc]=new char[100]; sprintf(info[lc],"y top        %d",pcx_ytop);
   info[++lc]=new char[100]; sprintf(info[lc],"x right      %d",pcx_xright);
   info[++lc]=new char[100]; sprintf(info[lc],"y bottom     %d",pcx_ybottom);
   info[++lc]=new char[100]; sprintf(info[lc],"horiz. res.  %d",pcx_hres);
   info[++lc]=new char[100]; sprintf(info[lc],"vert. res.   %d",pcx_vres);
   info[++lc]=new char[100]; sprintf(info[lc],"bitplanes    %d",pcx_bitplanes);
   info[++lc]=new char[100]; sprintf(info[lc],"bits/scan line  %d",pcx_bitsperscanline);
   info[++lc]=new char[100]; sprintf(info[lc],"palette info    %d",pcx_paletteinfo);
   info[++lc]=new char[100]; sprintf(info[lc],"palette at end  %d",paletteatend);
   info[++lc]=new char[100]; sprintf(info[lc],"no.of colors    %d",noofcolors);
   fclose(fp);

   l->title = listtitle;
   l->info  = info;
   l->count = lc+1;
   l->itemstoshow = min(20,lc+1);
   l->firstitem   = 0;
   l->wantsort    = 0;
   l->wantsave    = 0;
   l->helptopic   = 0;
   l->allowedit   = 0;
   l->selection   = &g.crud;
   l->width       = 0;
   l->transient   = 1;
   l->wantfunction = 0;
   l->autoupdate   = 0;
   l->clearbutton  = 0;
   l->highest_erased = 0;
   l->f1 = null;
   l->f2 = null;
   l->f3 = null;
   l->f4 = delete_list;
   l->f6 = null;
   l->listnumber = 0;
   list(l);
}


  
//--------------------------------------------------------------------------//
// writepcxfile                                                             //
// save screen image on disk                                                //
//--------------------------------------------------------------------------//
int writepcxfile(char *filename, int write_all, int compress)
{
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
  FILE *fp;
  int bpp,ino,i,j,k,k3;
  uchar temp[202];
  int xstart,ystart,xend,yend;
  int count,data,old;

  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize;
      yend = ystart + z[ci].ysize;
  } else
  {   xstart=g.selected_ulx;
      xend=g.selected_lrx;
      ystart=g.selected_uly;
      yend=g.selected_lry;
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 

  temp[0]=10;                 // PCX file
  temp[1]=5;                  // version 5
  temp[2]=1;                  // Run-length compression
  if(g.want_color_type > 0) temp[3]=8;   // bits per pixel
  else temp[3]=1;
  fwrite(&temp,4,1,fp); 
    
  putword(xstart,fp);
  k=ystart;
  putword(k,fp);

  putword(xend,fp);

  k=yend;
  putword(k,fp);
  k=300;
  putword(k,fp);
  putword(k,fp);
  k=0;
  for(j=1;j<=48;j++) putbyte(k,fp); 
                               // header palette (48 bytes of 0's)
  
  putbyte(k,fp);           // reserved
  k=1;
  putbyte(k,fp);           // no. of pixel planes
   
  i=(xend-xstart+1)*temp[3];
  while(i%8) i++;
  i /= 8;                  // bytes per line

  putword(i,fp);
 
  k=0;
  putword(k,fp);           // palette interp
  k=g.main_xsize;
  putword(k,fp);           // x screen size
  k=g.main_ysize;
  putword(k,fp);           // y screen size
  k=0;
  for(j=1;j<=54;j++) putbyte(k,fp); 
                           // 54 blanks to end of header

  ////  Write image data in compressed form      

   if(g.want_color_type == 0) message("Saving as monochrome");
  
   for(j=ystart;j<yend;j++)
   {   i=xstart;
       if(write_all)
          data = z[ci].image[z[ci].cf][j-ystart][i-xstart];
       else
          data = readpixelonimage(i,j,bpp,ino);
       count=1;
       old = data;

       while(i<=xend)
       {
a:         i++;
           if(i<=xend)
           {    if(g.want_color_type > 0)
                {  if(write_all)
                      data = z[ci].image[z[ci].cf][j-ystart][i-xstart];
                   else
                      data = readpixelonimage(i,j,bpp,ino);
                }
                else
                {  if(write_all)
                   {  data = 0;
                      for(k3=0;k3<8;k3++)
                      if( z[ci].image[z[ci].cf][j-ystart][(i-xstart)+k3] > 127 )
                           data += 1<<(7-k3);                     
                   }      
                   else
                     data=readbyte(i,j);     
                   i+=7;
                }
                if((data==old)&&(count<63)&&(i<=xend))
                {    count++;
                     goto a;
                }
           }
           if((count>1)||(old>=0xc0))
           {    count |= 0xc0;
                putbyte(count,fp);
                putbyte(old,fp);
           }
           else putbyte(old,fp);  
           count=1;
           old=data;           
       }

   }

    /*----- Write palette ---------------------------*/


   if(g.want_color_type > 0)
   {   putbyte(12, fp);
       for(k=0;k<=255;k++)
       {   putbyte(g.palette[k].red<<2, fp);
           putbyte(g.palette[k].green<<2, fp);
           putbyte(g.palette[k].blue<<2, fp);
       }
   }
   close_file(fp, compress);  
   return OK; 
}

