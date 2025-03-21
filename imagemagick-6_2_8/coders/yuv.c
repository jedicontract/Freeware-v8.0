/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            Y   Y  U   U  V   V                              %
%                             Y Y   U   U  V   V                              %
%                              Y    U   U  V   V                              %
%                              Y    U   U   V V                               %
%                              Y     UUU     V                                %
%                                                                             %
%                                                                             %
%            Read/Write Raw CCIR 601 4:1:1 or 4:2:2 Image Format.             %
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
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/resize.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteYUVImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d Y U V I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadYUVImage() reads an image with digital YUV (CCIR 601 4:1:1, plane
%  or partition interlaced, or 4:2:2 plane, partition interlaced or
%  noninterlaced) bytes and returns it.  It allocates the memory necessary
%  for the new Image structure and returns a pointer to the new image.
%
%  The format of the ReadYUVImage method is:
%
%      Image *ReadYUVImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadYUVImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *chroma_image,
    *image,
    *resize_image;

  InterlaceType
    interlace;

  long
    horizontal_factor,
    vertical_factor,
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *chroma_pixels;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    *scanline;

  /*
    Allocate image structure.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  image->depth=8;
  interlace=image_info->interlace;
  horizontal_factor=2;
  vertical_factor=2;
  if (image_info->sampling_factor != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(image_info->sampling_factor,&geometry_info);
      horizontal_factor=(long) geometry_info.rho;
      vertical_factor=(long) geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        vertical_factor=horizontal_factor;
      if ((horizontal_factor != 1) && (horizontal_factor != 2) &&
          (vertical_factor != 1) && (vertical_factor != 2))
        ThrowReaderException(CorruptImageError,"UnexpectedSamplingFactor");
    }
  if ((interlace == UndefinedInterlace) ||
      ((interlace == NoInterlace) && (vertical_factor == 2)))
    {
      interlace=NoInterlace;    /* CCIR 4:2:2 */
      if (vertical_factor == 2)
        interlace=PlaneInterlace; /* CCIR 4:1:1 */
    }
  if (interlace != PartitionInterlace)
    {
      /*
        Open image file.
      */
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      for (i=0; i < image->offset; i++)
        (void) ReadBlobByte(image);
    }
  /*
    Allocate memory for a scanline.
  */
  if (interlace == NoInterlace)
    scanline=(unsigned char *)
      AcquireMagickMemory((size_t) (2*image->columns+2));
  else
    scanline=(unsigned char *) AcquireMagickMemory((size_t) image->columns);
  if (scanline == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  do
  {
    chroma_image=CloneImage(image,image->columns/horizontal_factor,
      image->rows/vertical_factor,MagickTrue,exception);
    if (chroma_image == (Image *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert raster image to pixel packets.
    */
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (interlace == PartitionInterlace)
      {
        AppendImageFormat("Y",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
      }
    for (y=0; y < (long) image->rows; y++)
    {
      register PixelPacket
        *chroma_pixels;

      if (interlace == NoInterlace)
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            count=ReadBlob(image,(size_t) (2*image->columns),scanline);
          p=scanline;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          chroma_pixels=SetImagePixels(chroma_image,0,y,
            chroma_image->columns,1);
          if (chroma_pixels == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x+=2)
          {
            chroma_pixels->red=0;
            chroma_pixels->green=ScaleCharToQuantum(*p++);
            q->red=ScaleCharToQuantum(*p++);
            q->green=0;
            q->blue=0;
            q++;
            q->green=0;
            q->blue=0;
            chroma_pixels->blue=ScaleCharToQuantum(*p++);
            q->red=ScaleCharToQuantum(*p++);
            chroma_pixels++;
            q++;
          }
        }
      else
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            count=ReadBlob(image,(size_t) image->columns,scanline);
          p=scanline;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleCharToQuantum(*p++);
            q->green=0;
            q->blue=0;
            q++;
          }
        }
      if (SyncImagePixels(image) == MagickFalse)
        break;
      if (interlace == NoInterlace)
        if (SyncImagePixels(chroma_image) == MagickFalse)
          break;
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
    }
    if (interlace == PartitionInterlace)
      {
        CloseBlob(image);
        AppendImageFormat("U",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
      }
    if (interlace != NoInterlace)
      {
        for (y=0; y < (long) chroma_image->rows; y++)
        {
          count=ReadBlob(image,(size_t) chroma_image->columns,scanline);
          p=scanline;
          q=SetImagePixels(chroma_image,0,y,chroma_image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) chroma_image->columns; x++)
          {
            q->red=0;
            q->green=ScaleCharToQuantum(*p++);
            q->blue=0;
            q++;
          }
          if (SyncImagePixels(chroma_image) == MagickFalse)
            break;
        }
      if (interlace == PartitionInterlace)
        {
          CloseBlob(image);
          AppendImageFormat("V",image->filename);
          status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
          if (status == MagickFalse)
            {
              image=DestroyImageList(image);
              return((Image *) NULL);
            }
        }
      for (y=0; y < (long) chroma_image->rows; y++)
      {
        count=ReadBlob(image,(size_t) chroma_image->columns,scanline);
        p=scanline;
        q=GetImagePixels(chroma_image,0,y,chroma_image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) chroma_image->columns; x++)
        {
          q->blue=ScaleCharToQuantum(*p++);
          q++;
        }
        if (SyncImagePixels(chroma_image) == MagickFalse)
          break;
      }
    }
    /*
      Scale image.
    */
    resize_image=ResizeImage(chroma_image,image->columns,image->rows,
      TriangleFilter,1.0,exception);
    chroma_image=DestroyImage(chroma_image);
    if (resize_image == (Image *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    for (y=0; y < (long) image->rows; y++)
    {
      q=GetImagePixels(image,0,y,image->columns,1);
      chroma_pixels=AcquireImagePixels(resize_image,0,y,resize_image->columns,1,
        &resize_image->exception);
      if ((q == (PixelPacket *) NULL) ||
          (chroma_pixels == (const PixelPacket *) NULL))
        break;
      for (x=0; x < (long) image->columns; x++)
      {
        q->green=chroma_pixels->green;
        q->blue=chroma_pixels->blue;
        chroma_pixels++;
        q++;
      }
      if (SyncImagePixels(image) == MagickFalse)
        break;
    }
    resize_image=DestroyImage(resize_image);
    image->colorspace=YCbCrColorspace;
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    if (interlace == PartitionInterlace)
      (void) CopyMagickString(image->filename,image_info->filename,
        MaxTextExtent);
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (interlace == NoInterlace)
      count=ReadBlob(image,(size_t) (2*image->columns),scanline);
    else
      count=ReadBlob(image,(size_t) image->columns,scanline);
    if (count != 0)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
  } while (count != 0);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r Y U V I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterYUVImage() adds attributes for the YUV image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterYUVImage method is:
%
%      RegisterYUVImage(void)
%
*/
ModuleExport void RegisterYUVImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("YUV");
  entry->decoder=(DecoderHandler *) ReadYUVImage;
  entry->encoder=(EncoderHandler *) WriteYUVImage;
  entry->adjoin=MagickFalse;
  entry->raw=MagickTrue;
  entry->description=ConstantString("CCIR 601 4:1:1 or 4:2:2");
  entry->module=ConstantString("YUV");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r Y U V I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterYUVImage() removes format registrations made by the
%  YUV module from the list of supported formats.
%
%  The format of the UnregisterYUVImage method is:
%
%      UnregisterYUVImage(void)
%
*/
ModuleExport void UnregisterYUVImage(void)
{
  (void) UnregisterMagickInfo("YUV");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e Y U V I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteYUVImage() writes an image to a file in the digital YUV
%  (CCIR 601 4:1:1, plane or partition interlaced, or 4:2:2 plane, partition
%  interlaced or noninterlaced) bytes and returns it.
%
%  The format of the WriteYUVImage method is:
%
%      MagickBooleanType WriteYUVImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteYUVImage(const ImageInfo *image_info,Image *image)
{
  Image
    *chroma_image,
    *yuv_image;

  InterlaceType
    interlace;

  long
    horizontal_factor,
    vertical_factor,
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  register const PixelPacket
    *p,
    *s;

  register long
    x;

  unsigned long
    height,
    width;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  interlace=image->interlace;
  horizontal_factor=2;
  vertical_factor=2;
  if (image_info->sampling_factor != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(image_info->sampling_factor,&geometry_info);
      horizontal_factor=(long) geometry_info.rho;
      vertical_factor=(long) geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        vertical_factor=horizontal_factor;
      if ((horizontal_factor != 1) && (horizontal_factor != 2) &&
          (vertical_factor != 1) && (vertical_factor != 2))
        ThrowWriterException(CorruptImageError,"UnexpectedSamplingFactor");
    }
  if ((interlace == UndefinedInterlace) ||
      ((interlace == NoInterlace) && (vertical_factor == 2)))
    {
      interlace=NoInterlace;    /* CCIR 4:2:2 */
      if (vertical_factor == 2)
        interlace=PlaneInterlace; /* CCIR 4:1:1 */
    }
  if (interlace != PartitionInterlace)
    {
      /*
        Open output image file.
      */
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
      if (status == MagickFalse)
        return(status);
    }
  else
    {
      AppendImageFormat("Y",image->filename);
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
      if (status == MagickFalse)
        return(status);
    }
  scene=0;
  do
  {
    /*
      Sample image to an even width and height, if necessary.
    */
    image->depth=8;
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    width=image->columns+(image->columns & (horizontal_factor-1));
    height=image->rows+(image->rows & (vertical_factor-1));
    yuv_image=ResizeImage(image,width,height,TriangleFilter,1.0,
      &image->exception);
    if (yuv_image == (Image *) NULL)
      ThrowWriterException(ResourceLimitError,image->exception.reason);
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(yuv_image,YCbCrColorspace);
    /*
      Downsample image.
    */
    chroma_image=ResizeImage(image,width/horizontal_factor,
      height/vertical_factor,TriangleFilter,1.0,&image->exception);
    if (chroma_image == (Image *) NULL)
      ThrowWriterException(ResourceLimitError,image->exception.reason);
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(chroma_image,YCbCrColorspace);
    if (interlace == NoInterlace)
      {
        /*
          Write noninterlaced YUV.
        */
        for (y=0; y < (long) yuv_image->rows; y++)
        {
          p=AcquireImagePixels(yuv_image,0,y,yuv_image->columns,1,
            &yuv_image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          s=AcquireImagePixels(chroma_image,0,y,chroma_image->columns,1,
            &chroma_image->exception);
          if (s == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) yuv_image->columns; x++)
          {
            (void) WriteBlobByte(image,ScaleQuantumToChar(s->green));
            (void) WriteBlobByte(image,ScaleQuantumToChar(p->red));
            p++;
            (void) WriteBlobByte(image,ScaleQuantumToChar(s->blue));
            (void) WriteBlobByte(image,ScaleQuantumToChar(p->red));
            p++;
            s++;
            x++;
          }
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
        yuv_image=DestroyImage(yuv_image);
      }
    else
      {
        /*
          Initialize Y channel.
        */
        for (y=0; y < (long) yuv_image->rows; y++)
        {
          p=AcquireImagePixels(yuv_image,0,y,yuv_image->columns,1,
            &yuv_image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) yuv_image->columns; x++)
          {
            (void) WriteBlobByte(image,ScaleQuantumToChar(p->red));
            p++;
          }
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
        yuv_image=DestroyImage(yuv_image);
        /*
          Initialize U channel.
        */
        if (interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("U",image->filename);
            status=OpenBlob(image_info,image,WriteBinaryBlobMode,
              &image->exception);
            if (status == MagickFalse)
              return(status);
          }
        for (y=0; y < (long) chroma_image->rows; y++)
        {
          p=AcquireImagePixels(chroma_image,0,y,chroma_image->columns,1,
            &chroma_image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) chroma_image->columns; x++)
          {
            (void) WriteBlobByte(image,ScaleQuantumToChar(p->green));
            p++;
          }
        }
        /*
          Initialize V channel.
        */
        if (interlace == PartitionInterlace)
          {
            CloseBlob(image);
            AppendImageFormat("V",image->filename);
            status=OpenBlob(image_info,image,WriteBinaryBlobMode,
              &image->exception);
            if (status == MagickFalse)
              return(status);
          }
        for (y=0; y < (long) chroma_image->rows; y++)
        {
          p=AcquireImagePixels(chroma_image,0,y,chroma_image->columns,1,
            &chroma_image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) chroma_image->columns; x++)
          {
            (void) WriteBlobByte(image,ScaleQuantumToChar(p->blue));
            p++;
          }
        }
      }
    chroma_image=DestroyImage(chroma_image);
    if (interlace == PartitionInterlace)
      (void) CopyMagickString(image->filename,image_info->filename,
        MaxTextExtent);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(SaveImagesTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (image_info->adjoin != MagickFalse);
  CloseBlob(image);
  return(MagickTrue);
}
