/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        H   H  TTTTT  M   M  L                               %
%                        H   H    T    MM MM  L                               %
%                        HHHHH    T    M M M  L                               %
%                        H   H    T    M   M  L                               %
%                        H   H    T    M   M  LLLLL                           %
%                                                                             %
%                                                                             %
%                  Write A Client-Side Image Map Using                        %
%                 Image Montage & Directory Information.                      %
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
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/paint.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteHTMLImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s H T M L                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsHTML() returns MagickTrue if the image format type, identified by the
%  magick string, is HTML.
%
%  The format of the IsHTML method is:
%
%      MagickBooleanType IsHTML(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsHTML(const unsigned char *magick,const size_t length)
{
  if (length < 5)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"<html",5) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r H T M L I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterHTMLImage() adds attributes for the HTML image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterHTMLImage method is:
%
%      RegisterHTMLImage(void)
%
*/
ModuleExport void RegisterHTMLImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("HTM");
  entry->encoder=(EncoderHandler *) WriteHTMLImage;
  entry->magick=(MagickHandler *) IsHTML;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Hypertext Markup Language and a client-side image map");
  entry->module=ConstantString("HTML");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("HTML");
  entry->encoder=(EncoderHandler *) WriteHTMLImage;
  entry->magick=(MagickHandler *) IsHTML;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Hypertext Markup Language and a client-side image map");
  entry->module=ConstantString("HTML");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("SHTML");
  entry->encoder=(EncoderHandler *) WriteHTMLImage;
  entry->magick=(MagickHandler *) IsHTML;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Hypertext Markup Language and a client-side image map");
  entry->module=ConstantString("HTML");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r H T M L I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterHTMLImage() removes format registrations made by the
%  HTML module from the list of supported formats.
%
%  The format of the UnregisterHTMLImage method is:
%
%      UnregisterHTMLImage(void)
%
*/
ModuleExport void UnregisterHTMLImage(void)
{
  (void) UnregisterMagickInfo("HTM");
  (void) UnregisterMagickInfo("HTML");
  (void) UnregisterMagickInfo("SHTML");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e H T M L I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteHTMLImage() writes an image in the HTML encoded image format.
%
%  The format of the WriteHTMLImage method is:
%
%      MagickBooleanType WriteHTMLImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteHTMLImage(const ImageInfo *image_info,
  Image *image)
{
  char
    basename[MaxTextExtent],
    buffer[MaxTextExtent],
    filename[MaxTextExtent],
    mapname[MaxTextExtent],
    url[MaxTextExtent];

  Image
    *next;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  RectangleInfo
    geometry;

  register char
    *p;

  /*
    Open image.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  CloseBlob(image);
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  *url='\0';
  if ((LocaleCompare(image_info->magick,"FTP") == 0) ||
      (LocaleCompare(image_info->magick,"HTTP") == 0))
    {
      /*
        Extract URL base from filename.
      */
      p=strrchr(image->filename,'/');
      if (p)
        {
          p++;
          (void) CopyMagickString(url,image_info->magick,MaxTextExtent);
          (void) ConcatenateMagickString(url,":",MaxTextExtent);
          url[strlen(url)+p-image->filename]='\0';
          (void) ConcatenateMagickString(url,image->filename,
            p-image->filename+2);
          (void) CopyMagickString(image->filename,p,MaxTextExtent);
        }
    }
  /*
    Refer to image map file.
  */
  (void) CopyMagickString(filename,image->filename,MaxTextExtent);
  AppendImageFormat("map",filename);
  GetPathComponent(filename,BasePath,basename);
  (void) CopyMagickString(mapname,basename,MaxTextExtent);
  (void) CopyMagickString(image->filename,image_info->filename,MaxTextExtent);
  (void) CopyMagickString(filename,image->filename,MaxTextExtent);
  write_info=CloneImageInfo(image_info);
  write_info->adjoin=MagickTrue;
  status=MagickTrue;
  if (LocaleCompare(image_info->magick,"SHTML") != 0)
    {
      const ImageAttribute
        *attribute;

      /*
        Open output image file.
      */
      status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
      if (status == MagickFalse)
        return(status);
      /*
        Write the HTML image file.
      */
      (void) WriteBlobString(image,"<html version=\"2.0\">\n");
      (void) WriteBlobString(image,"<head>\n");
      attribute=GetImageAttribute(image,"Label");
      if (attribute != (const ImageAttribute *) NULL)
        (void) FormatMagickString(buffer,MaxTextExtent,
          "<title>%s</title>\n",attribute->value);
      else
        {
          GetPathComponent(filename,BasePath,basename);
          (void) FormatMagickString(buffer,MaxTextExtent,
            "<title>%s</title>\n",basename);
        }
      (void) WriteBlobString(image,buffer);
      (void) WriteBlobString(image,"</head>\n");
      (void) WriteBlobString(image,"<body>\n");
      (void) WriteBlobString(image,"<center>\n");
      (void) FormatMagickString(buffer,MaxTextExtent,"<h1>%s</h1>\n",
        image->filename);
      (void) WriteBlobString(image,buffer);
      (void) WriteBlobString(image,"<br><br>\n");
      (void) CopyMagickString(filename,image->filename,MaxTextExtent);
      AppendImageFormat("gif",filename);
      (void) FormatMagickString(buffer,MaxTextExtent,
        "<img ismap usemap=\"#%s\" src=\"%s\" border=0>\n",
        mapname,filename);
      (void) WriteBlobString(image,buffer);
      /*
        Determine the size and location of each image tile.
      */
      SetGeometry(image,&geometry);
      if (image->montage != (char *) NULL)
        (void) ParseAbsoluteGeometry(image->montage,&geometry);
      /*
        Write an image map.
      */
      (void) FormatMagickString(buffer,MaxTextExtent,"<map name=\"%s\">\n",
        mapname);
      (void) WriteBlobString(image,buffer);
      (void) FormatMagickString(buffer,MaxTextExtent,"  <area href=\"%s",
        url);
      (void) WriteBlobString(image,buffer);
      if (image->directory == (char *) NULL)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,
            "%s\" shape=rect coords=\"0,0,%lu,%lu\">\n",image->filename,
            geometry.width-1,geometry.height-1);
          (void) WriteBlobString(image,buffer);
        }
      else
        for (p=image->directory; *p != '\0'; p++)
          if (*p != '\n')
            (void) WriteBlobByte(image,(unsigned char) *p);
          else
            {
              (void) FormatMagickString(buffer,MaxTextExtent,
                "\" shape=rect coords=\"%ld,%ld,%ld,%ld\">\n",geometry.x,
                geometry.y,geometry.x+(long) geometry.width-1,geometry.y+
                (long) geometry.height-1);
              (void) WriteBlobString(image,buffer);
              if (*(p+1) != '\0')
                {
                  (void) FormatMagickString(buffer,MaxTextExtent,
                    "  <area href=%s\"",url);
                  (void) WriteBlobString(image,buffer);
                }
              geometry.x+=geometry.width;
              if ((geometry.x+geometry.width) > image->columns)
                {
                  geometry.x=0;
                  geometry.y+=geometry.height;
                }
            }
      (void) WriteBlobString(image,"</map>\n");
      (void) CopyMagickString(filename,image->filename,MaxTextExtent);
      (void) WriteBlobString(image,"</center>\n");
      (void) WriteBlobString(image,"</body>\n");
      (void) WriteBlobString(image,"</html>\n");
      CloseBlob(image);
      /*
        Write the image as GIF.
      */
      (void) CopyMagickString(image->filename,filename,MaxTextExtent);
      AppendImageFormat("gif",image->filename);
      next=GetNextImageInList(image);
      image->next=NewImageList();
      (void) CopyMagickString(image->magick,"GIF",MaxTextExtent);
      (void) WriteImage(write_info,image);
      image->next=next;
      /*
        Determine image map filename.
      */
      GetPathComponent(image->filename,BasePath,filename);
      (void) ConcatenateMagickString(filename,"_map.shtml",MaxTextExtent);
      (void) CopyMagickString(image->filename,filename,MaxTextExtent);
    }
  /*
    Open image map.
  */
  status=OpenBlob(write_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  write_info=DestroyImageInfo(write_info);
  /*
    Determine the size and location of each image tile.
  */
  SetGeometry(image,&geometry);
  if (image->montage != (char *) NULL)
    (void) ParseAbsoluteGeometry(image->montage,&geometry);
  /*
    Write an image map.
  */
  (void) FormatMagickString(buffer,MaxTextExtent,"<map name=\"%s\">\n",
    mapname);
  (void) WriteBlobString(image,buffer);
  (void) FormatMagickString(buffer,MaxTextExtent,"  <area href=\"%s",url);
  (void) WriteBlobString(image,buffer);
  if (image->directory == (char *) NULL)
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%s\" shape=rect coords=\"0,0,%lu,%lu\">\n",image->filename,
        geometry.width-1,geometry.height-1);
      (void) WriteBlobString(image,buffer);
    }
  else
    for (p=image->directory; *p != '\0'; p++)
      if (*p != '\n')
        (void) WriteBlobByte(image,(unsigned char) *p);
      else
        {
          (void) FormatMagickString(buffer,MaxTextExtent,
            "\" shape=rect coords=\"%ld,%ld,%ld,%ld\">\n",geometry.x,geometry.y,
            geometry.x+(long) geometry.width-1,geometry.y+
            (long) geometry.height-1);
          (void) WriteBlobString(image,buffer);
          if (*(p+1) != '\0')
            {
              (void) FormatMagickString(buffer,MaxTextExtent,
                "  <area href=%s\"",url);
              (void) WriteBlobString(image,buffer);
            }
          geometry.x+=geometry.width;
          if ((geometry.x+geometry.width) > image->columns)
            {
              geometry.x=0;
              geometry.y+=geometry.height;
            }
        }
  (void) WriteBlobString(image,"</map>\n");
  CloseBlob(image);
  (void) CopyMagickString(image->filename,filename,MaxTextExtent);
  return(status);
}
