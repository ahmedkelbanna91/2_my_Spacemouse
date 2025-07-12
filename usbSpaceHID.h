// #define MSCUPDATE
#define USBSERIAL




#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#endif

#define USBVID 0x256F
#define USBPID 0xC63A
#define USBMANUFACTURER "ABANNATECH"
#define USBPRODUCT "SpaceMouse Wireless BLE"
#define USBSERIALNO "123456"

#if USBVID > USHRT_MAX
#error USBVID MAX_VALUE = 65535
#endif
#if USBPID > USHRT_MAX
#error USBPID MAX_VALUE = 65535
#endif

#include "USB.h"
#if ARDUINO_USB_ON_BOOT
#error please disable all usb on boot from tools menu
#endif

#ifdef USBSERIAL
USBCDC USBSerial;
#define SERIAL USBSerial
#else
#define SERIAL Serial
#endif

#ifdef MSCUPDATE
#include "FirmwareMSC.h"
FirmwareMSC MSC_Update;
#endif

#include "USBHID.h"
USBHID HID;

static const uint8_t report_descriptor[] = {
  // --- Global Items ---
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x08,  // Usage (Multi-axis Controller)
  0xA1, 0x01,  // Collection (Application)

  // --- Translation Report (ID 1) ---
  0xA1, 0x00, 0x85, 0x01,
  0x16, 0xA2, 0xFE, 0x26, 0x5E, 0x01,
  0x09, 0x30, 0x09, 0x31, 0x09, 0x32,
  0x75, 0x10, 0x95, 0x03, 0x81, 0x02,
  0xC0,

  // --- Rotation Report (ID 2) ---
  0xA1, 0x00, 0x85, 0x02,
  0x16, 0xA2, 0xFE, 0x26, 0x5E, 0x01,
  0x09, 0x33, 0x09, 0x34, 0x09, 0x35,
  0x75, 0x10, 0x95, 0x03, 0x81, 0x02,
  0xC0,

  // --- Button Report (ID 3) ---
  0xA1, 0x00, 0x85, 0x03,
  0x05, 0x09, 0x19, 0x01, 0x29, 0x02,
  0x15, 0x00, 0x25, 0x01,
  0x75, 0x01, 0x95, 0x02, 0x81, 0x02,
  0x75, 0x01, 0x95, 0x0E, 0x81, 0x03,
  0xC0,

  // --- LED Control Report (ID 4) ---
  0xA1, 0x02, 0x85, 0x04,
  0x05, 0x08, 0x09, 0x4B,
  0x15, 0x00, 0x25, 0x01,
  0x95, 0x01, 0x75, 0x01, 0x91, 0x02,
  0x95, 0x01, 0x75, 0x07, 0x91, 0x03,
  0xC0,

  // --- Power Status Report (ID 23) ---
  0x06, 0x00, 0xFF,
  0x85, 0x17,
  0x09, 0x01,
  0x15, 0x00, 0x25, 0x64,
  0x75, 0x08, 0x95, 0x01,
  0x81, 0x02,
  0x09, 0x02,
  0x25, 0x01, 0x95, 0x01,
  0x81, 0x02,

  0xC0  // End Application Collection
};

class SpaceMouseHID_Device : public USBHIDDevice {
public:
  bool ledState = false;

  SpaceMouseHID_Device(void) {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
      HID.addDevice(this, sizeof(report_descriptor));
    }
  }

  void begin(void) {
    HID.begin();
  }

  uint16_t _onGetDescriptor(uint8_t* buffer) {
    memcpy(buffer, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
  }

  void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) override {
    if (report_id == 4 && len >= 1) {
      ledState = (buffer[0] != 0);
      SERIAL.printf("LED state set to %s\n", ledState ? "ON" : "OFF");
    }
  }

  bool getLed() {
    return ledState;
  }

  bool send(uint8_t id, const void* value, size_t len) {
    return HID.SendReport(id, value, len);
  }

  void sendBattery(uint8_t percent, bool charging) {
    static unsigned long lastSent = 0;
    unsigned long now = millis();

    if (now - lastSent >= 1000) {
      lastSent = now;
      uint8_t payload[2];
      payload[0] = constrain(percent, 0, 100);
      payload[1] = charging ? 1 : 0;
      HID.SendReport(23, payload, sizeof(payload));
    }
  }
};

SpaceMouseHID_Device SpaceMouseHID;


