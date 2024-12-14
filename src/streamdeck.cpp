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

Some stuff borrowed from the python-elgato-streamdeck library.
Credit to:
   dean [at] fourwalledcubicle [dot] com
         www.fourwalledcubicle.com
*/
#include "streamdeck.hpp"

void StreamdeckController::init() {
  USBHost::contribute_Transfers(mytransfers,
                                sizeof(mytransfers) / sizeof(Transfer_t));
  USBHIDParser::driver_ready_for_hid_collection(this);
}

hidclaim_t StreamdeckController::claim_collection(USBHIDParser *driver,
                                                  Device_t *dev,
                                                  uint32_t topusage) {
  if (mydevice != NULL && dev != mydevice)
    return CLAIM_NO;

  // Right now, only claim the Stream Deck MkII
  if (dev->idVendor == 0xfd9 && dev->idProduct == 0x80) {
    // 15 keys on the SD MkII
    num_states = 15U;
    states = (uint8_t *)calloc(num_states, sizeof(uint8_t));
  } else
    return CLAIM_NO;

  mydevice = dev;
  collections_claimed++;

  driver_ = driver;
  driver_->setTXBuffers(drv_tx1_, drv_tx2_, 0);

  return CLAIM_INTERFACE;
}

void StreamdeckController::disconnect_collection(Device_t *dev) {
  if (--collections_claimed == 0U) {
    free(states);
    num_states = 0U;
    mydevice = NULL;
  }
}

bool StreamdeckController::hid_process_in_data(const Transfer_t *transfer) {
  if (transfer->length != 512U)
    return false;

  streamdeck_in_report_type_t *report =
      (streamdeck_in_report_type_t *)transfer->buffer;
  if (report->reportType != HID_REPORT_TYPE_IN)
    return false;

  // if (num_states != report->stateCount) {
  //   USBHDBGSerial.printf(
  //       "Warning: tracked number of states does not match input state
  //       count!");
  // }

  int16_t changed_index = -1;
  for (uint16_t i = 0; i < num_states; i++) {
    // Report differences in key states.
    if (report->states[i] != states[i]) {
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
    memcpy(states, report->states, num_states);
  }

  return true;
}

bool StreamdeckController::hid_process_out_data(const Transfer_t *transfer) {
  // USBHDBGSerial.printf("HID output success, length: %u\n", transfer->length);
  if (pending_out_reports.size()) {
    streamdeck_out_report_type_t *report =
        (streamdeck_out_report_type_t *)pending_out_reports.front();
    // USBHDBGSerial.printf("Resuming transfer of payload #%u.\n",
    // report->payloadNumber);
    if (driver_->sendPacket((const uint8_t *)report,
                            sizeof(streamdeck_out_report_type_t))) {
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
  // TODO: switch this to rotating buffers in case reports get sent in
  // succession
  static streamdeck_feature_report_type_t report;
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

  // SET_REPORT
  // TODO: switch this to rotating buffers in case reports get sent in
  // succession
  // This reset might trigger a port change, which is a problem.
  static streamdeck_feature_report_type_t report;
  report.reportType = 0x03;
  report.request = 0x02;
  report.value = 0;
  for (uint8_t i = 0; i < sizeof(report.filler); i++) {
    report.filler[i] = 0;
  }
  setReport(report.reportType, 0, 0, &report, sizeof(report));
}

// Sends a blank outbound report to the device and empties the pending queue.
void StreamdeckController::flushImageReports() {
  for (uint8_t i = 0; i < pending_out_reports.size(); i++) {
    pending_out_reports.pop();
  }

  uint8_t *temp_buffer = (uint8_t *)&out_report[current_ob];
  temp_buffer[0] = 2U;
  for (uint16_t i = 1; i < sizeof(streamdeck_out_report_type_t); i++) {
    temp_buffer[i] = 0;
  }
  driver_->sendPacket(temp_buffer, sizeof(streamdeck_out_report_type_t));
  current_ob++;
}

// Sets a blank (black) image to the given key
void StreamdeckController::setKeyBlank(const uint16_t keyIndex) {
  setKeyImage(keyIndex, BLANK_KEY_IMAGE, sizeof(BLANK_KEY_IMAGE));
}

void StreamdeckController::blankAllKeys() {
  for (uint16_t i = 0; i < num_states; i++) {
    setKeyBlank(i);
    delay(1);
  }
}

// Sets a jpeg image of the given length to the given key
void StreamdeckController::setKeyImage(const uint16_t keyIndex,
                                       const uint8_t *image, uint16_t length) {
  const uint16_t bytesPerPage = sizeof(streamdeck_out_report_type_t) - 8;
  uint16_t pageCount = 0;
  uint16_t byteCount = 0;
  // uint16_t totalPages = ceil((float)length / (float)bytesPerPage);

  // Separate the image into chunks that fit into our 1024 bytes reports, set
  // headers, and attempt to transfer them. If transfer buffers are full, adds
  // their memory addresses to a queue which will be dealt with when previous
  // transfers succeed.
  //
  // Logic adapted from:
  // - https://den.dev/blog/reverse-engineering-stream-deck/
  while (byteCount < length) {
    out_report[current_ob].reportType = HID_REPORT_TYPE_OUT;
    out_report[current_ob].command = 7;
    out_report[current_ob].buttonId = keyIndex;

    uint16_t sliceLen = min(length - byteCount, bytesPerPage);
    out_report[current_ob].payloadLength = sliceLen;
    out_report[current_ob].isFinal = sliceLen != bytesPerPage ? 1 : 0;
    out_report[current_ob].payloadNumber = pageCount;

    memcpy(out_report[current_ob].payload, image + byteCount, sliceLen);
    for (uint16_t i = bytesPerPage; i >= sliceLen; i--) {
      out_report[current_ob].payload[i] = 0;
    }

    byteCount += sliceLen;
    pageCount++;

    if (!driver_->sendPacket((const uint8_t *)&out_report[current_ob],
                             sizeof(streamdeck_out_report_type_t))) {
      // USBHDBGSerial.printf("Failed to send image payload #%u, queueing for
      // later.\n", out_report[current_ob].payloadNumber);

      // Only queue reports up to the maximum output buffer limit, minus 2
      // (buffers already in use).
      if (pending_out_reports.size() < STREAMDECK_IMAGE_OUTPUT_BUFFERS - 2) {
        pending_out_reports.push((uint32_t)&out_report[current_ob]);
      }
    }

    current_ob =
        current_ob < STREAMDECK_IMAGE_OUTPUT_BUFFERS - 1 ? current_ob + 1 : 0;
    delay(1); // Slight delay to ensure ISR has time to work its magic between
              // calls.
  }
}