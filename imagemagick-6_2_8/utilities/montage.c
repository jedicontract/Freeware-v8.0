/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              M   M   OOO   N   N  TTTTT   AAA    GGGG  EEEEE                %
%              MM MM  O   O  NN  N    T    A   A  G      E                    %
%              M M M  O   O  N N N    T    AAAAA  G  GG  EEE                  %
%              M   M  O   O  N  NN    T    A   A  G   G  E                    %
%              M   M   OOO   N   N    T    A   A   GGGG  EEEEE                %
%                                                                             %
%                                                                             %
%              Montage Magick Image File Format Image via X11.                %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                              July 1992                                      %
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
%  Montage creates a composite by combining several separate images. The
%  images are tiled on the composite image with the name of the image
%  optionally appearing just below the individual tile.
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
%  M a i n                                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/
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
  status=MontageImageCommand(image_info,argc,argv,(char **) NULL,&exception);
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
