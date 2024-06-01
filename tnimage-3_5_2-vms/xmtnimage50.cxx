//--------------------------------------------------------------------------//
// xmtnimage50.cc                                                           //
// Misc. format image files                                                 //
// Latest revision: 09-15-2005                                              //
// Copyright (C) 2005 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//
 
#include "xmtnimage.h"
extern Globals     g;
extern Image      *z;
extern int         ci;

// Variables for reading files from xmtnimage3.cc
extern int override;
static inline char bitflip(char c);

//--------------------------------------------------------------------------//
// bitflip                                                                  //
//--------------------------------------------------------------------------//
static inline char bitflip(char c)
{
  int k;
  char a = 0;
  for(k=0;k<8;k++)
  { 
    if( c&(1<<k) ) a |= (1<<(7-k));
  }   
  return 0xff & a;
}

 
//--------------------------------------------------------------------------//
// writeasciifile                                                           //
//--------------------------------------------------------------------------//
int writeasciifile(char *filename, int write_all, int compress)
{  
  int bpp,i,ino,j,v,xstart,ystart,xend,yend,rr,gg,bb;
  int want_color = 1 - g.usegrayscale;
  char answerstring[64];
  double dval;
  FILE *fp;
  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize - 1;
      yend = ystart + z[ci].ysize - 1;
  } else
  {   xstart=g.selected_ulx;
      xend=g.selected_lrx;
      ystart=g.selected_uly;
      yend=g.selected_lry;
  }
  if(g.save_ascii_format==CALIBRATED_VALUE &&
    ((write_all==1 && z[ci].cal_log[2]==-1) || write_all==0))
  {   message("Image z units are uncalibrated", ERROR); 
      return BADPARAMETERS;
  }

  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 

  fprintf(fp,"%d ",z[ci].xsize);
  fprintf(fp,"%d ",z[ci].ysize);

  for(j=ystart; j<=yend; j++)
  { for(i=xstart; i<=xend; i++)
    {   v = readpixelonimage(i,j,bpp,ino);
        v = convertpixel(v, bpp, g.want_bpp, want_color);
        switch(g.save_ascii_format)
        {   case INTEGER: fprintf(fp,"%d ",v); break;
            case DOUBLE:  fprintf(fp,"%g ", (double)v / g.maxvalue[bpp]); break;
            case RGBVALUE: 
                   valuetoRGB(v,rr,gg,bb,bpp);
                   fprintf(fp,"%d %d %d   ", rr,gg,bb); break;
            case CALIBRATED_VALUE: 
                   calibratepixel(i,j,ino,2,dval,answerstring);
                   fprintf(fp,"%s ", answerstring); break;
        }
    }
    fprintf(fp,"\n");
  } 
  close_file(fp, compress);  
  return OK;
}



//--------------------------------------------------------------------------//
// writerawfile                                                             //
//--------------------------------------------------------------------------//
int writerawfile(char *filename, int write_all, int compress)
{
  int bpp,i,ino,j,off,xstart,ystart,xend,yend;
  int want_color = 1 - g.usegrayscale;
  uint v;
  FILE *fp;
  if(write_all==1)
  {   xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend = xstart + z[ci].xsize - 1;
      yend = ystart + z[ci].ysize - 1;
  } else
  {   xstart=g.selected_ulx;
      xend=g.selected_lrx;
      ystart=g.selected_uly;
      yend=g.selected_lry;
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 

  off = g.off[g.want_bpp];
  for(j=ystart;j<=yend;j++)
  {  for(i=xstart;i<=xend;i++)
     {   v = readpixelonimage(i,j,bpp,ino);
         v = convertpixel(v, bpp, g.want_bpp, want_color);
         putpixel(v,off,fp);
     } 
  }     
  close_file(fp, compress);  
  return OK;
}



//--------------------------------------------------------------------------//
// write3Dfile                                                              //
//--------------------------------------------------------------------------//
int write3Dfile(char *filename, int write_all, int compress)
{
  int bpp,f,i,i2,j,v;
  int want_color = 1 - g.usegrayscale;
  FILE *fp;
  if(write_all!=1 || ci<0)
  {   message("Must select an entire image to save as 3D");
      return BADPARAMETERS;
  }
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 

  bpp = z[ci].bpp;
  for(f=0;f<z[ci].frames;f++)
  for(j=0;j<z[ci].ysize;j++)
  {  
     for(i=0,i2=0; i<z[ci].xsize; i++,i2+=g.off[bpp])
     {   v = pixelat(z[ci].image[f][j]+i2, bpp);
         v = convertpixel(v, bpp, g.want_bpp, want_color);
         putpixel(v,g.off[g.want_bpp],fp);  
     }
  }     
  close_file(fp, compress);  
  return OK;
}


//--------------------------------------------------------------------------//
// readimafile                                                              //
// Transfer IMA file (from NPDATA) from disk to screen.                     //
// Returns error code (OK=no errors).                                       //
//--------------------------------------------------------------------------//
int readimafile(char *filename)
{  
   FILE *fp;
   uchar crudbuf[9];
   uchar *buffer;
   uchar value,color;
   int junksize, bytes;

   int xsize = 640;
   int ysize = 480;
   uchar crud;
   char *junk;
   int k,k1,k2,subfile,line;

   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }


   junksize = sizeof(int) * g.tif_skipbytes;
   junk = new char[junksize];
   if(g.tif_skipbytes)
   {   bytes = fread(&junk[0],1,g.tif_skipbytes,fp);
       if(bytes != g.tif_skipbytes) message("Error reading skip bytes");
   }
   free(junk);

   buffer = new uchar[g.off[g.bitsperpixel]*(xsize+4)]; 
   if(buffer==NULL) return(NOMEM);
   if(newimage(basefilename(filename),
        0,0,xsize,ysize,4,INDEXED,1,g.want_shell,1,PERM,1,g.window_border,0)!=OK) 
       { delete[] buffer; return(NOMEM); }

   for(k=0;k<5;k++) fread(&crud,1,1,fp);       /* 6 bytes at start of file  */

                                   /* 12 subfiles, 1 for each GET stmt.     */   
   for(subfile=0;subfile<12;subfile++) 
   {  for(k=0;k<4;k++)
        fread(&crud,1,1,fp);       /* 4 bytes at start of each subfile=junk */
     
      for(line=0;line<40;line++)   /* Each subfile has 40 lines             */
      {                            /* Each line has 4 bitmaps of 640 pixels */                                     
                                   /*  i.e., 80x4 bytes                     */
   
          fread(buffer,320,1,fp);    
          for(k1=0;k1<80;k1++)
          { for(k=0;k<8;k++) crudbuf[k]=0;                
            for(k2=0;k2<4;k2++)
            {   value=1<<k2;
                color=buffer[k1+k2*80];
                if(color&0x80) crudbuf[0]+=value;
                if(color&0x40) crudbuf[1]+=value;
                if(color&0x20) crudbuf[2]+=value;
                if(color&0x10) crudbuf[3]+=value; 
                if(color&0x08) crudbuf[4]+=value; 
                if(color&0x04) crudbuf[5]+=value; 
                if(color&0x02) crudbuf[6]+=value; 
                if(color&0x01) crudbuf[7]+=value;                 
            }
            for(k=0;k<8;k++)
                z[ci].image[z[ci].cf][subfile*40+line][k1*8+k] = crudbuf[k]<<4; 
          }          
      }   
      for(k=0;k<1171;k++)fread(&crud,1,1,fp);
   }
   fclose(fp);
   delete[] buffer;
   return(OK);
}



