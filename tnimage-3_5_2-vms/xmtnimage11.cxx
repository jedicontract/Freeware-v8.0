//--------------------------------------------------------------------------//
// xmtnimage11.cc                                                           //
// Reading & writing TIF files                                              //
// Latest revision: 03-05-2006                                              //
// Copyright (C) 2006 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern int         ci;                 // current image
extern Image      *z;
extern Globals     g;
#include "tiffio.h"                  // For libtiff version

//------Variables for writing TIF files-------//

taginfo *tag;
short int totaltags;
int stripline;
int currentstrip;
int image_quant=0;
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
// readtiffile - all this junk just figure out which tiff routine to use.   //
//--------------------------------------------------------------------------//
int readtiffile(char *filename, char *actual_filename)
{
   FILE *fp;
                              // Up to 16 colors with different bits/pixels
   int cbps[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
   int cmyk=0,k;
   int mac=0;                         //  =1 if a Macintosh tif file
   image_quant=0;
   short int tif_byteorder=0;
   short int tif_version=0;
   short int tif_noofdirectories=0;
   int tif_offsettoIFD=0;
   short int tif_tag=0;
   short int tif_type=0;
   char *tif_string;
   int tif_length       =0;
   int tif_value        =0;

   int tif_imagewidth   =0;
   int tif_imagelength  =0;
   int tif_bitspersample=8;
   int tif_compression  =1;
   int tif_photointerp  =1;
   int tif_sampperpixel =0;
   short int second_image=0;       
   int tif_fillorder    =1;
   int bytestouse       =4;
   int tif_value_bps    =1;
   int bps=1;
   char junk;
       
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }
   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   tif_string = new char[1024];

   fread(&tif_byteorder,2,1,fp);  // This must be the same in all Tif files
   if(tif_byteorder==0x4d4d) mac=1; else mac=0;
   tif_fread(&tif_version,2,fp,mac,2); 
   tif_fread(&tif_offsettoIFD,4,fp,mac,4);

   fseek(fp, g.tif_skipbytes+tif_offsettoIFD, SEEK_SET);
   tif_fread(&tif_noofdirectories,2,fp,mac,2);

   for(k=1;k<=tif_noofdirectories;k++) 
   {
       tif_tag = 0;
       tif_fread(&tif_tag,2,fp,mac,2);
       tif_fread(&tif_type,2,fp,mac,2);
       tif_fread(&tif_length,4,fp,mac,4);

            // Bytestouse are needed in case it is mac file
       if(tif_type==1) bytestouse = 1;
       if(tif_type==2) bytestouse = 1;
       if(tif_type==3) bytestouse = 2;
       if(tif_type==4) bytestouse = 4;
       if(tif_type==5) bytestouse = 4;
       if(tif_length>1)bytestouse = 4;

       tif_fread(&tif_value,4,fp,mac,bytestouse);    
       if(bytestouse==1) tif_value &= 0xff;
       if(bytestouse==2) tif_value &= 0xffff;
       if(!second_image)
       {  
          switch (tif_tag)
          {  case 256: tif_imagewidth   = tif_value; break;
             case 257: tif_imagelength  = tif_value; break;
             case 258: bps = tif_value;
                       tif_value_bps = bps;
                       break;
             case 259: tif_compression  = tif_value; break;
             case 262: tif_photointerp  = tif_value;
                       if(tif_photointerp==5) cmyk=1; else cmyk=0;
                       break;
             case 266: tif_fillorder    = tif_value; break;
             case 277: tif_sampperpixel = tif_value; break;
             default: break;
         }
       }         
   }

   ////  Now that we know the samples/pixel it is safe to guess at
   ////  what bps means.

   if((tif_value_bps!=0) && ((tif_sampperpixel>1)||(tif_value_bps>32)) )
   {  bps = 0;
      for(k=0; k<tif_sampperpixel; k++)
      {   cbps[k] = tif_value_at(tif_value_bps+k*2,2,fp,mac);
          bps += cbps[k];
      }
   }
   if(!between(bps, 0, 48))
   {   message("Bad bits/sample in TIFF file");  
       bps = tif_value_bps*tif_sampperpixel;
   }
   tif_bitspersample = bps;

   if(tif_sampperpixel<=0)
   { // message("Bad samples/pixel in TIFF file,\nusing 1"); 
      tif_sampperpixel=1; 
   }

   fclose(fp); // VMS  CHAPG  Error!

   if(
       //// attributes to use native code, specify with ==
       (tif_bitspersample == 16  ||                         // 16 bit
       tif_bitspersample ==  8  ||                         // 8 bit
       tif_compression <=1      ||                         // not compressed
       int(tif_bitspersample/8)*8 != tif_bitspersample ||  // not multiple of 8
       (tif_sampperpixel==1 && tif_bitspersample > 8) ||   // grayscale over 8
       !g.uselibtiff ) &&
       //// attributes to use libtiff, specify with !=
       (tif_compression   !=  4 )                         // type 4 compression
       )
         return readtiffile_general(filename, actual_filename);
   else
         return readtiffile_libtiff(filename, actual_filename);
}     
 


//--------------------------------------------------------------------------//
// readtiffile_libtiff                                                      //
// libtiff just crashes if it tries to read an unsupported type of tif      //
// file. So we have to read the file twice: once to find out if libtiff     //
// can handle it, and once to actually read it.                             //
// we mainly need libtiff to read LZW compressed tiffs. libtiff can't       //
// handle the vast majority of scientific tiffs.                            //
//--------------------------------------------------------------------------//
int readtiffile_libtiff(char *filename, char *actual_filename)
{
  TIFF *image;
  int bpp,bytes,x,y,rr,gg,bb,errors=0;
  uint16 photo, bps, spp, fillorder;
  uint32 width, height;
  tsize_t stripSize;
  unsigned long imageOffset, result;
  int stripMax, stripCount;
  char *buffer, tempbyte;
  unsigned long bufferSize, count;
  char tempstring[128];

  if((image = TIFFOpen(filename, "r")) == NULL)
  {   message("Could not open incoming image\n");
      return ERROR;
  }      
  if((TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) || (spp != 1)) errors++;
  if((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0) || (bps != 1)) errors++;
  bpp = bps * spp;
  stripSize = TIFFStripSize (image);
  stripMax = TIFFNumberOfStrips (image);
  imageOffset = 0;
  if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0) errors++;
  if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height) == 0) errors++;

  //// Libtiff wastes space here, uses buffer larger than needed
  //// to hold image. The only solution is to use a giant buffer
  //// for the data, then copy the data into the image buffer.
  //// This could be over 10 MB ...
  
  bufferSize = TIFFNumberOfStrips (image) * stripSize; // can be larger than x*y*bytes/pixel
  newimage(actual_filename, 0, 0, width, height, bpp, COLOR, 
      1, g.want_shell, 1, PERM, 1, g.window_border, 0);  
  if((buffer = (char *) malloc(bufferSize)) == NULL)
  {   message("Could not allocate enough memory\nfor the uncompressed image");
      TIFFClose(image);
      return NOMEM;
  }      
  for (stripCount = 0; stripCount < stripMax; stripCount++)
  {   if((result = TIFFReadEncodedStrip (image, stripCount,
          buffer + imageOffset, stripSize)) == (uint)-1) errors++;
      imageOffset += result;
  }
  if(TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &photo) == 0) errors++;
  if(photo != PHOTOMETRIC_MINISWHITE)// Flip bits
      for(count = 0; count < bufferSize; count++)
         buffer[count] = ~buffer[count];
  if(TIFFGetField(image, TIFFTAG_FILLORDER, &fillorder) == 0) errors++;
  if(fillorder == FILLORDER_MSB2LSB && bpp>8)// We need to swap bits -- ABCDEFGH becomes HGFEDCBA
  {   for(count = 0; count < bufferSize; count++)
      {    tempbyte = 0;
           if(buffer[count] & 128) tempbyte += 1;
           if(buffer[count] & 64) tempbyte += 2;
           if(buffer[count] & 32) tempbyte += 4;
           if(buffer[count] & 16) tempbyte += 8;
           if(buffer[count] & 8) tempbyte += 16;
           if(buffer[count] & 4) tempbyte += 32;
           if(buffer[count] & 2) tempbyte += 64;
           if(buffer[count] & 1) tempbyte += 128;
           buffer[count] = tempbyte;
      }
  }
  bytes = min(bufferSize,width*height*bpp/8);
  memcpy(z[ci].image_1d, buffer, bytes);

  //// Another abomination to fix the weird byte order in libtiff

  if(bpp>=24)   
  {    for(y=0; y<(int)height; y++)  
       for(x=0; x<(int)width; x++)  
       {    RGBat(z[ci].image[0][y]+3*x, bpp, rr, gg, bb); 
            swap(rr,bb);
            rr = 255-rr; gg = 255-gg; bb = 255-bb;
            putRGBbytes(z[ci].image[0][y]+3*x, rr, gg, bb, bpp);
        }
  } 
  free(buffer);
  TIFFClose(image);
  if(errors)
  {   sprintf(tempstring, "%d Errors reading TIFF file", errors);
      // message(tempstring); 
  }
  return OK;
}


