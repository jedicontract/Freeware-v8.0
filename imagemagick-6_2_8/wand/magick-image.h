/*
  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickWand image Methods.
*/

#ifndef _MAGICKWAND_MAGICK_IMAGE_H
#define _MAGICKWAND_MAGICK_IMAGE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WandExport ChannelStatistics
  *MagickGetImageChannelStatistics(MagickWand *);

extern WandExport char
  *MagickGetImageAttribute(MagickWand *,const char *),
  *MagickGetImageFilename(MagickWand *),
  *MagickGetImageFormat(MagickWand *),
  *MagickGetImageSignature(MagickWand *),
  *MagickIdentifyImage(MagickWand *);

extern WandExport CompositeOperator
  MagickGetImageCompose(MagickWand *);

extern WandExport ColorspaceType
  MagickGetImageColorspace(MagickWand *);

extern WandExport CompressionType
  MagickGetImageCompression(MagickWand *);

extern WandExport DisposeType
  MagickGetImageDispose(MagickWand *);

extern WandExport double
  MagickGetImageGamma(MagickWand *),
  MagickGetImageTotalInkDensity(MagickWand *);

extern WandExport Image
  *GetImageFromMagickWand(MagickWand *);

extern WandExport ImageType
  MagickGetImageType(MagickWand *);

extern WandExport InterlaceType
  MagickGetImageInterlaceScheme(MagickWand *);

