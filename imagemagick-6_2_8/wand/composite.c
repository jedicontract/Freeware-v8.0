/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO  M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE          %
%       C      O   O MM MM  P   P  O   O  SS       I      T    E              %
%       C      O   O M M M  PPPP   O   O   SSS     I      T    EEE            %
%       C      O   O M   M  P      O   O     SS    I      T    E              %
%        CCCC   OOO  M   M  P       OOO   SSSSS  IIIII    T    EEEEE          %
%                                                                             %
%                                                                             %
%                    ImageMagick Image Composite Methods                      %
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
%
*/

/*
  Include declarations.
*/
#include "wand/studio.h"
#include "wand/MagickWand.h"
#include "wand/mogrify-private.h"

/*
  Typedef declarations.
*/
typedef struct _CompositeOptions
{
  ChannelType
    channel;

  char
    *blend_geometry,
    *displace_geometry,
    *dissolve_geometry,
    *geometry,
    *unsharp_geometry,
    *watermark_geometry;

  CompositeOperator
    compose;

  GravityType
    gravity;

  long
    stegano;

  MagickBooleanType
    stereo,
    tile;
} CompositeOptions;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  C o m p o s i t e I m a g e C o m m a n d                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImageCommand() reads one or more images and an optional mask and
%  composites them into a new image.
%
%  The format of the CompositeImageCommand method is:
%
%      MagickBooleanType CompositeImageCommand(ImageInfo *image_info,int argc,
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

static MagickBooleanType CompositeImageList(ImageInfo *image_info,Image **image,
  Image *composite_image,CompositeOptions *option_info,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  assert(exception != (ExceptionInfo *) NULL);
  status=MagickTrue;
  if (composite_image != (Image *) NULL)
    {
      assert(composite_image->signature == MagickSignature);
      if (option_info->compose == BlendCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->blend_geometry);
      if (option_info->compose == DisplaceCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->displace_geometry);
      if (option_info->compose == DissolveCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->dissolve_geometry);
      if (option_info->compose == ModulateCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->watermark_geometry);
      if (option_info->compose == ThresholdCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->unsharp_geometry);
      /*
        Composite image.
      */
      if (option_info->stegano != 0)
        {
          Image
            *stegano_image;

          (*image)->offset=option_info->stegano-1;
          stegano_image=SteganoImage(*image,composite_image,exception);
          if (stegano_image != (Image *) NULL)
            {
              *image=DestroyImageList(*image);
              *image=stegano_image;
            }
        }
      else
        if (option_info->stereo != MagickFalse)
          {
            Image
              *stereo_image;

            stereo_image=StereoImage(*image,composite_image,exception);
            if (stereo_image != (Image *) NULL)
              {
                *image=DestroyImageList(*image);
                *image=stereo_image;
              }
          }
        else
          if (option_info->tile != MagickFalse)
            {
              long
                x,
                y;

              unsigned long
                columns;

              /*
                Tile the composite image.
              */
              (void) SetImageAttribute(composite_image,
                "[modify-outside-overlay]","false");
              columns=composite_image->columns;
              for (y=0; y < (long) (*image)->rows; y+=composite_image->rows)
                for (x=0; x < (long) (*image)->columns; x+=columns)
                  status&=CompositeImageChannel(*image,option_info->channel,
                    option_info->compose,composite_image,x,y);
              GetImageException(*image,exception);
            }
          else
            {
              char
                composite_geometry[MaxTextExtent];

              RectangleInfo
                geometry;

              /*
                Digitally composite image.
              */
              SetGeometry(*image,&geometry);
              (void) ParseAbsoluteGeometry(option_info->geometry,&geometry);
              (void) FormatMagickString(composite_geometry,MaxTextExtent,
                "%lux%lu%+ld%+ld",composite_image->columns,
                composite_image->rows,geometry.x,geometry.y);
              (*image)->gravity=(GravityType) option_info->gravity;
              (void) ParseGravityGeometry(*image,composite_geometry,&geometry);
              status&=CompositeImageChannel(*image,option_info->channel,
                option_info->compose,composite_image,geometry.x,geometry.y);
              GetImageException(*image,exception);
            }
    }
  return(status != 0 ? MagickTrue : MagickFalse);
}

static void CompositeUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-affine matrix       affine transform matrix",
      "-authenticate value  decrypt image with this password",
      "-blend geometry      blend images",
      "-blue-primary point  chromaticity blue primary point",
      "-channel type        apply option to select image channels",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compose operator    composite operator",
      "-compress type       type of pixel compression when writing the image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-displace geometry   shift image pixels defined by a displacement map",
      "-display server      get image or font from this X server",
      "-dispose method      GIF disposal method",
      "-dissolve value      dissolve the two images a given percent",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-encoding type       text encoding type",
      "-endian type         endianness (MSB or LSB) of the image",
      "-extract geometry    extract area from image",
      "-filter type         use this filter when resizing an image",
      "-font name           render text with this font",
      "-format \"string\"     output formatted image characteristics",
      "-geometry geometry   location of the composite image",
      "-gravity type        which direction to gravitate towards",
      "-green-primary point chromaticity green primary point",
      "-help                print program options",
      "-identify            identify the format and characteristics of the image",
      "-interlace type      type of image interlacing scheme",
      "-label name          assign a label to an image",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-page geometry       size and location of an image canvas (setting)",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-red-primary point   chromaticity red primary point",
      "-rotate degrees      apply Paeth rotation to the image",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resize geometry     resize the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scene value         image scene number",
      "-sharpen geometry    sharpen the image",
      "-size geometry       width and height of image",
      "-stegano offset      hide watermark within an image",
      "-stereo              combine two image to create a stereo anaglyph",
      "-strip               strip image of all profiles and comments",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-thumbnail geometry  create a thumbnail of the image",
      "-tile                repeat composite operation across and down image",
      "-transform           affine transform image",
      "-treedepth value     color tree depth",
      "-type type           image type",
      "-units type          the units of image resolution",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-watermark geometry  percent brightness and saturation of a watermark",
      "-white-point point   chromaticity white point",
      "-write filename      write images to this file",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] image [options ...] composite\n"
    "  [ [options ...] mask ] [options ...] composite\n",
    GetClientName());
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  exit(0);
}