//--------------------------------------------------------------------------//
// readtiffile_general                                                      //
// Non-libtiff method                                                       //
// Assumes 32-bit integers and 16-bit short integers.                       //
// Returns error code (OK if successful).                                   //
// Based on information in the document "TIFF Revision 6.0 Final Q June 3,  //
// 1992" from Aldus Corporation.                                            //
//--------------------------------------------------------------------------//
int readtiffile_general(char *filename, char *actual_filename)
{  


   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   FILE *fp;
   uchar *buffer=NULL;
   uchar color1,color2;
   int* yvalue=0;  

   char temp2[128];
   char tempstring[1024];
   int palette[3][256];
   int ct,cmyk=0,i,i2,j,k,k2,ok=1,y,vv,frame;
   int r1,g1,b1,a,value;
   uint color;
   int mac=0;                         //  =1 if a Macintosh tif file
   g.getout=0;
   int sizefac=1;
   int image_type=7;
   image_quant=0;
   int invertbytes = g.invertbytes;

   double pfac;
   short int tif_byteorder=0;
   short int tif_version=0;
   short int tif_noofdirectories=0;
   short unsigned int palettetemp=0;

   int tif_offsettoIFD=0;
   short int tif_tag=0;
   short int tif_type=0;
   int w=0, tile=0;
   int status=OK;
   double xfac,yfac;
   int ytimesyfac, kxfac,kyfac;
     // tif_xoffset = x pixel starting position

   char *tif_string;
   int tif_length       =0;
   int tif_value        =0;

   int tif_imagewidth   =0;
   int tif_imagelength  =0;
   int tif_bitspersample=8;
   int tif_compression  =1;
   int tif_photointerp  =1;
   int tif_sampperpixel =0;
   int tif_rowsperstrip =0;
   int tif_stripbytes   =0;
   int tif_planarconfig =1;
   int tif_grayresponse =0; 
   int tif_grayrespcurv =0;
   int tif_start        =0;
   int tif_integer      =0;
   int tif_colormap     =0;
   int tif_tilelength   =0;
   int tif_tilewidth    =0;
   int tif_tileoffsets  =0;
   int tif_tileoffsettype=0;
   

   short int second_image=0;       
   int tif_predictor    =1;
   int tiles_across     =1;
   int tiles_down       =1;
   int tilexstart       =0;
   int tileystart       =0;
   int tif_stripsperimage=0;
   int tif_fillorder    =1;
   int* tif_strip=NULL;       // File offset for start of each strip 
   int* bytesperstrip=NULL;   // No.of bytes in each strip
   int mbpp  = g.bitsperpixel;   // Bits/pixel to store image in memory 
   int strip            =0;   // Counter for strips     
   int nooftiles        =1;   // If not broken up, it is 1 big tile
   int pos;
   int bytestouse       =4;
   int tif_value_bps    =1;
   int plane            =0;
                              // Up to 16 colors with different bits/pixels
   int cbps[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
   int bps=1;
   int cc,mm,yy,kk,ts,tb;

   int bytestoread;
   int* tileoffset=NULL;      // File offset for start of each tile
   int xtile=1,ytile=1;       // indexes to keep track of location
   int otif_imagelength =0;
   int length;
   int alloc1=0,alloc2  =0;   // Flags if arrays allocated    
   int alloc3=0,alloc4  =0;   // Flags if arrays allocated    
   int alloc5           =0;
   int dbpp;                  // difference in bits/pixel 
   int count            =0;                 
   int shift            =0;   // Amount to shift 4-bpp images
   int bitmask          =0;  
   int highest          =0;
   int palette_factor   =1;
   char junk;
   uchar cf,rr,gg,bb;
   uchar rr2,gg2,bb2;
   int red, green, blue;
       
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }
   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   tif_string = new char[1024];

   fread(&tif_byteorder,2,1,fp);  // This must be the same in all Tif files
   if(tif_byteorder==0x4d4d) mac=1; else mac=0;
   tif_fread(&tif_version,2,fp,mac,2); 
   tif_fread(&tif_offsettoIFD,4,fp,mac,4);

   fseek(fp, g.tif_skipbytes+tif_offsettoIFD, SEEK_SET);
   tif_fread(&tif_noofdirectories,2,fp,mac,2);

   for(k=1;k<=tif_noofdirectories;k++) 
   {
       tif_tag = 0;
       tif_fread(&tif_tag,2,fp,mac,2);
       tif_fread(&tif_type,2,fp,mac,2);
       tif_fread(&tif_length,4,fp,mac,4);

            // Bytestouse are needed in case it is mac file
       if(tif_type==1) bytestouse = 1;
       if(tif_type==2) bytestouse = 1;
       if(tif_type==3) bytestouse = 2;
       if(tif_type==4) bytestouse = 4;
       if(tif_type==5) bytestouse = 4;
       if(tif_length>1)bytestouse = 4;

       tif_fread(&tif_value,4,fp,mac,bytestouse);    
       if(bytestouse==1) tif_value &= 0xff;
       if(bytestouse==2) tif_value &= 0xffff;
       if(!second_image)
       {  
          switch (tif_tag)
          {  case 256: tif_imagewidth   = tif_value; break;
             case 257: tif_imagelength  = tif_value; break;
             case 258: bps = tif_value;  // The meaning of this may
                                         // change depending on the
                                         // tif_sampperpixel, which could
                                         // be read later.
                       tif_value_bps = bps;
                       break;
             case 259: tif_compression  = tif_value; break;
             case 262: tif_photointerp  = tif_value;
                       if(tif_photointerp==5) cmyk=1; else cmyk=0;
                       break;
             case 266: tif_fillorder    = tif_value; break;
             case 273: if(!tif_start) tif_start = tif_value;
                       //  tif_stripoffsets = tif_value;
                       break;
             case 274: ////tif_orientation  = tif_value;
                       break;
             case 277: tif_sampperpixel = tif_value; break;
             case 278: tif_rowsperstrip = tif_value;
                       tif_stripsperimage = (tif_imagelength + 
                          tif_rowsperstrip - 1)/tif_rowsperstrip;
                       break;
             case 279:         // This value will be ignored if it turns
                               // out that there is only one strip.
                       tif_stripbytes   = tif_value;break;
             case 280: //// tif_minvalue     = tif_value;
                       break;
             case 281: //// tif_maxvalue     = tif_value;
                       break;
             case 282: //// tif_xres         = tif_value;
                       break;
             case 283: //// tif_yres         = tif_value;
                       break;
             case 284: tif_planarconfig = tif_value;break;
             case 290: tif_grayresponse = tif_value;break; 
             case 291: tif_grayrespcurv = tif_value;break;
             case 317: tif_predictor    = tif_value;break;       
             case 320: tif_colormap     = tif_value;break;       
             case 322: tif_tilewidth    = tif_value;break;
             case 323: tif_tilelength   = tif_value;break;
             case 324: tif_tileoffsets  = tif_value;
                       tif_tileoffsettype= tif_type;
                       break;
             case 325: //// tif_tilebytes = tif_value;
                       break;
          }

          length=tif_length*bytestouse;
          strip = 0;

          if((length>4)||(tif_type==5)) /* read field pointed to by value*/
          {  pos=ftell(fp);          
             fseek(fp, g.tif_skipbytes+tif_value, SEEK_SET);             
             if(tif_tag==273)
             {   tif_strip = new int[tif_length+40];
                 if(tif_strip==NULL){ message(g.nomemory,ERROR); goto tifend;}
                 alloc1=1;
             }  
             if(tif_tag==279)
             {   bytesperstrip=new int[tif_length+40];
                 if(bytesperstrip==NULL)
                 {   message(g.nomemory,ERROR); 
                     goto tifend; 
                 }
                 alloc2=1;
             }
             if(tif_type==4)              /* series of 4-byte integers */  
             {
                  for(j=0;j<tif_length;j++)
                  {  tif_integer=0;
                     tif_fread(&tif_integer,4,fp,mac,4);    
                     if((tif_tag==273)&&(alloc1==1))
                         tif_strip[strip]=tif_integer;
                     if((tif_tag==279)&&(alloc2==1))
                         bytesperstrip[strip]=tif_integer;
                     if(alloc1) tif_start=tif_strip[0];    
                     strip++;
                     if(strip>tif_length)
                     {    
                          message("Error: strip number exceeds\nnumber of image lines",ERROR);  
                          status = NOMEM;
                          goto tifend;
                     }     
                  }
             } 
             if(tif_tag==271)       /* make string */
                  fread(tif_string,min(100,tif_length),1,fp);

             fseek(fp,pos,SEEK_SET); /* jump back to where we were before*/
          }
       }   
   } 

   ////  Now that we know the samples/pixel it is safe to guess at
   ////  what bps means.

   if((tif_value_bps!=0) && ((tif_sampperpixel>1)||(tif_value_bps>32)) )
   {  bps = 0;
      for(k=0; k<tif_sampperpixel; k++)
      {   cbps[k] = tif_value_at(tif_value_bps+k*2,2,fp,mac);
          bps += cbps[k];
      }
   }
   if(!between(bps, 0, 48))
   {   message("Bad bits/sample in TIFF file");  
       bps = tif_value_bps*tif_sampperpixel;
   }
   tif_bitspersample = bps;

   if(tif_sampperpixel<=0)
   { // message("Bad samples/pixel in TIFF file,\nusing 1"); 
      tif_sampperpixel=1; 
   }

   ////  If it is 1 strip and no list of strip bytes was found, create
   ////  a short list and put strip length in.                        

   if(alloc1==0)
   {  alloc1=1;
      tif_strip = new int[20];
      tif_strip[0] = tif_start;
   }  
   if(alloc2==0)
   {  alloc2=1;
      bytesperstrip=new int[20];
      bytesperstrip[0] = tif_stripbytes;
   }

   tif_fread(&tif_offsettoIFD,4,fp,mac,4);   
   if(tif_offsettoIFD)
   {   second_image++;
       if(second_image==1)
         message("Only the 1st image in your file will be displayed");
   }    

   strcpy(tempstring,"Sorry, can't read "); 
   ok=0;   
        //---To add a new compression:-------------------------------
        //     1. change ok to 1 after corresponding compression type
        //     2. remove strcat statement
        //     3. add decoding in tif_getlineofpixels
        //-----------------------------------------------------------

   switch(tif_compression)  
   {   case -1   :
       case 0    : 
       case 1    :ok=1; break;
       case 2    :ok=0; strcat(tempstring,"CCITT RLE");  break;
       case 5    :ok=1; break;     //LZW
       case 6    :ok=0; strcat(tempstring,"JPEG");  break;
       case 32766:ok=0; strcat(tempstring,"2-bit RLE/NeXT\n");break;
       case 32771:ok=0; strcat(tempstring,"CCITT/align\n");break;
       case 32773:ok=1; break;     //PackBits 
       case 32809:ok=0; strcat(tempstring,"Thunderscan\n");break;
       case 32900:ok=0; strcat(tempstring,"Pixar RLE");  break;
       case 32901:ok=1; break;     //SGI RLE (16bpp packbits)
       default   :ok=0; 
                  strcat(tempstring,"type ");   
                  itoa(tif_compression,temp2,10);
                  strcat(tempstring,temp2);   
                  strcat(tempstring,"\n");   
                  break;
      }

   if(!ok)
   {  strcat(tempstring," compressed TIF images");
      message(tempstring,WARNING);
      status = ERROR;
      goto tifend;
   }    

         //---If 2 or more samples per pixel (color), make bpp ----//
         //---at least 16. `mbpp' is now the bits/pixel for the----//
         //---image in image[] buffer. `tif_bitspersample' is  ----//
         //---the original bits/pixel of the image file.       ----//

   if((tif_sampperpixel>1)&&(tif_bitspersample<16)) 
   {  message("Using 16 bpp for color image",WARNING);
      mbpp = max(tif_bitspersample, 16);
   }
   else mbpp=tif_bitspersample;

   //---pfac = No. of pixels to worry about for each byte----//
   //---     = 8 for 1 bit/pixel, 1 for 8 bits/pixel     ----//
   //---make it slightly smaller in case of round off err----//

   pfac = 7.9999/(double)tif_bitspersample;

   if((mbpp==1)||(mbpp==4))
      while(tif_imagewidth%(int)(1.001*pfac)) tif_imagewidth++;
   mbpp = max(8, mbpp);

   sizefac = max(g.off[g.bitsperpixel], (1+mbpp)/8);
   sizefac = max(2,sizefac);     // In case 3 samples/pixel -> 16 bpp

       //----In case the image is broken up into `tiles'---------//    

   if(tif_tilewidth)        
   {  
       tiles_across = (tif_imagewidth +tif_tilewidth -1)/tif_tilewidth;
       tiles_down   = (tif_imagelength+tif_tilelength-1)/tif_tilelength;
       nooftiles = tiles_across * tiles_down;

       //----Actual image width & length are now greater----// 
       //----because of bytes wasted to pad up to tile  ----// 
       //----boundaries.                                ----//

       otif_imagelength = tif_imagelength;
       tif_imagewidth = tif_tilewidth * tiles_across; 
       tif_imagelength = tif_tilelength * tiles_down; 
                             
       //---A tiled image can have its offsets listed   ----//
       //---as if they were strips, or can have separate----//
       //---tile offsets. Use (nooftiles > 1) to test   ----//
       //---if it is a tiled image. Use (tif_tileoffsets----//
       //---!=0) to test if it has tile offsets.        ----//

       if(tif_tileoffsets)   
       {   tileoffset = new int(nooftiles);
           if(tileoffset==NULL){ message(g.nomemory,ERROR); goto tifend;}
           alloc3=1;
           fseek(fp, g.tif_skipbytes+tif_tileoffsets, SEEK_SET);
           for(k=0;k<nooftiles;k++)
           {   if(tif_tileoffsettype==3)bytestoread=2; else bytestoread=4;
               tif_fread(&tileoffset[k],bytestoread,fp,mac,bytestoread);
           }
       }    
   }else
   {   tif_tilewidth  = tif_imagewidth;
       tif_tilelength = tif_imagelength;
       otif_imagelength = tif_imagelength;
   }   

   ////  Limitations for some types of files                     
   ////  LZW files must be vertical & full size if they are 8bpp.
   ////  TIF files must be full size to de-difference correctly. 

   if(tif_compression==5) 
   {    if(tif_bitspersample!=8) { g.tif_xsize=1; g.tif_ysize=1;  }
   }    
   if(tif_predictor==2) { g.tif_xsize=1; g.tif_ysize=1;  }

   //---------start of allocate space section----------------//

   yvalue = new int[tif_imagelength+1];
   if(yvalue!=0)
   {    alloc5=1;
        for(k=0;k<=tif_imagelength;k++) yvalue[k]=k;   // Not used, for interlaced gifs
   }else{ message(g.nomemory,ERROR); goto tifend;}

   if(tif_rowsperstrip==0) tif_rowsperstrip = tif_imagelength;
   buffer = new uchar[tif_rowsperstrip*sizefac*(tif_imagewidth+4)];  
   if(buffer==NULL){ message(g.nomemory,ERROR); goto tifend;}
   alloc4=1;
   if(tif_sampperpixel>1) ct=COLOR; else ct=INDEXED;
   if(tif_imagelength > 1e8){ message("Error reading tif file"); goto tifend; }

   status=newimage(actual_filename, g.tif_xoffset, g.tif_yoffset,
                   (int)(0.99999 + (tif_imagewidth)*g.tif_xsize),
                   (int)(0.99999 + (tif_imagelength)*g.tif_ysize),
                   mbpp, ct, 1, g.want_shell, 1, PERM, 1,
                   g.window_border, 0);  

   if(status != OK){ message(g.nomemory,ERROR); goto tifend;}
   z[ci].backedup = 0;
   cf = 0;
   frame = z[ci].cf;
   z[ci].bpp = mbpp;  // needed for readLZWencodedfile
   
   //-----------end of allocate space section----------------//

   if(tif_grayresponse)
   {  fseek(fp, g.tif_skipbytes+tif_grayrespcurv, SEEK_SET);
      for(k=0;k<=255;k++)        // Gamma[][] is allocated in all images
      {    z[ci].gamma[k]=0;     // Set to 0 because 32 bit integers
           tif_fread(&z[ci].gamma[k],2,fp,mac,2);
      }
   }

   //----If image has palette, read & set palette------------//

   highest=0;
   palette_factor = 1;
   if((1<<tif_bitspersample) > 256)
   {  // message("Error reading colormap!");
   }else
   if(tif_colormap && tif_photointerp==3)
   {  fseek(fp, g.tif_skipbytes+tif_colormap, SEEK_SET);
      for(k=0;k<(1<<tif_bitspersample);k++)
      {   tif_fread(&palettetemp,2,fp,mac,2);
          highest=max(highest,palettetemp);
          palette[0][k] = palettetemp;
      }
      for(k=0;k<(1<<tif_bitspersample);k++)
      {   tif_fread(&palettetemp,2,fp,mac,2);
          highest=max(highest,palettetemp);
          palette[1][k] = palettetemp;
      }
      for(k=0;k<(1<<tif_bitspersample);k++)
      {   tif_fread(&palettetemp,2,fp,mac,2);
          highest=max(highest,palettetemp);
          palette[2][k] = palettetemp;
      }
      if(highest<64) palette_factor = 1;       // 6 bit colormap
      else if(highest<256) palette_factor=4;   // 8 bit colormap
      else palette_factor=1024;                // 16-bit colormap
      for(k=0;k<(1<<tif_bitspersample);k++)
      {   g.palette[k].red = min(63, palette[0][k]/palette_factor);
          g.palette[k].green = min(63, palette[1][k]/palette_factor);
          g.palette[k].blue = min(63, palette[2][k]/palette_factor);
      }
      setpalette(g.palette);
   }

  xfac=g.tif_xsize;
  kxfac=(int)(xfac*64);           // integer to speed up multiplication
                                  // so  xfac*k  ==  ((kxfac*k)>>6).
  yfac=g.tif_ysize;               // yfac=size in y dimension, 0-1 1=full size
  kyfac=(int)(yfac*64);           // integer to speed up multiplication
                                  // so  yfac*k  ==  ((kyfac*k)>>6).

       // Determine if image is a standard bpp Tif 

  ts = 0;
  if(tif_sampperpixel>1)
  {  if(cbps[0]) ts++;
     if(cbps[1]) ts++;
     if(cbps[2]) ts++;
     if(cbps[3]) ts++;
  }
  tb = tif_bitspersample;   
  image_type=8;                                    // custom
  if (tb==1 )           image_type=0;              // monochrome
  if((tb==4 )&&(ts==0)) image_type=1;              // 4 bpp        
  if((tb==8 )&&(ts==0)) image_type=2;              // 8 bpp palette
  if((tb==16)&&(ts==3)) image_type=4;              // 16 bpp color
  if((tb==24)&&(ts==3)) image_type=5;              // 24 bpp color
  if((tb==32)&&(ts>=3)) image_type=6;              // 32 bpp color
  if((tb==48)&&(ts==3)) image_type=7;              // 48 bpp color
  fseek(fp, g.tif_skipbytes+tif_start, SEEK_SET);

  ////  Start reading image data. Pretend the image could have both    
  ////  and strips even though it currently can only have one or the   
  ////  other. It is only a matter of time before the spec. changes    
  ////  again to allow both.                                           
  tileystart = 0;

  if(tif_compression==5)
  {
        readLZWencodedfile(fp,0,9,256,tif_imagewidth,tif_imagelength,kxfac,
              kyfac,tif_bitspersample,TIF,tif_stripsperimage,
              bytesperstrip,tif_strip,yvalue);
     
        ////  Horizontally de-difference (LZW only)   
        ////  rr,gg,bb must be signed characters.      

        if(tif_predictor==2)   
        { for(j=0;j<tif_imagelength;j++) 
          {  rr2=0;gg2=0;bb2=0;
             for(i=0;i<tif_imagewidth*g.off[tb];i+=g.off[tb])
             {  
                value= pixelat(z[ci].image[z[ci].cf][j]+i,tb);
                valuetoRGB(value,r1,g1,b1,tb);
                rr=r1; gg=g1; bb=b1;
                rr += rr2;
                gg += gg2;
                bb += bb2;
                value = RGBvalue(rr,gg,bb,tb);
                putpixelbytes(z[ci].image[z[ci].cf][j]+i,value,1,tb,1);
                rr2=rr; gg2=gg; bb2=bb;
             }  
          }     
        }
        if(tif_bitspersample>=24) swap_image_bytes(ci); 
  }              
  else
  for(tile=0;tile<nooftiles;tile++)
  {
     y = tileystart;
     if(tif_tileoffsets) fseek(fp, g.tif_skipbytes+tileoffset[tile],SEEK_SET);
     currentstrip=0;
     stripline=0;
     while(1)
     {    ytimesyfac=(int)(y*yfac);    
          
          //--------Get some bytes---------------------//

          w = (int)((tif_tilewidth)/pfac);

          if(tif_planarconfig==2) w/=3;

          tif_getlineofpixels(fp, buffer, w, tif_compression, tif_bitspersample,
                    mac, tif_rowsperstrip, tif_strip, nooftiles);

          //--------If they are padding bytes in a ----//
          //--------tiled image, throw them away   ----//
        
          if((nooftiles>1)&&(ytile==tiles_down)&&(y>otif_imagelength))
              memset(buffer,0,(int)(tif_imagewidth*g.tif_xsize));

          ////  Flip bits
          if(tif_fillorder == 2)
             for(k=0; k<w; k++) buffer[k] = bitflip(buffer[k]);

          ////  Default case is significantly slower, so keep special   
          ////  cases for the common types of 8,16,24,and 32 bpp.    

          switch(image_type)
          {  
             case 7:             // 48 bit
               if(tif_planarconfig==2)
               {
                  for(k=0;k<tif_imagewidth;k++)
                      z[ci].image[cf][y][k*6+4-plane] = buffer[k];
               }else         
               {  for(k=0;k<w;k+=6)
                  {   
                      red   = (buffer[k+1]<<8) + buffer[k+0];  
                      green = (buffer[k+3]<<8) + buffer[k+2];
                      blue  = (buffer[k+5]<<8) + buffer[k+4];
                      i = ((kxfac*k)>>6) + tilexstart;                    
                      if(xfac!=1)
                      {  i = ((kxfac*k)>>6) + tilexstart; 
                         i += 4-(i&3);   //Align i to start on pixel boundary
                      }else
                      {  i = k + tilexstart; 
                      }
                      putRGBbytes(z[ci].image[frame][ytimesyfac]+i, red, green, blue, 48);
                    }
               }
               break;


             case 6:             // 32 bpp, 2 or more colors
               if(tif_planarconfig==2)
               {
                  for(k=0;k<tif_imagewidth;k++)
                      z[ci].image[cf][y][k*4+3-plane] = buffer[k];
               }else         
               {  if(cmyk && g.read_cmyk)
                  { for(k=0;k<w;k+=4)
                    {  
                        if(g.fix_cmyk)
                        {   if(invertbytes)
                            {   cc = buffer[k+1];  // Note: reversed bytes
                                mm = buffer[k+2];
                                yy = buffer[k+3];
                                kk = buffer[k+4];
                            }else
                            {   
                                cc = buffer[k+3];  // Note: reversed bytes
                                mm = buffer[k+2];
                                yy = buffer[k+1];
                                kk = buffer[k+0];
                            }
                        }
                        else
                        {   if(invertbytes)
                            {   cc = buffer[k+4];  // Note: reversed bytes
                                mm = buffer[k+3];
                                yy = buffer[k+2];
                                kk = buffer[k+1];
                            }else
                            {   
                                cc = buffer[k+0];  // Note: reversed bytes
                                mm = buffer[k+1];
                                yy = buffer[k+2];
                                kk = buffer[k+3];
                            }
                        }
                        i = ((kxfac*k)>>6) + tilexstart;                    
                        if(xfac!=1)
                        {  i = ((kxfac*k)>>6) + tilexstart; 
                           i += 4-(i&3);   //Align i to start on pixel boundary
                        }else
                        {  i = k + tilexstart; 
                        }
                        rr = max(0,min(255,255-cc-kk));
                        gg = max(0,min(255,255-mm-kk));
                        bb = max(0,min(255,255-yy-kk));
                        vv = RGBvalue(rr,gg,bb,32);
                        putpixelbytes(z[ci].image[frame][ytimesyfac]+i, vv, 1, 32, 1);
                    }
                  }else
                  { for(k=0;k<w;k+=4)
                    {   if(xfac!=1)
                        {  i = ((kxfac*k)>>6) + tilexstart; 
                           i += 4-(i&3); // Align i to start on pixel boundary
                        }else
                        {  i = k + tilexstart; 
                        }
                        rr = buffer[k+2];
                        gg = buffer[k+1];
                        bb = buffer[k+0];
                        vv = RGBvalue(rr,gg,bb,32);
                        putpixelbytes(z[ci].image[frame][ytimesyfac]+i, vv, 1, 32, 1);
                    }
                  }  
               }
               break;
             case 5:    // 24 bpp, 2 or more colors
               if(tif_planarconfig==2)
               {
                  for(k=0;k<tif_imagewidth;k++)
                      z[ci].image[cf][y][k*3+2-plane] = buffer[k];
               }else 
               for(k=0;k<w;k+=3)
               {      if(xfac!=1)
                      {  i = ((kxfac*k)>>6) + tilexstart; 
                         while(i%3) i++; // Align i to start on pixel boundary
                      }else
                      {  i = k + tilexstart; 
                      }                  
                      z[ci].image[z[ci].cf][ytimesyfac][i+2] = buffer[k  ];
                      z[ci].image[z[ci].cf][ytimesyfac][i+1] = buffer[k+1];
                      z[ci].image[z[ci].cf][ytimesyfac][i+0] = buffer[k+2];
               }
               break;
             case 4:   // 16 bpp, 2 or more colors
               for(k=0;k<w;k+=2)
               {      if(xfac!=1)
                      {  i = ((kxfac*k)>>6) + tilexstart; 
                         i += i & 1;    // Align i to start on pixel boundary
                      }else
                      {  i = k + tilexstart; 
                      }
                      z[ci].image[z[ci].cf][ytimesyfac][i+1] = buffer[k  ];
                      z[ci].image[z[ci].cf][ytimesyfac][i  ] = buffer[k+1];
               }
               break; 
             case 2:    // 8 bpp, 1 color
               if(xfac==1)
                     memcpy(z[ci].image[z[ci].cf][ytimesyfac]+tilexstart,buffer,w);
               else
                     for(k=0;k<w;k++)
                     {   i = (kxfac*(k+tilexstart)) >> 6; 
                         z[ci].image[z[ci].cf][ytimesyfac][i] = buffer[k];
                     }   
               break;
             case 1:   // 4 bpp from a VGA - can have 4 bit palette
                shift = 4;
                if((tif_colormap)&&(tif_photointerp==3)) shift=0;
                if(tif_planarconfig==1)
                {     for(k=0;k<w;k++)
                      {  color1=(buffer[k]>>4)<<shift;
                         color2=(buffer[k]&0x0f)<<shift;
                         if(xfac==1)
                         { z[ci].image[z[ci].cf][ytimesyfac][(k<<1)+tilexstart  ]= color1;
                           z[ci].image[z[ci].cf][ytimesyfac][(k<<1)+tilexstart+1]= color2;
                         }else
                         { i =((kxfac*((k<<1)  ))>>6) + tilexstart;
                           z[ci].image[z[ci].cf][ytimesyfac][i]=color1;
                           i =((kxfac*((k<<1)+1))>>6) + tilexstart;
                           z[ci].image[z[ci].cf][ytimesyfac][i]=color2;
                         }
                      }
                }else 
                {message("Error reading TIF file-\nunsupported planar config",ERROR);
                 status=ERROR;
                }    
                break;
             case 0:       // monochrome- convert to 8 bpp then to color_type
                for(i=0;i<w;i++)
                { 
                     for(k2=0;k2<8;k2++)
                     {   
                         bitmask = 1 << (7-k2);
                         color = (buffer[i] & bitmask) >> (7-k2);
                         if(color) color=255;
                         if(tif_photointerp==0) color=255-color;
                         if(xfac==1)
                            z[ci].image[z[ci].cf][ytimesyfac][i*8+k2+tilexstart]=color; 
                         else    
                         {   i2 = ( (kxfac*i*8+k2 ) >> 6 ) + tilexstart;
                             z[ci].image[z[ci].cf][ytimesyfac][i2]=color;  
                         }    
                     }
                }    
                break;
             default:
               mbpp = z[ci].bpp;
               getbits(0,buffer,0,RESET);  //reset
               count=0;
               for(k=0,k2=0;;k+=g.off[mbpp],k2++)
               {   count++;                  
                   if(count>tif_tilewidth) break;
                   if(tif_sampperpixel==1)
                   {   value = getbits(tif_bitspersample,buffer,w,TIF);
                       if(tif_bitspersample<=8) // filter thru current palette
                       {   dbpp = 8-tif_bitspersample;
                           value++;
                           value <<= dbpp;
                           value--;
                           valuetoRGB(value,r1,g1,b1,8);   //get palette entries
                           value = RGBvalue(r1,g1,b1,mbpp);//calc.output value
                       }else
                       if(tif_bitspersample<=16) // convert to 16 bpp
                       {   dbpp = 16-tif_bitspersample;
                           value++;
                           value <<= dbpp;
                           value--;
                       }else
                       if(tif_bitspersample<=24) // convert to 24 bpp
                       {   value++;
                           dbpp = 24-tif_bitspersample;
                           value <<= dbpp;
                           value--;
                       }else
                       if(tif_bitspersample<=32) // convert to 32 bpp
                       {   value++;
                           dbpp = 32-tif_bitspersample;
                           value <<= dbpp;
                           value--;
                       }    
                   }else 
                   {   r1 = getbits(cbps[0],buffer,w,TIF);
                       g1 = getbits(cbps[1],buffer,w,TIF);
                       b1 = getbits(cbps[2],buffer,w,TIF);
                       kk= getbits(cbps[3],buffer,w,TIF);

                       dbpp = g.redbpp[mbpp]   - cbps[0];
                       if(dbpp>0) r1 <<= ( dbpp);
                       if(dbpp<0) r1 >>= (-dbpp);
                     
                       dbpp = g.greenbpp[mbpp] - cbps[1];    
                       if(dbpp>0) g1 <<= ( dbpp);
                       if(dbpp<0) g1 >>= (-dbpp);
                     
                       dbpp = g.bluebpp[mbpp]  - cbps[2];  
                       if(dbpp>0) b1 <<= ( dbpp);
                       if(dbpp<0) b1 >>= (-dbpp);
                       if((g.read_cmyk)&&(cmyk))
                       {    dbpp = g.redbpp[mbpp] - cbps[3];  
                            if(dbpp>0) kk <<= ( dbpp);
                            if(dbpp<0) kk >>= (-dbpp);

                            a  = g.maxred[mbpp];
                            r1 = max(0,min(a,a-r1-kk));
                            a  = g.maxgreen[mbpp];
                            g1 = max(0,min(a,a-g1-kk));
                            a  = g.maxblue[mbpp];
                            b1 = max(0,min(a,a-b1-kk));
                       }
                       value = RGBvalue(r1,g1,b1,mbpp);  //Pixel value in memory
                   }                      

                   if(xfac!=1)
                   {       i = ((kxfac*k)>>6) + tilexstart; 
                           switch(g.off[mbpp])//Align i to start on pixel boundary
                           {  case 2: i &= 0xfffffffe; break;
                              case 3: i -= i%3;        break;
                              case 4: i &= 0xfffffffc; break;  
                           }   
                   }else i = k + tilexstart;                   
                   putpixelbytes(z[ci].image[z[ci].cf][ytimesyfac]+i, value, 1,
                           mbpp, 1);
               }
               break;  
          }  
          if(keyhit()) if(getcharacter()==27){ status=ABORT; break; }
          if(g.getout){ status=ABORT; break; }
          y++;     
          if(tif_planarconfig==2 && y>=tif_imagelength && plane<tif_sampperpixel-1)
          {   plane++; y=0; }
          if(y-tileystart>=tif_tilelength) break;  // At end of a tile
          if(y>tif_imagelength) break;             // Finished entire image
     }
     tilexstart += tif_tilewidth;
     xtile++;
     if(xtile > tiles_across)
     {   tilexstart=0;
         xtile=1;
         ytile++;
         tileystart += tif_tilelength;
     }  
       
  }   

tifend:
  fclose(fp);
  z[ci].touched = 0;

  switch(tif_photointerp)
  {     case 0:
        case 1:  z[ci].colortype=GRAY; break;
        case 3:  z[ci].colortype=INDEXED; break;
        default: z[ci].colortype=COLOR; break;
  }
  if(tif_sampperpixel==1 && tif_bitspersample>8) z[ci].colortype=GRAY;
  if(tif_sampperpixel==1 && tif_photointerp==0) invertimage();
  if(!mac && tif_bitspersample>8 && z[ci].colortype==GRAY) swap_image_bytes(ci);
  
  if(alloc5) delete[] yvalue;
  if(alloc4) delete[] buffer;
  if(alloc3) delete[] tileoffset;
  if(alloc2) delete[] bytesperstrip;
  if(alloc1) delete[] tif_strip;
  delete[] tif_string;
  return(status);      
}



