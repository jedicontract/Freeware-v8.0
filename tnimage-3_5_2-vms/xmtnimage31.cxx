//--------------------------------------------------------------------------//
// xmtnimage31.cc                                                           //
// fits and pds files                                                       //
// Latest revision: 06-12-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;
extern PrinterStruct printer;


//--------------------------------------------------------------------------//
//  readpdsfile                                                             //
//--------------------------------------------------------------------------//
int readpdsfile(char *filename)
{  
    return(readpds2file(filename));
}            

//--------------------------------------------------------------------------//
//  readpds2file                                                            //
//  Comments embedded within a label cause remainder of label to be ignored.//
//--------------------------------------------------------------------------//
int readpds2file(char *filename)
{  
  int format, status=OK, bpp=8, xsize=0, ysize=0, index=0, len=0,
      bytesperpixel, colortype=GRAY, xpos=0, ypos=0,x,x2,y,
      suffix_bytes=0, record_bytes=0, image_ptr=0,
      label_records=0;
  uchar cval=0;
  uint image_start=0;
  float fval=0;
  double dval=0;
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       status=NOTFOUND;
       return(status);
  }

  char tempstring[1024];
  char record[2880];
  char name[128];
  char value[128];
  char buf[10];
  char *equals=0;

  while(1)
  {   fgets(record,2880,fp); 
      if(record[0]!='/' || record[1]!='*')
      {    memcpy(name, record, 20);
           name[20]=0;
           if(name[0]=='E' && name[1]=='N' && name[2]=='D' && name[3]<=32) break;
           equals = 2+strchr(record,'=');
           len = strlen(record) - (equals - record);
           memcpy(value, equals, len);
           value[min(len-2, (int)strcspn(value, "/*"))] = 0;
           if(!strcmp(name,"RECORD_BYTES        ")) record_bytes=atoi(value);
           //// if(!strcmp(name,"FILE_RECORDS        ")) file_records=atoi(value);
           if(!strcmp(name,"LABEL_RECORDS       ")) label_records=atoi(value);
           //// if(!strcmp(name,"TRAILER_RECORDS     ")) trailer_records=atoi(value);
           if(!strcmp(name,"^IMAGE              ")) image_ptr=atoi(value);
           if(!strcmp(name,"IMAGE_LINES         ")) ysize=atoi(value);
           if(!strcmp(name,"LINE_SAMPLES        ")) xsize=atoi(value);
           if(!strcmp(name,"LINE_SUFFIX_BYTES   ")) suffix_bytes=atoi(value);
           if(!strcmp(name,"SAMPLE_BITS         ")) bpp=atoi(value);
           //// if(!strcmp(name,"SAMPLE_BIT_MASK     ")) bitmask=atoi(value);
     }
  }  
  

  bytesperpixel = (7+bpp)/8;
  if(image_ptr) image_start = record_bytes * image_ptr;
  else image_start = record_bytes * label_records;
  fseek(fp, image_start, SEEK_SET);

  if(record_bytes > xsize*bytesperpixel && suffix_bytes==0) 
     suffix_bytes = xsize*bytesperpixel - record_bytes;

  ////  Check parameters
  if(xsize<=0 || ysize<=0)
  {   message("Illegal image size", ERROR);
      fclose(fp);
      return ERROR;
  }

  format=INTEGER;
  switch(bpp)
  {   case 8:  format=CHARACTER; break;
      case 16: format=WORD; break;
      case 32: format=INTEGER; break;
      case -32: bpp=24; format=FLOAT; break;
      case -64: bpp=24; format=DOUBLE; break;
      default: 
          sprintf(tempstring,"Unsupported image depth: %d",bpp);
          message(tempstring, ERROR);
          fclose(fp);
          return ERROR;
  }

  if(newimage(basefilename(filename),
              xpos, ypos,       
              xsize, ysize,
              bpp, colortype,
              1,       // frames
              g.want_shell,
              1,       // scrollbars
              PERM, 0,
              g.window_border,
              0)!=OK) { fclose(fp); return(NOMEM); }

  char *buffer = new char[record_bytes];
  for(y=0;y<ysize;y++)  
  {   fread(buffer, record_bytes, 1, fp);
      index = 0;
      for(x=0,x2=0;x<xsize;x++,x2+=bytesperpixel)  
      {   switch(format)
          {   case CHARACTER: cval = buffer[index++];
                              z[ci].image[0][y][x] = cval; 
                              break;
              case WORD:      z[ci].image[0][y][x2+1]=buffer[index++]-128;
                              z[ci].image[0][y][x2]=buffer[index++]-128;
                              break;
              case INTEGER:   z[ci].image[0][y][x2+3]=buffer[index++]-128;
                              z[ci].image[0][y][x2+2]=buffer[index++]-128;
                              z[ci].image[0][y][x2+1]=buffer[index++]-128;
                              z[ci].image[0][y][x2+0]=buffer[index++]-128;
                              break;
              case FLOAT:     memcpy(buf, buffer+index, 4);
                              buf[4]=0;
                              fval = atof(buf);
                              index+=4;
                              putpixelbytes(z[ci].image[0][y]+x2,(int)fval,1,32,1);
                              break;
              case DOUBLE:    memcpy(buf, buffer+index, 8);
                              buf[8]=0;
                              dval = atof(buf);
                              index+=8;
                              putpixelbytes(z[ci].image[0][y]+x2,(int)dval,1,32,1);
                              break;
          }
      }
  }
  fclose(fp);
  setSVGApalette(1); 
  z[ci].colortype=GRAY;
  z[ci].hitgray=0;
  delete[] buffer;
  return 0;
}          


