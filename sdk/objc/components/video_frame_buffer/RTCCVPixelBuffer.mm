/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "RTCCVPixelBuffer.h"

#import "api/video_frame_buffer/RTCNativeMutableI420Buffer.h"

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv.h"

#if !defined(NDEBUG) && defined(WEBRTC_IOS)
#import <UIKit/UIKit.h>
#import <VideoToolbox/VideoToolbox.h>
#endif

@implementation RTC_OBJC_TYPE (RTCCVPixelBuffer) {
  int _width;
  int _height;
  int _bufferWidth;
  int _bufferHeight;
  int _cropWidth;
  int _cropHeight;
}

@synthesize pixelBuffer = _pixelBuffer;
@synthesize cropX = _cropX;
@synthesize cropY = _cropY;
@synthesize cropWidth = _cropWidth;
@synthesize cropHeight = _cropHeight;

+ (NSSet<NSNumber*>*)supportedPixelFormats {
  return [NSSet setWithObjects:@(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange),
                               @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
                               @(kCVPixelFormatType_32BGRA),
                               @(kCVPixelFormatType_32ARGB),
                               nil];
}

- (instancetype)initWithPixelBuffer:(CVPixelBufferRef)pixelBuffer {
  return [self initWithPixelBuffer:pixelBuffer
                      adaptedWidth:CVPixelBufferGetWidth(pixelBuffer)
                     adaptedHeight:CVPixelBufferGetHeight(pixelBuffer)
                         cropWidth:CVPixelBufferGetWidth(pixelBuffer)
                        cropHeight:CVPixelBufferGetHeight(pixelBuffer)
                             cropX:0
                             cropY:0];
}

- (instancetype)initWithPixelBuffer:(CVPixelBufferRef)pixelBuffer
                       adaptedWidth:(int)adaptedWidth
                      adaptedHeight:(int)adaptedHeight
                          cropWidth:(int)cropWidth
                         cropHeight:(int)cropHeight
                              cropX:(int)cropX
                              cropY:(int)cropY {
  if (self = [super init]) {
    _width = adaptedWidth;
    _height = adaptedHeight;
    _pixelBuffer = pixelBuffer;
    _bufferWidth = CVPixelBufferGetWidth(_pixelBuffer);
    _bufferHeight = CVPixelBufferGetHeight(_pixelBuffer);
    _cropWidth = cropWidth;
    _cropHeight = cropHeight;
    // Can only crop at even pixels.
    _cropX = cropX & ~1;
    _cropY = cropY & ~1;
    CVBufferRetain(_pixelBuffer);
  }

  return self;
}

- (void)dealloc {
  CVBufferRelease(_pixelBuffer);
}

- (int)width {
  return _width;
}

- (int)height {
  return _height;
}

- (BOOL)requiresCropping {
  return _cropWidth != _bufferWidth || _cropHeight != _bufferHeight;
}

- (BOOL)requiresScalingToWidth:(int)width height:(int)height {
  return _cropWidth != width || _cropHeight != height;
}

- (int)bufferSizeForCroppingAndScalingToWidth:(int)width height:(int)height {
  const OSType srcPixelFormat = CVPixelBufferGetPixelFormatType(_pixelBuffer);
  switch (srcPixelFormat) {
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: {
      int srcChromaWidth = (_cropWidth + 1) / 2;
      int srcChromaHeight = (_cropHeight + 1) / 2;
      int dstChromaWidth = (width + 1) / 2;
      int dstChromaHeight = (height + 1) / 2;

      return srcChromaWidth * srcChromaHeight * 2 + dstChromaWidth * dstChromaHeight * 2;
    }
    case kCVPixelFormatType_32BGRA:
    case kCVPixelFormatType_32ARGB: {
      return 0;  // Scaling RGBA frames does not require a temporary buffer.
    }
  }
  RTC_NOTREACHED() << "Unsupported pixel format.";
  return 0;
}

