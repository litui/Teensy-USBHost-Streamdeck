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
#include <JPEGDEC.h>
#include <JPEGENC.h>
#include <PNGdec.h>

JPEGENC jpgEncoder;
JPEGDEC jpgDecoder;
PNG pngDecoder;

namespace Streamdeck {

tgx::RGB565 *outboundFrameBuffer;
tgx::Image<tgx::RGB565> outboundSprite;

size_t createKeyJpeg(const device_settings_t *settings, tgx::RGB565 colour,
                     uint8_t *buffer, size_t bufferSize) {
  JPEGENCODE jpe;

  const uint16_t x = settings->keyHeight, y = settings->keyWidth;
  tgx::RGB565 fb[x * y];

  tgx::Image<tgx::RGB565> im(fb, x, y);
  im.fillScreen(colour);

  jpgEncoder.open(buffer, bufferSize);
  jpgEncoder.encodeBegin(&jpe, x, y, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420,
                         JPEGE_Q_HIGH);
  jpgEncoder.addFrame(&jpe, (uint8_t *)fb, 72);
  size_t dataSize = jpgEncoder.close();

  return dataSize;
}

size_t autoTransformJpeg(const device_settings_t *settings, uint8_t *inBuffer,
                         size_t inBufSize, uint8_t *outBuffer,
                         size_t outBufSize) {
  return rotateAndScaleJpeg(settings, inBuffer, inBufSize, outBuffer, outBufSize, settings->keyRotation*90, 1.0);
}

size_t flipJpeg(const device_settings_t *settings, uint8_t *inBuffer,
                          size_t inBufSize, uint8_t *outBuffer,
                          size_t outBufSize, bool left, bool right) {
  // TODO: figure out how flipping is going to work.
}

size_t rotateAndScaleJpeg(const device_settings_t *settings, uint8_t *inBuffer,
                          size_t inBufSize, uint8_t *outBuffer,
                          size_t outBufSize, float degrees, float scale) {
  const uint16_t x = settings->keyHeight, y = settings->keyWidth;
  tgx::RGB565 mainFrameBuffer[x * y];
  tgx::Image<tgx::RGB565> im(mainFrameBuffer, x, y);

  outboundFrameBuffer = (tgx::RGB565 *)calloc(outBufSize, sizeof(tgx::RGB565));
  outboundSprite = tgx::Image<tgx::RGB565>(outboundFrameBuffer, x, y);
  im.fillScreen(tgx::RGB565_Black);
  outboundSprite.fillScreen(tgx::RGB565_Black);

  jpgDecoder.openRAM(inBuffer, inBufSize, TGX_JPEGDraw);

  if (!outboundSprite.JPEGDecode(jpgDecoder, tgx::iVec2(0, 0))) {
    free(outboundFrameBuffer);
    return 0;
  }

  im.blitScaledRotated(outboundSprite, tgx::iVec2(35, 35), tgx::iVec2(35, 35),
                       scale, degrees);

  free(outboundFrameBuffer);

  JPEGENCODE jpe;

  jpgEncoder.open(outBuffer, outBufSize);
  jpgEncoder.encodeBegin(&jpe, x, y, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_444,
                         JPEGE_Q_HIGH);
  jpgEncoder.addFrame(&jpe, (uint8_t *)mainFrameBuffer, 144);
  return jpgEncoder.close();
}

} // namespace Streamdeck

#endif // STREAMDECK_IMAGE_HELPER_ENABLE