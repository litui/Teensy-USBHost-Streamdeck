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

/* All options in this file can be overridden using build directives in
 * PlatformIO */

// This number of report buffers should be enough for 2-3 images to fully
// buffer. The default setting aims for around 3-5 kBytes max per JPG being sent
// to the streamdeck (remove exif data). If your images are significantly
// larger, you may want to increase this value. Each buffer takes up 1024 Bytes.
// Buffer count must be an exponent of 2. These are persistent in a circular
// buffer and the memory is not freed.
#ifndef STREAMDECK_USBHOST_OUTPUT_BUFFERS
#define STREAMDECK_USBHOST_OUTPUT_BUFFERS 4U
#endif // STREAMDECK_USBHOST_OUTPUT_BUFFERS

// Resetting the streamdeck causes USBHost_t36 Pipes to break. If you still want
// to do this anyway, set value to 1
#ifndef STREAMDECK_USBHOST_ENABLE_RESET
#define STREAMDECK_USBHOST_ENABLE_RESET 0U
#endif // STREAMDECK_USBHOST_ENABLE_RESET

// Choose whether or not to include the default blank keyimage at compile time.
// Defaults to enabled. Disabling this also removes the setKeyBlank and
// blankAllKeys functions.
#ifndef STREAMDECK_USBHOST_ENABLE_BLANK_IMAGE
#define STREAMDECK_USBHOST_ENABLE_BLANK_IMAGE 1U
#endif // STREAMDECK_USBHOST_ENABLE_BLANK_IMAGE

