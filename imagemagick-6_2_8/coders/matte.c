/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                     M   M   AAA   TTTTT  TTTTT  EEEEE                       %
%                     MM MM  A   A    T      T    E                           %
%                     M M M  AAAAA    T      T    EEE                         %
%                     M   M  A   A    T      T    E                           %
%                     M   M  A   A    T      T    EEEEE                       %
%                                                                             %
%                                                                             %
%                     Write Matte Channel To MIFF File.                       %
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
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
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
  WriteMATTEImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M A T T E I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMATTEImage() adds attributes for the MATTE image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMATTEImage method is:
%
%      RegisterMATTEImage(void)
%
*/
ModuleExport void RegisterMATTEImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("MATTE");
  entry->encoder=(EncoderHandler *) WriteMATTEImage;
  entry->description=ConstantString("MATTE format");
  entry->module=ConstantString("MATTE");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M A T T E I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMATTEImage() removes format registrations made by the
%  MATTE module from the list of supported formats.
%
%  The format of the UnregisterMATTEImage method is:
%
%      UnregisterMATTEImage(void)
%
*/
ModuleExport void UnregisterMATTEImage(void)
{
  (void) UnregisterMagickInfo("MATTE");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M A T T E I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Function WriteMATTEImage() writes an image of matte bytes to a file.  It
%  consists of data from the matte component of the image [0..255].
%
%  The format of the WriteMATTEImage method is:
%
%      MagickBooleanType WriteMATTEImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteMATTEImage(const ImageInfo *image_info,
  Image *image)
{
  Image
    *matte_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  if (image->matte == MagickFalse)
    ThrowWriterException(CoderError,"ImageDoesNotHaveAMatteChannel");
  matte_image=
    CloneImage(image,image->columns,image->rows,MagickTrue,&image->exception);
  if (matte_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageType(matte_image,TrueColorType);
  matte_image->matte=MagickFalse;
  /*
    Convert image to matte pixels.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    q=SetImagePixels(matte_image,0,y,matte_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=p->opacity;
      q->green=p->opacity;
      q->blue=p->opacity;
      q->opacity=OpaqueOpacity;
      p++;
      q++;
    }
    if (SyncImagePixels(matte_image) == MagickFalse)
      break;
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
  (void) FormatMagickString(matte_image->filename,MaxTextExtent,
    "MIFF:%s",image->filename);
  status=WriteImage(image_info,matte_image);
  matte_image=DestroyImage(matte_image);
  return(status);
}