//--------------------------------------------------------------------------//
//  aboutpdsfile                                                            //
//--------------------------------------------------------------------------//
void aboutpdsfile(char *filename)
{  
  static listinfo *l;
  static char *listtitle;
  listtitle = new char[100];
  strcpy(listtitle, "About the File");
  static char **info;                                    
  int done=0, k; 
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       return;
  }

  char record[2880];
  int lc=-1;
  info = new char*[100];   // max. 100 lines - Change if you add more items

  info[++lc] = new char[100];                       
  sprintf(info[lc],"Filename:  %s",basefilename(filename));
  info[++lc]=new char[100]; sprintf(info[lc],"File format: PDS (Planetary Data System)");

  while(1)
  {   fgets(record,2880,fp); 
      if(record[0]=='E' && record[1]=='N' && record[2]=='D' && record[3]<=32) done=1;
      for(k=0;k<(int)strlen(record);k++) if(record[k]=='\n') record[k]=0;
      if(done || lc>=100) break;
      info[++lc]=new char[100]; strcpy(info[lc],record);
  }  
  
  l = new listinfo;
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
  l->f5 = null;
  l->f6 = null;
  l->listnumber = 0;
  list(l);
  return;
}  
          

//--------------------------------------------------------------------------//
//  aboutfitsfile                                                           //
//--------------------------------------------------------------------------//
void aboutfitsfile(char *filename)
{  
  static listinfo *l;
  static char *listtitle;
  listtitle = new char[100];
  strcpy(listtitle, "About the File");
  static char **info;                                    
  int done=0, k;
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       return;
  }

  char record[2880];
  char card[81];
  card[80]=0;
  int lc=-1;
  info = new char*[100];   // max. 100 lines - Change if you add more items

  info[++lc] = new char[100];                       
  sprintf(info[lc],"Filename:  %s",basefilename(filename));
  info[++lc]=new char[100]; sprintf(info[lc],"File format: FITS (Flexible Image Transport System)");

  while(!done)
  { 
      fread(&record,2880,1,fp); 
      for(k=0;k<36;k++)
      {    memcpy((void*)card, record+k*80, 80);
          if(card[0]=='E' && card[1]=='N' && card[2]=='D' && card[3]<=32) done=1;
          if(done || lc>=99) break;
          card[80] = 0;
          info[++lc]=new char[100]; strncpy(info[lc],card,99);
      }
      if(done || lc>=99) break;
  }
  fclose(fp);

  l = new listinfo;
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
  l->f5 = null;
  l->f6 = null;
  l->listnumber = 0;
  list(l);
  return;
}  


