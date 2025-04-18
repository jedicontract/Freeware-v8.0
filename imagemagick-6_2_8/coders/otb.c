/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                              OOO   TTTTT  BBBB                              %
%                             O   O    T    B   B                             %
%                             O   O    T    BBBB                              %
%                             O   O    T    B   B                             %
%                              OOO     T    BBBB                              %
%                                                                             %
%                                                                             %
%                      Read/Write On-The-Air Image Format.                    %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               January 2000                                  %
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

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteOTBImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d O T B I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadOTBImage() reads a on-the-air (level 0) bitmap and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadOTBImage method is:
%
%      Image *ReadOTBImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadOTBImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define GetBit(a,i) (((a) >> (i)) & 1L)

  Image
    *image;

  int
    byte;

  long
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  unsigned char
    bit,
    info,
    depth;

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
    Initialize image structure.
  */
  info=ReadBlobByte(image);
  if (GetBit(info,4) == 0)
    {
      image->columns=ReadBlobByte(image);
      image->rows=ReadBlobByte(image);
    }
  else
    {
      image->columns=ReadBlobMSBShort(image);
      image->rows=ReadBlobMSBShort(image);
    }
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  depth=ReadBlobByte(image);
  if (depth != 1)
    ThrowReaderException(CoderError,"OnlyLevelZerofilesSupported");
  if (AllocateImageColormap(image,2) == MagickFalse)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Convert bi-level image to pixel packets.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    bit=0;
    byte=0;
    for (x=0; x < (long) image->columns; x++)
    {
      if (bit == 0)
        {
          byte=ReadBlobByte(image);
          if (byte == EOF)
            ThrowReaderException(CorruptImageError,"CorruptImage");
        }
      indexes[x]=(byte & (0x01 << (7-bit))) ? 0x00 : 0x01;
      bit++;
      if (bit == 8)
        bit=0;
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
  (void) SyncImage(image);
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
%   R e g i s t e r O T B I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterOTBImage() adds attributes for the OTB image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterOTBImage method is:
%
%      RegisterOTBImage(void)
%
*/
ModuleExport void RegisterOTBImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("OTB");
  entry->decoder=(DecoderHandler *) ReadOTBImage;
  entry->encoder=(EncoderHandler *) WriteOTBImage;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("On-the-air bitmap");
  entry->module=ConstantString("OTB");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r O T B I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterOTBImage() removes format registrations made by the
%  OTB module from the list of supported formats.
%
%  The format of the UnregisterOTBImage method is:
%
%      UnregisterOTBImage(void)
%
*/
ModuleExport void UnregisterOTBImage(void)
{
  (void) UnregisterMagickInfo("OTB");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e O T B I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteOTBImage() writes an image to a file in the On-the-air Bitmap
%  (level 0) image format.
%
%  The format of the WriteOTBImage method is:
%
%      MagickBooleanType WriteOTBImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteOTBImage(const ImageInfo *image_info,Image *image)
{
#define SetBit(a,i,set) \
  a=(unsigned char) ((set) ? (a) | (1L << (i)) : (a) & ~(1L << (i)))

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

  unsigned char
    bit,
    byte,
    info,
    polarity;

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
    Convert image to a bi-level image.
  */
  SetImageType(image,BilevelType);
  polarity=PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2);
  if (image->colors == 2)
    polarity=PixelIntensityToQuantum(&image->colormap[0]) <
      PixelIntensityToQuantum(&image->colormap[1]);
  info=0;
  if ((image->columns >= 256) || (image->rows >= 256))
    SetBit(info,4,1);
  (void) WriteBlobByte(image,info);
  if ((image->columns >= 256) || (image->rows >= 256))
    {
      (void) WriteBlobMSBShort(image,(unsigned short) image->columns);
      (void) WriteBlobMSBShort(image,(unsigned short) image->rows);
    }
  else
    {
      (void) WriteBlobByte(image,(unsigned char) image->columns);
      (void) WriteBlobByte(image,(unsigned char) image->rows);
    }
  (void) WriteBlobByte(image,1);  /* depth */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    bit=0;
    byte=0;
    for (x=0; x < (long) image->columns; x++)
    {
      if (indexes[x] != polarity)
        byte|=0x1 << (7-bit);
      bit++;
      if (bit == 8)
        {
          (void) WriteBlobByte(image,byte);
          bit=0;
          byte=0;
        }
    }
    if (bit != 0)
      (void) WriteBlobByte(image,byte);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SaveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  CloseBlob(image);
  return(MagickTrue);
}
