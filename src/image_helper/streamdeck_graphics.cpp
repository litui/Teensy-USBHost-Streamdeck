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

#include <JPEGDEC.h>
#include <JPEGENC.h>
#include <PNGdec.h>
#include <circular_buffer.h>

JPEGENC jpgEncoder;

namespace Streamdeck {
size_t createKeyJpeg(const device_settings_t *settings, tgx::RGB565 colour, uint8_t *buffer, size_t bufferSize) {
  JPEGENCODE jpe;

  const uint16_t x = settings->keyHeight, y = settings->keyWidth;
  uint16_t fb[x * y];

  tgx::Image<tgx::RGB565> im(fb, x, y);
  im.fillScreen(colour);

  jpgEncoder.open(buffer, bufferSize);
  jpgEncoder.encodeBegin(&jpe, x, y, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420, JPEGE_Q_HIGH);
  jpgEncoder.addFrame(&jpe, (uint8_t*)fb, 16);
  size_t dataSize = jpgEncoder.close();

  Serial.printf("Jpeg encoded, size: %lu\n", dataSize);

  return dataSize;
}


} // namespace Streamdeck

#endif // STREAMDECK_IMAGE_HELPER_ENABLE