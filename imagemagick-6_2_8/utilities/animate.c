/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                AAA   N   N  IIIII  M   M   AAA   TTTTT  EEEEE               %
%               A   A  NN  N    I    MM MM  A   A    T    E                   %
%               AAAAA  N N N    I    M M M  AAAAA    T    EEE                 %
%               A   A  N  NN    I    M   M  A   A    T    E                   %
%               A   A  N   N  IIIII  M   M  A   A    T    EEEEE               %
%                                                                             %
%                                                                             %
%                   Interactively Animate an Image Sequence.                  %
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
%  Animate displays a sequence of images in the MIFF format on any
%  workstation display running an X server.  Animate first determines the
%  hardware capabilities of the workstation.  If the number of unique
%  colors in an image is less than or equal to the number the workstation
%  can support, the image is displayed in an X window.  Otherwise the
%  number of colors in the image is first reduced to match the color
%  resolution of the workstation before it is displayed.
%
%  This means that a continuous-tone 24 bits-per-pixel image can display on a
%  8 bit pseudo-color device or monochrome device.  In most instances the
%  reduced color image closely resembles the original.  Alternatively, a
%  monochrome or pseudo-color image can display on a continuous-tone 24
%  bits-per-pixel device.
%
%
*/

/*
  Include declarations.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wand/MagickWand.h"
#if defined(__WINDOWS__)
#include <windows.h>
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M a i n                                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

#if defined(__WINDOWS__)
int WINAPI WinMain(HINSTANCE instance,HINSTANCE last,LPSTR command,int state)
{
  char
    **argv;

  int
    argc,
    main(int,char **);

  argv=StringToArgv(command,&argc);
  return(main(argc,argv));
}
#endif

int main(int argc,char **argv)
{
  char
    *option;

  ExceptionInfo
    exception;

  ImageInfo
    *image_info;

  MagickBooleanType
    status;

  register long
    i;

  InitializeMagick(*argv);
  GetExceptionInfo(&exception);
  for (i=1; i < (long) argc; i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    if ((LocaleCompare("debug",option+1) == 0) && (i < (long) (argc-1)))
      (void) SetLogEventMask(argv[++i]);
    if ((LocaleCompare("version",option+1) == 0) ||
        (LocaleCompare("-version",option+1) == 0))
      {
        (void) fprintf(stdout,"Version: %s\n",
          GetMagickVersion((unsigned long *) NULL));
        (void) fprintf(stdout,"Copyright: %s\n\n",GetMagickCopyright());
        exit(0);
      }
  }
  image_info=CloneImageInfo((ImageInfo *) NULL);
  status=AnimateImageCommand(image_info,argc,argv,(char **) NULL,&exception);
  if ((status == MagickFalse) || (exception.severity != UndefinedException))
    {
      if (exception.severity < ErrorException)
        status=MagickTrue;
      CatchException(&exception);
    }
  image_info=DestroyImageInfo(image_info);
  (void) DestroyExceptionInfo(&exception);
  DestroyMagick();
  return(status == MagickFalse ? 1 : 0);
}