//--------------------------------------------------------------------------//
// readimgfile                                                              //
// Reads:   IMG files (8 bit from frame grabber)                            //
//          IMM files (1 bit)                                               //
//          RAW files (1-32 bit invalid or unknown file type)               //
//          ASCII files (1-32 bit invalid or unknown file type)             //
// Also can try to read any 1-32 bit image.                                 //
// oldfilename, compress, & signedpixels are only used by initialize_image. //
// g.raw_filename and g.raw_filename2 must be set before calling this func. //
//--------------------------------------------------------------------------//
int readimgfile(int type, int noofargs, char **arg, int compress, char *identifier)
{  
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(ABORT); }  
   static Dialog *dialog;
   int j, k, fsize;
   FILE *fp = NULL;
   fsize = filesize(g.raw_filename);
   if(g.raw_xsize == 0 || g.raw_ysize == 0) g.raw_xsize = g.raw_ysize = cint(sqrt(fsize));
  
   ////  User Input

  if(noofargs > 5)
  {  if(noofargs>5) g.raw_platform = atoi(arg[6]);
     if(noofargs>6) 
     {    g.raw_colortype = atoi(arg[7]);
          switch(g.raw_colortype)
          {  case GRAY: g.raw_want_color=0; g.read_cmyk=0; break;
             case INDEXED: g.raw_want_color=1; g.read_cmyk=0; break;
             case COLOR: g.raw_want_color=1; g.read_cmyk=0; break;
             case CMYK: g.raw_want_color=1; g.read_cmyk=1; break;
          }
     }
     if(noofargs>7) g.raw_packing = atoi(arg[8]);
     if(noofargs>8) g.raw_xsize = atoi(arg[9]);  if(g.raw_xsize<1) return ERROR;
     if(noofargs>9) g.raw_ysize = atoi(arg[10]); if(g.raw_ysize<1) return ERROR;
     if(noofargs>10) g.raw_skip = atoi(arg[11]);
     if(noofargs>11) g.raw_bpp  = atoi(arg[12]); if(!between(g.raw_bpp,0,32)) return ERROR;
     if(noofargs>12) g.raw_rbpp = atoi(arg[13]);
     if(noofargs>13) g.raw_gbpp = atoi(arg[14]);
     if(noofargs>14) g.raw_bbpp = atoi(arg[15]);
     if(noofargs>15) g.raw_kbpp = atoi(arg[16]);
     if(noofargs>16) g.raw_flipbits = atoi(arg[17]);
  }else
  {
    if(! override && !g.read_raw)
        do_read_img_file(g.raw_filename, g.raw_filename, identifier, type, compress);
    else
    {     
      dialog = new Dialog;
      if(dialog==NULL){ message(g.nomemory); return(NOMEM); }
      if(type==RAW)
         strcpy(dialog->title,"Read Raw Bytes");
      else if(type==ASCII)
         strcpy(dialog->title,"Read ASCII");
      else
         strcpy(dialog->title,"IMG File Parameters");
      strcpy(dialog->radio[0][0],"Image Source");              
      strcpy(dialog->radio[0][1],"PC (little endian)");
      strcpy(dialog->radio[0][2],"Mac (big endian)");
      strcpy(dialog->radio[0][3],"MVS Mainframe");
 
      strcpy(dialog->radio[1][0],"Color");
      strcpy(dialog->radio[1][1],"Grayscale");
      strcpy(dialog->radio[1][2],"Indexed color");
      strcpy(dialog->radio[1][3],"RGB Color");
      strcpy(dialog->radio[1][4],"CMYK Color");
                      
      strcpy(dialog->radio[2][0],"Bit Packing");
      strcpy(dialog->radio[2][1],"TIF-like");
      strcpy(dialog->radio[2][2],"GIF-like");
      strcpy(dialog->radio[2][3],"None");

      strcpy(dialog->radio[3][0],"Data type");
      strcpy(dialog->radio[3][1],"Unsigned");
      strcpy(dialog->radio[3][2],"Signed");

      if(g.raw_platform == PC)  dialog->radioset[0] = 1;  // PC Mac IBM = 1 2 3     
      if(g.raw_platform == MAC) dialog->radioset[0] = 2;
      if(g.raw_platform == MVS) dialog->radioset[0] = 3;
      dialog->radioset[1] = g.raw_colortype+1;     
      dialog->radioset[2] = g.raw_packing+1;
      dialog->radioset[3] = g.raw_signed+1;
  
      strcpy(dialog->boxes[0] ,"Image Parameters");
      strcpy(dialog->boxes[1] ,"X size (pixels)" );
      strcpy(dialog->boxes[2] ,"Y size (pixels)" );
      if(type==ASCII)
         strcpy(dialog->boxes[3] ,"Values to skip" );
      else
         strcpy(dialog->boxes[3] ,"Bytes to skip" );
      strcpy(dialog->boxes[4] ,"Total frames" );
      strcpy(dialog->boxes[5] ,"Image depth" );// label
      strcpy(dialog->boxes[6] ,"Gray bits/pixel");       
      strcpy(dialog->boxes[7] ,"Red bits/pixel" );
      strcpy(dialog->boxes[8] ,"Green bits/pixel" );
      strcpy(dialog->boxes[9] ,"Blue bits/pixel " );
      strcpy(dialog->boxes[10] ,"Black bits/pixel" );
      strcpy(dialog->boxes[11] ,"Auto Increment" ); 
      strcpy(dialog->boxes[12] ,"Increment / line" );
      strcpy(dialog->boxes[13] ,"Flip bits" );

      sprintf(dialog->answer[1][0], "%d", g.raw_xsize);
      sprintf(dialog->answer[2][0], "%d", g.raw_ysize);
      sprintf(dialog->answer[3][0], "%d", g.raw_skip);
      sprintf(dialog->answer[4][0], "%d", g.raw_frames);

      sprintf(dialog->answer[6][0], "%d", g.raw_bpp);
      sprintf(dialog->answer[7][0], "%d", g.raw_rbpp);
      sprintf(dialog->answer[8][0], "%d", g.raw_gbpp);
      sprintf(dialog->answer[9][0], "%d", g.raw_bbpp);
      sprintf(dialog->answer[10][0], "%d", g.raw_kbpp);

      dialog->boxset[11] = g.raw_want_increment;    
      sprintf(dialog->answer[12][0], "%d", g.raw_inc_inc);
      dialog->boxset[13] = g.raw_flipbits;    
   
      dialog->radiono[0]=4;
      dialog->radiono[1]=5;
      dialog->radiono[2]=4;
      dialog->radiono[3]=3;
      dialog->radiono[4]=0;
      for(j=0;j<10;j++) for(k=0;k<20;k++) dialog->radiotype[j][k]=RADIO;
      dialog->boxtype[0]=LABEL; 
      dialog->boxtype[1]=STRING;
      dialog->boxtype[2]=STRING;
      dialog->boxtype[3]=STRING;
      dialog->boxtype[4]=INTCLICKBOX;
      dialog->boxtype[5]=LABEL;
      dialog->boxtype[6]=INTCLICKBOX;
      dialog->boxtype[7]=INTCLICKBOX;
      dialog->boxtype[8]=INTCLICKBOX;
      dialog->boxtype[9]=INTCLICKBOX;
      dialog->boxtype[10]=INTCLICKBOX;
      dialog->boxtype[11]=TOGGLE;
      dialog->boxtype[12]=INTCLICKBOX;
      dialog->boxtype[13]=TOGGLE;
    
      dialog->boxmin[4]=1; dialog->boxmax[4]=32;
      dialog->boxmin[6]=0; dialog->boxmax[6]=32;
      dialog->boxmin[7]=0; dialog->boxmax[7]=32;
      dialog->boxmin[8]=0; dialog->boxmax[8]=32;
      dialog->boxmin[9]=0; dialog->boxmax[9]=32;
      dialog->boxmin[10]=0; dialog->boxmax[10]=32;
      dialog->boxmin[12]=1; dialog->boxmax[12]=100;
     
      dialog->noofradios=4;
      dialog->noofboxes=14;
      dialog->helptopic=11;  
      dialog->want_changecicb = 0;
      dialog->f1 = readimgfilecheck;
      dialog->f2 = null;
      dialog->f3 = null;
      dialog->f4 = readimgfilefinish;
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
      strcpy(dialog->path,".");
      dialog->param[10] = type;
      dialog->param[11] = compress;
      dialog->ptr[0] = (void*)g.raw_filename;
      dialog->ptr[15] = (void*)fp;
      dialog->ptr[16] = (void*)g.raw_filename2;
      dialog->ptr[17] = (void*)identifier;
      dialog->message[0] = 0;      
      dialog->busy = 0;
      dialogbox(dialog);
    }
  }
  return OK;
}


