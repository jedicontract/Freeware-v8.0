//--------------------------------------------------------------------------//
// xmtnimage18.cc                                                           //
// Reading & writing of bmp files                                           //
// Latest revision: 04-17-2002                                              //
// Copyright (C) 2002 by Thomas J. Nelson                                   //
// See xmtnimage.h for Copyright Notice                                     //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//

#include "xmtnimage.h"

extern Globals     g;
extern Image      *z;
extern int         ci;


//----bmp.h-----------------------------------------------------------------//

#define COMPRESS_RGB        0L      // No compression                 
#define COMPRESS_RLE8       1L      // 8 bits per pixel compression   
#define COMPRESS_RLE4       2L      // 4 bits per pixel compression   
#define BMP_ID              0x4d42  // BMP "magic" number             

#define LSN(value)        ((value) & 0x0f)            // Least-significant nibble   
#define MSN(value)        (((value) & 0xf0) >> 4)        // Most-significant nibble    

//  BMP File Format Bitmap Header
  
typedef struct BmpInfo      // Offset   Description                        
{   ushort   Type;          //  00h     File Type Identifier               
    uint  FileSize;         //  02h     Size of File                       
    ushort   Reserved1;     //  06h     Reserved (should be 0)             
    ushort   Reserved2;     //  08h     Reserved (should be 0)             
    uint  Offset;           //  0Ah     Offset to bitmap data              
} BMPINFO;

//  Presentation Manager (OS/2 1.x) Information Header Format.
  
typedef struct PmInfoHeader    // Offset   Description                       
{   uint   Size;               //  0Eh     Size of Remianing Header          
    ushort    Width;           //  12h     Width of Bitmap in Pixels         
    ushort    Height;          //  14h     Height of Bitmap in Pixels        
    ushort    Planes;          //  16h     Number of Planes                  
    ushort    BitCount;        //  18h     Color Bits Per Pixel              
} PMINFOHEAD;

//  Windows 3.x Information Header Format.
  
typedef struct WinInfoHeader   // Offset  Description                        
{   uint  Size;                //  0Eh    Size of Remianing Header           
    uint  Width;               //  12h    Width of Bitmap in Pixels          
    uint  Height;              //  16h    Height of Bitmap in Pixels         
    ushort   Planes;           //  1Ah    Number of Planes                   
    ushort   BitCount;         //  1Ch    Bits Per Pixel                     
    uint  Compression;         //  1Eh    Compression Scheme (0=none)        
    uint  SizeImage;           //  22h    Size of bitmap in bytes            
    uint  XPelsPerMeter;       //  26h    Horz. Resolution in Pixels/Meter   
    uint  YPelsPerMeter;       //  2Ah    Vert. Resolution in Pixels/Meter   
    uint  ClrUsed;             //  2Eh    Number of Colors in Color Table    
    uint  ClrImportant;        //  32h    Number of Important Colors         
} WININFOHEAD;


//  Presentation Manager (OS/2 2.0) Information Header Format.
  
typedef struct Pm2InfoHeader   // Offset  Description                        
{  
    uint   Size;               //  0Eh    Size of Info Header (always 64)    
    ushort Width;              //  12h    Width of Bitmap in Pixels          
    ushort Height;             //  14h    Height of Bitmap in Pixels         
    ushort Planes;             //  16h    Number of Planes                   
    ushort BitCount;           //  18h    Color Bits Per Pixel               
    uint   Compression;        //  1Ah    Compression Scheme (0=none)        
    uint   SizeImage;          //  1Eh    Size of bitmap in bytes            
    uint   XPelsPerMeter;      //  22h    Horz. Resolution in Pixels/Meter   
    uint   YPelsPerMeter;      //  26h    Vert. Resolution in Pixels/Meter   
    uint   ClrUsed;            //  2Ah    Number of Colors in Color Table    
    uint   ClrImportant;       //  2Eh    Number of Important Colors         
    ushort Units;              //  32h    Resolution Mesaurement Used        
    ushort Reserved;           //  34h    Reserved FIelds (always 0)         
    ushort Recording;          //  36h    Orientation of Bitmap              
    ushort Rendering;          //  38h    Halftone Algorithm Used on Image   
    uint   Size1;              //  3Ah    Halftone Algorithm Data            
    uint   Size2;              //  3Eh    Halftone Algorithm Data            
    uint   ColorEncoding;      //  42h    Color Table Format (always 0)      
    uint   Identifier;         //  46h    Misc. Field for Application Use    
} PM2INFOHEAD;

//  Presentation Manager (OS/2) RGB Color Triple.
  
typedef struct PmRgbTriple
{   uchar   rgbBlue;             // Blue Intensity Value    
    uchar   rgbGreen;            // Green Intensity Value   
    uchar   rgbRed;              // Red Intensity Value     
} PMRGBTRIPLE;

//  Windows 3.x RGB Color Quadruple.
  
typedef struct WinRgbQuad
{   uchar   rgbBlue;             // Blue Intensity Value     
    uchar   rgbGreen;            // Green Intensity Value    
    uchar   rgbRed;              // Red Intensity Value      
    uchar   rgbReserved;         // Reserved (should be 0)   
} WINRGBQUAD;

//  OS/2 2.0 RGB Color Quadruple.
  
typedef struct Pm2RgbQuad
{
    uchar   rgbBlue;             // Blue Intensity Value     
    uchar   rgbGreen;            // Green Intensity Value    
    uchar   rgbRed;              // Red Intensity Value      
    uchar   rgbReserved;         // Reserved (should be 0)   
} PM2RGBQUAD;