static void RelinquishCompositeOptions(CompositeOptions *option_info)
{
  if (option_info->blend_geometry != (char *) NULL)
    option_info->blend_geometry=(char *)
      RelinquishMagickMemory(option_info->blend_geometry);
  if (option_info->displace_geometry != (char *) NULL)
    option_info->displace_geometry=(char *)
      RelinquishMagickMemory(option_info->displace_geometry);
  if (option_info->dissolve_geometry != (char *) NULL)
    option_info->dissolve_geometry=(char *)
      RelinquishMagickMemory(option_info->dissolve_geometry);
  if (option_info->geometry != (char *) NULL)
    option_info->geometry=(char *)
      RelinquishMagickMemory(option_info->geometry);
  if (option_info->unsharp_geometry != (char *) NULL)
    option_info->unsharp_geometry=(char *)
      RelinquishMagickMemory(option_info->unsharp_geometry);
  if (option_info->watermark_geometry != (char *) NULL)
    option_info->watermark_geometry=(char *)
      RelinquishMagickMemory(option_info->watermark_geometry);
}

WandExport MagickBooleanType CompositeImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
#define NotInitialized  (unsigned int) (~0)
#define DestroyComposite() \
{ \
  RelinquishCompositeOptions(&option_info); \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowCompositeException(asperity,tag,option) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag,"`%s'", \
    option); \
  DestroyComposite(); \
  return(MagickFalse); \
}
#define ThrowCompositeInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyComposite(); \
  return(MagickFalse); \
}

  char
    *filename,
    *option;

  CompositeOptions
    option_info;

  const char
    *format;

  Image
    *composite_image,
    *image,
    *image_stack[MaxImageStackDepth+1],
    *mask_image;

  MagickBooleanType
    fire,
    pend;

  MagickStatusType
    status;

  long
    j,
    k;

  register long
    i;

  /*
    Set default.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc < 4)
    CompositeUsage();
  (void) ResetMagickMemory(&option_info,0,sizeof(CompositeOptions));
  option_info.compose=OverCompositeOp;
  filename=(char *) NULL;
  format="%w,%h,%m";
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  option=(char *) NULL;
  pend=MagickFalse;
  status=MagickTrue;
  /*
    Check command syntax.
  */
  composite_image=NewImageList();
  image=NewImageList();
  mask_image=NewImageList();
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  for (i=1; i < (long) (argc-1); i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowCompositeException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowCompositeException(OptionError,"UnableToParseExpression",option);
        if (image_stack[k] != (Image *) NULL)
          {
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k-1],image_stack[k]);
          }
        k--;
        continue;
      }
    if (IsMagickOption(option) == MagickFalse)
      {
        Image
          *image;

        /*
          Read input image.
        */
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        filename=argv[i];
        if ((LocaleCompare(filename,"--") == 0) && (i < (argc-1)))
          filename=argv[++i];
        (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
        image=ReadImage(image_info,exception);
        status&=(image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (image == (Image *) NULL)
          continue;
        AppendImageToList(&image_stack[k],image);
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("blend",option+1) == 0)
          {
            (void) CloneString(&option_info.blend_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.blend_geometry,argv[i]);
            option_info.compose=BlendCompositeOp;
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            long
              channel;

            if (*option == '+')
              {
                option_info.channel=DefaultChannels;
                break;
              }
            i++;
            if (i == (long) (argc-1))
              ThrowCompositeException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowCompositeException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            option_info.channel=(ChannelType) channel;
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowCompositeException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            long
              compose;

            option_info.compose=UndefinedCompositeOp;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            compose=ParseMagickOption(MagickCompositeOptions,MagickFalse,
              argv[i]);
            if (compose < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedComposeOperator",argv[i]);
            option_info.compose=(CompositeOperator) compose;
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            long
              compression;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedImageCompression",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            LogEventType
              event_mask;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowCompositeException(OptionError,"UnrecognizedEventType",
                argv[i]);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowCompositeException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("displace",option+1) == 0)
          {
            (void) CloneString(&option_info.displace_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.displace_geometry,argv[i]);
            option_info.compose=DisplaceCompositeOp;
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            long
              dispose;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowCompositeException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dissolve",option+1) == 0)
          {
            (void) CloneString(&option_info.dissolve_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.dissolve_geometry,argv[i]);
            option_info.compose=DissolveCompositeOp;
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            long
              endian;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,
              argv[i]);
            if (endian < 0)
              ThrowCompositeException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("filter",option+1) == 0)
          {
            long
              filter;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            filter=ParseMagickOption(MagickFilterOptions,MagickFalse,
              argv[i]);
            if (filter < 0)
              ThrowCompositeException(OptionError,"UnrecognizedImageFilter",
                argv[i]);
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            format=argv[i];
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("geometry",option+1) == 0)
          {
            (void) CloneString(&option_info.geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.geometry,argv[i]);
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            long
              gravity;

            option_info.gravity=UndefinedGravity;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            gravity=ParseMagickOption(MagickGravityOptions,MagickFalse,
              argv[i]);
            if (gravity < 0)
              ThrowCompositeException(OptionError,"UnrecognizedGravityType",
                argv[i]);
            option_info.gravity=(GravityType) gravity;
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if ((LocaleCompare("help",option+1) == 0) ||
            (LocaleCompare("-help",option+1) == 0))
          CompositeUsage();
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("identify",option+1) == 0)
          break;
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedInterlaceType",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            long
              resource;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowCompositeException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (strchr(argv[i],'%') == (char *) NULL))
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("process",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("stegano",option+1) == 0)
          {
            option_info.stegano=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            option_info.stegano=atol(argv[i])+1;
            break;
          }
        if (LocaleCompare("stereo",option+1) == 0)
          {
            option_info.stereo=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            option_info.tile=(*option == '-') ? MagickTrue : MagickFalse;
            (void) CopyMagickString(argv[i]+1,"sans0",MaxTextExtent);
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            long
              type;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowCompositeException(OptionError,"UnrecognizedImageType",
                argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("units",option+1) == 0)
          {
            long
              units;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            units=ParseMagickOption(MagickResolutionOptions,MagickFalse,
              argv[i]);
            if (units < 0)
              ThrowCompositeException(OptionError,"UnrecognizedUnitsType",
                argv[i]);
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            (void) CloneString(&option_info.unsharp_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.unsharp_geometry,argv[i]);
            option_info.compose=ThresholdCompositeOp;
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if ((LocaleCompare("version",option+1) == 0) ||
            (LocaleCompare("-version",option+1) == 0))
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("watermark",option+1) == 0)
          {
            (void) CloneString(&option_info.watermark_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.watermark_geometry,argv[i]);
            option_info.compose=ModulateCompositeOp;
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
    }
    fire=(MagickBooleanType) ParseMagickOption(MagickMogrifyOptions,
      MagickFalse,option+1);
    if (fire == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowCompositeException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i-- != (long) (argc-1))
    ThrowCompositeException(OptionError,"MissingAnImageFilename",argv[i]);
  if ((image_stack[k] == (Image *) NULL) ||
      (GetImageListLength(image_stack[k]) < 2))
    ThrowCompositeException(OptionError,"MissingAnImageFilename",argv[argc-1]);
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  composite_image=GetImageFromList(image_stack[k],0);
  image=GetImageFromList(image_stack[k],1);
  mask_image=GetImageFromList(image_stack[k],2);
  if (mask_image != (Image *) NULL)
    {
      (void) SetImageType(composite_image,TrueColorMatteType);
      if (composite_image->matte == MagickFalse)
        SetImageOpacity(composite_image,OpaqueOpacity);
      status&=CompositeImage(composite_image,CopyOpacityCompositeOp,mask_image,
        0,0);
      if (status == MagickFalse)
        GetImageException(composite_image,exception);
    }
  composite_image=CloneImage(composite_image,0,0,MagickTrue,exception);
  if (composite_image == (Image *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  (void) TransformImage(&composite_image,(char *) NULL,
    composite_image->geometry);
  image=CloneImage(image,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  status&=CompositeImageList(image_info,&image,composite_image,&option_info,
    exception);
  composite_image=DestroyImage(composite_image);
  /*
    Write composite images.
  */
  status&=WriteImages(image_info,image,argv[argc-1],exception);
  if (metadata != (char **) NULL)
    {
      char
        *text;

      text=InterpretImageAttributes(image_info,image,format);
      if (text == (char *) NULL)
        {
          char
            *message;

          message=GetExceptionMessage(errno);
          ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
            message);
          message=(char *) RelinquishMagickMemory(message);
        }
      (void) ConcatenateString(&(*metadata),text);
      (void) ConcatenateString(&(*metadata),"\n");
      text=(char *) RelinquishMagickMemory(text);
    }
  image=DestroyImage(image);
  RelinquishCompositeOptions(&option_info);
  DestroyComposite();
  return(status != 0 ? MagickTrue : MagickFalse);
}
