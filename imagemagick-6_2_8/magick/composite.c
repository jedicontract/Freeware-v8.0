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
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/cache-view.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/quantum.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e C h a n n e l                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImageChannel() returns the second image composited onto the first
%  at the specified offsets.
%
%  The format of the CompositeImageChannel method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const CompositeOperator compose,Image *composite_image,
%        const long x_offset,const long y_offset)
%      MagickBooleanType CompositeImageChannel(Image *image,
%        const ChannelType channel,const CompositeOperator compose,
%        Image *composite_image,const long x_offset,const long y_offset)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o composite_image: The composite image.
%
%    o x_offset: The column offset of the composited image.
%
%    o y_offset: The row offset of the composited image.
%
*/

static inline MagickRealType Add(const MagickRealType p,const MagickRealType q)
{
  MagickRealType
    pixel;

  pixel=p+q;
  if (pixel > QuantumRange)
    pixel-=(QuantumRange+1.0);
  return(pixel);
}

static inline void CompositeAdd(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  composite->red=Add(p->red,q->red);
  composite->green=Add(p->green,q->green);
  composite->blue=Add(p->blue,q->blue);
  composite->opacity=Add(alpha,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=Add(p->index,q->index);
}

static inline MagickRealType Atop(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    (1.0-QuantumScale*beta)*q*QuantumScale*alpha);
  return(pixel);
}

static inline void CompositeAtop(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Atop(p->red,alpha,q->red,beta);
  composite->green=gamma*Atop(p->green,alpha,q->green,beta);
  composite->blue=gamma*Atop(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Atop(p->index,alpha,q->index,beta);
}

static inline void CompositeBumpmap(const MagickPixelPacket *p,
  const MagickRealType magick_unused(alpha),const MagickPixelPacket *q,
  const MagickRealType magick_unused(beta),MagickPixelPacket *composite)
{
  MagickRealType
    intensity;

  intensity=MagickPixelIntensity(p);
  composite->red=QuantumScale*intensity*q->red;
  composite->green=QuantumScale*intensity*q->green;
  composite->blue=QuantumScale*intensity*q->blue;
  composite->opacity=QuantumScale*intensity*p->opacity;
  if (q->colorspace == CMYKColorspace)
    composite->index=QuantumScale*intensity*q->index;
}

static inline void CompositeClear(const MagickPixelPacket *q,
  MagickPixelPacket *composite)
{
  composite->red=0.0;
  composite->green=0.0;
  composite->blue=0.0;
  composite->opacity=(MagickRealType) TransparentOpacity;
  if (q->colorspace == CMYKColorspace)
    composite->index=0.0;
}

static MagickRealType ColorBurn(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    delta,
    pixel;

  delta=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha);
  if (delta <= ((1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)))
    {
      pixel=QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(pixel);
    }
  pixel=QuantumScale*((1.0-QuantumScale*alpha)*(QuantumScale*(1.0-QuantumScale*
    alpha)*p*(1.0-QuantumScale*beta)+QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0-QuantumScale*alpha)-(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta))/
    QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*(1.0-
    QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha)));
  return(pixel);
}