// Composite structure of the BMP image file format.
// This structure holds information for all three flavors of the BMP format.
  
typedef struct BmpHeader
{
    BMPINFO      Header;        // Bitmap Header                  
    PMINFOHEAD   PmInfo;        // OS/2 1.x Information Header    
    PMRGBTRIPLE *PmColorTable;  // OS/2 1.x Color Table           
    WININFOHEAD  WinInfo;       // Windows 3 Information Header   
    WINRGBQUAD  *WinColorTable; // Windows 3 Color Table          
    PM2INFOHEAD  Pm2Info;       // OS/2 2.0 Information Header    
    PM2RGBQUAD  *Pm2ColorTable; // OS/2 2.0 Color Table           
} BMPHEADER;


short int ReadBmpHeader(
  BMPHEADER *BmpHead,     // Pointer to BMP header structure    
  FILE      *FpBmp);      // BMP image file input FILE stream   
void WriteBmpHeader(
  BMPHEADER *BmpHead,     // Pointer to BMP header structure    
  FILE      *FpBmp);      // BMP image file output FILE stream   
short int BmpDecodeScanLine(
  uchar *DecodedBuffer,   // Pointer to buffer to hold decoded data           
  ushort  BufferSize,     // Size of buffer to hold decoded data in bytes     
  ushort  LineLength,     // The length of a scan line in pixels              
  uint Method,            // Data encoding method used on scan line data      
  FILE *FpBmp);           // FILE pointer to the open input BMP image file    
short int BmpEncodeScanLine(
  uchar  *inbuf        ,  // Pointer to buffer to hold input scan line         
  uchar  *EncodedBuffer,  // Pointer to buffer to hold encodeded scan line     
  ushort  BufferSize,     // Size of buffer holding unencoded data             
  ushort  LineLength,     // The length of a scan line in pixels               
  uint  Method);          // Encoding method to use                            


//--------------------------------------------------------------------------//



//--------------------------------------------------------------------------//
// readbmpfile                                                              //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
int readbmpfile(char *filename)
{
    int xsize,ysize,ibpp,ct,status=OK,j2,k,noofcolors;
    uint i2;
    ushort i, j;                                 
    FILE       *fp;              // BMP image file input FILE stream    
    uchar      *buffer;          // Buffer to hold scan line data       
    ushort      bufSize;         // Size of the scan line buffer        
    short int   byteCount;       // Number of bytes in a buffer         
    uint        compression;     // Compression value                   
    BMPHEADER   bmpHead;         // BMP image file header structure     
    int         junk;

    if((fp=fopen(filename,"rb"))==NULL)
    {   error_message(filename, errno);
        return(NOTFOUND);
    }

    //-------BMP-specific stuff------------------------------------------//

    if(g.tif_skipbytes) for(k=0;k<g.tif_skipbytes;k++) fread(&junk,1,1,fp);
    ReadBmpHeader(&bmpHead, fp);
    if (ferror(fp)) return(ERROR);
    if (bmpHead.Header.Type != BMP_ID)
    {   message("Invalid BMP file header",ERROR);
        return(ABORT);
    }

    // Seek to the bitmap data (we should already be at the data)   
    fseek(fp, g.tif_skipbytes+bmpHead.Header.Offset, SEEK_SET);

    if (bmpHead.WinInfo.Size)   // Windows 3.x BMP file   
    {
        xsize = 2*((1+bmpHead.WinInfo.Width)/2);
        ysize = bmpHead.WinInfo.Height;
        ibpp  = bmpHead.WinInfo.BitCount;
        if(ibpp<=8) ct=INDEXED; else ct=COLOR;
        if(newimage(basefilename(filename), 1, 1, xsize, ysize, ibpp, ct, 1, 
            g.want_shell, 1, PERM, 1,
            g.window_border, 0)!=OK) 
            return(NOMEM); 

           // Calculate the size of the scan line buffer in bytes   
        bufSize=(2*xsize*((bmpHead.WinInfo.BitCount + 7) >> 3) + 32);
           // Allocate scan line buffer memory   
        if((buffer = (uchar *)calloc(bufSize, sizeof(uchar))) == (uchar *) NULL)
        {   fclose(fp);
            status=NOMEM;
            eraseimage(ci,0,0,1);
            return(status);
        }

           // If the BMP file contains compressed image data, decode it   
        if (bmpHead.WinInfo.Compression == COMPRESS_RLE4 ||
            bmpHead.WinInfo.Compression == COMPRESS_RLE8)
        {   compression = bmpHead.WinInfo.Compression;
            for(i=0;i<bmpHead.WinInfo.Height; i++)
            {   i2 = bmpHead.WinInfo.Height - i - 1;
                if ((byteCount =
                    BmpDecodeScanLine(buffer, bufSize, bmpHead.WinInfo.Width,
                      compression, fp)) < 0)
                {   message("Error reading BMP file",ERROR);
                    eraseimage(ci,0,0,1);
                    fclose(fp);
                    return(ERROR);
                }
                memcpy(z[ci].image[z[ci].cf][i2],buffer,byteCount);
            }
        }
        else    // The BMP file contains uncompressed image data       
        if (bmpHead.WinInfo.Compression == COMPRESS_RGB &&
           (bmpHead.WinInfo.BitCount == 4 ||
            bmpHead.WinInfo.BitCount == 8 ||
            bmpHead.WinInfo.BitCount == 24) )
        {
           switch(bmpHead.WinInfo.BitCount)
           { case 4:
                byteCount=xsize/2;
                for(i=0;i<bmpHead.WinInfo.Height;i++)  // i,i2 are unsigned
                {  i2 = bmpHead.WinInfo.Height - i - 1;
                   fread(buffer,1,byteCount,fp);
                   for(j=0,j2=0;j<byteCount; j++,j2+=2)
                   {  z[ci].image[z[ci].cf][i2][j2  ] = (buffer[j]>>4);   //hi nibl
                      z[ci].image[z[ci].cf][i2][j2+1] = (buffer[j]&0x0F); //lo nibl
                   }
                }   
                break;   
             case 24:
                byteCount=(xsize*bmpHead.WinInfo.BitCount)/8;
                byteCount = (byteCount / 4) * 4;
                for(i=0;i<bmpHead.WinInfo.Height;i++)  // i,i2 are unsigned
                {   fread(buffer,1,byteCount,fp);
                    i2 = bmpHead.WinInfo.Height - i - 1;
                    memcpy(z[ci].image[z[ci].cf][i2],buffer,byteCount);
                }
                break;
             default:
                byteCount=(xsize*bmpHead.WinInfo.BitCount)/8;
                for(i=0;i<bmpHead.WinInfo.Height;i++)  // i,i2 are unsigned
                {   fread(buffer,1,byteCount,fp);
                    i2 = bmpHead.WinInfo.Height - i - 1;
                    memcpy(z[ci].image[z[ci].cf][i2],buffer,byteCount);
                }
                break;
           }  
        }else 
        {  message("Unsupported BMP format",ERROR); 
           status=ABORT;
        }   

                    // Set palette                    
        if(bmpHead.WinInfo.BitCount!=24 || bmpHead.WinInfo.ClrUsed!=0)
        {   noofcolors = bmpHead.WinInfo.ClrUsed;
            if(noofcolors==0) noofcolors = 1<<bmpHead.WinInfo.BitCount;
            if(noofcolors>256)noofcolors=256;
            for(k=0;k<noofcolors;k++)
            {   g.palette[k].red  = bmpHead.WinColorTable[k].rgbRed/4;
                g.palette[k].green= bmpHead.WinColorTable[k].rgbGreen/4;
                g.palette[k].blue = bmpHead.WinColorTable[k].rgbBlue/4;
            }
            memcpy(z[ci].palette, g.palette, 768);
            setpalette(g.palette);
        }   
    }                                       
    else
    if (bmpHead.PmInfo.Size)    // OS/2 1.x BMP file   
    {
      message("Can't read OS/2 1.x BMP file",ERROR);
      status=ABORT;
    }
    else
    if (bmpHead.Pm2Info.Size)    // OS/2 2.0 BMP file   
    {
      message("Can't read OS/2 2.x BMP file",ERROR);
      status=ABORT;
    }

    //-------End of bmp reading-specific stuff----------------//
    fclose(fp);
    if(status!=OK) eraseimage(ci,0,0,1);
    return(status);
}


