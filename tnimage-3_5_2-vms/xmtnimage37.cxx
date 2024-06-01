//--------------------------------------------------------------------------//
// xmtnimage37.cc                                                           //
// Latest revision: 06-12-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// XWD, PGM, PPM, PBM formats                                               //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
uchar ccc; // for swapbyte

//--------------------------------------------------------------------------//
// readxwdfile                                                              //
// read XWD (X windows bitmap) file.                                        //
//--------------------------------------------------------------------------//
int readxwdfile(char *filename)
{
  int status=OK, k,y, colortype=g.colortype, xpos=0, ypos=0, pixels, xsize;
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       status=NOTFOUND;
       return(status);
  }

  XWDHeader xwd;
  fread(&xwd,sizeof(XWDStruct),1,fp); 
#ifdef LITTLE_ENDIAN
  swapbytes((uchar*)&xwd, sizeof(XWDStruct), sizeof(int));
#endif

  fseek(fp,-1+xwd.header_size,SEEK_SET);
  if(xwd.colormap_entries > 256){ message("Colormap error reading xwd file"); return 0; }
  XWDColorMap xwdcolormap[256];
  for(k=0;k<xwd.colormap_entries;k++)
  {   xwdcolormap[k].entry_number = getdword(fp);
      xwdcolormap[k].red     = getword(fp);
      xwdcolormap[k].green   = getword(fp);
      xwdcolormap[k].blue    = getword(fp);
      xwdcolormap[k].flags   = getbyte(fp);
      xwdcolormap[k].padding = getbyte(fp);
  }

  if(xwd.file_version!=7){ message("Unable to read XWD file"); return ERROR; }
  switch(xwd.visual_class)
  {     case StaticGray:
        case GrayScale:    colortype=GRAY; break; 
        case DirectColor:
        case TrueColor:    colortype=COLOR; break;
        default:           colortype=INDEXED; break;
  }          
  if(g.want_shell){ xpos = xwd.window_x; ypos = xwd.window_y; }  
  pixels = xwd.bytes_per_line/(xwd.bits_per_pixel/8);
  xsize = max(xwd.pixmap_width, pixels);
  if(newimage(basefilename(filename),
        xpos, ypos,
        xsize, xwd.pixmap_height,     
        xwd.bits_per_pixel, 
        colortype, 1,       // frames
        g.want_shell, 1,
        PERM, 0,
        g.window_border,
        0)!=OK) { fclose(fp); return(NOMEM); }
  for(k=0; k<xwd.colormap_entries; k++)
  {    z[ci].palette[k].red = xwdcolormap[k].red>>10;
       z[ci].palette[k].green = xwdcolormap[k].green>>10;
       z[ci].palette[k].blue = xwdcolormap[k].blue>>10;
  }
  memcpy(z[ci].opalette, z[ci].palette, 768); 

  uchar junk;
  setpalette(z[ci].palette);
  for(y=0; y<xwd.pixmap_height; y++)
  {  if(xwd.bits_per_pixel==32)
     { 
       fread(&z[ci].image[0][y][3], xwd.bytes_per_line-3, 1, fp);
       for(k=0;k<3;k++) fread(&junk,1,1,fp);
     }else
       fread(&z[ci].image[0][y][0], xwd.bytes_per_line, 1, fp);
  }  
  fclose(fp);
  return OK;
}


//--------------------------------------------------------------------------//
// aboutxwdfile                                                             //
// give information about XWD (X windows bitmap) file.                      //
//--------------------------------------------------------------------------//
void aboutxwdfile(char *filename)
{
  static char **info;                                    
  static listinfo *l;
  static char *listtitle;
  listtitle = new char[100];
  strcpy(listtitle, "About the File");
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       return;
  }

  XWDHeader xwd;
  XWDColorMap xwdcolormap;
  fread(&xwd,sizeof(XWDStruct),1,fp); 
  fread(&xwdcolormap,sizeof(XWDColorMap),1,fp); 
#ifdef LITTLE_ENDIAN
  swapbytes((uchar*)&xwd, sizeof(XWDStruct), sizeof(int));
