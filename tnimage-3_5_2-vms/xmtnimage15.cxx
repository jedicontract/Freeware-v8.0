//--------------------------------------------------------------------------//
// xmtnimage15.cc                                                           //
// Reading & writing of Targa (TGA) files                                   //
// Latest revision: 02-23-2000                                              //
// Copyright (C) 2000 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;


typedef struct TgaHeader
{   uchar   IdLength;            // Image ID Field Length        
    uchar   CmapType;            // Color Map Type               
    uchar   ImageType;           // Image Type                   

    ////  Color Map   
    ushort   CmapIndex;           // First Entry Index            
    ushort   CmapLength;          // Color Map Length             
    uchar    CmapEntrySize;       // Color Map Entry Size         

    ////  Image Specification
    ushort   X_Origin;            // X-origin of Image            
    ushort   Y_Origin;            // Y-origin of Image            
    ushort   ImageWidth;          // Image Width                  
    ushort   ImageHeight;         // Image Height                 
    uchar    PixelDepth;          // Pixel Depth                  
    uchar    ImagDesc;            // Image Descriptor             
} TGAHEADER;


////  Image/Color Map Data
typedef struct TgaColorMap
{   char   *IdField;             // Image ID Field               
    uchar  *CmapData;            // Color Map Data               
} TGACMAP;
              

////  Developer Area Tag Structure
typedef struct TgaTag
{   ushort  TagNumber;           // Number of the Tag            
    uint    TagOffset;           // Offset of the Tag Data       
    uint    TagSize;             // Size of the Tag Data         
    uchar  *TagData;             // Pointer to the Tag Data      
    struct TgaTag *Next;         // Link to next Tag             
} TGATAG;


////  Developer Area
typedef struct TgaDeveloper
{   ushort    NumberOfTags;     // Number of Tags in Directory    
    TGATAG   *TagList;          // Link to list of Tags           
} TGADEVELOPER;


////  Extension Area
typedef struct TgaExtension        
{   ushort   Size;                // Extension Size               
    char     AuthorName[41];      // Author Name                  
    char     AuthorComment[324];  // Author Comment               
    ushort   StampMonth;          // Date/Time Stamp: Month       
    ushort   StampDay;            // Date/Time Stamp: Day         
    ushort   StampYear;           // Date/Time Stamp: Year        
    ushort   StampHour;           // Date/Time Stamp: Hour        
    ushort   StampMinute;         // Date/Time Stamp: Minute      
    ushort   StampSecond;         // Date/Time Stamp: Second      
    char     JobName[41];         // Job Name/ID                  
    ushort   JobHour;             // Job Time: Hours              
    ushort   JobMinute;           // Job Time: Minutes            
    ushort   JobSecond;           // Job Time: Seconds            
    char     SoftwareId[41];      // Software ID                  
    ushort   VersionNumber;       // Software Version Number      
    uchar    VersionLetter;       // Software Version Letter      
    uint     KeyColor;            // Key Color                    
    ushort   PixelNumerator;      // Pixel Aspect Ratio Numerator     
    ushort   PixelDenominator;    // Pixel Aspect Ratio Denominator   
    ushort   GammaNumerator;      // Gamma Value                  
    ushort   GammaDenominator;    // Gamma Value                  
    uint     ColorOffset;         // Color Correction Offset      
    uint     StampOffset;         // Postage Stamp Offset         
    uint     ScanOffset;          // Scan Line Table Offset       
    uchar    AttributesType;      // Attributes Types             
    uint    *ScanLineTable;       // Scan Line Table              
    uchar    StampWidth;          // Width of postage stamp image   
    uchar    StampHeight;         // Height of postage stamp image   
    uchar   *StampImage;          // Postage Stamp Image          
    ushort   ColorTable[1024];    // Color Correction Table       
} TGAEXTENSION;                                  
                                    
                                    
////  TGA File Footer
typedef struct TgaFooter
{   uint ExtensionOffset;      // Extension Area Offset        
    uint DeveloperOffset;      // Developer Directory Offset   
    char  Signature[18];       // Signature, dot, and NULL     
} TGAFOOTER;
                            

////  Complete TGA File Format
typedef struct TgaFormat
{   TGAHEADER      Head;            // Header Area                            
    TGACMAP        Cmap;            // Image/Color Map Area                   
    TGADEVELOPER   Developer;       // Developer Area                         
    TGAEXTENSION   Extension;       // Extension Area                         
    TGAFOOTER      Foot;            // File Footer Area                       
    int            NewTgaFormat;    // TRUE if file is v2.0 TGA format        
    uint           TgaDataOffset;   // Offset of the image data in the file    
} TGA;

#define TGASIGNATURE   "TRUEVISION-XFILE.\0"
#define BYTESPERPIXEL   ((tgaHead.Head.PixelDepth + 7) >> 3)




int bad=0;

short ReadTgaHeader(TGA* TgaHead,FILE* FpTga);
short tgacodeline(
   uchar *inbuffer,           // Input buffer  
   uchar *outbuf,             // Pointer to buffer to hold encoded scan line
   int  linelength,           // The length of a scan line in pixels
   ushort  pixelsize);        // The number of bytes in a pixel
short TgaDecodeScanLine(
   uchar *DecodedBuffer,      // Pointer to buffer to hold decoded data          
   ushort  LineLength,        // The length of a scan line in pixels             
   ushort  PixelSize,         // The number of bytes in a pixel                  
   FILE *FpTga);              // FILE pointer to the open input TGA image file   
void WriteTgaFooter(TGA *TgaHead, FILE *FpTga);
void WriteTgaHeader(TGA *TgaHead, FILE *FpTga);