//--------------------------------------------------------------------------//
//  readimgfilecheck                                                        //
//--------------------------------------------------------------------------//
void readimgfilecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   FILE *fp;
   radio=radio; box=box; boxbutton=boxbutton;
   g.raw_colortype = a->radioset[1]-1;
   switch(g.raw_colortype)     
   {   case GRAY:   
       case INDEXED:
          XtSetSensitive(a->boxwidget[6][0], True);
          XtSetSensitive(a->boxwidget[7][0], False);
          XtSetSensitive(a->boxwidget[8][0], False);
          XtSetSensitive(a->boxwidget[9][0], False);
          XtSetSensitive(a->boxwidget[10][0], False);
          break;
       case COLOR:      
          XtSetSensitive(a->boxwidget[6][0], False);
          XtSetSensitive(a->boxwidget[7][0], True);
          XtSetSensitive(a->boxwidget[8][0], True);
          XtSetSensitive(a->boxwidget[9][0], True);
          XtSetSensitive(a->boxwidget[10][0], False);
          break;
       case CMYK:
          XtSetSensitive(a->boxwidget[6][0], False);
          XtSetSensitive(a->boxwidget[7][0], True);
          XtSetSensitive(a->boxwidget[8][0], True);
          XtSetSensitive(a->boxwidget[9][0], True);
          XtSetSensitive(a->boxwidget[10][0], True);
   }

   if(radio != -2) return;  //// User clicked Ok or Enter

   ////////

   char *filename    = (char *)a->ptr[0];
   fp                = (FILE*)a->ptr[15];
   char *oldfilename = (char *)a->ptr[16];
   char *identifier  = (char *)a->ptr[17];
   int type          = a->param[10];
   int compress      = a->param[11];

   if(a->radioset[0] == 1) g.raw_platform = PC;  // PC Mac IBM = 1 2 3
   if(a->radioset[0] == 2) g.raw_platform = MAC;
   if(a->radioset[0] == 3) g.raw_platform = MVS;
   g.raw_packing   = a->radioset[2]-1;
   g.raw_signed    = a->radioset[3]-1;
   g.raw_xsize = atoi(a->answer[1][0]);
   g.raw_ysize = atoi(a->answer[2][0]);
   g.raw_skip  = atoi(a->answer[3][0]);
   g.raw_frames= atoi(a->answer[4][0]);
   g.raw_bpp   = atoi(a->answer[6][0]);
   g.raw_rbpp  = atoi(a->answer[7][0]);
   g.raw_gbpp  = atoi(a->answer[8][0]);
   g.raw_bbpp  = atoi(a->answer[9][0]);
   g.raw_kbpp  = atoi(a->answer[10][0]);
   g.raw_want_increment = a->boxset[11];
   g.raw_inc_inc = atoi(a->answer[12][0]);
   g.raw_flipbits = a->boxset[13];    

   switch(g.raw_colortype)
   {  case GRAY: g.raw_want_color=0; g.read_cmyk=0; break;
      case INDEXED: g.raw_want_color=1; g.read_cmyk=0; break;
      case COLOR: g.raw_want_color=1; g.read_cmyk=0; break;
      case CMYK: g.raw_want_color=1; g.read_cmyk=1; break;
   }
   if(g.raw_colortype > INDEXED)
       g.raw_bpp = g.raw_rbpp + g.raw_gbpp + g.raw_bbpp + g.raw_kbpp;
   switch(type)
   {  case ASCII: break;
      case IMM: g.raw_bpp=1; g.raw_packing=TIF; g.raw_platform=0; g.raw_colortype=GRAY; break;
      case IMG: g.raw_bpp=8; g.raw_packing=TIF; g.raw_platform=0; g.raw_colortype=GRAY; break;
      case RAW: break;
   }
   do_read_img_file(filename, oldfilename, identifier, type, compress);
}