//--------------------------------------------------------------------------//
//  readfitsfile                                                            //
//--------------------------------------------------------------------------//
int readfitsfile(char *filename)
{  
  int done=0,format, status=OK, naxis=0, k, bpp=8, xsize=0, ysize=0, 
      bytesperpixel, colortype=GRAY, xpos=0, ypos=0,x,x2,y;
  uchar cval=0;
  int ival=0;
  float fval=0;
  double dval=0;
  FILE *fp;
  if ((fp=fopen(filename,"rb")) == NULL)
  {    error_message(filename, errno);
       status=NOTFOUND;
       return(status);
  }

  char tempstring[1024];
  char record[2880];
  char card[81];
  char keyword[8];
  char value[80];
  keyword[7]=0;
  card[80]=0;
  uchar buf[3];

  while(!done)
  { 
      fread(&record,2880,1,fp); 
      for(k=0;k<36;k++)
      {    memcpy((void*)card, record+k*80, 80);
           if(card[0]=='E' && card[1]=='N' && card[2]=='D' && card[3]<=32) done=1;
           if(done) break;
           memcpy(keyword, card, 7);
           memcpy(value,   card+9, 70);
           if(!strcmp(keyword,"BITPIX ")) bpp=atoi(value);
           if(!strcmp(keyword,"NAXIS  ")) naxis=atoi(value);
           if(!strcmp(keyword,"NAXIS1 ")) xsize=atoi(value);
           if(!strcmp(keyword,"NAXIS2 ")) ysize=atoi(value);           
           //// if(!strcmp(keyword,"BSCALE ")) bscale=atof(value);           
           //// if(!strcmp(keyword,"BZERO  ")) bzero=atof(value);           
      }
  }  

  ////  Check parameters
  if(naxis!=2)
  {   sprintf(tempstring, "%d axes in FITS file\nnot supported",naxis);
      message(tempstring, ERROR);
      fclose(fp);
      return ERROR;
  }
  if(xsize<=0 || ysize<=0)
  {   message("Illegal image size", ERROR);
      fclose(fp);
      return ERROR;
  }

  format=INTEGER;
  switch(bpp)
  {   case 8:  colortype=GRAY; format=CHARACTER; break;
      case 16: format=WORD; break;
      case 32: format=INTEGER; break;
      case -32: bpp=24; format=FLOAT; break;
      case -64: bpp=24; format=DOUBLE; break;
      default: 
          sprintf(tempstring,"Unsupported image depth: %d",bpp);
          message(tempstring, ERROR);
          fclose(fp);
          return ERROR;
  }

  bytesperpixel = (7+bpp)/8;
  if(newimage(basefilename(filename),
              xpos, ypos,       
              xsize, ysize,
              bpp, colortype,
              1,       // frames
              g.want_shell,
              1,       // scrollbars
              PERM, 0,
              g.window_border,
              0)!=OK) { fclose(fp); return(NOMEM); }

  for(y=ysize-1;y>=0;y--)  
  for(x=0,x2=0;x<xsize;x++,x2+=bytesperpixel)  
  {     switch(format)
        {     case CHARACTER: 
                              fread(&cval,1,1,fp);
                              z[ci].image[0][y][x] = cval; 
                              break;
              case WORD:      
                              fread(&buf,2,1,fp);
#ifdef LITTLE_ENDIAN
                              z[ci].image[0][y][x2+1]=buf[0]-128;
                              z[ci].image[0][y][x2]=buf[1];
#else
                              z[ci].image[0][y][x2]=buf[0]-128;
                              z[ci].image[0][y][x2+1]=buf[1];
#endif
                              break;
              case INTEGER:   fread(&ival,4,1,fp);
                              putpixelbytes(z[ci].image[0][y]+x2,ival,1,32,1);
                              break;
              case FLOAT:     fread(&fval,4,1,fp); 
                              putpixelbytes(z[ci].image[0][y]+x2,(int)fval,1,32,1);
                              break;
              case DOUBLE:    fread(&dval,8,1,fp); 
                              putpixelbytes(z[ci].image[0][y]+x2,(int)dval,1,32,1);
                              break;
        }
  }
  fclose(fp);
  setSVGApalette(1); 
  z[ci].colortype=GRAY;
  z[ci].hitgray=0;
  return 0;
}  
          
          