//--------------------------------------------------------------------------//
// writetgafile                                                             //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
int writetgafile(char *filename, int write_all, int compress)
{
  if(memorylessthan(16384)){  message(g.nomemory,ERROR); return(NOMEM); } 

  int want_compress_tga = 1;
  //-------Common to all image writing functions--------//
  FILE *fp;
  int i,k,k2;
  int pbpp,bpp = g.want_bpp;
  int ino;

  uint val;
  int xstart,ystart,xend,yend;
  int xsize,ysize;
  
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

  xsize=xend-xstart+1;
  ysize=yend-ystart+1;
  double v = atof(g.version);
  int ver = (int)(v*100);
  
  //-------Tga-specific code----------------------------//
  
   TGA         tgaHead;        // TGA image file header structure 
   ushort      bufSize;        // Size of the scan line buffer    
   short       byteCount;      // Number of bytes in a buffer     
   uint        imageSize;      // Size of the image data in bytes 
   uchar       *inbuffer;      // Buffer to hold raw scan line data 
   uchar       *outbuffer;     // Buffer to hold compressed scan line

   imageSize = 0;              // Initialize image data byte count 

   switch(bpp)
   {  case  8: 
         if(g.want_color_type==1) 
               tgaHead.Head.ImageType = 11;       // RLE, monochrome
         else
               tgaHead.Head.ImageType = 9;        // RLE, color mapped
         break;
      case 15: tgaHead.Head.ImageType =10; break; // RLE, True color
      case 16: tgaHead.Head.ImageType =10; break;
      case 24: tgaHead.Head.ImageType =10; break;
      case 32: tgaHead.Head.ImageType =10; break;
      default: message("Internal error 15a",ERROR);
   }

   tgaHead.Head.IdLength =0;

   if(g.want_color_type==2) 
   {  tgaHead.Head.CmapType   =1;      // 1=has palette
      tgaHead.Head.CmapIndex  =0;      // 1st entry no. in palette
      tgaHead.Head.CmapLength =256;    // no.of entries
      tgaHead.Head.CmapEntrySize =24;  // no.of bits/entry
   }else   
   {  tgaHead.Head.CmapType   =0;
      tgaHead.Head.CmapIndex  =0;
      tgaHead.Head.CmapLength =0;
      tgaHead.Head.CmapEntrySize =0;
   }
   tgaHead.Head.X_Origin   =0;
   tgaHead.Head.Y_Origin   =0;
   tgaHead.Head.ImageWidth = xsize;
   tgaHead.Head.ImageHeight= ysize;
   tgaHead.Head.PixelDepth = bpp;
   tgaHead.Head.ImagDesc   =0;    // Bottom to Top

   tgaHead.Cmap.IdField    =0;
   if(g.want_color_type==2) 
      tgaHead.Cmap.CmapData   =(BYTE *)malloc(780);
   else
      tgaHead.Cmap.CmapData   =0;

   tgaHead.Developer.NumberOfTags =0;
   tgaHead.Developer.TagList      =0;

   tgaHead.Extension.Size        =0;
   strcpy(tgaHead.Extension.AuthorName,"");
   strcpy(tgaHead.Extension.AuthorComment,"");
   tgaHead.Extension.StampMonth  =0;
   tgaHead.Extension.StampDay    =0;
   tgaHead.Extension.StampYear   =0;
   tgaHead.Extension.StampHour   =0;
   tgaHead.Extension.StampMinute =0;
   tgaHead.Extension.StampSecond =0;
   strcpy(tgaHead.Extension.JobName,"");
   tgaHead.Extension.JobHour     =0;
   tgaHead.Extension.JobMinute   =0;
   tgaHead.Extension.JobSecond   =0;
   strcpy(tgaHead.Extension.SoftwareId,"tnimage");
   tgaHead.Extension.VersionNumber = ver;
   tgaHead.Extension.VersionLetter = ' ';
   tgaHead.Extension.KeyColor      =0;
   tgaHead.Extension.PixelNumerator=0;
   tgaHead.Extension.PixelDenominator=0;
   tgaHead.Extension.GammaNumerator=0;
   tgaHead.Extension.GammaDenominator=0;
   tgaHead.Extension.ColorOffset     =0;
   tgaHead.Extension.StampOffset     =0;
   tgaHead.Extension.ScanOffset      =0;
   tgaHead.Extension.AttributesType  =0;
   tgaHead.Extension.ScanLineTable   =0;
   tgaHead.Extension.StampWidth      =0;
   tgaHead.Extension.StampHeight     =0;
   tgaHead.Extension.StampImage      =0;
//   tgaHead.Extension.ColorTable    =0;   // 1024 bytes max -color corr.table

   tgaHead.Foot.ExtensionOffset    =0;
   tgaHead.Foot.DeveloperOffset    =0;
   strcpy(tgaHead.Foot.Signature,TGASIGNATURE);
                                    
   tgaHead.NewTgaFormat       =TRUE;
   tgaHead.TgaDataOffset         =0;
                            

   //----------------------------------------------------------------------//
   // Calculate the encoded buffer size in bytes.  The worst case size     //
   // is based on every pixel in the scan line being encoded as a          //
   // single, raw run-length packet.                                       //
   //----------------------------------------------------------------------//

   bufSize = tgaHead.Head.ImageWidth + // Number of packet count bytes   
            (tgaHead.Head.ImageWidth * // Number of pixels               
             BYTESPERPIXEL+32);        // Number of bytes per pixel      

      // Allocate buffer memory   
   if((inbuffer=(uchar *)calloc(bufSize,sizeof(uchar)))==(uchar *) NULL)
   {   message(g.nomemory,ERROR);
       return(ABORT);
   }
   if((outbuffer=(uchar *)calloc(bufSize,sizeof(uchar)))==(uchar *) NULL)
   {   message(g.nomemory,ERROR);
       return(ABORT);
   }
   inbuffer[bufSize-1]=99;  
   outbuffer[bufSize-1]=99;  
   WriteTgaHeader(&tgaHead,fp); 

   if(g.want_color_type==2) free(tgaHead.Cmap.CmapData);

                             // Encode a scan line and write it to the file   

   for(i=tgaHead.Head.ImageHeight-1;i>=0;i--)
   {
      for(k=0,k2=0;k<xsize;k++,k2+=g.off[bpp])
      {  val = readpixelonimage(k+xstart,i+ystart,pbpp,ino); //pbpp,ino by ref
         if(pbpp!=bpp) val=convertpixel(val,pbpp,bpp,1);
         putpixelbytes(inbuffer+k2,val,1,bpp);
      }
      if((byteCount=tgacodeline(inbuffer,outbuffer,xsize,BYTESPERPIXEL))<0)
      {  message("Error encoding Targa file!",ERROR);
         return(ABORT);
      }

      if(want_compress_tga)
        fwrite(outbuffer,byteCount,1,fp);
      else 
        fwrite(inbuffer,xsize*BYTESPERPIXEL,1,fp);
      imageSize += byteCount;          //Update the byte count   
   }

   WriteTgaFooter(&tgaHead,fp); 
   if((inbuffer[bufSize-1]!=99)||(outbuffer[bufSize-1]!=99))
     message("Internal error 15b",ERROR);

  //----------------------------------------------------//
  close_file(fp, compress);  
  free(outbuffer);
  free(inbuffer);
  return(OK); 
}



