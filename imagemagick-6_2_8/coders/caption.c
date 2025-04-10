/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               CCCC   AAA   PPPP   TTTTT  IIIII   OOO   N   N                %
%              C      A   A  P   P    T      I    O   O  NN  N                %
%              C      AAAAA  PPPP     T      I    O   O  N N N                %
%              C      A   A  P        T      I    O   O  N  NN                %
%               CCCC  A   A  P        T    IIIII   OOO   N   N                %
%                                                                             %
%                                                                             %
%                             Read Text Caption.                              %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               February 2002                                 %
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
#include "magick/annotate.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d C A P T I O N I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadCAPTIONImage() reads a CAPTION image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadCAPTIONImage method is:
%
%      Image *ReadCAPTIONImage(const ImageInfo *image_info,
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
static Image *ReadCAPTIONImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    *caption,
    geometry[MaxTextExtent];

  const char
    *gravity;

  DrawInfo
    *draw_info;

  Image
    *image;

  MagickBooleanType
    status;

  register char
    *p,
    *q,
    *s;

  register long
    i;

  size_t
    length;

  TypeMetric
    metrics;

  /*
    Initialize Image structure.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (*image_info->filename != '@')
    caption=AcquireString(image_info->filename);
  else
    {
      caption=FileToString(image_info->filename+1,~0,exception);
      if ((caption != (char *) NULL) && (*caption != '\0'))
        if (caption[strlen(caption)-1] == '\n')
          caption[strlen(caption)-1]='\0';
    }
  if (caption == (char *) NULL)
    return((Image *) NULL);
  image=AllocateImage(image_info);
  if (image->columns == 0)
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  /*
    Format caption.
  */
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  draw_info->text=AcquireString(caption);
  gravity=GetImageOption(image_info,"gravity");
  if (gravity != (char *) NULL)
    draw_info->gravity=(GravityType) ParseMagickOption(MagickGravityOptions,
      MagickFalse,gravity);
  length=strlen(caption)+1;
  for (p=caption; *p != '\0'; p++)
  {
    if (*p == '\r')
      *p=' ';
    if ((*p == '\\') && (*(p+1) == 'n'))
      {
        (void) CopyMagickString(p,p+1,length-(p-caption));
        *p='\n';
      }
  }
  q=draw_info->text;
  s=(char *) NULL;
  for (p=caption; *p != '\0'; p++)
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      s=p;
    *q++=(*p);
    *q='\0';
    status=GetTypeMetrics(image,draw_info,&metrics);
    if (status == MagickFalse)
      ThrowReaderException(TypeError,"UnableToGetTypeMetrics");
    if (*p != '\n')
      if ((metrics.width+metrics.max_advance/4) < (double) image->columns)
        continue;
    if (s == (char *) NULL)
      {
        s=p;
        while ((isspace((int) ((unsigned char) *s)) == 0) && (*s != '\0'))
          s++;
    	}
    if (*s != '\0')
      {
        *s='\n';
        p=s+1;
        s=(char *) NULL;
      }
    q=draw_info->text;
  }
  i=0;
  for (p=caption; *p != '\0'; p++)
    if (*p == '\n')
      i++;
  if (image->rows == 0)
    image->rows=(unsigned long) ((i+1)*(metrics.ascent-metrics.descent));
  SetImageBackgroundColor(image);
  /*
    Draw caption.
  */
  (void) CopyMagickString(draw_info->text,caption,length);
  (void) FormatMagickString(geometry,MaxTextExtent,"+%g+%g",
    metrics.max_advance/4,metrics.ascent);
  if (draw_info->gravity == UndefinedGravity)
    draw_info->geometry=AcquireString(geometry);
  (void) AnnotateImage(image,draw_info);
  draw_info=DestroyDrawInfo(draw_info);
  caption=(char *) RelinquishMagickMemory(caption);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r C A P T I O N I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterCAPTIONImage() adds attributes for the CAPTION image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterCAPTIONImage method is:
%
%      RegisterCAPTIONImage(void)
%
*/
ModuleExport void RegisterCAPTIONImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("CAPTION");
  entry->decoder=(DecoderHandler *) ReadCAPTIONImage;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("Image caption");
  entry->module=ConstantString("CAPTION");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r C A P T I O N I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterCAPTIONImage() removes format registrations made by the
%  CAPTION module from the list of supported formats.
%
%  The format of the UnregisterCAPTIONImage method is:
%
%      UnregisterCAPTIONImage(void)
%
*/
ModuleExport void UnregisterCAPTIONImage(void)
{
  (void) UnregisterMagickInfo("CAPTION");
}