//--------------------------------------------------------------------------//
// writebmpfile                                                             //
// This has only been tested for uncompressed Windows bmp format.           //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
int writebmpfile(char *filename, int write_all, int subtype, int compress)
{
    int i,k,k2,x1,x2,y1,y2,xsize,ysize,bpp,ino;
    uchar *inbuf;
    uchar endline[3] ={ 0x00,0x01 }; // end of line 
    uchar endimage[3]={ 0x00,0x00 }; // end of image code
    FILE         *fp;                // BMP image file output FILE stream   
    uchar        *buffer;            // Buffer to hold scan line data       
    ushort        bufSize;           // Size of the scan line buffer        
    short int     byteCount;         // Number of bytes in a buffer                
    BMPHEADER     bmpHead;           // BMP image file header structure     
    int bytesperline,value;
   
  
    unsigned int filesize,bitmapsize;
    
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
    if(g.want_color_type==0)
    {  message("Saving as monochrome!"); 
       xsize = ((x2-x1+7)>>3)<<3;
    }else 
       xsize = x2-x1;
    xsize = 4*((3+xsize)/4);   // must be divisible by 4
    ysize = y2-y1+1;

    bytesperline = xsize*g.off[g.want_bpp];
    bitmapsize = ysize*bytesperline;
    filesize   = 54 + 1024 + bitmapsize;

    ////  Fill header with information
    
    // BMPINFO    Header;                // Bitmap Header  

    bmpHead.Header.Type     =  0x4D42;   // Always "BM" for Windows 3.x
    bmpHead.Header.FileSize =  filesize; // Size of file in bytes 
    bmpHead.Header.Reserved1=  0;        // Always 0
    bmpHead.Header.Reserved2=  0;        // Always 0
    bmpHead.Header.Offset   =  1078;     // Offset to bitmap data =
                                         //   1024 for palette +
                                         //     54 for windows header


    //  PMINFOHEAD   PmInfo;             // OS/2 1.x Information Header 

    bmpHead.PmInfo.Size    = 0;          // Size of Remianing Header 
    bmpHead.PmInfo.Width   = 0;          // Width in pixels
    bmpHead.PmInfo.Height  = 0;          // Height in pixels
    bmpHead.PmInfo.Planes  = 0;          // No.of bit planes
    bmpHead.PmInfo.BitCount= 0;          // Bpp


    //  WININFOHEAD  WinInfo;            // Windows 3 Information Header
                                         // (Size=54)

    bmpHead.WinInfo.Size     = 40;       // Size of Remaining Header = 40
    bmpHead.WinInfo.Width    = xsize;    // Width in pixels
    bmpHead.WinInfo.Height   = ysize;    // Height in pixels
    bmpHead.WinInfo.Planes   = 1;        // No of de planes always 1
    bmpHead.WinInfo.BitCount = g.want_bpp; // Bits Per Pixel  
    bmpHead.WinInfo.Compression = COMPRESS_RGB; // Compression Scheme (0=none)
    bmpHead.WinInfo.SizeImage   = bitmapsize;   // Size of bitmap in bytes 
    bmpHead.WinInfo.XPelsPerMeter= 0;    // Horz. Resolution in Pixels/Meter
    bmpHead.WinInfo.YPelsPerMeter= 0;    // Vert. Resolution in Pixels/Meter
    bmpHead.WinInfo.ClrUsed      = 256;  // Number of Colors in Color Table
    bmpHead.WinInfo.ClrImportant = 256;  // Number of Important Colors


    // PM2INFOHEAD  Pm2Info;             // OS/2 2.0 Information Header  

    bmpHead.Pm2Info.Size     = 0;        // Size of Info Header (always 64) 
    bmpHead.Pm2Info.Width    = 0;        // Width of Bitmap in Pixels
    bmpHead.Pm2Info.Height   = 0;        // Height of Bitmap in Pixels
    bmpHead.Pm2Info.Planes   = 0;        // Number of Planes  
    bmpHead.Pm2Info.BitCount = 0;        // Color Bits Per Pixel 
    bmpHead.Pm2Info.Compression  = 0;    // Compression Scheme (0=none) 
    bmpHead.Pm2Info.SizeImage    = 0;    // Size of bitmap in bytes  
    bmpHead.Pm2Info.XPelsPerMeter= 0;    // Horz. Resolution in Pixels/Meter 
    bmpHead.Pm2Info.YPelsPerMeter= 0;    // Vert. Resolution in Pixels/Meter 
    bmpHead.Pm2Info.ClrUsed      = 0;    // Number of Colors in Color Table
    bmpHead.Pm2Info.ClrImportant = 0;    // Number of Important Colors  
    bmpHead.Pm2Info.Units        = 0;    // Resolution Mesaurement Used 
    bmpHead.Pm2Info.Reserved     = 0;    // Reserved FIelds (always 0) 
    bmpHead.Pm2Info.Recording    = 0;    // Orientation of Bitmap     
    bmpHead.Pm2Info.Rendering    = 0;    // Halftone Algorithm Used on Image
    bmpHead.Pm2Info.Size1        = 0;    // Halftone Algorithm Data 
    bmpHead.Pm2Info.Size2        = 0;    // Halftone Algorithm Data
    bmpHead.Pm2Info.ColorEncoding= 0;    // Color Table Format (always 0)
    bmpHead.Pm2Info.Identifier   = 0;    // Misc. Field for Application Use

               // Allocate memory for the color table entries 

    if(subtype==BMP)
    { bmpHead.WinColorTable=(WINRGBQUAD *)calloc((size_t)256,sizeof(WINRGBQUAD));
      if(bmpHead.WinColorTable==NULL) return(NOMEM);
    }
    if(subtype==OS2)
    { bmpHead.Pm2ColorTable=(PM2RGBQUAD *)calloc((size_t)256,sizeof(PM2RGBQUAD));
      if(bmpHead.Pm2ColorTable==NULL) return(NOMEM);
    }

               // WINRGBQUAD  *WinColorTable; // Windows 3 Color Table

    if(subtype==BMP) for(k=0;k<=255;k++)
    {  bmpHead.WinColorTable[k].rgbBlue     = 4*g.palette[k].blue;  
       bmpHead.WinColorTable[k].rgbGreen    = 4*g.palette[k].green; 
       bmpHead.WinColorTable[k].rgbRed      = 4*g.palette[k].red;   
       bmpHead.WinColorTable[k].rgbReserved = 0;
    }

              // PM2RGBQUAD  *Pm2ColorTable; // OS/2 2.0 Color Table

    if(subtype==OS2) for(k=0;k<=255;k++)
    {  bmpHead.Pm2ColorTable[k].rgbBlue     = 4*g.palette[k].blue;  
       bmpHead.Pm2ColorTable[k].rgbGreen    = 4*g.palette[k].green; 
       bmpHead.Pm2ColorTable[k].rgbRed      = 4*g.palette[k].red;   
       bmpHead.Pm2ColorTable[k].rgbReserved = 0;
    }

    if((fp=open_file(filename, compress, TNI_WRITE, g.compression, g.decompression, 
       g.compression_ext))==NULL) 
       return CANTCREATEFILE;
    if(subtype==BMP)
    {
        bufSize = bytesperline + 2;
        if((buffer=(uchar *)calloc(bufSize,sizeof(uchar)))==(uchar *)NULL)
            return(NOMEM);
        if((inbuf =(uchar *)calloc(bufSize,sizeof(uchar)))==(uchar *)NULL)
            return(NOMEM);

        if(bmpHead.WinInfo.Compression==COMPRESS_RGB)  // no compression
        {
            WriteBmpHeader(&bmpHead, fp);
            for(i=bmpHead.WinInfo.Height-1;i>=0; i--)
            {   for(k=0,k2=0;k<xsize;k++,k2+=g.off[g.want_bpp])
                {   value=readpixelonimage(k+x1,i+y1,bpp,ino);
                    if(bpp!=g.want_bpp)value=convertpixel(value,bpp,g.want_bpp,1);
                    putpixelbytes(inbuf+k2,value,1,g.want_bpp);
                }    
                fwrite(inbuf,bytesperline,1,fp);
            }
        }
        if(bmpHead.WinInfo.Compression==COMPRESS_RLE8 &&
          (bmpHead.WinInfo.BitCount==4 || bmpHead.WinInfo.BitCount==8))
        {
            WriteBmpHeader(&bmpHead, fp);
            for(i=bmpHead.WinInfo.Height-1;i>=0; i--)
            {   for(k=0,k2=0;k<xsize;k++,k2+=g.off[g.want_bpp])
                {   value=readpixelonimage(k+x1,i+y1,bpp,ino);
                    if(bpp!=g.want_bpp)value=convertpixel(value,bpp,g.want_bpp,1);
                    putpixelbytes(inbuf+k2,value,1,g.want_bpp);
                }    
                if ((byteCount = BmpEncodeScanLine(inbuf,buffer,bufSize, 
                    bmpHead.WinInfo.Width,bmpHead.WinInfo.Compression))<0)
                {   message("Error writing bmp file",ERROR);
                    return(ABORT);
                }
                fwrite(inbuf,byteCount,1,fp);
                fwrite(endline,2,1,fp);
            }
            fwrite(endimage,2,1,fp);
        }    
    }                                       
    if(subtype==OS2)  // OS/2 2.0 BMP file   
    {
       // This space intentionally left blank
    }

    close_file(fp, compress);  
    return OK;      // Successful termination   
}