//--------------------------------------------------------------------------//
// readtgafile                                                              //
//--------------------------------------------------------------------------//
int readtgafile(char *filename)
{
   int status=OK;
   int j,j1,k,xsize,ysize,depth,x1,y1,ct;
   int direction=0;
   char junk;
   RGB palette[256];

   //---------------Targa-specific code------------------------
   int i;
   ushort      bufSize;        // Size of the scan line buffer               
   short       byteCount;      // Number of bytes in a buffer                
   uint        imageSize;      // Size of the image data in bytes            
   uchar       *buffer;        // Buffer to hold scan line data              
   FILE        *fp;            // TGA image file input FILE stream           
   TGA         tgaHead;        // TGA image file header structure            

   imageSize = 0;              // Initialize image data byte count   
   if ((fp=fopen(filename,"rb")) == NULL)
   {   error_message(filename, errno);
       return(NOTFOUND);
   }

   if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
   if(ReadTgaHeader(&tgaHead, fp)){ status=NOMEM; return(status);  }
   if(ferror(fp)) { status=ERROR; return(status); }
   memcpy(palette, g.palette, 768);

   xsize= tgaHead.Head.ImageWidth;
   ysize= tgaHead.Head.ImageHeight;
   depth= tgaHead.Head.PixelDepth;
   x1= tgaHead.Head.X_Origin;
   y1= tgaHead.Head.Y_Origin;
   direction=0x30 & tgaHead.Head.ImagDesc; // 0=bottom to top 32=top to bottom

                                           // "16 bit" TGA's are actually 15 bpp
   if(depth>8) ct=COLOR; else ct=INDEXED;

   if(newimage(basefilename(filename), x1+1, y1+33, xsize, ysize, depth, ct,
       1, g.want_shell, 1, PERM, 1,
       g.window_border, 0)!=OK)  return(NOMEM); 

   memcpy(z[ci].palette, palette, 768);
   memcpy(z[ci].opalette, palette, 768);
   memcpy(z[ci].spalette, palette, 768);
   setpalette(palette);
                                        // Seek to the image data   
   fseek(fp,g.tif_skipbytes+tgaHead.TgaDataOffset,SEEK_SET); 

    if (tgaHead.Head.ImageType == 9 || tgaHead.Head.ImageType == 10 ||
        tgaHead.Head.ImageType == 11)
    {
        // Calculate the decoded buffer size in bytes.  
        bufSize = (32 + tgaHead.Head.ImageWidth * BYTESPERPIXEL); 
        if((buffer=(uchar *)calloc(bufSize, sizeof(uchar))) == (uchar *) NULL)
        {   message(g.nomemory,ERROR);
            fclose(fp);
            return(NOMEM);
        }
               // Decode a scan line   
        byteCount = tgaHead.Head.ImageWidth * BYTESPERPIXEL;

        if(direction==0)   // Tga images can be stored bottom to top
        { for(k=tgaHead.Head.ImageHeight-1;k>=0; k--)
          { 
            if ((byteCount =
                 TgaDecodeScanLine(buffer, tgaHead.Head.ImageWidth,
                                   BYTESPERPIXEL, fp)) < 0)
            {   status=ERROR; 
                message("Error decoding Targa file",ERROR);
                break; 
            }
            memcpy(z[ci].image[z[ci].cf][k],buffer,byteCount);
            imageSize += byteCount;
          }
        }else
        { for(k=0;k<tgaHead.Head.ImageHeight;k++)
          { if ((byteCount =
                 TgaDecodeScanLine(buffer, tgaHead.Head.ImageWidth,
                                   BYTESPERPIXEL, fp)) < 0)
            {   status=ERROR; 
                break; 
            }
            memcpy(z[ci].image[z[ci].cf][k],buffer,byteCount);
            imageSize += byteCount;
          }
        }

        // Change image type to "unencoded"   
        switch(tgaHead.Head.ImageType)
        {
            case 9:     
                tgaHead.Head.ImageType = 1;     // Unencoded, color-mapped    
                break;
            case 10:
                tgaHead.Head.ImageType = 2;     // Unencoded, true-color      
                break;
            case 11:
                tgaHead.Head.ImageType = 3;     // Unencoded, monochrome      
                break;
        }
    }else if (tgaHead.Head.ImageType == 1 || tgaHead.Head.ImageType == 2 ||
        tgaHead.Head.ImageType == 3)
    {   byteCount = tgaHead.Head.ImageWidth * BYTESPERPIXEL;
        if(direction==0)
        { for(i=tgaHead.Head.ImageHeight-1;i>=0; i--)
          {   fread(z[ci].image[z[ci].cf][i],byteCount,1,fp); 
              imageSize += byteCount;
          }
        }else
        { for(i=0;i<tgaHead.Head.ImageHeight; i++)
          {   fread(z[ci].image[z[ci].cf][i],byteCount,1,fp); 
              imageSize += byteCount;
          }
        }
    }else
    {   message("Error reading Targa image",ERROR);
        status=ABORT;
    }


    ////  Correct "16 bit" 15-bit pixels to 16 bits.
    if(depth==16)
    {  for(i=0;i<tgaHead.Head.ImageHeight; i++)
       for(j=0,j1=0;j<tgaHead.Head.ImageWidth;j++,j1+=2)
           z[ci].image[z[ci].cf][i][j1+1] *=2;
    }

   //----------------------------------------------------------
   fclose(fp);
   if(bad) message("Errors in Targa file",ERROR);
   bad=0;
   return(status);
}



