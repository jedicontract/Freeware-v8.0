/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                CCCC   OOO   N   N  JJJJJ  U   U  RRRR   EEEEE               %
%               C      O   O  NN  N    J    U   U  R   R  E                   %
%               C      O   O  N N N    J    U   U  RRRR   EEE                 %
%               C      O   O  N  NN  J J    U   U  R R    E                   %
%                CCCC   OOO   N   N  JJJ     UUU   R  R   EEEEE               %
%                                                                             %
%                                                                             %
%                     Interpret Magick Scripting Language.                    %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               December 2001                                 %
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
%  Conjure interprets and executes scripts in the Magick Scripting Language
%  (MSL). The Magick scripting language (MSL) will primarily benefit those
%  that want to accomplish custom image processing tasks but do not wish
%  to program, or those that do not have access to a Perl interpreter or a
%  compiler. The interpreter is called conjure and here is an example script:
%
%      <?xml version="1.0" encoding="UTF-8"?>
%      <image size="400x400" >
%      <read filename="image.gif" />
%      <get width="base-width" height="base-height" />
%      <resize geometry="%[dimensions]" />
%      <get width="width" height="height" />
%      <print output="Image sized from %[base-width]x%[base-height]
%          to %[width]x%[height].\n" />
%      <write filename="image.png" />
%      </image>
%
%
*/

/*
  Include declarations.
*/
#include "wand/studio.h"
#include "wand/MagickWand.h"
#include "wand/mogrify-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C o n j u r e I m a g e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConjureImageCommand() describes the format and characteristics of one or
%  more image files. It will also report if an image is incomplete or corrupt.
%  The information displayed includes the scene number, the file name, the
%  width and height of the image, whether the image is colormapped or not,
%  the number of colors in the image, the number of bytes in the image, the
%  format of the image (JPEG, PNM, etc.), and finally the number of seconds
%  it took to read and process the image.
%
%  The format of the ConjureImageCommand method is:
%
%      MagickBooleanType ConjureImageCommand(ImageInfo *image_info,int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void ConjureUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-debug events        display copious debugging information",
      "-help                print program options",
      "-log format          format of debugging information",
      "-monitor             monitor progress",
      "-quiet               suppress all error or warning messages",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] file [ [options ...] file ...]\n",
    GetClientName());
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf("\nIn additiion, define any key value pairs required by "
    "your script.  For\nexample,\n\n");
  (void) printf("    conjure -size 100x100 -color blue -foo bar script.msl\n");
  exit(0);
}

WandExport MagickBooleanType ConjureImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **wand_unused(metadata),ExceptionInfo *exception)
{
#define DestroyConjure() \
{ \
  image=DestroyImageList(image); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowConjureException(asperity,tag,option) \
{ \
  if (exception->severity < (asperity)) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyConjure(); \
  return(MagickFalse); \
}
#define ThrowConjureInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyConjure(); \
  return(MagickFalse); \
}

  char
    *option;

  Image
    *image;

  long
    number_images;

  MagickStatusType
    status;

  register long
    i;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc < 2)
    ConjureUsage();
  image=NewImageList();
  number_images=0;
  option=(char *) NULL;
  /*
    Conjure an image.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowConjureException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  for (i=1; i < (long) argc; i++)
  {
    option=argv[i];
    if (IsMagickOption(option) != MagickFalse)
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == (long) argc)
                  ThrowConjureException(OptionError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            continue;
          }
        if ((LocaleCompare("help",option+1) == 0) ||
            (LocaleCompare("-help",option+1) == 0))
          {
            if (*option == '-')
              ConjureUsage();
            continue;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == (long) argc)
                  ThrowConjureException(OptionError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            continue;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          continue;
        if (LocaleCompare("quiet",option+1) == 0)
          continue;
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-') ? MagickTrue : MagickFalse;
            continue;
          }
        if ((LocaleCompare("version",option+1) == 0) ||
            (LocaleCompare("-version",option+1) == 0))
          {
            (void) fprintf(stdout,"Version: %s\n",
              GetMagickVersion((unsigned long *) NULL));
            (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCopyright());
            exit(0);
            continue;
          }
        /*
          Persist key/value pair.
        */
        (void) DeleteImageOption(image_info,option+1);
        status&=SetImageOption(image_info,option+1,argv[i+1]);
        if (status == MagickFalse)
          ThrowConjureException(ImageError,"UnableToPersistKey",option);
        i++;
        continue;
      }
    /*
      Interpret MSL script.
    */
    (void) DeleteImageOption(image_info,"filename");
    status&=SetImageOption(image_info,"filename",argv[i]);
    if (status == MagickFalse)
      ThrowConjureException(ImageError,"UnableToPersistKey",argv[i]);
    (void) FormatMagickString(image_info->filename,MaxTextExtent,"msl:%s",
      argv[i]);
    image=ReadImage(image_info,exception);
    CatchException(exception);
    status&=image != (Image *) NULL;
    if (image != (Image *) NULL)
      image=DestroyImageList(image);
    number_images++;
  }
  if (i != argc)
    ThrowConjureException(OptionError,"MissingAnImageFilename",argv[i]);
  if (number_images == 0)
    ThrowConjureException(OptionError,"MissingAnImageFilename",argv[argc-1]);
  if (image != (Image *) NULL)
    image=DestroyImageList(image);
  for (i=0; i < (long) argc; i++)
    argv[i]=(char *) RelinquishMagickMemory(argv[i]);
  argv=(char **) RelinquishMagickMemory(argv);
  return(status != 0 ? MagickTrue : MagickFalse);
}