//--------------------------------------------------------------------------//
// bmpread - read Windows or PM bmp header                                  //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
short int ReadBmpHeader(BMPHEADER *BmpHead, FILE  *FpBmp)  
{
    int i2;
    short int i;              // Loop Counter   
    uint InfoHeaderSize;      // Size of the BMP information header in bytes   
    ushort  NumColors;        // Number of colors in color table   

    BmpHead->Header.Type      = getword(FpBmp);   //file type =="BM"
    BmpHead->Header.FileSize  = getdword(FpBmp);
    BmpHead->Header.Reserved1 = getword(FpBmp);
    BmpHead->Header.Reserved2 = getword(FpBmp);
    BmpHead->Header.Offset    = getdword(FpBmp);  //offset to start of image

    InfoHeaderSize            = getdword(FpBmp);

    BmpHead->PmInfo.Size   = 0;
    BmpHead->WinInfo.Size  = 0;
    BmpHead->Pm2Info.Size  = 0;

    //-----------------------------------------------------------------//
    // The size if the information header indicates if the BMP file
    // originated on an MS Windows or OS/2 Presentation Manager system.
    //-----------------------------------------------------------------//
      
    if (InfoHeaderSize == 12L)         // OS/2 1.x   
    {
        BmpHead->PmInfo.Size     = InfoHeaderSize;
        BmpHead->PmInfo.Width    = getword(FpBmp);
        BmpHead->PmInfo.Height   = getword(FpBmp);
        BmpHead->PmInfo.Planes   = getword(FpBmp);
        BmpHead->PmInfo.BitCount = getword(FpBmp);

        if (BmpHead->PmInfo.BitCount != 24)
        {
            // Determine number of entries in color table   
            NumColors = (ushort) (1U << (BmpHead->PmInfo.Planes *
              BmpHead->PmInfo.BitCount));
 
            // Allocate memory for the color table entries   
            if ((BmpHead->PmColorTable = (PMRGBTRIPLE *)
                 calloc((size_t) NumColors, sizeof(PMRGBTRIPLE))) ==
                 (PMRGBTRIPLE *) NULL)
                return(-1);

            // Read in the color table one color triple at a time   
            for (i = 0; i < NumColors; i++)
            {
                BmpHead->PmColorTable[i].rgbBlue  = getbyte(FpBmp);
                BmpHead->PmColorTable[i].rgbGreen = getbyte(FpBmp);
                BmpHead->PmColorTable[i].rgbRed   = getbyte(FpBmp);
            }
        }
    }
    else                               // Windows 3   
    if (InfoHeaderSize == 40L)
    {                              
        BmpHead->WinInfo.Size          = InfoHeaderSize;  // Header size=40
        BmpHead->WinInfo.Width         = getdword(FpBmp);
        BmpHead->WinInfo.Height        = getdword(FpBmp);
        BmpHead->WinInfo.Planes        = getword(FpBmp);
        BmpHead->WinInfo.BitCount      = getword(FpBmp);  // Bits/pixel

        BmpHead->WinInfo.Compression   = getdword(FpBmp); // 0=uncompressed
                                                          // 1=8 bit rle
                                                          // 2=4 bit rle
                                                          
        BmpHead->WinInfo.SizeImage     = getdword(FpBmp); // Size of bitmap or
                                                          // 0 if uncompressed
        BmpHead->WinInfo.XPelsPerMeter = getdword(FpBmp);
        BmpHead->WinInfo.YPelsPerMeter = getdword(FpBmp);
        BmpHead->WinInfo.ClrUsed       = getdword(FpBmp); // No.of colors in
                                                          // palette, or 0 if
                                                          // maximum possible

        BmpHead->WinInfo.ClrImportant  = getdword(FpBmp); // No.of colors used
                                                          // or 0 if all of'em

        // Read in the color table (if any)   
        if (BmpHead->WinInfo.BitCount != 24 || BmpHead->WinInfo.ClrUsed != 0)
        {
            // Determine number of entries in color table   
            if (BmpHead->WinInfo.ClrUsed)
                NumColors = BmpHead->WinInfo.ClrUsed;
            else
                NumColors = (ushort) (1U << (BmpHead->WinInfo.Planes *
                  BmpHead->WinInfo.BitCount));

            // Allocate memory for the color table entries   
            if ((BmpHead->WinColorTable = (WINRGBQUAD *)
                 calloc((size_t) NumColors, sizeof(WINRGBQUAD))) ==
                 (WINRGBQUAD *) NULL)
                return(-1);
      
            // Read in the color table one color quad at a time   
            for(i2=0;i2<NumColors;i2++)
            {   BmpHead->WinColorTable[i2].rgbBlue     = getbyte(FpBmp);
                BmpHead->WinColorTable[i2].rgbGreen    = getbyte(FpBmp);
                BmpHead->WinColorTable[i2].rgbRed      = getbyte(FpBmp);
                BmpHead->WinColorTable[i2].rgbReserved = getbyte(FpBmp);
            }
        }
    }
    else                               // OS/2 2.0   
    if (InfoHeaderSize == 64L)
    {                              
        BmpHead->Pm2Info.Size          = InfoHeaderSize;
        BmpHead->Pm2Info.Width         = getdword(FpBmp);
        BmpHead->Pm2Info.Height        = getdword(FpBmp);
        BmpHead->Pm2Info.Planes        = getword(FpBmp);
        BmpHead->Pm2Info.BitCount      = getword(FpBmp);
        BmpHead->Pm2Info.Compression   = getdword(FpBmp);
        BmpHead->Pm2Info.SizeImage     = getdword(FpBmp);
        BmpHead->Pm2Info.XPelsPerMeter = getdword(FpBmp);
        BmpHead->Pm2Info.YPelsPerMeter = getdword(FpBmp);
        BmpHead->Pm2Info.ClrUsed       = getdword(FpBmp);
        BmpHead->Pm2Info.ClrImportant  = getdword(FpBmp);
        BmpHead->Pm2Info.Units         = getword(FpBmp);
        BmpHead->Pm2Info.Reserved      = getword(FpBmp);
        BmpHead->Pm2Info.Recording     = getword(FpBmp);
        BmpHead->Pm2Info.Rendering     = getword(FpBmp);
        BmpHead->Pm2Info.Size1         = getdword(FpBmp);
        BmpHead->Pm2Info.Size2         = getdword(FpBmp);
        BmpHead->Pm2Info.ColorEncoding = getdword(FpBmp);
        BmpHead->Pm2Info.Identifier    = getdword(FpBmp);
      
        // Read in the color table (if any)   
        if (BmpHead->Pm2Info.BitCount != 24 || BmpHead->Pm2Info.ClrUsed != 0)
        {
            // Determine number of entries in color table   
            if (BmpHead->Pm2Info.ClrUsed)
                NumColors = BmpHead->Pm2Info.ClrUsed;
            else
                NumColors = (ushort) (1U << (BmpHead->Pm2Info.Planes *
                  BmpHead->Pm2Info.BitCount));

            // Allocate memory for the color table entries   
            if ((BmpHead->Pm2ColorTable = (PM2RGBQUAD *)
                 calloc((size_t) NumColors, sizeof(PM2RGBQUAD))) ==
                 (PM2RGBQUAD *) NULL)
                return(-1);
      
            // Read in the color table one color quad at a time   
            for (i = 0; i < NumColors; i++)
            {
                BmpHead->Pm2ColorTable[i].rgbBlue     = getbyte(FpBmp);
                BmpHead->Pm2ColorTable[i].rgbGreen    = getbyte(FpBmp);
                BmpHead->Pm2ColorTable[i].rgbRed      = getbyte(FpBmp);
                BmpHead->Pm2ColorTable[i].rgbReserved = getbyte(FpBmp);
            }
        }
    }
    return(0);
}