//--------------------------------------------------------------------------//
// getbits                                                                  //
// Gets n bits from a buffer. n can be 0 to sizeof(int).                    //
// Returns an int. The bits are right-shifted.                              //
// Works for 16 or 32-bit integers.                                         //
//                                                                          //
// blockpos  = Current position in array `block' (wraps to 0 if all the     //
//             bytes are used, i.e. if blockpos exceeds blocksize.          //
// blocksize = Size of array `block'                                        //
// block     = char array of the data already read from disk                //
//                                                                          //
// Keeps track of bits left over and bytes unread.                          //
// If mode==RESET                                                           //
//  Starts over at start of buffer                                          //
// If mode==NONE                                                            //
//  Assumes bits are not packed. Gets 1 or more even bytes per call.        //
// If mode==GIF                                                             //
//  Assumes bits are packed in the following format (example for n=9) :     //
//                      7  6  5  4    3  2  1  0                            //
//                    ----------------------------                          //
//     Byte 1          a7 a6 a5 a4   a3 a2 a1 a0                            //
//     Byte 2          b6 b5 b4 b3   b2 b1 b0 a8                            //
//     Byte 3          c5 c4 c3 c2   c1 c0 b8 b7                            //
//     Byte 4          d4 c3 d2 d1   d0 c8 c7 c6                            //
//     ...                                                                  //
// If mode==TIF                                                             //
//  Assumes bits are packed in the following format (example for n=9) :     //
//                      7  6  5  4    3  2  1  0                            //
//                    ----------------------------                          //
//     Byte 1          a8 a7 a6 a5   a4 a3 a2 a1                            //
//     Byte 2          a0 b8 b7 b6   b5 b4 b3 b2                            //
//     Byte 3          b1 b0 c8 c7   c6 c5 c4 c3                            //
//     Byte 4          c2 c1 c0 d8   d7 d6 d5 d4                            //
//     ...                                                                  //
//--------------------------------------------------------------------------//
int getbits (int n, uchar* block, int blocksize, int mode)
{
   static int blockpos = 0;
   static int bitsleft = 8;
   static uchar nextbyte = block[0];

   
   int newno=0,answer=0;
   int need = n;
   int n1 = n;                                   // No.of bits from 1st byte

   newno=0;
   answer=0;

   if(mode==RESET)
   {  bitsleft=8;
      blockpos=0;
      nextbyte=block[0];
      return(0);
   }
   else if(mode==NONE)
   {  bitsleft=((n+7)>>3)<<3;                // Bits to get (next mult. of 8)  
      while(bitsleft)
      {  
         bitsleft -= 8;
         answer += nextbyte;
         if(bitsleft) answer <<= 8;
         blockpos++;        
         if(blockpos >= blocksize) blockpos=0;
         nextbyte = block[blockpos];  
      }            
   } 
   else if(mode==GIF)do
   {
      n1 = min(min(8, need), bitsleft);
      newno = bits(nextbyte, 7+n1-bitsleft, n1); // also right-shifts newno
      newno <<= (n - need);
      answer += newno;
      bitsleft -= n1;
      need -= n1;

      if(!bitsleft)
      {   blockpos++;        
          if(blockpos >= blocksize) blockpos=0;
          nextbyte = block[blockpos];       // Get rest of bits from next byte
          bitsleft = 8;
      }
      if(blockpos >= blocksize)
      {   blockpos=0;
          nextbyte = block[0];
          bitsleft=8;
      }

   } while(need);
   else if(mode==TIF) do
   {
                                            // Nextbyte is the byte being used

      n1 = min(min(8, need), bitsleft);     // n1=No.of bits to take from byte

                                            // newno = Value obtained from the
                                            //   n1 bits (right-shifted by the
                                            //   function bits()).
      newno = bits(nextbyte, bitsleft-1, n1); 
      bitsleft -= n1;
      need -= n1;
      newno <<= need;
      answer += newno;

      if(!bitsleft)
      {   blockpos++;        
          if(blockpos >= blocksize) 
          {   blockpos=0;
              nextbyte=0;
          }else
          {   nextbyte = block[blockpos];   // Get rest of bits from next byte
          }
          bitsleft = 8;
      }
      if(blockpos >= blocksize)
      {   blockpos=0;
          nextbyte=0;
          bitsleft=8;
      }

   } while(need);
   else answer=0;

   return(answer);
}


