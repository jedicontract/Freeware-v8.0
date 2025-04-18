/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   W   W  PPPP                               %
%                            P   P  W   W  P   P                              %
%                            PPPP   W   W  PPPP                               %
%                            P      W W W  P                                  %
%                            P       W W   P                                  %
%                                                                             %
%                                                                             %
%                    Read Seattle Film Works Image Format.                    %
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
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/resource_.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P W P                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPWP() returns MagickTrue if the image format type, identified by the
%  magick string, is PWP.
%
%  The format of the IsPWP method is:
%
%      MagickBooleanType IsPWP(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsPWP(const unsigned char *magick,const size_t length)
{
  if (length < 5)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"SFW95",5) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P W P I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPWPImage() reads a Seattle Film Works multi-image file and returns
%  it.  It allocates the memory necessary for the new Image structure and
%  returns a pointer to the new image.
%
%  The format of the ReadPWPImage method is:
%
%      Image *ReadPWPImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadPWPImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  FILE
    *file;

  Image
    *image,
    *next_image,
    *pwp_image;

  ImageInfo
    *read_info;

  int
    c,
    unique_file;

  MagickBooleanType
    status;

  register Image
    *p;

  register long
    i;

  ssize_t
    count;

  unsigned char
    magick[MaxTextExtent];

  unsigned long
    filesize;

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
  pwp_image=AllocateImage(image_info);
  image=pwp_image;
  status=OpenBlob(image_info,pwp_image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    return((Image *) NULL);
  count=ReadBlob(pwp_image,5,magick);
  if ((count == 0) || (LocaleNCompare((char *) magick,"SFW95",5) != 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  read_info=CloneImageInfo(image_info);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  unique_file=AcquireUniqueFileResource(read_info->filename);
  for ( ; ; )
  {
    for (c=ReadBlobByte(pwp_image); c != EOF; c=ReadBlobByte(pwp_image))
    {
      for (i=0; i < 17; i++)
        magick[i]=magick[i+1];
      magick[17]=(unsigned char) c;
      if (LocaleNCompare((char *) (magick+12),"SFW94A",6) == 0)
        break;
    }
    if (c == EOF)
      break;
    if (LocaleNCompare((char *) (magick+12),"SFW94A",6) != 0)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    /*
      Dump SFW image to a temporary file.
    */
    file=(FILE *) NULL;
    if (unique_file != -1)
      file=fdopen(unique_file,"wb");
    if ((unique_file == -1) || (file == (FILE *) NULL))
      {
        ThrowFileException(exception,FileOpenError,"UnableToWriteFile",
          image->filename);
        image=DestroyImageList(image);
        return((Image *) NULL);
      }
    (void) fwrite("SFW94A",1,6,file);
    filesize=65535L*magick[2]+256L*magick[1]+magick[0];
    for (i=0; i < (long) filesize; i++)
    {
      c=ReadBlobByte(pwp_image);
      (void) fputc(c,file);
    }
    (void) fclose(file);
    next_image=ReadImage(read_info,exception);
    if (next_image == (Image *) NULL)
      break;
    (void) FormatMagickString(next_image->filename,MaxTextExtent,
      "slide_%02ld.sfw",next_image->scene);
    if (image == (Image *) NULL)
      image=next_image;
    else
      {
        /*
          Link image into image list.
        */
        for (p=image; p->next != (Image *) NULL; p=GetNextImageInList(p));
        next_image->previous=p;
        next_image->scene=p->scene+1;
        p->next=next_image;
      }
    if (image_info->number_scenes != 0)
      if (next_image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(LoadImagesTag,TellBlob(pwp_image),
          GetBlobSize(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) RelinquishUniqueFileResource(read_info->filename);
  read_info=DestroyImageInfo(read_info);
  CloseBlob(pwp_image);
  pwp_image=DestroyImage(pwp_image);
  if (EOFBlob(image) != MagickFalse)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      (void) ThrowMagickException(exception,GetMagickModule(),CorruptImageError,
        "UnexpectedEndOfFile","`%s': %s",image->filename,message);
      message=(char *) RelinquishMagickMemory(message);
    }
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P W P I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPWPImage() adds attributes for the PWP image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPWPImage method is:
%
%      RegisterPWPImage(void)
%
*/
ModuleExport void RegisterPWPImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PWP");
  entry->decoder=(DecoderHandler *) ReadPWPImage;
  entry->magick=(MagickHandler *) IsPWP;
  entry->description=ConstantString("Seattle Film Works");
  entry->module=ConstantString("PWP");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P W P I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPWPImage() removes format registrations made by the
%  PWP module from the list of supported formats.
%
%  The format of the UnregisterPWPImage method is:
%
%      UnregisterPWPImage(void)
%
*/
ModuleExport void UnregisterPWPImage(void)
{
  (void) UnregisterMagickInfo("PWP");
}