//--------------------------------------------------------------------------//
//  do_read_img_file                                                        //
//--------------------------------------------------------------------------//
void do_read_img_file(char *filename, char *oldfilename, char *identifier, 
     int  type, int compress)
{
   short int short_xsize, short_ysize;
   int done=0, f, i, i2, j, k, kk, mm, rr, gg ,bb, dbpp=0, ibpp=8, increment=0, 
       evenbits=0, bufsize, bytesperrow=0, size1, size2, noofcolors=1, status=OK;
   uint value,vvv;
   FILE *fp;
   double pfac;
   uchar *bufr, *bufr2, *buffer;
   char option[128], crud[256], *junk;
   char tempstring[256];
   char tempstring1[256];
   char tempstring2[256];
   int oread_cmyk = g.read_cmyk;
   int bytes = 0, junksize;

   g.raw_xsize = max(1, g.raw_xsize);
   g.raw_ysize = max(1, g.raw_ysize);
   if(g.raw_bpp==1 || g.raw_bpp ==15) ibpp = g.raw_bpp;
   else ibpp = min(32, 8*((g.raw_bpp+7)/8)); // bpp increased to next multiple of 8.
   if(g.raw_packing==NONE) g.raw_bpp = ibpp;
   pfac = 7.9999/(double)g.raw_bpp;
   bytesperrow = (int)(g.raw_xsize/pfac);

   ////  Allocate space

   g.tif_xsize=1.0;
   g.tif_ysize=1.0;
   noofcolors = ((7+g.raw_bpp)/8);
   size1   = noofcolors * g.raw_xsize + 10;
   size2   = g.off[max(g.bitsperpixel,ibpp)] * g.raw_xsize + 10;
   bufsize = max(size1, size2);      

   bufr   = new uchar[bufsize]; if(bufr==NULL){ message(g.nomemory,ERROR); return; }
   bufr2  = new uchar[bufsize]; if(bufr2==NULL){ delete[] bufr; message(g.nomemory,ERROR);return;}
   buffer = new uchar[bufsize+10];
   if(buffer==NULL)
   {   message(g.nomemory,ERROR);  
       delete[] bufr;
       delete[] bufr2;
       return; 
   }
   bufr[bufsize-1]  = 99;
   bufr2[bufsize-1] = 99;
   buffer[bufsize+9]= 99;

   ////  Read image 

   if(type==ASCII) strcpy(option,"r");
   else strcpy(option,"rb");
   if ((fp=fopen(filename, option)) == NULL)
   {   error_message(filename, errno);
       return;
   }
   switch(type)
   {  case ASCII:
         fscanf(fp, "%16s %16s", tempstring1, tempstring2);         
         if(strcspn(tempstring1, " +-0123456789") || strcspn(tempstring2, " +-0123456789")) 
         {    sprintf(tempstring,"Invalid x and y dimensions in ASCII file:\nx=%s\ny=%s",
                   tempstring1, tempstring2);
              message(tempstring);
              fclose(fp);
              delete[] bufr;
              delete[] bufr2;
              return; 
         }
         sscanf(tempstring1,"%hd",&short_xsize);   // %hd = short int
         sscanf(tempstring2,"%hd", &short_ysize);   // %hd = short int
         g.raw_xsize = short_xsize;
         g.raw_ysize = short_ysize;
         break;
      case IMG:
         g.raw_xsize = getword(fp);
         g.raw_ysize = getword(fp);
         break;
      case IMM:
         g.raw_xsize = getword(fp);
         g.raw_ysize = getword(fp);
         evenbits = ((g.raw_xsize*g.raw_bpp + 7)/8)*8; 
         while(g.raw_xsize*g.raw_bpp < evenbits) g.raw_xsize++;
         break;
      case RAW:
         break;
   }
   
   //// Allocate image buffer

   if(newimage(basefilename(filename), g.tif_xoffset + 1, g.tif_yoffset,
        (int)(0.99999 + g.raw_xsize*g.tif_xsize), 
        (int)(0.99999 + g.raw_ysize*g.tif_ysize), 
        g.raw_bpp, g.raw_colortype, g.raw_frames, g.want_shell, 1, PERM, 1,
        g.window_border, 0) !=OK ) 
   {  message(g.nomemory,ERROR);  
      delete[] bufr;
      delete[] bufr2;
      delete[] buffer;
      return; 
   }

   junksize = sizeof(int) * max(g.raw_skip, g.tif_skipbytes);
   junk = new char[junksize];
   if(type!=ASCII && g.tif_skipbytes) fread(&junk[0],1,g.tif_skipbytes,fp);
   if(g.raw_skip) 
   {   bytes = fread(&junk[0],1,g.raw_skip,fp);
       if(bytes != g.raw_skip) message("Error reading skip bytes");
   }
   free(junk);

   increment=0;
   if(g.raw_platform==1) g.invertbytes=1; else g.invertbytes=0;
   if(type==ASCII)
   {   for(f=0; f<g.raw_frames; f++)
       for(j=0; j<g.raw_ysize; j++)   
       {  for(i=0,i2=0; i<g.raw_xsize; i++,i2+=g.off[ibpp])   
          {   
              fscanf(fp,"%s",tempstring1);
              vvv=atoi(tempstring1);
              vvv = min(vvv, (uint)g.maxvalue[ibpp]);
              putpixelbytes(z[ci].image[f][j]+i2, vvv, 1, ibpp, 1);
              if(feof(fp)){ done=1; break; }
              if(done) break;
          }
          if(done) break;
       }
   }else 
   for(f=0; f<g.raw_frames; f++)
   { 
     for(j=0; j<g.raw_ysize; j++)   
     { if(g.raw_want_increment) increment += g.raw_inc_inc;
       if(increment) for(k=0; k<increment; k++) fread(crud, noofcolors, 1, fp);   
       fread(buffer, bytesperrow, 1, fp);   
       if(g.raw_flipbits) for(k=0; k<bytesperrow; k++) buffer[k] = bitflip(buffer[k]);
       if(feof(fp)) break;
       getbits(0, buffer, 0, RESET);     

       for(i=0,i2=0; i<g.raw_xsize; i++,i2+=g.off[ibpp])   
       {  if(g.raw_colortype <= INDEXED)
          {   value = getbits(g.raw_bpp, buffer, bytesperrow, g.raw_packing);
              if(g.raw_bpp<=8)                        // Filter thru current palette
              {  if(g.raw_bpp==1) 
                 {    if(value) value=g.maxcolor; 
                 }else
                 {  dbpp = 8-g.raw_bpp;               //  if it is <= 8 bits/pixel
                    if(dbpp>0) value <<= ( dbpp); else value >>= (-dbpp);
                    if(ibpp>8)                        // Convert to grayscale 
                    {  valuetoRGB(value,rr,gg,bb,8);  // Use current palette           
                       value = RGBvalue(rr,gg,bb,ibpp);
                    }   
                 }   
              }else
              {   dbpp = ibpp-g.raw_bpp;              // If >8 bits/pixel, don't use
                  if(dbpp>0) value <<= ( dbpp);       //   palette. 
                  if(dbpp<0) value >>= (-dbpp);
               }
          }else
          {  rr = getbits(g.raw_rbpp, buffer, bytesperrow, g.raw_packing);
             gg = getbits(g.raw_gbpp, buffer, bytesperrow, g.raw_packing);
             bb = getbits(g.raw_bbpp, buffer, bytesperrow, g.raw_packing);
             if(g.raw_colortype == CMYK)
                 kk = getbits(g.raw_kbpp, buffer, bytesperrow, g.raw_packing);
             else 
                 kk = 0;
             dbpp = g.redbpp[ibpp]  - g.raw_rbpp;
             if(dbpp>0) rr <<= ( dbpp); else  rr >>= (-dbpp);
             dbpp = g.greenbpp[ibpp]- g.raw_gbpp;    
             if(dbpp>0) gg <<= ( dbpp); else  gg >>= (-dbpp);
             dbpp = g.bluebpp[ibpp] - g.raw_bbpp;  
             if(dbpp>0) bb <<= ( dbpp); else  bb >>= (-dbpp);
             dbpp = g.redbpp[ibpp]  - g.raw_kbpp;  
             if(dbpp>0) kk <<=( dbpp); else kk >>= (-dbpp);
             rr = max(0,min(rr, g.maxred[ibpp]));
             gg = max(0,min(gg, g.maxred[ibpp]));
             bb = max(0,min(bb, g.maxred[ibpp]));
             kk = max(0,min(kk, g.maxred[ibpp]));
             if(g.read_cmyk)
             {  mm = g.maxred[ibpp]; 
                rr = max(0,min(mm,mm-rr-kk));
                mm = g.maxgreen[ibpp]; 
                gg = max(0,min(mm,mm-gg-kk));
                mm = g.maxblue[ibpp]; 
                bb = max(0,min(mm,mm-bb-kk));
             }
             value = RGBvalue(rr,gg,bb,ibpp);   
          }                                  
          putpixelbytes(z[ci].image[f][j]+i2,value,1,ibpp,1);
       }
    }
   }
   fclose(fp);
   delete[] bufr;
   delete[] bufr2;
   delete[] buffer;
   initialize_image(filename, oldfilename, ci, type, compress, 
                    g.raw_signed, identifier, status);
   g.read_cmyk = oread_cmyk;
}



