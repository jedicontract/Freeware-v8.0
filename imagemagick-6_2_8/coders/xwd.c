/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            X   X  W   W  DDDD                               %
%                             X X   W   W  D   D                              %
%                              X    W   W  D   D                              %
%                             X X   W W W  D   D                              %
%                            X   X   W W   DDDD                               %
%                                                                             %
%                                                                             %
%                Read/Write X Windows System Window Dump Format.              %
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
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#if defined(HasX11)
#include "magick/xwindow-private.h"
#if !defined(vms)
#include <X11/XWDFile.h>
#else
#include "XWDFile.h"
#endif
#endif

/*
  Forward declarations.
*/
#if defined(HasX11)
static MagickBooleanType
  WriteXWDImage(const ImageInfo *,Image *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s X W D                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsXWD() returns MagickTrue if the image format type, identified by the
%  magick string, is XWD.
%
%  The format of the IsXWD method is:
%
%      MagickBooleanType IsXWD(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsXWD(const unsigned char *magick,const size_t length)
{
  if (length < 8)
    return(MagickFalse);
  if (memcmp(magick+1,"\000\000",2) == 0)
    {
      if (memcmp(magick+4,"\007\000\000",3) == 0)
        return(MagickTrue);
      if (memcmp(magick+5,"\000\000\007",3) == 0)
        return(MagickTrue);
    }
  return(MagickFalse);
}

#if defined(HasX11)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d X W D I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadXWDImage() reads an X Window System window dump image file and
%  returns it.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadXWDImage method is:
%
%      Image *ReadXWDImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadXWDImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    *comment;

  Image
    *image;

  IndexPacket
    index;

  int
    x_status;

  long
    y;

  MagickStatusType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned long
    pixel;

  size_t
    length;

  ssize_t
    count;

  unsigned long
    lsb_first;

  XColor
    *colors;

  XImage
    *ximage;

  XWDFileHeader
    header;

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
     Read in header information.
  */
  count=ReadBlob(image,sz_XWDheader,(unsigned char *) &header);
  if (count == 0)
    ThrowReaderException(CorruptImageError,"UnableToReadImageHeader");
  image->columns=header.pixmap_width;
  image->rows=header.pixmap_height;
  image->depth=8;
  /*
    Ensure the header byte-order is most-significant byte first.
  */
  lsb_first=1;
  if ((int) (*(char *) &lsb_first) != 0)
    MSBOrderLong((unsigned char *) &header,sz_XWDheader);
  /*
    Check to see if the dump file is in the proper format.
  */
  if (header.file_version != XWD_FILE_VERSION)
    ThrowReaderException(CorruptImageError,"FileFormatVersionMismatch");
  if (header.header_size < sz_XWDheader)
    ThrowReaderException(CorruptImageError,"CorruptImage");
  length=(size_t) header.header_size-sz_XWDheader;
  comment=(char *) AcquireMagickMemory(length+MaxTextExtent);
  if (comment == (char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  count=ReadBlob(image,length,(unsigned char *) comment);
  comment[length]='\0';
  (void) SetImageAttribute(image,"Comment",comment);
  comment=(char *) RelinquishMagickMemory(comment);
  if (count == 0)
    ThrowReaderException(CorruptImageError,
      "UnableToReadWindowNameFromDumpFile");
  /*
    Initialize the X image.
  */
  ximage=(XImage *) AcquireMagickMemory(sizeof(*ximage));
  if (ximage == (XImage *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  ximage->depth=(int) header.pixmap_depth;
  ximage->format=(int) header.pixmap_format;
  ximage->xoffset=(int) header.xoffset;
  ximage->data=(char *) NULL;
  ximage->width=(int) header.pixmap_width;
  ximage->height=(int) header.pixmap_height;
  ximage->bitmap_pad=(int) header.bitmap_pad;
  ximage->bytes_per_line=(int) header.bytes_per_line;
  ximage->byte_order=(int) header.byte_order;
  ximage->bitmap_unit=(int) header.bitmap_unit;
  ximage->bitmap_bit_order=(int) header.bitmap_bit_order;
  ximage->bits_per_pixel=(int) header.bits_per_pixel;
  ximage->red_mask=header.red_mask;
  ximage->green_mask=header.green_mask;
  ximage->blue_mask=header.blue_mask;
  x_status=XInitImage(ximage);
  if (x_status == 0)
    ThrowReaderException(CorruptImageError,"UnableToReadImageHeader");
  /*
    Read colormap.
  */
  colors=(XColor *) NULL;
  if (header.ncolors != 0)
    {
      XWDColor
        color;

      colors=(XColor *)
        AcquireMagickMemory((size_t) header.ncolors*sizeof(*colors));
      if (colors == (XColor *) NULL)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      for (i=0; i < (long) header.ncolors; i++)
      {
        count=ReadBlob(image,sz_XWDColor,(unsigned char *) &color);
        if (count == 0)
          ThrowReaderException(CorruptImageError,
            "UnableToReadColormapFromDumpFile");
        colors[i].pixel=color.pixel;
        colors[i].red=color.red;
        colors[i].green=color.green;
        colors[i].blue=color.blue;
        colors[i].flags=(char) color.flags;
      }
      /*
        Ensure the header byte-order is most-significant byte first.
      */
      lsb_first=1;
      if ((int) (*(char *) &lsb_first) != 0)
        for (i=0; i < (long) header.ncolors; i++)
        {
          MSBOrderLong((unsigned char *) &colors[i].pixel,
            sizeof(colors[i].pixel));
          MSBOrderShort((unsigned char *) &colors[i].red,
            3*sizeof(colors[i].red));
        }
    }
  /*
    Allocate the pixel buffer.
  */
  if (ximage->format == ZPixmap)
    length=(size_t) ximage->bytes_per_line*ximage->height;
  else
    length=(size_t) ximage->bytes_per_line*ximage->height*ximage->depth;
  ximage->data=(char *) AcquireMagickMemory(length);
  if (ximage->data == (char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  count=ReadBlob(image,length,(unsigned char *) ximage->data);
  if (count == 0)
    ThrowReaderException(CorruptImageError,"UnableToReadPixmapFromDumpFile");
  /*
    Convert image to MIFF format.
  */
  image->columns=(unsigned long) ximage->width;
  image->rows=(unsigned long) ximage->height;
  if ((colors == (XColor *) NULL) || (ximage->red_mask != 0) ||
      (ximage->green_mask != 0) || (ximage->blue_mask != 0))
    image->storage_class=DirectClass;
  else
    image->storage_class=PseudoClass;
  image->colors=header.ncolors;
  if (image_info->ping == MagickFalse)
    switch (image->storage_class)
    {
      case DirectClass:
      default:
      {
        register unsigned long
          color;

        unsigned long
          blue_mask,
          blue_shift,
          green_mask,
          green_shift,
          red_mask,
          red_shift;

        /*
          Determine shift and mask for red, green, and blue.
        */
        red_mask=ximage->red_mask;
        red_shift=0;
        while ((red_mask != 0) && ((red_mask & 0x01) == 0))
        {
          red_mask>>=1;
          red_shift++;
        }
        green_mask=ximage->green_mask;
        green_shift=0;
        while ((green_mask != 0) && ((green_mask & 0x01) == 0))
        {
          green_mask>>=1;
          green_shift++;
        }
        blue_mask=ximage->blue_mask;
        blue_shift=0;
        while ((blue_mask != 0) && ((blue_mask & 0x01) == 0))
        {
          blue_mask>>=1;
          blue_shift++;
        }
        /*
          Convert X image to DirectClass packets.
        */
        if (image->colors != 0)
          for (y=0; y < (long) image->rows; y++)
          {
            q=SetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) image->columns; x++)
            {
              pixel=XGetPixel(ximage,(int) x,(int) y);
              index=(IndexPacket) ((pixel >> red_shift) & red_mask);
              q->red=ScaleShortToQuantum(colors[index].red);
              index=(IndexPacket) ((pixel >> green_shift) & green_mask);
              q->green=ScaleShortToQuantum(colors[index].green);
              index=(IndexPacket) ((pixel >> blue_shift) & blue_mask);
              q->blue=ScaleShortToQuantum(colors[index].blue);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
            if (QuantumTick(y,image->rows) != MagickFalse)
              if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                  (QuantumTick(y,image->rows) != MagickFalse))
                {
                  status=image->progress_monitor(LoadImageTag,y,image->rows,
                    image->client_data);
                  if (status == MagickFalse)
                    break;
                }
          }
        else
          for (y=0; y < (long) image->rows; y++)
          {
            q=SetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) image->columns; x++)
            {
              pixel=XGetPixel(ximage,(int) x,(int) y);
              color=(pixel >> red_shift) & red_mask;
              color=(color*65535UL)/red_mask;
              q->red=ScaleShortToQuantum((unsigned short) color);
              color=(pixel >> green_shift) & green_mask;
              color=(color*65535UL)/green_mask;
              q->green=ScaleShortToQuantum((unsigned short) color);
              color=(pixel >> blue_shift) & blue_mask;
              color=(color*65535UL)/blue_mask;
              q->blue=ScaleShortToQuantum((unsigned short) color);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
            if (QuantumTick(y,image->rows) != MagickFalse)
              if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                  (QuantumTick(y,image->rows) != MagickFalse))
                {
                  status=image->progress_monitor(LoadImageTag,y,image->rows,
                    image->client_data);
                  if (status == MagickFalse)
                    break;
                }
          }
        break;
      }
      case PseudoClass:
      {
        /*
          Convert X image to PseudoClass packets.
        */
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].red=ScaleShortToQuantum(colors[i].red);
          image->colormap[i].green=ScaleShortToQuantum(colors[i].green);
          image->colormap[i].blue=ScaleShortToQuantum(colors[i].blue);
        }
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            index=ConstrainColormapIndex(image,XGetPixel(ximage,(int) x,
              (int) y));
            indexes[x]=index;
            *q++=image->colormap[index];
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (QuantumTick(y,image->rows) != MagickFalse)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
        break;
      }
    }
  /*
    Free image and colormap.
  */
  if (header.ncolors != 0)
    colors=(XColor *) RelinquishMagickMemory(colors);
  ximage->data=(char *) RelinquishMagickMemory(ximage->data);
  ximage=(XImage *) RelinquishMagickMemory(ximage);
  if (EOFBlob(image) != MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r X W D I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterXWDImage() adds attributes for the XWD image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterXWDImage method is:
%
%      RegisterXWDImage(void)
%
*/
ModuleExport void RegisterXWDImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("XWD");
#if defined(HasX11)
  entry->decoder=(DecoderHandler *) ReadXWDImage;
  entry->encoder=(EncoderHandler *) WriteXWDImage;
#endif
  entry->magick=(MagickHandler *) IsXWD;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("X Windows system window dump (color)");
  entry->module=ConstantString("XWD");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r X W D I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterXWDImage() removes format registrations made by the
%  XWD module from the list of supported formats.
%
%  The format of the UnregisterXWDImage method is:
%
%      UnregisterXWDImage(void)
%
*/
ModuleExport void UnregisterXWDImage(void)
{
  (void) UnregisterMagickInfo("XWD");
}

#if defined(HasX11)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e X W D I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteXWDImage() writes an image to a file in X window dump
%  rasterfile format.
%
%  The format of the WriteXWDImage method is:
%
%      MagickBooleanType WriteXWDImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteXWDImage(const ImageInfo *image_info,Image *image)
{
  const ImageAttribute
    *attribute;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register long
    i;

  register unsigned char
    *q;

  size_t
    length;

  unsigned char
    *pixels;

  unsigned long
    bits_per_pixel,
    bytes_per_line,
    lsb_first,
    scanline_pad;

  XWDFileHeader
    xwd_info;

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
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  /*
    Initialize XWD file header.
  */
  xwd_info.header_size=(CARD32) sz_XWDheader;
  attribute=GetImageAttribute(image,"Comment");
  if (attribute != (const ImageAttribute *) NULL)
    xwd_info.header_size+=strlen(attribute->value);
  xwd_info.header_size++;
  xwd_info.file_version=(CARD32) XWD_FILE_VERSION;
  xwd_info.pixmap_format=(CARD32) ZPixmap;
  xwd_info.pixmap_depth=(CARD32) (image->storage_class == DirectClass ? 24 : 8);
  xwd_info.pixmap_width=(CARD32) image->columns;
  xwd_info.pixmap_height=(CARD32) image->rows;
  xwd_info.xoffset=(CARD32) 0;
  xwd_info.byte_order=(CARD32) MSBFirst;
  xwd_info.bitmap_unit=(CARD32) (image->storage_class == DirectClass ? 32 : 8);
  xwd_info.bitmap_bit_order=(CARD32) MSBFirst;
  xwd_info.bitmap_pad=(CARD32) (image->storage_class == DirectClass ? 32 : 8);
  bits_per_pixel=(unsigned long) (image->storage_class == DirectClass ? 24 : 8);
  xwd_info.bits_per_pixel=(CARD32) bits_per_pixel;
  bytes_per_line=(CARD32) ((((xwd_info.bits_per_pixel*
    xwd_info.pixmap_width)+((xwd_info.bitmap_pad)-1))/
    (xwd_info.bitmap_pad))*((xwd_info.bitmap_pad) >> 3));
  xwd_info.bytes_per_line=(CARD32) bytes_per_line;
  xwd_info.visual_class=(CARD32)
    (image->storage_class == DirectClass ? DirectColor : PseudoColor);
  xwd_info.red_mask=(CARD32)
    (image->storage_class == DirectClass ? 0xff0000 : 0);
  xwd_info.green_mask=(CARD32)
    (image->storage_class == DirectClass ? 0xff00 : 0);
  xwd_info.blue_mask=(CARD32) (image->storage_class == DirectClass ? 0xff : 0);
  xwd_info.bits_per_rgb=(CARD32) (image->storage_class == DirectClass ? 24 : 8);
  xwd_info.colormap_entries=(CARD32)
    (image->storage_class == DirectClass ? 256 : image->colors);
  xwd_info.ncolors=(unsigned int)
    (image->storage_class == DirectClass ? 0 : image->colors);
  xwd_info.window_width=(CARD32) image->columns;
  xwd_info.window_height=(CARD32) image->rows;
  xwd_info.window_x=0;
  xwd_info.window_y=0;
  xwd_info.window_bdrwidth=(CARD32) 0;
  /*
    Write XWD header.
  */
  lsb_first=1;
  if ((int) (*(char *) &lsb_first) != 0)
    MSBOrderLong((unsigned char *) &xwd_info,sizeof(xwd_info));
  (void) WriteBlob(image,sz_XWDheader,(unsigned char *) &xwd_info);
  if (attribute != (const ImageAttribute *) NULL)
    (void) WriteBlob(image,strlen(attribute->value),(unsigned char *)
      attribute->value);
  (void) WriteBlob(image,1,(const unsigned char *) "\0");
  if (image->storage_class == PseudoClass)
    {
      XColor
        *colors;

      XWDColor
        color;

      /*
        Dump colormap to file.
      */
      colors=(XColor *)
        AcquireMagickMemory((size_t) image->colors*sizeof(*colors));
      if (colors == (XColor *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      for (i=0; i < (long) image->colors; i++)
      {
        colors[i].pixel=(unsigned long) i;
        colors[i].red=ScaleQuantumToShort(image->colormap[i].red);
        colors[i].green=ScaleQuantumToShort(image->colormap[i].green);
        colors[i].blue=ScaleQuantumToShort(image->colormap[i].blue);
        colors[i].flags=(char) (DoRed | DoGreen | DoBlue);
        colors[i].pad='\0';
        if ((int) (*(char *) &lsb_first) != 0)
          {
            MSBOrderLong((unsigned char *) &colors[i].pixel,
              sizeof(colors[i].pixel));
            MSBOrderShort((unsigned char *) &colors[i].red,
              3*sizeof(colors[i].red));
          }
      }
      for (i=0; i < (long) image->colors; i++)
      {
        color.pixel=(CARD32) colors[i].pixel;
        color.red=colors[i].red;
        color.green=colors[i].green;
        color.blue=colors[i].blue;
        color.flags=(CARD8) colors[i].flags;
        (void) WriteBlob(image,sz_XWDColor,(unsigned char *) &color);
      }
      colors=(XColor *) RelinquishMagickMemory(colors);
    }
  /*
    Allocate memory for pixels.
  */
  length=3*bytes_per_line;
  if (image->storage_class == PseudoClass)
    length=bytes_per_line;
  pixels=(unsigned char *) AcquireMagickMemory(length);
  if (pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(pixels,0,length);
  /*
    Convert MIFF to XWD raster pixels.
  */
  scanline_pad=(bytes_per_line-((image->columns*bits_per_pixel) >> 3));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    q=pixels;
    if (image->storage_class == PseudoClass)
      {
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
          *q++=(unsigned char) indexes[x];
      }
    else
      for (x=0; x < (long) image->columns; x++)
      {
        *q++=ScaleQuantumToChar(p->red);
        *q++=ScaleQuantumToChar(p->green);
        *q++=ScaleQuantumToChar(p->blue);
        p++;
      }
    for (x=0; x < (long) scanline_pad; x++)
      *q++='\0';
    (void) WriteBlob(image,(size_t) (q-pixels),pixels);
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
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(MagickTrue);
}
#endif
