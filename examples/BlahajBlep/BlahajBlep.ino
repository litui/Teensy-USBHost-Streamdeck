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
*/

/* Simple Blahaj Blep buttons example

   This file has not yet been tested in the Arduino IDE so might need slight
   changes.

   On load, will put blobhaj emoji onto each button. When buttons are pressed
   the blobhaj will blep/mlem. When released, the blobhaj will return to its
   neutral state.

*/
#include "images.h"
#include "streamdeck.hpp"
#include <USBHost_t36.h>

namespace USB {

USBHost myusb;
USBHIDParser hid1(myusb);
StreamdeckController sdc1(myusb);

USBDriver *drivers[] = {&hid1};
#define CNT_DEVICES (sizeof(drivers) / sizeof(drivers[0]))
const char *driver_names[CNT_DEVICES] = {"HID1"};
bool driver_active[CNT_DEVICES] = {false};

// Lets also look at HID Input devices
USBHIDInput *hiddrivers[] = {&sdc1};
#define CNT_HIDDEVICES (sizeof(hiddrivers) / sizeof(hiddrivers[0]))
const char *hid_driver_names[CNT_HIDDEVICES] = {"StreamDeck1"};
bool hid_driver_active[CNT_HIDDEVICES] = {false};

void init() { myusb.begin(); }

void tick() {
  myusb.Task();

  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i],
                      drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz)
          Serial.printf("  manufacturer: %s\n", psz);
        psz = drivers[i]->product();
        if (psz && *psz)
          Serial.printf("  product: %s\n", psz);
        psz = drivers[i]->serialNumber();
        if (psz && *psz)
          Serial.printf("  Serial: %s\n", psz);
      }
    }
  }

  for (uint8_t i = 0; i < CNT_HIDDEVICES; i++) {
    if (*hiddrivers[i] != hid_driver_active[i]) {
      if (hid_driver_active[i]) {
        Serial.printf("*** HID Device %s - disconnected ***\n",
                      hid_driver_names[i]);
        hid_driver_active[i] = false;
      } else {
        Serial.printf("*** HID Device %s %x: %x - connected ***\n",
                      hid_driver_names[i], hiddrivers[i]->idVendor(),
                      hiddrivers[i]->idProduct());
        hid_driver_active[i] = true;

        const uint8_t *psz = hiddrivers[i]->manufacturer();
        if (psz && *psz)
          Serial.printf("  manufacturer: %s\n", psz);
        psz = hiddrivers[i]->product();
        if (psz && *psz)
          Serial.printf("  product: %s\n", psz);
        psz = hiddrivers[i]->serialNumber();
        if (psz && *psz)
          Serial.printf("  Serial: %s\n", psz);

        StreamdeckController *sdc = (StreamdeckController *)hiddrivers[i];
        sdc->attachSinglePress(buttonPressed);
        sdc->flushImageReports();

        for (uint8_t i = 0; i < 15; i++) {
          sdc->setKeyImage(i, image_released, sizeof(image_released));
        }
      }
    }
  }
}

void buttonPressed(StreamdeckController *sdc, uint16_t keyIndex,
                   uint8_t newValue, uint8_t oldValue) {
  if (newValue == 1) {
    sdc->setKeyImage(keyIndex, image_pressed, sizeof(image_pressed));
  } else {
    sdc->setKeyImage(keyIndex, image_released, sizeof(image_released));
  }
}