//--------------------------------------------------------------------------//
//  readimgfilefinish                                                       //
//--------------------------------------------------------------------------//
void readimgfilefinish(dialoginfo *a)
{
   a=a;   
}


//--------------------------------------------------------------------------//
// read3Dfile                                                               //
//--------------------------------------------------------------------------//
int read3Dfile(char *filename)
{  
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(ABORT); }
   static Dialog *dialog;
   dialog = new Dialog;
   if(dialog==NULL){ message(g.nomemory); return(NOMEM); }

   sprintf(dialog->answer[0][0], "%d", g.read3d_frames);
   sprintf(dialog->answer[1][0], "%d", g.read3d_xsize);
   sprintf(dialog->answer[2][0], "%d", g.read3d_ysize);
   sprintf(dialog->answer[3][0], "%d", g.read3d_bpp);
   sprintf(dialog->answer[4][0], "%d", g.read3d_skipbytes);

   strcpy(dialog->title,"3D Parameters");
   strcpy(dialog->boxes[0],"No. of frames");
   strcpy(dialog->boxes[1],"Frame width (pixels)");
   strcpy(dialog->boxes[2],"Frame height (pixels)");
   strcpy(dialog->boxes[3],"Bits/pixel");
   strcpy(dialog->boxes[4],"Bytes to skip");
   dialog->boxtype[0]=STRING; 
   dialog->boxtype[1]=STRING; 
   dialog->boxtype[2]=STRING; 
   dialog->boxtype[3]=STRING; 
   dialog->boxtype[4]=STRING; 

   dialog->noofradios=0;
   dialog->radiono[0]=0;
   dialog->noofboxes=5;
   dialog->helptopic=37;
   dialog->want_changecicb = 0;
   dialog->f1 = read3dfilecheck;
   dialog->f2 = null;
   dialog->f3 = null;
   dialog->f4 = read3dfilefinish;
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
   strcpy(dialog->path,".");
   dialog->ptr[0] = filename;
   dialog->message[0] = 0;      
   dialog->busy = 0;
   dialogbox(dialog); 
   return OK;
}
 

//--------------------------------------------------------------------------//
//  read3dfilecheck                                                         //
//--------------------------------------------------------------------------//
void read3dfilecheck(dialoginfo *a, int radio, int box, int boxbutton)
{
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return; }
   FILE *fp;
   int bytesperscanline,i,j,noofcolors,colortype;
   char *filename;
   radio=radio; box=box; boxbutton=boxbutton;
   if(radio != -2) return;  //// User clicked Ok or Enter

   filename = (char*)a->ptr[0];
   g.read3d_frames = atoi(a->answer[0][0]);
   g.read3d_xsize = atoi(a->answer[1][0]);
   g.read3d_ysize = atoi(a->answer[2][0]);
   g.read3d_bpp = atoi(a->answer[3][0]);
   g.read3d_skipbytes = atoi(a->answer[4][0]);
   noofcolors = (7+g.read3d_bpp)/8;
   if(noofcolors>1) colortype=COLOR; else colortype=INDEXED;

   if(newimage("Image", 0, 0, g.read3d_xsize, g.read3d_ysize, g.read3d_bpp,
       colortype, g.read3d_frames, g.want_shell, 1, PERM, 
       g.window_border,1,0)!=OK)  return;

   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return;
   }
   if(g.read3d_skipbytes) fseek(fp, g.read3d_skipbytes, SEEK_SET);
   bytesperscanline = g.read3d_xsize*((g.read3d_bpp+7)/8);
   for(i=0;i<g.read3d_frames;i++)
   for(j=0;j<g.read3d_ysize;j++)
        fread(z[ci].image[i][j],bytesperscanline,1,fp);
   fclose(fp);
}


//--------------------------------------------------------------------------//
//  read3dfilefinish                                                        //
//--------------------------------------------------------------------------//
void read3dfilefinish(dialoginfo *a)
{
   delete[] a;
}


//--------------------------------------------------------------------------//
// swap_image_bytes - swap bytes for entire image                           //
//--------------------------------------------------------------------------//
void swap_image_bytes(int ino)
{ 
  int f,j,length;
  if(!between(ino, 0, g.image_count-1)) return;
  if(z[ino].bpp <=8) return;
  length = z[ino].xsize * g.off[z[ino].bpp];
  if(g.autoundo && !z[ino].backedup) backupimage(ino,0);

  for(f=0;f<z[ino].frames;f++)
  for(j=0;j<z[ino].ysize;j++)
  {
      swapbytes(z[ino].image[f][j], length, g.off[z[ino].bpp]);
      if(z[ino].backedup)
          swapbytes(z[ino].backup[f][j], length, g.off[z[ino].bpp]);
  }
  repair(ino);
}


//--------------------------------------------------------------------------//
// swap_pixel_bytes - fix pixel bytes for a pixel (for big endian)          //
//--------------------------------------------------------------------------//
int swap_pixel_bytes(int value, int bpp)
{
  uchar *ptr = (uchar*)&value;
  switch(bpp)
  {   case 7:
      case 8:  swap(ptr[3],ptr[1]);
               value &= 0xff;
               break;
      case 15:
      case 16: value>>=8;
               value &=0xffff;
               swap(ptr[3],ptr[2]); 
               break;
      case 24: swapbytes(ptr, 3, 3); break;
      case 32: swapbytes(ptr, 4, 4);
               value >>= 8;
               break;
      case 48: swapbytes(ptr, 6, 6);
               value >>= 8;
               break;
      case 64: swapbytes(ptr, 8, 8); break;
      default: break;
  }
  return value;
}