- (BOOL)cropAndScaleTo:(CVPixelBufferRef)outputPixelBuffer
        withTempBuffer:(nullable uint8_t*)tmpBuffer {
  const OSType srcPixelFormat = CVPixelBufferGetPixelFormatType(_pixelBuffer);
  const OSType dstPixelFormat = CVPixelBufferGetPixelFormatType(outputPixelBuffer);

  switch (srcPixelFormat) {
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: {
      size_t dstWidth = CVPixelBufferGetWidth(outputPixelBuffer);
      size_t dstHeight = CVPixelBufferGetHeight(outputPixelBuffer);
      if (dstWidth > 0 && dstHeight > 0) {
        RTC_DCHECK(dstPixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange ||
                   dstPixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange);
        if ([self requiresScalingToWidth:dstWidth height:dstHeight]) {
          RTC_DCHECK(tmpBuffer);
        }
        [self cropAndScaleNV12To:outputPixelBuffer withTempBuffer:tmpBuffer];
      }
      break;
    }
    case kCVPixelFormatType_32BGRA:
    case kCVPixelFormatType_32ARGB: {
      RTC_DCHECK(srcPixelFormat == dstPixelFormat);
      [self cropAndScaleARGBTo:outputPixelBuffer];
      break;
    }
    default: { RTC_NOTREACHED() << "Unsupported pixel format."; }
  }

  return YES;
}

- (id<RTC_OBJC_TYPE(RTCI420Buffer)>)toI420 {
  const OSType pixelFormat = CVPixelBufferGetPixelFormatType(_pixelBuffer);

  CVPixelBufferLockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);

  RTC_OBJC_TYPE(RTCMutableI420Buffer)* i420Buffer =
      [[RTC_OBJC_TYPE(RTCMutableI420Buffer) alloc] initWithWidth:[self width] height:[self height]];

  switch (pixelFormat) {
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: {
      const uint8_t* srcY =
          static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 0));
      const int srcYStride = CVPixelBufferGetBytesPerRowOfPlane(_pixelBuffer, 0);
      const uint8_t* srcUV =
          static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 1));
      const int srcUVStride = CVPixelBufferGetBytesPerRowOfPlane(_pixelBuffer, 1);

      // Crop just by modifying pointers.
      srcY += srcYStride * _cropY + _cropX;
      srcUV += srcUVStride * (_cropY / 2) + _cropX;

      // TODO(magjed): Use a frame buffer pool.
      webrtc::NV12ToI420Scaler nv12ToI420Scaler;
      nv12ToI420Scaler.NV12ToI420Scale(srcY,
                                       srcYStride,
                                       srcUV,
                                       srcUVStride,
                                       _cropWidth,
                                       _cropHeight,
                                       i420Buffer.mutableDataY,
                                       i420Buffer.strideY,
                                       i420Buffer.mutableDataU,
                                       i420Buffer.strideU,
                                       i420Buffer.mutableDataV,
                                       i420Buffer.strideV,
                                       i420Buffer.width,
                                       i420Buffer.height);
      break;
    }
    case kCVPixelFormatType_32BGRA:
    case kCVPixelFormatType_32ARGB: {
      CVPixelBufferRef scaledPixelBuffer = NULL;
      CVPixelBufferRef sourcePixelBuffer = NULL;
      if ([self requiresCropping] ||
          [self requiresScalingToWidth:i420Buffer.width height:i420Buffer.height]) {
        CVPixelBufferCreate(
            NULL, i420Buffer.width, i420Buffer.height, pixelFormat, NULL, &scaledPixelBuffer);
        [self cropAndScaleTo:scaledPixelBuffer withTempBuffer:NULL];

        CVPixelBufferLockBaseAddress(scaledPixelBuffer, kCVPixelBufferLock_ReadOnly);
        sourcePixelBuffer = scaledPixelBuffer;
      } else {
        sourcePixelBuffer = _pixelBuffer;
      }
      const uint8_t* src = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(sourcePixelBuffer));
      const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(sourcePixelBuffer);

      if (pixelFormat == kCVPixelFormatType_32BGRA) {
        // Corresponds to libyuv::FOURCC_ARGB
        libyuv::ARGBToI420(src,
                           bytesPerRow,
                           i420Buffer.mutableDataY,
                           i420Buffer.strideY,
                           i420Buffer.mutableDataU,
                           i420Buffer.strideU,
                           i420Buffer.mutableDataV,
                           i420Buffer.strideV,
                           i420Buffer.width,
                           i420Buffer.height);
      } else if (pixelFormat == kCVPixelFormatType_32ARGB) {
        // Corresponds to libyuv::FOURCC_BGRA
        libyuv::BGRAToI420(src,
                           bytesPerRow,
                           i420Buffer.mutableDataY,
                           i420Buffer.strideY,
                           i420Buffer.mutableDataU,
                           i420Buffer.strideU,
                           i420Buffer.mutableDataV,
                           i420Buffer.strideV,
                           i420Buffer.width,
                           i420Buffer.height);
      }

      if (scaledPixelBuffer) {
        CVPixelBufferUnlockBaseAddress(scaledPixelBuffer, kCVPixelBufferLock_ReadOnly);
        CVBufferRelease(scaledPixelBuffer);
      }
      break;
    }
    default: { RTC_NOTREACHED() << "Unsupported pixel format."; }
  }

  CVPixelBufferUnlockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);

  return i420Buffer;
}