extern WandExport MagickBooleanType
  MagickAdaptiveSharpenImage(MagickWand *,const double,const double),
  MagickAdaptiveSharpenImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickAdaptiveThresholdImage(MagickWand *,const unsigned long,
    const unsigned long,const long),
  MagickAddImage(MagickWand *,const MagickWand *),
  MagickAddNoiseImage(MagickWand *,const NoiseType),
  MagickAddNoiseImageChannel(MagickWand *,const ChannelType,const NoiseType),
  MagickAffineTransformImage(MagickWand *,const DrawingWand *),
  MagickAnnotateImage(MagickWand *,const DrawingWand *,const double,
    const double,const double,const char *),
  MagickAnimateImages(MagickWand *,const char *),
  MagickBlackThresholdImage(MagickWand *,const PixelWand *),
  MagickBlurImage(MagickWand *,const double,const double),
  MagickBlurImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickBorderImage(MagickWand *,const PixelWand *,const unsigned long,
    const unsigned long),
  MagickCharcoalImage(MagickWand *,const double,const double),
  MagickChopImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickClipImage(MagickWand *),
  MagickClipPathImage(MagickWand *,const char *,const MagickBooleanType),
  MagickColorFloodfillImage(MagickWand *,const PixelWand *,const double,
    const PixelWand *,const long,const long),
  MagickColorizeImage(MagickWand *,const PixelWand *,const PixelWand *),
  MagickCommentImage(MagickWand *,const char *),
  MagickCompositeImage(MagickWand *,const MagickWand *,const CompositeOperator,
    const long,const long),
  MagickCompositeImageChannel(MagickWand *,const ChannelType,const MagickWand *,    const CompositeOperator,const long,const long),
  MagickConstituteImage(MagickWand *,const unsigned long,const unsigned long,
    const char *,const StorageType,const void *),
  MagickContrastImage(MagickWand *,const MagickBooleanType),
  MagickContrastStretchImage(MagickWand *,const double,const double),
  MagickContrastStretchImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickConvolveImage(MagickWand *,const unsigned long,const double *),
  MagickConvolveImageChannel(MagickWand *,const ChannelType,const unsigned long,
    const double *),
  MagickCropImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickCycleColormapImage(MagickWand *,const long),
  MagickDespeckleImage(MagickWand *),
  MagickDisplayImage(MagickWand *,const char *),
  MagickDisplayImages(MagickWand *,const char *),
  MagickDrawImage(MagickWand *,const DrawingWand *),
  MagickEdgeImage(MagickWand *,const double),
  MagickEmbossImage(MagickWand *,const double,const double),
  MagickEnhanceImage(MagickWand *),
  MagickEqualizeImage(MagickWand *),
  MagickEvaluateImage(MagickWand *,const MagickEvaluateOperator,const double),
  MagickEvaluateImageChannel(MagickWand *,const ChannelType,
    const MagickEvaluateOperator,const double),
  MagickFlipImage(MagickWand *),
  MagickFlopImage(MagickWand *),
  MagickFrameImage(MagickWand *,const PixelWand *,const unsigned long,
    const unsigned long,const long,const long),
  MagickGammaImage(MagickWand *,const double),
  MagickGammaImageChannel(MagickWand *,const ChannelType,const double),
  MagickGaussianBlurImage(MagickWand *,const double,const double),
  MagickGaussianBlurImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickGetImageBackgroundColor(MagickWand *,PixelWand *),
  MagickGetImageBluePrimary(MagickWand *,double *,double *),
  MagickGetImageBorderColor(MagickWand *,PixelWand *),
  MagickGetImageChannelDistortion(MagickWand *,const MagickWand *,
    const ChannelType, const MetricType,double *),
  MagickGetImageDistortion(MagickWand *,const MagickWand *,const MetricType,
    double *),
  MagickGetImageChannelExtrema(MagickWand *,const ChannelType,unsigned long *,
    unsigned long *),
  MagickGetImageChannelMean(MagickWand *,const ChannelType,double *,double *),
  MagickGetImageColormapColor(MagickWand *,const unsigned long,PixelWand *),
  MagickGetImageExtrema(MagickWand *,unsigned long *,unsigned long *),
  MagickGetImageGreenPrimary(MagickWand *,double *,double *),
  MagickGetImageMatte(MagickWand *),
  MagickGetImageMatteColor(MagickWand *,PixelWand *),
  MagickGetImagePage(MagickWand *,unsigned long *,unsigned long *,long *,
    long *),
  MagickGetImagePixelColor(MagickWand *,const long,const long,PixelWand *),
  MagickGetImagePixels(MagickWand *,const long,const long,const unsigned long,
    const unsigned long,const char *,const StorageType,void *),
  MagickGetImageRedPrimary(MagickWand *,double *,double *),
  MagickGetImageResolution(MagickWand *,double *,double *),
  MagickGetImageWhitePoint(MagickWand *,double *,double *),
  MagickHasNextImage(MagickWand *),
  MagickHasPreviousImage(MagickWand *),
  MagickImplodeImage(MagickWand *,const double),
  MagickLabelImage(MagickWand *,const char *),
  MagickLevelImage(MagickWand *,const double,const double,const double),
  MagickLevelImageChannel(MagickWand *,const ChannelType,const double,
    const double,const double),
  MagickMagnifyImage(MagickWand *),
  MagickMapImage(MagickWand *,const MagickWand *,const MagickBooleanType),
  MagickMatteFloodfillImage(MagickWand *,const Quantum,const double,
    const PixelWand *,const long,const long),
  MagickMedianFilterImage(MagickWand *,const double),
  MagickMinifyImage(MagickWand *),
  MagickModulateImage(MagickWand *,const double,const double,const double),
  MagickMotionBlurImage(MagickWand *,const double,const double,const double),
  MagickNegateImage(MagickWand *,const MagickBooleanType),
  MagickNegateImageChannel(MagickWand *,const ChannelType,
    const MagickBooleanType),
  MagickNewImage(MagickWand *,const unsigned long,const unsigned long,
    const PixelWand *),
  MagickNextImage(MagickWand *),
  MagickNormalizeImage(MagickWand *),
  MagickNormalizeImageChannel(MagickWand *,const ChannelType),
  MagickOilPaintImage(MagickWand *,const double),
  MagickPaintOpaqueImage(MagickWand *,const PixelWand *,const PixelWand *,
    const double),
  MagickPaintOpaqueImageChannel(MagickWand *,const ChannelType,
    const PixelWand *,const PixelWand *,const double),
  MagickPaintTransparentImage(MagickWand *,const PixelWand *,const Quantum,
    const double),
  MagickPingImage(MagickWand *,const char *),
  MagickPingImageBlob(MagickWand *,const void *,const size_t length),
  MagickPingImageFile(MagickWand *,FILE *),
  MagickPosterizeImage(MagickWand *,const unsigned long,
    const MagickBooleanType),
  MagickPreviousImage(MagickWand *),
  MagickProfileImage(MagickWand *,const char *,const void *,
    const unsigned long),
  MagickQuantizeImage(MagickWand *,const unsigned long,const ColorspaceType,
    const unsigned long,const MagickBooleanType,const MagickBooleanType),
  MagickQuantizeImages(MagickWand *,const unsigned long,const ColorspaceType,
    const unsigned long,const MagickBooleanType,const MagickBooleanType),
  MagickRadialBlurImage(MagickWand *,const double),
  MagickRadialBlurImageChannel(MagickWand *,const ChannelType,const double),
  MagickRaiseImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long,const MagickBooleanType),
  MagickRandomThresholdImage(MagickWand *,const double,const double),
  MagickRandomThresholdImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickReadImage(MagickWand *,const char *),
  MagickReadImageBlob(MagickWand *,const void *,const size_t length),
  MagickReadImageFile(MagickWand *,FILE *),
  MagickReduceNoiseImage(MagickWand *,const double),
  MagickRemoveImage(MagickWand *),
  MagickResampleImage(MagickWand *,const double,const double,const FilterTypes,
    const double),
  MagickResizeImage(MagickWand *,const unsigned long,const unsigned long,
    const FilterTypes,const double),
  MagickRollImage(MagickWand *,const long,const long),
  MagickRotateImage(MagickWand *,const PixelWand *,const double),
  MagickSampleImage(MagickWand *,const unsigned long,const unsigned long),
  MagickScaleImage(MagickWand *,const unsigned long,const unsigned long),
  MagickSegmentImage(MagickWand *,const ColorspaceType,const MagickBooleanType,
    const double,const double),
  MagickSeparateImageChannel(MagickWand *,const ChannelType),
  MagickSepiaToneImage(MagickWand *,const double),
  MagickSetImage(MagickWand *,const MagickWand *),
  MagickSetImageAttribute(MagickWand *,const char *,const char *),
  MagickSetImageBackgroundColor(MagickWand *,const PixelWand *),
  MagickSetImageBias(MagickWand *,const double),
  MagickSetImageBluePrimary(MagickWand *,const double,const double),
  MagickSetImageBorderColor(MagickWand *,const PixelWand *),
  MagickSetImageChannelDepth(MagickWand *,const ChannelType,
    const unsigned long),
  MagickSetImageColormapColor(MagickWand *,const unsigned long,
    const PixelWand *),
  MagickSetImageCompose(MagickWand *,const CompositeOperator),
  MagickSetImageCompression(MagickWand *,const CompressionType),
  MagickSetImageDelay(MagickWand *,const unsigned long),
  MagickSetImageDepth(MagickWand *,const unsigned long),
  MagickSetImageDispose(MagickWand *,const DisposeType),
  MagickSetImageColorspace(MagickWand *,const ColorspaceType),
  MagickSetImageCompressionQuality(MagickWand *,const unsigned long),
  MagickSetImageGreenPrimary(MagickWand *,const double,const double),
  MagickSetImageGamma(MagickWand *,const double),
  MagickSetImageExtent(MagickWand *,const unsigned long,const unsigned long),
  MagickSetImageFilename(MagickWand *,const char *),
  MagickSetImageFormat(MagickWand *,const char *),
  MagickSetImageInterlaceScheme(MagickWand *,const InterlaceType),
  MagickSetImageIterations(MagickWand *,const unsigned long),
  MagickSetImageMatte(MagickWand *,const MagickBooleanType),
  MagickSetImageMatteColor(MagickWand *,const PixelWand *),
  MagickSetImagePage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickSetImagePixels(MagickWand *,const long,const long,const unsigned long,
    const unsigned long,const char *,const StorageType,const void *),
  MagickSetImageProfile(MagickWand *,const char *,const void *,
    const unsigned long),
  MagickSetImageRedPrimary(MagickWand *,const double,const double),
  MagickSetImageRenderingIntent(MagickWand *,const RenderingIntent),
  MagickSetImageResolution(MagickWand *,const double,const double),
  MagickSetImageScene(MagickWand *,const unsigned long),
  MagickSetImageTicksPerSecond(MagickWand *,const unsigned long),
  MagickSetImageType(MagickWand *,const ImageType),
  MagickSetImageUnits(MagickWand *,const ResolutionType),
  MagickSetImageVirtualPixelMethod(MagickWand *,const VirtualPixelMethod),
  MagickSetImageWhitePoint(MagickWand *,const double,const double),
  MagickShadeImage(MagickWand *,const MagickBooleanType,const double,const 
    double),
  MagickShadowImage(MagickWand *,const double,const double,const long,
    const long),
  MagickSharpenImage(MagickWand *,const double,const double),
  MagickSharpenImageChannel(MagickWand *,const ChannelType,const double,
    const double),
  MagickShaveImage(MagickWand *,const unsigned long,const unsigned long),
  MagickShearImage(MagickWand *,const PixelWand *,const double,const double),
  MagickSigmoidalContrastImage(MagickWand *,const MagickBooleanType,
    const double,const double),
  MagickSigmoidalContrastImageChannel(MagickWand *,const ChannelType,
    const MagickBooleanType,const double,const double),
  MagickSolarizeImage(MagickWand *,const double),
  MagickSpliceImage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickSpreadImage(MagickWand *,const double),
  MagickStripImage(MagickWand *),
  MagickSwirlImage(MagickWand *,const double),
  MagickTintImage(MagickWand *,const PixelWand *,const PixelWand *),
  MagickTransposeImage(MagickWand *),
  MagickTransverseImage(MagickWand *),
  MagickThresholdImage(MagickWand *,const double),
  MagickThresholdImageChannel(MagickWand *,const ChannelType,const double),
  MagickThumbnailImage(MagickWand *,const unsigned long,const unsigned long),
  MagickTrimImage(MagickWand *,const double),
  MagickUnsharpMaskImage(MagickWand *,const double,const double,const double,
    const double),
  MagickUnsharpMaskImageChannel(MagickWand *,const ChannelType,const double,
    const double,const double,const double),
  MagickVignetteImage(MagickWand *,const double,const double,
    const long,const long),
  MagickWaveImage(MagickWand *,const double,const double),
  MagickWhiteThresholdImage(MagickWand *,const PixelWand *),
  MagickWriteImage(MagickWand *,const char *),
  MagickWriteImageFile(MagickWand *,FILE *),
  MagickWriteImages(MagickWand *,const char *,const MagickBooleanType),
  MagickWriteImagesFile(MagickWand *,FILE *);