//--------------------------------------------------------------------------//
// swapbytes                                                                //
// reverses the order of bytes  n=no. of bytes in array,size=bytes at a time//
//--------------------------------------------------------------------------//
void swapbytes(uchar* a, int n, int size)
{
   int k;
   switch(size)
   {   case 1: return;           // Too small, throw it back
       case 2:
          for(k=0;k<n-1;k+=2)    // For 16 bit ints
             swap(a[k],a[k+1]);
          break;
       case 3: 
          for(k=0;k<n-2;k+=3)    // For 24 bit pixels
               swap(a[k], a[k+2]);
          break;
       case 4:
          for(k=0;k<=n-4;k+=4)    // For 32 bit ints
          {     
                swap(a[k],a[k+3]);
#ifndef LITTLE_ENDIAN
                a[k+3] = 0;
#endif
                swap(a[k+1],a[k+2]);
          }
          break;
       case 6:
          for(k=0;k<=n-6;k+=6)    // For 48 bit ints
          {     swap(a[k],a[k+5]);
                swap(a[k+1],a[k+4]);
                swap(a[k+2],a[k+3]);
          } 
          break;
       case 8:
          for(k=0;k<=n-8;k+=8)    // For 64 bit ints
          {     swap(a[k],a[k+7]);
                swap(a[k+1],a[k+6]);
                swap(a[k+2],a[k+5]);
                swap(a[k+3],a[k+4]);
          } 
          break;
       default:
          fprintf(stderr,"Error in swapbytes (size=%d)\n",size);
   }
}


//--------------------------------------------------------------------------//
// writeimgfile                                                             //
// Save screen image on disk.                                               //
// This format consists of 4 bytes that give x & y size, followed by the    //
// packed data in a raster sequence. Since there is no way to specify       //
// bits/pixel, only 1 and 8 are supported.                                  //
// (Frame grabber format - don't confuse with GEM)                          //
//--------------------------------------------------------------------------//
int writeimgfile(char *filename, int write_all, int compress)
{
  FILE *fp;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
  int ino,bpp,i,j,s,xsize,count;
  int x1,y1,x2,y2;
  
  
  if(write_all==1)
  {   x1 = z[ci].xpos;
      y1 = z[ci].ypos;
      x2 =  x1 + z[ci].xsize;
      y2 =  y1 + z[ci].ysize;
  }else
  {   x1 = g.selected_ulx;
      x2 = g.selected_lrx;
      y1 = g.selected_uly;
      y2 = g.selected_lry;
  }

  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE;
  
  if(g.want_color_type == 0)
  {  message("Saving as monochrome!"); 
     xsize = ((x2-x1+7)>>3)<<3;
  }else 
     xsize = x2-x1;
 
  putword(xsize,fp); 
  s = y2-y1;
  putword(s,fp); 

  ////  Write image data    

  for(j=y1;j<y2;j++)
  {   if(g.want_color_type == 0)
      {   for(i=x1,count=0;count<(xsize>>3);i+=8)
          {  count++;
             s = readbyte(i,j); 
             putbyte(s,fp);
          }
      }else
      { if(write_all)
          fwrite(z[ci].image[z[ci].cf][j-y1],g.off[g.want_bpp]*xsize,1,fp);
        else
          for(i=x1;i<x2;i++)
          {  s = readpixelonimage(i,j,bpp,ino); 
             fwrite(&s,g.off[g.want_bpp],1,fp);
          }
      }
  }

  close_file(fp, compress);  
  return OK;
}


//--------------------------------------------------------------------------//
// writefft                                                                 //
// Save fft in ASCII format.                                                //
//--------------------------------------------------------------------------//
int writefft(int ino)
{
  static PromptStruct ps;
  int status=YES;
  if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
  static char fftfile[FILENAMELENGTH]="fft.ascii";
  if(!z[ino].floatexists)
  {   message("No FFT exists for this image",ERROR);
      return(BADPARAMETERS);
  }
  status = message("Filename:",fftfile,PROMPT,FILENAMELENGTH-1,54);
  if(status != YES) return ABORT;
  ps.filename = fftfile;
  ps.f1 = writefft_part2;
  ps.f2 = null;
  ps.ino = ino;
  check_overwrite_file(&ps);
  return OK;
}


//--------------------------------------------------------------------------//
// writefft_part2                                                           //
//--------------------------------------------------------------------------//
void writefft_part2(PromptStruct *ps)
{
  FILE *fp;
  char *fftfile = ps->filename;
  int i,j;
  int status=OK;
  int size;
  int ino = ps->ino;
  if ((fp=fopen(fftfile, "w")) == NULL)
  {   error_message(fftfile, errno);
      return;
  }

  //// In case they close image while typing filename
  if(!between(ino, 0, g.image_count-1)){ message("Can't find image");  return; }
  if(!z[ino].floatexists)
  {   message("No FFT exists for this image",ERROR);
      return;
  }

  fprintf(fp,"FFT of %s\n",z[ino].name);  
  fprintf(fp,"xsize %d\n", z[ino].xsize);  
  fprintf(fp,"ysize %d\n", z[ino].ysize);  
  fprintf(fp,"Real\n");  
  for(j=0;j<z[ino].fftysize;j++)
  {   for(i=0;i<z[ino].fftxsize;i++)
         fprintf(fp,"%g ",z[ino].fft[j][i].real());
      fprintf(fp,"\n");  
  }
  fprintf(fp,"\n\nImaginary\n");  
  for(j=0;j<z[ino].fftysize;j++)
  {   for(i=0;i<z[ino].fftxsize;i++)
         fprintf(fp,"%g ",z[ino].fft[j][i].imag());
      fprintf(fp,"\n");  
  }

  fclose(fp);
  size = filesize(fftfile);              // Check to ensure not an empty file
  if(size==0) status=ZEROLENGTH; 
  if(access(fftfile,F_OK)) status=ERROR; // access = 0 if file exists
  if(status==OK) message("FFT saved");
  else  message("Error saving FFT",ERROR);
  return;
}