//--------------------------------------------------------------------------//
// tgaread                                                                  //
// read TGA header into a TGAHEADER structure                               //
//                                                                          //
//   The TGAHEADER structure contains all the information found in both     //
//   version 1.0 and 2.0 of the TGA format except for the image data.       //
//   Image data is not stored because of its  typically large size.         //
//                                                                          //
//   Returns: A negative value if a memory allocation error occured,        //
//            otherwise 0 if no errors occur.                               //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
short ReadTgaHeader(
  TGA  *TgaHead,               // Pointer to TGA header structure  
  FILE *FpTga)                 // TGA image file input FILE stream 
{
    int     i,i2,j;            // Loop counters  
    ushort    cmapsize;          // Size of the color map in bytes
    ushort    stampsize;         // Size of the postage stamp image in bytes
    TGATAG *tag;               // TGA tag structure  
    TGATAG *head;              // Head of TGA tag linked list 
    TGATAG *current_tag=NULL;  // Current node of linked list

                               // Read the TGA file header

    if((TgaHead->Head.IdLength = getbyte(FpTga))==255)
    {   message("Error reading TGA file"); return -1; }
    TgaHead->Head.CmapType      = getbyte(FpTga);
    TgaHead->Head.ImageType     = getbyte(FpTga);
    TgaHead->Head.CmapIndex     = getword(FpTga);
    TgaHead->Head.CmapLength    = getword(FpTga);
    TgaHead->Head.CmapEntrySize = getbyte(FpTga);
    TgaHead->Head.X_Origin      = getword(FpTga);
    TgaHead->Head.Y_Origin      = getword(FpTga);
    TgaHead->Head.ImageWidth    = getword(FpTga);
    TgaHead->Head.ImageHeight   = getword(FpTga);
    TgaHead->Head.PixelDepth    = getbyte(FpTga);
    TgaHead->Head.ImagDesc      = getbyte(FpTga);

                               // Read the Image ID field. 
    if (TgaHead->Head.IdLength)
    {
                               // Allocate memory for the Id Field data 
        if ((TgaHead->Cmap.IdField =
            (char *) calloc(TgaHead->Head.IdLength, sizeof(char))) ==
            (char *) NULL)  return(-1);        // Failed to allocate memory 

                               // Read the Image ID data 
                               // Not necessarily a NULL-terminated string
        for(i=0; i<TgaHead->Head.IdLength; i++)
            TgaHead->Cmap.IdField[i] = (char) getbyte(FpTga);
    }

                               // Read color map data (Version 1.0 and 2.0)

    if (TgaHead->Head.CmapType)
    {
        // Determine the size of the color map   
        cmapsize = ((TgaHead->Head.CmapEntrySize + 7) >> 3) *
                     TgaHead->Head.CmapLength;

        // Allocate memory for the color map data   
        if ((TgaHead->Cmap.CmapData =
            (uchar *) calloc(cmapsize, sizeof(uchar))) == (uchar *) NULL)
             return(-2);       // No memory

                               // Read the color map data 
        for(i=0; i<cmapsize; i++)
           TgaHead->Cmap.CmapData[i] = getbyte(FpTga);
        for(i=0,i2=0; i2<min(768,cmapsize); i++,i2+=3)
        {  g.palette[i].red  = TgaHead->Cmap.CmapData[i2+2] >>2; 
           g.palette[i].green= TgaHead->Cmap.CmapData[i2+1] >>2; 
           g.palette[i].blue = TgaHead->Cmap.CmapData[i2  ] >>2; 
        }
    }

    // Store the offset of the image data.  This field is not part of
    // the TGA format, but it helps the software locate the data when
    // reading/writing TGA files.
      
    TgaHead->TgaDataOffset = ftell(FpTga);

    // Check the version of the TGA file (Versions 1.0 and 2.0).
      
    fseek(FpTga, -26, SEEK_END);  // Seek to the (possible) TGA footer   

    // Read in the (possible) offset values   
    TgaHead->Foot.ExtensionOffset = getdword(FpTga);
    TgaHead->Foot.DeveloperOffset = getdword(FpTga);

    // Read in the (possible) signature   
    for(i=0; i<(int)sizeof(TgaHead->Foot.Signature); i++)
        TgaHead->Foot.Signature[i] = (char) getbyte(FpTga);

    // Check if the data read is a TGA signature string   
    if(!strcmp(TgaHead->Foot.Signature, TGASIGNATURE))
        TgaHead->NewTgaFormat = TRUE;   // Yes.  Version 2.0   
    else
        TgaHead->NewTgaFormat = FALSE;  // No.  Version 1.0   

    // If version is 2.0 then check for developer and extension areas.
    // Don't need tif_skipbytes here since offsets are from end of file.
      
    if (TgaHead->NewTgaFormat == TRUE)
    {
        // Check for the presence of a developers area   
        if (TgaHead->Foot.DeveloperOffset)
        {
            // Seek to the developer area   
            fseek(FpTga, TgaHead->Foot.DeveloperOffset, SEEK_SET);

            // Get the number of tags in the directory   
            TgaHead->Developer.NumberOfTags = getword(FpTga);

            // Read the tags and store as a singly-linked list   
            head = (TGATAG *) NULL;
            for(i=0; i<TgaHead->Developer.NumberOfTags; i++)
            {
                // Allocate the tag   
                if ((tag =
                    (TGATAG *) malloc(sizeof(TGATAG))) == (TGATAG *) NULL)
                    return(-3);     // Failed to allocate memory   
                    
                // Read the tag information   
                tag->TagNumber = getword(FpTga);
                tag->TagOffset = getdword(FpTga);
                tag->TagSize   = getdword(FpTga);

                //
                // Read the tag data
                  
                // Seek to the tag data   
                fseek(FpTga, tag->TagOffset, SEEK_SET);

                // Read the tag data   
                for(j=0; j<(int)tag->TagSize; j++)
                    tag->TagData[j] = getbyte(FpTga);

                // Link the tag to the list   
                if (head == (TGATAG *) NULL)
                {
                    head = tag;
                    current_tag = head;
                }
                else
                {
                    current_tag->Next = tag;
                    tag->Next = (TGATAG *) NULL;
                    current_tag = tag;
                }
            }
            TgaHead->Developer.TagList = head;  // Assign list to structure   
        }

        // Check for the presence of an extension area   
        if (TgaHead->Foot.ExtensionOffset)
        {
            // Seek to the extension area   
            fseek(FpTga, TgaHead->Foot.ExtensionOffset, SEEK_SET);

            // Read the extension area size information   
            TgaHead->Extension.Size = getword(FpTga);

            // Read the extension information defined in TGA version 2.0   
            for(i=0; i<(int)sizeof(TgaHead->Extension.AuthorName); i++)
                TgaHead->Extension.AuthorName[i] = (char) getbyte(FpTga);

            for(i=0; i<(int)sizeof(TgaHead->Extension.AuthorComment); i++)
                TgaHead->Extension.AuthorComment[i] = (char) getbyte(FpTga);

            TgaHead->Extension.StampMonth  = getword(FpTga);     
            TgaHead->Extension.StampDay    = getword(FpTga);       
            TgaHead->Extension.StampYear   = getword(FpTga);      
            TgaHead->Extension.StampHour   = getword(FpTga);      
            TgaHead->Extension.StampMinute = getword(FpTga);    
            TgaHead->Extension.StampSecond = getword(FpTga);

            for(i=0; i<(int)sizeof(TgaHead->Extension.JobName); i++)
                TgaHead->Extension.JobName[i] = (char) getbyte(FpTga);

            TgaHead->Extension.JobHour     = getword(FpTga);        
            TgaHead->Extension.JobMinute   = getword(FpTga);      
            TgaHead->Extension.JobSecond   = getword(FpTga);

            for(i=0; i<(int)sizeof(TgaHead->Extension.SoftwareId); i++)
                TgaHead->Extension.SoftwareId[i] = (char) getbyte(FpTga);

            TgaHead->Extension.VersionNumber    = getword(FpTga);  
            TgaHead->Extension.VersionLetter    = getbyte(FpTga);  
            TgaHead->Extension.KeyColor         = getdword(FpTga);       
            TgaHead->Extension.PixelNumerator   = getword(FpTga); 
            TgaHead->Extension.PixelDenominator = getword(FpTga);
            TgaHead->Extension.GammaNumerator   = getword(FpTga); 
            TgaHead->Extension.GammaDenominator = getword(FpTga);
            TgaHead->Extension.ColorOffset      = getdword(FpTga);    
            TgaHead->Extension.StampOffset      = getdword(FpTga);    
            TgaHead->Extension.ScanOffset       = getdword(FpTga);     
            TgaHead->Extension.AttributesType   = getbyte(FpTga);

            // Check for the presence of a scan line table   
            if (TgaHead->Extension.ScanOffset)
            {
                // Seek to the postage scan line table   
                fseek(FpTga, TgaHead->Extension.ScanOffset, SEEK_SET);
                
                // Allocate memory.  One uint per line in image   
                if ((TgaHead->Extension.ScanLineTable =
                    (uint *) calloc(TgaHead->Head.ImageHeight, sizeof(uint))) ==
                    (uint *) NULL)
                {
                    return(-4);     // Failed to allocate memory   
                }

                // Read in scan line offset values   
                for(i=0; i<TgaHead->Head.ImageHeight; i++)
                    TgaHead->Extension.ScanLineTable[i] = getdword(FpTga);
            }

            // Check for the presence of a postage stamp image  
            if (TgaHead->Extension.StampOffset)
            {
                // Seek to the postage stamp image   
                fseek(FpTga, TgaHead->Extension.StampOffset, SEEK_SET);

                // Read the size of the stamp in pixels   
                TgaHead->Extension.StampWidth  = getbyte(FpTga);
                TgaHead->Extension.StampHeight = getbyte(FpTga);

                // Calculate the size of the stamp in bytes   
                stampsize = TgaHead->Extension.StampWidth *
                            TgaHead->Extension.StampHeight *
                            ((TgaHead->Head.PixelDepth + 7) >> 3);

                // Allocate memory for the postage stamp image   
                if ((TgaHead->Extension.StampImage =
                    (uchar *) calloc(stampsize, sizeof(uchar))) ==
                    (uchar *) NULL)
                {
                    return(-5);     // Failed to allocate memory   
                }

                //// Read the stamp data one byte at a time 
                for(i=0; i<stampsize; i++)
                    TgaHead->Extension.StampImage[i] = getbyte(FpTga);
            }

            //// Check for the presence of a color correction table 
            if (TgaHead->Extension.ColorOffset)
            {
                ///// Seek to the color correction table
                fseek(FpTga, TgaHead->Extension.ColorOffset, SEEK_SET);

                //// Read in the entire 1024 ushort (2048 byte) table 
                for(i=0; i<(int)sizeof(TgaHead->Extension.ColorTable); i++)
                    TgaHead->Extension.ColorTable[i] = getword(FpTga);
            }
        }
    }
    return(0);
}
   