static inline void CompositeColorBurn(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*ColorBurn(p->red,alpha,q->red,beta);
  composite->green=gamma*ColorBurn(p->green,alpha,q->green,beta);
  composite->blue=gamma*ColorBurn(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*ColorBurn(p->index,alpha,q->index,beta);
}

static MagickRealType ColorDodge(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    delta,
    pixel;

  delta=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha);
  if (delta >= ((1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)))
    {
      pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)+QuantumScale*
        (1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+QuantumScale*
        (1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)/
    (1.0-QuantumScale*(1.0-QuantumScale*alpha)*p/(1.0-QuantumScale*alpha))+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeColorDodge(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*ColorDodge(p->red,alpha,q->red,beta);
  composite->green=gamma*ColorDodge(p->green,alpha,q->green,beta);
  composite->blue=gamma*ColorDodge(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*ColorDodge(p->index,alpha,q->index,beta);
}

static inline MagickRealType Darken(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if (((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)) <
      ((1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)))
    {
      pixel=(1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q*QuantumScale*
        alpha;
      return(pixel);
    }
  pixel=(1.0-QuantumScale*beta)*q+(1.0-QuantumScale*alpha)*p*QuantumScale*beta;
  return(pixel);
}

static inline void CompositeDarken(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Darken(p->red,alpha,q->red,beta);
  composite->green=gamma*Darken(p->green,alpha,q->green,beta);
  composite->blue=gamma*Darken(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Darken(p->index,alpha,q->index,beta);
}

static inline MagickRealType Difference(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*
    (1.0-QuantumScale*beta)*q-2.0*Min(QuantumScale*(1.0-QuantumScale*alpha)*p*
    (1.0-QuantumScale*beta),QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeDifference(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Difference(p->red,alpha,q->red,beta);
  composite->green=gamma*Difference(p->green,alpha,q->green,beta);
  composite->blue=gamma*Difference(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Difference(p->index,alpha,q->index,beta);
}

static inline MagickRealType Exclusion(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)-2.0*
    QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
    (1.0-QuantumScale*beta)*q)+QuantumScale*(1.0-QuantumScale*alpha)*p*
    (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0 -(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeExclusion(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Exclusion(p->red,alpha,q->red,beta);
  composite->green=gamma*Exclusion(p->green,alpha,q->green,beta);
  composite->blue=gamma*Exclusion(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Exclusion(p->index,alpha,q->index,beta);
}

static MagickRealType HardLight(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*alpha)*p) < (1.0-QuantumScale*alpha))
    {
      pixel=2.0*QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
        (1.0-QuantumScale*beta)*q+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)-2.0*
    ((1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    ((1.0-QuantumScale*alpha)-QuantumScale*(1.0-QuantumScale*alpha)*p)+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeHardLight(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*HardLight(p->red,alpha,q->red,beta);
  composite->green=gamma*HardLight(p->green,alpha,q->green,beta);
  composite->blue=gamma*HardLight(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*HardLight(p->index,alpha,q->index,beta);
}

static void CompositeHSB(const MagickRealType red,const MagickRealType green,
  const MagickRealType blue,double *hue,double *saturation,double *brightness)
{
  MagickRealType
    delta,
    max,
    min;

  /*
    Convert RGB to HSB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(brightness != (double *) NULL);
  max=(red > green ? red : green);
  if (blue > max)
    max=blue;
  min=(red < green ? red : green);
  if (blue < min)
    min=blue;
  *hue=0.0;
  *saturation=0.0;
  if (max != 0.0)
    *saturation=(double) ((max-min)/max);
  *brightness=(double) (QuantumScale*max);
  if (*saturation == 0.0)
    return;
  delta=max-min;
  if (red == max)
    *hue=(double) ((green-blue)/delta);
  else
    if (green == max)
      *hue=(double) (2.0+(blue-red)/delta);
    else
      if (blue == max)
        *hue=(double) (4.0+(red-green)/delta);
  *hue/=6.0;
  if (*hue < 0.0)
    *hue+=1.0;
  else
    if (*hue > 1.0)
      *hue-=1.0;
}

static inline MagickRealType In(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType magick_unused(q),
  const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta);
  return(pixel);
}

static inline void CompositeIn(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*In(p->red,alpha,q->red,beta);
  composite->green=gamma*In(p->green,alpha,q->green,beta);
  composite->blue=gamma*In(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*In(p->index,alpha,q->index,beta);
}

static inline MagickRealType Lighten(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if (((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)) >
      ((1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)))
    {
      pixel=(1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q*QuantumScale*
        alpha;
      return(pixel);
    }
  pixel=(1.0-QuantumScale*beta)*q+(1.0-QuantumScale*alpha)*p*QuantumScale*beta;
  return(pixel);
}

static inline void CompositeLighten(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Lighten(p->red,alpha,q->red,beta);
  composite->green=gamma*Lighten(p->green,alpha,q->green,beta);
  composite->blue=gamma*Lighten(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Lighten(p->index,alpha,q->index,beta);
}

static inline MagickRealType Minus(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*beta)*q);
}

static inline void CompositeMinus(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)-(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Minus(p->red,alpha,q->red,beta);
  composite->green=gamma*Minus(p->green,alpha,q->green,beta);
  composite->blue=gamma*Minus(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Minus(p->index,alpha,q->index,beta);
}

static inline MagickRealType Multiply(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)*q+
    (1.0-QuantumScale*alpha)*p*QuantumScale*beta+(1.0-QuantumScale*beta)*q*
    QuantumScale*alpha;
  return(pixel);
}

static inline void CompositeMultiply(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Multiply(p->red,alpha,q->red,beta);
  composite->green=gamma*Multiply(p->green,alpha,q->green,beta);
  composite->blue=gamma*Multiply(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Multiply(p->index,alpha,q->index,beta);
}

static inline MagickRealType Out(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType magick_unused(q),
  const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p*QuantumScale*beta);
}

static inline void CompositeOut(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)*QuantumScale*beta;
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Out(p->red,alpha,q->red,beta);
  composite->green=gamma*Out(p->green,alpha,q->green,beta);
  composite->blue=gamma*Out(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Out(p->index,alpha,q->index,beta);
}

static inline void CompositeOver(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=1.0-QuantumScale*QuantumScale*alpha*beta;
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*MagickOver_(p->red,alpha,q->red,beta);
  composite->green=gamma*MagickOver_(p->green,alpha,q->green,beta);
  composite->blue=gamma*MagickOver_(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*MagickOver_(p->index,alpha,q->index,beta);
}

static MagickRealType Overlay(const MagickRealType p,const MagickRealType alpha,
  const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*beta)*q) < (1.0-QuantumScale*beta))
    {
      pixel=2.0*QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
        (1.0-QuantumScale*beta)*q+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)-2.0*
    ((1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    ((1.0-QuantumScale*alpha)-QuantumScale*(1.0-QuantumScale*alpha)*p)+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeOverlay(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Overlay(p->red,alpha,q->red,beta);
  composite->green=gamma*Overlay(p->green,alpha,q->green,beta);
  composite->blue=gamma*Overlay(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Overlay(p->index,alpha,q->index,beta);
}

static inline MagickRealType Plus(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q);
}

static inline void CompositePlus(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Plus(p->red,alpha,q->red,beta);
  composite->green=gamma*Plus(p->green,alpha,q->green,beta);
  composite->blue=gamma*Plus(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Plus(p->index,alpha,q->index,beta);
}

static inline MagickRealType Screen(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*
    (1.0-QuantumScale*beta)*q-QuantumScale*(1.0-QuantumScale*alpha)*p*
    QuantumScale*(1.0-QuantumScale*beta)*q;
  return(QuantumRange*pixel);
}

static inline void CompositeScreen(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Screen(p->red,alpha,q->red,beta);
  composite->green=gamma*Screen(p->green,alpha,q->green,beta);
  composite->blue=gamma*Screen(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Screen(p->index,alpha,q->index,beta);
}

static MagickRealType SoftLight(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*alpha)*p) < (1.0-QuantumScale*alpha))
    {
      pixel=QuantumScale*(1.0-QuantumScale*beta)*q*((1.0-QuantumScale*alpha)-
        (1.0-QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta))*
        (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha)))+
        QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
        QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  if ((8.0*QuantumScale*(1.0-QuantumScale*beta)*q) <= (1.0-QuantumScale*beta))
    {
      pixel=QuantumScale*(1.0-QuantumScale*beta)*q*((1.0-QuantumScale*alpha)-
        (1.0-QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta))*
        (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha))*
        (3.0-8.0*QuantumScale*(1.0-QuantumScale*beta)*q/
        (1.0-QuantumScale*beta)))+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)+
    (pow(QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta),0*5)*
    (1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha)))+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(pixel);
}

static inline void CompositeSoftLight(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*SoftLight(p->red,alpha,q->red,beta);
  composite->green=gamma*SoftLight(p->green,alpha,q->green,beta);
  composite->blue=gamma*SoftLight(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*SoftLight(p->index,alpha,q->index,beta);
}

static inline MagickRealType Subtract(const MagickRealType p,
  const MagickRealType magick_unused(alpha),const MagickRealType q,
  const MagickRealType magick_unused(beta))
{
  MagickRealType
    pixel;

  pixel=p-q;
  if (pixel < 0.0)
    pixel+=(QuantumRange+1.0);
  return(pixel);
}

static inline void CompositeSubtract(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  composite->red=Subtract(p->red,alpha,q->red,beta);
  composite->green=Subtract(p->green,alpha,q->green,beta);
  composite->blue=Subtract(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=Subtract(p->index,alpha,q->index,beta);
}

static inline MagickRealType Threshold(const MagickRealType p,
  const MagickRealType magick_unused(alpha),const MagickRealType q,
  const MagickRealType magick_unused(beta),const MagickRealType threshold,
  const MagickRealType amount)
{
  MagickRealType
    pixel;

  pixel=p-q;
  if ((MagickRealType) fabs((double) (2.0*pixel)) < threshold)
    {
      pixel=q;
      return(pixel);
    }
  pixel=q+(pixel*amount);
  return(pixel);
}

static inline void CompositeThreshold(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,const MagickRealType threshold,
  const MagickRealType amount,MagickPixelPacket *composite)
{
  composite->red=Threshold(p->red,alpha,q->red,beta,threshold,amount);
  composite->green=Threshold(p->green,alpha,q->green,beta,threshold,amount);
  composite->blue=Threshold(p->blue,alpha,q->blue,beta,threshold,amount);
  composite->opacity=QuantumRange-
    Threshold(p->opacity,alpha,q->opacity,beta,threshold,amount);
  if (q->colorspace == CMYKColorspace)
    composite->index=Threshold(p->index,alpha,q->index,beta,threshold,amount);
}

static inline MagickRealType Xor(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(1.0-QuantumScale*alpha)*p*QuantumScale*beta+(1.0-QuantumScale*beta)*q*
    QuantumScale*alpha;
  return(pixel);
}

static inline void CompositeXor(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    2*(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Xor(p->red,alpha,q->red,beta);
  composite->green=gamma*Xor(p->green,alpha,q->green,beta);
  composite->blue=gamma*Xor(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Xor(p->index,alpha,q->index,beta);
}

static void HSBComposite(const double hue,const double saturation,
  const double brightness,MagickRealType *red,MagickRealType *green,
  MagickRealType *blue)
{
  MagickRealType
    f,
    h,
    p,
    q,
    t;

  /*
    Convert HSB to RGB colorspace.
  */
  assert(red != (MagickRealType *) NULL);
  assert(green != (MagickRealType *) NULL);
  assert(blue != (MagickRealType *) NULL);
  if (saturation == 0.0)
    {
      *red=QuantumRange*brightness;
      *green=(*red);
      *blue=(*red);
      return;
    }
  h=6.0*hue;
  if (h == 6.0)
    h=0.0;
  f=h-(int) h;
  p=brightness*(1.0-saturation);
  q=brightness*(1.0-saturation*f);
  t=brightness*(1.0-saturation*(1.0-f));
  switch ((int) h)
  {
    case 0:
    default:
    {
      *red=QuantumRange*brightness;
      *green=QuantumRange*t;
      *blue=QuantumRange*p;
      break;
    }
    case 1:
    {
      *red=QuantumRange*q;
      *green=QuantumRange*brightness;
      *blue=QuantumRange*p;
      break;
    }
    case 2:
    {
      *red=QuantumRange*p;
      *green=QuantumRange*brightness;
      *blue=QuantumRange*t;
      break;
    }
    case 3:
    {
      *red=QuantumRange*p;
      *green=QuantumRange*q;
      *blue=QuantumRange*brightness;
      break;
    }
    case 4:
    {
      *red=QuantumRange*t;
      *green=QuantumRange*p;
      *blue=QuantumRange*brightness;
      break;
    }
    case 5:
    {
      *red=QuantumRange*brightness;
      *green=QuantumRange*p;
      *blue=QuantumRange*q;
      break;
    }
  }
}

static MagickPixelPacket InterpolateColor(const Image *image,
  ViewInfo *image_view,const double x_offset,const double y_offset,
  ExceptionInfo *exception)
{
  MagickPixelPacket
    pixel,
    pixels[4];

  MagickRealType
    alpha[4],
    gamma;

  PointInfo
    delta;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  GetMagickPixelPacket(image,&pixel);
  p=AcquireCacheView(image_view,(long) x_offset,(long) y_offset,2,2,exception);
  if (p == (const PixelPacket *) NULL)
    return(pixel);
  for (i=0; i < 4L; i++)
  {
    indexes=GetCacheViewIndexes(image_view);
    alpha[i]=1.0;
    if (image->matte != MagickFalse)
      alpha[i]=QuantumScale*((MagickRealType) QuantumRange-p->opacity);
    GetMagickPixelPacket(image,pixels+i);
    SetMagickPixelPacket(p,indexes,pixels+i);
    pixels[i].red*=alpha[i];
    pixels[i].green*=alpha[i];
    pixels[i].blue*=alpha[i];
    if (image->colorspace == CMYKColorspace)
      pixels[i].index*=alpha[i];
    p++;
  }
  delta.x=x_offset-floor(x_offset);
  delta.y=y_offset-floor(y_offset);
  gamma=(((1.0-delta.y)*((1.0-delta.x)*alpha[0]+delta.x*alpha[1])+
    delta.y*((1.0-delta.x)*alpha[2]+delta.x*alpha[3])));
  gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
  pixel.red=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].red+delta.x*
    pixels[1].red)+delta.y*((1.0-delta.x)*pixels[2].red+delta.x*
    pixels[3].red)));
  pixel.green=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].green+delta.x*
    pixels[1].green)+delta.y*((1.0-delta.x)*pixels[2].green+delta.x*
    pixels[3].green)));
  pixel.blue=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].blue+delta.x*
    pixels[1].blue)+delta.y*((1.0-delta.x)*pixels[2].blue+delta.x*
    pixels[3].blue)));
  if (image->matte != MagickFalse)
    pixel.opacity=((1.0-delta.y)*((1.0-delta.x)*pixels[0].opacity+delta.x*
      pixels[1].opacity)+delta.y*((1.0-delta.x)*pixels[2].opacity+delta.x*
      pixels[3].opacity));
  if (image->colorspace == CMYKColorspace)
    pixel.index=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].index+delta.x*
      pixels[1].index)+delta.y*((1.0-delta.x)*pixels[2].index+delta.x*
      pixels[3].index)));
  return(pixel);
}