//--------------------------------------------------------------------------//
// tif_value_at                                                             //
// Read value from a tiff file when the given tif_value is a pointer.       //
//--------------------------------------------------------------------------//
short int tif_value_at(unsigned int tif_pointer, int size, FILE* fp, int mac)
{ 
     int result=0;
     int pos=0;
     pos=ftell(fp);          
     fseek(fp,g.tif_skipbytes+tif_pointer,SEEK_SET);             
     tif_fread(&result,size,fp,mac,size);   
     fseek(fp,pos,SEEK_SET);   /* jump back to where we were before*/     
     return(result);
}



//--------------------------------------------------------------------------//
// tif_fread                                                                //
// Read a value from a tiff file, taking into account mac Tif's             //
// For mac files, you also need to specify how many bytes to use, because   //
//  tags always read 4 bytes, but mac's may only use the 1st 2 or all 4.    //
//  Example:   1 128  0  0   =  384 (if tif_type==4)(4 byte)                //
//             0   0  0  3   =    3 (if tif_type==3)(2 byte)                //
//  (this only applies when reading the tif_value).                         //
// Maximum = 8 bytes.                                                       //
//--------------------------------------------------------------------------//
int tif_fread(char* a, int bytes, FILE* fp, int mac, int bytestouse)
{  mac=mac;                // keep compiler quiet
   bytestouse=bytestouse;
   fread(a,bytes,1,fp);
#ifndef LITTLE_ENDIAN
   if(bytestouse!=0) swapbytes((uchar*)a, bytes, bytestouse);
#endif

   return bytes;
}

