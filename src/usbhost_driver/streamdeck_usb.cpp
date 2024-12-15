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
#include "streamdeck_usb.hpp"

namespace Streamdeck {

void StreamdeckController::init() {
  USBHost::contribute_Pipes(mypipes, sizeof(mypipes) / sizeof(Pipe_t));
  USBHost::contribute_Transfers(mytransfers,
                                sizeof(mytransfers) / sizeof(Transfer_t));
  USBHIDParser::driver_ready_for_hid_collection(this);
}

hidclaim_t StreamdeckController::claim_collection(USBHIDParser *driver,
                                                  Device_t *dev,
                                                  uint32_t topusage) {
  if (mydevice != NULL && dev != mydevice)
    return CLAIM_NO;

  if (dev->idVendor != USB_VID_ELGATO)
    return CLAIM_NO;

  for (uint8_t i = 0; i < sizeof(DeviceList)/sizeof(device_settings_t); i++) {
    if (dev->idProduct == DeviceList[i].productId) {
      Serial.println("Found product!");
      settings = (device_settings_t*)&DeviceList[i];
    }
  }
  if (!settings)
    return CLAIM_NO;
  
  states = (uint8_t *)calloc(settings->keyCount, sizeof(uint8_t));

  mydevice = dev;
  collections_claimed++;

  driver_ = driver;
  driver_->setTXBuffers(drv_tx1_, drv_tx2_, 0);

  return CLAIM_INTERFACE;
}

void StreamdeckController::disconnect_collection(Device_t *dev) {
  if (--collections_claimed == 0U) {
    free(states);
    settings = nullptr;
    mydevice = NULL;
  }
}

bool StreamdeckController::hid_process_in_data(const Transfer_t *transfer) {
  if (transfer->length != 512U)
    return false;

  report_type_512_4_in_t *report =
      (report_type_512_4_in_t *)transfer->buffer;
  if (report->reportType != HID_REPORT_TYPE_IN)
    return false;

  int16_t changed_index = -1;
  for (uint16_t i = 0; i < settings->keyCount; i++) {
    // Report differences in key states.
    if (report->states[i] != states[i]) {
      // Serial.printf("hid_process_in_data called: key %u changed to state
      // %u\n", i, report->states[i]);
      if (singleStateChangedFunction)
        // Hook to user function.
        singleStateChangedFunction(this, i, report->states[i], states[i]);
      changed_index = i;
    }
  }
  if (changed_index > -1) {
    // USBHDBGSerial.printf("States changed! index %d\n", changed_index);
    if (anyStateChangedFunction)
      // Hook to user function.
      anyStateChangedFunction(this, report->states, states);
  }
  memcpy(states, report->states, settings->keyCount);
  return true;
}

bool StreamdeckController::hid_process_out_data(const Transfer_t *transfer) {
  // USBHDBGSerial.printf("HID output success, length: %u\n", transfer->length);
  if (!pending_out_reports.empty()) {
    report_type_1024_8_out_t *report =
        (report_type_1024_8_out_t *)pending_out_reports.front();
    // USBHDBGSerial.printf("Resuming transfer of payload #%u.\n",
    // report->payloadNumber);
    if (driver_->sendPacket((const uint8_t *)report,
                            sizeof(report_type_1024_8_out_t))) {
      pending_out_reports.pop();
    }
  }
  return true;
}

bool StreamdeckController::hid_process_control(const Transfer_t *transfer) {
  // USBHDBGSerial.printf("HID Control...\n");
  return true;
}

bool StreamdeckController::setReport(const uint8_t reportType,
                                     const uint8_t reportId,
                                     const uint8_t interface, void *report,
                                     const uint16_t length) {
  return driver_->sendControlPacket(
      0x21U, // bmRequestType = 00100001, Class-specific requests
      0x09U, // SET_REPORT (s. 7.2.2 of DCD for USB HID v1.11)
      (uint16_t)((reportType << 8) | reportId), interface, length, report);
}

void StreamdeckController::setBrightness(float percent) {
  static report_type_32_3_out_t report;
  report.reportType = 0x03;
  report.request = 0x08;
  report.value = (uint8_t)min(100, max(percent * 100, 0));
  for (uint8_t i = 0; i < sizeof(report.filler); i++) {
    report.filler[i] = 0;
  }

  setReport(report.reportType, 0, 0, &report, sizeof(report));
}

void StreamdeckController::reset() {
  // SET_IDLE
  driver_->sendControlPacket(0x21, 0xa, 0, 0, 0, nullptr);

#if STREAMDECK_USBHOST_ENABLE_RESET
  // SET_REPORT
  // This reset routine triggers a port change, which is a problem for
  // USBHost_t36.
  static streamdeck_feature_report_type_t report;
  report.reportType = 0x03;
  report.request = 0x02;
  report.value = 0;
  for (uint8_t i = 0; i < sizeof(report.filler); i++) {
    report.filler[i] = 0;
  }
  setReport(report.reportType, 0, 0, &report, sizeof(report));
#endif // STREAMDECK_USBHOST_ENABLE_RESET
}

// Sends a blank outbound report to the device and empties the pending queue.
void StreamdeckController::flushImageReports() {
  for (uint8_t i = 0; i < pending_out_reports.size(); i++) {
    pending_out_reports.pop();
  }

  // static streamdeck_out_report_type_t report;
  // report.reportType = HID_REPORT_TYPE_OUT;
  // report.command = 0;
  // report.buttonId = 0;
  // report.isFinal = 0;
  // report.payloadLength = 0;
  // report.payloadNumber = 0;
  // for (uint8_t i = 0; i < sizeof(report.payload); i++) {
  //   report.payload[i] = 0;
  // }
  // out_report.write((uint8_t *)&report, sizeof(report));

  // driver_->sendPacket((const uint8_t *)out_report.peek_back(),
  // sizeof(report));
}

#if STREAMDECK_USBHOST_ENABLE_BLANK_IMAGE
// Sets a blank (black) image to the given key
void StreamdeckController::setKeyBlank(const uint16_t keyIndex) {
  setKeyImage(keyIndex, BLANK_KEY_IMAGE, sizeof(BLANK_KEY_IMAGE));
}

void StreamdeckController::blankAllKeys() {
  for (uint16_t i = 0; i < settings->keyCount; i++) {
    setKeyBlank(i);
  }
  delay(5);
}
#endif // STREAMDECK_USBHOST_ENABLE_BLANK_IMAGE

// Sets a jpeg image of the given length to the given key
void StreamdeckController::setKeyImage(const uint16_t keyIndex,
                                       const uint8_t *image, uint16_t length) {
  const uint16_t bytesPerPage = sizeof(report_type_1024_8_out_t) - 8;
  uint16_t pageCount = 0;
  uint16_t byteCount = 0;

  // Wait for the queue to be empty before starting a new transfer.
  // while(!pending_out_reports.empty()) { delay(1); }

  // Separate the image into chunks that fit into our 1024 bytes reports, set
  // headers, and attempt to transfer them. If transfer buffers are full, adds
  // their memory addresses to a queue which will be dealt with when previous
  // transfers succeed.
  //
  // Logic adapted from:
  // - https://den.dev/blog/reverse-engineering-stream-deck/
  while (byteCount < length) {
    report_type_1024_8_out_t report;

    // Serial.printf("Page count: %u\n", pageCount);
    report.reportType = HID_REPORT_TYPE_OUT;
    report.command = 7;
    report.buttonId = keyIndex;

    uint16_t sliceLen = min(length - byteCount, bytesPerPage);
    report.payloadLength = sliceLen;
    report.isFinal = sliceLen != bytesPerPage ? 1 : 0;
    report.payloadNumber = pageCount;

    memcpy(report.payload, image + byteCount, sliceLen);
    for (uint16_t i = bytesPerPage; i >= sliceLen; i--) {
      report.payload[i] = 0;
    }

    out_report.write((uint8_t *)&report, sizeof(report));

    byteCount += sliceLen;
    pageCount++;

    if (!driver_->sendPacket((const uint8_t *)out_report.peek_back(),
                             sizeof(report))) {

      pending_out_reports.push((uint32_t)out_report.peek_back());
    } else {
      // Delay inserted to ensure the ISR has time to process.
      delay(1);
    }
  }
}

} // namespace Streamdeck