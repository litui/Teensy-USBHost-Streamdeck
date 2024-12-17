/*
Copyright 2024 Aria Burrell <litui@litui.ca>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Some ideas borrowed from the python-elgato-streamdeck library.
Credit to:
   dean [at] fourwalledcubicle [dot] com
         www.fourwalledcubicle.com
*/
#include "streamdeck_graphics.hpp"
#if STREAMDECK_IMAGE_HELPER_ENABLE

#include <Entropy.h>

const tgx::Shader LOADED_SHADERS = tgx::SHADER_ORTHO | tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2;

namespace Streamdeck {

tgx::RGB565 *outboundFrameBuffer;
tgx::Image<tgx::RGB565> outboundSprite;

// Allocate a framebuffer of the appropriate size if not provided
void Image::allocateFrameBuffer(const uint16_t width, const uint16_t height) {
  frameBuffer_ = (RGB565 *)calloc(width * height, sizeof(RGB565));
  madeFrameBuffer = true;
}

// Only frees the framebuffer if Image class created it.
void Image::freeFrameBuffer() {
  if (madeFrameBuffer)
    free(frameBuffer_);
}

bool Image::importJpeg(const uint8_t *inBuffer, const uint16_t inLength) {
  // Load JPEGDEC to process the jpeg headers
  if (!jpgDecoder.openRAM((uint8_t *)inBuffer, inLength, TGX_JPEGDraw)) {
    return false;
  }

  // Create temporary framebuffer to hold the image until we downscale and blit
  // it into our main framebuffer
  int tempWidth = jpgDecoder.getWidth(), tempHeight = jpgDecoder.getHeight();
  RGB565 *tempFb = (RGB565 *)malloc(tempWidth * tempHeight * sizeof(RGB565));
  tgx::Image<tgx::RGB565> tempImg(tempFb, tempWidth, tempHeight);

  // Use tgx shortcuts for JPEGDEC to process the data into our framebuffer
  uint8_t result = tempImg.JPEGDecode(jpgDecoder, tgx::iVec2(0, 0));
  jpgDecoder.close();

  if (!result) {
    // If it fails, make sure we free the malloc
    free(tempFb);
    return false;
  }

  blitWithScalingAndAutorotation(tempImg);

  // Having blitted, we can now remove the temp framebuffer.
  free(tempFb);

  return true;
}

size_t Image::exportJpeg(uint8_t *outBuffer, uint16_t outLength) {
  JPEGENCODE jpe;

  jpgEncoder.open(outBuffer, outLength);
  jpgEncoder.encodeBegin(&jpe, width_, height_, JPEGE_PIXEL_RGB565,
                         JPEGE_SUBSAMPLE_444, JPEGE_Q_HIGH);
  jpgEncoder.addFrame(&jpe, (uint8_t *)frameBuffer_, width_ * 2);
  return jpgEncoder.close();
}

void Image::blitWithScalingAndAutorotation(tgx::Image<tgx::RGB565> srcImg) {
  // Determine centre-points for our blitting routines.
  coord thisCentre(width_ / 2, height_ / 2);
  coord tempCentre(srcImg.width() / 2, srcImg.height() / 2);

  if (srcImg.width() > width_ || srcImg.height() > height_) {
    // Determine scale factor; the blit function maintains aspect ratio

    float scaleFactor = srcImg.width() > srcImg.height()
                            ? (float)width_ / (float)srcImg.width()
                            : (float)height_ / (float)srcImg.height();

    im_.blitScaledRotated(srcImg, tempCentre, thisCentre, scaleFactor,
                      autoRotation);

    // If the image is the same size or smaller, just blit it into place,
    // centred.
  } else {
    // Use the scaled rotated blit due to autorotation, but fix scale at 1.0
    im_.blitScaledRotated(srcImg, tempCentre, thisCentre, 1.0, autoRotation);
  }
}

#if STREAMDECK_IMAGE_HELPER_USE_SD
// The below overloaded routines are only used when the SD is in use. Assumes SD
// has been set up correctly.

bool Image::importJpeg(File file) {
  if (!jpgDecoder.open(file, TGX_JPEGDraw))
    return false;

  // Create temporary framebuffer to hold the image until we downscale and blit
  // it into our main framebuffer
  int tempWidth = jpgDecoder.getWidth(), tempHeight = jpgDecoder.getHeight();
  RGB565 *tempFb = (RGB565 *)malloc(tempWidth * tempHeight * sizeof(RGB565));
  tgx::Image<tgx::RGB565> tempImg(tempFb, tempWidth, tempHeight);

  // Use tgx shortcuts for JPEGDEC to process the data into our framebuffer
  uint8_t result = tempImg.JPEGDecode(jpgDecoder, coord(0, 0));
  jpgDecoder.close();

  if (!result) {
    // If it fails, make sure we free the malloc
    free(tempFb);
    return false;
  }

  blitWithScalingAndAutorotation(tempImg);

  // Having blitted, we can now remove the temp framebuffer.
  free(tempFb);

  return true;
}

bool Image::importJpeg(const char *filePath) {
  if (!SD.exists(filePath))
    return false;

  File file = SD.open(filePath);
  if (!jpgDecoder.open(file, TGX_JPEGDraw)) {
    file.close();
    return false;
  }

  // Create temporary framebuffer to hold the image until we downscale and blit
  // it into our main framebuffer
  int tempWidth = jpgDecoder.getWidth(), tempHeight = jpgDecoder.getHeight();
  RGB565 *tempFb = (RGB565 *)malloc(tempWidth * tempHeight * sizeof(RGB565));
  tgx::Image<tgx::RGB565> tempImg(tempFb, tempWidth, tempHeight);

  // Use tgx shortcuts for JPEGDEC to process the data into our framebuffer
  uint8_t result = tempImg.JPEGDecode(jpgDecoder, coord(0, 0));
  jpgDecoder.close();
  file.close();

  if (!result) {
    // If it fails, make sure we free the malloc
    free(tempFb);
    return false;
  }

  blitWithScalingAndAutorotation(tempImg);

  // Having blitted, we can now remove the temp framebuffer.
  free(tempFb);

  return true;
}

bool Image::importJpegRandom(const char *directory) {
  if (!SD.exists(directory))
    return false;

  File dir = SD.open(directory);
  if (!dir.isDirectory()) {
    dir.close();
    return false;
  }

  // Iterate through files to get a total count
  uint16_t fileCount = 0;
  while (File fh = dir.openNextFile()) {
    if (!fh.isDirectory()) {
      fileCount++;
    }
  }

  // Pick a file number and start over
  uint16_t randPick = Entropy.random(fileCount);
  dir.rewindDirectory();

  // Select our random file handle for use below
  File fh;
  while (fh = dir.openNextFile()) {
    if (!fh.isDirectory()) {
      if (randPick == 0) {
        break;
      }
      randPick--;
    }
  }
  importJpeg(fh);

  fh.close();
  dir.close();
  return true;
}
#endif // STREAMDECK_IMAGE_HELPER_USE_SD

bool Image::importJpegRandom(uint8_t *arrayOfJpgs[], uint16_t sizes[], size_t jpgCount) {
  size_t randPick = Entropy.random(jpgCount);
  uint8_t* element = arrayOfJpgs[randPick];
  uint16_t size = sizes[randPick];
  return importJpeg(element, size);
}

bool Image::sendToKey(StreamdeckController *sdc, uint16_t keyIndex) {
  // Allocate
  uint8_t *tempJpgBuffer =
      (uint8_t *)calloc(STREAMDECK_IMAGE_HELPER_OUT_BUFFER_SIZE, 1);
  size_t jpgSize = exportJpeg(tempJpgBuffer, STREAMDECK_IMAGE_HELPER_OUT_BUFFER_SIZE);
  sdc->setKeyImage(keyIndex, tempJpgBuffer, jpgSize);
  free(tempJpgBuffer);
  return true;
}

void Image::transform(float scaleFactor, float rotationDegrees, RGB565 backgroundColour) {
  // Allocate same-sized framebuffer for temporary holding of the image
  RGB565 *tempFb = (RGB565 *)malloc(width_ * height_ * sizeof(RGB565));
  tgx::Image<tgx::RGB565> tempImg(tempFb, width_, height_);

  tempImg.copyFrom(im_);

  coord half(width_ / 2, height_ / 2);

  // Clear the first image and blit & transform the copy onto it.
  im_.clear(backgroundColour);
  im_.blitScaledRotated(tempImg, half, half, scaleFactor, rotationDegrees);

  free(tempFb);
}

} // namespace Streamdeck

#endif // STREAMDECK_IMAGE_HELPER_ENABLE