int tif_fread(uchar* a, int bytes, FILE* fp, int mac, int bytestouse)
{  mac=mac;                // keep compiler quiet
   bytestouse=bytestouse;
   fread(a,bytes,1,fp);
#ifndef LITTLE_ENDIAN
   if(bytestouse!=4 && bytestouse!=0) swapbytes(a, bytes, bytestouse);
#endif
   return bytes;
}

int tif_fread(short int* a, int bytes, FILE* fp, int mac, int bytestouse)
{  mac=mac; bytes=bytes; bytestouse=2;  // keep compiler quiet
   short int k;  
   k = getword(fp);
   if(mac) k = (k>>8) + ((k&0xff)<<8);
   *a = (short int)(k & 0xffff);
   return bytes;
}

int tif_fread(ushort *a, int bytes, FILE* fp, int mac, int bytestouse)
{  mac=mac; bytes=bytes; bytestouse=2;  // keep compiler quiet
   ushort k;
   k = getword(fp);
   if(mac) k = (k>>8) + ((k&0xff)<<8);
   *a = (ushort)k;
   return bytes;
}

int tif_fread(int* a, int bytes, FILE* fp, int mac, int bytestouse)
{  int k;
   int answer;
   uchar buf[8];
   fread(buf,bytes,1,fp);
   answer = 0;
   if(mac)
   {   
      for(k=0;k<bytes;k++) 
          answer += ((buf[bytestouse-1-k]) << (k<<3)); 
   }   
   else 
   {  for(k=0;k<bytes;k++) answer += ((buf[k]) << (k<<3));  }
   *a = answer;
   return bytes;
}


//--------------------------------------------------------------------------//
// tif_getlineofpixels                                                      //
// Reads a line of pixels in the specified compression format.              //
// `length'    =  bytes per line                                            //
// `stripline' is a global = which line in the strip                        //
// `currentstrip'is a global = which strip                                  //
// Reads enough bytes for 1 horiz.line of pixels. Assumes a strip is an     //
// integral number of scan lines. Doesn't handle LZW (handled elsewhere).   //
//--------------------------------------------------------------------------//
void tif_getlineofpixels(FILE* fp, uchar* buffer, int length, int compression, 
   int bpp, int mac, int rowsperstrip, int* tif_strip, int nooftiles)
{
    int count=0, k, n, c;
    if((nooftiles<=1)&&(stripline==0)) 
        fseek(fp, g.tif_skipbytes+tif_strip[currentstrip], SEEK_SET);
    switch(compression)
    {   case 2:           // CCITT RLE
             break;
        case 5:           // LZW - handled elsewhere
             break;
        case 32773:       // PackBits   
             do
             {   tif_fread(&n,1,fp,mac,1);
                 if(between(n,0,127))
                 {   
                     tif_fread((char*)(buffer+count),n+1,fp,mac,n+1);
                     count += n+1;
                 }    
                 else if(n != -128)
                 {   
                     if(n>0) n=-256+n;
                     tif_fread(&c,1,fp,mac,1);
                     for(k=count;k<count+(-n+1);k++) *(buffer+k)=c;
                     count += -n+1;
                 }
             }while(count<length);
             break;
        case 32901:       // SG RLE 16 bpp PackBits (untested)
             do
             {   tif_fread(&n,1,fp,mac,1);
                 if(n>=0)
                 {   if(bpp==8) 
                     {  tif_fread((char*)(buffer+count),n+1,fp,mac,n+1);
                        count += n+1;
                     }else
                     {  tif_fread((char*)(buffer+count),2*n+2,fp,mac,2*n+2);
                        count += 2*n+2;
                     }
                 }    
                 else if(n != -128)
                 {   tif_fread(&c,1,fp,mac,1);
                     if(bpp==8)
                     {  for(k=count;k<count+(-n+1);k++) *(buffer+count)=c;
                        count += -n+1;
                     }else  
                     {  for(k=count;k<count+(-2*n+2);k++) *(buffer+count)=c;
                        count += -2*n+2;
                     }   

                 }
             }while(count<length);
             break;
        case 1:          // Swap bytes here if image quant (bytes are in wrong
                         // order). 
             tif_fread(buffer,length,fp,mac,g.off[bpp]);  
             // if(image_quant && bpp==16) swapbytes(buffer,length,2);
             break;
        default:          // No compression or unknown
             tif_fread(buffer,length,fp,mac,0);  
             // if(image_quant && bpp==16) swapbytes(buffer,length,2);
             break;
    }
    stripline++;
    if(stripline==rowsperstrip) { stripline=0; currentstrip++; }
    getbits(0,buffer,0,RESET);  //reset
}