uchar getbyte(FILE* fp){  return(fgetc(fp)); }
void putbyte(uchar c, FILE* fp){ fputc(c,fp); }
ushort getword(FILE* fp)  

{
#ifdef LITTLE_ENDIAN
   return getlittleword(fp);
#else 
   return getbigword(fp);
#endif
}

uint getdword(FILE* fp) 
{
#ifdef LITTLE_ENDIAN
   return getlittledword(fp);
#else 
   return getbigdword(fp);
#endif
}

ushort getlittleword(FILE* fp) 
{  ushort i;
   fread(&i,2,1,fp); 
   return i;
}

uint getlittledword(FILE* fp) 
{  uint i;
   fread(&i,4,1,fp); 
   return i;
}

ushort getbigword(FILE* fp) 
{  ushort i;
   fread(&i,2,1,fp); 
   swapbytes((uchar*)&i, 2, 2);
   return i;
}

uint getbigdword(FILE* fp)       
{  uint i;
   fread(&i,4,1,fp); 
   swapbytes((uchar*)&i, 4, 4); 
   return i;
}


void putword(ushort i, FILE* fp)  
{
#ifdef LITTLE_ENDIAN
   putlittleword(i, fp);
#else 
   putbigword(i, fp);
#endif
}

void putdword(uint i, FILE* fp)  
{
#ifdef LITTLE_ENDIAN
   putlittledword(i, fp);
#else 
   putbigdword(i, fp);
#endif
}

