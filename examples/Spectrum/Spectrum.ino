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

/* Spectrum Analysis example

   ** This requires a Teensy Audio board with attached microphone or line-in **

   This file has been tested in Arduino IDE 2.3.4 and works fine.

   On loop, will generate an FFT analysis and send it to an attached Stream Deck
   in the form of a 15-16 (depending on number of keys) channel spectrum
   analyzer with slow-falling peak indicators.
*/

#include "streamdeck.h"
#include <Audio.h>

// USB Stuff:

float audioLevels[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float peakLevels[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint32_t lastMillis = 0;
uint32_t jpegCount = 0;

USBHost myusb;
USBHIDParser hid1(myusb);
Streamdeck::StreamdeckController sdc1(myusb);

USBDriver *drivers[] = {&hid1};
#define CNT_DEVICES (sizeof(drivers) / sizeof(drivers[0]))
const char *driver_names[CNT_DEVICES] = {"HID1"};
bool driver_active[CNT_DEVICES] = {false};

USBHIDInput *hiddrivers[] = {&sdc1};
#define CNT_HIDDEVICES (sizeof(hiddrivers) / sizeof(hiddrivers[0]))
const char *hid_driver_names[CNT_HIDDEVICES] = {"StreamDeck1"};
bool hid_driver_active[CNT_HIDDEVICES] = {false};

// Audio stuff:

AudioInputI2S i2s1;
AudioMixer4 mixer1;
// AudioOutputI2S i2s2;
AudioAnalyzeFFT1024 fft1024;
AudioConnection patchCord1(i2s1, 0, mixer1, 0);
// AudioConnection patchCord2(i2s1, 0, i2s2, 0);
AudioConnection patchCord3(i2s1, 1, mixer1, 1);
// AudioConnection patchCord4(i2s1, 1, i2s2, 1);
AudioConnection patchCord5(mixer1, fft1024);
AudioControlSGTL5000 audioShield; // xy=366,225

const int myInput = AUDIO_INPUT_MIC;

void setup() {
  // Initialize USB
  myusb.begin();

  // Audio requires memory to work.
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(0.5);

  mixer1.gain(0, 0.8);
  mixer1.gain(1, 0.8);
}

void loop() {
  using namespace Streamdeck;
  myusb.Task();

  uint32_t currentMillis = millis();

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

        // Adding initialization state and callbacks should go here.
        // StreamdeckController *sdc = (StreamdeckController*) hiddrivers[i];
      }
    }
    if (hid_driver_active[i]) {
      StreamdeckController *sdc = (StreamdeckController *)hiddrivers[i];
      device_settings_t *settings = sdc->getSettings();
      sdc->Task();
      getLevels(audioLevels);

      const uint8_t kRows = settings->keyRows;
      const uint8_t kCols = settings->keyCols;
      const uint16_t kWidth = settings->keyWidth;
      const uint16_t kHeight = settings->keyHeight;

      const uint8_t colsPerImg = 16 / kCols;
      const uint8_t totalCols = colsPerImg * kCols;

      // Holder for gradually descending values
      const float reductionFactor = 0.002;

      // The peak indicators need to slowly descend.
      for (uint8_t i = 0; i < totalCols; i++) {
        if (audioLevels[i] > peakLevels[i]) {
          peakLevels[i] = audioLevels[i];
        } else {
          peakLevels[i] -= reductionFactor;
        }
      }

      // uint8_t y = 2;
      for (uint8_t y = 0; y < kRows; y++) {
        RGB565 colour = tgx::RGB565_Green;
        if (y == 0) {
          colour = tgx::RGB565_Red;
        } else if (y == 1) {
          colour = tgx::RGB565_Yellow;
        }
        uint8_t cTotal = 0;
        for (uint8_t x = 0; x < kCols; x++) {
          Image im(settings);
          im.IM()->fillScreen(tgx::RGB565_Black);

          for (uint8_t c = 0; c < colsPerImg; c++) {
            uint16_t colWidth = kWidth / colsPerImg;

            float audioPeak = audioLevels[cTotal];
            float peakLine = peakLevels[cTotal];
            float rowSplit = (1.0 / (float)(kRows)) * (kRows - y - 1);

            float splitPeak = audioPeak / (float)kRows;

            uint16_t barLeft = c * colWidth;
            uint16_t barRight = (c * colWidth) + colWidth - 2;
            uint16_t barTop = 0;
            uint16_t barBottom = kHeight * (audioPeak - rowSplit) * kRows;
            tgx::iBox2 bar(barLeft, barRight, barTop, barBottom);

            if (barBottom != barTop)
              im.IM()->fillRect(bar, colour);

            uint16_t barPeak = kHeight * (peakLine - rowSplit) * kRows;
            if (barPeak != barTop)
              im.IM()->drawThickLineAA(tgx::fVec2(barLeft, barPeak),
                                       tgx::fVec2(barRight, barPeak), 2.0,
                                       tgx::END_ROUNDED, tgx::END_ROUNDED,
                                       tgx::RGB565_White, 1.0);

            cTotal++;
          }
          im.sendToKey(sdc, x + (y * kCols));
          jpegCount++;
        }
      }

      // Track JPGs per second and fps.
      if (currentMillis > lastMillis + 1000) {
        lastMillis = currentMillis;
        printf("JPGs per second: %lu, fps: %lu\n", jpegCount,
               jpegCount / settings->keyCount);
        jpegCount = 0;
      }
      delay(1);
    }
  }
}

bool getLevels(float *level) {
  if (fft1024.available()) {
    level[0] = fft1024.read(0);
    level[1] = fft1024.read(1);
    level[2] = fft1024.read(2, 3);
    level[3] = fft1024.read(4, 6);
    level[4] = fft1024.read(7, 10);
    level[5] = fft1024.read(11, 15);
    level[6] = fft1024.read(16, 22);
    level[7] = fft1024.read(23, 32);
    level[8] = fft1024.read(33, 46);
    level[9] = fft1024.read(47, 66);
    level[10] = fft1024.read(67, 93);
    level[11] = fft1024.read(94, 131);
    level[12] = fft1024.read(132, 184);
    level[13] = fft1024.read(185, 257);
    level[14] = fft1024.read(258, 359);
    level[15] = fft1024.read(360, 511);

    return true;
  }
  return false;
}