//--------------------------------------------------------------------------//
// abouttiffile                                                             //
// Display information about tif file.                                      //
//--------------------------------------------------------------------------//
void abouttiffile(char *filename, int compressed)
{  
   static char *listtitle;
   static char **info;                                    
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return; } 
   FILE *fp;
   char temp[256];
   char label1[16]="rgb????????????";
   char label2[16]="cmyk???????????";
   int j,k;
   int tif_sampperpixel=0;
   int tif_colormap    =0;
   int tif_value_bps   =1;
   int tif_byteorder   =0;
   int tif_version     =0;
   int tif_noofdirectories=0;
   int tif_offsettoIFD=0;
   int tif_tag=0;
   int tif_type=0;
   int tif_start=0;
   int tif_integer=0;  
   int tif_integer2=0;  
   int tif_photointerp=0;
   int cbps[16],bps=1;        // Up to 16 colors with different bits/pixels
   int mac=0;
   int bytestouse = 4;
   int bytes,pos;
   char junk;
   int count=1;
   char tif_string[1024];
   int length;
   int tif_length     =0;
   int tif_value      =0;
   if((fp=open_file(filename, compressed, TNI_READ, g.compression, g.decompression, 
       g.compression_ext))==NULL) return ;
                         //// Can't read tif file thru pipe so jump out.
                         //// Some files are pipe-friendly but for others the
                         //// only alternative is to create a temp file.
                         //// This is too much trouble just to get information.
   if(compressed)    
   {   sprintf(temp, "TIF file compressed with\n%s",g.compression);
       message(temp);
       close_file(fp, compressed); 
       return; 
   }
   
   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);

   fread(&tif_byteorder,2,1,fp);  // This must be the same in all Tif files
   if(tif_byteorder==0x4d4d) mac=1; else mac=0;
   tif_fread(&tif_version,2,fp,mac,2); 
   tif_fread(&tif_offsettoIFD,4,fp,mac,4);

   fseek(fp, g.tif_skipbytes+tif_offsettoIFD, SEEK_SET);
   tif_fread(&tif_noofdirectories,2,fp,mac,2);

   //-------------Put information in list box-------------------------//

   listtitle = new char[100];
   strcpy(listtitle, "About the File");
   int listcount=-1;
   info = new char*[100];   // max. 100 lines - Change if you add more items

   info[++listcount] = new char[100];                       
   sprintf(info[listcount],"Filename:  %s",basefilename(filename));

   info[++listcount] = new char[100];                       
   sprintf(info[listcount],"File format: TIFF");

   info[++listcount] = new char[100];                       
   if(tif_byteorder==0x4d4d) 
   {   sprintf(info[listcount],"Byte order: big endian");
       mac=1; 
   }else
   {   sprintf(info[listcount],"Byte order: little endian");
       mac=0;
   }



   info[++listcount] = new char[100];                       
   if(mac) sprintf(info[listcount],"Macintosh TIFF file");
   else    sprintf(info[listcount],"PC (Intel) TIFF file");

   info[++listcount] = new char[100];                       
   sprintf(info[listcount],"TIFF version %d",tif_version);
   
   info[++listcount] = new char[100];                       
   sprintf(info[listcount],"Offset to IFD  %d",tif_offsettoIFD);

   Tiftag t[100];
   int tc=0;
   for(k=1;k<=tif_noofdirectories;k++) 
   {
       tif_tag=0;
       tif_fread(&tif_tag, 2, fp, mac, 2);   
       tif_fread(&tif_type, 2, fp, mac, 2);  
       tif_fread(&tif_length, 4, fp, mac, 4);
       if(tif_type==1) bytestouse = 1;
       if(tif_type==2) bytestouse = 1;
       if(tif_type==3) bytestouse = 2;
       if(tif_type==4) bytestouse = 4;
       if(tif_type==5) bytestouse = 4;

       if(tif_length>1) bytestouse = 4;

       tif_fread(&tif_value, 4, fp, mac, bytestouse);    
       if(bytestouse==1) tif_value &= 0xff;
       if(bytestouse==2) tif_value &= 0xffff;

       ////  If tag is an offset, store in array so it can be sorted.
       length=tif_length*bytestouse;
       if((length>4)||(tif_type==5)) 
       {   t[tc].tag    = tif_tag;
           t[tc].value  = tif_value;
           t[tc].length = tif_length;
           t[tc].type   = tif_type;
           tc++;
       }
           
       info[++listcount] = new char[100];                       
       if(listcount > 200) break;

       switch(tif_tag)
       {     case 254: sprintf(info[listcount],"new subfile type %d",tif_value);break;
             case 255: sprintf(info[listcount],"subfile type %d",tif_value);break;
             case 256: sprintf(info[listcount],"image width  %d",tif_value);break;
             case 257: sprintf(info[listcount],"image length %d",tif_value);break;
             case 258: bps = tif_value;  // The meaning of this may
                                         // change depending on the
                                         // tif_sampperpixel, which could
                                         // be read later.
                       tif_value_bps = bps;
                       if((tif_value_bps!=0) && ((tif_sampperpixel>1)||(tif_value_bps>32)) )
                           sprintf(info[listcount],"bpp offset %d", bps);
                       else
                           sprintf(info[listcount],"bits/pixel %d", bps);
                       break;
             case 259: switch(tif_value)
                       {  case 0:
                          case 1    :strcpy(temp,"none");       break;
                          case 2    :strcpy(temp,"CCITT RLE");  break;
                          case 5    :strcpy(temp,"LZW");        break;
                          case 6    :strcpy(temp,"JPEG");       break;
                          case 32766:strcpy(temp,"2bit RLE/NeXT");break;
                          case 32771:strcpy(temp,"CCITT/align");break;
                          case 32773:strcpy(temp,"PackBits");   break;
                          case 32809:strcpy(temp,"Thunderscan");break;
                          case 32900:strcpy(temp,"Pixar RLE");  break;
                          case 32901:strcpy(temp,"SGI RLE");    break;
                          default: strcpy(temp,"other");
                       }
                       sprintf(info[listcount],"compression %s",temp);
                       break;
             case 262: switch(tif_value)
                       {  case 0:  strcpy(temp,"B/W-gray");      break;
                          case 1:  strcpy(temp,"Neg.B/W-gray");  break;
                          case 2:  strcpy(temp,"RGB color");     break;
                          case 3:  strcpy(temp,"Pal.color");     break;
                          case 4:  strcpy(temp,"Transparency");  break;
                          case 5:  strcpy(temp,"CMYK color");    break;
                          default: strcpy(temp,"Other");         break;
                       }
                       tif_photointerp=tif_value;
                       sprintf(info[listcount],"photo interp  %s",temp);break;
             case 263: sprintf(info[listcount],"thresholding  %d",tif_value);break; 
             case 266: sprintf(info[listcount],"fill order    %d ",tif_value);break;
             case 269: sprintf(info[listcount],"document name offset %d",tif_value);break;               
             case 270: sprintf(info[listcount],"image descrip.offset %d",tif_value);break;
             case 271: sprintf(info[listcount],"make offset   %d  ",tif_value);break;
             case 272: sprintf(info[listcount],"model offset  %d ",tif_value);break;
             case 273: if(!tif_start)tif_start=tif_value;
                       sprintf(info[listcount],"strip offsets %d",tif_value);break;
             case 274: sprintf(info[listcount],"orientation   %d",tif_value);break;
             case 277: tif_sampperpixel = tif_value;                         
                       sprintf(info[listcount],"samples/pixel %d",tif_value);break;
             case 278: sprintf(info[listcount],"rows/strip    %d",tif_value);break;
             case 279: sprintf(info[listcount],"strip bytes   %d",tif_value);break;
             case 280: sprintf(info[listcount],"min value     %d",tif_value);break;
             case 281: sprintf(info[listcount],"max value     %d",tif_value);break;
             case 282: sprintf(info[listcount],"x resolution  %d",tif_value);break;
             case 283: sprintf(info[listcount],"y resolution  %d",tif_value);break;
             case 284: sprintf(info[listcount],"image planes  %d",tif_value);break;
             case 286: sprintf(info[listcount],"x position    %d",tif_value);break;
             case 287: sprintf(info[listcount],"y position    %d",tif_value);break;
             case 288: sprintf(info[listcount],"free offsets  %d",tif_value);break;                
             case 289: sprintf(info[listcount],"free bytes    %d",tif_value);break;               
             case 290: sprintf(info[listcount],"gray response %d",tif_value);break; 
             case 291: sprintf(info[listcount],"gray resp.curve at %d",tif_value);break;
             case 296: if(tif_value==1) strcpy(temp,"none");
                       if(tif_value==2) strcpy(temp,"inches");
                       if(tif_value==3) strcpy(temp,"centimeters");                     
                       sprintf(info[listcount],"resolution unit %d  units %s",tif_value,temp);
                       break;
             case 301: sprintf(info[listcount],"color resp.curve %d",tif_value);break;
             //case 305: sprintf(info[listcount],"software ");break;
             case 306: sprintf(info[listcount],"date & time ");break;
             case 315: sprintf(info[listcount],"artist"); break;
             case 316: sprintf(info[listcount],"host computer");break;
             case 317: sprintf(info[listcount],"predictor   %d",tif_value);break;
             case 318: sprintf(info[listcount],"white point %d",tif_value);break;
             case 319: sprintf(info[listcount],"Primary chromaticities %d",tif_value);
             case 320: sprintf(info[listcount],"color map offset %d ",tif_value);
                       tif_colormap     = tif_value;break;       
             case 322: sprintf(info[listcount],"tile width   %d",tif_value);break;
             case 323: sprintf(info[listcount],"tile length  %d",tif_value);break;
             case 324: sprintf(info[listcount],"tile offsets %d",tif_value);break;
             case 325: sprintf(info[listcount],"tile bytes   %d",tif_value);break;
             case 338: sprintf(info[listcount],"extra samples %d",tif_value);break;
             default: 
                if((unsigned)tif_tag>=32768)
                      sprintf(info[listcount],"  (Unknown vendor-specific tag) %d",tif_value);
                else
                      sprintf(info[listcount],"  value  %d",tif_value);
       }
     
   }
   qsort(t, tc, sizeof(Tiftag), tif_cmp);

   for(k=1;k<tc;k++) 
   {
       tif_tag = t[k].tag;
       tif_length = t[k].length;
       tif_value = t[k].value;
       tif_type = t[k].type;
       
       length=tif_length*bytestouse;
       if((length>4)||(tif_type==5)) /* read field pointed to by value*/
       {    
             pos=ftell(fp);          
             fseek(fp, g.tif_skipbytes+tif_value, SEEK_SET);             
             switch(tif_type)
             { case 1:      /* series of chars */  
                  for(j=0;j<tif_length;j++)
                  {  tif_integer=0;
                     tif_fread(&tif_integer,1,fp,mac,1);
                     // sprintf(info[listcount]," byte %d",tif_integer);    
                  }
                  break;
               case 2:      /* string */
                  bytes = min(100,tif_length);
                  tif_fread(tif_string, bytes, fp, mac, bytes);
                  info[++listcount] = new char[100];                       
                  if(tif_tag==305) sprintf(info[listcount],"software %s",tif_string);
                  else sprintf(info[listcount],"  %s",tif_string);           
                  break;
               case 3:      /* series of 2-byte integers */  
                  info[++listcount] = new char[100];                       
                  sprintf(info[listcount],"  No.of 2-byte ints %d",tif_length);
                  for(j=0;j<tif_length;j++)
                  {  tif_integer=0;
                     tif_fread(&tif_integer,2,fp,mac,2);
                     // sprintf(info[listcount]," int %d",tif_integer);    
                  }
                  break;
               case 4:      /* series of 4-byte integers */  
                  info[++listcount] = new char[100];                       
                  if(tif_tag==273)
                     sprintf(info[listcount],"  No.of strips  %d",tif_length);
                  else
                     sprintf(info[listcount],"  No.of 4-byte ints  %d",tif_length);
                  tif_integer=0;
                  for(j=0;j<tif_length;j++)
                  {  tif_integer=0;
                     tif_fread(&tif_integer,4,fp,mac,4);
                     if((tif_tag==273)&&(j==0))
                     {   tif_start=tif_integer;    
                         info[++listcount] = new char[100];                       
                         sprintf(info[listcount],"Image start %d",tif_start);                
                     }
                     if(tif_tag==273)      sprintf(info[listcount]," strip start %d",tif_integer);    
                     else if(tif_tag==279) sprintf(info[listcount]," bytes/strip %d",tif_integer); 
                     else                  sprintf(info[listcount]," int %d",tif_integer);    

                  }
                  break;
               case 5:      /* 2 4-byte integers */  
                  tif_integer=0;
                  tif_fread(&tif_integer,4,fp,mac,4);
                  tif_fread(&tif_integer2,4,fp,mac,4);
                  break;
             } 
       }
     
   }
   if((tif_colormap)&&(tif_photointerp==3))
   {   info[++listcount] = new char[100];                       
       sprintf(info[listcount],"Palette present");
   }

     // Now that we know the samples/pixel it is safe to guess at
     // what bps means.
     // Maybe it was bits/pixel or maybe it was pointer to a list of
     // tif_sampperpixel bits/pixel's.

   info[++listcount] = new char[100];                       
   sprintf(info[listcount],"tif_samp/pixel %d",tif_sampperpixel);

   if((tif_value_bps!=0) && ((tif_sampperpixel>1)||(tif_value_bps>32)) )
   {  bps = 0;
      for(k=0;k<tif_sampperpixel;k++)
      {   cbps[k] = tif_value_at(tif_value_bps+k*2, 2, fp, mac);
          strcpy(temp," x");
          strcat(temp," bits/samp");
          if(tif_photointerp==5) temp[1]=label2[k];
          else temp[1]=label1[k];
          info[++listcount] = new char[100];                       
          sprintf(info[listcount],"%s %d",temp, cbps[k]);
          bps += cbps[k];
      }
      info[++listcount] = new char[100];                       
      sprintf(info[listcount],"Total bits/pixel %d", bps);
   }

   tif_value=0;
   tif_offsettoIFD=0;     
   
   tif_fread(&tif_offsettoIFD,4,fp,mac,4);
   if(tif_offsettoIFD > 0)
   {   count++;
       info[++listcount] = new char[100];                       
       sprintf(info[listcount],"Image number %d",count);
       info[++listcount] = new char[100];                       
       sprintf(info[listcount],"Offset to next IFD %d",tif_offsettoIFD);
   }    
   else
   {   info[++listcount] = new char[100];                       
       sprintf(info[listcount],"Image data start %d",tif_start);
   }
   close_file(fp, compressed);  

   ////-----------------done filling list---------------////

   static listinfo *l;
   l = new listinfo;
   l->title  = listtitle;
   l->info   = info;
   l->count  = listcount+1;
   l->itemstoshow = min(20,listcount+1);
   l->firstitem   = 0;
   l->wantsort    = 0;
   l->wantsave    = 0;
   l->helptopic   = 1;
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
   l->wc = 0;
   list(l);
}


//--------------------------------------------------------------------------//
// tif_cmp - for qsort                                                      //
//--------------------------------------------------------------------------//
int tif_cmp(const void *p, const void *q)
{
   return sgn( ((Tiftag*)p)->value - ((Tiftag*)q)->value);
}