#endif

  //-------------Put information in list box-------------------------//

  int lc=-1;
  info = new char*[100];   // max. 100 lines - Change if you add more items

  info[++lc] = new char[100];                       
  sprintf(info[lc],"Filename:  %s",basefilename(filename));

   //-------program name-------------------------//

  info[++lc]=new char[100]; sprintf(info[lc],"File format: XWD (X-Window screen dump)");
  info[++lc]=new char[100]; sprintf(info[lc],"Header size:   %d",xwd.header_size);
  info[++lc]=new char[100]; sprintf(info[lc],"file_version:  %d",xwd.file_version);
  info[++lc]=new char[100]; sprintf(info[lc],"pixmap_format: %d",xwd.pixmap_format);
  info[++lc]=new char[100]; sprintf(info[lc],"pixmap_depth:  %d",xwd.pixmap_depth);
  info[++lc]=new char[100]; sprintf(info[lc],"pixmap_width:  %d",xwd.pixmap_width);
  info[++lc]=new char[100]; sprintf(info[lc],"pixmap_height: %d",xwd.pixmap_height);
  info[++lc]=new char[100]; sprintf(info[lc],"xoffset:     %d", xwd.xoffset);
  info[++lc]=new char[100]; sprintf(info[lc],"byte order:  %d", xwd.byte_order);
  info[++lc]=new char[100]; sprintf(info[lc],"bitmap unit: %d", xwd.bitmap_unit);
  info[++lc]=new char[100]; sprintf(info[lc],"bitmap bit order: %d",xwd.bitmap_bit_order);
  info[++lc]=new char[100]; sprintf(info[lc],"bitmap pad:  %d", xwd.bitmap_pad);
  info[++lc]=new char[100]; sprintf(info[lc],"bits per pixel: %d",xwd.bits_per_pixel);
  info[++lc]=new char[100]; sprintf(info[lc],"bytes per line: %d",xwd.bytes_per_line);
  info[++lc]=new char[100]; sprintf(info[lc],"visual class:   %d",xwd.visual_class);
  info[++lc]=new char[100]; sprintf(info[lc],"red mask: %d", xwd.red_mask);
  info[++lc]=new char[100]; sprintf(info[lc],"green mask: %d", xwd.green_mask);
  info[++lc]=new char[100]; sprintf(info[lc],"blue mask: %d", xwd.blue_mask);
  info[++lc]=new char[100]; sprintf(info[lc],"bits per rgb: %d", xwd.bits_per_rgb);
  info[++lc]=new char[100]; sprintf(info[lc],"colormap entries: %d", xwd.colormap_entries);
  info[++lc]=new char[100]; sprintf(info[lc],"no. of colors: %d", xwd.ncolors);
  info[++lc]=new char[100]; sprintf(info[lc],"window width: %d", xwd.window_width);
  info[++lc]=new char[100]; sprintf(info[lc],"window height: %d",xwd.window_height);
  info[++lc]=new char[100]; sprintf(info[lc],"window x: %d", xwd.window_x);
  info[++lc]=new char[100]; sprintf(info[lc],"window y: %d", xwd.window_y);
  info[++lc]=new char[100]; sprintf(info[lc],"window border width %d",xwd.window_bdrwidth);  
  fclose(fp);

  l = new listinfo;
  l->title = listtitle;
  l->info  = info;
  l->count = lc+1;
  l->itemstoshow = min(20,lc+1);
  l->firstitem   = 0;
  l->wantsave    = 0;
  l->wantsort    = 0;
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
  l->f5 = null;
  l->f6 = null;
  l->listnumber = 0;
  list(l);

}


//-------------------------------------------------------------------------//
// getnextnumber                                                           //
//-------------------------------------------------------------------------//
int getnextnumber(char *buffer, int &bufpos, FILE *fp)
{
  char answer[16];
  int len,c, numberpos=0;

  len = (int)strlen(buffer);
  answer[0] = 0;
  c = buffer[bufpos];
  while(!isdigit(c)) 
  {    c = buffer[++bufpos];
       if(bufpos>len)
       {    fgets(buffer,128,fp);
            bufpos = 0;
            c = buffer[bufpos];
            if(feof(fp)) return 0;
       }
  }
  while(isdigit(c))
  {    answer[numberpos++] = c;
       c = buffer[++bufpos];
       answer[numberpos] = 0;
  }
  answer[numberpos] = 0;
  return atoi(answer);
}


//-------------------------------------------------------------------------//
// readpbmfile                                                             //
// puts image in buffer so image can be refreshed by camera.               //
//-------------------------------------------------------------------------//
int readpbmfile(char *filename, uchar **buffer)
{
  FILE *fp;
  int c,j,k,x,x2,y,rr,gg,bb,bufpos,bytesperline=1,bytesperpixel,colortype=GRAY,xsize,ysize,bpp=8,
      frames=1,value,maxvalue=0,vfac=1, noofvalues;
  char pbmid[4];
  uchar *add;
  char tempstring[128];
  char header[3][16];
  int digit=0,pos=0;
  g.getout = 0;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       return ERROR;
  }

  //// Perform non-blocking I/O so screen can redraw itself while waiting
  //// for camera driver (if reading from a FIFO).
  //// Need feedback as to whether this works in Solaris or Irix