//--------------------------------------------------------------------------//
// bmpwrite - write a bmp header to file stream.                            //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
void WriteBmpHeader( BMPHEADER *BmpHead,  FILE *FpBmp)
{
    short int i; 
    ushort  NumColors;    // Number of colors in color table   

//    putword  = PutLittleWord;   // Write using little-endian byte order   
//    putdword = PutLittleDword;  // This doesn't compile in gcc  

    // Write the bit map file header   
    putword(BmpHead->Header.Type,      FpBmp);
    putdword(BmpHead->Header.FileSize, FpBmp);
    putword(BmpHead->Header.Reserved1, FpBmp);
    putword(BmpHead->Header.Reserved2, FpBmp);
    putdword(BmpHead->Header.Offset,   FpBmp);

    //
    // Write the bit map information header.
    //
    // The size if the information header indicates if the BMP file
    // originated on an MS Windows or OS/2 Presentation Manager system.
      
    if (BmpHead->PmInfo.Size)          // OS/2 1.x   
    {
        putword(BmpHead->PmInfo.Size,     FpBmp);
        putword(BmpHead->PmInfo.Width,    FpBmp);
        putword(BmpHead->PmInfo.Height,   FpBmp);
        putword(BmpHead->PmInfo.Planes,   FpBmp);
        putword(BmpHead->PmInfo.BitCount, FpBmp);

        if (BmpHead->PmColorTable)
        {
            // Determine number of entries in color table   
            NumColors = (ushort) (1U << (BmpHead->PmInfo.Planes *
              BmpHead->PmInfo.BitCount));

            // Write the color table one color triple at a time   
            for (i = 0; i < NumColors; i++)
            {
                 putbyte(BmpHead->PmColorTable[i].rgbBlue,  FpBmp);
                 putbyte(BmpHead->PmColorTable[i].rgbGreen, FpBmp);
                 putbyte(BmpHead->PmColorTable[i].rgbRed,   FpBmp);
            }
        }
    }
    else                               // Windows 3   
    if (BmpHead->WinInfo.Size)
    {                              
        putdword(BmpHead->WinInfo.Size,          FpBmp);
        putdword(BmpHead->WinInfo.Width,         FpBmp);
        putdword(BmpHead->WinInfo.Height,        FpBmp);
        putword(BmpHead->WinInfo.Planes,         FpBmp);
        putword(BmpHead->WinInfo.BitCount,       FpBmp);
        putdword(BmpHead->WinInfo.Compression,   FpBmp);
        putdword(BmpHead->WinInfo.SizeImage,     FpBmp);
        putdword(BmpHead->WinInfo.XPelsPerMeter, FpBmp);
        putdword(BmpHead->WinInfo.YPelsPerMeter, FpBmp);
        putdword(BmpHead->WinInfo.ClrUsed,       FpBmp);
        putdword(BmpHead->WinInfo.ClrImportant,  FpBmp);

        if(BmpHead->WinColorTable)
        {
            // Determine number of entries in color table   
            if (BmpHead->WinInfo.ClrUsed)
                NumColors = BmpHead->WinInfo.ClrUsed;
            else
                NumColors = (ushort) (1U << (BmpHead->WinInfo.Planes *
                  BmpHead->WinInfo.BitCount));

            // Write the color table one color quad at a time   
            for (i=0; i<NumColors; i++)
            {
                putbyte(BmpHead->WinColorTable[i].rgbBlue,     FpBmp);
                putbyte(BmpHead->WinColorTable[i].rgbGreen,    FpBmp);
                putbyte(BmpHead->WinColorTable[i].rgbRed,      FpBmp);
                putbyte(BmpHead->WinColorTable[i].rgbReserved, FpBmp);
            }
        }
    }
    else                               // OS/2 2.0   
    if (BmpHead->Pm2Info.Size)
    {                              
        putdword(BmpHead->Pm2Info.Size,          FpBmp);
        putdword(BmpHead->Pm2Info.Width,         FpBmp);
        putdword(BmpHead->Pm2Info.Height,        FpBmp);
        putword( BmpHead->Pm2Info.Planes,        FpBmp);
        putword( BmpHead->Pm2Info.BitCount,      FpBmp);
        putdword(BmpHead->Pm2Info.Compression,   FpBmp);
        putdword(BmpHead->Pm2Info.SizeImage,     FpBmp);
        putdword(BmpHead->Pm2Info.XPelsPerMeter, FpBmp);
        putdword(BmpHead->Pm2Info.YPelsPerMeter, FpBmp);
        putdword(BmpHead->Pm2Info.ClrUsed,       FpBmp);
        putdword(BmpHead->Pm2Info.ClrImportant,  FpBmp);
        putword( BmpHead->Pm2Info.Units,         FpBmp);
        putword( BmpHead->Pm2Info.Reserved,      FpBmp);
        putword( BmpHead->Pm2Info.Recording,     FpBmp);
        putword( BmpHead->Pm2Info.Rendering,     FpBmp);
        putdword(BmpHead->Pm2Info.Size1,         FpBmp);
        putdword(BmpHead->Pm2Info.Size2,         FpBmp);
        putdword(BmpHead->Pm2Info.ColorEncoding, FpBmp);
        putdword(BmpHead->Pm2Info.Identifier,    FpBmp);

        if (BmpHead->Pm2ColorTable)
        {
            // Determine number of entries in color table   
            if (BmpHead->Pm2Info.ClrUsed)
                NumColors = BmpHead->Pm2Info.ClrUsed;
            else
                NumColors = (ushort) (1U << (BmpHead->Pm2Info.Planes *
                  BmpHead->Pm2Info.BitCount));

            // Write the color table one color quad at a time   
            for (i = 0; i < NumColors; i++)
            {
                putbyte(BmpHead->Pm2ColorTable[i].rgbBlue,     FpBmp);
                putbyte(BmpHead->Pm2ColorTable[i].rgbGreen,    FpBmp);
                putbyte(BmpHead->Pm2ColorTable[i].rgbRed,      FpBmp);
                putbyte(BmpHead->Pm2ColorTable[i].rgbReserved, FpBmp);
            }
        }
    }
}


