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
#include <Arduino.h>

#define USB_VID_ELGATO 0x0fd9U

#define USB_PID_STREAMDECK_ORIGINAL 0x0060
#define USB_PID_STREAMDECK_ORIGINAL_V2 0x006d
#define USB_PID_STREAMDECK_MINI 0x0063
#define USB_PID_STREAMDECK_NEO 0x009a
#define USB_PID_STREAMDECK_XL 0x006c
#define USB_PID_STREAMDECK_XL_V2 0x008f
#define USB_PID_STREAMDECK_MK2 0x0080
#define USB_PID_STREAMDECK_PEDAL 0x0086
#define USB_PID_STREAMDECK_MINI_MK2 0x0090
#define USB_PID_STREAMDECK_PLUS 0x0084

namespace Streamdeck {

enum image_format_t { IMAGE_FORMAT_BITMAP = 0, IMAGE_FORMAT_JPEG };

enum key_rotation_t {
  ROTATE_NONE = 0,
  ROTATE_90_DEGREES = 1,
  ROTATE_180_DEGREES = 2,
  ROTATE_270_DEGREES = 3
};

enum control_type_t {
  CONTROL_TYPE_KEY = 0,
  CONTROL_TYPE_DIAL,
  CONTROL_TYPE_TOUCHSCREEN
};

struct device_settings_t {
  uint16_t productId;
  uint16_t keyCount;
  uint8_t keyCols;
  uint8_t keyRows;
  uint16_t keyWidth;
  uint16_t keyHeight;
  image_format_t imageFormat;
  bool keyFlipH;
  bool keyFlipV;
  key_rotation_t keyRotation;
  uint16_t imageReportLength;
  uint16_t imageReportHeaderLength;
};

// For right now, only supporting devices with JPEG image format.
const device_settings_t DeviceList[] = {
    {.productId = USB_PID_STREAMDECK_ORIGINAL_V2,
     .keyCount = 15,
     .keyCols = 5,
     .keyRows = 3,
     .keyWidth = 72,
     .keyHeight = 72,
     .imageFormat = IMAGE_FORMAT_JPEG,
     .keyFlipH = false,
     .keyFlipV = false,
     .keyRotation = ROTATE_180_DEGREES,
     .imageReportLength = 1024,
     .imageReportHeaderLength = 8},
    {.productId = USB_PID_STREAMDECK_MK2,
     .keyCount = 15,
     .keyCols = 5,
     .keyRows = 3,
     .keyWidth = 72,
     .keyHeight = 72,
     .imageFormat = IMAGE_FORMAT_JPEG,
     .keyFlipH = false,
     .keyFlipV = false,
     .keyRotation = ROTATE_180_DEGREES,
     .imageReportLength = 1024,
     .imageReportHeaderLength = 8},
    {.productId = USB_PID_STREAMDECK_XL,
     .keyCount = 32,
     .keyCols = 8,
     .keyRows = 4,
     .keyWidth = 96,
     .keyHeight = 96,
     .imageFormat = IMAGE_FORMAT_JPEG,
     .keyFlipH = false,
     .keyFlipV = false,
     .keyRotation = ROTATE_180_DEGREES,
     .imageReportLength = 1024,
     .imageReportHeaderLength = 8}};

} // namespace Streamdeck