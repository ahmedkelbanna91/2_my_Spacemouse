#include <USB.h>
#include <USBHID.h>

// extern "C" {

//   // --- Device Descriptor Callback ----------------------------------------
//   uint8_t const *tud_descriptor_device_cb(void) {
//     static const tusb_desc_device_t desc = {
//       .bLength = sizeof(tusb_desc_device_t),
//       .bDescriptorType = TUSB_DESC_DEVICE,
//       .bcdUSB = 0x0200,
//       .bDeviceClass = TUSB_CLASS_MISC,
//       .bDeviceSubClass = MISC_SUBCLASS_COMMON,
//       .bDeviceProtocol = MISC_PROTOCOL_IAD,
//       .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
//       .idVendor = 0x256F,   // your VID
//       .idProduct = 0xC635,  // your PID
//       .bcdDevice = 0x0100,
//       .iManufacturer = 0x01,  // string index 1
//       .iProduct = 0x02,       // string index 2
//       .iSerialNumber = 0x03,  // string index 3
//       .bNumConfigurations = 0x01
//     };
//     return (uint8_t const *)&desc;
//   }

//   // --- String Descriptor Callback ----------------------------------------
//   uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
//     static uint16_t _desc_str[32];
//     const char *strings[] = {
//       (char[]){ 0 },         // 0: supported language (0x0409 = English)
//       "BANNA",               // 1: Manufacturer
//       "SpaceMouse Compact",  // 2: Product
//       "SN123456"             // 3: Serial
//     };

//     if (index == 0) {
//       _desc_str[0] = (TUSB_DESC_STRING << 8) | 2;
//       _desc_str[1] = 0x0409;  // English
//       return _desc_str;
//     }
//     if (index >= sizeof(strings) / sizeof(strings[0])) return nullptr;

//     const char *str = strings[index];
//     size_t chr_count = strlen(str);
//     if (chr_count > 31) chr_count = 31;

//     _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
//     for (size_t i = 0; i < chr_count; i++) {
//       _desc_str[1 + i] = str[i];
//     }
//     return _desc_str;
//   }

// }  // extern "C"


USBHID HID;

static const uint8_t hidReportDescriptor[] = {
  // --- Global Items ---
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x08,  // Usage (Multi-axis Controller)
  0xA1, 0x01,  // Collection (Application)

  // --- Translation Report (ID 1) ---
  0xA1, 0x00, 0x85, 0x01, 0x16, 0xA2, 0xFE, 0x26, 0x5E, 0x01, 0x09, 0x30,
  0x09, 0x31, 0x09, 0x32, 0x75, 0x10, 0x95, 0x03, 0x81, 0x02, 0xC0,

  // --- Rotation Report (ID 2) ---
  0xA1, 0x00, 0x85, 0x02, 0x16, 0xA2, 0xFE, 0x26, 0x5E, 0x01, 0x09, 0x33,
  0x09, 0x34, 0x09, 0x35, 0x75, 0x10, 0x95, 0x03, 0x81, 0x02, 0xC0,

  // --- Button Report (ID 3) --- Corrected for 2 buttons
  0xA1, 0x00, 0x85, 0x03, 0x05, 0x09, 0x19, 0x01, 0x29, 0x02, 0x15, 0x00,
  0x25, 0x01, 0x75, 0x01, 0x95, 0x02, 0x81, 0x02, 0x75, 0x01, 0x95, 0x0E,
  0x81, 0x03, 0xC0,

  // --- LED Control Report (ID 4) ---
  0xA1, 0x02, 0x85, 0x04, 0x05, 0x08, 0x09, 0x4B, 0x15, 0x00, 0x25, 0x01,
  0x95, 0x01, 0x75, 0x01, 0x91, 0x02, 0x95, 0x01, 0x75, 0x07, 0x91, 0x03,
  0xC0,

  0xC0  // End Application Collection
};

class CustomHIDDevice : public USBHIDDevice {
public:
  CustomHIDDevice() {
    static bool initialized = false;
    if (!initialized) {
      initialized = true;
      HID.addDevice(this, sizeof(hidReportDescriptor));
    }
  }

  void begin() {
    HID.begin();
  }

  // Descriptor callback
  uint16_t _onGetDescriptor(uint8_t *buffer) override {
    memcpy(buffer, hidReportDescriptor, sizeof(hidReportDescriptor));
    return sizeof(hidReportDescriptor);
  }

  bool ledState = false;

  // Output from host (LEDs): report ID 4
  void _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) override {
    if (report_id == 4 && len >= 1) {
      ledState = (buffer[0] != 0);
      // Serial.printf("LED state set to %s\n", ledState ? "ON" : "OFF");
    }
  }

  bool getLedState() {
    return ledState;
  }
};
CustomHIDDevice UsbDevice;





// State machine to track, which report to send next
enum SpaceMouseHIDStates {
  ST_INIT,       // init variables
  ST_START,      // start to check if something is to be sent
  ST_SENDTRANS,  // send translations
  ST_SENDROT,    // send rotations
  ST_SENDKEYS    // send keys
};

SpaceMouseHIDStates nextState;

#define HIDMAXBUTTONS 2  // for Compact
#if (NUMKEYS > 0)
// Array with the bitnumbers, which should assign keys to buttons
uint8_t bitNumber[NUMHIDKEYS] = BUTTONLIST;
#endif
unsigned long lastHIDsentRep;  // time from millis(), when the last HID report was sent



// check if a new HID report shall be send
bool IsNewHidReportDue(unsigned long now) {
  // calculate the difference between now and the last time it was sent
  // such a difference calculation is safe with regard to integer overflow after 48 days
  return (now - lastHIDsentRep >= HIDUPDATERATE_MS);
}


#if (NUMKEYS > 0)
// Takes the data in keys and sort them into the bits of keyData
// Which key from keyData should belong to which byte is defined in bitNumber = BUTTONLIST see config.h
void prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug) {
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
        Serial.printf("bitNumber: %d -> keyData[%d] = 0x%02X\n", bitNumber[i], (bitNumber[i] / 8), keyData[(bitNumber[i] / 8)]);
      }
    }
  }
}
#endif

bool send_command(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z, uint8_t *keys, int debug) {

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
        HID.SendReport(1, payload, sizeof(payload));

        lastHIDsentRep += HIDUPDATERATE_MS;
        hasSentNewData = true;  // return value

        if (x == 0 && y == 0 && rz == 0) {
          countTransZeros++;
        } else {
          countTransZeros = 0;
        }
        nextState = ST_SENDROT;
        if (debug == 20 || debug == 21) {
          Serial.printf(
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
        HID.SendReport(2, payload, sizeof(payload));

        lastHIDsentRep += HIDUPDATERATE_MS;
        hasSentNewData = true;  // return value

        if (rx == 0 && ry == 0 && rz == 0) {
          countRotZeros++;
        } else {
          countRotZeros = 0;
        }
        if (debug == 20 || debug == 22) {
          Serial.printf(
            "Rot payload: %02X %02X %02X %02X %02X %02X, countRotZeros: %d\n",
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
        HID.SendReport(3, keyData, HIDMAXBUTTONS);

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
