/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            EEEEE  M   M  FFFFF                              %
%                            E      MM MM  F                                  %
%                            EEE    M M M  FFF                                %
%                            E      M   M  F                                  %
%                            EEEEE  M   M  F                                  %
%                                                                             %
%                                                                             %
%                  Read Windows Enahanced Metafile Format.                    %
%                                                                             %
%                              Software Design                                %
%                              Bill Radcliffe                                 %
%                                   2001                                      %
%                                                                             %
%  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

/*
 * Include declarations.
 */

#include "magick/studio.h"
#if defined(HasWINGDI32)
#  if defined(__CYGWIN__)
#    include <windows.h>
#  else
     // All MinGW needs ...
#    include <wingdi.h>
#  endif
#endif

#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s E F M                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEMF() returns MagickTrue if the image format type, identified by the
%  magick string, is a Microsoft Windows Enhanced MetaFile (EMF) file.
%
%  The format of the ReadEMFImage method is:
%
%      MagickBooleanType IsEMF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsEMF(const unsigned char *magick,const size_t length)
{
  if (length < 48)
    return(MagickFalse);
  if (memcmp(magick+40,"\040\105\115\106\000\000\001\000",8) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s W M F                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsWMF() returns MagickTrue if the image format type, identified by the
%  magick string, is a Windows MetaFile (WMF) file.
%
%  The format of the ReadEMFImage method is:
%
%      MagickBooleanType IsEMF(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsWMF(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\327\315\306\232",4) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\001\000\011\000",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  R e a d E M F I m a g e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadEMFImage() reads an Microsoft Windows Enhanced MetaFile (EMF) or
%  Windows MetaFile (WMF) file using the Windows API and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadEMFImage method is:
%
%      Image *ReadEMFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

/*
  This method reads either an enhanced metafile, a regular 16bit Windows
  metafile, or an Aldus Placeable metafile and converts it into an enhanced
  metafile.  Width and height are returned in .01mm units.
*/
#if defined(HasWINGDI32)
static HENHMETAFILE ReadEnhMetaFile(const char *szFileName,long *width,
  long *height)
{
#pragma pack( push )
#pragma pack( 2 )
  typedef struct
  {
    DWORD dwKey;
    WORD hmf;
    SMALL_RECT bbox;
    WORD wInch;
    DWORD dwReserved;
    WORD wCheckSum;
  } APMHEADER, *PAPMHEADER;
#pragma pack( pop )

  DWORD
    dwSize;

  ENHMETAHEADER
    emfh;

  HANDLE
    hFile;

  HDC
    hDC;

  HENHMETAFILE
    hTemp;

  LPBYTE
    pBits;

  METAFILEPICT
    mp;

  HMETAFILE
    hOld;

  *width=512;
  *height=512;
  hTemp=GetEnhMetaFile(szFileName);
  if (hTemp != (HENHMETAFILE) NULL)
    {
      /*
        Enhanced metafile.
      */
      GetEnhMetaFileHeader(hTemp,sizeof(ENHMETAHEADER),&emfh);
      *width=emfh.rclFrame.right-emfh.rclFrame.left;
      *height=emfh.rclFrame.bottom-emfh.rclFrame.top;
      return(hTemp);
    }
  hOld=GetMetaFile(szFileName);
  if (hOld != (HMETAFILE) NULL)
    {
      /*
        16bit windows metafile.
      */
      dwSize=GetMetaFileBitsEx(hOld,0,NULL);
      if (dwSize == 0)
        {
          DeleteMetaFile(hOld);
          return((HENHMETAFILE) NULL);
        }
      pBits=(LPBYTE) AcquireMagickMemory(dwSize);
      if (pBits == (LPBYTE) NULL)
        {
          DeleteMetaFile(hOld);
          return((HENHMETAFILE) NULL);
        }
      if (GetMetaFileBitsEx(hOld,dwSize,pBits) == 0)
        {
          pBits=(char *) RelinquishMagickMemory(pBits);
          DeleteMetaFile(hOld);
          return((HENHMETAFILE) NULL);
        }
      /*
        Make an enhanced metafile from the windows metafile.
      */
      mp.mm=MM_ANISOTROPIC;
      mp.xExt=1000;
      mp.yExt=1000;
      mp.hMF=NULL;
      hDC=GetDC(NULL);
      hTemp=SetWinMetaFileBits(dwSize,pBits,hDC,&mp);
      ReleaseDC(NULL,hDC);
      DeleteMetaFile(hOld);
      pBits=(char *) RelinquishMagickMemory(pBits);
      GetEnhMetaFileHeader(hTemp,sizeof(ENHMETAHEADER),&emfh);
      *width=emfh.rclFrame.right-emfh.rclFrame.left;
      *height=emfh.rclFrame.bottom-emfh.rclFrame.top;
      return(hTemp);
    }
  /*
    Aldus Placeable metafile.
  */
  hFile=CreateFile(szFileName,GENERIC_READ,0,NULL,OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return(NULL);
  dwSize=GetFileSize(hFile,NULL);
  pBits=(LPBYTE) AcquireMagickMemory(dwSize);
  ReadFile(hFile,pBits,dwSize,&dwSize,NULL);
  CloseHandle(hFile);
  if (((PAPMHEADER) pBits)->dwKey != 0x9ac6cdd7l)
    {
      pBits=(char *) RelinquishMagickMemory(pBits);
      return((HENHMETAFILE) NULL);
    }
  /*
    Make an enhanced metafile from the placable metafile.
  */
  mp.mm=MM_ANISOTROPIC;
  mp.xExt=((PAPMHEADER) pBits)->bbox.Right-((PAPMHEADER) pBits)->bbox.Left;
  *width=mp.xExt;
  mp.xExt=(mp.xExt*2540l)/(DWORD) (((PAPMHEADER) pBits)->wInch);
  mp.yExt=((PAPMHEADER)pBits)->bbox.Bottom-((PAPMHEADER) pBits)->bbox.Top;
  *height=mp.yExt;
  mp.yExt=(mp.yExt*2540l)/(DWORD) (((PAPMHEADER) pBits)->wInch);
  mp.hMF=NULL;
  hDC=GetDC(NULL);
  hTemp=SetWinMetaFileBits(dwSize,&(pBits[sizeof(APMHEADER)]),hDC,&mp);
  ReleaseDC(NULL,hDC);
  pBits=(char *) RelinquishMagickMemory(pBits);
  return(hTemp);
}

#define CENTIMETERS_INCH 2.54

static Image *ReadEMFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  BITMAPINFO
    DIBinfo;

  HBITMAP
    hBitmap,
    hOldBitmap;

  HDC
    hDC;

  HENHMETAFILE
    hemf;

  Image
    *image;

  long
    height,
    width,
    y;

  RECT
    rect;

  register long
    x;

  register PixelPacket
    *q;

  RGBQUAD
    *pBits,
    *ppBits;

  image=AllocateImage(image_info);
  hemf=ReadEnhMetaFile(image_info->filename,&width,&height);
  if (hemf == 0)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  if ((image->columns == 0) || (image->rows == 0))
    {
      double
        y_resolution,
        x_resolution;

       y_resolution=DefaultResolution;
       x_resolution=DefaultResolution;
      if (image->y_resolution > 0)
        {
          y_resolution=image->y_resolution;
          if (image->units == PixelsPerCentimeterResolution)
            y_resolution*=CENTIMETERS_INCH;
        }
      if (image->x_resolution > 0)
        {
          x_resolution=image->x_resolution;
          if (image->units == PixelsPerCentimeterResolution)
            x_resolution*=CENTIMETERS_INCH;
        }
      image->rows=(unsigned long)
        ((height*CENTIMETERS_INCH)/1000*y_resolution+0.5);
      image->columns=(unsigned long)
        ((width*CENTIMETERS_INCH)/1000*x_resolution+0.5);
    }
  if (image_info->size != (char *) NULL)
    {
      long
        x;

      image->columns=width;
      image->rows=height;
      x=0;
      y=0;
      (void) GetGeometry(image_info->size,&x,&y,&image->columns,&image->rows);
    }
  if (image_info->page != (char *) NULL)
    {
      char
        *geometry;

      long
        sans;

      register char
        *p;

      MagickStatusType
        flags;

      geometry=GetPageGeometry(image_info->page);
      p=strchr(geometry,'>');
      if (p == (char *) NULL)
        {
          flags=ParseMetaGeometry(geometry,&sans,&sans,&image->columns,
            &image->rows);
          if (image->x_resolution != 0.0)
            image->columns=(unsigned long)
              ((image->columns*image->x_resolution)+0.5);
          if (image->y_resolution != 0.0)
            image->rows=(unsigned long)
              ((image->rows*image->y_resolution)+0.5);
        }
      else
        {
          *p='\0';
          flags=ParseMetaGeometry(geometry,&sans,&sans,&image->columns,
            &image->rows);
          if (image->x_resolution != 0.0)
            image->columns=(unsigned long)
              (((image->columns*image->x_resolution)/DefaultResolution)+0.5);
          if (image->y_resolution != 0.0)
            image->rows=(unsigned long)
              (((image->rows*image->y_resolution)/DefaultResolution)+0.5);
        }
      geometry=(char *) RelinquishMagickMemory(geometry);
    }
  hDC=GetDC(NULL);
  if (hDC == 0)
    ThrowReaderException(ResourceLimitError,"UnableToCreateADC");
  /*
    Initialize the bitmap header info.
  */
  (void) ResetMagickMemory(&DIBinfo,0,sizeof(BITMAPINFO));
  DIBinfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  DIBinfo.bmiHeader.biWidth=image->columns;
  DIBinfo.bmiHeader.biHeight=(-1)*image->rows;
  DIBinfo.bmiHeader.biPlanes=1;
  DIBinfo.bmiHeader.biBitCount=32;
  DIBinfo.bmiHeader.biCompression=BI_RGB;
  hBitmap=
    CreateDIBSection(hDC,&DIBinfo,DIB_RGB_COLORS,(void **) &ppBits,NULL,0);
  ReleaseDC(NULL,hDC);
  if (hBitmap == 0)
    ThrowReaderException(ResourceLimitError,"UnableToCreateBitmap");
  hDC=CreateCompatibleDC(NULL);
  if (hDC == 0)
    {
      DeleteObject(hBitmap);
      ThrowReaderException(ResourceLimitError,"UnableToCreateADC");
    }
  hOldBitmap=(HBITMAP) SelectObject(hDC,hBitmap);
  if (hOldBitmap == 0)
    {
      DeleteDC(hDC);
      DeleteObject(hBitmap);
      ThrowReaderException(ResourceLimitError,"UnableToCreateBitmap");
    }
  /*
    Initialize the bitmap to the image background color.
  */
  pBits=ppBits;
  for (y=0; y < (long) image->rows; y++)
  {
    for (x=0; x < (long) image->columns; x++)
    {
      pBits->rgbRed=ScaleQuantumToChar(image->background_color.red);
      pBits->rgbGreen=ScaleQuantumToChar(image->background_color.green);
      pBits->rgbBlue=ScaleQuantumToChar(image->background_color.blue);
      pBits++;
    }
  }
  rect.top=0;
  rect.left=0;
  rect.right=image->columns;
  rect.bottom=image->rows;
  /*
    Convert metafile pixels.
  */
  PlayEnhMetaFile(hDC,hemf,&rect);
  pBits=ppBits;
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=ScaleCharToQuantum(pBits->rgbRed);
      q->green=ScaleCharToQuantum(pBits->rgbGreen);
      q->blue=ScaleCharToQuantum(pBits->rgbBlue);
      q->opacity=OpaqueOpacity;
      pBits++;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  if (hemf)
    DeleteEnhMetaFile(hemf);
  SelectObject(hDC,hOldBitmap);
  DeleteDC(hDC);
  DeleteObject(hBitmap);
  return(GetFirstImageInList(image));
}
#endif /* HasWINGDI32 */

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r E M F I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterEMFImage() adds attributes for the EMF image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterEMFImage method is:
%
%      RegisterEMFImage(void)
%
*/
ModuleExport void RegisterEMFImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EMF");
#if defined(HasWINGDI32)
  entry->decoder=ReadEMFImage;
#endif
  entry->description=ConstantString(
    "Windows WIN32 API rendered Enhanced Meta File");
  entry->magick=(MagickHandler *) IsEMF;
  entry->blob_support=MagickFalse;
  entry->module=ConstantString("WMF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("WMFWIN32");
#if defined(HasWINGDI32)
  entry->decoder=ReadEMFImage;
#endif
  entry->description=ConstantString("Windows WIN32 API rendered Meta File");
  entry->magick=(MagickHandler *) IsWMF;
  entry->blob_support=MagickFalse;
  entry->module=ConstantString("WMFWIN32");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r E M F I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterEMFImage() removes format registrations made by the
%  EMF module from the list of supported formats.
%
%  The format of the UnregisterEMFImage method is:
%
%      UnregisterEMFImage(void)
%
*/
ModuleExport void UnregisterEMFImage(void)
{
  (void) UnregisterMagickInfo("EMF");
  (void) UnregisterMagickInfo("WMFWIN32");
}