- (CVPixelBufferRef)mirrorBuffer {
    //CVPixelBufferRef是CVImageBufferRef的别名,两者操作几乎一致。
    //获取CMSampleBuffer的图像地址
    CVImageBufferRef pixelBuffer = self.pixelBuffer;
    if (!pixelBuffer) {
        return nil;
    }
    //表示开始操作数据
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    //图像宽度(像素)
    size_t buffer_width = CVPixelBufferGetWidth(pixelBuffer);
    //图像高度(像素)
    size_t buffer_height = CVPixelBufferGetHeight(pixelBuffer);
    //获取CVImageBufferRef中的y数据
    uint8_t *src_y_frame = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
    //获取CMVImageBufferRef中的uv数据
    uint8_t *src_uv_frame =(unsigned char *) CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
    //y stride
    size_t plane1_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
    //uv stride
    size_t plane2_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);
    //y height
    size_t plane1_height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 0);
    //uv height
    size_t plane2_height = CVPixelBufferGetHeightOfPlane(pixelBuffer, 1);
    //y_size
    size_t plane1_size = plane1_stride * plane1_height;
    //uv_size
    size_t plane2_size = plane2_stride * plane2_height;
    //yuv_size(内存空间)
    size_t frame_size = plane1_size + plane2_size;

    uint8_t *nv12_y = (uint8_t *)malloc(frame_size);
    uint8_t *nv12_uv = nv12_y + plane1_stride * plane1_height;
    
    // 1. NV12镜像
    libyuv::NV12Mirror(/*const uint8_t *src_y*/ src_y_frame,
                       /*int src_stride_y*/ (int)plane1_stride,
                       /*const uint8_t *src_uv*/ src_uv_frame,
                       /*int src_stride_uv*/ (int)plane1_stride >> plane2_stride,
                       /*uint8_t *dst_y*/ nv12_y,
                       /*int dst_stride_y*/ (int)plane1_stride,
                       /*uint8_t *dst_uv*/ nv12_uv,
                       /*int dst_stride_uv*/ (int)plane1_stride >> plane2_stride,
                       /*int width*/ (int)buffer_width,
                       /*int height*/ (int)buffer_height);
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    
    // 2.NV12转换为CVPixelBufferRef
    NSDictionary *pixelAttributes = @{(id)kCVPixelBufferIOSurfacePropertiesKey : @{}};
    CVPixelBufferRef dstPixelBuffer = NULL;
    CVReturn result = CVPixelBufferCreate(kCFAllocatorDefault,
                                          buffer_width, buffer_height, kCVPixelFormatType_420YpCbCr8BiPlanarFullRange,
                                          (__bridge CFDictionaryRef)pixelAttributes, &dstPixelBuffer);

    CVPixelBufferLockBaseAddress(dstPixelBuffer, 0);
    uint8_t *yDstPlane = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(dstPixelBuffer, 0);
    memcpy(yDstPlane, nv12_y, plane1_size);
    uint8_t *uvDstPlane = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(dstPixelBuffer, 1);
    memcpy(uvDstPlane, nv12_uv, plane2_size);
    if (result != kCVReturnSuccess) {
        NSLog(@"Unable to create cvpixelbuffer %d", result);
    }
    CVPixelBufferUnlockBaseAddress(dstPixelBuffer, 0);
    free(nv12_y);

    return dstPixelBuffer;
}

#pragma mark - Debugging

