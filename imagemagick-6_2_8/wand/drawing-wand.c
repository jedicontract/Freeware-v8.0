/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               DDDD   RRRR    AAA   W   W  IIIII  N   N    GGGG              %
%               D   D  R   R  A   A  W   W    I    NN  N   G                  %
%               D   D  RRRR   AAAAA  W   W    I    N N N   G  GG              %
%               D   D  R R    A   A  W W W    I    N  NN   G   G              %
%               DDDD   R  R   A   A   W W   IIIII  N   N    GGG               %
%                                                                             %
%                         W   W   AAA   N   N  DDDD                           %
%                         W   W  A   A  NN  N  D   D                          %
%                         W W W  AAAAA  N N N  D   D                          %
%                         WW WW  A   A  N  NN  D   D                          %
%                         W   W  A   A  N   N  DDDD                           %
%                                                                             %
%                                                                             %
%                   ImageMagick Image Vector Drawing Methods                  %
%                                                                             %
%                              Software Design                                %
%                              Bob Friesenhahn                                %
%                                March 2002                                   %
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
#include "wand/magick-wand-private.h"
#include "wand/mogrify-private.h"
#include "wand/wand.h"

/*
  Define declarations.
*/
#define DRAW_BINARY_IMPLEMENTATION 0

#define CurrentContext  (wand->graphic_context[wand->index])
#define DrawingWandId  "DrawingWand"
#define ThrowDrawException(severity,tag,reason) (void) ThrowMagickException( \
  &wand->exception,GetMagickModule(),severity,tag,"`%s'",reason);

/*
  Typedef declarations.
*/
typedef enum
{
  PathDefaultOperation,
  PathCloseOperation,                        /* Z|z (none) */
  PathCurveToOperation,                      /* C|c (x1 y1 x2 y2 x y)+ */
  PathCurveToQuadraticBezierOperation,       /* Q|q (x1 y1 x y)+ */
  PathCurveToQuadraticBezierSmoothOperation, /* T|t (x y)+ */
  PathCurveToSmoothOperation,                /* S|s (x2 y2 x y)+ */
  PathEllipticArcOperation,                  /* A|a (rx ry x-axis-rotation large-arc-flag sweep-flag x y)+ */
  PathLineToHorizontalOperation,             /* H|h x+ */
  PathLineToOperation,                       /* L|l (x y)+ */
  PathLineToVerticalOperation,               /* V|v y+ */
  PathMoveToOperation                        /* M|m (x y)+ */
} PathOperation;

typedef enum
{
  DefaultPathMode,
  AbsolutePathMode,
  RelativePathMode
} PathMode;

struct _DrawingWand
{
  unsigned long
    id;

  char
    name[MaxTextExtent];

  /* Support structures */
  Image
    *image;

  ExceptionInfo
    exception;

  /* MVG output string and housekeeping */
  char
    *mvg;               /* MVG data */

  size_t
    mvg_alloc,          /* total allocated memory */
    mvg_length;         /* total MVG length */

  unsigned long
    mvg_width;          /* current line width */

  /* Pattern support */
  char
    *pattern_id;

  RectangleInfo
    pattern_bounds;

  size_t
    pattern_offset;

  /* Graphic wand */
  unsigned long
    index;              /* array index */

  DrawInfo
    **graphic_context;

  MagickBooleanType
    filter_off;         /* true if not filtering attributes */

  /* Pretty-printing depth */
  unsigned long
    indent_depth;       /* number of left-hand pad characters */

  /* Path operation support */
  PathOperation
    path_operation;

  PathMode
    path_mode;

  MagickBooleanType
    destroy,
    debug;

  unsigned long
    signature;
};

/* Vector table for invoking subordinate renderers */
struct _DrawVTable
{
  DrawingWand *(*DestroyDrawingWand) (DrawingWand *);
  void (*DrawAnnotation)(DrawingWand *,const double,const double,
    const unsigned char *);
  void (*DrawArc)(DrawingWand *,const double,const double,const double,
    const double,const double,const double);
  void (*DrawBezier)(DrawingWand *,const unsigned long,const PointInfo *);
  void (*DrawCircle)(DrawingWand *,const double,const double,const double,
    const double);
  void (*DrawColor)(DrawingWand *,const double,const double,const PaintMethod);
  void (*DrawComment)(DrawingWand *,const char *);
  void (*DrawDestroyContext) (DrawContext context);
  void (*DrawEllipse)(DrawingWand *,const double,const double,const double,
    const double,const double,const double);
  MagickBooleanType (*DrawComposite)(DrawingWand *,const CompositeOperator,
    const double,const double,const double,const double,const Image *);
  void (*DrawLine)(DrawingWand *,const double,const double,const double,
    const double);
  void (*DrawMatte)(DrawingWand *,const double,const double,const PaintMethod);
  void (*DrawPathClose)(DrawingWand *);
  void (*DrawPathCurveToAbsolute)(DrawingWand *,const double,const double,
    const double,const double,const double,const double);
  void (*DrawPathCurveToRelative)(DrawingWand *,const double,const double,
    const double,const double,const double,const double);
  void (*DrawPathCurveToQuadraticBezierAbsolute)(DrawingWand *,const double,
    const double,const double,const double);
  void (*DrawPathCurveToQuadraticBezierRelative)(DrawingWand *,const double,
    const double,const double,const double);
  void (*DrawPathCurveToQuadraticBezierSmoothAbsolute)(DrawingWand *,
    const double,const double);
  void (*DrawPathCurveToQuadraticBezierSmoothRelative)(DrawingWand *,
    const double,const double);
  void (*DrawPathCurveToSmoothAbsolute)(DrawingWand *,const double,
    const double,const double,const double);
  void (*DrawPathCurveToSmoothRelative)(DrawingWand *,const double,
    const double,const double,const double);
  void (*DrawPathEllipticArcAbsolute)(DrawingWand *,const double,const double,
    const double,const MagickBooleanType,const MagickBooleanType,const double,
    const double);
  void (*DrawPathEllipticArcRelative)(DrawingWand *,const double,const double,
    const double,const MagickBooleanType,const MagickBooleanType,const double,
    const double);
  void (*DrawPathFinish)(DrawingWand *);
  void (*DrawPathLineToAbsolute)(DrawingWand *,const double,const double);
  void (*DrawPathLineToRelative)(DrawingWand *,const double,const double);
  void (*DrawPathLineToHorizontalAbsolute)(DrawingWand *,const double);
  void (*DrawPathLineToHorizontalRelative)(DrawingWand *,const double);
  void (*DrawPathLineToVerticalAbsolute)(DrawingWand *,const double);
  void (*DrawPathLineToVerticalRelative)(DrawingWand *,const double);
  void (*DrawPathMoveToAbsolute)(DrawingWand *,const double,const double);
  void (*DrawPathMoveToRelative)(DrawingWand *,const double,const double);
  void (*DrawPathStart)(DrawingWand *);
  void (*DrawPoint)(DrawingWand *,const double,const double);
  void (*DrawPolygon)(DrawingWand *,const unsigned long,const PointInfo *);
  void (*DrawPolyline)(DrawingWand *,const unsigned long,const PointInfo *);
  void (*DrawPopClipPath)(DrawingWand *);
  void (*DrawPopDefs)(DrawingWand *);
  MagickBooleanType (*DrawPopPattern)(DrawingWand *);
  void (*DrawPushClipPath)(DrawingWand *,const char *);
  void (*DrawPushDefs)(DrawingWand *);
  MagickBooleanType (*DrawPushPattern)(DrawingWand *,const char *,const double,
    const double,const double,const double);
  void (*DrawRectangle)(DrawingWand *,const double,const double,const double,
    const double);
  void (*DrawRoundRectangle)(DrawingWand *,double,double,double,double,
    double,double);
  void (*DrawAffine)(DrawingWand *,const AffineMatrix *);
  MagickBooleanType (*DrawSetClipPath)(DrawingWand *,const char *);
  void (*DrawSetClipRule)(DrawingWand *,const FillRule);
  void (*DrawSetClipUnits)(DrawingWand *,const ClipPathUnits);
  void (*DrawSetFillColor)(DrawingWand *,const PixelWand *);
  void (*DrawSetFillAlpha)(DrawingWand *,const double);
  void (*DrawSetFillRule)(DrawingWand *,const FillRule);
  MagickBooleanType (*DrawSetFillPatternURL)(DrawingWand *,const char *);
  MagickBooleanType (*DrawSetFont)(DrawingWand *,const char *);
  MagickBooleanType (*DrawSetFontFamily)(DrawingWand *,const char *);
  void (*DrawSetFontSize)(DrawingWand *,const double);
  void (*DrawSetFontStretch)(DrawingWand *,const StretchType);
  void (*DrawSetFontStyle)(DrawingWand *,const StyleType);
  void (*DrawSetFontWeight)(DrawingWand *,const unsigned long);
  void (*DrawSetGravity)(DrawingWand *,const GravityType);
  void (*DrawRotate)(DrawingWand *,const double);
  void (*DrawScale)(DrawingWand *,const double,const double);
  void (*DrawSkewX)(DrawingWand *,const double);
  void (*DrawSkewY)(DrawingWand *,const double);
  void (*DrawSetStrokeAntialias)(DrawingWand *,const MagickBooleanType);
  void (*DrawSetStrokeColor)(DrawingWand *,const PixelWand *);
  MagickBooleanType (*DrawSetStrokeDashArray)(DrawingWand *,const double *);
  void (*DrawSetStrokeDashOffset)(DrawingWand *,const double);
  void (*DrawSetStrokeLineCap)(DrawingWand *,const LineCap);
  void (*DrawSetStrokeLineJoin)(DrawingWand *,const LineJoin);
  void (*DrawSetStrokeMiterLimit)(DrawingWand *,const unsigned long);
  void (*DrawSetStrokeAlpha)(DrawingWand *,const double);
  MagickBooleanType (*DrawSetStrokePatternURL)(DrawingWand *,const char *);
  void (*DrawSetStrokeWidth)(DrawingWand *,const double);
  void (*DrawSetTextAntialias)(DrawingWand *,const MagickBooleanType);
  void (*DrawSetTextDecoration)(DrawingWand *,const DecorationType);
  void (*DrawSetTextUnderColor)(DrawingWand *,const PixelWand *);
  void (*DrawTranslate)(DrawingWand *,const double,const double);
  void (*DrawSetViewbox)(DrawingWand *,unsigned long,unsigned long,
    unsigned long,unsigned long);
  void (*PeekDrawingWand)(DrawingWand *);
  MagickBooleanType (*PopDrawingWand)(DrawingWand *);
  MagickBooleanType (*PushDrawingWand)(DrawingWand *);
};

/*
  Forward declarations.
*/
static int
  MvgPrintf(DrawingWand *,const char *,...) wand_attribute((format
    (printf,2,3))),
  MvgAutoWrapPrintf(DrawingWand *,const char *,...) wand_attribute((format
    (printf,2,3)));

static void
  MvgAppendColor(DrawingWand *,const PixelPacket *);

/*
  "Printf" for MVG commands
*/
static int MvgPrintf(DrawingWand *wand,const char *format,...)
{
  size_t
    alloc_size;

  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",format);
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  alloc_size=20*MaxTextExtent; /* 40K */
  if (wand->mvg == (char *) NULL)
    {
      wand->mvg=(char *) AcquireMagickMemory(alloc_size);
      if (wand->mvg == (char *) NULL)
        {
          ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
            wand->name);
          return(-1);
        }
      wand->mvg_alloc=alloc_size;
      wand->mvg_length=0;
    }
  if (wand->mvg_alloc < (wand->mvg_length+10*MaxTextExtent))
    {
      size_t
        realloc_size;

      realloc_size=wand->mvg_alloc+alloc_size;
      wand->mvg=(char *)
        ResizeMagickMemory(wand->mvg,realloc_size);
      if (wand->mvg == (char *) NULL)
        {
          ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
            wand->name);
          return -1;
        }
      wand->mvg_alloc=realloc_size;
    }
  {
    int
      formatted_length;

    va_list
      argp;

    while (wand->mvg_width < wand->indent_depth)
    {
      wand->mvg[wand->mvg_length]=' ';
      wand->mvg_length++;
      wand->mvg_width++;
    }
    wand->mvg[wand->mvg_length]='\0';
    va_start(argp, format);
#if defined(HAVE_VSNPRINTF)
    formatted_length=vsnprintf(wand->mvg+wand->mvg_length,
      wand->mvg_alloc-wand->mvg_length-1,format,argp);
#else
    formatted_length=vsprintf(wand->mvg+wand->mvg_length,
      format,argp);
#endif
    va_end(argp);
    if (formatted_length < 0)
      ThrowDrawException(DrawError,"UnableToPrint",format)
    else
      {
        wand->mvg_length+=formatted_length;
        wand->mvg_width+=formatted_length;
      }
    wand->mvg[wand->mvg_length]='\0';
    if ((wand->mvg_length > 1) &&
        (wand->mvg[wand->mvg_length-1] == '\n'))
      wand->mvg_width=0;
    assert((wand->mvg_length+1) < wand->mvg_alloc);
    return formatted_length;
  }
}

static int MvgAutoWrapPrintf(DrawingWand *wand,const char *format,...)
{
  char
    buffer[MaxTextExtent];

  int
    formatted_length;

  va_list
    argp;

  va_start(argp,format);
#if defined(HAVE_VSNPRINTF)
  formatted_length=vsnprintf(buffer,sizeof(buffer)-1,format,argp);
#else
  formatted_length=vsprintf(buffer,format,argp);
#endif
  va_end(argp);
  *(buffer+sizeof(buffer)-1)='\0';
  if (formatted_length < 0)
    ThrowDrawException(DrawError,"UnableToPrint",format)
  else
    {
      if (((wand->mvg_width + formatted_length) > 78) &&
          (buffer[formatted_length-1] != '\n'))
        (void) MvgPrintf(wand, "\n");
      (void) MvgPrintf(wand,"%s",buffer);
    }
  return(formatted_length);
}

static void MvgAppendColor(DrawingWand *wand,const PixelPacket *color)
{
  if ((color->red == 0) && (color->green == 0) && (color->blue == 0) &&
     (color->opacity == TransparentOpacity))
    (void) MvgPrintf(wand,"none");
  else
    {
      char
        tuple[MaxTextExtent];

      MagickPixelPacket
        pixel;

      GetMagickPixelPacket(wand->image,&pixel);
      pixel.colorspace=RGBColorspace;
      pixel.matte=color->opacity != OpaqueOpacity ? MagickTrue : MagickFalse;
      pixel.red=(MagickRealType) color->red;
      pixel.green=(MagickRealType) color->green;
      pixel.blue=(MagickRealType) color->blue;
      pixel.opacity=(MagickRealType) color->opacity;
      GetColorTuple(&pixel,MagickTrue,tuple);
      (void) MvgPrintf(wand,"%s",tuple);
    }
}

static void MvgAppendPointsCommand(DrawingWand *wand,char *command,
  const unsigned long number_coordinates,const PointInfo *coordinates)
{
  const PointInfo
    *coordinate;

  unsigned long
    i;

  (void) MvgPrintf(wand, "%s", command);
  for (i=number_coordinates, coordinate=coordinates; i != 0; i--)
  {
    (void) MvgAutoWrapPrintf(wand," %g,%g",coordinate->x,
      coordinate->y);
    coordinate++;
  }
  (void) MvgPrintf(wand, "\n");
}

