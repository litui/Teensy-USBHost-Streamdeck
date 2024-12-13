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

enum report_type_t {
	HID_REPORT_TYPE_UNKNOWN = 0,
	HID_REPORT_TYPE_IN = 1,
	HID_REPORT_TYPE_OUT = 2,
	HID_REPORT_TYPE_FEATURE = 3,
};

struct __attribute__ ((packed)) streamdeck_in_report_type_t {
	uint8_t reportType;
	uint8_t headerField1;
	uint8_t headerField2;
	uint8_t headerField3;
	uint8_t states[508];
};

struct __attribute__ ((packed)) streamdeck_out_report_type_t {
  uint8_t reportType;
  uint8_t command;
  uint8_t buttonId;
  uint8_t isFinal;
  uint16_t payloadLength;  // little endian
  uint16_t payloadNumber;  // little endian
  uint8_t payload[1016];
};

struct __attribute__ ((packed)) streamdeck_feature_report_type_t {
  uint8_t reportType;
  uint8_t command;
  uint8_t value;
  uint8_t filler[29];
};

uint8_t feature_buffer[512];

void dump_hexbytes(const void *ptr, uint32_t len, uint32_t indent) {
  if (ptr == NULL || len == 0) return;
  uint32_t count = 0;
  //  if (len > 64) len = 64; // don't go off deep end...
  const uint8_t *p = (const uint8_t *)ptr;
  while (len--) {
    if (*p < 16) Serial.print('0');
    Serial.print(*p++, HEX);
    count++;
    if (((count & 0x1f) == 0) && len) {
      Serial.print("\n");
      for (uint32_t i = 0; i < indent; i++) Serial.print(" ");
    } else
      Serial.print(' ');
  }
  Serial.println();
}

void StreamdeckController::init()
{
	USBHost::contribute_Transfers(mytransfers, sizeof(mytransfers)/sizeof(Transfer_t));
	USBHost::contribute_Pipes(mypipes, sizeof(mypipes)/sizeof(Pipe_t));
  USBHIDParser::driver_ready_for_hid_collection(this);
}

hidclaim_t StreamdeckController::claim_collection(USBHIDParser *driver, Device_t *dev, uint32_t topusage)
{
	if (mydevice != NULL && dev != mydevice) return CLAIM_NO;

	// Right now, only claim the Stream Deck MkII
	if (dev->idVendor == 0xfd9 && dev->idProduct == 0x80) {
		// 15 keys on the SD MkII
		num_states = 15;
		states = (uint8_t*) calloc(num_states, sizeof(uint8_t));
	}

	mydevice = dev;
	collections_claimed++;

	driver_ = driver;
	driver_->setTXBuffers(drv_tx1_, drv_tx2_, 0);

	if (claimedFunction) (*claimedFunction)(this);

	return CLAIM_INTERFACE;
}

void StreamdeckController::disconnect_collection(Device_t *dev) {
  if (--collections_claimed == 0) {
		free(states);
		num_states = 0;
    mydevice = NULL;
  }
}

bool StreamdeckController::hid_process_in_data(const Transfer_t *transfer) {
  if (transfer->length != 512U)
		return false;

	streamdeck_in_report_type_t* report = (streamdeck_in_report_type_t*) transfer->buffer;
	if (report->reportType != HID_REPORT_TYPE_IN)
		return false;

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
  Serial.printf("HID output, length: %u\n", transfer->length);
	dump_hexbytes(transfer->buffer, transfer->length, 0);
  return true;
}

bool StreamdeckController::hid_process_control(const Transfer_t *transfer) {
	Serial.printf("HID Control...\n");
	// dump_hexbytes(transfer->buffer, transfer->length, 0);
	return false;
}

void StreamdeckController::setBrightness(float percent) {
	driver_->sendControlPacket(3, 8, (uint8_t)min(100, round(percent * 100.0)), 0, 0, feature_buffer);
}

void StreamdeckController::setKeyImage(const uint16_t keyIndex, const uint8_t *image, uint16_t length) {
	const uint16_t bytesPerPage = sizeof(streamdeck_out_report_type_t) - 8;
  uint16_t pageCount = 0;
  uint16_t byteCount = 0;
  // uint16_t totalPages = ceil((float)length / (float)bytesPerPage);

	while (byteCount < length) {
    streamdeck_out_report_type_t report;
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

		// uint8_t report[1024U];
		// report[0] = HID_REPORT_TYPE_OUT;  // type field
		// report[1] = 7;                    // command field
		// // Only an 8 bit field for keyIndex on the report.
		// report[2] = min(255U, keyIndex);   // key field

		// uint16_t sliceLen = min(length - byteCount, bytesPerPage);
    // report[3] = sliceLen != bytesPerPage ? 1 : 0;  // final page field
    // report[4] = (sliceLen >> 8) & 0xFFU;
		// report[5] = sliceLen & 0xFFU;
    // report[6] = (pageCount >> 8) & 0xFFU;
		// report[7] = pageCount & 0xFFU;

		// memcpy(report+8, image + byteCount, sliceLen);
		// for (uint16_t i = bytesPerPage; i >= sliceLen; i--)
		// 	report[i+8] = 0;

		byteCount += sliceLen;
    pageCount++;

		if(!driver_->sendPacket((const uint8_t*)&report, -1)) {
			Serial.println("Failed.");
			break;
		}
	}

	// Set up appropriately sized buffers for transmitting data.
}