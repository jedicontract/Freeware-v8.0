/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            DDDD   IIIII  BBBB                               %
%                            D   D    I    B   B                              %
%                            D   D    I    BBBB                               %
%                            D   D    I    B   B                              %
%                            DDDD   IIIII  BBBB                               %
%                                                                             %
%                                                                             %
%                   Read/Write Windows DIB Image Format.                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
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
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/transform.h"

/*
  Typedef declarations.
*/
typedef struct _DIBInfo
{
  unsigned long
    size;

  long
    width,
    height;

  unsigned short
    planes,
    bits_per_pixel;

  unsigned long
    compression,
    image_size,
    x_pixels,
    y_pixels,
    number_colors,
    red_mask,
    green_mask,
    blue_mask,
    alpha_mask,
    colors_important;

  long
    colorspace;

  PointInfo
    red_primary,
    green_primary,
    blue_primary,
    gamma_scale;
} DIBInfo;

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteDIBImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DecodeImage unpacks the packed image pixels into runlength-encoded
%  pixel packets.
%
%  The format of the DecodeImage method is:
%
%      MagickBooleanType DecodeImage(Image *image,
%        const MagickBooleanType compression,unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o image: The address of a structure of type Image.
%
%    o compression:  A value of 1 means the compressed pixels are runlength
%      encoded for a 256-color bitmap.  A value of 2 means a 16-color bitmap.
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the decoding process.
%
%
*/
static MagickBooleanType DecodeImage(Image *image,
  const MagickBooleanType compression,unsigned char *pixels)
{
#if !defined(__WINDOWS__) || defined(__MINGW32__)
#define BI_RGB  0
#define BI_RLE8  1
#define BI_RLE4  2
#define BI_BITFIELDS  3
#endif

  int
    count;

  long
    y;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register unsigned char
    *p,
    *q;

  unsigned char
    byte;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(pixels != (unsigned char *) NULL);
  (void) ResetMagickMemory(pixels,0,(size_t) image->columns*image->rows*
    sizeof(*pixels));
  byte=0;
  x=0;
  p=pixels;
  q=pixels+(size_t) image->columns*image->rows;
  for (y=0; y < (long) image->rows; )
  {
    if ((p < pixels) || (p >= q))
      break;
    count=ReadBlobByte(image);
    if (count == EOF)
      break;
    if (count != 0)
      {
        count=Min((unsigned long) count,(size_t) (q-p));
        /*
          Encoded mode.
        */
        byte=(unsigned char) ReadBlobByte(image);
        if (compression == BI_RLE8)
          {
            for (i=0; i < count; i++)
              *p++=(unsigned char) byte;
          }
        else
          {
            for (i=0; i < count; i++)
              *p++=(unsigned char)
                ((i & 0x01) != 0 ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
          }
        x+=count;
      }
    else
      {
        /*
          Escape mode.
        */
        count=ReadBlobByte(image);
        if (count == 0x01)
          return(MagickTrue);
        switch (count)
        {
          case 0x00:
          {
            /*
              End of line.
            */
            x=0;
            y++;
            p=pixels+y*image->columns;
            break;
          }
          case 0x02:
          {
            /*
              Delta mode.
            */
            x+=ReadBlobByte(image);
            y+=ReadBlobByte(image);
            p=pixels+y*image->columns+x;
            break;
          }
          default:
          {
            /*
              Absolute mode.
            */
            count=Min((unsigned long) count,(size_t) (q-p));
            if (compression == BI_RLE8)
              for (i=0; i < count; i++)
                *p++=(unsigned char) ReadBlobByte(image);
            else
              for (i=0; i < count; i++)
              {
                if ((i & 0x01) == 0)
                  byte=(unsigned char) ReadBlobByte(image);
                *p++=(unsigned char)
                  ((i & 0x01) != 0 ? (byte & 0x0f) : ((byte >> 4) & 0x0f));
              }
            x+=count;
            /*
              Read pad byte.
            */
            if (compression == BI_RLE8)
              {
                if ((count & 0x01) != 0)
                  (void) ReadBlobByte(image);
              }
            else
              if (((count & 0x03) == 1) || ((count & 0x03) == 2))
                (void) ReadBlobByte(image);
            break;
          }
        }
      }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(LoadImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) ReadBlobByte(image);  /* end of line */
  (void) ReadBlobByte(image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E n c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeImage compresses pixels using a runlength encoded format.
%
%  The format of the EncodeImage method is:
%
%    static MagickBooleanType EncodeImage(Image *image,
%      const unsigned long bytes_per_line,const unsigned char *pixels,
%      unsigned char *compressed_pixels)
%
%  A description of each parameter follows:
%
%    o image:  The image.
%
%    o bytes_per_line: The number of bytes in a scanline of compressed pixels
%
%    o pixels:  The address of a byte (8 bits) array of pixel data created by
%      the compression process.
%
%    o compressed_pixels:  The address of a byte (8 bits) array of compressed
%      pixel data.
%
%
*/
static size_t EncodeImage(Image *image,const unsigned long bytes_per_line,
  const unsigned char *pixels,unsigned char *compressed_pixels)
{
  long
    y;

  MagickBooleanType
    status;

  register const unsigned char
    *p;

  register long
    i,
    x;

  register unsigned char
    *q;

  /*
    Runlength encode pixels.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(pixels != (const unsigned char *) NULL);
  assert(compressed_pixels != (unsigned char *) NULL);
  p=pixels;
  q=compressed_pixels;
  i=0;
  for (y=0; y < (long) image->rows; y++)
  {
    for (x=0; x < (long) bytes_per_line; x+=i)
    {
      /*
        Determine runlength.
      */
      for (i=1; ((x+i) < (long) bytes_per_line); i++)
        if ((*(p+i) != *p) || (i == 255))
          break;
      *q++=(unsigned char) i;
      *q++=(*p);
      p+=i;
    }
    /*
      End of line.
    */
    *q++=0x00;
    *q++=0x00;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(LoadImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    End of bitmap.
  */
  *q++=0;
  *q++=0x01;
  return((size_t) (q-compressed_pixels));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s D I B                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDIB() returns MagickTrue if the image format type, identified by the
%  magick string, is DIB.
%
%  The format of the IsDIB method is:
%
%      MagickBooleanType IsDIB(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsDIB(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if (memcmp(magick,"\050\000",2) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d D I B I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadDIBImage() reads a Microsoft Windows bitmap image file and
%  returns it.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadDIBImage method is:
%
%      image=ReadDIBImage(image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadDIBImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  DIBInfo
    dib_info;

  Image
    *image;

  IndexPacket
    index;

  long
    bit,
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p;

  size_t
    length;

  ssize_t
    count;

  unsigned char
    *pixels;

  unsigned long
    bytes_per_line;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Determine if this is a DIB file.
  */
  (void) ResetMagickMemory(&dib_info,0,sizeof(dib_info));
  dib_info.size=ReadBlobLSBLong(image);
  if (dib_info.size!=40)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  /*
    Microsoft Windows 3.X DIB image file.
  */
  dib_info.width=(short) ReadBlobLSBLong(image);
  dib_info.height=(short) ReadBlobLSBLong(image);
  dib_info.planes=ReadBlobLSBShort(image);
  dib_info.bits_per_pixel=ReadBlobLSBShort(image);
  dib_info.compression=ReadBlobLSBLong(image);
  dib_info.image_size=ReadBlobLSBLong(image);
  dib_info.x_pixels=ReadBlobLSBLong(image);
  dib_info.y_pixels=ReadBlobLSBLong(image);
  dib_info.number_colors=ReadBlobLSBLong(image);
  dib_info.colors_important=ReadBlobLSBLong(image);
  if ((dib_info.compression == BI_BITFIELDS) &&
      ((dib_info.bits_per_pixel == 16) || (dib_info.bits_per_pixel == 32)))
    {
      dib_info.red_mask=ReadBlobLSBLong(image);
      dib_info.green_mask=ReadBlobLSBLong(image);
      dib_info.blue_mask=ReadBlobLSBLong(image);
    }
  image->matte=dib_info.bits_per_pixel == 32 ? MagickTrue : MagickFalse;
  image->columns=dib_info.width;
  image->rows=AbsoluteValue(dib_info.height);
  image->depth=8;
  if ((dib_info.number_colors != 0) || (dib_info.bits_per_pixel < 16))
    {
      image->storage_class=PseudoClass;
      image->colors=dib_info.number_colors;
      if (image->colors == 0)
        image->colors=1L << dib_info.bits_per_pixel;
    }
  if (image_info->size)
    {
      RectangleInfo
        geometry;

      MagickStatusType
        flags;

      flags=ParseAbsoluteGeometry(image_info->size,&geometry);
      if (flags & WidthValue)
        if ((geometry.width != 0) && (geometry.width < image->columns))
          image->columns=geometry.width;
      if (flags & HeightValue)
        if ((geometry.height != 0) && (geometry.height < image->rows))
          image->rows=geometry.height;
    }
  if (image->storage_class == PseudoClass)
    {
      size_t
        packet_size;

      unsigned char
        *dib_colormap;

      /*
        Read DIB raster colormap.
      */
      if (AllocateImageColormap(image,image->colors) == MagickFalse)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      dib_colormap=(unsigned char *) AcquireMagickMemory(4*image->colors);
      if (dib_colormap == (unsigned char *) NULL)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      packet_size=4;
      count=ReadBlob(image,packet_size*image->colors,dib_colormap);
      if (count != (ssize_t) (packet_size*image->colors))
        ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
      p=dib_colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        image->colormap[i].blue=ScaleCharToQuantum(*p++);
        image->colormap[i].green=ScaleCharToQuantum(*p++);
        image->colormap[i].red=ScaleCharToQuantum(*p++);
        if (packet_size == 4)
          p++;
      }
      dib_colormap=(unsigned char *) RelinquishMagickMemory(dib_colormap);
    }
  /*
    Read image data.
  */
  if (dib_info.compression == BI_RLE4)
    dib_info.bits_per_pixel<<=1;
  bytes_per_line=4*((image->columns*dib_info.bits_per_pixel+31)/32);
  length=bytes_per_line*image->rows;
  pixels=(unsigned char *) AcquireMagickMemory((size_t)
    Max(bytes_per_line,image->columns+256)*image->rows*sizeof(*pixels));
  if (pixels == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if ((dib_info.compression == BI_RGB) ||
      (dib_info.compression == BI_BITFIELDS))
    {
      count=ReadBlob(image,length,pixels);
      if (count != (ssize_t) (length))
        ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    }
  else
    {
      /*
        Convert run-length encoded raster pixels.
      */
      status=DecodeImage(image,dib_info.compression ? MagickTrue : MagickFalse,
        pixels);
      if (status == MagickFalse)
        ThrowReaderException(CorruptImageError,"UnableToRunlengthDecodeImage");
    }
  /*
    Initialize image structure.
  */
  image->units=PixelsPerCentimeterResolution;
  image->x_resolution=dib_info.x_pixels/100.0;
  image->y_resolution=dib_info.y_pixels/100.0;
  /*
    Convert DIB raster image to pixel packets.
  */
  switch (dib_info.bits_per_pixel)
  {
    case 1:
    {
      /*
        Convert bitmap scanline.
      */
      for (y=(long) image->rows-1; y >= 0; y--)
      {
        p=pixels+(image->rows-y-1)*bytes_per_line;
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-7); x+=8)
        {
          for (bit=0; bit < 8; bit++)
          {
            index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
            indexes[x+bit]=index;
            *q++=image->colormap[index];
          }
          p++;
        }
        if ((image->columns % 8) != 0)
          {
            for (bit=0; bit < (long) (image->columns % 8); bit++)
            {
              index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
              indexes[x+bit]=index;
              *q++=image->colormap[index];
            }
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(LoadImageTag,image->rows-y-1,
                image->rows,image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 4:
    {
      /*
        Convert PseudoColor scanline.
      */
      for (y=(long) image->rows-1; y >= 0; y--)
      {
        p=pixels+(image->rows-y-1)*bytes_per_line;
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-1); x+=2)
        {
          index=ConstrainColormapIndex(image,(*p >> 4) & 0xf);
          indexes[x]=index;
          *q++=image->colormap[index];
          index=ConstrainColormapIndex(image,*p & 0xf);
          indexes[x+1]=index;
          *q++=image->colormap[index];
          p++;
        }
        if ((image->columns % 2) != 0)
          {
            index=ConstrainColormapIndex(image,(*p >> 4) & 0xf);
            indexes[x]=index;
            *q++=image->colormap[index];
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(LoadImageTag,image->rows-y-1,
                image->rows,image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 8:
    {
      /*
        Convert PseudoColor scanline.
      */
      if ((dib_info.compression == BI_RLE8) ||
          (dib_info.compression == BI_RLE4))
        bytes_per_line=image->columns;
      for (y=(long) image->rows-1; y >= 0; y--)
      {
        p=pixels+(image->rows-y-1)*bytes_per_line;
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          index=ConstrainColormapIndex(image,*p);
          indexes[x]=index;
          *q=image->colormap[index];
          p++;
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(LoadImageTag,image->rows-y-1,
                image->rows,image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 16:
    {
      unsigned short
        word;

      /*
        Convert PseudoColor scanline.
      */
      image->storage_class=DirectClass;
      if (dib_info.compression == BI_RLE8)
        bytes_per_line=2*image->columns;
      for (y=(long) image->rows-1; y >= 0; y--)
      {
        p=pixels+(image->rows-y-1)*bytes_per_line;
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          word=(*p++);
          word|=(*p++ << 8);
          if (dib_info.red_mask == 0)
            {
              q->red=ScaleCharToQuantum(ScaleColor5to8((word >> 10) & 0x1f));
              q->green=ScaleCharToQuantum(ScaleColor5to8((word >> 5) & 0x1f));
              q->blue=ScaleCharToQuantum(ScaleColor5to8(word & 0x1f));
            }
          else
            {
              q->red=ScaleCharToQuantum(ScaleColor5to8((word >> 11) & 0x1f));
              q->green=ScaleCharToQuantum(ScaleColor6to8((word >> 5) & 0x3f));
              q->blue=ScaleCharToQuantum(ScaleColor5to8(word & 0x1f));
            }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(LoadImageTag,image->rows-y-1,
                image->rows,image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 24:
    case 32:
    {
      /*
        Convert DirectColor scanline.
      */
      for (y=(long) image->rows-1; y >= 0; y--)
      {
        p=pixels+(image->rows-y-1)*bytes_per_line;
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          q->blue=ScaleCharToQuantum(*p++);
          q->green=ScaleCharToQuantum(*p++);
          q->red=ScaleCharToQuantum(*p++);
          if (image->matte != MagickFalse)
            q->opacity=ScaleCharToQuantum(*p++);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(LoadImageTag,image->rows-y-1,
                image->rows,image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    default:
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  }
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  if (EOFBlob(image) != MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  if (dib_info.height < 0)
    {
      Image
        *flipped_image;

      /*
        Correct image orientation.
      */
      flipped_image=FlipImage(image,exception);
      if (flipped_image == (Image *) NULL)
        {
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      image=DestroyImage(image);
      image=flipped_image;
    }
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r D I B I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterDIBImage() adds attributes for the DIB image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterDIBImage method is:
%
%      RegisterDIBImage(void)
%
*/
ModuleExport void RegisterDIBImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("DIB");
  entry->decoder=(DecoderHandler *) ReadDIBImage;
  entry->encoder=(EncoderHandler *) WriteDIBImage;
  entry->magick=(MagickHandler *) IsDIB;
  entry->adjoin=MagickFalse;
  entry->stealth=MagickTrue;
  entry->description=ConstantString(
    "Microsoft Windows 3.X Packed Device-Independent Bitmap");
  entry->module=ConstantString("DIB");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r D I B I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterDIBImage() removes format registrations made by the
%  DIB module from the list of supported formats.
%
%  The format of the UnregisterDIBImage method is:
%
%      UnregisterDIBImage(void)
%
*/
ModuleExport void UnregisterDIBImage(void)
{
  (void) UnregisterMagickInfo("DIB");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e D I B I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteDIBImage() writes an image in Microsoft Windows bitmap encoded
%  image format.
%
%  The format of the WriteDIBImage method is:
%
%      MagickBooleanType WriteDIBImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteDIBImage(const ImageInfo *image_info,Image *image)
{
  DIBInfo
    dib_info;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register unsigned char
    *q;

  unsigned char
    *dib_data,
    *pixels;

  unsigned long
    bytes_per_line;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  /*
    Initialize DIB raster file header.
  */
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  if (image->storage_class == DirectClass)
    {
      /*
        Full color DIB raster.
      */
      dib_info.number_colors=0;
      dib_info.bits_per_pixel=image->matte ? 32 : 24;
    }
  else
    {
      /*
        Colormapped DIB raster.
      */
      dib_info.bits_per_pixel=8;
			if (image_info->depth > 8)
        dib_info.bits_per_pixel=16;
      if (IsMonochromeImage(image,&image->exception) != MagickFalse)
        dib_info.bits_per_pixel=1;
      dib_info.number_colors=(dib_info.bits_per_pixel == 16) ? 0 :
        (1UL << dib_info.bits_per_pixel);
    }
  bytes_per_line=4*((image->columns*dib_info.bits_per_pixel+31)/32);
  dib_info.size=40;
  dib_info.width=(long) image->columns;
  dib_info.height=(long) image->rows;
  dib_info.planes=1;
  dib_info.compression=dib_info.bits_per_pixel == 16 ? BI_BITFIELDS : BI_RGB;
  dib_info.image_size=bytes_per_line*image->rows;
  dib_info.x_pixels=75*39;
  dib_info.y_pixels=75*39;
  switch (image->units)
  {
    case UndefinedResolution:
    case PixelsPerInchResolution:
    {
      dib_info.x_pixels=(unsigned long) (100.0*image->x_resolution/2.54);
      dib_info.y_pixels=(unsigned long) (100.0*image->y_resolution/2.54);
      break;
    }
    case PixelsPerCentimeterResolution:
    {
      dib_info.x_pixels=(unsigned long) (100.0*image->x_resolution);
      dib_info.y_pixels=(unsigned long) (100.0*image->y_resolution);
      break;
    }
  }
  dib_info.colors_important=dib_info.number_colors;
  /*
    Convert MIFF to DIB raster pixels.
  */
  pixels=(unsigned char *) AcquireMagickMemory(dib_info.image_size);
  if (pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(pixels,0,dib_info.image_size);
  switch (dib_info.bits_per_pixel)
  {
    case 1:
    {
      register unsigned char
        bit,
        byte;

      /*
        Convert PseudoClass image to a DIB monochrome image.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        q=pixels+(image->rows-y-1)*bytes_per_line;
        bit=0;
        byte=0;
        for (x=0; x < (long) image->columns; x++)
        {
          byte<<=1;
          byte|=indexes[x] ? 0x01 : 0x00;
          bit++;
          if (bit == 8)
            {
              *q++=byte;
              bit=0;
              byte=0;
            }
           p++;
         }
       if (bit != 0)
         {
           *q++=(unsigned char) (byte << (8-bit));
           x++;
         }
       for (x=(long) (image->columns+7)/8; x < (long) bytes_per_line; x++)
         *q++=0x00;
       if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 8:
    {
      /*
        Convert PseudoClass packet to DIB pixel.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        q=pixels+(image->rows-y-1)*bytes_per_line;
        for (x=0; x < (long) image->columns; x++)
          *q++=indexes[x];
        for ( ; x < (long) bytes_per_line; x++)
          *q++=0x00;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 16:
    {
      unsigned short
        word;
      /*
        Convert PseudoClass packet to DIB pixel. 
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels+(image->rows-y-1)*bytes_per_line;
        for (x=0; x < (long) image->columns; x++)
        {
          word=(ScaleColor8to5(ScaleQuantumToChar(p->red)) << 11) |
            (ScaleColor8to6(ScaleQuantumToChar(p->green)) << 5) |
            (ScaleColor8to5(ScaleQuantumToChar(p->blue) << 0));
          *q++=(unsigned char)(word & 0xff);
          *q++=(unsigned char)(word >> 8);
          p++;
        }
        for (x=2*image->columns; x < (long) bytes_per_line; x++)
          *q++=0x00;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
    case 24:
    case 32:
    {
      /*
        Convert DirectClass packet to DIB RGB pixel.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels+(image->rows-y-1)*bytes_per_line;
        for (x=0; x < (long) image->columns; x++)
        {
          *q++=ScaleQuantumToChar(p->blue);
          *q++=ScaleQuantumToChar(p->green);
          *q++=ScaleQuantumToChar(p->red);
          if (image->matte != MagickFalse)
            *q++=ScaleQuantumToChar(p->opacity);
          p++;
        }
        if (dib_info.bits_per_pixel == 24)
          for (x=3*image->columns; x < (long) bytes_per_line; x++)
            *q++=0x00;
        if (image->previous == (Image *) NULL)
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
      }
      break;
    }
  }
  if (dib_info.bits_per_pixel == 8)
    if (image->compression != NoCompression)
      {
        size_t
          length;

        /*
          Convert run-length encoded raster pixels.
        */
        length=2*(bytes_per_line+2)*(image->rows+2)+2;
        dib_data=(unsigned char *) AcquireMagickMemory(length);
        if (pixels == (unsigned char *) NULL)
          {
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          }
        dib_info.image_size=EncodeImage(image,bytes_per_line,pixels,dib_data);
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        pixels=dib_data;
        dib_info.compression = BI_RLE8;
      }
  /*
    Write DIB header.
  */
  (void) WriteBlobLSBLong(image,dib_info.size);
  (void) WriteBlobLSBLong(image,dib_info.width);
  (void) WriteBlobLSBLong(image,dib_info.height);
  (void) WriteBlobLSBShort(image,dib_info.planes);
  (void) WriteBlobLSBShort(image,dib_info.bits_per_pixel);
  (void) WriteBlobLSBLong(image,dib_info.compression);
  (void) WriteBlobLSBLong(image,dib_info.image_size);
  (void) WriteBlobLSBLong(image,dib_info.x_pixels);
  (void) WriteBlobLSBLong(image,dib_info.y_pixels);
  (void) WriteBlobLSBLong(image,dib_info.number_colors);
  (void) WriteBlobLSBLong(image,dib_info.colors_important);
  if (image->storage_class == PseudoClass)
    {
      if (dib_info.bits_per_pixel <= 8)
        {
          unsigned char
            *dib_colormap;

          /*
            Dump colormap to file.
          */
          dib_colormap=(unsigned char *) AcquireMagickMemory((size_t)
            (4*(1 << dib_info.bits_per_pixel)));
          if (dib_colormap == (unsigned char *) NULL)
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          q=dib_colormap;
          for (i=0; i < (long) Min(image->colors,dib_info.number_colors); i++)
          {
            *q++=ScaleQuantumToChar(image->colormap[i].blue);
            *q++=ScaleQuantumToChar(image->colormap[i].green);
            *q++=ScaleQuantumToChar(image->colormap[i].red);
            *q++=(Quantum) 0x0;
          }
          for ( ; i < (1L << dib_info.bits_per_pixel); i++)
          {
            *q++=(Quantum) 0x0;
            *q++=(Quantum) 0x0;
            *q++=(Quantum) 0x0;
            *q++=(Quantum) 0x0;
          }
          (void) WriteBlob(image,4*(1 << dib_info.bits_per_pixel),dib_colormap);
          dib_colormap=(unsigned char *) RelinquishMagickMemory(dib_colormap);
        }
      else
        if ((dib_info.bits_per_pixel == 16) &&
            (dib_info.compression == BI_BITFIELDS))
          {
            (void) WriteBlobLSBLong(image,0xf800);
            (void) WriteBlobLSBLong(image,0x07e0);
            (void) WriteBlobLSBLong(image,0x001f);
          }
    }
  (void) WriteBlob(image,dib_info.image_size,pixels);
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(MagickTrue);
}