//--------------------------------------------------------------------------//
// readfft                                                                  //
// Read fft in ASCII format.                                                //
//--------------------------------------------------------------------------//
int readfft(int noofargs, char **arg)
{
   static char *filename=NULL;
   char temp1[256];
   char temp2[256];
   char temp3[256];
   FILE *fp;
   ////  Leave this allocated
   if(filename==NULL){ filename = new char[FILENAMELENGTH]; filename[0]=0; }
   char oldfilename[FILENAMELENGTH];
   if(strlen(filename)) strcpy(oldfilename, filename);
   int frame, i, j, status=OK, xsize, ysize;
   
   if(noofargs == 0)
      strcpy(filename, getfilename(oldfilename, NULL));
   else 
      strcpy(filename, arg[1]);
      
   if((fp=fopen(filename, "r")) == NULL)
   {   error_message(filename, errno);
       return(ERROR);
   }
   
   // Always use field width when reading strings with fscanf to avoid
   // buffer overflow.
   
   fscanf(fp,"%3s %2s ",temp1, temp2);          // 'FFT of'
   if(strcmp(temp1,"FFT") != SAME){ message("Invalid FFT file",ERROR); return ERROR; }
   fscanf(fp,"%256s",temp3);                    // [filename]
   fscanf(fp,"%256s %d",temp1, &xsize);         // xsize 12345
   fscanf(fp,"%256s %d",temp1, &ysize);         // ysize 12345   
   fscanf(fp,"%256s",temp1);                    // Real
   
   if(newimage("FFT image",0,0,xsize,ysize,g.bitsperpixel,GRAY,1,g.want_shell,1,PERM,1,
      g.window_border,0)!=OK) return(NOMEM); 
   if(setupfft(ci,xsize,ysize)!=OK)
   {  message("Unable to create FFT matrix!",ERROR); 
      return ERROR;
   }
   z[ci].fftstate=REAL;
   setimagetitle(ci, temp3);
   for(j=0;j<ysize;j++)
   for(i=0;i<xsize;i++)
      if(!fscanf(fp,"%lg",&z[ci].fft[j][i].real())){ status=ERROR; break; }

   fscanf(fp,"%256s",temp1);                    // Imaginary
   for(j=0;j<ysize;j++)
   for(i=0;i<xsize;i++)
      if(!fscanf(fp,"%lg",&z[ci].fft[j][i].imag())){ status=ERROR; break; }

   if(status==OK)
   {  z[ci].fftdisplay=REAL;                    // Wait here in case user aborts
      scalefft(ci);
      rebuild_display(ci);
                                                // Map fft gray values to image[]
      frame = z[ci].cf;
      for(j=0;j<z[ci].ysize;j++)
         memcpy(z[ci].image[frame][j], z[ci].img[j], 
            z[ci].xsize * g.off[z[ci].bpp]);

      switchto(ci);
      redrawscreen();
   }else message("Error reading FFT",ERROR);
   fclose(fp);
   return status;
}

