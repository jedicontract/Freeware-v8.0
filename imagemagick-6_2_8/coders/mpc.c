/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            M   M  PPPP    CCCC                              %
%                            MM MM  P   P  C                                  %
%                            M M M  PPPP   C                                  %
%                            M   M  P      C                                  %
%                            M   M  P       CCCC                              %
%                                                                             %
%                                                                             %
%              Read/Write Magick Persistant Cache Image Format.               %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/hashmap.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/profile.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define PopCharPixel(q,pixel) \
{ \
  *(q)++=(unsigned char) (pixel); \
}
#define PopLongPixel(q,pixel) \
{ \
  *(q)++=(unsigned char) ((pixel) >> 24); \
  *(q)++=(unsigned char) ((pixel) >> 16); \
  *(q)++=(unsigned char) ((pixel) >> 8); \
  *(q)++=(unsigned char) (pixel); \
}
#define PopShortPixel(q,pixel) \
{ \
  *(q)++=(unsigned char) ((pixel) >> 8); \
  *(q)++=(unsigned char) (pixel); \
}
#define PushCharPixel(pixel,p) \
{ \
  pixel=(unsigned char) (*(p)); \
  (p)++; \
}
#define PushLongPixel(pixel,p) \
{ \
  pixel=(unsigned long) \
    ((*(p) << 24) | (*((p)+1) << 16) | (*((p)+2) << 8) | *((p)+3)); \
  (p)+=4; \
}
#define PushShortPixel(pixel,p) \
{ \
  pixel=(unsigned short) ((*(p) << 8) | *((p)+1)); \
  (p)+=2; \
}

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteMPCImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M P C                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMPC() returns MagickTrue if the image format type, identified by the
%  magick string, is an Magick Persistent Cache image.
%
%  The format of the IsMPC method is:
%
%      MagickBooleanType IsMPC(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsMPC(const unsigned char *magick,const size_t length)
{
  if (length < 14)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"id=MagickCache",14) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d C A C H E I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadMPCImage() reads an Magick Persistent Cache image file and returns
%  it.  It allocates the memory necessary for the new Image structure and
%  returns a pointer to the new image.
%
%  The format of the ReadMPCImage method is:
%
%      Image *ReadMPCImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  Decompression code contributed by Kyle Shorter.
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadMPCImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    cache_filename[MaxTextExtent],
    id[MaxTextExtent],
    keyword[MaxTextExtent],
    *options;

  GeometryInfo
    geometry_info;

  Image
    *image;

  int
    c;

  LinkedListInfo
    *profiles;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  MagickStatusType
    flags;

  register long
    i;

  register unsigned char
    *p;

  size_t
    length;

  ssize_t
    count;

  StringInfo
    *profile;

  unsigned long
    depth,
    quantum_depth;

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
  (void) CopyMagickString(cache_filename,image->filename,MaxTextExtent);
  AppendImageFormat("cache",cache_filename);
  c=ReadBlobByte(image);
  if (c == EOF)
    {
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  *id='\0';
  (void) ResetMagickMemory(keyword,0,sizeof(keyword));
  offset=0;
  do
  {
    /*
      Decode image header;  header terminates one character beyond a ':'.
    */
    profiles=(LinkedListInfo *) NULL;
    length=MaxTextExtent;
    options=AcquireString((char *) NULL);
    quantum_depth=QuantumDepth;
    image->depth=8;
    image->compression=NoCompression;
    while ((isgraph(c) != MagickFalse) && (c != (int) ':'))
    {
      register char
        *p;

      if (c == (int) '{')
        {
          char
            *comment;

          /*
            Read comment-- any text between { }.
          */
          length=MaxTextExtent;
          comment=AcquireString((char *) NULL);
          for (p=comment; comment != (char *) NULL; p++)
          {
            c=ReadBlobByte(image);
            if ((c == EOF) || (c == (int) '}'))
              break;
            if ((size_t) (p-comment+1) >= length)
              {
                *p='\0';
                length<<=1;
                comment=(char *) ResizeMagickMemory(comment,
                  (length+MaxTextExtent)*sizeof(*comment));
                if (comment == (char *) NULL)
                  break;
                p=comment+strlen(comment);
              }
            *p=(char) c;
          }
          if (comment == (char *) NULL)
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          *p='\0';
          (void) SetImageAttribute(image,"Comment",comment);
          comment=(char *) RelinquishMagickMemory(comment);
          c=ReadBlobByte(image);
        }
      else
        if (isalnum(c) != MagickFalse)
          {
            /*
              Get the keyword.
            */
            p=keyword;
            do
            {
              if (isspace((int) ((unsigned char) c)) != 0)
                break;
              if (c == (int) '=')
                break;
              if ((size_t) (p-keyword) < (MaxTextExtent/2))
                *p++=(char) c;
              c=ReadBlobByte(image);
            } while (c != EOF);
            *p='\0';
            p=options;
            while (isspace((int) ((unsigned char) c)) != 0)
              c=ReadBlobByte(image);
            if (c == (int) '=')
              {
                /*
                  Get the keyword value.
                */
                c=ReadBlobByte(image);
                while ((c != (int) '}') && (c != EOF))
                {
                  if ((size_t) (p-options+1) >= length)
                    {
                      *p='\0';
                      length<<=1;
                      options=(char *) ResizeMagickMemory(options,
                        (length+MaxTextExtent)*sizeof(*options));
                      if (options == (char *) NULL)
                        break;
                      p=options+strlen(options);
                    }
                  if (options == (char *) NULL)
                    ThrowReaderException(ResourceLimitError,
                      "MemoryAllocationFailed");
                  *p++=(char) c;
                  c=ReadBlobByte(image);
                  if (*options != '{')
                    if (isspace((int) ((unsigned char) c)) != 0)
                      break;
                }
              }
            *p='\0';
            if (*options == '{')
              (void) CopyMagickString(options,options+1,MaxTextExtent);
            /*
              Assign a value to the specified keyword.
            */
            switch (*keyword)
            {
              case 'b':
              case 'B':
              {
                if (LocaleCompare(keyword,"background-color") == 0)
                  {
                    (void) QueryColorDatabase(options,&image->background_color,
                      exception);
                    break;
                  }
                if (LocaleCompare(keyword,"blue-primary") == 0)
                  {
                    flags=ParseGeometry(options,&geometry_info);
                    image->chromaticity.blue_primary.x=geometry_info.rho;
                    image->chromaticity.blue_primary.y=geometry_info.sigma;
                    if ((flags & SigmaValue) == 0)
                      image->chromaticity.blue_primary.y=
                        image->chromaticity.blue_primary.x;
                    break;
                  }
                if (LocaleCompare(keyword,"border-color") == 0)
                  {
                    (void) QueryColorDatabase(options,&image->border_color,
                      exception);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'c':
              case 'C':
              {
                if (LocaleCompare(keyword,"class") == 0)
                  {
                    image->storage_class=(ClassType)
                      ParseMagickOption(MagickClassOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"colors") == 0)
                  {
                    image->colors=(unsigned long) atol(options);
                    break;
                  }
                if (LocaleCompare(keyword,"colorspace") == 0)
                  {
                    image->colorspace=(ColorspaceType) ParseMagickOption(
                      MagickColorspaceOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"compression") == 0)
                  {
                    image->compression=(CompressionType) ParseMagickOption(
                      MagickCompressionOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"columns") == 0)
                  {
                    image->columns=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'd':
              case 'D':
              {
                if (LocaleCompare(keyword,"delay") == 0)
                  {
                    image->delay=(unsigned long) atol(options);
                    break;
                  }
                if (LocaleCompare(keyword,"depth") == 0)
                  {
                    image->depth=(unsigned long) atol(options);
                    break;
                  }
                if (LocaleCompare(keyword,"dispose") == 0)
                  {
                    image->dispose=(DisposeType) ParseMagickOption(
                      MagickDisposeOptions,MagickFalse,options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'e':
              case 'E':
              {
                if (LocaleCompare(keyword,"endian") == 0)
                  {
                    image->endian=(EndianType) ParseMagickOption(
                      MagickEndianOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"error") == 0)
                  {
                    image->error.mean_error_per_pixel=atof(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'g':
              case 'G':
              {
                if (LocaleCompare(keyword,"gamma") == 0)
                  {
                    image->gamma=atof(options);
                    break;
                  }
                if (LocaleCompare(keyword,"green-primary") == 0)
                  {
                    flags=ParseGeometry(options,&geometry_info);
                    image->chromaticity.green_primary.x=geometry_info.rho;
                    image->chromaticity.green_primary.y=geometry_info.sigma;
                    if ((flags & SigmaValue) == 0)
                      image->chromaticity.green_primary.y=
                        image->chromaticity.green_primary.x;
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'i':
              case 'I':
              {
                if (LocaleCompare(keyword,"id") == 0)
                  {
                    (void) CopyMagickString(id,options,MaxTextExtent);
                    break;
                  }
                if (LocaleCompare(keyword,"iterations") == 0)
                  {
                    image->iterations=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'm':
              case 'M':
              {
                if (LocaleCompare(keyword,"matte") == 0)
                  {
                    image->matte=(MagickBooleanType) ParseMagickOption(
                      MagickBooleanOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"matte-color") == 0)
                  {
                    (void) QueryColorDatabase(options,&image->matte_color,
                      exception);
                    break;
                  }
                if (LocaleCompare(keyword,"maximum-error") == 0)
                  {
                    image->error.normalized_maximum_error=atof(options);
                    break;
                  }
                if (LocaleCompare(keyword,"mean-error") == 0)
                  {
                    image->error.normalized_mean_error=atof(options);
                    break;
                  }
                if (LocaleCompare(keyword,"montage") == 0)
                  {
                    (void) CloneString(&image->montage,options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'p':
              case 'P':
              {
                if (LocaleCompare(keyword,"page") == 0)
                  {
                    char
                      *geometry;

                    geometry=GetPageGeometry(options);
                    (void) ParseAbsoluteGeometry(geometry,&image->page);
                    geometry=(char *) RelinquishMagickMemory(geometry);
                    break;
                  }
                if (LocaleNCompare(keyword,"profile-",8) == 0)
                  {
                    if (profiles == (LinkedListInfo *) NULL)
                      profiles=NewLinkedList(0);
                    (void) AppendValueToLinkedList(profiles,
                      AcquireString(keyword+8));
                    profile=AcquireStringInfo((size_t) atol(options));
                    (void) SetImageProfile(image,keyword+8,profile);
                    profile=DestroyStringInfo(profile);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'o':
              case 'O':
              {
                if (LocaleCompare(keyword,"opaque") == 0)
                  {
                    image->matte=(MagickBooleanType) ParseMagickOption(
                      MagickBooleanOptions,MagickFalse,options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'q':
              case 'Q':
              {
                if (LocaleCompare(keyword,"quality") == 0)
                  {
                    image->quality=(unsigned long) atol(options);
                    break;
                  }
                if (LocaleCompare(keyword,"quantum-depth") == 0)
                  {
                    quantum_depth=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'r':
              case 'R':
              {
                if (LocaleCompare(keyword,"red-primary") == 0)
                  {
                    flags=ParseGeometry(options,&geometry_info);
                    image->chromaticity.red_primary.x=geometry_info.rho;
                    if ((flags & SigmaValue) != 0)
                      image->chromaticity.red_primary.y=geometry_info.sigma;
                    break;
                  }
                if (LocaleCompare(keyword,"rendering-intent") == 0)
                  {
                    image->rendering_intent=(RenderingIntent) ParseMagickOption(
                      MagickIntentOptions,MagickFalse,options);
                    break;
                  }
                if (LocaleCompare(keyword,"resolution") == 0)
                  {
                    flags=ParseGeometry(options,&geometry_info);
                    image->x_resolution=geometry_info.rho;
                    image->y_resolution=geometry_info.sigma;
                    if ((flags & SigmaValue) == 0)
                      image->y_resolution=image->x_resolution;
                    break;
                  }
                if (LocaleCompare(keyword,"rows") == 0)
                  {
                    image->rows=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 's':
              case 'S':
              {
                if (LocaleCompare(keyword,"scene") == 0)
                  {
                    image->scene=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 't':
              case 'T':
              {
                if (LocaleCompare(keyword,"ticks-per-second") == 0)
                  {
                    image->ticks_per_second=(unsigned long) atol(options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'u':
              case 'U':
              {
                if (LocaleCompare(keyword,"units") == 0)
                  {
                    image->units=(ResolutionType) ParseMagickOption(
                      MagickResolutionOptions,MagickFalse,options);
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              case 'w':
              case 'W':
              {
                if (LocaleCompare(keyword,"white-point") == 0)
                  {
                    flags=ParseGeometry(options,&geometry_info);
                    image->chromaticity.white_point.x=geometry_info.rho;
                    image->chromaticity.white_point.y=geometry_info.sigma;
                    if ((flags & SigmaValue) == 0)
                      image->chromaticity.white_point.y=
                        image->chromaticity.white_point.x;
                    break;
                  }
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
              default:
              {
                (void) SetImageAttribute(image,keyword,options);
                break;
              }
            }
          }
        else
          c=ReadBlobByte(image);
      while (isspace((int) ((unsigned char) c)) != 0)
        c=ReadBlobByte(image);
    }
    options=(char *) RelinquishMagickMemory(options);
    (void) ReadBlobByte(image);
    /*
      Verify that required image information is defined.
    */
    if ((LocaleCompare(id,"MagickCache") != 0) ||
        (image->storage_class == UndefinedClass) ||
        (image->compression == UndefinedCompression) || (image->columns == 0) ||
        (image->rows == 0))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    if (quantum_depth != QuantumDepth)
      ThrowReaderException(CacheError,"InconsistentPersistentCacheDepth");
    if (image->montage != (char *) NULL)
      {
        register char
          *p;

        /*
          Image directory.
        */
        length=MaxTextExtent;
        image->directory=AcquireString((char *) NULL);
        p=image->directory;
        do
        {
          *p='\0';
          if ((strlen(image->directory)+MaxTextExtent) >= length)
            {
              /*
                Allocate more memory for the image directory.
              */
              length<<=1;
              image->directory=(char *) ResizeMagickMemory(image->directory,
                (length+MaxTextExtent)*sizeof(*image->directory));
              if (image->directory == (char *) NULL)
                ThrowReaderException(CorruptImageError,"UnableToReadImageData");
              p=image->directory+strlen(image->directory);
            }
          c=ReadBlobByte(image);
          *p++=(char) c;
        } while (c != (int) '\0');
      }
    if (profiles != (LinkedListInfo *) NULL)
      {
        const char
          *name;

        const StringInfo
          *profile;

        /*
          Read image profiles.
        */
        ResetLinkedListIterator(profiles);
        name=(const char *) GetNextValueInLinkedList(profiles);
        while (name != (const char *) NULL)
        {
          profile=GetImageProfile(image,name);
          if (profile != (StringInfo *) NULL)
            count=ReadBlob(image,profile->length,profile->datum);
          name=(const char *) GetNextValueInLinkedList(profiles);
        }
        profiles=DestroyLinkedList(profiles,RelinquishMagickMemory);
      }
    depth=GetImageQuantumDepth(image,MagickFalse);
    if (image->storage_class == PseudoClass)
      {
        /*
          Create image colormap.
        */
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        if (image->colors != 0)
          {
            size_t
              packet_size;

            unsigned char
              *colormap;

            /*
              Read image colormap from file.
            */
            packet_size=(size_t) (3*depth/8);
            colormap=(unsigned char *)
              AcquireMagickMemory(packet_size*image->colors);
            if (colormap == (unsigned char *) NULL)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
            count=ReadBlob(image,packet_size*image->colors,colormap);
            if (count != (ssize_t) (packet_size*image->colors))
              ThrowReaderException(CorruptImageError,
                "InsufficientImageDataInFile");
            p=colormap;
            switch (depth)
            {
              default:
                ThrowReaderException(CorruptImageError,
                  "ImageDepthNotSupported");
              case 8:
              {
                unsigned char
                  pixel;

                for (i=0; i < (long) image->colors; i++)
                {
                  PushCharPixel(pixel,p);
                  image->colormap[i].red=ScaleCharToQuantum(pixel);
                  PushCharPixel(pixel,p);
                  image->colormap[i].green=ScaleCharToQuantum(pixel);
                  PushCharPixel(pixel,p);
                  image->colormap[i].blue=ScaleCharToQuantum(pixel);
                }
                break;
              }
              case 16:
              {
                unsigned short
                  pixel;

                for (i=0; i < (long) image->colors; i++)
                {
                  PushShortPixel(pixel,p);
                  image->colormap[i].red=ScaleShortToQuantum(pixel);
                  PushShortPixel(pixel,p);
                  image->colormap[i].green=ScaleShortToQuantum(pixel);
                  PushShortPixel(pixel,p);
                  image->colormap[i].blue=ScaleShortToQuantum(pixel);
                }
                break;
              }
              case 32:
              {
                unsigned long
                  pixel;

                for (i=0; i < (long) image->colors; i++)
                {
                  PushLongPixel(pixel,p);
                  image->colormap[i].red=ScaleLongToQuantum(pixel);
                  PushLongPixel(pixel,p);
                  image->colormap[i].green=ScaleLongToQuantum(pixel);
                  PushLongPixel(pixel,p);
                  image->colormap[i].blue=ScaleLongToQuantum(pixel);
                }
                break;
              }
            }
            colormap=(unsigned char *) RelinquishMagickMemory(colormap);
          }
      }
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Attach persistent pixel cache.
    */
    status=PersistCache(image,cache_filename,MagickTrue,&offset,exception);
    if (status == MagickFalse)
      ThrowReaderException(CacheError,"UnableToPersistPixelCache");
    /*
      Proceed to next image.
    */
    do
    {
      c=ReadBlobByte(image);
    } while ((isgraph(c) == MagickFalse) && (c != EOF));
    if (c != EOF)
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
  } while (c != EOF);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M P C I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMPCImage() adds attributes for the Cache image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMPCImage method is:
%
%      RegisterMPCImage(void)
%
*/
ModuleExport void RegisterMPCImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("CACHE");
  entry->description=ConstantString("Magick Persistent Cache image format");
  entry->module=ConstantString("CACHE");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MPC");
  entry->decoder=(DecoderHandler *) ReadMPCImage;
  entry->encoder=(EncoderHandler *) WriteMPCImage;
  entry->magick=(MagickHandler *) IsMPC;
  entry->description=ConstantString("Magick Persistent Cache image format");
  entry->module=ConstantString("MPC");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M P C I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMPCImage() removes format registrations made by the
%  MPC module from the list of supported formats.
%
%  The format of the UnregisterMPCImage method is:
%
%      UnregisterMPCImage(void)
%
*/
ModuleExport void UnregisterMPCImage(void)
{
  (void) UnregisterMagickInfo("CACHE");
  (void) UnregisterMagickInfo("MPC");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M P C I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteMPCImage() writes an Magick Persistent Cache image to a file.
%
%  The format of the WriteMPCImage method is:
%
%      MagickBooleanType WriteMPCImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The image.
%
%
*/
static MagickBooleanType WriteMPCImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent],
    cache_filename[MaxTextExtent];

  const ImageAttribute
    *attribute;

  MagickBooleanType
    status;

  MagickOffsetType
    offset,
    scene;

  register long
    i;

  unsigned long
    depth;

  /*
    Open persistent cache.
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
  (void) CopyMagickString(cache_filename,image->filename,MaxTextExtent);
  AppendImageFormat("cache",cache_filename);
  scene=0;
  offset=0;
  do
  {
    /*
      Write persistent cache meta-information.
    */
    depth=GetImageQuantumDepth(image,MagickTrue);
    if ((image->storage_class == PseudoClass) &&
        (image->colors > (1UL << depth)))
      image->storage_class=DirectClass;
    (void) WriteBlobString(image,"id=MagickCache\n");
    (void) FormatMagickString(buffer,MaxTextExtent,"quantum-depth=%d\n",
      QuantumDepth);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "class=%s  colors=%lu  matte=%s\n",MagickOptionToMnemonic(
      MagickClassOptions,image->storage_class),image->colors,
      MagickOptionToMnemonic(MagickBooleanOptions,(long) image->matte));
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "columns=%lu  rows=%lu  depth=%lu\n",image->columns,image->rows,
      image->depth);
    (void) WriteBlobString(image,buffer);
    if (image->colorspace != UndefinedColorspace)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"colorspace=%s\n",
          MagickOptionToMnemonic(MagickColorspaceOptions,image->colorspace));
        (void) WriteBlobString(image,buffer);
      }
    if (image->endian != UndefinedEndian)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"endian=%s\n",
          MagickOptionToMnemonic(MagickEndianOptions,image->endian));
        (void) WriteBlobString(image,buffer);
      }
    if (image->compression != UndefinedCompression)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,
          "compression=%s  quality=%lu\n",
          MagickOptionToMnemonic(MagickCompressionOptions,image->compression),
          image->quality);
        (void) WriteBlobString(image,buffer);
      }
    if (image->units != UndefinedResolution)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"units=%s\n",
          MagickOptionToMnemonic(MagickResolutionOptions,image->units));
        (void) WriteBlobString(image,buffer);
      }
    if ((image->x_resolution != 0) || (image->y_resolution != 0))
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"resolution=%gx%g\n",
          image->x_resolution,image->y_resolution);
        (void) WriteBlobString(image,buffer);
      }
    if ((image->page.width != 0) || (image->page.height != 0))
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"page=%lux%lu%+ld%+ld\n",
          image->page.width,image->page.height,image->page.x,image->page.y);
        (void) WriteBlobString(image,buffer);
      }
    else
      if ((image->page.x != 0) || (image->page.y != 0))
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"page=%+ld%+ld\n",
            image->page.x,image->page.y);
          (void) WriteBlobString(image,buffer);
        }
    if ((GetNextImageInList(image) != (Image *) NULL) ||
        (GetPreviousImageInList(image) != (Image *) NULL))
      {
        if (image->scene == 0)
          (void) FormatMagickString(buffer,MaxTextExtent,
            "iterations=%lu  delay=%lu  ticks-per-second=%lu\n",
            image->iterations,image->delay,image->ticks_per_second);
        else
          (void) FormatMagickString(buffer,MaxTextExtent,
            "scene=%lu  iterations=%lu  delay=%lu  ticks-per-second=%lu\n",
            image->scene,image->iterations,image->delay,
            image->ticks_per_second);
        (void) WriteBlobString(image,buffer);
      }
    else
      {
        if (image->scene != 0)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"scene=%lu\n",
              image->scene);
            (void) WriteBlobString(image,buffer);
          }
        if (image->iterations != 0)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"iterations=%lu\n",
              image->iterations);
            (void) WriteBlobString(image,buffer);
          }
        if (image->delay != 0)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"delay=%lu\n",
              image->delay);
            (void) WriteBlobString(image,buffer);
          }
        if (image->ticks_per_second != UndefinedTicksPerSecond)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,
              "ticks-per-second=%lu\n",image->ticks_per_second);
            (void) WriteBlobString(image,buffer);
          }
      }
    if (image->dispose != UndefinedDispose)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"dispose=%s\n",
          MagickOptionToMnemonic(MagickDisposeOptions,image->dispose));
        (void) WriteBlobString(image,buffer);
      }
    if (image->rendering_intent != UndefinedIntent)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,
          "rendering-intent=%s\n",
           MagickOptionToMnemonic(MagickIntentOptions,image->rendering_intent));
        (void) WriteBlobString(image,buffer);
      }
    if (image->gamma != 0.0)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"gamma=%g\n",
          image->gamma);
        (void) WriteBlobString(image,buffer);
      }
    if (image->chromaticity.white_point.x != 0.0)
      {
        /*
          Note chomaticity points.
        */
        (void) FormatMagickString(buffer,MaxTextExtent,
          "red-primary=%g,%g  green-primary=%g,%g  blue-primary=%g,%g\n",
          image->chromaticity.red_primary.x,image->chromaticity.red_primary.y,
          image->chromaticity.green_primary.x,
          image->chromaticity.green_primary.y,
          image->chromaticity.blue_primary.x,
          image->chromaticity.blue_primary.y);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"white-point=%g,%g\n",
          image->chromaticity.white_point.x,image->chromaticity.white_point.y);
        (void) WriteBlobString(image,buffer);
      }
    if (image->profiles != (void *) NULL)
      {
        const char
          *name;

        const StringInfo
          *profile;

        /*
          Generic profile.
        */
        ResetImageProfileIterator(image);
        name=GetNextImageProfile(image);
        while (name != (const char *) NULL)
        {
          profile=GetImageProfile(image,name);
          if (profile != (StringInfo *) NULL)
            {
              (void) FormatMagickString(buffer,MaxTextExtent,
                "profile-%s=%lu\n",name,(unsigned long) profile->length);
              (void) WriteBlobString(image,buffer);
            }
          name=GetNextImageProfile(image);
        }
      }
    if (image->montage != (char *) NULL)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"montage=%s\n",
          image->montage);
        (void) WriteBlobString(image,buffer);
      }
    ResetImageAttributeIterator(image);
	  attribute=GetNextImageAttribute(image);
		while (attribute != (const ImageAttribute *) NULL)
    {
      if (*attribute->key != '[')
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s=",attribute->key);
          (void) WriteBlobString(image,buffer);
          for (i=0; i < (long) strlen(attribute->value); i++)
            if (isspace((int) ((unsigned char) attribute->value[i])) != 0)
              break;
          if (i <= (long) strlen(attribute->value))
            (void) WriteBlobByte(image,'{');
          (void) WriteBlob(image,strlen(attribute->value),(unsigned char *)
            attribute->value);
          if (i <= (long) strlen(attribute->value))
            (void) WriteBlobByte(image,'}');
          (void) WriteBlobByte(image,'\n');
        }
  	  attribute=GetNextImageAttribute(image);
    }
    (void) WriteBlobString(image,"\f\n:\032");
    if (image->montage != (char *) NULL)
      {
        /*
          Write montage tile directory.
        */
        if (image->directory != (char *) NULL)
          (void) WriteBlobString(image,image->directory);
        (void) WriteBlobByte(image,'\0');
      }
    if (image->profiles != 0)
      {
        const char
          *name;

        const StringInfo
          *profile;

        /*
          Write image profiles.
        */
        ResetImageProfileIterator(image);
        name=GetNextImageProfile(image);
        while (name != (const char *) NULL)
        {
          profile=GetImageProfile(image,name);
          (void) WriteBlob(image,profile->length,profile->datum);
          name=GetNextImageProfile(image);
        }
      }
    if (image->storage_class == PseudoClass)
      {
        register unsigned char
          *q;

        size_t
          packet_size;

        unsigned char
          *colormap;

        unsigned long
          pixel;

        /*
          Allocate colormap.
        */
        packet_size=(size_t) (3*depth/8);
        colormap=(unsigned char *)
          AcquireMagickMemory(packet_size*image->colors*sizeof(*colormap));
        if (colormap == (unsigned char *) NULL)
          return(MagickFalse);
        /*
          Write colormap to file.
        */
        q=colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          switch (depth)
          {
            default:
              ThrowWriterException(CorruptImageError,"ImageDepthNotSupported");
            case 32:
            {
              pixel=ScaleQuantumToLong(image->colormap[i].red);
              PopLongPixel(q,pixel);
              pixel=ScaleQuantumToLong(image->colormap[i].green);
              PopLongPixel(q,pixel);
              pixel=ScaleQuantumToLong(image->colormap[i].blue);
              PopLongPixel(q,pixel);
            }
            case 16:
            {
              pixel=ScaleQuantumToShort(image->colormap[i].red);
              PopShortPixel(q,pixel);
              pixel=ScaleQuantumToShort(image->colormap[i].green);
              PopShortPixel(q,pixel);
              pixel=ScaleQuantumToShort(image->colormap[i].blue);
              PopShortPixel(q,pixel);
              break;
            }
            case 8:
            {
              pixel=(unsigned long) ScaleQuantumToChar(image->colormap[i].red);
              PopCharPixel(q,pixel);
              pixel=(unsigned long)
                ScaleQuantumToChar(image->colormap[i].green);
              PopCharPixel(q,pixel);
              pixel=(unsigned long) ScaleQuantumToChar(image->colormap[i].blue);
              PopCharPixel(q,pixel);
              break;
            }
          }
        }
        (void) WriteBlob(image,packet_size*image->colors,colormap);
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
      }
    /*
      Initialize persistent pixel cache.
    */
    status=PersistCache(image,cache_filename,MagickFalse,&offset,
      &image->exception);
    if (status == MagickFalse)
      ThrowWriterException(CacheError,"UnableToPersistPixelCache");
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
  return(status);
}