void puttag(taginfo t, FILE* fp)  
{
#ifdef LITTLE_ENDIAN
   putlittletag(t, fp);
#else 
   putbigtag(t, fp);
#endif
}

void putlittleword(ushort i, FILE* fp){ fwrite(&i,2,1,fp); }
void putlittledword(uint i, FILE* fp){ fwrite(&i,4,1,fp); }
void putlittletag(taginfo t, FILE* fp){ fwrite(&t, sizeof(taginfo), 1, fp); }
void putbigword(ushort i, FILE* fp)
{ 
    short uint a = i;
    swapbytes((uchar*)&a, 2, 2);
    fwrite(&a,2,1,fp); 
}
void putbigdword(uint i, FILE* fp)
{   
    uint a = i;
    swapbytes((uchar*)&a, 4, 4);
    fwrite(&a,4,1,fp); 
}

void putbigtag(taginfo tag, FILE* fp)
{   
    taginfo t;
    memcpy((void*)&t, (void*)&tag, 12);
    swapbytes((uchar*)&t.id, 2, 2);
    swapbytes((uchar*)&t.ttype, 2, 2);
    swapbytes((uchar*)&t.count, 4, 4);
    swapbytes((uchar*)&t.value, 4, 4);
    fwrite(&t, 12, 1, fp); 
}

void putstring(char *s, int count, FILE* fp)
{   
    fwrite(s,count,1,fp); 
}

void putpixel(uint v, int size, FILE* fp)
{
    switch(size)
    {    case 1: putbyte((uchar)v,fp);break;
         case 2: putword(v,fp);break;
         case 3: putstring((char*)&v,3,fp); break;  
         case 4: putdword(v,fp);break;
         default:
#ifndef LITTLE_ENDIAN
                 swapbytes((uchar*)&v, size, size);
#endif
                 fwrite((uchar*)&v, size, 1, fp);         
    }
}


//--------------------------------------------------------------------------//
// tgacodeline                                                              //
// Encode a TGA image file RLE coded scan line.                             //
// Returns the no. of bytes put in bufr, or negative value if error         //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
short tgacodeline(uchar *inbuffer, uchar *outbuf, int linelength, ushort pixelsize) 
{
    int i,k,k2;                // Loop Counter 
    uchar p1[4],p2[4];         // Previous and current pixel 
    int pos=0;                 // Input buffer position
    int index=0;               // Output buffer position
    int size=pixelsize;        // Bytes/pixel
    int count=0;               // No.of pixels processed

    int same=0;
    int raw=1;
    int rawstart=0;
    int match=0;               // Flag if 2 pixels match

    for(i=0;i<size;i++) p1[i]=inbuffer[pos++];    // Pixel #0
    do
    {  match=1;
       for(i=0;i<size;i++){ p2[i]=inbuffer[pos++]; if(p2[i]!=p1[i])match=0; } 
       count++;    
       
       if((match==1)||(raw>=127))  // A run of identical pixels
       {  if(raw>1)                // Save the run of different pixels
          {  
              outbuf[index++] = (count-rawstart-2) & 0x7f;
             for(k=rawstart;k<count-1;k++)
             {   for(k2=0;k2<size;k2++) 
                    outbuf[index++]=inbuffer[k*size+k2];
             }    
          }
          raw=0;
          if(match==1)same++; else raw--;
          rawstart=0; 
       }                          
       if((match==0)||(same>=127)) // A run of different pixels
       {  if(same>0)               // Save the run of identical pixels
          {  outbuf[index++] = (same|0x80);
             for(k=0;k<size;k++) outbuf[index++]=p1[k];
             if(rawstart==0) rawstart=count;
          }
          same=0; 
          if(match==0)raw++; else same--;  
       }       

       if(count+1>=linelength) 
       {  if(same>0)               // Save the run of identical pixels
          {  outbuf[index++] = same|0x80;
             for(k=0;k<size;k++) outbuf[index++]=p2[k];
          }
          if(raw>0)                // Save the run of different pixels
          {  outbuf[index++] = (count-rawstart) & 0x7f;
             for(k=rawstart;k<=count;k++)
             {  for(k2=0;k2<size;k2++) 
                  outbuf[index++]=inbuffer[k*size+k2];
             }    
          }
          break;
       }       
       for(i=0;i<size;i++) p1[i]=p2[i];
    }while(1);
    return (index);
}