//--------------------------------------------------------------------------//
// bmpencodescanline                                                        //
//   Method may be:                                                         //
//       0 - Unencoded                                                      //
//       1 - Four bits per pixel                                            //
//       2 - Eight bits per pixel                                           //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
short int BmpEncodeScanLine(uchar  *inbuf, uchar *EncodedBuffer, ushort BufferSize,
      ushort LineLength, uint Method)
{
    int     runCount;       // The number of pixels in the current run        
    ushort  pixelCount;     // The number of pixels read from the scan line   
    short   bufIndex;       // The index of DecodedBuffer                     
    uchar   pixelValue1;    // Pixel value read from the scan line (4-byte max)   
    uchar   pixelValue2;    // Pixel value read from the scan line            
    int     inpos=0;

    // Check that a proper compression method has been specified   
    if (Method != COMPRESS_RLE4 && Method != COMPRESS_RLE8)
        return(-1);

    bufIndex   = 0;
    runCount   = (Method == COMPRESS_RLE4 ? 2 : 1);
    pixelCount = (Method == COMPRESS_RLE4 ? 2 : 1);
                                    
    pixelValue1 = inbuf[inpos++];             // Read in first pixel value

    for (;;)
    {
        pixelValue2 = inbuf[inpos++];         // get another pixel
        pixelCount += (Method==COMPRESS_RLE4 ? 2 : 1);
        if(pixelValue1 == pixelValue2)        // Compare pixels
        {   runCount += (Method==COMPRESS_RLE4 ? 2 : 1);
            if(runCount < 256)                // Maximum run-length=256 pixels
            {   if(pixelCount<LineLength)  // Don't run past end of scan line
                    continue;                 // Continue reading the run  
            }
        }

        //
        // If we have gotten this far then we have either come to the end of
        // a pixel run, have reached the maximum number of pixels encodable
        // in a run, or read to the end of the scan line.  Now encode the
        // current run.
        //

        //
        // Literal runs must have a runCount greater than 2 or the
        // literal run indicator will be confused with an escape code.
        // This scheme will also only encode even-length runs as literal
        // runs.  This frees us from keeping track of left over nibbles
        // from odd-length runs.
        //
        if (runCount < 0)
        {
            // Make sure writing this next run will not overflow the buffer   
            if (bufIndex + runCount >= BufferSize - 2)
                return(-1);

            EncodedBuffer[bufIndex++] = 0;          // Literal Run indicator    
            EncodedBuffer[bufIndex++] = runCount;   // Number of pixels in run   

            if (Method == COMPRESS_RLE4) runCount /= 2;

            // Write the pixel data run   
            while (runCount--) EncodedBuffer[bufIndex++]=pixelValue1;
        }
        else                // Write an Encoded Run   
        {
            EncodedBuffer[bufIndex++] = runCount;    // Number of pixels in run   
            EncodedBuffer[bufIndex++] = pixelValue1; // Value of pixels in run   
        }

        // If we've encoded the entire line then break out of the loop   
        if (pixelCount == LineLength) break;

        // Start a new pixel run count    
        runCount = (Method == COMPRESS_RLE4 ? 2 : 1);

        // Store next pixel value run to match   
        pixelValue1 = pixelValue2;
    }
    return(bufIndex);
}



