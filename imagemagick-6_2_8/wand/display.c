/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   IIIII  SSSSS  PPPP   L       AAA   Y   Y               %
%               D   D    I    SS     P   P  L      A   A   Y Y                %
%               D   D    I     SSS   PPPP   L      AAAAA    Y                 %
%               D   D    I       SS  P      L      A   A    Y                 %
%               DDDD   IIIII  SSSSS  P      LLLLL  A   A    Y                 %
%                                                                             %
%                                                                             %
%              Methods to Interactively Display and Edit an Image             %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                                July 1992                                    %
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
#include "wand/studio.h"
#include "wand/MagickWand.h"
#include "wand/mogrify-private.h"
#include "magick/display-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D i s p l a y I m a g e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DisplayImageCommand() displays a sequence of images on any workstation
%  display running an X server.  Display first determines the hardware
%  capabilities of the workstation. If the number of unique colors in an image
%  is less than or equal to the number the workstation can support, the image
%  is displayed in an X window. Otherwise the number of colors in the image is
%  first reduced to match the color resolution of the workstation before it is
%  displayed.
%
%  This means that a continuous-tone 24 bits/pixel image can display on a 8
%  bit pseudo-color device or monochrome device. In most instances the reduced
%  color image closely resembles the original. Alternatively, a monochrome or
%  pseudo-color image sequence can display on a continuous-tone 24 bits/pixels
%  device.
%
%  The format of the DisplayImageCommand method is:
%
%      MagickBooleanType DisplayImageCommand(ImageInfo *image_info,int argc,
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
*/

static void DisplayUsage(void)
{
  const char
    **p;

  static const char
    *buttons[]=
    {
      "1    press to map or unmap the Command widget",
      "2    press and drag to magnify a region of an image",
      "3    press to load an image from a visual image directory",
      (char *) NULL
    },
    *options[]=
    {
      "-antialias           remove pixel-aliasing",
      "-authenticate value  decrypt image with this password",
      "-auto-orient         automatically orient image",
      "-backdrop            display image centered on a backdrop",
      "-border geometry     surround image with a border of color",
      "-channel type        apply option to select image channels",
      "-clip                clip along the first path from the 8BIM profile",
      "-clip-path id        clip along a named path from the 8BIM profile",
      "-coalesce            merge a sequence of images",
      "-colormap type       Shared or Private",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       type of pixel compression when writing the image",
      "-contrast            enhance or reduce the image contrast",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-despeckle           reduce the speckles within an image",
      "-display server      display image to this X server",
      "-dispose method      GIF disposal method",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-edge factor         apply a filter to detect edges in the image",
      "-endian type         endianness (MSB or LSB) of the image",
      "-enhance             apply a digital filter to enhance a noisy image",
      "-extract geometry    extract area from image",
      "-filter type         use this filter when resizing an image",
      "-flatten             flatten a sequence of images",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-format \"string\"     output formatted image characteristics",
      "-frame geometry      surround image with an ornamental border",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred size and location of the Image window",
      "-help                print program options",
      "-identify            identify the format and characteristics of the image",
      "-immutable           displayed image cannot be modified",
      "-interlace type      type of image interlacing scheme",
      "-label name          assign a label to an image",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-loop iterations     loop images then exit",
      "-map type            display image using this Standard Colormap",
      "-matte               store matte channel if the image has one",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color",
      "-page geometry       size and location of an image canvas",
      "-profile filename    add, delete, or apply an image profile",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-remote command      execute a command in an remote display process",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resize geometry     resize the image",
      "-roll geometry       roll an image vertically or horizontally",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sample geometry     scale image with pixel sampling",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-segment value       segment an image",
      "-set attribute value set an image attribute",
      "-sharpen geometry    sharpen the image",
      "-size geometry       width and height of image",
      "-strip               strip image of all profiles and comments",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-texture filename    name of texture to tile onto the image background",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-update seconds      detect when image file is modified and redisplay",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-visual type         display image using this visual type",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-window id           display image to background of this window",
      "-window-group id     exit program when this window id is destroyed",
      "-write filename      write image to a file",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] file [ [options ...] file ...]\n",
    GetClientName());
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -foreground, -iconGeometry, -iconic, -mattecolor,\n");
  (void) printf("-name, -shared-memory, -usePixmap, or -title.\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
  (void) printf("\nButtons: \n");
  for (p=buttons; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  exit(0);
}

WandExport MagickBooleanType DisplayImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **wand_unused(metadata),ExceptionInfo *exception)
{
#if defined(HasX11)
#define DestroyDisplay() \
{ \
  if ((state & ExitState) == 0) \
    DestroyXResources(); \
  if (display != (Display *) NULL) \
    { \
      XCloseDisplay(display); \
      display=(Display *) NULL; \
    } \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  if (image_marker != (unsigned long *) NULL) \
    image_marker=(unsigned long *) RelinquishMagickMemory(image_marker); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowDisplayException(asperity,tag,option) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag,"`%s'", \
    option); \
  DestroyDisplay(); \
  return(MagickFalse); \
}
#define ThrowDisplayInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyDisplay(); \
  return(MagickFalse); \
}

  char
    *resource_value,
    *server_name;

  const char
    *option;

  Display
    *display;

  Image
    *image,
    *image_stack[MaxImageStackDepth+1];

  long
    first_scene,
    image_number,
    iteration,
    j,
    k,
    l,
    last_scene,
    scene;

  MagickStatusType
    pend,
    status;

  QuantizeInfo
    *quantize_info;

  register long
    i;

  unsigned long
    *image_marker,
    iterations,
    last_image,
    state;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  SetNotifyHandlers;
  display=(Display *) NULL;
  first_scene=0;
  j=1;
  k=0;
  image_marker=(unsigned long *) NULL;
  image_stack[k]=NewImageList();
  image_number=0;
  last_image=0;
  last_scene=0;
  option=(char *) NULL;
  pend=MagickFalse;
  resource_database=(XrmDatabase) NULL;
  (void) ResetMagickMemory(&resource_info,0,sizeof(resource_info));
  server_name=(char *) NULL;
  state=0;
  status=MagickTrue;
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowDisplayException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  image_marker=(unsigned long *)
    AcquireMagickMemory((argc+1)*sizeof(*image_marker));
  if (image_marker == (unsigned long *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowDisplayException(ResourceLimitError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
    }
  for (i=0; i <= argc; i++)
    image_marker[i]=(unsigned long) argc;
  /*
    Check for server name specified on the command line.
  */
  for (i=1; i < (long) argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == (long) argc)
          ThrowDisplayException(OptionError,"MissingArgument",option);
        server_name=argv[i];
      }
    if ((LocaleCompare("help",option+1) == 0) ||
        (LocaleCompare("-help",option+1) == 0))
      DisplayUsage();
  }
  /*
    Get user defaults from X resource database.
  */
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    ThrowDisplayException(XServerError,"UnableToOpenXServer",
      XDisplayName(server_name));
  (void) XSetErrorHandler(XError);
  resource_database=XGetResourceDatabase(display,GetClientName());
  XGetResourceInfo(resource_database,GetClientName(),&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  image_info->density=XGetResourceInstance(resource_database,GetClientName(),
    "density",(char *) NULL);
  if (image_info->density == (char *) NULL)
    image_info->density=XGetScreenDensity(display);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "interlace","none");
  image_info->interlace=(InterlaceType)
    ParseMagickOption(MagickInterlaceOptions,MagickFalse,resource_value);
  image_info->page=XGetResourceInstance(resource_database,GetClientName(),
    "pageGeometry",(char *) NULL);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "quality","75");
  image_info->quality=(unsigned long) atol(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "verbose","False");
  image_info->verbose=IsMagickTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "dither","True");
  quantize_info->dither=IsMagickTrue(resource_value);
  /*
    Parse command line.
  */
  iteration=0;
  iterations=0;
  for (i=1; ((i <= (long) argc) && ((state & ExitState) == 0)); i++)
  {
    if (i < (long) argc)
      option=argv[i];
    else
      if (image_stack[k] != (Image *) NULL)
        break;
      else
        if (isatty(STDIN_FILENO) != MagickFalse)
          option="logo:";
        else
          {
            int
              c;

            c=getc(stdin);
            if (c == EOF)
              break;
            else
              {
                c=ungetc(c,stdin);
                option="-";
              }
          }
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowDisplayException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowDisplayException(OptionError,"UnableToParseExpression",option);
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
        /*
          Option is a file name.
        */
        MogrifyImageStack(image_stack[k],MagickFalse,pend);
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          const char
            *filename;

          /*
            Read image.
          */
          filename=option;
          if ((LocaleCompare(filename,"--") == 0) && (i < (argc-1)))
            {
              option=argv[++i];
              filename=option;
            }
          (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              (void) InterpretImageFilename(filename,MaxTextExtent,
                image_info->filename,(int) scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                (void) FormatMagickString(filename,MaxTextExtent,"%s.%lu",
                  image_info->filename,scene);
              (void) CopyMagickString(image_info->filename,filename,
                MaxTextExtent);
            }
          image=ReadImage(image_info,exception);
          CatchException(exception);
          status&=(image != (Image *) NULL) &&
            (exception->severity < ErrorException);
          if (image == (Image *) NULL)
            continue;
          AppendImageToList(&image_stack[k],image);
          MogrifyImageStack(image_stack[k],MagickFalse,MagickTrue);
          do
          {
            /*
              Transmogrify image as defined by the image processing options.
            */
            resource_info.quantum=1;
            if (first_scene != last_scene)
              image_stack[k]->scene=(unsigned long) scene;
            if (resource_info.window_id != (char *) NULL)
              {
                /*
                  Display image to a specified X window.
                */
                status=XDisplayBackgroundImage(display,&resource_info,
                  image_stack[k]);
                if (status != MagickFalse)
                  state|=RetainColorsState;
              }
            else
              do
              {
                Image
                  *nexus;

                /*
                  Display image to X server.
                */
                nexus=XDisplayImage(display,&resource_info,argv,argc,
                  image_stack+k,&state);
                if (nexus == (Image *) NULL)
                  break;
                while ((nexus != (Image *) NULL) && ((state & ExitState) == 0))
                {
                  if (nexus->montage != (char *) NULL)
                    {
                      /*
                        User selected a visual directory image (montage).
                      */
                      image_stack[k]=DestroyImageList(image_stack[k]);
                      image_stack[k]=nexus;
                      break;
                    }
                  if (first_scene != last_scene)
                    image_stack[k]->scene=(unsigned long) scene;
                  image=XDisplayImage(display,&resource_info,argv,argc,&nexus,
                    &state);
                  if ((image == (Image *) NULL) &&
                      (GetNextImageInList(nexus) != (Image *) NULL))
                    {
                      image_stack[k]=DestroyImageList(image_stack[k]);
                      image_stack[k]=GetNextImageInList(nexus);
                      nexus=NewImageList();
                    }
                  else
                    {
                      if (nexus != image_stack[k])
                        nexus=DestroyImageList(nexus);
                      nexus=image;
                    }
                }
              } while ((state & ExitState) == 0);
            if (resource_info.write_filename != (char *) NULL)
              {
                /*
                  Write image.
                */
                (void) CopyMagickString(image_stack[k]->filename,
                  resource_info.write_filename,MaxTextExtent);
                (void) SetImageInfo(image_info,MagickTrue,
                  &image_stack[k]->exception);
                status&=WriteImage(image_info,image_stack[k]);
                GetImageException(image_stack[k],exception);
              }
            /*
              Proceed to next/previous image.
            */
            image=image_stack[k];
            if ((state & FormerImageState) != 0)
              for (l=0; l < resource_info.quantum; l++)
              {
                image=GetPreviousImageInList(image);
                if (image == (Image *) NULL)
                  break;
              }
            else
              for (l=0; l < resource_info.quantum; l++)
              {
                image=GetNextImageInList(image);
                if (image == (Image *) NULL)
                  break;
              }
            if (image != (Image *) NULL)
              image_stack[k]=image;
          } while ((image != (Image *) NULL) && ((state & ExitState) == 0));
          /*
            Free image resources.
          */
          image_stack[k]=DestroyImageList(image_stack[k]);
          if ((state & FormerImageState) == 0)
            {
              last_image=(unsigned long) image_number;
              image_marker[i]=(unsigned long) image_number++;
            }
          else
            {
              /*
                Proceed to previous image.
              */
              for (i--; i > 0; i--)
                if (image_marker[i] == (unsigned long) (image_number-2))
                  break;
              image_number--;
            }
          if ((i == (long) (argc-1)) && ((state & ExitState) == 0))
            i=0;
          if ((state & ExitState) != 0)
            break;
        }
        /*
          Determine if we should proceed to the first image.
        */
        if (image_number < 0)
          {
            if ((state & FormerImageState) != 0)
              {

                for (i=1; i < (argc-2); i++)
                  if (last_image == image_marker[i])
                    break;
                image_number=(long) image_marker[i]+1;
              }
            continue;
          }
        if (resource_info.window_id != (char *) NULL)
          state|=ExitState;
        if (iterations != 0)
          if (iteration == (long) iterations)
            state|=ExitState;
        iteration++;
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("antialias",option+1) == 0)
          break;
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("auto-orient",option+1) == 0)
          break;
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'b':
      {
        if (LocaleCompare("backdrop",option+1) == 0)
          {
            resource_info.backdrop=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.background_color=argv[i];
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.border_color=argv[i];
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            resource_info.border_width=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            resource_info.border_width=(unsigned int) atoi(argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            long
              channel;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowDisplayException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowDisplayException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("clip",option+1) == 0)
          break;
        if (LocaleCompare("clip-path",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("coalesce",option+1) == 0)
          break;
        if (LocaleCompare("colormap",option+1) == 0)
          {
            resource_info.colormap=PrivateColormap;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.colormap=UndefinedColormap;
            if (LocaleCompare("private",argv[i]) == 0)
              resource_info.colormap=PrivateColormap;
            if (LocaleCompare("shared",argv[i]) == 0)
              resource_info.colormap=SharedColormap;
            if (resource_info.colormap == UndefinedColormap)
              ThrowDisplayException(OptionError,"UnrecognizedColormapType",
                argv[i]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            quantize_info->number_colors=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            quantize_info->colorspace=UndefinedColorspace;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowDisplayException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            quantize_info->colorspace=(ColorspaceType) colorspace;
            if (quantize_info->colorspace == GRAYColorspace)
              quantize_info->number_colors=256;
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowDisplayException(OptionError,
                "UnrecognizedImageCompression",argv[i]);
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          break;
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowDisplayException(OptionError,"UnrecognizedEventType",
                argv[i]);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowDisplayException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          break;
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowDisplayException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,
              argv[i]);
            if (endian < 0)
              ThrowDisplayException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          break;
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            filter=ParseMagickOption(MagickFilterOptions,MagickFalse,
              argv[i]);
            if (filter < 0)
              ThrowDisplayException(OptionError,"UnrecognizedImageFilter",
                argv[i]);
            break;
          }
        if (LocaleCompare("flatten",option+1) == 0)
          break;
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.font=XGetResourceClass(resource_database,
              GetClientName(),"font",argv[i]);
            break;
          }
        if (LocaleCompare("foreground",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.foreground_color=argv[i];
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            resource_info.image_geometry=(char *) NULL;
            if (*option == '+')
              break;
            (void) CopyMagickString(argv[i]+1,"sans",MaxTextExtent);
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            resource_info.image_geometry=argv[i];
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'h':
      {
        if ((LocaleCompare("help",option+1) == 0) ||
            (LocaleCompare("-help",option+1) == 0))
          break;
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'i':
      {
        if (LocaleCompare("identify",option+1) == 0)
          break;
        if (LocaleCompare("iconGeometry",option+1) == 0)
          {
            resource_info.icon_geometry=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            resource_info.icon_geometry=argv[i];
            break;
          }
        if (LocaleCompare("iconic",option+1) == 0)
          {
            resource_info.iconic=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("immutable",option+1) == 0)
          {
            resource_info.immutable=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowDisplayException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowDisplayException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) ||
                (strchr(argv[i],'%') == (char *) NULL))
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            iterations=(unsigned long) atol(argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'm':
      {
        if (LocaleCompare("magnify",option+1) == 0)
          {
            resource_info.magnify=2;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            resource_info.magnify=(unsigned int) atoi(argv[i]);
            break;
          }
        if (LocaleCompare("map",option+1) == 0)
          {
            resource_info.map_type=(char *) NULL;
            if (*option == '+')
              break;
            (void) CopyMagickString(argv[i]+1,"sans",MaxTextExtent);
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.map_type=argv[i];
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.matte_color=argv[i];
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            if (*option == '+')
              break;
            quantize_info->number_colors=2;
            quantize_info->colorspace=GRAYColorspace;
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'n':
      {
        if (LocaleCompare("name",option+1) == 0)
          {
            resource_info.name=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.name=argv[i];
            break;
          }
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            resource_info.image_geometry=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.image_geometry=argv[i];
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'r':
      {
        if (LocaleCompare("raise",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("remote",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (XRemoteCommand(display,resource_info.window_id,argv[i]) != 0)
              return(MagickFalse);
            i--;
            break;
          }
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sans",option+1) == 0)
          {
            i++;
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsSceneGeometry(argv[i],MagickFalse) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            first_scene=atol(argv[i]);
            last_scene=first_scene;
            (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("shared-memory",option+1) == 0)
          {
            resource_info.use_shared_memory= (*option == '-') ? MagickTrue :
              MagickFalse;
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
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
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 't':
      {
        if (LocaleCompare("text-font",option+1) == 0)
          {
            resource_info.text_font=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.text_font=XGetResourceClass(resource_database,
              GetClientName(),"font",argv[i]);
            break;
          }
        if (LocaleCompare("texture",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("title",option+1) == 0)
          {
            resource_info.title=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.title=argv[i];
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            quantize_info->tree_depth=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'u':
      {
        if (LocaleCompare("update",option+1) == 0)
          {
            resource_info.update=(unsigned int) (*option == '-');
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowDisplayInvalidArgumentException(option,argv[i]);
            resource_info.update=(unsigned int) atoi(argv[i]);
            break;
          }
        if (LocaleCompare("use-pixmap",option+1) == 0)
          {
            resource_info.use_pixmap=(*option == '-') ? MagickTrue :
              MagickFalse;
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if ((LocaleCompare("version",option+1) == 0) ||
            (LocaleCompare("-version",option+1) == 0))
          break;
        if (LocaleCompare("visual",option+1) == 0)
          {
            resource_info.visual_type=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.visual_type=argv[i];
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowDisplayException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowDisplayException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case 'w':
      {
        if (LocaleCompare("window",option+1) == 0)
          {
            resource_info.window_id=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.window_id=argv[i];
            break;
          }
        if (LocaleCompare("window-group",option+1) == 0)
          {
            resource_info.window_group=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.window_group=argv[i];
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            resource_info.write_filename=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowDisplayException(OptionError,"MissingArgument",option);
            resource_info.write_filename=argv[i];
            if (IsAccessible(resource_info.write_filename) != MagickFalse)
              {
                char
                  answer[2];

                (void) fprintf(stderr,"Overwrite %s? ",
                  resource_info.write_filename);
                (void) fgets(answer,(int) sizeof(answer),stdin);
                if (((*answer != 'y') && (*answer != 'Y')))
                  return(MagickFalse);
              }
            break;
          }
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
      }
      case '?':
        break;
      default:
        ThrowDisplayException(OptionError,"UnrecognizedOption",option);
    }
    status=(MagickStatusType) ParseMagickOption(MagickMogrifyOptions,
      MagickFalse,option+1);
    if (status != 0)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowDisplayException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (state & RetainColorsState)
    {
      XRetainWindowColors(display,XRootWindow(display,XDefaultScreen(display)));
      (void) XSync(display,MagickFalse);
    }
  DestroyDisplay();
  return(status != 0 ? MagickTrue : MagickFalse);
#else
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "XWindowLibraryIsNotAvailable","`%s'",image_info->filename);
  DisplayUsage();
  return(MagickFalse);
#endif
}