//--------------------------------------------------------------------------//
//  writefitsfile                                                           //
//--------------------------------------------------------------------------//
int writefitsfile(char *filename, int write_all, int compress)
{
  char record[2881];
  char tempstring[81];
  int want_color = 1 - g.usegrayscale;
  int bpp, ino, i, j, xstart, ystart, xend, yend, extra_bytes;
  int v;
  
  FILE *fp;
  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize - 1;
      yend = ystart + z[ci].ysize - 1;
  } else
  {   xstart = g.selected_ulx;
      xend   = g.selected_lrx;
      ystart = g.selected_uly;
      yend   = g.selected_lry;
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
      return CANTCREATEFILE ; 

  memset(record,32,2880); // Pad with spaces
  add_fits_key(record,0,"SIMPLE", "T");  
  add_fits_key(record,1,"BITPIX", g.want_bpp);  
  add_fits_key(record,2,"NAXIS" , 2);  
  add_fits_key(record,3,"NAXIS1", xend - xstart + 1);  
  add_fits_key(record,4,"NAXIS2", yend - ystart + 1);  
  add_fits_key(record,5,"DATAMIN", 0);  
  add_fits_key(record,6,"DATAMAX", (int)g.maxvalue[g.want_bpp]);  
  add_fits_key(record,7,"BZERO", "0.0");  
  add_fits_key(record,8,"BSCALE", "1.0");  
  sprintf(tempstring, "%s %s", basefilename(g.appname), g.version);
  add_fits_key(record,9,"HISTORY", tempstring);  
  add_fits_key(record,10,"END", "");  
  fwrite(record,2880,1,fp);  

  for(j=yend;j>=ystart;j--)
  {  for(i=xstart;i<=xend;i++)
     {   
         v = readpixelonimage(i,j, bpp, ino);
         v = convertpixel(v, bpp, g.want_bpp, want_color);
         switch(g.want_bpp)
         {    case 8: putbyte(v, fp);  
                      break;
              case 16:
                      putword(v, fp);  
                      break;
              default:
                      fwrite(&v, g.off[g.want_bpp], 1, fp);  
                      break;
         }
     }
  }     
  extra_bytes = 2880 - g.off[g.want_bpp]*(xend-xstart+1)*(yend-ystart+1) % 2880;
  memset(record,0,2880); // Pad with spaces
  fwrite(record,extra_bytes,1,fp);  
  close_file(fp, compress);  
  return OK;
}

   

//--------------------------------------------------------------------------//
//  add_fits_key                                                            //
//--------------------------------------------------------------------------//
void add_fits_key(char *r, int n, const char *keyword, int value)
{  
   char tempstring[81];
   sprintf(tempstring,"%d",value);
   add_fits_key(r,n,keyword,tempstring);  
}
void add_fits_key(char *r, int n, const char *keyword, const char *value)
{ 
   int len = strlen(value);
   int offset = n*80;
   memset(r+offset,32,80);
   memcpy(r+offset, (void*)keyword, strlen(keyword));
   if(strlen(value)) 
   {   r[offset+8] = '=';
       memcpy(r+offset+30-len, (void*)value, strlen(value));
   }
}