//--------------------------------------------------------------------------//
// bmpdecodescanline                                                        //
// Adapted from "bmp_code.txt" by James D. Murray, Anaheim, CA, USA         //
//--------------------------------------------------------------------------//
short BmpDecodeScanLine(uchar *DecodedBuffer, ushort BufferSize, ushort LineLength,
      uint Method, FILE *FpBmp) 
{
    LineLength=LineLength;
    uchar    runCount;       // Number of pixels in the run    
    uchar    runValue;       // Value of pixels in the run     
    uchar    Value;          // Temporary pixel value holder   
    ushort    bufIndex;       // The index of DecodedBuffer     

    // Check that a proper compression method has been specified   
    if (Method != COMPRESS_RLE4 && Method != COMPRESS_RLE8) return(-1);

    bufIndex = 0;         // Initialize the buffer index    
    while(bufIndex<BufferSize)
    {
        runCount = getbyte(FpBmp);  // Number of pixels in the run   
        runValue = getbyte(FpBmp);  // Value of pixels in the run    

        switch(runCount)
        {   case 0:        // Literal Run or Escape Code   
                switch(runValue)
                {
                    case 0:             // End of Scan Line  0 0
                    case 1:             // End of image      0 1
                        return(bufIndex);
                    case 2:             // Delta Escape Code 0 2
                        message("Invalid BMP file",ERROR);
                        return(-2);
                    default:            // Literal Run   
                           // Check for a possible buffer overflow   
                        if(bufIndex+runValue>BufferSize) return(-2);
                        if(Method==COMPRESS_RLE8)
                        {  while (runValue--)
                           {   DecodedBuffer[bufIndex]=getbyte(FpBmp);
                               bufIndex++;
                           }
                        }else if(Method == COMPRESS_RLE4)
                        {
                            //
                            // Alternate writing the most-significant and
                            // Least-significant nibble to the buffer.  The
                            // odd-length literal runs are a bit tricky.
                              
                            while (runValue--)
                            {   Value = getbyte(FpBmp);
                                DecodedBuffer[bufIndex] = (MSN(Value) << 4);
                                if (runValue--)
                                    DecodedBuffer[bufIndex++] |= LSN(Value);
                            }
                        }
                }
                break;
            default:    // Encoded Run   
                if (Method == COMPRESS_RLE4)            // a 4-bit value
                {
                    // Check for a possible buffer overflow   
                    if (bufIndex + (runCount / 2) > BufferSize)
                    {   message("a Bufr ovrflow",ERROR);
                        return(-2);
                    }
                    
                    // Alternate writing the most-significant and
                    // Least-significant nibble to the buffer.
                    while (runCount--)
                    {  DecodedBuffer[bufIndex] = (MSN(runValue) << 4);
                        if (runCount--)
                            DecodedBuffer[bufIndex++] |= LSN(runValue);
                    }
                }
                else                                    // an 8-bit value 
                {
                    // Check for a possible buffer overflow   
                    if (bufIndex + runCount > BufferSize) return(-2);
                    while(runCount--) DecodedBuffer[bufIndex++]=runValue;
                }
                break;
        }
    }
    return(-3);     // No End-of-Scan Line or End-of-Bitmap code!   
}


