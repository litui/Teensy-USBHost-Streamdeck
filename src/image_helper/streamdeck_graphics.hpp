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
#pragma once
#include "device_specifics.hpp"
#include "streamdeck_config.hpp"
#include "usbhost_driver/streamdeck_usb.hpp"
#include <Arduino.h>

#if STREAMDECK_IMAGE_HELPER_ENABLE

#include <JPEGDEC.h>
#include <JPEGENC.h>
#include <PNGdec.h>
#include <tgx.h>

#if STREAMDECK_IMAGE_HELPER_USE_SD
#include <SD.h>
#endif // STREAMDECK_IMAGE_HELPER_USE_SD

namespace Streamdeck {

typedef tgx::RGB565 RGB565;
typedef tgx::iVec2 coord;

class Image {
public:
  Image(int width, int height) {
    width_ = width;
    height_ = height;
    allocateFrameBuffer(width_, height_);
    im_ = tgx::Image<tgx::RGB565>(frameBuffer_, width_, height_);
  };
  Image(device_settings_t *settings) {
    autoRotation = (float)settings->keyRotation * 90.0f;
    autoFlipH = settings->keyFlipH;
    autoFlipV = settings->keyFlipV;
    width_ = settings->keyWidth;
    height_ = settings->keyHeight;
    allocateFrameBuffer(width_, height_);
    im_ = tgx::Image<tgx::RGB565>(frameBuffer_, width_, height_);
  }
  Image(RGB565 *frameBuffer, int width, int height) {
    width_ = width;
    height_ = height;
    frameBuffer_ = frameBuffer;
    im_ = tgx::Image<tgx::RGB565>(frameBuffer_, width_, height_);
  }
  Image(RGB565 *frameBuffer, device_settings_t *settings) {
    autoRotation = (float)settings->keyRotation * 90.0f;
    autoFlipH = settings->keyFlipH;
    autoFlipV = settings->keyFlipV;
    width_ = settings->keyWidth;
    height_ = settings->keyHeight;
    frameBuffer_ = frameBuffer;
    im_ = tgx::Image<tgx::RGB565>(frameBuffer_, width_, height_);
  }
  ~Image() { freeFrameBuffer(); }

  // Import
  bool importJpeg(const uint8_t *inBuffer, const uint16_t inLength);
  bool importJpegRandom(uint8_t *arrayOfJpgs[], uint16_t sizes[],
                        size_t jpgCount);
  // bool importPng(const uint8_t* buffer, const uint16_t bufferLen);
#if STREAMDECK_IMAGE_HELPER_USE_SD
  bool importJpeg(File file);
  bool importJpeg(const char *filePath);
  bool importJpegRandom(const char *directory);
  // bool importPng(File file);
  // bool importPng(const char* path);
#endif // STREAMDECK_IMAGE_HELPER_USE_SD

  // Export
  size_t exportJpeg(uint8_t *outBuffer, uint16_t outLength);

  // USB Shortcuts
  bool sendToKey(StreamdeckController *sdc, uint16_t keyIndex);

  // Graphical manipulations
  void fill(RGB565 colour = tgx::RGB565_Black);
  void transform(float scaleFactor, float rotationDegrees,
                 RGB565 backgroundColour = tgx::RGB565_Black);
  void rotate3d(float xRotationDegrees, float yRotationDegrees,
                float zRotationDegrees);

private:
  tgx::Image<tgx::RGB565> im_;

  JPEGENC jpgEncoder;
  JPEGDEC jpgDecoder;
  PNG pngDecoder;

  void allocateFrameBuffer(const uint16_t width, const uint16_t height);
  void freeFrameBuffer();

  // Graphical shortcuts
  void blitWithScalingAndAutorotation(tgx::Image<tgx::RGB565> srcImg);

  RGB565 *frameBuffer_ = nullptr;
  bool madeFrameBuffer = false;
  int width_ = 0;
  int height_ = 0;
  float autoRotation = 0.0f;
  bool autoFlipH = false;
  bool autoFlipV = false;
};

} // namespace Streamdeck

#endif // STREAMDECK_IMAGE_HELPER_ENABLE