MagickExport MagickBooleanType CompositeImage(Image *image,
  const CompositeOperator compose,const Image *composite_image,
  const long x_offset,const long y_offset)
{
  MagickBooleanType
    status;

  status=CompositeImageChannel(image,DefaultChannels,compose,composite_image,
    x_offset,y_offset);
  return(status);
}

MagickExport MagickBooleanType CompositeImageChannel(Image *image,
  const ChannelType magick_unused(channel),const CompositeOperator compose,
  const Image *composite_image,const long x_offset,const long y_offset)
{
  const ImageAttribute
    *attribute;

  const PixelPacket
    *pixels;

  double
    brightness,
    hue,
    sans,
    saturation;

  GeometryInfo
    geometry_info;

  Image
    *displace_image;

  IndexPacket
    *composite_indexes,
    *indexes;

  long
    y;

  MagickBooleanType
    modify_outside_overlay;

  MagickPixelPacket
    composite,
    destination,
    source;

  MagickRealType
    amount,
    destination_dissolve,
    midpoint,
    percent_brightness,
    percent_saturation,
    source_dissolve,
    threshold;

  MagickStatusType
    flags;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  ViewInfo
    *composite_view,
    *image_view;

  /*
    Prepare composite image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(composite_image != (Image *) NULL);
  assert(composite_image->signature == MagickSignature);
  if (compose == NoCompositeOp)
    return(MagickTrue);
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
  displace_image=(Image *) NULL;
  amount=0.5;
  destination_dissolve=1.0;
  modify_outside_overlay=MagickFalse;
  percent_brightness=100.0;
  percent_saturation=100.0;
  source_dissolve=1.0;
  threshold=0.05f;
  switch (compose)
  {
    case ClearCompositeOp:
    case SrcCompositeOp:
    case SrcInCompositeOp:
    case InCompositeOp:
    case SrcOutCompositeOp:
    case OutCompositeOp:
    case DstInCompositeOp:
    case DstAtopCompositeOp:
    {
      /*
        These operators modify destination outside the overlaid region.
      */
      modify_outside_overlay=MagickTrue;
      break;
    }
    case CopyOpacityCompositeOp:
    {
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      modify_outside_overlay=MagickTrue;
      break;
    }
    case DisplaceCompositeOp:
    {
      MagickPixelPacket
        pixel;

      MagickRealType
        horizontal_scale,
        vertical_scale,
        x_displace,
        y_displace;

      register IndexPacket
        *displace_indexes;

      register PixelPacket
        *r;

      ViewInfo
        *composite_view,
        *displace_view,
        *image_view;

      /*
        Allocate the displace image.
      */
      displace_image=CloneImage(composite_image,0,0,MagickTrue,
        &image->exception);
      if (displace_image == (Image *) NULL)
        return(MagickFalse);
      horizontal_scale=20.0;
      vertical_scale=20.0;
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Determine the horizontal and vertical displacement scale.
          */
          SetGeometryInfo(&geometry_info);
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          horizontal_scale=geometry_info.rho;
          vertical_scale=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            vertical_scale=horizontal_scale;
        }
      /*
        Shift image pixels as defined by a displacement map.
      */
      GetMagickPixelPacket(displace_image,&pixel);
      image_view=OpenCacheView(image);
      displace_view=OpenCacheView(displace_image);
      composite_view=OpenCacheView(composite_image);
      for (y=0; y < (long) composite_image->rows; y++)
      {
        if (((y+y_offset) < 0) || ((y+y_offset) >= (long) image->rows))
          continue;
        p=AcquireCacheView(composite_view,0,y,composite_image->columns,1,
          &image->exception);
        q=GetCacheView(image_view,0,y+y_offset,image->columns,1);
        r=GetCacheView(displace_view,0,y,displace_image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL) ||
            (r == (PixelPacket *) NULL))
          break;
        displace_indexes=GetCacheViewIndexes(displace_view);
        q+=x_offset;
        for (x=0; x < (long) composite_image->columns; x++)
        {
          if (((x_offset+x) < 0) || ((x_offset+x) >= (long) image->columns))
            {
              p++;
              q++;
              continue;
            }
          x_displace=(horizontal_scale*(PixelIntensityToQuantum(p)-
            (((MagickRealType) QuantumRange+1.0)/2)))/
            (((MagickRealType) QuantumRange+1.0)/2.0);
          y_displace=x_displace;
          if (composite_image->matte != MagickFalse)
            y_displace=(vertical_scale*(p->opacity-(((MagickRealType)
              QuantumRange+1.0)/2.0)))/(((MagickRealType) QuantumRange+1.0)/2);
          pixel=InterpolateColor(image,image_view,(double) (x_offset+x+
            x_displace),(double) (y_offset+y+y_displace),&image->exception);
          SetPixelPacket(&pixel,r,displace_indexes+x);
          p++;
          q++;
          r++;
        }
        if (SyncCacheView(displace_view) == MagickFalse)
          break;
      }
      composite_view=CloseCacheView(composite_view);
      displace_view=CloseCacheView(displace_view);
      image_view=CloseCacheView(image_view);
      composite_image=displace_image;
      break;
    }
    case DissolveCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Geometry arguments to dissolve factors.
          */
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          destination_dissolve=1.0;
          if ((source_dissolve-MagickEpsilon) < 0.0)
            source_dissolve=0.0;
          if ((source_dissolve+MagickEpsilon) > 1.0)
            {
              destination_dissolve=2.0-source_dissolve;
              source_dissolve=1.0;
            }
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
          if ((destination_dissolve-MagickEpsilon) < 0.0)
            destination_dissolve=0.0;
          modify_outside_overlay=MagickTrue;
          if ((destination_dissolve+MagickEpsilon) > 1.0 )
            {
              destination_dissolve=1.0;
              modify_outside_overlay=MagickFalse;
            }
        }
      break;
    }
    case BlendCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          if ((source_dissolve-MagickEpsilon) < 0.0)
            source_dissolve=0.0;
          if ((source_dissolve+MagickEpsilon) > 1.0)
            source_dissolve=1.0;
          destination_dissolve=1.0-source_dissolve;
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
          if ((destination_dissolve-MagickEpsilon) < 0.0)
            destination_dissolve=0.0;
          modify_outside_overlay=MagickTrue;
          if ((destination_dissolve+MagickEpsilon) > 1.0)
            {
              destination_dissolve=1.0;
              modify_outside_overlay=MagickFalse;
            }
        }
      break;
    }
    case ModulateCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Determine the brightness and saturation scale.
          */
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          percent_brightness=geometry_info.rho;
          if ((flags & SigmaValue) != 0)
            percent_saturation=geometry_info.sigma;
        }
      break;
    }
    case ThresholdCompositeOp:
    {
      /*
        Determine the amount and threshold.
      */
      if (composite_image->geometry != (char *) NULL)
        {
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          amount=geometry_info.rho;
          threshold=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            threshold=0.05f;
        }
      threshold*=QuantumRange;
      break;
    }
    default:
      break;
  }
  attribute=GetImageAttribute(composite_image,"[modify-outside-overlay]");
  if (attribute != (ImageAttribute *) NULL)
    modify_outside_overlay=MagickFalse;
  /*
    Composite image.
  */
  hue=0.0;
  saturation=0.0;
  brightness=0.0;
  midpoint=((MagickRealType) QuantumRange+1.0)/2;
  GetMagickPixelPacket(composite_image,&source);
  GetMagickPixelPacket(image,&destination);
  image_view=OpenCacheView(image);
  composite_view=OpenCacheView(composite_image);
  for (y=0; y < (long) image->rows; y++)
  {
    if (modify_outside_overlay == MagickFalse)
      {
        if (y < y_offset)
          continue;
        if ((y-y_offset) >= (long) composite_image->rows)
          break;
      }
    /*
      If pixels is NULL, y is outside overlay region.
    */
    pixels=(PixelPacket *) NULL;
    p=(PixelPacket *) NULL;
    if ((y >= y_offset) && ((y-y_offset) < (long) composite_image->rows))
      {
        p=AcquireCacheView(composite_view,0,y-y_offset,composite_image->columns,
          1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        pixels=p;
        if (x_offset < 0)
          p-=x_offset;
      }
    q=GetCacheView(image_view,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetCacheViewIndexes(image_view);
    composite_indexes=GetCacheViewIndexes(composite_view);
    for (x=0; x < (long) image->columns; x++)
    {
      if (modify_outside_overlay == MagickFalse)
        {
          if (x < x_offset)
            {
              q++;
              continue;
            }
          if ((x-x_offset) >= (long) composite_image->columns)
            break;
        }
      destination.red=(MagickRealType) q->red;
      destination.green=(MagickRealType) q->green;
      destination.blue=(MagickRealType) q->blue;
      if (image->matte != MagickFalse)
        destination.opacity=(MagickRealType) q->opacity;
      if (image->colorspace == CMYKColorspace)
        {
          destination.red=QuantumRange-destination.red;
          destination.green=QuantumRange-destination.green;
          destination.blue=QuantumRange-destination.blue;
          destination.index=QuantumRange-(MagickRealType) indexes[x];
        }
      /*
        Handle destination modifications outside overlaid region.
      */
      composite=destination;
      if ((pixels == (PixelPacket *) NULL) || (x < x_offset) ||
          ((x-x_offset) >= (long) composite_image->columns))
        {
          switch (compose)
          {
            case DissolveCompositeOp:
            case BlendCompositeOp:
            {
              composite.opacity=QuantumRange-destination_dissolve*
                (QuantumRange-composite.opacity);
              break;
            }
            case ClearCompositeOp:
            case SrcCompositeOp:
            {
              CompositeClear(&destination,&composite);
              break;
            }
            case InCompositeOp:
            case SrcInCompositeOp:
            case SrcOutCompositeOp:
            case DstInCompositeOp:
            case DstAtopCompositeOp:
            case CopyOpacityCompositeOp:
            {
              composite.opacity=(MagickRealType) TransparentOpacity;
              break;
            }
            default:
              break;

          }
          if (image->colorspace == CMYKColorspace)
            {
              composite.red=QuantumRange-composite.red;
              composite.green=QuantumRange-composite.green;
              composite.blue=QuantumRange-composite.blue;
              composite.index=QuantumRange-composite.index;
            }
          q->red=RoundToQuantum(composite.red);
          q->green=RoundToQuantum(composite.green);
          q->blue=RoundToQuantum(composite.blue);
          if (image->matte != MagickFalse)
            q->opacity=RoundToQuantum(composite.opacity);
          if (image->colorspace == CMYKColorspace)
            indexes[x]=RoundToQuantum(composite.index);
          q++;
          continue;
        }
      /*
        Handle normal overlay of source onto destination.
      */
      source.red=(MagickRealType) p->red;
      source.green=(MagickRealType) p->green;
      source.blue=(MagickRealType) p->blue;
      if (composite_image->matte != MagickFalse)
        source.opacity=(MagickRealType) p->opacity;
      if (composite_image->colorspace == CMYKColorspace)
        {
          source.red=QuantumRange-source.red;
          source.green=QuantumRange-source.green;
          source.blue=QuantumRange-source.blue;
          source.index=QuantumRange-(MagickRealType)
            composite_indexes[x-x_offset];
        }
      switch (compose)
      {
        case ClearCompositeOp:
        {
          CompositeClear(&destination,&composite);
          break;
        }
        case SrcCompositeOp:
        case CopyCompositeOp:
        case ReplaceCompositeOp:
        {
          composite=source;
          break;
        }
        case DstCompositeOp:
          break;
        case OverCompositeOp:
        case SrcOverCompositeOp:
        {
          CompositeOver(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstOverCompositeOp:
        {
          CompositeOver(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case SrcInCompositeOp:
        case InCompositeOp:
        {
          CompositeIn(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstInCompositeOp:
        {
          CompositeIn(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case OutCompositeOp:
        case SrcOutCompositeOp:
        {
          CompositeOut(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstOutCompositeOp:
        {
          CompositeOut(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case AtopCompositeOp:
        case SrcAtopCompositeOp:
        {
          CompositeAtop(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstAtopCompositeOp:
        {
          CompositeAtop(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case XorCompositeOp:
        {
          CompositeXor(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case PlusCompositeOp:
        {
          CompositePlus(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case MultiplyCompositeOp:
        {
          CompositeMultiply(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ScreenCompositeOp:
        {
          CompositeScreen(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case OverlayCompositeOp:
        {
          CompositeOverlay(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DarkenCompositeOp:
        {
          CompositeDarken(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case LightenCompositeOp:
        {
          CompositeLighten(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ColorDodgeCompositeOp:
        {
          CompositeColorDodge(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ColorBurnCompositeOp:
        {
          CompositeColorBurn(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case HardLightCompositeOp:
        {
          CompositeHardLight(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case SoftLightCompositeOp:
        {
          CompositeSoftLight(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DifferenceCompositeOp:
        {
          CompositeDifference(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ExclusionCompositeOp:
        {
          CompositeExclusion(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case MinusCompositeOp:
        {
          CompositeMinus(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case BumpmapCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          CompositeBumpmap(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DissolveCompositeOp:
        {
          CompositeOver(&source,QuantumRange-source_dissolve*
            (QuantumRange-source.opacity),&destination,QuantumRange-
            destination_dissolve*(QuantumRange-destination.opacity),&composite);
          break;
        }
        case BlendCompositeOp:
        {
          CompositePlus(&source,QuantumRange-source_dissolve*(QuantumRange-
            source.opacity),&destination,QuantumRange-destination_dissolve*
            (QuantumRange-destination.opacity),&composite);
          break;
        }
        case DisplaceCompositeOp:
        {
          composite=source;
          break;
        }
        case ThresholdCompositeOp:
        {
          CompositeThreshold(&source,source.opacity,&destination,
            destination.opacity,threshold,amount,&composite);
          break;
        }
        case ModulateCompositeOp:
        {
          long
            offset;

          if (source.opacity == TransparentOpacity)
            break;
          offset=(long) (MagickPixelIntensityToQuantum(&source)-midpoint);
          if (offset == 0)
            break;
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          brightness+=(0.01*percent_brightness*offset)/midpoint;
          saturation*=0.01*percent_saturation;
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          break;
        }
        case HueCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&hue,&sans,&sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case SaturateCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&sans,&saturation,
            &sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case LuminizeCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&sans,&sans,
            &brightness);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case ColorizeCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&sans,
            &sans,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&hue,&saturation,
            &sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case AddCompositeOp:  /* deprecated */
        {
          CompositeAdd(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case SubtractCompositeOp:  /* deprecated */
        {
          CompositeSubtract(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case CopyRedCompositeOp:  /* deprecated */
        case CopyCyanCompositeOp:
        {
          composite.red=source.red;
          break;
        }
        case CopyGreenCompositeOp:  /* deprecated */
        case CopyMagentaCompositeOp:
        {
          composite.green=source.green;
          break;
        }
        case CopyBlueCompositeOp:  /* deprecated */
        case CopyYellowCompositeOp:
        {
          composite.blue=source.blue;
          break;
        }
        case CopyOpacityCompositeOp:  /* deprecated */
        {
          if (source.matte != MagickFalse)
            {
              composite.opacity=source.opacity;
              break;
            }
          composite.opacity=(MagickRealType) (QuantumRange-
            MagickPixelIntensityToQuantum(&source));
          break;
        }
        case CopyBlackCompositeOp:  /* deprecated */
        {
          composite.index=source.index;
          break;
        }
        default:
          break;
      }
      if (image->colorspace == CMYKColorspace)
        {
          composite.red=QuantumRange-composite.red;
          composite.green=QuantumRange-composite.green;
          composite.blue=QuantumRange-composite.blue;
          composite.index=QuantumRange-composite.index;
        }
      q->red=RoundToQuantum(composite.red);
      q->green=RoundToQuantum(composite.green);
      q->blue=RoundToQuantum(composite.blue);
      if (image->matte != MagickFalse)
        q->opacity=RoundToQuantum(composite.opacity);
      if (image->colorspace == CMYKColorspace)
        indexes[x]=RoundToQuantum(composite.index);
      p++;
      if (p >= (pixels+composite_image->columns))
        p=pixels;
      q++;
    }
    if (SyncCacheView(image_view) == MagickFalse)
      break;
  }
  composite_view=CloseCacheView(composite_view);
  image_view=CloseCacheView(image_view);
  if (displace_image != (Image * ) NULL)
    displace_image=DestroyImage(displace_image);
  return(MagickTrue);
}