char usbState[32] = "USB: Unknown";
char mscState[32] = "MSC: Idle";
char mscProgress[32] = "";

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t* data = (arduino_usb_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
        SERIAL.println("USB PLUGGED");
        snprintf(usbState, sizeof(usbState), "USB: Plugged");
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        SERIAL.println("USB UNPLUGGED");
        snprintf(usbState, sizeof(usbState), "USB: Unplugged");
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        SERIAL.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
        snprintf(usbState, sizeof(usbState), "USB: Suspended");
        break;
      case ARDUINO_USB_RESUME_EVENT:
        SERIAL.println("USB RESUMED");
        snprintf(usbState, sizeof(usbState), "USB: Resumed");
        break;
      default: break;
    }
  }
#ifdef MSCUPDATE
  else if (event_base == ARDUINO_FIRMWARE_MSC_EVENTS) {
    arduino_firmware_msc_event_data_t* data = (arduino_firmware_msc_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_FIRMWARE_MSC_START_EVENT:
        SERIAL.println("MSC Update Start");
        snprintf(mscState, sizeof(mscState), "MSC: Start");
        snprintf(mscProgress, sizeof(mscProgress), "");
        break;
      case ARDUINO_FIRMWARE_MSC_WRITE_EVENT:
        SERIAL.printf("MSC Update Write %u bytes at offset %u\n", data->write.size, data->write.offset);
        SERIAL.print(".");
        snprintf(mscState, sizeof(mscState), "MSC: Writing");
        snprintf(mscProgress, sizeof(mscProgress), "%u bytes", data->write.offset + data->write.size);
        break;
      case ARDUINO_FIRMWARE_MSC_END_EVENT:
        SERIAL.printf("\nMSC Update End: %u bytes\n", data->end.size);
        snprintf(mscState, sizeof(mscState), "MSC: Done");
        snprintf(mscProgress, sizeof(mscProgress), "%u bytes", data->end.size);
        delay(2000);
        break;
      case ARDUINO_FIRMWARE_MSC_ERROR_EVENT:
        SERIAL.printf("MSC Update ERROR! Progress: %u bytes\n", data->error.size);
        snprintf(mscState, sizeof(mscState), "MSC: ERROR");
        snprintf(mscProgress, sizeof(mscProgress), "%u bytes", data->error.size);
        break;
      case ARDUINO_FIRMWARE_MSC_POWER_EVENT:
        SERIAL.printf("MSC Update Power: power: %u, start: %u, eject: %u",
                      data->power.power_condition, data->power.start, data->power.load_eject);
        break;
      default: break;
    }
  }
#endif

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, usbState);
  u8g2.drawStr(0, 22, mscState);
  u8g2.drawStr(0, 32, mscProgress);
  u8g2.sendBuffer();
}


void setupUSB() {
  USB.VID(USBVID);
  USB.PID(USBPID);
  USB.productName(USBPRODUCT);
  USB.manufacturerName(USBMANUFACTURER);
  USB.serialNumber(USBSERIALNO);
#ifdef USBSERIAL
  USB.onEvent(usbEventCallback);
  USBSerial.begin();
  USBSerial.setDebugOutput(true);
  USBSerial.setTimeout(20);
  USBSerial.println("USB CDC Ready!");
#else
  SERIAL.begin(115200);
#endif
#ifdef MSCUPDATE
#ifdef USBSERIAL
  MSC_Update.onEvent(usbEventCallback);
#endif
  MSC_Update.begin();
#endif
  SpaceMouseHID.begin();
  USB.begin();
}


// State machine to track, which report to send next
enum SpaceMouseHIDStates {
  ST_INIT,       // init variables
  ST_START,      // start to check if something is to be sent
  ST_SENDTRANS,  // send translations
  ST_SENDROT,    // send rotations
  ST_SENDKEYS    // send keys
};

SpaceMouseHIDStates nextState;

#define HIDMAXBUTTONS 2        // for Compact
unsigned long lastHIDsentRep;  // time from millis(), when the last HID report was sent


#if (NUMKEYS > 0)
// Array with the bitnumbers, which should assign keys to buttons
uint8_t bitNumber[NUMHIDKEYS] = BUTTONLIST;

// Takes the data in keys and sort them into the bits of keyData
// Which key from keyData should belong to which byte is defined in bitNumber = BUTTONLIST see config.h
void prepareKeyBytes(uint8_t* keys, uint8_t* keyData, int debug) {
  for (int i = 0; i < HIDMAXBUTTONS; i++) {
    keyData[i] = 0;
  }

  // bitNumber[] should be { 0, 1 } for the Compact
  for (int i = 0; i < NUMHIDKEYS; i++) {
    if (keys[i]) {
      int bitNo = bitNumber[i];
      int byteNo = bitNo / 8;
      int bitPos = bitNo % 8;
      // ACCUMULATE instead of overwrite:
      keyData[byteNo] |= (1 << bitPos);

      if (debug == 8) {
        SERIAL.printf("bitNumber: %d -> keyData[%d] = 0x%02X\n", bitNumber[i], (bitNumber[i] / 8), keyData[(bitNumber[i] / 8)]);
      }
    }
  }
}
#endif