#if !defined(NDEBUG) && defined(WEBRTC_IOS)
- (id)debugQuickLookObject {
  CGImageRef cgImage;
  VTCreateCGImageFromCVPixelBuffer(_pixelBuffer, NULL, &cgImage);
  UIImage *image = [UIImage imageWithCGImage:cgImage scale:1.0 orientation:UIImageOrientationUp];
  CGImageRelease(cgImage);
  return image;
}
#endif

#pragma mark - Private

- (void)cropAndScaleNV12To:(CVPixelBufferRef)outputPixelBuffer withTempBuffer:(uint8_t*)tmpBuffer {
  // Prepare output pointers.
  CVReturn cvRet = CVPixelBufferLockBaseAddress(outputPixelBuffer, 0);
  if (cvRet != kCVReturnSuccess) {
    RTC_LOG(LS_ERROR) << "Failed to lock base address: " << cvRet;
  }
  const int dstWidth = CVPixelBufferGetWidth(outputPixelBuffer);
  const int dstHeight = CVPixelBufferGetHeight(outputPixelBuffer);
  uint8_t* dstY =
      reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(outputPixelBuffer, 0));
  const int dstYStride = CVPixelBufferGetBytesPerRowOfPlane(outputPixelBuffer, 0);
  uint8_t* dstUV =
      reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(outputPixelBuffer, 1));
  const int dstUVStride = CVPixelBufferGetBytesPerRowOfPlane(outputPixelBuffer, 1);

  // Prepare source pointers.
  CVPixelBufferLockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);
  const uint8_t* srcY = static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 0));
  const int srcYStride = CVPixelBufferGetBytesPerRowOfPlane(_pixelBuffer, 0);
  const uint8_t* srcUV = static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(_pixelBuffer, 1));
  const int srcUVStride = CVPixelBufferGetBytesPerRowOfPlane(_pixelBuffer, 1);

  // Crop just by modifying pointers.
  srcY += srcYStride * _cropY + _cropX;
  srcUV += srcUVStride * (_cropY / 2) + _cropX;

  webrtc::NV12Scale(tmpBuffer,
                    srcY,
                    srcYStride,
                    srcUV,
                    srcUVStride,
                    _cropWidth,
                    _cropHeight,
                    dstY,
                    dstYStride,
                    dstUV,
                    dstUVStride,
                    dstWidth,
                    dstHeight);

  CVPixelBufferUnlockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);
  CVPixelBufferUnlockBaseAddress(outputPixelBuffer, 0);
}

- (void)cropAndScaleARGBTo:(CVPixelBufferRef)outputPixelBuffer {
  // Prepare output pointers.
  CVReturn cvRet = CVPixelBufferLockBaseAddress(outputPixelBuffer, 0);
  if (cvRet != kCVReturnSuccess) {
    RTC_LOG(LS_ERROR) << "Failed to lock base address: " << cvRet;
  }
  const int dstWidth = CVPixelBufferGetWidth(outputPixelBuffer);
  const int dstHeight = CVPixelBufferGetHeight(outputPixelBuffer);

  uint8_t* dst = reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddress(outputPixelBuffer));
  const int dstStride = CVPixelBufferGetBytesPerRow(outputPixelBuffer);

  // Prepare source pointers.
  CVPixelBufferLockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);
  const uint8_t* src = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(_pixelBuffer));
  const int srcStride = CVPixelBufferGetBytesPerRow(_pixelBuffer);

  // Crop just by modifying pointers. Need to ensure that src pointer points to a byte corresponding
  // to the start of a new pixel (byte with B for BGRA) so that libyuv scales correctly.
  const int bytesPerPixel = 4;
  src += srcStride * _cropY + (_cropX * bytesPerPixel);

  // kCVPixelFormatType_32BGRA corresponds to libyuv::FOURCC_ARGB
  libyuv::ARGBScale(src,
                    srcStride,
                    _cropWidth,
                    _cropHeight,
                    dst,
                    dstStride,
                    dstWidth,
                    dstHeight,
                    libyuv::kFilterBox);

  CVPixelBufferUnlockBaseAddress(_pixelBuffer, kCVPixelBufferLock_ReadOnly);
  CVPixelBufferUnlockBaseAddress(outputPixelBuffer, 0);
}

@end