//--------------------------------------------------------------------------//
// tgadecode                                                                //
// Decode a TGA image file RLE coded scan line.                             //
// Returns 0 , or negative value if error                                   //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
short TgaDecodeScanLine(uchar *DecodedBuffer, ushort LineLength, ushort PixelSize,   
      FILE *FpTga)         
{
    ushort    i;              // Loop counter                                   
    short     byteCount;      // Number of bytes written to the buffer          
    ushort    runCount;       // The pixel run count                            
    ushort    bufIndex;       // The index of DecodedBuffer                     
    ushort    bufMark;        // Index marker of DecodedBuffer                  
    ushort    pixelCount;     // The number of pixels read from the scan line   

    bufIndex   = 0;         // Initialize buffer index    
    byteCount  = 0;         // Initialize byte count      
    pixelCount = 0;         // Initialize pixel counter   

    //// Main decoding loop 

    while(pixelCount < LineLength)
    {   runCount = getbyte(FpTga);   // Get the pixel count 
     
        //// Make sure writing this next run will not overflow the buffer 
        if(pixelCount + (runCount & 0x7f)  > LineLength)
        {     bad=1;
              return(LineLength);    // Don't let pixel count overflow bufr 
        }
        //// If the run is encoded...
        if (runCount & 0x80)
        {   runCount &= ~0x80;       // Mask off the upper bit
            bufMark = bufIndex;      // Save the start-of-run index  
            pixelCount += (runCount + 1);  // Update total pixel count 
            byteCount += ((runCount + 1) * PixelSize); // Update the buffer byte count 

            //// Write the first pixel of the run to the buffer 
            for(i=0; i< PixelSize; i++)
                DecodedBuffer[bufIndex++] = getbyte(FpTga);

            //// Write remainder of pixel run to buffer 'runCount' times 
            while(runCount--)
            {   for(i=0; i<PixelSize; i++)
                    DecodedBuffer[bufIndex++] = DecodedBuffer[bufMark + i];
            }

        }
        else    //// the run is unencoded (raw)
        {   pixelCount += (runCount + 1);    // Update total pixel count 
            byteCount  += ((runCount + 1) * PixelSize);  // Update the buffer byte count 
            do            // Write runCount pixels 
            {   for(i=0; i<PixelSize; i++)
                    DecodedBuffer[bufIndex++] = getbyte(FpTga);
            }while(runCount--);
        }
    }
    return(byteCount);
}


//--------------------------------------------------------------------------//
// tgawriteheader                                                           //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
void WriteTgaHeader(TGA *TgaHead, FILE *FpTga)
{                                                                        
    int i,i2;
    ushort cmapsize;     //// Size of the color map in bytes 

    //// Write the TGA header (Version 1.0 and 2.0).

    putbyte(TgaHead->Head.IdLength, FpTga);
    putbyte(TgaHead->Head.CmapType, FpTga);
    putbyte(TgaHead->Head.ImageType, FpTga);
    putword(TgaHead->Head.CmapIndex, FpTga);
    putword(TgaHead->Head.CmapLength, FpTga);
    putbyte(TgaHead->Head.CmapEntrySize, FpTga);
    putword(TgaHead->Head.X_Origin, FpTga);
    putword(TgaHead->Head.Y_Origin, FpTga);
    putword(TgaHead->Head.ImageWidth, FpTga);
    putword(TgaHead->Head.ImageHeight, FpTga);
    putbyte(TgaHead->Head.PixelDepth, FpTga);
    putbyte(TgaHead->Head.ImagDesc, FpTga);


    ////  Write the Image ID field. 
    if(TgaHead->Head.IdLength)
        for(i=0; i<TgaHead->Head.IdLength; i++)
            putbyte(TgaHead->Cmap.IdField[i], FpTga);

    ////  Write the color map data (Version 1.0 and 2.0).
    if(TgaHead->Head.CmapType)
    {  //// Determine the size of the color map 
        cmapsize = ((TgaHead->Head.CmapEntrySize + 7) >> 3) *
                     TgaHead->Head.CmapLength;

        //// Write the color map data 
        for(i=0,i2=0; i<256; i++,i2+=3)
        {   TgaHead->Cmap.CmapData[i2+2] = (uchar)g.palette[i].red * 4;        
            TgaHead->Cmap.CmapData[i2+1] = (uchar)g.palette[i].green * 4;        
            TgaHead->Cmap.CmapData[i2  ] = (uchar)g.palette[i].blue * 4;        
        }
        for(i=0; i<cmapsize; i++)
            putbyte(TgaHead->Cmap.CmapData[i], FpTga);
    }
}