// check if a new HID report shall be send
bool IsNewHidReportDue(unsigned long now) {
  return (now - lastHIDsentRep >= HIDUPDATERATE_MS);
}

bool sendUSBData(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z, uint8_t* keys, int debug) {

  static uint8_t countTransZeros = 0;  // count how many times, the zero data has been sent
  static uint8_t countRotZeros = 0;
  unsigned long now = millis();
  bool hasSentNewData = false;  // this value will be returned

#if (NUMKEYS > 0)
  static uint8_t keyData[HIDMAXBUTTONS];      // key data to be sent via HID
  static uint8_t prevKeyData[HIDMAXBUTTONS];  // previous key data
  prepareKeyBytes(keys, keyData, debug);      // sort the bytes from keys into the bits in keyData
#endif

  switch (nextState)  // state machine
  {
    case ST_INIT:
      // init the variables
      lastHIDsentRep = now;
      nextState = ST_START;
      break;
    case ST_START:
      // Evaluate everytime, without waiting for 8ms
      if (countTransZeros < 3 || countRotZeros < 3 || (x != 0 || y != 0 || z != 0 || rx != 0 || ry != 0 || rz != 0)) {
        nextState = ST_SENDTRANS;
      } else {
#if (NUMKEYS > 0)
        // compare key data to previous key data
        if (memcmp(keyData, prevKeyData, HIDMAXBUTTONS) != 0) {
          nextState = ST_SENDKEYS;
        }
#endif
        if (nextState == ST_START && IsNewHidReportDue(now)) {
          lastHIDsentRep = now - HIDUPDATERATE_MS;
        }
      }
      break;
    case ST_SENDTRANS:
      // send translation data, if the 8 ms from the last hid report have past
      if (IsNewHidReportDue(now)) {
        uint8_t payload[6];
        int16_t trans[] = { x, y, z };
        memcpy(payload, trans, 6);
        SpaceMouseHID.send(1, payload, sizeof(payload));

        lastHIDsentRep += HIDUPDATERATE_MS;
        hasSentNewData = true;  // return value

        if (x == 0 && y == 0 && rz == 0) {
          countTransZeros++;
        } else {
          countTransZeros = 0;
        }
        nextState = ST_SENDROT;

        if (debug == 20 || debug == 21) {
          SERIAL.printf(
            "Tran payload: %02X %02X %02X %02X %02X %02X, countRotZeros: %d\n",
            payload[0], payload[1], payload[2],
            payload[3], payload[4], payload[5],
            countRotZeros);
        }
      }
      break;
    case ST_SENDROT:
      // send rotational data, if the 8 ms from the last hid report have past
      if (IsNewHidReportDue(now)) {
        uint8_t payload[6];
        int16_t rot[] = { rx, ry, rz };
        memcpy(payload, rot, 6);
        SpaceMouseHID.send(2, payload, sizeof(payload));

        lastHIDsentRep += HIDUPDATERATE_MS;
        hasSentNewData = true;  // return value

        if (rx == 0 && ry == 0 && rz == 0) {
          countRotZeros++;
        } else {
          countRotZeros = 0;
        }
        if (debug == 20 || debug == 22) {
          SERIAL.printf(
            "Rot payload:  %02X %02X %02X %02X %02X %02X, countRotZeros: %d\n",
            payload[0], payload[1], payload[2],
            payload[3], payload[4], payload[5],
            countRotZeros);
        }

// check if the next state should be keys
#if (NUMKEYS > 0)
        // compare key data to previous key data
        if (memcmp(keyData, prevKeyData, HIDMAXBUTTONS) != 0) {
          nextState = ST_SENDKEYS;
        } else {
          // go back to start
          nextState = ST_START;
        }
#else
        // if no keys are used, go to start state after rotations
        nextState = ST_START;
#endif
      }
      break;
#if (NUMKEYS > 0)
    case ST_SENDKEYS:
      // report the keys, if the 8 ms since the last report have past
      if (IsNewHidReportDue(now)) {
        SpaceMouseHID.send(3, keyData, HIDMAXBUTTONS);

        lastHIDsentRep += HIDUPDATERATE_MS;
        memcpy(prevKeyData, keyData, HIDMAXBUTTONS);  // copy actual keyData to previous keyData
        hasSentNewData = true;                        // return value
        nextState = ST_START;                         // go back to start
      }
      break;
#endif
    default:
      nextState = ST_START;  // send nothing if all data is zero
      break;
  }
  return hasSentNewData;
}
