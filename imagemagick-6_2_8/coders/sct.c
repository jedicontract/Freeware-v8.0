/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            SSSSS   CCCC  TTTTT                              %
%                            SS     C        T                                %
%                             SSS   C        T                                %
%                               SS  C        T                                %
%                            SSSSS   CCCC    T                                %
%                                                                             %
%                                                                             %
%                    Read Scitex HandShake Image Format.                      %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S C T                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsSCT() returns MagickTrue if the image format type, identified by the
%  magick string, is SCT.
%
%  The format of the IsSCT method is:
%
%      MagickBooleanType IsSCT(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsSCT(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"CT",2) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S C T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadSCTImage() reads a Scitex image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadSCTImage method is:
%
%      Image *ReadSCTImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadSCTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    magick[2];

  Image
    *image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    height,
    width;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  ssize_t
    count;

  unsigned char
    buffer[768];

  unsigned long
    separations,
    separations_mask,
    units;

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
    Read control block.
  */
  count=ReadBlob(image,80,buffer);
  count=ReadBlob(image,2,(unsigned char *) magick);
  if ((LocaleNCompare((char *) magick,"CT",2) != 0) &&
      (LocaleNCompare((char *) magick,"LW",2) != 0) &&
      (LocaleNCompare((char *) magick,"BM",2) != 0) &&
      (LocaleNCompare((char *) magick,"PG",2) != 0) &&
      (LocaleNCompare((char *) magick,"TX",2) != 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  if ((LocaleNCompare((char *) magick,"LW",2) == 0) ||
      (LocaleNCompare((char *) magick,"BM",2) == 0) ||
      (LocaleNCompare((char *) magick,"PG",2) == 0) ||
      (LocaleNCompare((char *) magick,"TX",2) == 0))
    ThrowReaderException(CoderError,"OnlyContinuousTonePictureSupported");
  count=ReadBlob(image,174,buffer);
  count=ReadBlob(image,768,buffer);
  /*
    Read paramter block.
  */
  units=ReadBlobByte(image);
  if (units == 0)
    image->units=PixelsPerCentimeterResolution;
  separations=ReadBlobByte(image);
  separations_mask=ReadBlobMSBShort(image);
  count=ReadBlob(image,14,buffer);
  buffer[14]='\0';
  height=atof((char *) buffer);
  count=ReadBlob(image,14,buffer);
  width=atof((char *) buffer);
  count=ReadBlob(image,12,buffer);
  buffer[12]='\0';
  image->rows=atol((char *) buffer);
  count=ReadBlob(image,12,buffer);
  image->columns=atol((char *) buffer);
  count=ReadBlob(image,200,buffer);
  count=ReadBlob(image,768,buffer);
  if (separations_mask == 0x0f)
    image->colorspace=CMYKColorspace;
  image->x_resolution=image->columns/width;
  image->y_resolution=image->rows/height;
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Convert SCT raster image to pixel packets.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=(Quantum) (QuantumRange-ScaleCharToQuantum(ReadBlobByte(image)));
      q++;
    }
    if ((image->columns % 2) != 0)
      (void) ReadBlobByte(image);  /* pad */
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->green=(Quantum) (QuantumRange-ScaleCharToQuantum(ReadBlobByte(image)));
      q++;
    }
    if ((image->columns % 2) != 0)
      (void) ReadBlobByte(image);  /* pad */
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      q->blue=(Quantum) (QuantumRange-ScaleCharToQuantum(ReadBlobByte(image)));
      q++;
    }
    if ((image->columns % 2) != 0)
      (void) ReadBlobByte(image);  /* pad */
    if (image->colorspace == CMYKColorspace)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          indexes[x]=(IndexPacket) (QuantumRange-
            ScaleCharToQuantum(ReadBlobByte(image)));
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->columns % 2) != 0)
          (void) ReadBlobByte(image);  /* pad */
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
%   R e g i s t e r S C T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterSCTImage() adds attributes for the SCT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterSCTImage method is:
%
%      RegisterSCTImage(void)
%
*/
ModuleExport void RegisterSCTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("SCT");
  entry->decoder=(DecoderHandler *) ReadSCTImage;
  entry->magick=(MagickHandler *) IsSCT;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("Scitex HandShake");
  entry->module=ConstantString("SCT");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S C T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterSCTImage() removes format registrations made by the
%  SCT module from the list of supported formats.
%
%  The format of the UnregisterSCTImage method is:
%
%      UnregisterSCTImage(void)
%
*/
ModuleExport void UnregisterSCTImage(void)
{
  (void) UnregisterMagickInfo("SCT");
}