#ifdef LINUX
  printstatus(MESSAGE,(char*)"Waiting...          ");
  pbmid[0]=0;
  int flags,fd;
  fd = fileno(fp);
  if((flags=fcntl(fd,F_GETFL,0))==-1) 
    fprintf(stderr,"Could not get flags for fd\n");
  else
  {   flags |= O_NONBLOCK;
      fcntl(fd, F_SETFL,flags);
  }
  do
  {   check_event();
      usleep(10000);  //// Must sleep otherwise image gets noisy
      fgets(pbmid,4,fp);
  } while(pbmid[0]==0 && g.getout==0);
  flags &= ~O_NONBLOCK;
  if(g.getout){ fclose(fp); return ABORT; }
  fcntl(fd, F_SETFL,flags);
  printstatus(READING);
#else
  fgets(pbmid,4,fp); 
#endif
 
  noofvalues = 3;
  if(pbmid[1]=='4') noofvalues = 2;
  while(digit<noofvalues)
  {   
      fgets(tempstring,127,fp); 
      if(tempstring[0]!='#')
      {    for(k=0;k<(int)strlen(tempstring);k++) 
           {   if(isdigit(tempstring[k])) header[digit][pos++]=tempstring[k];
               else { digit++; pos = 0; }
               header[digit][pos]=0;
           }
      }
  }
  xsize = atoi(header[0]);
  ysize = atoi(header[1]);
  maxvalue = atoi(header[2]);
  switch(pbmid[1])
  {   case '1':      // pbm 1bpp ascii
         bpp=8;
         colortype=GRAY;
         bytesperline=xsize;
         break;
      case '2':      // pgm ascii    
         for(bpp=0;bpp<48;bpp++) if(((1+maxvalue)>>bpp)==1)break;
         bpp=8*((7+bpp)/8);
         colortype=GRAY;
         bytesperline=xsize*g.off[bpp];
         break;
      case '3':      // ppm ascii    
         bpp=24;
         colortype=COLOR;
         bytesperline=3*xsize;
         break;
      case '4':      // pbm 1 bpp binary
         bpp=8;
         colortype=GRAY;
         bytesperline=(xsize+7)/8;
         break;
      case '5':      // pgm ascii    
         for(bpp=0;bpp<48;bpp++) if(((1+maxvalue)>>bpp)==1)break;
         bpp=8*((7+bpp)/8);
         colortype=GRAY;
         bytesperline=xsize*g.off[bpp];
         break;
      case '6':      // ppm ascii    
         bpp=24;
         colortype=COLOR;
         bytesperline=3*xsize;
         break;
  }         
  if(bytesperline+8 >= 65536) { message("Error reading PBM file"); return ERROR; }
  char rowbuf[65536];                  // Row of pixels
  if(buffer==NULL)
  {   if(newimage(basefilename(filename),
         0,0,xsize,ysize,bpp,colortype,frames,g.want_shell,1,
         PERM, 0, g.window_border, 0)!=OK) 
         { fclose(fp); return(NOMEM); }
      buffer = z[ci].image[0];
  }
  bytesperpixel =g.off[bpp];
  if(maxvalue) vfac = 256/(1+maxvalue);
  
  if(pbmid[1]<='3')
  {   bufpos=0;        
      for(y=0; y<ysize; y++) 
      {   
         for(x=0,x2=0; x<xsize; x++,x2+=g.off[bpp]) 
         {    
            if(colortype==GRAY)
               value = getnextnumber(rowbuf, bufpos, fp);
            else
            {  rr = vfac*getnextnumber(rowbuf, bufpos, fp);
               gg = vfac*getnextnumber(rowbuf, bufpos, fp);
               bb = vfac*getnextnumber(rowbuf, bufpos, fp);
               value = RGBvalue(rr,gg,bb,bpp);
            }
            switch(pbmid[1])
            {   case '1':      // pbm 1bpp ascii
                   if(value==1) buffer[y][x] = 0;
                   else buffer[y][x] = 255;
                   break;
                case '2':      // pgm ascii    
                   if(maxvalue<255) value *= vfac;
                   putpixelbytes(buffer[y]+x2, value, 1, bpp, 1);
                   break;
                case '3':      // ppm ascii    
                   putpixelbytes(buffer[y]+x2, value, 1, bpp, 1);
                   break;
            }
         }         
      }
  }else
  {  
      for(y=0; y<ysize; y++) 
      {   fread(rowbuf,bytesperline,1,fp);
          switch(pbmid[1])
          {   case '4':      // pbm 1bpp
                 x=0;
                 for(k=0;k<bytesperline;k++)
                 for(j=7;j>=0;j--)
                 {   c = rowbuf[k];
                     if(c & (1<<j)) value=0; else value=255;
                     buffer[y][x++] = value;
                 }   
                 break;
              case '5':      // pgm    
                 memcpy(buffer[y], rowbuf, bytesperline);
                 break;
              case '6':      // ppm    
                 add = buffer[y];
                 memcpy(add, rowbuf, bytesperline);
                 for(k=0;k<bytesperline;k+=bytesperpixel) 
                      swapbyte(*(add+k+0),*(add+k+2));
                 break;
          }         
      }
  }
  fclose(fp);
  return OK;
}