//--------------------------------------------------------------------------//
// readgemfile                                                              //
// Transfer GEM IMG file from disk to screen.                               //
//--------------------------------------------------------------------------//
int readgemfile(char *filename,int subtype)
{  
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return(NOMEM); }
   short uint bpp;
   FILE *fp;
   short uint version,headerlength,patternlength,xmicrons,ymicrons,flag;
   int x,y,count=0,f;
   int c,c2,repeat;
   uchar *buffer;
   uchar *buf2;
   int mbpp;
   short uint xsize=0,ysize=0;
   int col=1,color=1;
   char junk; 
   int i,j,k,len;

   g.tif_xsize=1;    // Size reduction disabled 
   g.tif_ysize=1;   

   len = filesize(filename);
   if ((fp=fopen(filename, "rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }

   /* Don't confuse tif_xsize, which is desired magnification (0-1), with */
   /* xsize, which is the no. of pixels across.                           */

   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);

   tif_fread(&version,      2,fp,1,2);
   tif_fread(&headerlength, 2,fp,1,2);
   tif_fread(&bpp,          2,fp,1,2);
   switch(bpp)                //This field is sometimes trashed
   { case 1: case 2: case 4: case 8: case 16: case 24: case 32:
             break;
     default: bpp=1; col=255; break;
   }   
   if(subtype==GEM) mbpp=8;
   mbpp = 8*((7+bpp)/8);
   tif_fread(&patternlength,2,fp,1,2);
   if(patternlength>32) patternlength=2;
   tif_fread(&xmicrons,     2,fp,1,2);
   tif_fread(&ymicrons,     2,fp,1,2);
   tif_fread(&xsize,        2,fp,1,2);
   tif_fread(&ysize,        2,fp,1,2);
   if(headerlength==9) tif_fread(&flag,2,fp,1,2);
   while(xsize%(8/bpp)) xsize++;
   buffer = new uchar[len+4];
   if(buffer==NULL){  fclose(fp); return(NOMEM);}
   buf2   = new uchar[xsize];
   if(buf2==NULL){ delete[] buffer; fclose(fp); return(NOMEM);}
   
     // Make an 8,16,24, or 32 bpp image buffer

   if(newimage(basefilename(filename), g.tif_xoffset+1, g.tif_yoffset, 
        (int)(0.99999 + (xsize)*g.tif_ysize), 
        (int)(0.99999 + (ysize)*g.tif_xsize), mbpp,INDEXED,1,g.want_shell,1,PERM,1,
        g.window_border,0)!=OK)
   {  fclose(fp); 
      delete[] buffer;
      delete[] buf2;
      return(NOMEM);
   }

   z[ci].originalbpp = bpp;
   f = 0;
   for(i=0;i<ysize;i++)  
   for(j=0;j<xsize;j++)  
     z[ci].image[f][i][j]=0;

   y=0; x=0; count=0; repeat=0;

   int count2=5;
   int xlimit;
   if(subtype==IMDS)
   {                                  
     xsize--;
     xlimit = xsize*g.off[mbpp];
     fseek(fp,g.tif_skipbytes+32,SEEK_SET);
     fread(buffer,len,1,fp);        // read entire image
     while(count<len)
     {  c = buffer[count++]; 
        count2++;
        if(count2==4092){ count2=0; count+=4; }
        for(j=7;j>=0;j--)
        {  if(c & (1<<j)) putpixelbytes(z[ci].image[f][y]+x,g.maxcolor,1,mbpp);
           x+=g.off[mbpp];
        }   
        if(x>xlimit){ x=0;y++; }
        if(y>=ysize)break;
     }
  
   }else                            // subtype = GEM - must be 8bpp
   { 
     fread(buffer,len,1,fp);        // read entire image
     while(count<len)
     {  
      c = buffer[count++]; 
      switch(c)
      {  case 0:
            c = buffer[count++];
            if(c==0x00)
            {  if(buffer[count++]==0xff)     // vertical replication code
                 repeat = buffer[count++];   // no.times to repeat scan line
            }else                            // pattern code of length patternlength
            {                                // pattern code - repeated c times 
               for(k=0;k<patternlength;k++)
                  buf2[k] = buffer[count++]; // get pattern                
               for(k=0;k<c;k++)              // repeat pattern
               {  for(i=0;i<patternlength;i++)  
                  { for(j=7;j>=0;j--) 
                    {  if(buf2[i] & (1<<j)) z[ci].image[f][y][x] = 0;
                       else z[ci].image[f][y][x] += col;
                       x++;
                    }   
                  }  
               }         
            }        
            break;                      
         case 0x80:                        // literal run next c bytes 
            c = buffer[count++];           // no of bytes to take literally
            for(k=0;k<c;k++) 
            {  c2 = buffer[count++];       // get byte
               for(j=7;j>=0;j--)
               {  if(c2 & (1<<j)) z[ci].image[f][y][x] = 0;
	          else z[ci].image[f][y][x] += col;
                  x++; 
                }   
            }       
            break;
         default:
            if(c & 0x80)                   // next 8*(c&0x7f)pixels are black
               x += 8*(c&0x7f);
            else                           // next 8*(c&0x7f)pixels are white
               for(k=0;k<8*(c&0x7f);k++) z[ci].image[f][y][x++] += (int)col; 
            break;
      }        
       
      if(x>=xsize)
      {     x=0; 
            color++; 
            if(bpp==1) col=255;
            else       col = (1<<(color-1)) * (256/(1<<bpp));      
            if(col>=256)col=255;
      }
      if(color>bpp)
      {    color=1;
           if(bpp==1) col=255;
           else       col = (1<<(color-1)) * (256/(1<<bpp));      
           if(col>=256)col=255;
           x=0; y++; 
           if(y<ysize)
           {  for(j=0;j<repeat;j++)
              {  
                for(k=0;k<=xsize;k++) 
		{    z[ci].image[f][y][x] = z[ci].image[f][y-1][x];
                     x++;
                }
                y++;
                x=0;
                if(y>=ysize) break;
              }    
              repeat=0;
           } 
      }
      if(y>=ysize) break;
     }
   }  
   delete[] buf2;
   delete[] buffer;
   fclose(fp);
   return(OK);
}


//--------------------------------------------------------------------------//
// readtextfile                                                             //
//--------------------------------------------------------------------------//
int readtextfile(char *filename)
{  
   FILE *fp;
   int len,h,direction,ascent,descent,x,y;   
   XCharStruct overall;
   char string[1024];
   if ((fp=fopen(filename,"r")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }

   message("Click on position to put text");
   getpoint(x,y);
   string[1] = 0;
   while(fgets(string, 1023, fp))
   {
      if(g.getout) break;
      len = strlen(string);
      XTextExtents(g.image_font_struct,string,len,&direction,&ascent,&descent,&overall);
      h = g.image_font_struct->ascent + g.image_font_struct->descent;
      remove_terminal_cr(string);
      print(string,x,y,g.fcolor,g.bcolor,&g.image_gc,0,0,0,0,0); 
      y += h;
   }
   fclose(fp);
   return OK;
}


//--------------------------------------------------------------------------//
// readbioradfile                                                           //
//--------------------------------------------------------------------------//
int readbioradfile(char *filename)
{  
   static int hit = 0;
   FILE *fp;
   char s[20];
   int i,j,k,found=0,byte,pos=0,y,bytes_per_line,bpp,byte_order=0;   
   int xsize, ysize;
   if ((fp=fopen(filename,"r")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }
   if(!hit) message("This Biorad format reader is still experimental.\nIf image does not read correctly,\ntry setting swap bytes."); 
   hit=1;

   while(!found && pos<100000)
   {   pos++;
       fread(&byte,1,1,fp);
       for(k=0;k<12;k++) s[k] = s[k+1];
       s[11] = byte;
       s[12] = 0;
       if(!strcmp(s,"Transparency")){ found=1;}
   }

   pos += 177;
   fseek(fp, pos, SEEK_SET);
   fread(&xsize,2,1,fp); 
   fread(&ysize,2,1,fp); 
   xsize &= 0xffff;
   ysize &= 0xffff;
   bpp = 16;
   if(xsize>0xffff || ysize>0xffff){message("Bad Bio-Rad file"); return ERROR;}
   pos += 4 ;
   fseek(fp, pos+1200, SEEK_SET); // skip 206 more bytes
  
   if(newimage(basefilename(filename), 0, 0, xsize, ysize, bpp, GRAY, 
        1,       // frames
        g.want_shell, 1,
        PERM, 0,
        g.window_border, 0)!=OK) { fclose(fp); return(NOMEM); }

   bytes_per_line = 2 * xsize;
   for(y=ysize-1; y>=0; y--)
       fread(&z[ci].image[0][y][0], bytes_per_line, 1, fp);
   fclose(fp);

   //// Find out if lower or higher byte changes faster
   //// File format is undocumented
   uchar l[10000], u[10000];
   int lmin=255, lmax=0, umin=255, umax=0;
   if(xsize>100 && ysize>100)
   {
        i=0;
        for(j=50;j<100;j++)
        for(k=50;k<100;k++)
        {    l[i] = z[ci].image[0][j][k*2];
             u[i] = z[ci].image[0][j][k*1];
             i++;
        }
        for(i=0;i<2500;i++)
        {    if(l[i] > lmax) lmax = l[i];
             if(u[i] > umax) umax = u[i];
             if(l[i] < lmin) lmin = l[i];
             if(u[i] < umin) umin = u[i];
        }
        if(lmax-lmin < umax-umin) byte_order=0; else byte_order=1;
   }
   if(byte_order){ swap_image_bytes(ci);}
   
   return OK;
}



//--------------------------------------------------------------------------//
// writemultiframefile                                                      //
//--------------------------------------------------------------------------//
int writemultiframefile(char *filename, int compress)
{
  int bpp,f,i,i2,j,k,v;
  FILE *fp;
  char tempstring[1024];
  if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
      g.compression_ext))==NULL) 
      return CANTCREATEFILE; 

  bpp = z[ci].bpp;
  memset(tempstring, 32, 1024);
  sprintf(tempstring, "%s %s %d %d %d %d %d\n", 
       "Multiframe",
       z[ci].name, 
       z[ci].xsize,
       z[ci].ysize,
       z[ci].bpp,
       z[ci].frames,
       z[ci].colortype);
  // first 1024 bytes are reserved for more information
  // get rid of \0 added by sprintf
  for(k=0;k<1024;k++) if(tempstring[k]==0) tempstring[k]=32;
  fwrite(tempstring, 1024, 1, fp);  
  fwrite(z[ci].palette, 768, 1, fp);
  for(f=0;f<z[ci].frames;f++)
  for(j=0;j<z[ci].ysize;j++)
  {  
     for(i=0,i2=0; i<z[ci].xsize; i++,i2+=g.off[bpp])
     {   v = pixelat(z[ci].image[f][j]+i2, bpp);
         fwrite(&v, sizeof(int), 1, fp);  
     }
  }     
  close_file(fp, compress);  
  return OK;
}




//--------------------------------------------------------------------------//
//  readmultiframefile                                                      //
//--------------------------------------------------------------------------//
int readmultiframefile(char *filename)
{
   if(memorylessthan(16384)){ message(g.nomemory,ERROR); return NOMEM; }
   FILE *fp;
   int i,i2,f,j,colortype,v;
   int bpp, frames, xsize, ysize;
   char tempstring[1024];
   char identifier[1024];
   char name[1024];
   uchar *add;

   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }

   fread(tempstring, 1024, 1, fp);
   sscanf(tempstring,"%s %s %d %d %d %d %d", 
          identifier,
          name,
          &xsize,
          &ysize,
          &bpp,
          &frames,
          &colortype);

   if(newimage("Image", 0, 0, xsize, ysize, bpp,
       colortype, frames, g.want_shell, 1, PERM, 
       g.window_border,1,0) != OK){ fclose(fp);  return NOMEM; }


  fread(z[ci].palette, 768, 1, fp);
  for(f=0;f<z[ci].frames;f++)
  for(j=0;j<z[ci].ysize;j++)
  {  
     for(i=0,i2=0; i<z[ci].xsize; i++,i2+=g.off[bpp])
     {   fread(&v, sizeof(int), 1, fp);  
         add = z[ci].image[f][j]+i2; 
         putpixelbytes(add, v, 1, bpp, 1);
     }
  }     
  fclose(fp);
  return OK;
}