//--------------------------------------------------------------------------//
// tgawritefooter                                                           //
// Adapted from "tga_code.txt" by James D. Murray, Anaheim, CA, USA.        //
//--------------------------------------------------------------------------//
void WriteTgaFooter(TGA *TgaHead, FILE *FpTga)
{                                                                        
    ushort    i,j;   

    uint   stampsize;      // Size of the postage stamp in bytes  
    uint   currpos;        // Current offset position in TGA file 
    TGATAG *tag;           // TGA tag structure                   

    ////   Write the image data (Versions 1.0 and 2.0).

    ////  Check if we are writing a version 2.0 TGA file.  If not, then we are
    ////  done.  If so, check if an extension and/or developer area is to be
    ////  written and then write the footer.  Then we are done.
      
    if (TgaHead->NewTgaFormat == TRUE)
    {
        ////  Check for the presence of a developers area 
        if (TgaHead->Foot.DeveloperOffset)
        {
            ////  Save the current offset as the new Developer Area offset 
            TgaHead->Foot.DeveloperOffset = ftell(FpTga);

            ////  Write the number of tags in the directory 
            putword(TgaHead->Developer.NumberOfTags, FpTga);

            ////  Write the directory to the TGA file.

            tag = TgaHead->Developer.TagList;
            for(i=0; i<TgaHead->Developer.NumberOfTags; i++)
            {
                currpos = ftell(FpTga);  // Get the current offset 

                ////   Calculate a new offset value for the tag data 
                currpos += (uint) (sizeof(tag->TagNumber) +
                                    sizeof(tag->TagOffset) +
                                    sizeof(tag->TagSize));

                ////  Assign the new offset value 
                tag->TagOffset = currpos;

                ////  Write the tag values 
                putword(tag->TagNumber, FpTga);
                putdword(tag->TagOffset, FpTga);
                putdword(tag->TagSize, FpTga);

                ////  Write the tag data 
                for(j=0; j< tag->TagSize; j++)
                    putbyte(tag->TagData[j], FpTga);

                tag = tag->Next;
            }
        }

        ////   Check for the presence of an extension area 
        if (TgaHead->Foot.ExtensionOffset)
        {
            ////  Save the current offset as the new Extension Area offset 
            TgaHead->Foot.ExtensionOffset = ftell(FpTga);

            ////  Write the extension area size information 
            putword(TgaHead->Extension.Size, FpTga);

            ////  Write the extension information defined in TGA version 2.0 
            for(i=0; i<sizeof(TgaHead->Extension.AuthorName); i++)
                putbyte(TgaHead->Extension.AuthorName[i], FpTga);

            for(i=0; i<sizeof(TgaHead->Extension.AuthorComment); i++)
                putbyte(TgaHead->Extension.AuthorComment[i], FpTga);

            putword(TgaHead->Extension.StampMonth, FpTga);
            putword(TgaHead->Extension.StampDay, FpTga); 
            putword(TgaHead->Extension.StampYear, FpTga);
            putword(TgaHead->Extension.StampHour, FpTga);
            putword(TgaHead->Extension.StampMinute, FpTga);
            putword(TgaHead->Extension.StampSecond, FpTga);

            for(i=0; i<sizeof(TgaHead->Extension.JobName); i++)
                putbyte(TgaHead->Extension.JobName[i], FpTga);

            putword(TgaHead->Extension.JobHour, FpTga);        
            putword(TgaHead->Extension.JobMinute, FpTga);      
            putword(TgaHead->Extension.JobSecond, FpTga);

            for(i=0; i<sizeof(TgaHead->Extension.SoftwareId); i++)
                putbyte(TgaHead->Extension.SoftwareId[i], FpTga);

            putword(TgaHead->Extension.VersionNumber, FpTga);  
            putbyte(TgaHead->Extension.VersionLetter, FpTga);  
            putdword(TgaHead->Extension.KeyColor, FpTga);       
            putword(TgaHead->Extension.PixelNumerator, FpTga); 
            putword(TgaHead->Extension.PixelDenominator, FpTga);
            putword(TgaHead->Extension.GammaNumerator, FpTga); 
            putword(TgaHead->Extension.GammaDenominator, FpTga);

            //// Here is where things get a bit tricky.  We can't assume that
            //// the offset values present in the extension area are still
            //// valid since WriteTgaHeader() cannot tell if the size of any
            //// information previous to the Exntension Area has changed.
            //// We must therefore calculate the size in bytes of any scan
            //// line table, postage stamp image, and the color correction
            //// table and derive their offset values.

            currpos = ftell(FpTga); // Start with the current offset 

            //// Advance to the end of the Extension Area 
            currpos += (uint) (sizeof(TgaHead->Extension.ScanOffset)  +
                                sizeof(TgaHead->Extension.StampOffset) +
                                sizeof(TgaHead->Extension.ColorOffset) +
                                sizeof(TgaHead->Extension.AttributesType));

            //// Is there a scan line table? 
            if (TgaHead->Extension.ScanOffset)
            {   TgaHead->Extension.ScanOffset = currpos;    // New offset 
                currpos += (uint) (TgaHead->Head.ImageHeight * sizeof(uint));
            }

            //// Is there a postage stamp image? 
            if (TgaHead->Extension.StampOffset)
            {   TgaHead->Extension.StampOffset = currpos;   
                currpos += (uint) TgaHead->Extension.StampWidth *
                                   TgaHead->Extension.StampHeight *
                                   ((TgaHead->Head.PixelDepth + 7) >> 3) +
                                   sizeof(TgaHead->Extension.StampWidth) +
                                   sizeof(TgaHead->Extension.StampHeight);
            }

            //// Is there a color offset table? 
            if (TgaHead->Extension.ColorOffset)
                TgaHead->Extension.ColorOffset = currpos;   // New offset 

            //// Write the offset values and the attribute type 
            putdword(TgaHead->Extension.ColorOffset, FpTga);    
            putdword(TgaHead->Extension.StampOffset, FpTga);    
            putdword(TgaHead->Extension.ScanOffset, FpTga);     
            putbyte(TgaHead->Extension.AttributesType, FpTga);

            //// Check for the presence of a scan line table 
            if (TgaHead->Extension.ScanOffset)
            {   // Write scan line offset values   
                for(i=0; i<TgaHead->Head.ImageHeight; i++)
                    putdword(TgaHead->Extension.ScanLineTable[i], FpTga);
            }

            //// Check for the presence of a postage stamp image
            if (TgaHead->Extension.StampOffset)
            {
                ////  Write the size of the stamp in pixels 
                putbyte(TgaHead->Extension.StampWidth, FpTga);
                putbyte(TgaHead->Extension.StampHeight, FpTga);

                //// Calculate the size of the stamp in bytes 
                stampsize = TgaHead->Extension.StampWidth *
                            TgaHead->Extension.StampHeight *
                            ((TgaHead->Head.PixelDepth + 7) >> 3);

                //// Write the stamp data one byte at a time 
                for(i=0; i<stampsize; i++)
                    putbyte(TgaHead->Extension.StampImage[i], FpTga);
            }

            //// Check for the presence of a color correction table 
            if (TgaHead->Extension.ColorOffset)
            {
                //// Write the entire 1024 ushort (2048 byte) table 
                for(i=0; i<sizeof(TgaHead->Extension.ColorTable); i++)
                    putword(TgaHead->Extension.ColorTable[i], FpTga);
            }
        }

        //// Write the version 2.0 footer

        //// Write the developer area and extension area offset values 
        putdword(TgaHead->Foot.ExtensionOffset, FpTga);
        putdword(TgaHead->Foot.DeveloperOffset, FpTga);
        
        //// Write the TGA signature 
        for(i=0; i<sizeof(TgaHead->Foot.Signature); i++)
            putbyte(TgaHead->Foot.Signature[i], FpTga);
    }
}


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                