WandExport MagickProgressMonitor
  MagickSetImageProgressMonitor(MagickWand *,const MagickProgressMonitor,
    void *);

extern WandExport MagickSizeType
  MagickGetImageSize(MagickWand *);

extern WandExport MagickWand
  *MagickAppendImages(MagickWand *,const MagickBooleanType),
  *MagickAverageImages(MagickWand *),
  *MagickCoalesceImages(MagickWand *),
  *MagickCombineImages(MagickWand *,const ChannelType),
  *MagickCompareImageChannels(MagickWand *,const MagickWand *,const ChannelType,
    const MetricType,double *),
  *MagickCompareImages(MagickWand *,const MagickWand *,const MetricType,
    double *),
  *MagickCompareImageLayers(MagickWand *,const MagickLayerMethod),
  *MagickDeconstructImages(MagickWand *),
  *MagickFlattenImages(MagickWand *),
  *MagickFxImage(MagickWand *,const char *),
  *MagickFxImageChannel(MagickWand *,const ChannelType,const char *),
  *MagickGetImage(MagickWand *),
  *MagickGetImageRegion(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  *MagickMorphImages(MagickWand *,const unsigned long),
  *MagickMosaicImages(MagickWand *),
  *MagickMontageImage(MagickWand *,const DrawingWand *,const char *,
    const char *,const MontageMode,const char *),
  *MagickOptimizeImageLayers(MagickWand *),
  *MagickPreviewImages(MagickWand *wand,const PreviewType),
  *MagickSteganoImage(MagickWand *,const MagickWand *,const long),
  *MagickStereoImage(MagickWand *,const MagickWand *),
  *MagickTextureImage(MagickWand *,const MagickWand *),
  *MagickTransformImage(MagickWand *,const char *,const char *),
  *NewMagickWandFromImage(const Image *);

extern WandExport PixelWand
  **MagickGetImageHistogram(MagickWand *,unsigned long *);

extern WandExport RenderingIntent
  MagickGetImageRenderingIntent(MagickWand *);

extern WandExport ResolutionType
  MagickGetImageUnits(MagickWand *);

extern WandExport unsigned char
  *MagickGetImageBlob(MagickWand *,size_t *),
  *MagickGetImagesBlob(MagickWand *,size_t *),
  *MagickGetImageProfile(MagickWand *,const char *,unsigned long *),
  *MagickRemoveImageProfile(MagickWand *,const char *,unsigned long *);

extern WandExport unsigned long
  MagickGetImageColors(MagickWand *),
  MagickGetImageCompressionQuality(MagickWand *),
  MagickGetImageDelay(MagickWand *),
  MagickGetImageChannelDepth(MagickWand *,const ChannelType),
  MagickGetImageDepth(MagickWand *),
  MagickGetImageHeight(MagickWand *),
  MagickGetImageIterations(MagickWand *),
  MagickGetImageScene(MagickWand *),
  MagickGetImageTicksPerSecond(MagickWand *),
  MagickGetImageWidth(MagickWand *),
  MagickGetNumberImages(MagickWand *);

extern WandExport VirtualPixelMethod
  MagickGetImageVirtualPixelMethod(MagickWand *);

/*
  Deprecated methods.
*/
extern WandExport char
  *MagickDescribeImage(MagickWand *);

extern WandExport long
  MagickGetImageIndex(MagickWand *);

extern WandExport MagickBooleanType
  MagickOpaqueImage(MagickWand *,const PixelWand *,const PixelWand *,
    const double),
  MagickSetImageIndex(MagickWand *,const long),
  MagickSetImageOption(MagickWand *,const char *,const char *,const char *),
  MagickTransparentImage(MagickWand *,const PixelWand *,const Quantum,
    const double);

extern WandExport MagickWand
  *MagickRegionOfInterestImage(MagickWand *,const unsigned long,
	  const unsigned long,const long,const long);

extern WandExport unsigned char
  *MagickWriteImageBlob(MagickWand *,size_t *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