//--------------------------------------------------------------------------//
// writetiffile  -  Returns 0 if successful, error number if unsuccessful   //
//   TIF class R                                                            //
//   The image offset is                                                    //
//        10 for header                                                     //
//      + 12*(no.of tags)  (==18 if B/W, 21 if color)                       //
//      + 4  for the 4 extra bytes after the list of tags.                  //
//      + 16 for the x-resolution & y-resolution (tags 282,283)             //
//      + 512 for gray response curve  (if want_color_type==1)              //
//      + 1536 for palette (if want_color_type==2)                          //
//   All int's are assumed to be 32 bits. Short int's are 16 bits.          //
//--------------------------------------------------------------------------//
int writetiffile(char *filename, int write_all, int compress)
{
   if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 
   FILE *fp;
   char softwarestring[15];
   short int palettetemp, shortk;
   int a, dbpp, count=0, frame, ino, j, k, oldxend, size=0, size1, size2,
       status=OK, xstart, xend, y, ystart, yend, ui2=0, ui2tot=0;
   uint value=0, xresptr=0, yresptr=0, bytecountptr=0, ui=0, uir, uig, uib;

   strcpy(softwarestring,"tnimage ");
   strcat(softwarestring, g.version);
   int* stripoffset;          // File offset for start of each strip
   int* bytesperstrip=NULL;   // No.of bytes in each strip
   uchar* bufr=NULL;
   uchar* bufr2=NULL;

   double bppfac       = 1.0; // Desired bytes/pixel to save as
   int intbppfac      = 1;    // bppfac rounded up to next integer   
   int lengthinpixels = 0;
   int widthinpixels  = 0;
   int bytesperrow    = 0;
   int photointerp    = 0;
   int bpptag         = 0;
   int bytecounttag   = 0;
   int xrestag        = 0;
   int yrestag        = 0;
   int grayrestag     = 0;
   int grayresptr     = 0;
   int softwaretag    = 0;
   int softwareptr    = 0;
   int colormaptag    = 0; 
   int colormapptr    = 0;
   int tif_offset     = 0;
   int bppptr         = 0;   // Pointer to file offset for bpp
   int samplesperpixel= 0;   // No. of colors/pixel (1,3, or 4)
   int rowsperstrip   = 4;   // Rows of pixels/strip
   int noofstrips     = 0;   // Total no.of strips
   int stripoffsettag = 0;   // Tag count when strip offset tag is written
   int stripoffsetptr = 0;   // File offset of strip offset tag
   int strip          = 0;   // Counter for strips
   int totalbytes     = 0;   // Counter for no.of bytes of image data written
   int bytesperimage  = 0;   // Total no.of bytes in the image
   int round_bpp      = 8*((7+g.want_bpp)/8);  // want_bpp rounded up to next 8
   int rr,gg,bb,kk=0;
   int shiftamount    = 0;
   int evenbits;
   int mbpp           = g.bitsperpixel; 
   int alloc1=0;
   int alloc2=0;
   int alloc3=0;
   int alloc4=0;
   int alloc5=0;

   char header[8] = { 0x49,0x49,0x2a,0x00,0x08,0x00,0x00,0x00 }; 
   char zero[4]   = { 0x00,0x00,0x00,0x00 };
   short int RGBbpp[8];

   RGBbpp[0] = g.want_redbpp;
   RGBbpp[1] = g.want_greenbpp;
   RGBbpp[2] = g.want_bluebpp;
   RGBbpp[3] = g.want_blackbpp;

   if(write_all==1)
   {  xstart = z[ci].xpos;
      ystart = z[ci].ypos;
      xend   = xstart + z[ci].xsize-1;
      yend   = ystart + z[ci].ysize-1;
   }
   else
   {  xstart = g.selected_ulx;
      xend   = g.selected_lrx;
      ystart = g.selected_uly;
      yend   = g.selected_lry;
   }

   oldxend = xend;
   frame = z[ci].cf;

   if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
       return CANTCREATEFILE;

   //----Fill tiff_header with image information---------------------//  
 
   lengthinpixels = yend-ystart+1;
   widthinpixels  = xend-xstart+1;

   //---Increase so it is multiple of 8 (scan lines must start on----//
   //---a byte boundary).                                        ----//

   evenbits = ((widthinpixels*g.want_bpp+7)/8)*8; 
   while(widthinpixels*g.want_bpp < evenbits) 
   {   widthinpixels++;
       xend++;
   }
  
   noofstrips = max(1,(lengthinpixels+rowsperstrip-1)/rowsperstrip);

   //------color information-----------------------------------------//

   ////  If entire image is being saved, get bytes using the original  
   ////  bpp of the image. Otherwise, use the screen bpp.             
   ////  Pixel values will be converted into want_bpp bits/pixel.       

   if(write_all) mbpp=z[ci].bpp;
   else mbpp = g.bitsperpixel;

   switch(g.want_bpp)
   {  case 1:  bppfac=1.0;  break;
      case 8:  bppfac=1.0;  break;
      case 15: bppfac=1.875;break;
      case 16: bppfac=2.0;  break;
      case 24: bppfac=3.0;  break;
      case 32: bppfac=4.0;  break;
      case 48: bppfac=6.0;  break;
      default: bppfac=g.want_bpp/8.0; break;
   }  
   intbppfac = ((7+g.want_bpp)/8);

   ////  Determine Tif parameters - samples/pixel, photointerp, etc.  
   ////  Samples/pixel is the highest color, even if intermediate ones  
   ////   have 0 bits/pixel.                                        
   
   bytesperrow    = (int)(widthinpixels * bppfac);
   samplesperpixel = g.want_noofcolors;
   if(g.usegrayscale) photointerp = 0;           //
               else photointerp = 2;             // RGB color

   switch(g.want_color_type)
   {   case 0:                                   // monochrome
             g.want_noofcolors = 1;
             bytesperrow    = widthinpixels/8;
             photointerp    = 1;
             samplesperpixel= 1;
             message("Saving as monochrome");
             break;
       case 1:   
             photointerp    = 1;                 // 8 bpp grayscale
             samplesperpixel= 1;
             break;
       case 2:   
             photointerp    = 3;                 // 8 bpp palette
             samplesperpixel= 1;
             break;
       case 3:   break;                          // 15 bpp
       case 4:                                   // 16 bpp
       case 5:                                   // 24 bpp
             if(g.want_noofcolors==1)
             {  if(g.want_bpp==8) photointerp = 3; else photointerp = 1;
             }
             break;                        
       case 6:
             samplesperpixel = 4;                // 32 bpp
             if(g.save_cmyk) photointerp=5;      // cmyk 
             break;             
       case 7:                                   // 48 bpp
             samplesperpixel = 3;
             break;
       case 8:                                   // custom
             // For case #8, want_noofcolors, want_redbpp, want_bluebpp,
             // and want_greenbpp must be previously set. 
             // If want_noofcolors is 1, they are ignored.
             // If want_noofcolors >1, want_noofcolors must equal the 
             //    no.of colors with non-zero bpp.

             if(g.want_noofcolors==1)
             {  if(g.want_bpp==8) photointerp = 3; else photointerp = 1;
             }
             if((g.save_cmyk)&&(samplesperpixel==4)) photointerp=5;  // cmyk 
             break;
   }
   bytesperimage  = bytesperrow*lengthinpixels;

   //-------arrays---------------------------------------------------//

   stripoffset   = new int[noofstrips];
   if(stripoffset==NULL){ message(g.nomemory,ERROR); goto wtifend;}
   alloc1=1;

   bytesperstrip = new int[noofstrips];
   if(bytesperstrip==NULL){ message(g.nomemory,ERROR); goto wtifend;}
   alloc2=1;

   size1         = rowsperstrip*(int)(intbppfac*widthinpixels)+10;
   size2         = rowsperstrip*(int)(g.off[mbpp]*widthinpixels)+10;
   size          = max(size1,size2);
   bufr          = new uchar[size];
     if(bufr==NULL){ message(g.nomemory,ERROR); goto wtifend;}
     alloc3=1;
   bufr2         = new uchar[size];
     if(bufr2==NULL){ message(g.nomemory,ERROR); goto wtifend;}
     alloc4=1;
   tag           = new taginfo[256];                // Max. of 256 tags
     if(tag==NULL){ message(g.nomemory,ERROR); goto wtifend;}
     alloc5=1;
   bufr[size-1]=99;                    // Sentinel value
   bufr2[size-1]=99;                   // Sentinel value

   ////  tags are 12 bytes each            
   ////  * denotes a pointer to file offset   

   totaltags=0;                        // A global (incremented by nexttag()) 

   //       Tag ID no.
   //       Type(1=byte,2=string,3=16bit int,4=32bit int,5=2 32bit ints)
   //       Count (no. of values)
   //       Value
   nexttag(255,3,1,1);                 // Subfile type - for compatibility
   nexttag(256,4,1,widthinpixels);     // image width
   nexttag(257,4,1,lengthinpixels);    // image length
   
   bpptag = totaltags;                 // fill in bpp ptr later if RGB                                   
   nexttag(258,3,samplesperpixel,g.want_bpp); //*list of bits/sample for all 3 colors 

   nexttag(259,3,1,1);                 // compression (none)
   nexttag(262,3,1,photointerp);       // photometric interp
   nexttag(266,3,1,1);                 // fill order (1)

   stripoffsettag = totaltags;         // fill in actual stripoffsettag later                                       
   nexttag(273,4,noofstrips,stripoffsetptr);   //*ptr to array of strip offsets
   
   nexttag(274,3,1,1);                 // orientation (1)
   nexttag(277,3,1,samplesperpixel);   // samples/pixel(no.of color channels)
   nexttag(278,4,1,rowsperstrip);      // rows/strip   

   bytecounttag = totaltags;           // fill in actual bytecountptr later                                       
   nexttag(279,4,noofstrips,bytecountptr); //*pointer to strip byte count list   

   xrestag = totaltags;                // fill in actual xresptr later
   nexttag(282,5,1,xresptr);           //*pointer to x resolution 

   yrestag = totaltags;                // fill in actual yresptr later
   nexttag(283,5,1,yresptr);           //*pointer to y resolution

   nexttag(284,3,1,1);                 // planar config (1)
                                       // Gray scale or palette color
   if((g.want_color_type==1)||(g.want_color_type==2))
   {  nexttag(290,3,1,3);              // gray response unit (set to 3)
      grayrestag = totaltags;          //*fill in actual grayresptr later
      nexttag(291,3,256,grayresptr);   // gray resp.curve- 512 bytes because short
   }                                   
   nexttag(296,3,1,2);                 // resolution unit 2=inches   
   softwaretag = totaltags;            // fill in actual pointer later
                                       //*pointer to software tag 
   nexttag(305,2,sizeof(softwarestring),softwareptr);    
   if(photointerp==3)                  // Palette color
   {  colormaptag = totaltags;         //*fill in actual colormapptr later
      nexttag(320,3,768,colormapptr);  // color map=1536 bytes because short int
   }
    
   ////  Total no.of tags is now known, so we can calculate offsets.   
   ////  This will finish inserting the data in the tag fields.        
   ////  The data pointed to by these tags will have to be written in  
   ////  the same order as it appears below to preserve the pointers.  

   tif_offset   = 10 + 12 * totaltags + 4;
   xresptr = tif_offset;               // Finish tag 282 ptr to xres 
   tag[xrestag].value = xresptr;  
   tif_offset += 8;

   yresptr = tif_offset;               // Finish tag 283 ptr to yres
   tag[yrestag].value = yresptr;
   tif_offset += 8;
                                       // Finish tag 291 - Gray response ptr
   if((g.want_color_type==1)||(g.want_color_type==2))
   {   grayresptr = tif_offset;
       tag[grayrestag].value = grayresptr;
       tif_offset += 512;
   }
   if(photointerp==3)                  // Finish tag 320 - Palette ptr
   {   colormapptr = tif_offset;
       tag[colormaptag].value = colormapptr;
       tif_offset += 1536;             // Spec says short int not byte!
   }     
   if(g.want_noofcolors>1)             // Finish tag 258 ptr to RGB bpp list
   {   bppptr = tif_offset;
       tag[bpptag].value = bppptr;
       tif_offset += 2*samplesperpixel;
   }
   
   softwareptr = tif_offset;           // Finish Tag 305 ptr to software ID
   tag[softwaretag].value = softwareptr;
   tif_offset += sizeof(softwarestring);

   bytecountptr = tif_offset;          // Finish Tag 279 ptr to byte counts
   if(noofstrips>1)                    // If more than 1 strip, 279 is a ptr
   {  tag[bytecounttag].value = bytecountptr;
      ui = bytesperimage;              // ui is bytes remaining in image
                                       // Calculate bytes/strip
      for(k=0;k<noofstrips;k++)          
      {  bytesperstrip[k] = min((int)ui, rowsperstrip * bytesperrow); 
         ui -= bytesperstrip[k];
         tif_offset += 4;
      }
   }else                               // If only 1 strip, 279 is image size,    
   {                                   // stored in the `value' part of the
                                       // tag, so nothing is added here.
     tag[bytecounttag].value = bytesperimage;
     bytesperstrip[0]=bytesperimage;   // This makes it easier to handle
                                       // but bytesperstrip[0] does not get
                                       // written if there is only 1 strip
   }
  
   //---This one must be last in case there is only 1 strip so the---//
   //---image data offset will be correct.                        ---//

   if(noofstrips>1)                    // Finish Tag 273 strip offsets
   {                                   // If more than 1 strip, 273 is a ptr
                                       // to a list of strip offsets
       stripoffsetptr = tif_offset;    // Finish Tag 273 - strip offsets
       tag[stripoffsettag].value = stripoffsetptr;
       for(k=0;k<noofstrips;k++)
       {  stripoffset[k]= stripoffsetptr + 4*noofstrips + totalbytes;
          totalbytes += bytesperstrip[k];
          tif_offset += 4;
       }
   }else                               // If only 1 strip, 273 is starting  
   {                                   // offset of all image data (stored
                                       // in `value' part of the tag).
       tag[stripoffsettag].value = tif_offset;
       stripoffset[0] = tif_offset;
   }
   
   ////  End of tags - start writing to disk.      
   ////  Must be in the same order as above.   

   fwrite(header,8,1,fp);              // 8 bytes = header + 1st IFD offset
   putword(totaltags,fp);              // 2 bytes = total no.of tags         
   for(k=0;k<totaltags;k++) 
      puttag(tag[k],fp);               // 12 bytes for each tag
   fwrite(zero,4,1,fp);                // 4 zero's

                                       //-----End of IFD's-----------//

   k=300; putdword(k,fp);              // x resolution
   k=1;   putdword(k,fp);
   k=300; putdword(k,fp);              // y resolution
   k=1;   putdword(k,fp);
                                       // Gray response gamma curve
   if((g.want_color_type==1)||(g.want_color_type==2))
   { 
      if(write_all==1)  
      {
         for(k=0;k<256;k++){ shortk=z[ci].gamma[k]; putword(shortk,fp); }
      }else
      {
         for(k=0;k<256;k++){ shortk=k; putword(shortk,fp); }
      }
   }
   if(photointerp==3)                  // Palette (color map)
   {  
      for(j=0;j<256;j++)
      {  palettetemp = (short)(g.palette[j].red*1024);
         putword(palettetemp,fp);
      }
      for(j=0;j<256;j++)
      {  palettetemp = (short)(g.palette[j].green*1024);
         putword(palettetemp,fp);
      }
      for(j=0;j<256;j++)
      {  palettetemp = (short)(g.palette[j].blue*1024);
         putword(palettetemp,fp);
      }
   }
   
   ////  R,G,B bits/pixel as a list of 3 or 4 short ints,
   ////  only if there are 2 or more samples/pixel.        

   if(g.want_noofcolors>1)
   {   for(k=0;k<samplesperpixel;k++)
            putword(RGBbpp[k],fp);
   }
   fwrite(softwarestring,sizeof(softwarestring),1,fp);

   ////  If only 1 strip, its offset is stored in the 12-byte field.    
   ////  Otherwise, store a pointer to here, and write the offsets here.
         
   if(noofstrips>1)
   {   for(k=0;k<noofstrips;k++) putdword(bytesperstrip[k],fp);
       for(k=0;k<noofstrips;k++) putdword(stripoffset[k],fp);
   }

   //  End of writing information - Start writing image              
   //  The first part handles special cases such as cmyk, custom     
   //  bits/pixel, or 15 bpp mode. No temporary image is created.    
   //  No color quantization is done.                                


   shiftamount = max(0,g.want_bpp-g.want_redbpp-g.want_greenbpp-g.want_bluebpp);

   if( ((g.want_color_type==8)||
        (g.want_color_type==3)||
        (g.save_cmyk)         ||
        (mbpp!=g.want_bpp && g.want_bpp!=48)    ||
        (g.usegrayscale))     && (g.want_color_type>0) ) 
   {  
      count = 0;
      for(strip=0; strip<noofstrips; strip++)      
      { y = ystart + strip * rowsperstrip;  
        ui=0;
        ui2=0;
        ui2tot=0;
        dbpp  = mbpp-g.want_bpp;
        for(j=y;j<=min(y+rowsperstrip-1,yend);j++)
        {  getbits(0,bufr,0,RESET);  //reset bits at each scan line
           for(k=xstart;k<=xend;k++)
           {    if(write_all)
                  value = pixelat(z[ci].image[frame][j-ystart] + 
                        g.off[mbpp]*(k-xstart), mbpp);
                else
                  value = readpixelonimage(k,j,mbpp,ino);
                if(k>oldxend) value=0;               
                if(g.want_noofcolors > 1)
                {  
                   valuetoRGB(value,rr,gg,bb,mbpp);
                   value = 0;                   
                   if(g.save_cmyk && g.want_noofcolors==4) // convert to cmyk
                   {   kk = min(g.maxred[mbpp]-rr, min(g.maxgreen[mbpp]-gg,
                                g.maxblue[mbpp]-bb));
                       rr  = g.maxred[mbpp]  -rr-kk;       // r is now cyan
                       gg  = g.maxgreen[mbpp]-gg-kk;       // g is now magenta
                       bb  = g.maxblue[mbpp] -bb-kk;       // b is now yellow
                       kk  = max(0,min(g.maxred[mbpp] ,kk));
                       rr  = max(0,min(g.maxred[mbpp] , rr));
                       gg  = max(0,min(g.maxgreen[mbpp],gg));
                       bb  = max(0,min(g.maxblue[mbpp], bb));
                   }
                   dbpp = g.want_redbpp - g.redbpp[mbpp];
                   if(dbpp>0) rr <<= ( dbpp);
                   if(dbpp<0) rr >>= (-dbpp);
                   value += rr << (g.want_greenbpp + g.want_bluebpp);

                   dbpp = g.want_greenbpp - g.greenbpp[mbpp];    
                   if(dbpp>0) gg <<= ( dbpp);
                   if(dbpp<0) gg >>= (-dbpp);
                   value += gg << g.want_bluebpp;

                   dbpp = g.want_bluebpp - g.bluebpp[mbpp]; 
                   if(dbpp>0) bb <<= ( dbpp);
                   if(dbpp<0) bb >>= (-dbpp);
                   value += bb;

                   if(g.want_noofcolors==4)
                   { 
                     value <<= g.want_blackbpp;                 
                     if(g.save_cmyk)                     
                     {  dbpp = g.want_blackbpp - g.redbpp[mbpp];                  
                        if(dbpp>0) kk <<= ( dbpp);
                        if(dbpp<0) kk >>= (-dbpp);

                        value = 0;                   
                        value += kk << (g.want_redbpp + g.want_greenbpp + g.want_bluebpp);
                        value += bb << (g.want_redbpp + g.want_greenbpp);
                        value += gg << g.want_redbpp;
                        value += rr;
                     }   
                   }
                } 
                else if(g.usegrayscale)   
                {
                       // For treating the data as a deep monochrome image,
                       // value is already the same as luminosity.
                  dbpp = g.want_bpp - mbpp;
                  if(dbpp>0) value <<= ( dbpp);
                  if(dbpp<0) value >>= (-dbpp);// Shift to desired bpp
                }else
                {      // Image is rgb but want 1 color - must reduce 
                       // rgb to luminosity.
                    valuetoRGB(value,rr,gg,bb,mbpp);
                    uir=rr;uig=gg;uib=bb;
                    value = 0;
                    dbpp = 30-g.redbpp[mbpp];    // Shift r,g,b to 0..2^^32
                    if(dbpp>0) uir <<= ( dbpp);// (30 bits for max. precision)
                    if(dbpp<0) uir >>= (-dbpp);
                    dbpp = 30-g.greenbpp[mbpp];
                    if(dbpp>0) uig <<= ( dbpp);
                    if(dbpp<0) uig >>= (-dbpp);
                    dbpp = 30-g.bluebpp[mbpp];
                    if(dbpp>0) uib <<= ( dbpp);
                    if(dbpp<0) uib >>= (-dbpp);
                                               // Convert to luminosity
                    value = (uint)(g.luminr*uir + g.luming*uig + g.luminb*uib);
                    dbpp = 30-g.want_bpp;
                    if(dbpp>0) value >>= ( dbpp);
                    if(dbpp<0) value <<= (-dbpp);// Shift back to desired scale
                }
                putpixelbytes(bufr2+ui2,value,1,round_bpp,1);
                ui2 += intbppfac;
           }   
        
           // ui2 counts input bytes, ui counts output (packed) bytes.
           // They are not necessarily the same value.

           a = packbits(g.want_bpp, bufr2+ui2tot, bufr+ui, intbppfac*widthinpixels); 
           ui += a;
           ui2tot += intbppfac*(xend-xstart+1);

        }       
        count = fwrite(bufr,bytesperstrip[strip],1,fp);
      }  
   }

   // This section handles 48 bits/pixel RGB

   else if(g.want_color_type==7)       
   {  
      for(strip=0; strip<noofstrips; strip++)
      {   y = ystart + strip * rowsperstrip;  
          ui=0;
          for(j=y; j<=min(y+rowsperstrip-1,yend); j++)
          { 
             if(write_all)      
             {  for(k=0;k<bytesperrow;k+=g.off[mbpp])
                {   RGBat(z[ci].image[frame][j-ystart]+k, mbpp, rr, gg, bb);
                    putRGBbytes(bufr+ui, rr, gg, bb, 48);
                    ui += intbppfac;
                }  
             }else 
             {
                for(k=xstart; k<=xend; k++)
                {   readRGBonimage(k,j,mbpp,ino,rr,gg,bb,-2);
                    convertRGBpixel(rr,gg,bb,mbpp,g.want_bpp);
                    putRGBbytes(bufr+ui, rr, gg, bb, 48);
                    ui = (uint)((double)ui + bppfac);
                } 
             }
          }       
          count = fwrite(bufr,bytesperstrip[strip],1,fp);
      }  
   }


   // This section handles palette, grayscale, or RGB color.         
   // The want_bpp to save image must be same as mbpp in memory.     

   else if(g.want_color_type>0)       
   {  
      for(strip=0;strip<noofstrips;strip++)
      { y = ystart + strip * rowsperstrip;  
        ui=0;
        for(j=y;j<=min(y+rowsperstrip-1,yend);j++)
        { 
           if(write_all)      
           { for(k=0;k<bytesperrow;k+=g.off[mbpp])
             {  value = pixelat(z[ci].image[frame][j-ystart]+k,mbpp);
                if(samplesperpixel==4) value<<=shiftamount;
                putpixelbytes(bufr+ui,value,1,mbpp,-1);
                ui+=intbppfac;
             }  
           }else 
           { for(k=xstart;k<=xend;k++)
             {  if(g.want_color_type == g.colortype)
                {   value = readpixelonimage(k,j,mbpp,ino);
                    value = convertpixel(value,mbpp,g.want_bpp,1-g.usegrayscale);
                    if(samplesperpixel==4) value<<=shiftamount;                 
                    putpixelbytes(bufr+ui,value,1,g.want_bpp,-1);
                }    
                ui = (uint)((double)ui + bppfac);
             } 
           }
        }       
        count = fwrite(bufr,bytesperstrip[strip],1,fp);
      }  
   }
   
   // This section handles monochrome mode only. No temporary image  
   // is needed.                                                     
   
   else if(g.want_color_type == 0)   
   {  
      for(strip=0;strip<noofstrips;strip++)
      { y = ystart + strip * rowsperstrip;  
        ui=0;
        for(j=y;j<min(y+rowsperstrip,yend);j++)
             for(k=xstart;k<xend;k+=8) 
                 bufr[ui++]=readbyte(k,j);
        count = fwrite(bufr,bytesperstrip[strip],1,fp);
      } 
   }    
   
wtifend:
   close_file(fp, compress);
   if(count!=1){ status=UNKNOWN; message("Disk error!",ERROR);}
   if(bufr[size-1]!=99)message ("Internal err - bufr",ERROR);
   if(bufr2[size-1]!=99)message("Internal err - bufr2",ERROR);
   if(alloc1) delete[] stripoffset;
   if(alloc2) delete[] bytesperstrip;
   if(alloc3) delete[] bufr;
   if(alloc4) delete[] bufr2;
   if(alloc5) delete[] tag;
   return(status); 
}


//--------------------------------------------------------------------------//
// nexttag                                                                  //
// Puts tag data in tag struct. (used by writetiffile)                      //
//--------------------------------------------------------------------------//
void nexttag(short int tagid, short int type,int length,int value)   
{                           
   tag[totaltags].id    = tagid;
   tag[totaltags].ttype = type;
   tag[totaltags].count = length;
   tag[totaltags].value = value;
   totaltags++;
   if(totaltags>99) message("Internal error at nexttag",ERROR);
}



                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            