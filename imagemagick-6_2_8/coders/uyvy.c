/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        U   U  Y   Y  V   V  Y   Y                           %
%                        U   U   Y Y   V   V   Y Y                            %
%                        U   U    Y    V   V    Y                             %
%                        U   U    Y     V V     Y                             %
%                         UUU     Y      V      Y                             %
%                                                                             %
%                                                                             %
%            Read/Write 16bit/pixel Interleaved YUV Image Format.             %
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

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteUYVYImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d U Y V Y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadUYVYImage() reads an image in the UYVY format and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadUYVYImage method is:
%
%      Image *ReadUYVYImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadUYVYImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  Image
    *image;

  long
    y;

  MagickBooleanType
    status;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  unsigned char
    u,
    v,
    y1,
    y2;

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
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  if ((image->columns % 2) != 0)
    image->columns++;
  (void) CopyMagickString(image->filename,image_info->filename,MaxTextExtent);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    return((Image *) NULL);
  for (i=0; i < image->offset; i++)
    (void) ReadBlobByte(image);
  image->depth=8;
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Accumulate UYVY, then unpack into two pixels.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) (image->columns >> 1); x++)
    {
      u=(unsigned char) ReadBlobByte(image);
      y1=(unsigned char) ReadBlobByte(image);
      v=(unsigned char) ReadBlobByte(image);
      y2=(unsigned char) ReadBlobByte(image);
      q->red=ScaleCharToQuantum(y1);
      q->green=ScaleCharToQuantum(u);
      q->blue=ScaleCharToQuantum(v);
      q++;
      q->red=ScaleCharToQuantum(y2);
      q->green=ScaleCharToQuantum(u);
      q->blue=ScaleCharToQuantum(v);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
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
  image->colorspace=YCbCrColorspace;
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  if (EOFBlob(image) != MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r U Y V Y I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterUYVYImage() adds attributes for the UYVY image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterUYVYImage method is:
%
%      RegisterUYVYImage(void)
%
*/
ModuleExport void RegisterUYVYImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PAL");
  entry->decoder=(DecoderHandler *) ReadUYVYImage;
  entry->encoder=(EncoderHandler *) WriteUYVYImage;
  entry->adjoin=MagickFalse;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=ConstantString("16bit/pixel interleaved YUV");
  entry->module=ConstantString("UYVY");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("UYVY");
  entry->decoder=(DecoderHandler *) ReadUYVYImage;
  entry->encoder=(EncoderHandler *) WriteUYVYImage;
  entry->adjoin=MagickFalse;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=ConstantString("16bit/pixel interleaved YUV");
  entry->module=ConstantString("UYVY");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r U Y V Y I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterUYVYImage() removes format registrations made by the
%  UYVY module from the list of supported formats.
%
%  The format of the UnregisterUYVYImage method is:
%
%      UnregisterUYVYImage(void)
%
*/
ModuleExport void UnregisterUYVYImage(void)
{
  (void) UnregisterMagickInfo("PAL");
  (void) UnregisterMagickInfo("UYVY");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e U Y V Y I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteUYVYImage() writes an image to a file in the digital UYVY
%  format.  This format, used by AccomWSD, is not dramatically higher quality
%  than the 12bit/pixel YUV format, but has better locality.
%
%  The format of the WriteUYVYImage method is:
%
%      MagickBooleanType WriteUYVYImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%      Implicit assumption: number of columns is even.
%
*/
static MagickBooleanType WriteUYVYImage(const ImageInfo *image_info,
  Image *image)
{
  MagickPixelPacket
    pixel;

  Image
    *uyvy_image;

  long
    y;

  MagickBooleanType
    full,
    status;

  register const PixelPacket
    *p;

  register long
    x;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->columns % 2) != 0)
    image->columns++;
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  /*
    Accumulate two pixels, then output.
  */
  uyvy_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (uyvy_image == (Image *) NULL)
    ThrowWriterException(ResourceLimitError,image->exception.reason);
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(uyvy_image,YCbCrColorspace);
  full=MagickFalse;
  (void) ResetMagickMemory(&pixel,0,sizeof(MagickPixelPacket));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(uyvy_image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if (full != MagickFalse)
        {
          pixel.green=(pixel.green+p->green)/2;
          pixel.blue=(pixel.blue+p->blue)/2;
          (void) WriteBlobByte(image,ScaleQuantumToChar((Quantum) pixel.green));
          (void) WriteBlobByte(image,ScaleQuantumToChar((Quantum) pixel.red));
          (void) WriteBlobByte(image,ScaleQuantumToChar((Quantum) pixel.blue));
          (void) WriteBlobByte(image,ScaleQuantumToChar(p->red));
        }
      pixel.red=(double) p->red;
      pixel.green=(double) p->green;
      pixel.blue=(double) p->blue;
      full=full == MagickFalse ? MagickTrue : MagickFalse;
      p++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SaveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  uyvy_image=DestroyImage(uyvy_image);
  CloseBlob(image);
  return(MagickTrue);
}