static void AdjustAffine(DrawingWand *wand,const AffineMatrix *affine)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((affine->sx != 1.0) || (affine->rx != 0.0) || (affine->ry != 0.0) ||
      (affine->sy != 1.0) || (affine->tx != 0.0) || (affine->ty != 0.0))
    {
      AffineMatrix
        current;

      current=CurrentContext->affine;
      CurrentContext->affine.sx=current.sx*affine->sx+current.ry*affine->rx;
      CurrentContext->affine.rx=current.rx*affine->sx+current.sy*affine->rx;
      CurrentContext->affine.ry=current.sx*affine->ry+current.ry*affine->sy;
      CurrentContext->affine.sy=current.rx*affine->ry+current.sy*affine->sy;
      CurrentContext->affine.tx=current.sx*affine->tx+current.ry*affine->ty+
        current.tx;
      CurrentContext->affine.ty=current.rx*affine->tx+current.sy*affine->ty+
        current.ty;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l e a r D r a w i n g W a n d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClearDrawingWand() clear resources associated with the drawing wand.
%
%  The format of the ClearDrawingWand method is:
%
%      DrawingWand *ClearDrawingWand(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand. to destroy
%
*/
WandExport void ClearDrawingWand(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  for ( ; wand->index > 0; wand->index--)
    CurrentContext=DestroyDrawInfo(CurrentContext);
  CurrentContext=DestroyDrawInfo(CurrentContext);
  wand->graphic_context=(DrawInfo **)
    RelinquishMagickMemory(wand->graphic_context);
  if (wand->pattern_id != (char *) NULL)
    wand->pattern_id=(char *) RelinquishMagickMemory(wand->pattern_id);
  wand->mvg=(char *) RelinquishMagickMemory(wand->mvg);
  if ((wand->destroy != MagickFalse) && (wand->image != (Image *) NULL))
    wand->image=DestroyImage(wand->image);
  else
    wand->image=(Image *) NULL;
  wand->mvg=(char *) NULL;
  wand->mvg_alloc=0;
  wand->mvg_length=0;
  wand->mvg_width=0;
  wand->pattern_id=(char *) NULL;
  wand->pattern_offset=0;
  wand->pattern_bounds.x=0;
  wand->pattern_bounds.y=0;
  wand->pattern_bounds.width=0;
  wand->pattern_bounds.height=0;
  wand->index=0;
  wand->graphic_context=(DrawInfo **)
    AcquireMagickMemory(sizeof(*wand->graphic_context));
  if (wand->graphic_context == (DrawInfo **) NULL)
    {
      ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
        wand->name);
      return;
    }
  CurrentContext=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
  wand->filter_off=MagickFalse;
  wand->indent_depth=0;
  wand->path_operation=PathDefaultOperation;
  wand->path_mode=DefaultPathMode;
  wand->image=AllocateImage((const ImageInfo *) NULL);
  GetExceptionInfo(&wand->exception);
  wand->destroy=MagickTrue;
  wand->debug=IsEventLogging();
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e D r a w i n g W a n d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneDrawingWand() makes an exact copy of the specified wand.
%
%  The format of the CloneDrawingWand method is:
%
%      DrawingWand *CloneDrawingWand(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%
*/
WandExport DrawingWand *CloneDrawingWand(const DrawingWand *wand)
{
  DrawingWand
    *clone_wand;

  register long
    i;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  clone_wand=(DrawingWand *) AcquireMagickMemory(sizeof(*clone_wand));
  if (clone_wand == (DrawingWand *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowWandFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  (void) ResetMagickMemory(clone_wand,0,sizeof(*clone_wand));
  clone_wand->id=AcquireWandId();
  (void) FormatMagickString(clone_wand->name,MaxTextExtent,"DrawingWand-%lu",
    clone_wand->id);
  GetExceptionInfo(&clone_wand->exception);
  InheritException(&clone_wand->exception,&wand->exception);
  clone_wand->mvg=AcquireString(wand->mvg);
  clone_wand->mvg_length=strlen(clone_wand->mvg);
  clone_wand->mvg_alloc=wand->mvg_length+1;
  clone_wand->mvg_width=wand->mvg_width;
  clone_wand->pattern_id=AcquireString(wand->pattern_id);
  clone_wand->pattern_offset=wand->pattern_offset;
  clone_wand->pattern_bounds=wand->pattern_bounds;
  clone_wand->index=wand->index;
  clone_wand->graphic_context=(DrawInfo **) AcquireMagickMemory((size_t)
    (wand->index+1)*sizeof(*wand->graphic_context));
  if (clone_wand->graphic_context == (DrawInfo **) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
        message);
      message=(char *) RelinquishMagickMemory(message);
      return((DrawingWand *) NULL);
    }
  for (i=0; i <= (long) wand->index; i++)
    clone_wand->graphic_context[i]=
      CloneDrawInfo((ImageInfo *) NULL,wand->graphic_context[i]);
  clone_wand->filter_off=wand->filter_off;
  clone_wand->indent_depth=wand->indent_depth;
  clone_wand->path_operation=wand->path_operation;
  clone_wand->path_mode=wand->path_mode;
  clone_wand->image=wand->image;
  if (wand->image != (Image *) NULL)
    clone_wand->image=CloneImage(wand->image,0,0,MagickTrue,
      &clone_wand->exception);
  clone_wand->destroy=wand->destroy;
  clone_wand->debug=IsEventLogging();
  if (clone_wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",clone_wand->name);
  clone_wand->signature=WandSignature;
  return(clone_wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y D r a w i n g W a n d                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyDrawingWand() frees all resources associated with the drawing wand.
%  Once the drawing wand has been freed, it should not be used and further
%  unless it re-allocated.
%
%  The format of the DestroyDrawingWand method is:
%
%      DrawingWand *DestroyDrawingWand(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand. to destroy
%
*/
WandExport DrawingWand *DestroyDrawingWand(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  for ( ; wand->index > 0; wand->index--)
    CurrentContext=DestroyDrawInfo(CurrentContext);
  CurrentContext=DestroyDrawInfo(CurrentContext);
  wand->graphic_context=(DrawInfo **)
    RelinquishMagickMemory(wand->graphic_context);
  if (wand->pattern_id != (char *) NULL)
    wand->pattern_id=(char *) RelinquishMagickMemory(wand->pattern_id);
  wand->mvg=(char *) RelinquishMagickMemory(wand->mvg);
  if ((wand->destroy != MagickFalse) && (wand->image != (Image *) NULL))
    wand->image=DestroyImage(wand->image);
  else
    wand->image=(Image *) NULL;
  (void) DestroyExceptionInfo(&wand->exception);
  wand->signature=(~WandSignature);
  RelinquishWandId(wand->id);
  wand=(DrawingWand *) RelinquishMagickMemory(wand);
  return(wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w A f f i n e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawAffine() adjusts the current affine transformation matrix with
%  the specified affine transformation matrix. Note that the current affine
%  transform is adjusted rather than replaced.
%
%  The format of the DrawAffine method is:
%
%      void DrawAffine(DrawingWand *wand,const AffineMatrix *affine)
%
%  A description of each parameter follows:
%
%    o wand: Drawing wand
%
%    o affine: Affine matrix parameters
%
*/
WandExport void DrawAffine(DrawingWand *wand,const AffineMatrix *affine)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(affine != (const AffineMatrix *) NULL);
  AdjustAffine(wand,affine);
  (void) MvgPrintf(wand,"affine %g,%g,%g,%g,%g,%g\n",affine->sx,affine->rx,
    affine->ry,affine->sy,affine->tx,affine->ty);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D r a w A l l o c a t e W a n d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawAllocateWand() allocates an initial drawing wand which is an
%  opaque handle required by the remaining drawing methods.
%
%  The format of the DrawAllocateWand method is:
%
%      DrawingWand DrawAllocateWand(const DrawInfo *draw_info,Image *image)
%
%  A description of each parameter follows:
%
%    o draw_info: Initial drawing defaults. Set to NULL to use
%                 ImageMagick defaults.
%
%    o image: The image to draw on.
%
*/
WandExport DrawingWand *DrawAllocateWand(const DrawInfo *draw_info,Image *image)
{
  DrawingWand
    *wand;

  wand=NewDrawingWand();
  if (draw_info != (const DrawInfo *) NULL)
    {
      CurrentContext=DestroyDrawInfo(CurrentContext);
      CurrentContext=CloneDrawInfo((ImageInfo *) NULL,draw_info);
    }
  if (image != (Image *) NULL)
    {
      wand->image=DestroyImage(wand->image);
      wand->destroy=MagickFalse;
    }
  wand->image=image;
  return(wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w A n n o t a t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawAnnotation() draws text on the image.
%
%  The format of the DrawAnnotation method is:
%
%      void DrawAnnotation(DrawingWand *wand,const double x,
%        const double y,const unsigned char *text)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: x ordinate to left of text
%
%    o y: y ordinate to text baseline
%
%    o text: text to draw
%
*/
WandExport void DrawAnnotation(DrawingWand *wand,const double x,
  const double y,const unsigned char *text)
{
  char
    *escaped_text;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(text != (const unsigned char *) NULL);
  escaped_text=EscapeString((const char *) text,'\'');
  (void) MvgPrintf(wand,"text %g,%g '%s'\n",x,y,escaped_text);
  escaped_text=(char *) RelinquishMagickMemory(escaped_text);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w A r c                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawArc() draws an arc falling within a specified bounding rectangle on the
%  image.
%
%  The format of the DrawArc method is:
%
%      void DrawArc(DrawingWand *wand,const double sx,const double sy,
%        const double ex,const double ey,const double sd,const double ed)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o sx: starting x ordinate of bounding rectangle
%
%    o sy: starting y ordinate of bounding rectangle
%
%    o ex: ending x ordinate of bounding rectangle
%
%    o ey: ending y ordinate of bounding rectangle
%
%    o sd: starting degrees of rotation
%
%    o ed: ending degrees of rotation
%
*/
WandExport void DrawArc(DrawingWand *wand,const double sx,
  const double sy,const double ex,const double ey,const double sd,
  const double ed)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"arc %g,%g %g,%g %g,%g\n",sx,sy,ex,ey,sd,ed);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w B e z i e r                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawBezier() draws a bezier curve through a set of points on the image.
%
%  The format of the DrawBezier method is:
%
%      void DrawBezier(DrawingWand *wand,
%        const unsigned long number_coordinates,const PointInfo *coordinates)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o number_coordinates: number of coordinates
%
%    o coordinates: coordinates
%
*/
WandExport void DrawBezier(DrawingWand *wand,
  const unsigned long number_coordinates,const PointInfo *coordinates)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(coordinates != (const PointInfo *) NULL);
  MvgAppendPointsCommand(wand,"bezier",number_coordinates,coordinates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C i r c l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawCircle() draws a circle on the image.
%
%  The format of the DrawCircle method is:
%
%      void DrawCircle(DrawingWand *wand,const double ox,
%        const double oy,const double px, const double py)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o ox: origin x ordinate
%
%    o oy: origin y ordinate
%
%    o px: perimeter x ordinate
%
%    o py: perimeter y ordinate
%
*/
WandExport void DrawCircle(DrawingWand *wand,const double ox,
  const double oy,const double px,const double py)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"circle %g,%g %g,%g\n",ox,oy,px,py);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C l e a r E x c e p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawClearException() clear any exceptions associated with the wand.
%
%  The format of the DrawClearException method is:
%
%      MagickBooleanType DrawClearException(DrawWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport MagickBooleanType DrawClearException(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  ClearMagickException(&wand->exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C o m p o s i t e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawComposite() composites an image onto the current image, using the
%  specified composition operator, specified position, and at the specified
%  size.
%
%  The format of the DrawComposite method is:
%
%      MagickBooleanType DrawComposite(DrawingWand *wand,
%        const CompositeOperator compose,const double x,
%        const double y,const double width,const double height,
%        MagickWand *magick_wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o compose: composition operator
%
%    o x: x ordinate of top left corner
%
%    o y: y ordinate of top left corner
%
%    o width: Width to resize image to prior to compositing.  Specify zero to
%      use existing width.
%
%    o height: Height to resize image to prior to compositing.  Specify zero
%      to use existing height.
%
%    o magick_wand: Image to composite is obtained from this wand.
%
*/
WandExport MagickBooleanType DrawComposite(DrawingWand *wand,
  const CompositeOperator compose,const double x,const double y,
  const double width,const double height,MagickWand *magick_wand)

{
  char
    *base64,
    *media_type;

  const char
    *mode;

  ImageInfo
    *image_info;

  Image
    *clone_image,
    *image;

  register char
    *p;

  register long
    i;

  size_t
    blob_length,
    encoded_length;

  unsigned char
    *blob;

  MonitorHandler
    handler;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(magick_wand != (MagickWand *) NULL);
  assert(width != 0.0);
  assert(height != 0.0);
  image=GetImageFromMagickWand(magick_wand);
  if (image == (Image *) NULL)
    return(MagickFalse);
  clone_image=CloneImage(image,0,0,MagickTrue,&wand->exception);
  if (clone_image == (Image *) NULL)
    return(MagickFalse);
  image_info=CloneImageInfo((ImageInfo *) NULL);
  handler=SetMonitorHandler((MonitorHandler) NULL);
  blob_length=2048;
  blob=(unsigned char *) ImageToBlob(image_info,clone_image,&blob_length,
    &wand->exception);
  (void) SetMonitorHandler(handler);
  image_info=DestroyImageInfo(image_info);
  clone_image=DestroyImageList(clone_image);
  if (blob == (void *) NULL)
    return(MagickFalse);
  encoded_length=0;
  base64=Base64Encode(blob,blob_length,&encoded_length);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  if (base64 == (char *) NULL)
    {
      char
        buffer[MaxTextExtent];

      (void) FormatMagickString(buffer,MaxTextExtent,"%ld bytes",
        (4L*blob_length/3L+4L));
      ThrowDrawException(ResourceLimitWarning,"UnableToAllocateMemory",
        wand->name);
      return(MagickFalse);
    }
  mode=MagickOptionToMnemonic(MagickCompositeOptions,(long) compose);
  media_type=MagickToMime(image->magick);
  (void) MvgPrintf(wand,"image %s %g,%g %g,%g 'data:%s;base64,\n",mode,x,y,
    width,height,media_type);
  p=base64;
  for (i=(long) encoded_length; i > 0; i-=76)
  {
    (void) MvgPrintf(wand,"%.76s",p);
    p+=76;
    if (i > 76)
      (void) MvgPrintf(wand,"\n");
  }
  (void) MvgPrintf(wand,"'\n");
  media_type=(char *) RelinquishMagickMemory(media_type);
  base64=(char *) RelinquishMagickMemory(base64);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C o l o r                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawColor() draws color on image using the current fill color, starting at
%  specified position, and using specified paint method. The available paint
%  methods are:
%
%    PointMethod: Recolors the target pixel
%    ReplaceMethod: Recolor any pixel that matches the target pixel.
%    FloodfillMethod: Recolors target pixels and matching neighbors.
%    ResetMethod: Recolor all pixels.
%
%  The format of the DrawColor method is:
%
%      void DrawColor(DrawingWand *wand,const double x,const double y,
%        const PaintMethod paint_method)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: x ordinate.
%
%    o y: y ordinate.
%
%    o paint_method: paint method.
%
*/
WandExport void DrawColor(DrawingWand *wand,const double x,
  const double y,const PaintMethod paint_method)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"color %g,%g '%s'\n",x,y,MagickOptionToMnemonic(
    MagickMethodOptions,(long) paint_method));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w C o m m e n t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawComment() adds a comment to a vector output stream.
%
%  The format of the DrawComment method is:
%
%      void DrawComment(DrawingWand *wand,const char *comment)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o comment: comment text
%
*/
WandExport void DrawComment(DrawingWand *wand,const char * comment)
{
  (void) MvgPrintf(wand,"#%s\n",comment);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w E l l i p s e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawEllipse() draws an ellipse on the image.
%
%  The format of the DrawEllipse method is:
%
%       void DrawEllipse(DrawingWand *wand,const double ox,
%         const double oy,const double rx,const double ry,const double start,
%         const double end)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o ox: origin x ordinate
%
%    o oy: origin y ordinate
%
%    o rx: radius in x
%
%    o ry: radius in y
%
%    o start: starting rotation in degrees
%
%    o end: ending rotation in degrees
%
*/
WandExport void DrawEllipse(DrawingWand *wand,const double ox,
  const double oy,const double rx,const double ry,const double start,
  const double end)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"ellipse %g,%g %g,%g %g,%g\n",ox,oy,rx,ry,start,end);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t C l i p P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetClipPath() obtains the current clipping path ID. The value returned
%  must be deallocated by the user when it is no longer needed.
%
%  The format of the DrawGetClipPath method is:
%
%      char *DrawGetClipPath(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport char *DrawGetClipPath(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (CurrentContext->clip_path != (char *) NULL)
    return((char *) AcquireString(CurrentContext->clip_path));
  return((char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t C l i p R u l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetClipRule() returns the current polygon fill rule to be used by the
%  clipping path.
%
%  The format of the DrawGetClipRule method is:
%
%     FillRule DrawGetClipRule(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport FillRule DrawGetClipRule(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->fill_rule);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t C l i p U n i t s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetClipUnits() returns the interpretation of clip path units.
%
%  The format of the DrawGetClipUnits method is:
%
%      ClipPathUnits DrawGetClipUnits(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport ClipPathUnits DrawGetClipUnits(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->clip_units);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t E x c e p t i o n                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetException() returns the severity, reason, and description of any
%  error that occurs when using other methods in this API.
%
%  The format of the DrawGetException method is:
%
%      char *DrawGetException(const DrawWand *wand,
%        ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o severity: The severity of the error is returned here.
%
*/
WandExport char *DrawGetException(const DrawingWand *wand,
  ExceptionType *severity)
{
  char
    *description;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(severity != (ExceptionType *) NULL);
  *severity=wand->exception.severity;
  description=(char *) AcquireMagickMemory(2*MaxTextExtent);
  if (description == (char *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowWandFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  *description='\0';
  if (wand->exception.reason != (char *) NULL)
    (void) CopyMagickString(description,GetLocaleExceptionMessage(
      wand->exception.severity,wand->exception.reason),
      MaxTextExtent);
  if (wand->exception.description != (char *) NULL)
    {
      (void) ConcatenateMagickString(description," (",MaxTextExtent);
      (void) ConcatenateMagickString(description,GetLocaleExceptionMessage(
        wand->exception.severity,wand->exception.description),
        MaxTextExtent);
      (void) ConcatenateMagickString(description,")",MaxTextExtent);
    }
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F i l l C o l o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFillColor() returns the fill color used for drawing filled objects.
%
%  The format of the DrawGetFillColor method is:
%
%      void DrawGetFillColor(const DrawingWand *wand,
%        PixelWand *fill_color)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_color: Return the fill color.
%
*/
WandExport void DrawGetFillColor(const DrawingWand *wand,
  PixelWand *fill_color)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  assert(fill_color != (PixelWand *) NULL);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  PixelSetQuantumColor(fill_color,&CurrentContext->fill);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F i l l O p a c i t y                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFillAlpha() returns the opacity used when drawing using the fill
%  color or fill texture.  Fully opaque is 1.0.
%
%  The format of the DrawGetFillAlpha method is:
%
%      double DrawGetFillAlpha(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/

WandExport double DrawGetFillAlpha(const DrawingWand *wand)
{
  return(DrawGetFillOpacity(wand));
}

WandExport double DrawGetFillOpacity(const DrawingWand *wand)
{
  double
    alpha;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  alpha=(double) QuantumScale*(QuantumRange-CurrentContext->fill.opacity);
  return(alpha);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F i l l R u l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFillRule() returns the fill rule used while drawing polygons.
%
%  The format of the DrawGetFillRule method is:
%
%      FillRule DrawGetFillRule(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport FillRule DrawGetFillRule(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->fill_rule);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFont() returns a null-terminaged string specifying the font used
%  when annotating with text. The value returned must be freed by the user
%  when no longer needed.
%
%  The format of the DrawGetFont method is:
%
%      char *DrawGetFont(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport char *DrawGetFont(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (CurrentContext->font != (char *) NULL)
    return(AcquireString(CurrentContext->font));
  return((char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t F a m i l y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFontFamily() returns the font family to use when annotating with text.
%  The value returned must be freed by the user when it is no longer needed.
%
%  The format of the DrawGetFontFamily method is:
%
%      char *DrawGetFontFamily(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport char *DrawGetFontFamily(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (CurrentContext->family != NULL)
    return(AcquireString(CurrentContext->family));
  return((char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t S i z e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFontSize() returns the font pointsize used when annotating with text.
%
%  The format of the DrawGetFontSize method is:
%
%      double DrawGetFontSize(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport double DrawGetFontSize(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->pointsize);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t S t r e t c h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFontStretch() returns the font stretch used when annotating with text.
%
%  The format of the DrawGetFontStretch method is:
%
%      StretchType DrawGetFontStretch(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport StretchType DrawGetFontStretch(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->stretch);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t S t y l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFontStyle() returns the font style used when annotating with text.
%
%  The format of the DrawGetFontStyle method is:
%
%      StyleType DrawGetFontStyle(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport StyleType DrawGetFontStyle(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->style);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t F o n t W e i g h t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetFontWeight() returns the font weight used when annotating with text.
%
%  The format of the DrawGetFontWeight method is:
%
%      unsigned long DrawGetFontWeight(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport unsigned long DrawGetFontWeight(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->weight);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t G r a v i t y                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetGravity() returns the text placement gravity used when annotating
%  with text.
%
%  The format of the DrawGetGravity method is:
%
%      GravityType DrawGetGravity(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport GravityType DrawGetGravity(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->gravity);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e A n t i a l i a s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeAntialias() returns the current stroke antialias setting.
%  Stroked outlines are antialiased by default.  When antialiasing is disabled
%  stroked pixels are thresholded to determine if the stroke color or
%  underlying canvas color should be used.
%
%  The format of the DrawGetStrokeAntialias method is:
%
%      MagickBooleanType DrawGetStrokeAntialias(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/
WandExport MagickBooleanType DrawGetStrokeAntialias(
  const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->stroke_antialias);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e C o l o r                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeColor() returns the color used for stroking object outlines.
%
%  The format of the DrawGetStrokeColor method is:
%
%      void DrawGetStrokeColor(const DrawingWand *wand,
$        PixelWand *stroke_color)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_color: Return the stroke color.
%
*/
WandExport void DrawGetStrokeColor(const DrawingWand *wand,
  PixelWand *stroke_color)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  assert(stroke_color != (PixelWand *) NULL);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  PixelSetQuantumColor(stroke_color,&CurrentContext->stroke);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e D a s h A r r a y                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeDashArray() returns an array representing the pattern of
%  dashes and gaps used to stroke paths (see DrawSetStrokeDashArray). The
%  array must be freed once it is no longer required by the user.
%
%  The format of the DrawGetStrokeDashArray method is:
%
%      double *DrawGetStrokeDashArray(const DrawingWand *wand,
%        unsigned long *number_elements)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o number_elements: address to place number of elements in dash array
%
*/
WandExport double *DrawGetStrokeDashArray(const DrawingWand *wand,
  unsigned long *number_elements)
{
  double
    *dash_array;

  register const double
    *p;

  register double
    *q;

  register long
    i;

  unsigned long
    n;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(number_elements != (unsigned long *) NULL);
  n=0;
  p=CurrentContext->dash_pattern;
  if (p != (const double *) NULL)
    while (*p++ != 0.0)
      n++;
  *number_elements=n;
  dash_array=(double *) NULL;
  if (n != 0)
    {
      dash_array=(double *) AcquireMagickMemory((size_t) n*sizeof(*dash_array));
      p=CurrentContext->dash_pattern;
      q=dash_array;
      for (i=0; i < (long) n; i++)
        *q++=(*p++);
    }
  return(dash_array);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e D a s h O f f s e t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeDashOffset() returns the offset into the dash pattern to
%  start the dash.
%
%  The format of the DrawGetStrokeDashOffset method is:
%
%      double DrawGetStrokeDashOffset(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport double DrawGetStrokeDashOffset(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->dash_offset);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e L i n e C a p                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeLineCap() returns the shape to be used at the end of
%  open subpaths when they are stroked. Values of LineCap are
%  UndefinedCap, ButtCap, RoundCap, and SquareCap.
%
%  The format of the DrawGetStrokeLineCap method is:
%
%      LineCap DrawGetStrokeLineCap(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport LineCap DrawGetStrokeLineCap(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->linecap);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e L i n e J o i n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeLineJoin() returns the shape to be used at the
%  corners of paths (or other vector shapes) when they are
%  stroked. Values of LineJoin are UndefinedJoin, MiterJoin, RoundJoin,
%  and BevelJoin.
%
%  The format of the DrawGetStrokeLineJoin method is:
%
%      LineJoin DrawGetStrokeLineJoin(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport LineJoin DrawGetStrokeLineJoin(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->linejoin);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e M i t e r L i m i t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeMiterLimit() returns the miter limit. When two line
%  segments meet at a sharp angle and miter joins have been specified for
%  'lineJoin', it is possible for the miter to extend far beyond the
%  thickness of the line stroking the path. The miterLimit' imposes a
%  limit on the ratio of the miter length to the 'lineWidth'.
%
%  The format of the DrawGetStrokeMiterLimit method is:
%
%      unsigned long DrawGetStrokeMiterLimit(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport unsigned long DrawGetStrokeMiterLimit(
  const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return CurrentContext->miterlimit;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e O p a c i t y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeAlpha() returns the opacity of stroked object outlines.
%
%  The format of the DrawGetStrokeAlpha method is:
%
%      double DrawGetStrokeAlpha(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
*/

WandExport double DrawGetStrokeAlpha(const DrawingWand *wand)
{
  return(DrawGetStrokeOpacity(wand));
}

WandExport double DrawGetStrokeOpacity(const DrawingWand *wand)
{
  double
    alpha;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  alpha=(double) QuantumScale*(QuantumRange-CurrentContext->stroke.opacity);
  return(alpha);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t S t r o k e W i d t h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetStrokeWidth() returns the width of the stroke used to draw object
%  outlines.
%
%  The format of the DrawGetStrokeWidth method is:
%
%      double DrawGetStrokeWidth(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport double DrawGetStrokeWidth(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->stroke_width);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t T e x t A l i g n m e n t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetTextAlignment() returns the alignment applied when annotating with
%  text.
%
%  The format of the DrawGetTextAlignment method is:
%
%      AlignType DrawGetTextAlignment(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport AlignType DrawGetTextAlignment(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->align);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t T e x t A n t i a l i a s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetTextAntialias() returns the current text antialias setting, which
%  determines whether text is antialiased.  Text is antialiased by default.
%
%  The format of the DrawGetTextAntialias method is:
%
%      MagickBooleanType DrawGetTextAntialias(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport MagickBooleanType DrawGetTextAntialias(
  const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->text_antialias);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t T e x t D e c o r a t i o n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetTextDecoration() returns the decoration applied when annotating with
%  text.
%
%  The format of the DrawGetTextDecoration method is:
%
%      DecorationType DrawGetTextDecoration(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport DecorationType DrawGetTextDecoration(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(CurrentContext->decorate);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t T e x t E n c o d i n g                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetTextEncoding() returns a null-terminated string which specifies the
%  code set used for text annotations. The string must be freed by the user
%  once it is no longer required.
%
%  The format of the DrawGetTextEncoding method is:
%
%      char *DrawGetTextEncoding(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport char *DrawGetTextEncoding(const DrawingWand *wand)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (CurrentContext->encoding != (char *) NULL)
    return((char *) AcquireString(CurrentContext->encoding));
  return((char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t V e c t o r G r a p h i c s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetVectorGraphics() returns a null-terminated string which specifies the
%  vector graphics generated by any graphics calls made since the wand was
%  instantiated.  The string must be freed by the user once it is no longer
%  required.
%
%  The format of the DrawGetVectorGraphics method is:
%
%      char *DrawGetVectorGraphics(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport char *DrawGetVectorGraphics(DrawingWand *wand)
{
  char
    value[MaxTextExtent],
    *xml;

  MagickPixelPacket
    pixel;

  register long
    i;

  XMLTreeInfo
    *child,
    *xml_info;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  xml_info=NewXMLTreeTag("drawing-wand");
  if (xml_info == (XMLTreeInfo *) NULL)
    return(char *) NULL;
  GetMagickPixelPacket(wand->image,&pixel);
  child=AddChildToXMLTree(xml_info,"clip-path",0);
  if (child != (XMLTreeInfo *) NULL)
    (void) SetXMLTreeContent(child,CurrentContext->clip_path);
  child=AddChildToXMLTree(xml_info,"clip-units",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickClipPathOptions,(long) CurrentContext->clip_units),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"decorate",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickDecorationOptions,(long) CurrentContext->decorate),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"encoding",0);
  if (child != (XMLTreeInfo *) NULL)
    (void) SetXMLTreeContent(child,CurrentContext->encoding);
  child=AddChildToXMLTree(xml_info,"fill",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      if (CurrentContext->fill.opacity != OpaqueOpacity)
        pixel.matte=CurrentContext->fill.opacity != OpaqueOpacity ?
          MagickTrue : MagickFalse;
      SetMagickPixelPacket(&CurrentContext->fill,(const IndexPacket *) NULL,
        &pixel);
      GetColorTuple(&pixel,MagickTrue,value);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"fill-opacity",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%g",
        (double) QuantumScale*(QuantumRange-CurrentContext->fill.opacity));
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"fill-rule",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickFillRuleOptions,(long) CurrentContext->fill_rule),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"font",0);
  if (child != (XMLTreeInfo *) NULL)
    (void) SetXMLTreeContent(child,CurrentContext->font);
  child=AddChildToXMLTree(xml_info,"font-family",0);
  if (child != (XMLTreeInfo *) NULL)
    (void) SetXMLTreeContent(child,CurrentContext->family);
  child=AddChildToXMLTree(xml_info,"font-size",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%g",
        CurrentContext->pointsize);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"font-stretch",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickStretchOptions,(long) CurrentContext->stretch),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"font-style",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickStyleOptions,(long) CurrentContext->style),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"font-weight",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%lu",
        CurrentContext->weight);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"gravity",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(MagickGravityOptions,
        (long) CurrentContext->gravity),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      if (CurrentContext->stroke.opacity != OpaqueOpacity)
        pixel.matte=CurrentContext->stroke.opacity != OpaqueOpacity ?
          MagickTrue : MagickFalse;
      SetMagickPixelPacket(&CurrentContext->stroke,(const IndexPacket *) NULL,
        &pixel);
      GetColorTuple(&pixel,MagickTrue,value);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-antialias",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%d",
        CurrentContext->stroke_antialias != MagickFalse ? 1 : 0);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-dasharray",0);
  if ((child != (XMLTreeInfo *) NULL) &&
      (CurrentContext->dash_pattern != (double *) NULL))
    {
      char
        *dash_pattern;

      dash_pattern=AcquireString((char *) NULL);
      for (i=0; CurrentContext->dash_pattern[i] != 0.0; i++)
      {
        if (i != 0)
          (void) ConcatenateString(&dash_pattern,",");
        (void) FormatMagickString(value,MaxTextExtent,"%g",
          CurrentContext->dash_pattern[i]);
        (void) ConcatenateString(&dash_pattern,value);
      }
      (void) SetXMLTreeContent(child,dash_pattern);
      dash_pattern=(char *) RelinquishMagickMemory(dash_pattern);
    }
  child=AddChildToXMLTree(xml_info,"stroke-dashoffset",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%g",
        CurrentContext->dash_offset);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-linecap",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(MagickLineCapOptions,
        (long) CurrentContext->linecap),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-linejoin",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(
        MagickLineJoinOptions,(long) CurrentContext->linejoin),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-miterlimit",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%lu",
        CurrentContext->miterlimit);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-opacity",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%g",
        (double) QuantumScale*(QuantumRange-CurrentContext->stroke.opacity));
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"stroke-width",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%g",
        CurrentContext->stroke_width);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"text-align",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CopyMagickString(value,MagickOptionToMnemonic(MagickAlignOptions,
        (long) CurrentContext->align),MaxTextExtent);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"text-antialias",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) FormatMagickString(value,MaxTextExtent,"%d",
        CurrentContext->text_antialias != MagickFalse ? 1 : 0);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"text-undercolor",0);
  if (child != (XMLTreeInfo *) NULL)
    {
      if (CurrentContext->undercolor.opacity != OpaqueOpacity)
        pixel.matte=CurrentContext->undercolor.opacity != OpaqueOpacity ?
          MagickTrue : MagickFalse;
      SetMagickPixelPacket(&CurrentContext->undercolor,
        (const IndexPacket *) NULL,&pixel);
      GetColorTuple(&pixel,MagickTrue,value);
      (void) SetXMLTreeContent(child,value);
    }
  child=AddChildToXMLTree(xml_info,"vector-graphics",0);
  if (child != (XMLTreeInfo *) NULL)
    (void) SetXMLTreeContent(child,wand->mvg);
  xml=XMLTreeInfoToXML(xml_info);
  xml_info=DestroyXMLTree(xml_info);
  return(xml);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w G e t T e x t U n d e r C o l o r                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawGetTextUnderColor() returns the color of a background rectangle
%  to place under text annotations.
%
%  The format of the DrawGetTextUnderColor method is:
%
%      void DrawGetTextUnderColor(const DrawingWand *wand,
%        PixelWand *under_color)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o under_color: Return the under color.
%
*/
WandExport void DrawGetTextUnderColor(const DrawingWand *wand,
  PixelWand *under_color)
{
  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  assert(under_color != (PixelWand *) NULL);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  PixelSetQuantumColor(under_color,&CurrentContext->undercolor);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w L i n e                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawLine() draws a line on the image using the current stroke color,
%  stroke opacity, and stroke width.
%
%  The format of the DrawLine method is:
%
%      void DrawLine(DrawingWand *wand,const double sx,const double sy,
%        const double ex,const double ey)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o sx: starting x ordinate
%
%    o sy: starting y ordinate
%
%    o ex: ending x ordinate
%
%    o ey: ending y ordinate
%
*/
WandExport void DrawLine(DrawingWand *wand,const double sx,
  const double sy,const double ex,const double ey)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"line %g,%g %g,%g\n",sx,sy,ex,ey);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w M a t t e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawMatte() paints on the image's opacity channel in order to set effected
%  pixels to transparent.
%  to influence the opacity of pixels. The available paint
%  methods are:
%
%    PointMethod: Select the target pixel
%    ReplaceMethod: Select any pixel that matches the target pixel.
%    FloodfillMethod: Select the target pixel and matching neighbors.
%    FillToBorderMethod: Select the target pixel and neighbors not matching border color.
%    ResetMethod: Select all pixels.
%
%  The format of the DrawMatte method is:
%
%      void DrawMatte(DrawingWand *wand,const double x,const double y,
%        const PaintMethod paint_method)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: x ordinate
%
%    o y: y ordinate
%
%    o paint_method: paint method.
%
*/
WandExport void DrawMatte(DrawingWand *wand,const double x,
  const double y,const PaintMethod paint_method)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"matte %g,%g '%s'\n",x,y,MagickOptionToMnemonic(
    MagickMethodOptions,(long) paint_method));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C l o s e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathClose() adds a path element to the current path which closes the
%  current subpath by drawing a straight line from the current point to the
%  current subpath's most recent starting point (usually, the most recent
%  moveto point).
%
%  The format of the DrawPathClose method is:
%
%      void DrawPathClose(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPathClose(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgAutoWrapPrintf(wand,"%s",wand->path_mode == AbsolutePathMode ?
    "Z" : "z");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o A b s o l u t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToAbsolute() draws a cubic Bezier curve from the current
%  point to (x,y) using (x1,y1) as the control point at the beginning of
%  the curve and (x2,y2) as the control point at the end of the curve using
%  absolute coordinates. At the end of the command, the new current point
%  becomes the final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToAbsolute method is:
%
%      void DrawPathCurveToAbsolute(DrawingWand *wand,const double x1,
%        const double y1,const double x2,const double y2,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: x ordinate of control point for curve beginning
%
%    o y1: y ordinate of control point for curve beginning
%
%    o x2: x ordinate of control point for curve ending
%
%    o y2: y ordinate of control point for curve ending
%
%    o x: x ordinate of the end of the curve
%
%    o y: y ordinate of the end of the curve
%
*/

static void DrawPathCurveTo(DrawingWand *wand,const PathMode mode,
  const double x1,const double y1,const double x2,const double y2,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathCurveToOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathCurveToOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand, "%c%g,%g %g,%g %g,%g",
        mode == AbsolutePathMode ? 'C' : 'c',x1,y1,x2,y2,x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g %g,%g %g,%g",x1,y1,x2,y2,x,y);
}

WandExport void DrawPathCurveToAbsolute(DrawingWand *wand,
  const double x1,const double y1,const double x2,const double y2,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveTo(wand,AbsolutePathMode,x1,y1,x2,y2,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o R e l a t i v e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToRelative() draws a cubic Bezier curve from the current
%  point to (x,y) using (x1,y1) as the control point at the beginning of
%  the curve and (x2,y2) as the control point at the end of the curve using
%  relative coordinates. At the end of the command, the new current point
%  becomes the final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToRelative method is:
%
%      void DrawPathCurveToRelative(DrawingWand *wand,const double x1,
%        const double y1,const double x2,const double y2,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: x ordinate of control point for curve beginning
%
%    o y1: y ordinate of control point for curve beginning
%
%    o x2: x ordinate of control point for curve ending
%
%    o y2: y ordinate of control point for curve ending
%
%    o x: x ordinate of the end of the curve
%
%    o y: y ordinate of the end of the curve
%
*/
WandExport void DrawPathCurveToRelative(DrawingWand *wand,
  const double x1,const double y1,const double x2,const double y2,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveTo(wand,RelativePathMode,x1,y1,x2,y2,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o Q u a d r a t i c B e z i e r A b s o l u t e %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToQuadraticBezierAbsolute() draws a quadratic Bezier curve
%  from the current point to (x,y) using (x1,y1) as the control point using
%  absolute coordinates. At the end of the command, the new current point
%  becomes the final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToQuadraticBezierAbsolute method is:
%
%      void DrawPathCurveToQuadraticBezierAbsolute(DrawingWand *wand,
%        const double x1,const double y1,onst double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: x ordinate of the control point
%
%    o y1: y ordinate of the control point
%
%    o x: x ordinate of final point
%
%    o y: y ordinate of final point
%
*/

static void DrawPathCurveToQuadraticBezier(DrawingWand *wand,
  const PathMode mode,const double x1,double y1,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathCurveToQuadraticBezierOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathCurveToQuadraticBezierOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand, "%c%g,%g %g,%g",mode == AbsolutePathMode ?
        'Q' : 'q',x1,y1,x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g %g,%g",x1,y1,x,y);
}

WandExport void DrawPathCurveToQuadraticBezierAbsolute(
  DrawingWand *wand,const double x1,const double y1,const double x,
  const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveToQuadraticBezier(wand,AbsolutePathMode,x1,y1,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o Q u a d r a t i c B e z i e r R e l a t i v %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToQuadraticBezierRelative() draws a quadratic Bezier curve
%  from the current point to (x,y) using (x1,y1) as the control point using
%  relative coordinates. At the end of the command, the new current point
%  becomes the final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToQuadraticBezierRelative method is:
%
%      void DrawPathCurveToQuadraticBezierRelative(DrawingWand *wand,
%        const double x1,const double y1,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: x ordinate of the control point
%
%    o y1: y ordinate of the control point
%
%    o x: x ordinate of final point
%
%    o y: y ordinate of final point
%
*/
WandExport void DrawPathCurveToQuadraticBezierRelative(
  DrawingWand *wand,const double x1,const double y1,const double x,
  const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveToQuadraticBezier(wand,RelativePathMode,x1,y1,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o Q u a d r a t i c B e z i e r S m o o t h   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToQuadraticBezierSmoothAbsolute() draws a quadratic
%  Bezier curve (using absolute coordinates) from the current point to
%  (x,y). The control point is assumed to be the reflection of the
%  control point on the previous command relative to the current
%  point. (If there is no previous command or if the previous command was
%  not a DrawPathCurveToQuadraticBezierAbsolute,
%  DrawPathCurveToQuadraticBezierRelative,
%  DrawPathCurveToQuadraticBezierSmoothAbsolut or
%  DrawPathCurveToQuadraticBezierSmoothRelative, assume the control point
%  is coincident with the current point.). At the end of the command, the
%  new current point becomes the final (x,y) coordinate pair used in the
%  polybezier.
%
%  The format of the DrawPathCurveToQuadraticBezierSmoothAbsolute method is:
%
%      void DrawPathCurveToQuadraticBezierSmoothAbsolute(
%        DrawingWand *wand,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: x ordinate of final point
%
%    o y: y ordinate of final point
%
*/

static void DrawPathCurveToQuadraticBezierSmooth(DrawingWand *wand,
  const PathMode mode,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathCurveToQuadraticBezierSmoothOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathCurveToQuadraticBezierSmoothOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g,%g",mode == AbsolutePathMode ?
        'T' : 't',x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g",x,y);
}

WandExport void DrawPathCurveToQuadraticBezierSmoothAbsolute(
  DrawingWand *wand,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveToQuadraticBezierSmooth(wand,AbsolutePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o Q u a d r a t i c B e z i e r S m o o t h   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToQuadraticBezierSmoothAbsolute() draws a quadratic
%  Bezier curve (using relative coordinates) from the current point to
%  (x,y). The control point is assumed to be the reflection of the
%  control point on the previous command relative to the current
%  point. (If there is no previous command or if the previous command was
%  not a DrawPathCurveToQuadraticBezierAbsolute,
%  DrawPathCurveToQuadraticBezierRelative,
%  DrawPathCurveToQuadraticBezierSmoothAbsolut or
%  DrawPathCurveToQuadraticBezierSmoothRelative, assume the control point
%  is coincident with the current point.). At the end of the command, the
%  new current point becomes the final (x,y) coordinate pair used in the
%  polybezier.
%
%  The format of the DrawPathCurveToQuadraticBezierSmoothRelative method is:
%
%      void DrawPathCurveToQuadraticBezierSmoothRelative(
%        DrawingWand *wand,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: x ordinate of final point
%
%    o y: y ordinate of final point
%
%
*/
WandExport void DrawPathCurveToQuadraticBezierSmoothRelative(
  DrawingWand *wand,const double x,const double y)
{
  DrawPathCurveToQuadraticBezierSmooth(wand,RelativePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o S m o o t h A b s o l u t e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToSmoothAbsolute() draws a cubic Bezier curve from the
%  current point to (x,y) using absolute coordinates. The first control
%  point is assumed to be the reflection of the second control point on
%  the previous command relative to the current point. (If there is no
%  previous command or if the previous command was not an
%  DrawPathCurveToAbsolute, DrawPathCurveToRelative,
%  DrawPathCurveToSmoothAbsolute or DrawPathCurveToSmoothRelative, assume
%  the first control point is coincident with the current point.) (x2,y2)
%  is the second control point (i.e., the control point at the end of the
%  curve). At the end of the command, the new current point becomes the
%  final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToSmoothAbsolute method is:
%
%      void DrawPathCurveToSmoothAbsolute(DrawingWand *wand,
%        const double x2const double y2,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x2: x ordinate of second control point
%
%    o y2: y ordinate of second control point
%
%    o x: x ordinate of termination point
%
%    o y: y ordinate of termination point
%
%
*/
static void DrawPathCurveToSmooth(DrawingWand *wand,const PathMode mode,
  const double x2,const double y2,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathCurveToSmoothOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathCurveToSmoothOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g,%g %g,%g",mode == AbsolutePathMode ?
        'S' : 's',x2,y2,x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g %g,%g",x2,y2,x,y);
}

WandExport void DrawPathCurveToSmoothAbsolute(DrawingWand *wand,
  const double x2,const double y2,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveToSmooth(wand,AbsolutePathMode,x2,y2,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h C u r v e T o S m o o t h R e l a t i v e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathCurveToSmoothRelative() draws a cubic Bezier curve from the
%  current point to (x,y) using relative coordinates. The first control
%  point is assumed to be the reflection of the second control point on
%  the previous command relative to the current point. (If there is no
%  previous command or if the previous command was not an
%  DrawPathCurveToAbsolute, DrawPathCurveToRelative,
%  DrawPathCurveToSmoothAbsolute or DrawPathCurveToSmoothRelative, assume
%  the first control point is coincident with the current point.) (x2,y2)
%  is the second control point (i.e., the control point at the end of the
%  curve). At the end of the command, the new current point becomes the
%  final (x,y) coordinate pair used in the polybezier.
%
%  The format of the DrawPathCurveToSmoothRelative method is:
%
%      void DrawPathCurveToSmoothRelative(DrawingWand *wand,
%        const double x2,const double y2,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x2: x ordinate of second control point
%
%    o y2: y ordinate of second control point
%
%    o x: x ordinate of termination point
%
%    o y: y ordinate of termination point
%
%
*/
WandExport void DrawPathCurveToSmoothRelative(DrawingWand *wand,
  const double x2,const double y2,const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathCurveToSmooth(wand,RelativePathMode,x2,y2,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h E l l i p t i c A r c A b s o l u t e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathEllipticArcAbsolute() draws an elliptical arc from the current
%  point to (x, y) using absolute coordinates. The size and orientation
%  of the ellipse are defined by two radii (rx, ry) and an
%  xAxisRotation, which indicates how the ellipse as a whole is rotated
%  relative to the current coordinate system. The center (cx, cy) of the
%  ellipse is calculated automatically to satisfy the constraints imposed
%  by the other parameters. largeArcFlag and sweepFlag contribute to the
%  automatic calculations and help determine how the arc is drawn. If
%  largeArcFlag is true then draw the larger of the available arcs. If
%  sweepFlag is true, then draw the arc matching a clock-wise rotation.
%
%  The format of the DrawPathEllipticArcAbsolute method is:
%
%      void DrawPathEllipticArcAbsolute(DrawingWand *wand,
%        const double rx,const double ry,const double x_axis_rotation,
%        const MagickBooleanType large_arc_flag,
%        const MagickBooleanType sweep_flag,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o rx: x radius
%
%    o ry: y radius
%
%    o x_axis_rotation: indicates how the ellipse as a whole is rotated
%                       relative to the current coordinate system
%
%    o large_arc_flag: If non-zero (true) then draw the larger of the
%                      available arcs
%
%    o sweep_flag: If non-zero (true) then draw the arc matching a
%                  clock-wise rotation
%
%
*/

static void DrawPathEllipticArc(DrawingWand *wand, const PathMode mode,
  const double rx,const double ry,const double x_axis_rotation,
  const MagickBooleanType large_arc_flag,const MagickBooleanType sweep_flag,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathEllipticArcOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathEllipticArcOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand, "%c%g,%g %g %u %u %g,%g",
        mode == AbsolutePathMode ? 'A' : 'a',rx,ry,x_axis_rotation,
        large_arc_flag,sweep_flag,x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g %g %u %u %g,%g",rx,ry,x_axis_rotation,
      large_arc_flag,sweep_flag,x,y);
}

WandExport void DrawPathEllipticArcAbsolute(DrawingWand *wand,
  const double rx,const double ry,const double x_axis_rotation,
  const MagickBooleanType large_arc_flag,const MagickBooleanType sweep_flag,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathEllipticArc(wand,AbsolutePathMode,rx,ry,x_axis_rotation,
    large_arc_flag,sweep_flag,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h E l l i p t i c A r c R e l a t i v e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathEllipticArcRelative() draws an elliptical arc from the current
%  point to (x, y) using relative coordinates. The size and orientation
%  of the ellipse are defined by two radii (rx, ry) and an
%  xAxisRotation, which indicates how the ellipse as a whole is rotated
%  relative to the current coordinate system. The center (cx, cy) of the
%  ellipse is calculated automatically to satisfy the constraints imposed
%  by the other parameters. largeArcFlag and sweepFlag contribute to the
%  automatic calculations and help determine how the arc is drawn. If
%  largeArcFlag is true then draw the larger of the available arcs. If
%  sweepFlag is true, then draw the arc matching a clock-wise rotation.
%
%  The format of the DrawPathEllipticArcRelative method is:
%
%      void DrawPathEllipticArcRelative(DrawingWand *wand,
%        const double rx,const double ry,const double x_axis_rotation,
%        const MagickBooleanType large_arc_flag,
%        const MagickBooleanType sweep_flag,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o rx: x radius
%
%    o ry: y radius
%
%    o x_axis_rotation: indicates how the ellipse as a whole is rotated
%                       relative to the current coordinate system
%
%    o large_arc_flag: If non-zero (true) then draw the larger of the
%                      available arcs
%
%    o sweep_flag: If non-zero (true) then draw the arc matching a
%                  clock-wise rotation
%
*/
WandExport void DrawPathEllipticArcRelative(DrawingWand *wand,
  const double rx,const double ry,const double x_axis_rotation,
  const MagickBooleanType large_arc_flag,const MagickBooleanType sweep_flag,
  const double x,const double y)
{
  DrawPathEllipticArc(wand,RelativePathMode,rx,ry,x_axis_rotation,
    large_arc_flag,sweep_flag,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h F i n i s h                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathFinish() terminates the current path.
%
%  The format of the DrawPathFinish method is:
%
%      void DrawPathFinish(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPathFinish(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"'\n");
  wand->path_operation=PathDefaultOperation;
  wand->path_mode=DefaultPathMode;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o A b s o l u t e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToAbsolute() draws a line path from the current point to the
%  given coordinate using absolute coordinates. The coordinate then becomes
%  the new current point.
%
%  The format of the DrawPathLineToAbsolute method is:
%
%      void DrawPathLineToAbsolute(DrawingWand *wand,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
%    o y: target y ordinate
%
*/
static void DrawPathLineTo(DrawingWand *wand,const PathMode mode,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathLineToOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathLineToOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g,%g",mode == AbsolutePathMode ?
        'L' : 'l',x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g",x,y);
}

WandExport void DrawPathLineToAbsolute(DrawingWand *wand,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathLineTo(wand,AbsolutePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o R e l a t i v e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToRelative() draws a line path from the current point to the
%  given coordinate using relative coordinates. The coordinate then becomes
%  the new current point.
%
%  The format of the DrawPathLineToRelative method is:
%
%      void DrawPathLineToRelative(DrawingWand *wand,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
%    o y: target y ordinate
%
*/
WandExport void DrawPathLineToRelative(DrawingWand *wand,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathLineTo(wand,RelativePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o H o r i z o n t a l A b s o l u t e           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToHorizontalAbsolute() draws a horizontal line path from the
%  current point to the target point using absolute coordinates.  The target
%  point then becomes the new current point.
%
%  The format of the DrawPathLineToHorizontalAbsolute method is:
%
%      void DrawPathLineToHorizontalAbsolute(DrawingWand *wand,
%        const PathMode mode,const double x)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
*/

static void DrawPathLineToHorizontal(DrawingWand *wand,
  const PathMode mode,const double x)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathLineToHorizontalOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathLineToHorizontalOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g",mode == AbsolutePathMode ?
        'H' : 'h',x);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g",x);
}

WandExport void DrawPathLineToHorizontalAbsolute(DrawingWand *wand,
  const double x)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathLineToHorizontal(wand,AbsolutePathMode,x);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o H o r i z o n t a l R e l a t i v e           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToHorizontalRelative() draws a horizontal line path from the
%  current point to the target point using relative coordinates.  The target
%  point then becomes the new current point.
%
%  The format of the DrawPathLineToHorizontalRelative method is:
%
%      void DrawPathLineToHorizontalRelative(DrawingWand *wand,
%        const double x)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
*/
WandExport void DrawPathLineToHorizontalRelative(DrawingWand *wand,
  const double x)
{
  DrawPathLineToHorizontal(wand,RelativePathMode,x);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o V e r t i c a l A b s o l u t e               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToVerticalAbsolute() draws a vertical line path from the
%  current point to the target point using absolute coordinates.  The target
%  point then becomes the new current point.
%
%  The format of the DrawPathLineToVerticalAbsolute method is:
%
%      void DrawPathLineToVerticalAbsolute(DrawingWand *wand,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o y: target y ordinate
%
*/

static void DrawPathLineToVertical(DrawingWand *wand,
  const PathMode mode,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathLineToVerticalOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathLineToVerticalOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g",mode == AbsolutePathMode ?
        'V' : 'v',y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g",y);
}

WandExport void DrawPathLineToVerticalAbsolute(DrawingWand *wand,
  const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathLineToVertical(wand,AbsolutePathMode,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h L i n e T o V e r t i c a l R e l a t i v e               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathLineToVerticalRelative() draws a vertical line path from the
%  current point to the target point using relative coordinates.  The target
%  point then becomes the new current point.
%
%  The format of the DrawPathLineToVerticalRelative method is:
%
%      void DrawPathLineToVerticalRelative(DrawingWand *wand,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o y: target y ordinate
%
*/
WandExport void DrawPathLineToVerticalRelative(DrawingWand *wand,
  const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathLineToVertical(wand,RelativePathMode,y);
}
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h M o v e T o A b s o l u t e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathMoveToAbsolute() starts a new sub-path at the given coordinate
%  using absolute coordinates. The current point then becomes the
%  specified coordinate.
%
%  The format of the DrawPathMoveToAbsolute method is:
%
%      void DrawPathMoveToAbsolute(DrawingWand *wand,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
%    o y: target y ordinate
%
*/
static void DrawPathMoveTo(DrawingWand *wand,const PathMode mode,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->path_operation != PathMoveToOperation) ||
      (wand->path_mode != mode))
    {
      wand->path_operation=PathMoveToOperation;
      wand->path_mode=mode;
      (void) MvgAutoWrapPrintf(wand,"%c%g,%g",mode == AbsolutePathMode ?
        'M' : 'm',x,y);
    }
  else
    (void) MvgAutoWrapPrintf(wand," %g,%g",x,y);
}

WandExport void DrawPathMoveToAbsolute(DrawingWand *wand,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathMoveTo(wand,AbsolutePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h M o v e T o R e l a t i v e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathMoveToRelative() starts a new sub-path at the given coordinate
%  using relative coordinates. The current point then becomes the
%  specified coordinate.
%
%  The format of the DrawPathMoveToRelative method is:
%
%      void DrawPathMoveToRelative(DrawingWand *wand,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x ordinate
%
%    o y: target y ordinate
%
*/
WandExport void DrawPathMoveToRelative(DrawingWand *wand,
  const double x,const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  DrawPathMoveTo(wand,RelativePathMode,x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P a t h S t a r t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPathStart() declares the start of a path drawing list which is terminated
%  by a matching DrawPathFinish() command. All other DrawPath commands must
%  be enclosed between a DrawPathStart() and a DrawPathFinish() command. This
%  is because path drawing commands are subordinate commands and they do not
%  function by themselves.
%
%  The format of the DrawPathStart method is:
%
%      void DrawPathStart(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPathStart(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"path '");
  wand->path_operation=PathDefaultOperation;
  wand->path_mode=DefaultPathMode;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o i n t                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPoint() draws a point using the current stroke color and stroke
%  thickness at the specified coordinates.
%
%  The format of the DrawPoint method is:
%
%      void DrawPoint(DrawingWand *wand,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: target x coordinate
%
%    o y: target y coordinate
%
*/
WandExport void DrawPoint(DrawingWand *wand,const double x,
  const double y)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"point %g,%g\n",x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o l y g o n                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPolygon() draws a polygon using the current stroke, stroke width, and
%  fill color or texture, using the specified array of coordinates.
%
%  The format of the DrawPolygon method is:
%
%      void DrawPolygon(DrawingWand *wand,
%        const unsigned long number_coordinates,const PointInfo *coordinates)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o number_coordinates: number of coordinates
%
%    o coordinates: coordinate array
%
*/
WandExport void DrawPolygon(DrawingWand *wand,
  const unsigned long number_coordinates,const PointInfo *coordinates)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  MvgAppendPointsCommand(wand,"polygon",number_coordinates,coordinates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o l y l i n e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPolyline() draws a polyline using the current stroke, stroke width, and
%  fill color or texture, using the specified array of coordinates.
%
%  The format of the DrawPolyline method is:
%
%      void DrawPolyline(DrawingWand *wand,
%        const unsigned long number_coordinates,const PointInfo *coordinates)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o number_coordinates: number of coordinates
%
%    o coordinates: coordinate array
%
*/
WandExport void DrawPolyline(DrawingWand *wand,
  const unsigned long number_coordinates,const PointInfo *coordinates)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  MvgAppendPointsCommand(wand,"polyline",number_coordinates,coordinates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o p C l i p P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPopClipPath() terminates a clip path definition.
%
%  The format of the DrawPopClipPath method is:
%
%      void DrawPopClipPath(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPopClipPath(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->indent_depth > 0)
    wand->indent_depth--;
  (void) MvgPrintf(wand,"pop clip-path\n");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o p D e f s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPopDefs() terminates a definition list
%
%  The format of the DrawPopDefs method is:
%
%      void DrawPopDefs(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPopDefs(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->indent_depth > 0)
    wand->indent_depth--;
  (void) MvgPrintf(wand,"pop defs\n");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P o p P a t t e r n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPopPattern() terminates a pattern definition.
%
%  The format of the DrawPopPattern method is:
%
%      MagickBooleanType DrawPopPattern(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport MagickBooleanType DrawPopPattern(DrawingWand *wand)
{
  char
    geometry[MaxTextExtent],
    key[MaxTextExtent];

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->image == (Image *) NULL)
    ThrowDrawException(WandError,"ContainsNoImages",wand->name);
  if (wand->pattern_id == (const char *) NULL)
    {
      ThrowDrawException(DrawWarning,"NotCurrentlyPushingPatternDefinition",
        wand->name);
      return(MagickFalse);
    }
  (void) FormatMagickString(key,MaxTextExtent,"[%s]",wand->pattern_id);
  (void) SetImageAttribute(wand->image,key,wand->mvg+
    wand->pattern_offset);
  (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
    wand->pattern_bounds.width,wand->pattern_bounds.height,
    wand->pattern_bounds.x,wand->pattern_bounds.y);
  (void) SetImageAttribute(wand->image,key,geometry);
  wand->pattern_id=(char *) RelinquishMagickMemory(wand->pattern_id);
  wand->pattern_offset=0;
  wand->pattern_bounds.x=0;
  wand->pattern_bounds.y=0;
  wand->pattern_bounds.width=0;
  wand->pattern_bounds.height=0;
  wand->filter_off=MagickFalse;
  if (wand->indent_depth > 0)
    wand->indent_depth--;
  (void) MvgPrintf(wand,"pop pattern\n");
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P u s h C l i p P a t h                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPushClipPath() starts a clip path definition which is comprized of
%  any number of drawing commands and terminated by a DrawPopClipPath()
%  command.
%
%  The format of the DrawPushClipPath method is:
%
%      void DrawPushClipPath(DrawingWand *wand,const char *clip_path_id)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o clip_path_id: string identifier to associate with the clip path for
%      later use.
%
*/
WandExport void DrawPushClipPath(DrawingWand *wand,
  const char *clip_path_id)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(clip_path_id != (const char *) NULL);
  (void) MvgPrintf(wand,"push clip-path %s\n",clip_path_id);
  wand->indent_depth++;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P u s h D e f s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPushDefs() indicates that commands up to a terminating DrawPopDefs()
%  command create named elements (e.g. clip-paths, textures, etc.) which
%  may safely be processed earlier for the sake of efficiency.
%
%  The format of the DrawPushDefs method is:
%
%      void DrawPushDefs(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawPushDefs(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"push defs\n");
  wand->indent_depth++;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w P u s h P a t t e r n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawPushPattern() indicates that subsequent commands up to a
%  DrawPopPattern() command comprise the definition of a named pattern.
%  The pattern space is assigned top left corner coordinates, a width
%  and height, and becomes its own drawing space.  Anything which can
%  be drawn may be used in a pattern definition.
%  Named patterns may be used as stroke or brush definitions.
%
%  The format of the DrawPushPattern method is:
%
%      MagickBooleanType DrawPushPattern(DrawingWand *wand,
%        const char *pattern_id,const double x,const double y,
%        const double width,const double height)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o pattern_id: pattern identification for later reference
%
%    o x: x ordinate of top left corner
%
%    o y: y ordinate of top left corner
%
%    o width: width of pattern space
%
%    o height: height of pattern space
%
*/
WandExport MagickBooleanType DrawPushPattern(DrawingWand *wand,
  const char *pattern_id,const double x,const double y,const double width,
  const double height)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(pattern_id != (const char *) NULL);
  if (wand->pattern_id != NULL)
    {
      ThrowDrawException(DrawError,"AlreadyPushingPatternDefinition",
        wand->pattern_id);
      return(MagickFalse);
    }
  wand->filter_off=MagickTrue;
  (void) MvgPrintf(wand,"push pattern %s %g,%g %g,%g\n",pattern_id,x,y,
    width,height);
  wand->indent_depth++;
  wand->pattern_id=AcquireString(pattern_id);
  wand->pattern_bounds.x=(long) ceil(x-0.5);
  wand->pattern_bounds.y=(long) ceil(y-0.5);
  wand->pattern_bounds.width=(unsigned long) (width+0.5);
  wand->pattern_bounds.height=(unsigned long) (height+0.5);
  wand->pattern_offset=wand->mvg_length;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w R e c t a n g l e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawRectangle() draws a rectangle given two coordinates and using
%  the current stroke, stroke width, and fill settings.
%
%  The format of the DrawRectangle method is:
%
%      void DrawRectangle(DrawingWand *wand,const double x1,
%        const double y1,const double x2,const double y2)
%
%  A description of each parameter follows:
%
%    o x1: x ordinate of first coordinate
%
%    o y1: y ordinate of first coordinate
%
%    o x2: x ordinate of second coordinate
%
%    o y2: y ordinate of second coordinate
%
*/
WandExport void DrawRectangle(DrawingWand *wand,const double x1,
  const double y1,const double x2,const double y2)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"rectangle %g,%g %g,%g\n",x1,y1,x2,y2);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w R e n d e r                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawRender() renders all preceding drawing commands onto the image.
%
%  The format of the DrawRender method is:
%
%      MagickBooleanType DrawRender(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport MagickBooleanType DrawRender(DrawingWand *wand)
{
  MagickBooleanType
    status;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  CurrentContext->primitive=wand->mvg;
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"MVG:\n'%s'\n",wand->mvg);
  if (wand->image == (Image *) NULL)
    ThrowDrawException(WandError,"ContainsNoImages",wand->name);
  status=DrawImage(wand->image,CurrentContext);
  InheritException(&wand->exception,&wand->image->exception);
  CurrentContext->primitive=(char *) NULL;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w R e s e t V e c t o r G r a p h i c s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawResetVectorGraphics() resets the vector graphics associated with the
%  specified wand.
%
%  The format of the DrawResetVectorGraphics method is:
%
%      void DrawResetVectorGraphics(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport void DrawResetVectorGraphics(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->mvg != (char *) NULL)
    wand->mvg=(char *) RelinquishMagickMemory(wand->mvg);
  wand->mvg_alloc=0;
  wand->mvg_length=0;
  wand->mvg_width=0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w R o t a t e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawRotate() applies the specified rotation to the current coordinate space.
%
%  The format of the DrawRotate method is:
%
%      void DrawRotate(DrawingWand *wand,const double degrees)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o degrees: degrees of rotation
%
*/
WandExport void DrawRotate(DrawingWand *wand,const double degrees)
{
  AffineMatrix
    affine;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  GetAffineMatrix(&affine);
  affine.sx=cos(DegreesToRadians(fmod(degrees,360.0)));
  affine.rx=sin(DegreesToRadians(fmod(degrees,360.0)));
  affine.ry=(-sin(DegreesToRadians(fmod(degrees,360.0))));
  affine.sy=cos(DegreesToRadians(fmod(degrees,360.0)));
  AdjustAffine(wand,&affine);
  (void) MvgPrintf(wand,"rotate %g\n",degrees);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w R o u n d R e c t a n g l e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawRoundRectangle() draws a rounted rectangle given two coordinates,
%  x & y corner radiuses and using the current stroke, stroke width,
%  and fill settings.
%
%  The format of the DrawRoundRectangle method is:
%
%      void DrawRoundRectangle(DrawingWand *wand,double x1,double y1,
%        double x2,double y2,double rx,double ry)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: x ordinate of first coordinate
%
%    o y1: y ordinate of first coordinate
%
%    o x2: x ordinate of second coordinate
%
%    o y2: y ordinate of second coordinate
%
%    o rx: radius of corner in horizontal direction
%
%    o ry: radius of corner in vertical direction
%
*/
WandExport void DrawRoundRectangle(DrawingWand *wand,double x1,
  double y1,double x2,double y2,double rx,double ry)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"roundrectangle %g,%g %g,%g %g,%g\n",x1,y1,x2,y2,rx,ry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S c a l e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawScale() adjusts the scaling factor to apply in the horizontal and
%  vertical directions to the current coordinate space.
%
%  The format of the DrawScale method is:
%
%      void DrawScale(DrawingWand *wand,const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: horizontal scale factor
%
%    o y: vertical scale factor
%
*/
WandExport void DrawScale(DrawingWand *wand,const double x,
  const double y)
{
  AffineMatrix
    affine;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  GetAffineMatrix(&affine);
  affine.sx=x;
  affine.sy=y;
  AdjustAffine(wand,&affine);
  (void) MvgPrintf(wand,"scale %g,%g\n",x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t C l i p P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetClipPath() associates a named clipping path with the image.  Only
%  the areas drawn on by the clipping path will be modified as long as it
%  remains in effect.
%
%  The format of the DrawSetClipPath method is:
%
%      MagickBooleanType DrawSetClipPath(DrawingWand *wand,
%        const char *clip_path)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o clip_path: name of clipping path to associate with image
%
*/
WandExport MagickBooleanType DrawSetClipPath(DrawingWand *wand,
  const char *clip_path)
{
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),clip_path);
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  assert(clip_path != (const char *) NULL);
  if ((CurrentContext->clip_path == (const char *) NULL) ||
      (wand->filter_off != MagickFalse) ||
      (LocaleCompare(CurrentContext->clip_path,clip_path) != 0))
    {
      (void) CloneString(&CurrentContext->clip_path,clip_path);
#if DRAW_BINARY_IMPLEMENTATION
      if (wand->image == (Image *) NULL)
        ThrowDrawException(WandError,"ContainsNoImages",wand->name);
      (void) DrawClipPath(wand->image,CurrentContext,CurrentContext->clip_path);
#endif
      (void) MvgPrintf(wand,"clip-path url(#%s)\n",clip_path);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t C l i p R u l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetClipRule() set the polygon fill rule to be used by the clipping path.
%
%  The format of the DrawSetClipRule method is:
%
%      void DrawSetClipRule(DrawingWand *wand,const FillRule fill_rule)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_rule: fill rule (EvenOddRule or NonZeroRule)
%
*/
WandExport void DrawSetClipRule(DrawingWand *wand,
  const FillRule fill_rule)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->fill_rule != fill_rule))
    {
      CurrentContext->fill_rule=fill_rule;
      (void) MvgPrintf(wand, "clip-rule '%s'\n",MagickOptionToMnemonic(
        MagickFillRuleOptions,(long) fill_rule));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t C l i p U n i t s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetClipUnits() sets the interpretation of clip path units.
%
%  The format of the DrawSetClipUnits method is:
%
%      void DrawSetClipUnits(DrawingWand *wand,
%        const ClipPathUnits clip_units)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o clip_units: units to use (UserSpace, UserSpaceOnUse, or ObjectBoundingBox)
%
*/
WandExport void DrawSetClipUnits(DrawingWand *wand,
  const ClipPathUnits clip_units)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->clip_units != clip_units))
    {
      CurrentContext->clip_units=clip_units;
      if (clip_units == ObjectBoundingBox)
        {
          AffineMatrix
            affine;

          GetAffineMatrix(&affine);
          affine.sx=CurrentContext->bounds.x2;
          affine.sy=CurrentContext->bounds.y2;
          affine.tx=CurrentContext->bounds.x1;
          affine.ty=CurrentContext->bounds.y1;
          AdjustAffine(wand,&affine);
        }
      (void) MvgPrintf(wand, "clip-units '%s'\n",MagickOptionToMnemonic(
        MagickClipPathOptions,(long) clip_units));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F i l l C o l o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFillColor() sets the fill color to be used for drawing filled objects.
%
%  The format of the DrawSetFillColor method is:
%
%      void DrawSetFillColor(DrawingWand *wand,const PixelWand *fill_wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_wand: fill wand.
%
*/

static inline MagickBooleanType IsColorEqual(const PixelPacket *p,
  const PixelPacket *q)
{
  if (p->red != q->red)
    return(MagickFalse);
  if (p->green != q->green)
    return(MagickFalse);
  if (p->blue != q->blue)
    return(MagickFalse);
  return(MagickTrue);
}

WandExport void DrawSetFillColor(DrawingWand *wand,const PixelWand *fill_wand)
{
  PixelPacket
    *current_fill,
    fill_color,
    new_fill;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(fill_wand != (const PixelWand *) NULL);
  PixelGetQuantumColor(fill_wand,&fill_color);
  new_fill=fill_color;
  current_fill=(&CurrentContext->fill);
  if ((wand->filter_off != MagickFalse) ||
      (IsColorEqual(current_fill,&new_fill) == MagickFalse))
    {
      CurrentContext->fill=new_fill;
      (void) MvgPrintf(wand,"fill '");
      MvgAppendColor(wand,&fill_color);
      (void) MvgPrintf(wand,"'\n");
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F i l l O p a c i t y                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFillAlpha() sets the opacity to use when drawing using the fill
%  color or fill texture.  Fully opaque is 1.0.
%
%  The format of the DrawSetFillAlpha method is:
%
%      void DrawSetFillAlpha(DrawingWand *wand,const double fill_opacity)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_opacity: fill opacity
%
*/

WandExport void DrawSetFillAlpha(DrawingWand *wand,const double fill_opacity)
{
  DrawSetFillOpacity(wand,fill_opacity);
}

WandExport void DrawSetFillOpacity(DrawingWand *wand,const double fill_opacity)
{
  Quantum
    opacity;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  opacity=RoundToQuantum((double) QuantumRange*(1.0-fill_opacity));
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->fill.opacity != opacity))
    {
      CurrentContext->fill.opacity=opacity;
      (void) MvgPrintf(wand,"fill-opacity %g\n",fill_opacity);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F i l l P a t t e r n U R L                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFillPatternURL() sets the URL to use as a fill pattern for filling
%  objects. Only local URLs ("#identifier") are supported at this time. These
%  local URLs are normally created by defining a named fill pattern with
%  DrawPushPattern/DrawPopPattern.
%
%  The format of the DrawSetFillPatternURL method is:
%
%      MagickBooleanType DrawSetFillPatternURL(DrawingWand *wand,
%        const char *fill_url)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_url: URL to use to obtain fill pattern.
%
*/
WandExport MagickBooleanType DrawSetFillPatternURL(DrawingWand *wand,
  const char *fill_url)
{
  char
    pattern[MaxTextExtent],
    pattern_spec[MaxTextExtent];

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),fill_url);
  if (wand->image == (Image *) NULL)
    ThrowDrawException(WandError,"ContainsNoImages",wand->name);
  assert(fill_url != (const char *) NULL);
  if (*fill_url != '#')
    {
      ThrowDrawException(DrawWarning,"NotARelativeuRL",fill_url);
      return(MagickFalse);
    }
  (void) FormatMagickString(pattern,MaxTextExtent,"[%s]",fill_url+1);
  if (GetImageAttribute(wand->image,pattern) == (ImageAttribute *) NULL)
    {
      ThrowDrawException(DrawWarning,"URLNotFound",fill_url)
      return(MagickFalse);
    }
  (void) FormatMagickString(pattern_spec,MaxTextExtent,"url(%s)",fill_url);
#if DRAW_BINARY_IMPLEMENTATION
  DrawPatternPath(wand->image,CurrentContext,pattern_spec,
    &CurrentContext->fill_pattern);
#endif
  if (CurrentContext->fill.opacity != TransparentOpacity)
    CurrentContext->fill.opacity=CurrentContext->opacity;
  (void) MvgPrintf(wand,"fill %s\n",pattern_spec);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F i l l R u l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFillRule() sets the fill rule to use while drawing polygons.
%
%  The format of the DrawSetFillRule method is:
%
%      void DrawSetFillRule(DrawingWand *wand,const FillRule fill_rule)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o fill_rule: fill rule (EvenOddRule or NonZeroRule)
%
*/
WandExport void DrawSetFillRule(DrawingWand *wand,
  const FillRule fill_rule)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->fill_rule != fill_rule))
    {
      CurrentContext->fill_rule=fill_rule;
      (void) MvgPrintf(wand, "fill-rule '%s'\n",MagickOptionToMnemonic(
        MagickStretchOptions,(long) fill_rule));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFont() sets the fully-sepecified font to use when annotating with
%  text.
%
%  The format of the DrawSetFont method is:
%
%      MagickBooleanType DrawSetFont(DrawingWand *wand,const char *font_name)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o font_name: font name
%
*/
WandExport MagickBooleanType DrawSetFont(DrawingWand *wand,
  const char *font_name)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(font_name != (const char *) NULL);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->font == (char *) NULL) ||
      (LocaleCompare(CurrentContext->font,font_name) != 0))
    {
      (void) CloneString(&CurrentContext->font,font_name);
      (void) MvgPrintf(wand,"font '%s'\n",font_name);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t F a m i l y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFontFamily() sets the font family to use when annotating with text.
%
%  The format of the DrawSetFontFamily method is:
%
%      MagickBooleanType DrawSetFontFamily(DrawingWand *wand,
%        const char *font_family)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o font_family: font family
%
*/
WandExport MagickBooleanType DrawSetFontFamily(DrawingWand *wand,
  const char *font_family)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(font_family != (const char *) NULL);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->family == (const char *) NULL) ||
      (LocaleCompare(CurrentContext->family,font_family) != 0))
    {
      (void) CloneString(&CurrentContext->family,font_family);
      (void) MvgPrintf(wand,"font-family '%s'\n",font_family);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t S i z e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFontSize() sets the font pointsize to use when annotating with text.
%
%  The format of the DrawSetFontSize method is:
%
%      void DrawSetFontSize(DrawingWand *wand,const double pointsize)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o pointsize: text pointsize
%
*/
WandExport void DrawSetFontSize(DrawingWand *wand,const double pointsize)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
     (AbsoluteValue(CurrentContext->pointsize-pointsize) > MagickEpsilon))
    {
      CurrentContext->pointsize=pointsize;
      (void) MvgPrintf(wand,"font-size %g\n",pointsize);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t S t r e t c h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFontStretch() sets the font stretch to use when annotating with text.
%  The AnyStretch enumeration acts as a wild-card "don't care" option.
%
%  The format of the DrawSetFontStretch method is:
%
%      void DrawSetFontStretch(DrawingWand *wand,
%        const StretchType font_stretch)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o font_stretch: font stretch (NormalStretch, UltraCondensedStretch,
%                    CondensedStretch, SemiCondensedStretch,
%                    SemiExpandedStretch, ExpandedStretch,
%                    ExtraExpandedStretch, UltraExpandedStretch, AnyStretch)
%
*/
WandExport void DrawSetFontStretch(DrawingWand *wand,
  const StretchType font_stretch)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->stretch != font_stretch))
    {
      CurrentContext->stretch=font_stretch;
      (void) MvgPrintf(wand, "font-stretch '%s'\n",MagickOptionToMnemonic(
        MagickStretchOptions,(long) font_stretch));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t S t y l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFontStyle() sets the font style to use when annotating with text.
%  The AnyStyle enumeration acts as a wild-card "don't care" option.
%
%  The format of the DrawSetFontStyle method is:
%
%      void DrawSetFontStyle(DrawingWand *wand,const StyleType style)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o style: font style (NormalStyle, ItalicStyle, ObliqueStyle, AnyStyle)
%
*/
WandExport void DrawSetFontStyle(DrawingWand *wand,const StyleType style)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->style != style))
    {
      CurrentContext->style=style;
      (void) MvgPrintf(wand, "font-style '%s'\n",MagickOptionToMnemonic(
        MagickStyleOptions,(long) style));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t F o n t W e i g h t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetFontWeight() sets the font weight to use when annotating with text.
%
%  The format of the DrawSetFontWeight method is:
%
%      void DrawSetFontWeight(DrawingWand *wand,
%        const unsigned long font_weight)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o font_weight: font weight (valid range 100-900)
%
*/
WandExport void DrawSetFontWeight(DrawingWand *wand,
  const unsigned long font_weight)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->weight != font_weight))
    {
      CurrentContext->weight=font_weight;
      (void) MvgPrintf(wand,"font-weight %lu\n",font_weight);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t G r a v i t y                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetGravity() sets the text placement gravity to use when annotating
%  with text.
%
%  The format of the DrawSetGravity method is:
%
%      void DrawSetGravity(DrawingWand *wand,const GravityType gravity)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o gravity: positioning gravity (NorthWestGravity, NorthGravity,
%               NorthEastGravity, WestGravity, CenterGravity,
%               EastGravity, SouthWestGravity, SouthGravity,
%               SouthEastGravity)
%
*/
WandExport void DrawSetGravity(DrawingWand *wand,const GravityType gravity)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->gravity != gravity) || (gravity != ForgetGravity))
    {
      CurrentContext->gravity=gravity;
      (void) MvgPrintf(wand,"gravity '%s'\n",MagickOptionToMnemonic(
        MagickGravityOptions,(long) gravity));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e C o l o r                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeColor() sets the color used for stroking object outlines.
%
%  The format of the DrawSetStrokeColor method is:
%
%      void DrawSetStrokeColor(DrawingWand *wand,
%        const PixelWand *stroke_wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_wand: stroke wand.
%
*/
WandExport void DrawSetStrokeColor(DrawingWand *wand,
  const PixelWand *stroke_wand)
{
  PixelPacket
    *current_stroke,
    new_stroke,
    stroke_color;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(stroke_wand != (const PixelWand *) NULL);
  PixelGetQuantumColor(stroke_wand,&stroke_color);
  new_stroke=stroke_color;
  current_stroke=(&CurrentContext->stroke);
  if ((wand->filter_off != MagickFalse) ||
      (IsColorEqual(current_stroke,&new_stroke) == MagickFalse))
    {
      CurrentContext->stroke=new_stroke;
      (void) MvgPrintf(wand,"stroke '");
      MvgAppendColor(wand,&stroke_color);
      (void) MvgPrintf(wand,"'\n");
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e P a t t e r n U R L                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokePatternURL() sets the pattern used for stroking object outlines.
%
%  The format of the DrawSetStrokePatternURL method is:
%
%      MagickBooleanType DrawSetStrokePatternURL(DrawingWand *wand,
%        const char *stroke_url)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_url: URL specifying pattern ID (e.g. "#pattern_id")
%
*/
WandExport MagickBooleanType DrawSetStrokePatternURL(DrawingWand *wand,
  const char *stroke_url)
{
  char
    pattern[MaxTextExtent],
    pattern_spec[MaxTextExtent];

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->image == (Image *) NULL)
    ThrowDrawException(WandError,"ContainsNoImages",wand->name);
  assert(stroke_url != NULL);
  if (stroke_url[0] != '#')
    ThrowDrawException(OptionWarning,"NotARelativeURL",stroke_url);
  (void) FormatMagickString(pattern,MaxTextExtent,"[%s]",stroke_url+1);
  if (GetImageAttribute(wand->image,pattern) == (ImageAttribute *) NULL)
    {
      ThrowDrawException(OptionWarning,"URLNotFound",stroke_url)
      return(MagickFalse);
    }
  (void) FormatMagickString(pattern_spec,MaxTextExtent,"url(%s)",stroke_url);
#if DRAW_BINARY_IMPLEMENTATION
  DrawPatternPath(wand->image,CurrentContext,pattern_spec,
    &CurrentContext->stroke_pattern);
#endif
  if (CurrentContext->stroke.opacity != TransparentOpacity)
    CurrentContext->stroke.opacity=CurrentContext->opacity;
  (void) MvgPrintf(wand,"stroke %s\n",pattern_spec);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e A n t i a l i a s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeAntialias() controls whether stroked outlines are antialiased.
%  Stroked outlines are antialiased by default.  When antialiasing is disabled
%  stroked pixels are thresholded to determine if the stroke color or
%  underlying canvas color should be used.
%
%  The format of the DrawSetStrokeAntialias method is:
%
%      void DrawSetStrokeAntialias(DrawingWand *wand,
%        const MagickBooleanType stroke_antialias)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_antialias: set to false (zero) to disable antialiasing
%
*/
WandExport void DrawSetStrokeAntialias(DrawingWand *wand,
  const MagickBooleanType stroke_antialias)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->stroke_antialias != stroke_antialias))
    {
      CurrentContext->stroke_antialias=stroke_antialias;
      (void) MvgPrintf(wand,"stroke-antialias %i\n",stroke_antialias != 0 ?
        1 : 0);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e D a s h A r r a y                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeDashArray() specifies the pattern of dashes and gaps used to
%  stroke paths. The strokeDashArray represents an array of numbers that
%  specify the lengths of alternating dashes and gaps in pixels. If an odd
%  number of values is provided, then the list of values is repeated to yield
%  an even number of values. To remove an existing dash array, pass a zero
%  number_elements argument and null dash_array.
%  A typical strokeDashArray_ array might contain the members 5 3 2.
%
%  The format of the DrawSetStrokeDashArray method is:
%
%      MagickBooleanType DrawSetStrokeDashArray(DrawingWand *wand,
%        const unsigned long number_elements,const double *dash_array)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o number_elements: number of elements in dash array
%
%    o dash_array: dash array values
%
*/
WandExport MagickBooleanType DrawSetStrokeDashArray(DrawingWand *wand,
  const unsigned long number_elements,const double *dash_array)
{
  MagickBooleanType
    update;

  register const double
    *p;

  register double
    *q;

  register long
    i;

  unsigned long
    n_new,
    n_old;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  n_new=number_elements;
  n_old=0;
  update=MagickFalse;
  q=CurrentContext->dash_pattern;
  if (q != (const double *) NULL)
    while (*q++ != 0.0)
      n_old++;
  if ((n_old == 0) && (n_new == 0))
    update=MagickFalse;
  else
    if (n_old != n_new)
      update=MagickTrue;
    else
      if ((CurrentContext->dash_pattern != (double *) NULL) &&
          (dash_array != (double *) NULL))
        {
          p=dash_array;
          q=CurrentContext->dash_pattern;
          for (i=0; i < (long) n_new; i++)
          {
            if (AbsoluteValue((*p)-(*q)) > MagickEpsilon)
              {
                update=MagickTrue;
                break;
              }
            p++;
            q++;
          }
        }
  if ((wand->filter_off != MagickFalse) || (update != MagickFalse))
    {
      if (CurrentContext->dash_pattern != (double *) NULL)
        CurrentContext->dash_pattern=(double *)
          RelinquishMagickMemory(CurrentContext->dash_pattern);
      if (n_new != 0)
        {
          CurrentContext->dash_pattern=(double *) AcquireMagickMemory((size_t)
            (n_new+1)*sizeof(*CurrentContext->dash_pattern));
          if (!CurrentContext->dash_pattern)
            {
              ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
                wand->name);
              return(MagickFalse);
            }
          q=CurrentContext->dash_pattern;
          p=dash_array;
          for (i=0; i < (long) n_new; i++)
            *q++=(*p++);
          *q=0;
        }
      (void) MvgPrintf(wand,"stroke-dasharray ");
      if (n_new == 0)
        (void) MvgPrintf(wand,"none\n");
      else
        {
          p=dash_array;
          (void) MvgPrintf(wand,"%g",*p++);
          for (i=1; i < (long) n_new; i++)
            (void) MvgPrintf(wand,",%g",*p++);
          (void) MvgPrintf(wand,"\n");
        }
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e D a s h O f f s e t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeDashOffset() specifies the offset into the dash pattern to
%  start the dash.
%
%  The format of the DrawSetStrokeDashOffset method is:
%
%      void DrawSetStrokeDashOffset(DrawingWand *wand,
%        const double dash_offset)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o dash_offset: dash offset
%
*/
WandExport void DrawSetStrokeDashOffset(DrawingWand *wand,
  const double dash_offset)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
     (AbsoluteValue(CurrentContext->dash_offset-dash_offset) > MagickEpsilon))
    {
      CurrentContext->dash_offset=dash_offset;
      (void) MvgPrintf(wand,"stroke-dashoffset %g\n",dash_offset);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e L i n e C a p                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeLineCap() specifies the shape to be used at the end of
%  open subpaths when they are stroked. Values of LineCap are
%  UndefinedCap, ButtCap, RoundCap, and SquareCap.
%
%  The format of the DrawSetStrokeLineCap method is:
%
%      void DrawSetStrokeLineCap(DrawingWand *wand,
%        const LineCap linecap)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o linecap: linecap style
%
*/
WandExport void DrawSetStrokeLineCap(DrawingWand *wand,
  const LineCap linecap)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->linecap != linecap))
    {
      CurrentContext->linecap=linecap;
      (void) MvgPrintf(wand,"stroke-linecap '%s'\n",MagickOptionToMnemonic(
        MagickLineCapOptions,(long) linecap));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e L i n e J o i n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeLineJoin() specifies the shape to be used at the
%  corners of paths (or other vector shapes) when they are
%  stroked. Values of LineJoin are UndefinedJoin, MiterJoin, RoundJoin,
%  and BevelJoin.
%
%  The format of the DrawSetStrokeLineJoin method is:
%
%      void DrawSetStrokeLineJoin(DrawingWand *wand,
%        const LineJoin linejoin)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o linejoin: line join style
%
%
*/
WandExport void DrawSetStrokeLineJoin(DrawingWand *wand,
  const LineJoin linejoin)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->linejoin != linejoin))
    {
      CurrentContext->linejoin=linejoin;
      (void) MvgPrintf(wand, "stroke-linejoin '%s'\n",MagickOptionToMnemonic(
        MagickLineJoinOptions,(long) linejoin));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e M i t e r L i m i t                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeMiterLimit() specifies the miter limit. When two line
%  segments meet at a sharp angle and miter joins have been specified for
%  'lineJoin', it is possible for the miter to extend far beyond the
%  thickness of the line stroking the path. The miterLimit' imposes a
%  limit on the ratio of the miter length to the 'lineWidth'.
%
%  The format of the DrawSetStrokeMiterLimit method is:
%
%      void DrawSetStrokeMiterLimit(DrawingWand *wand,
%        const unsigned long miterlimit)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o miterlimit: miter limit
%
*/
WandExport void DrawSetStrokeMiterLimit(DrawingWand *wand,
  const unsigned long miterlimit)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (CurrentContext->miterlimit != miterlimit)
    {
      CurrentContext->miterlimit=miterlimit;
      (void) MvgPrintf(wand,"stroke-miterlimit %lu\n",miterlimit);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e O p a c i t y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeAlpha() specifies the opacity of stroked object outlines.
%
%  The format of the DrawSetStrokeAlpha method is:
%
%      void DrawSetStrokeAlpha(DrawingWand *wand,
%        const double stroke_opacity)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_opacity: stroke opacity.  The value 1.0 is opaque.
%
*/

WandExport void DrawSetStrokeAlpha(DrawingWand *wand,
  const double stroke_opacity)
{
  DrawSetStrokeOpacity(wand,stroke_opacity);
}

WandExport void DrawSetStrokeOpacity(DrawingWand *wand,
  const double stroke_opacity)
{
  Quantum
    opacity;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  opacity=RoundToQuantum((double) QuantumRange*(1.0-stroke_opacity));
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->stroke.opacity != opacity))
    {
      CurrentContext->stroke.opacity=opacity;
      (void) MvgPrintf(wand,"stroke-opacity %g\n",stroke_opacity);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t S t r o k e W i d t h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetStrokeWidth() sets the width of the stroke used to draw object
%  outlines.
%
%  The format of the DrawSetStrokeWidth method is:
%
%      void DrawSetStrokeWidth(DrawingWand *wand,
%        const double stroke_width)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o stroke_width: stroke width
%
*/
WandExport void DrawSetStrokeWidth(DrawingWand *wand,
  const double stroke_width)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
     (AbsoluteValue(CurrentContext->stroke_width-stroke_width) > MagickEpsilon))
    {
      CurrentContext->stroke_width=stroke_width;
      (void) MvgPrintf(wand,"stroke-width %g\n",stroke_width);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t T e x t A l i g n m e n t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetTextAlignment() specifies a text alignment to be applied when
%  annotating with text.
%
%  The format of the DrawSetTextAlignment method is:
%
%      void DrawSetTextAlignment(DrawingWand *wand,const AlignType alignment)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o alignment: text alignment.  One of UndefinedAlign, LeftAlign,
%      CenterAlign, or RightAlign.
%
*/
WandExport void DrawSetTextAlignment(DrawingWand *wand,
  const AlignType alignment)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->align != alignment))
    {
      CurrentContext->align=alignment;
      (void) MvgPrintf(wand,"text-align '%s'\n",MagickOptionToMnemonic(
        MagickAlignOptions,(long) alignment));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t T e x t A n t i a l i a s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetTextAntialias() controls whether text is antialiased.  Text is
%  antialiased by default.
%
%  The format of the DrawSetTextAntialias method is:
%
%      void DrawSetTextAntialias(DrawingWand *wand,
%        const MagickBooleanType text_antialias)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o text_antialias: antialias boolean. Set to false (0) to disable
%      antialiasing.
%
*/
WandExport void DrawSetTextAntialias(DrawingWand *wand,
  const MagickBooleanType text_antialias)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->text_antialias != text_antialias))
    {
      CurrentContext->text_antialias=text_antialias;
      (void) MvgPrintf(wand,"text-antialias %i\n",text_antialias != 0 ? 1 : 0);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t T e x t D e c o r a t i o n                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetTextDecoration() specifies a decoration to be applied when
%  annotating with text.
%
%  The format of the DrawSetTextDecoration method is:
%
%      void DrawSetTextDecoration(DrawingWand *wand,
%        const DecorationType decoration)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o decoration: text decoration.  One of NoDecoration, UnderlineDecoration,
%      OverlineDecoration, or LineThroughDecoration
%
*/
WandExport void DrawSetTextDecoration(DrawingWand *wand,
  const DecorationType decoration)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->decorate != decoration))
    {
      CurrentContext->decorate=decoration;
      (void) MvgPrintf(wand,"decorate '%s'\n",MagickOptionToMnemonic(
        MagickDecorationOptions,(long) decoration));
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t T e x t E n c o d i n g                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetTextEncoding() specifies specifies the code set to use for
%  text annotations. The only character encoding which may be specified
%  at this time is "UTF-8" for representing Unicode as a sequence of
%  bytes. Specify an empty string to set text encoding to the system's
%  default. Successful text annotation using Unicode may require fonts
%  designed to support Unicode.
%
%  The format of the DrawSetTextEncoding method is:
%
%      void DrawSetTextEncoding(DrawingWand *wand,const char *encoding)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o encoding: character string specifying text encoding
%
*/
WandExport void DrawSetTextEncoding(DrawingWand *wand,
  const char *encoding)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(encoding != (char *) NULL);
  if ((wand->filter_off != MagickFalse) ||
      (CurrentContext->encoding == (char *) NULL) ||
      (LocaleCompare(CurrentContext->encoding,encoding) != 0))
    {
      (void) CloneString(&CurrentContext->encoding,encoding);
      (void) MvgPrintf(wand,"encoding '%s'\n",encoding);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t T e x t U n d e r C o l o r                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetTextUnderColor() specifies the color of a background rectangle
%  to place under text annotations.
%
%  The format of the DrawSetTextUnderColor method is:
%
%      void DrawSetTextUnderColor(DrawingWand *wand,
%        const PixelWand *under_wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o under_wand: text under wand.
%
*/
WandExport void DrawSetTextUnderColor(DrawingWand *wand,
  const PixelWand *under_wand)
{
  PixelPacket
    under_color;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(under_wand != (const PixelWand *) NULL);
  PixelGetQuantumColor(under_wand,&under_color);
  if ((wand->filter_off != MagickFalse) ||
      (IsColorEqual(&CurrentContext->undercolor,&under_color) == MagickFalse))
    {
      CurrentContext->undercolor=under_color;
      (void) MvgPrintf(wand,"text-undercolor '");
      MvgAppendColor(wand,&under_color);
      (void) MvgPrintf(wand,"'\n");
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t V e c t o r G r a p h i c s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetVectorGraphics() sets the vector graphics associated with the
%  specified wand.  Use this method with DrawGetVectorGraphics() as a method
%  to persist the vector graphics state.
%
%  The format of the DrawSetVectorGraphics method is:
%
%      MagickBooleanType DrawSetVectorGraphics(DrawingWand *wand,
%        const char *xml)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o xml: The drawing wand XML.
%
*/

static inline MagickBooleanType IsPoint(const char *point)
{
  char
    *p;

  (void) strtol(point,&p,10);
  return(p != point ? MagickTrue : MagickFalse);
}

WandExport MagickBooleanType DrawSetVectorGraphics(DrawingWand *wand,
  const char *xml)
{
  const char
    *value;

  XMLTreeInfo
    *child,
    *xml_info;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  CurrentContext=DestroyDrawInfo(CurrentContext);
  CurrentContext=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  xml_info=NewXMLTree(xml,&wand->exception);
  if (xml_info == (XMLTreeInfo *) NULL)
    return(MagickFalse);
  child=GetXMLTreeChild(xml_info,"clip-path");
  if (child != (XMLTreeInfo *) NULL)
    (void) CloneString(&CurrentContext->clip_path,GetXMLTreeContent(child));
  child=GetXMLTreeChild(xml_info,"clip-units");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->clip_units=(ClipPathUnits) ParseMagickOption(
          MagickClipPathOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"decorate");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->decorate=(DecorationType) ParseMagickOption(
          MagickDecorationOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"encoding");
  if (child != (XMLTreeInfo *) NULL)
    (void) CloneString(&CurrentContext->encoding,GetXMLTreeContent(child));
  child=GetXMLTreeChild(xml_info,"fill");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        (void) QueryColorDatabase(value,&CurrentContext->fill,&wand->exception);
    }
  child=GetXMLTreeChild(xml_info,"fill-opacity");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->fill.opacity=RoundToQuantum(QuantumRange*(1.0-
          atof(value)));
    }
  child=GetXMLTreeChild(xml_info,"fill-rule");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->fill_rule=(FillRule) ParseMagickOption(
          MagickFillRuleOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"font");
  if (child != (XMLTreeInfo *) NULL)
    (void) CloneString(&CurrentContext->font,GetXMLTreeContent(child));
  child=GetXMLTreeChild(xml_info,"font-family");
  if (child != (XMLTreeInfo *) NULL)
    (void) CloneString(&CurrentContext->family,GetXMLTreeContent(child));
  child=GetXMLTreeChild(xml_info,"font-size");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->pointsize=atof(value);
    }
  child=GetXMLTreeChild(xml_info,"font-stretch");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->stretch=(StretchType) ParseMagickOption(
          MagickStretchOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"font-style");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->style=(StyleType) ParseMagickOption(MagickStyleOptions,
          MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"font-weight");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->weight=(unsigned long) atol(value);
    }
  child=GetXMLTreeChild(xml_info,"gravity");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->gravity=(GravityType) ParseMagickOption(
          MagickGravityOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"stroke");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        (void) QueryColorDatabase(value,&CurrentContext->stroke,
          &wand->exception);
    }
  child=GetXMLTreeChild(xml_info,"stroke-antialias");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->stroke_antialias=atol(value) != 0 ? MagickTrue :
          MagickFalse;
    }
  child=GetXMLTreeChild(xml_info,"stroke-dasharray");
  if (child != (XMLTreeInfo *) NULL)
    {
      char
        *q,
        token[MaxTextExtent];

      long
        j;

      register long
        x;

      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        {
          if (CurrentContext->dash_pattern != (double *) NULL)
            CurrentContext->dash_pattern=(double *) RelinquishMagickMemory(
              CurrentContext->dash_pattern);
          q=(char *) value;
          if (IsPoint(q) != MagickFalse)
            {
              char
                *p;

              p=q;
              GetMagickToken(p,&p,token);
              if (*token == ',')
                GetMagickToken(p,&p,token);
              for (x=0; IsPoint(token) != MagickFalse; x++)
              {
                GetMagickToken(p,&p,token);
                if (*token == ',')
                  GetMagickToken(p,&p,token);
              }
              CurrentContext->dash_pattern=(double *) AcquireMagickMemory(
                (size_t) (2*x+1)*sizeof(*CurrentContext->dash_pattern));
              if (CurrentContext->dash_pattern == (double *) NULL)
                {
                  char
                    *message;

                  message=GetExceptionMessage(errno);
                  ThrowWandFatalException(ResourceLimitFatalError,
                    "MemoryAllocationFailed",message);
                  message=(char *) RelinquishMagickMemory(message);
                }
              for (j=0; j < x; j++)
              {
                GetMagickToken(q,&q,token);
                if (*token == ',')
                  GetMagickToken(q,&q,token);
                CurrentContext->dash_pattern[j]=atof(token);
              }
              if ((x & 0x01) != 0)
                for ( ; j < (2*x); j++)
                  CurrentContext->dash_pattern[j]=
                    CurrentContext->dash_pattern[j-x];
              CurrentContext->dash_pattern[j]=0.0;
            }
        }
    }
  child=GetXMLTreeChild(xml_info,"stroke-dashoffset");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->dash_offset=atof(value);
    }
  child=GetXMLTreeChild(xml_info,"stroke-linecap");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->linecap=(LineCap) ParseMagickOption(
          MagickLineCapOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"stroke-linejoin");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->linejoin=(LineJoin) ParseMagickOption(
          MagickLineJoinOptions,MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"stroke-miterlimit");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->miterlimit=(unsigned long) atol(value);
    }
  child=GetXMLTreeChild(xml_info,"stroke-opacity");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->stroke.opacity=RoundToQuantum(QuantumRange*(1.0-
          atof(value)));
    }
  child=GetXMLTreeChild(xml_info,"stroke-width");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->stroke_width=atof(value);
    }
  child=GetXMLTreeChild(xml_info,"text-align");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->align=(AlignType) ParseMagickOption(MagickAlignOptions,
          MagickFalse,value);
    }
  child=GetXMLTreeChild(xml_info,"text-antialias");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        CurrentContext->text_antialias=atol(value) != 0 ? MagickTrue :
          MagickFalse;
    }
  child=GetXMLTreeChild(xml_info,"text-undercolor");
  if (child != (XMLTreeInfo *) NULL)
    {
      value=GetXMLTreeContent(child);
      if (value != (const char *) NULL)
        (void) QueryColorDatabase(value,&CurrentContext->undercolor,
          &wand->exception);
    }
  child=GetXMLTreeChild(xml_info,"vector-graphics");
  if (child != (XMLTreeInfo *) NULL)
    {
      (void) CloneString(&wand->mvg,GetXMLTreeContent(child));
      wand->mvg_length=strlen(wand->mvg);
      wand->mvg_alloc=wand->mvg_length+1;
    }
  xml_info=DestroyXMLTree(xml_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S k e w X                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSkewX() skews the current coordinate system in the horizontal
%  direction.
%
%  The format of the DrawSkewX method is:
%
%      void DrawSkewX(DrawingWand *wand,const double degrees)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o degrees: number of degrees to skew the coordinates
%
*/
WandExport void DrawSkewX(DrawingWand *wand,const double degrees)
{
  AffineMatrix
    affine;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  GetAffineMatrix(&affine);
  affine.ry=tan(DegreesToRadians(fmod(degrees,360.0)));
  AdjustAffine(wand,&affine);
  (void) MvgPrintf(wand,"skewX %g\n",degrees);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S k e w Y                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSkewY() skews the current coordinate system in the vertical
%  direction.
%
%  The format of the DrawSkewY method is:
%
%      void DrawSkewY(DrawingWand *wand,const double degrees)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o degrees: number of degrees to skew the coordinates
%
*/
WandExport void DrawSkewY(DrawingWand *wand,const double degrees)
{
  AffineMatrix
    affine;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  GetAffineMatrix(&affine);
  affine.rx=tan(DegreesToRadians(fmod(degrees,360.0)));
  DrawAffine(wand,&affine);
  (void) MvgPrintf(wand,"skewY %g\n",degrees);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w T r a n s l a t e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawTranslate() applies a translation to the current coordinate
%  system which moves the coordinate system origin to the specified
%  coordinate.
%
%  The format of the DrawTranslate method is:
%
%      void DrawTranslate(DrawingWand *wand,const double x,
%        const double y)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x: new x ordinate for coordinate system origin
%
%    o y: new y ordinate for coordinate system origin
%
*/
WandExport void DrawTranslate(DrawingWand *wand,const double x,
  const double y)
{
  AffineMatrix
    affine;

  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  GetAffineMatrix(&affine);
  affine.tx=x;
  affine.ty=y;
  AdjustAffine(wand,&affine);
  (void) MvgPrintf(wand,"translate %g,%g\n",x,y);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D r a w S e t V i e w b o x                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DrawSetViewbox() sets the overall canvas size to be recorded with the
%  drawing vector data.  Usually this will be specified using the same
%  size as the canvas image.  When the vector data is saved to SVG or MVG
%  formats, the viewbox is use to specify the size of the canvas image that
%  a viewer will render the vector data on.
%
%  The format of the DrawSetViewbox method is:
%
%      void DrawSetViewbox(DrawingWand *wand,unsigned long x1,
%        unsigned long y1,unsigned long x2,unsigned long y2)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
%    o x1: left x ordinate
%
%    o y1: top y ordinate
%
%    o x2: right x ordinate
%
%    o y2: bottom y ordinate
%
*/
WandExport void DrawSetViewbox(DrawingWand *wand,unsigned long x1,
  unsigned long y1,unsigned long x2,unsigned long y2)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) MvgPrintf(wand,"viewbox %lu %lu %lu %lu\n",x1,y1,x2,y2);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s D r a w i n g W a n d                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDrawingWand() returns MagickTrue if the wand is verified as a drawing wand.
%
%  The format of the IsDrawingWand method is:
%
%      MagickBooleanType IsDrawingWand(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/
WandExport MagickBooleanType IsDrawingWand(const DrawingWand *wand)
{
  if (wand == (const DrawingWand *) NULL)
    return(MagickFalse);
  if (wand->signature != WandSignature)
    return(MagickFalse);
  if (LocaleNCompare(wand->name,DrawingWandId,strlen(DrawingWandId)) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w D r a w i n g W a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewDrawingWand() returns a drawing wand required for all other methods in
%  the API.
%
%  The format of the NewDrawingWand method is:
%
%      DrawingWand NewDrawingWand(void)
%
%
*/
WandExport DrawingWand *NewDrawingWand(void)
{
  const char
    *quantum;

  DrawingWand
    *wand;

  unsigned long
    depth;

  quantum=GetMagickQuantumDepth(&depth);
  if (depth != QuantumDepth)
    ThrowWandFatalException(WandError,"QuantumDepthMismatch",quantum);
  wand=(DrawingWand *) AcquireMagickMemory(sizeof(*wand));
  if (wand == (DrawingWand *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowWandFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  (void) ResetMagickMemory(wand,0,sizeof(*wand));
  wand->id=AcquireWandId();
  (void) FormatMagickString(wand->name,MaxTextExtent,"%s-%lu",DrawingWandId,
    wand->id);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  wand->mvg=(char *) NULL;
  wand->mvg_alloc=0;
  wand->mvg_length=0;
  wand->mvg_width=0;
  wand->pattern_id=(char *) NULL;
  wand->pattern_offset=0;
  wand->pattern_bounds.x=0;
  wand->pattern_bounds.y=0;
  wand->pattern_bounds.width=0;
  wand->pattern_bounds.height=0;
  wand->index=0;
  wand->graphic_context=(DrawInfo **)
    AcquireMagickMemory(sizeof(*wand->graphic_context));
  if (wand->graphic_context == (DrawInfo **) NULL)
    {
      ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
        wand->name);
      return((DrawingWand *) NULL);
    }
  wand->filter_off=MagickFalse;
  wand->indent_depth=0;
  wand->path_operation=PathDefaultOperation;
  wand->path_mode=DefaultPathMode;
  wand->image=AllocateImage((const ImageInfo *) NULL);
  GetExceptionInfo(&wand->exception);
  wand->destroy=MagickTrue;
  wand->debug=IsEventLogging();
  wand->signature=WandSignature;
  CurrentContext=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
  return(wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P e e k D r a w i n g W a n d                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PeekDrawingWand() returns the current drawing wand.
%
%  The format of the PeekDrawingWand method is:
%
%      DrawInfo *PeekDrawingWand(const DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/

WandExport DrawInfo *DrawPeekGraphicWand(const DrawingWand *wand)
{
  return(PeekDrawingWand(wand));
}

WandExport DrawInfo *PeekDrawingWand(const DrawingWand *wand)
{
  DrawInfo
    *draw_info;

  assert(wand != (const DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  draw_info=CloneDrawInfo((ImageInfo *) NULL,CurrentContext);
  GetAffineMatrix(&draw_info->affine);
  (void) CloneString(&draw_info->primitive,wand->mvg);
  return(draw_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P o p D r a w i n g W a n d                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PopDrawingWand() destroys the current drawing wand and returns to the
%  previously pushed drawing wand. Multiple drawing wands may exist. It is an
%  error to attempt to pop more drawing wands than have been pushed, and it is
%  proper form to pop all drawing wands which have been pushed.
%
%  The format of the PopDrawingWand method is:
%
%      MagickBooleanType PopDrawingWand(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/

WandExport void DrawPopGraphicContext(DrawingWand *wand)
{
  (void) PopDrawingWand(wand);
}

WandExport MagickBooleanType PopDrawingWand(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->index == 0)
    {
      ThrowDrawException(DrawError,"UnbalancedDrawingWandPushPop",
        wand->name)
      return(MagickFalse);
    }
  /*
    Destroy clip path if not same in preceding wand.
  */
#if DRAW_BINARY_IMPLEMENTATION
  if (wand->image == (Image *) NULL)
    ThrowDrawException(WandError,"ContainsNoImages",wand->name);
  if (CurrentContext->clip_path != (char *) NULL)
    if (LocaleCompare(CurrentContext->clip_path,
        wand->graphic_context[wand->index-1]->clip_path) != 0)
      (void) SetImageClipMask(wand->image,(Image *) NULL);
#endif
  CurrentContext=DestroyDrawInfo(CurrentContext);
  wand->index--;
  if (wand->indent_depth > 0)
    wand->indent_depth--;
  (void) MvgPrintf(wand,"pop graphic-context\n");
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P u s h D r a w i n g W a n d                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PushDrawingWand() clones the current drawing wand to create a new drawing
%  wand.  The original drawing drawing wand(s) may be returned to by invoking
%  PopDrawingWand().  The drawing wands are stored on a drawing wand stack.
%  For every Pop there must have already been an equivalent Push.
%
%  The format of the PushDrawingWand method is:
%
%      MagickBooleanType PushDrawingWand(DrawingWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The drawing wand.
%
*/

WandExport void DrawPushGraphicContext(DrawingWand *wand)
{
  (void) PushDrawingWand(wand);
}

WandExport MagickBooleanType PushDrawingWand(DrawingWand *wand)
{
  assert(wand != (DrawingWand *) NULL);
  assert(wand->signature == WandSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  wand->index++;
  wand->graphic_context=(DrawInfo **) ResizeMagickMemory(wand->graphic_context,
    (size_t) (wand->index+1)*sizeof(*wand->graphic_context));
  if (wand->graphic_context == (DrawInfo **) NULL)
    {
      ThrowDrawException(ResourceLimitError,"MemoryAllocationFailed",
        wand->name);
      return(MagickFalse);
    }
  CurrentContext=CloneDrawInfo((ImageInfo *) NULL,
    wand->graphic_context[wand->index-1]);
  (void) MvgPrintf(wand,"push graphic-context\n");
  wand->indent_depth++;
  return(MagickTrue);
}
