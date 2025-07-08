
#include "Arduino.h"
#include "config.h"
#include "calibration.h"
#include "kinematics.h"
#include "spaceKeys.h"

#if ROTARY_AXIS > 0 or ROTARY_KEYS > 0
#include "encoderWheel.h"
#endif

#ifdef LEDpin
void lightSimpleLED(boolean light);
#endif
#ifdef LEDRING
#include "ledring.h"
#endif

#include "USB.h"
#include "USBHID.h"



// State machine to track, which report to send next
enum SpaceMouseHIDStates {
  ST_INIT,       // init variables
  ST_START,      // start to check if something is to be sent
  ST_SENDTRANS,  // send translations
  ST_SENDROT,    // send rotations
  ST_SENDKEYS    // send keys
};
SpaceMouseHIDStates nextState;
#if (NUMKEYS > 0)
// Array with the bitnumbers, which should assign keys to buttons
uint8_t bitNumber[NUMHIDKEYS] = BUTTONLIST;
void prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug);
#endif

uint8_t countTransZeros = 0;  // count how many times, the zero data has been sent
uint8_t countRotZeros = 0;
unsigned long lastHIDsentRep;  // time from millis(), when the last HID report was sent
bool ledState = false;

USBHID HID;

// The USB VID and PID for this emulated space mouse pro must be set in the boards.txt in arduino IDE or in set_hwids.py in platformIO.
static const uint8_t hidReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x08,        // Usage (Multi-Axis)
  0xA1, 0x01,        // Collection (Application)
                     // Report 1: Translation
  0xa1, 0x00,        // Collection (Physical)
  0x85, 0x01,        // Report ID (1)
  0x16, 0xA2, 0xFE,  // Logical Minimum (-350) (0xFEA2 in little-endian)
  0x26, 0x5E, 0x01,  // Logical Maximum (350) (0x015E in little-endian)
  0x36, 0x88, 0xFA,  // Physical Minimum (-1400) (0xFA88 in little-endian)
  0x46, 0x78, 0x05,  // Physical Maximum (1400) (0x0578 in little-endian)
  0x09, 0x30,        // Usage (X)
  0x09, 0x31,        // Usage (Y)
  0x09, 0x32,        // Usage (Z)
  0x75, 0x10,        // Report Size (16)
  0x95, 0x03,        // Report Count (3)
#ifdef ADV_HID_REL   // see Advanced HID settings in config_sample.h
  0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
#else
  0x81, 0x02,  // Input (variable,absolute)
#endif
  0xC0,              // End Collection
                     // Report 2: Rotation
  0xa1, 0x00,        // Collection (Physical)
  0x85, 0x02,        // Report ID (2)
  0x16, 0xA2, 0xFE,  // Logical Minimum (-350)
  0x26, 0x5E, 0x01,  // Logical Maximum (350)
  0x36, 0x88, 0xFA,  // Physical Minimum (-1400)
  0x46, 0x78, 0x05,  // Physical Maximum (1400)
  0x09, 0x33,        // Usage (RX)
  0x09, 0x34,        // Usage (RY)
  0x09, 0x35,        // Usage (RZ)
  0x75, 0x10,        // Report Size (16)
  0x95, 0x03,        // Report Count (3)
#ifdef ADV_HID_REL   // see Advanced HID settings in config_sample.h
  0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
#else
  0x81, 0x02,  // Input (variable,absolute)
#endif
  0xC0,                 // End Collection
                        // Report 3: Keys  // find #define HIDMAXBUTTONS 32 in config_sample.h
  0xa1, 0x00,           // Collection (Physical)
  0x85, 0x03,           //  Report ID (3)
  0x15, 0x00,           //   Logical Minimum (0)
  0x25, 0x01,           //    Logical Maximum (1)
  0x75, 0x01,           //    Report Size (1)
  0x95, HIDMAXBUTTONS,  //    Report Count (32)
  0x05, 0x09,           //    Usage Page (Button)
  0x19, 1,              //    Usage Minimum (Button #1)
  0x29, HIDMAXBUTTONS,  //    Usage Maximum (Button #24)
  0x81, 0x02,           //    Input (variable,absolute)
  0xC0,                 // End Collection
                        // Report 4: LEDs
  0xA1, 0x02,           //   Collection (Logical)
  0x85, 0x04,           //     Report ID (4)
  0x05, 0x08,           //     Usage Page (LEDs)
  0x09, 0x4B,           //     Usage (Generic Indicator)
  0x15, 0x00,           //     Logical Minimum (0)
  0x25, 0x01,           //     Logical Maximum (1)
  0x95, 0x01,           //     Report Count (1)
  0x75, 0x01,           //     Report Size (1)
  0x91, 0x02,           //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x95, 0x01,           //     Report Count (1)
  0x75, 0x07,           //     Report Size (7)
  0x91, 0x03,           //     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0xC0,                 //   End Collection
  0xc0                  // END_COLLECTION
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

  // Output from host (LEDs): report ID 4
  void _onOutput(uint8_t report_id, const uint8_t *buffer, uint16_t len) override {
    if (report_id == 4 && len >= 1) {
      ledState = (buffer[0] != 0);
      Serial.printf("LED state set to %s\n", ledState ? "ON" : "OFF");
    }
  }





  bool send_command(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z, uint8_t *keys, int debug) {
    unsigned long now = millis();

    bool hasSentNewData = false;  // this value will be returned

#if (NUMKEYS > 0)
    static uint8_t keyData[HIDMAXBUTTONS / 8];      // key data to be sent via HID
    static uint8_t prevKeyData[HIDMAXBUTTONS / 8];  // previous key data
    prepareKeyBytes(keys, keyData, debug);          // sort the bytes from keys into the bits in keyData
#endif

#ifdef ADV_HID_JIGGLE
    static bool toggleValue;  // variable to track if values shall be jiggled or not
#endif

    switch (nextState)  // state machine
    {
      case ST_INIT:
        // init the variables
        lastHIDsentRep = now;
        nextState = ST_START;
#ifdef ADV_HID_JIGGLE
        toggleValue = false;
#endif
        break;
      case ST_START:
        // Evaluate everytime, without waiting for 8ms
        if (countTransZeros < 3 || countRotZeros < 3 || (x != 0 || y != 0 || z != 0 || rx != 0 || ry != 0 || rz != 0)) {
          // if one of the values is not zero,
          // or not all zero data packages are sent (sent 3 of them)
          // start sending data
          nextState = ST_SENDTRANS;
        } else {
// if nothing is to be sent, check for keys. If no keys, don't change state
#if (NUMKEYS > 0)
          if (memcmp(keyData, prevKeyData, HIDMAXBUTTONS / 8) != 0)
          // compare key data to previous key data
          {
            nextState = ST_SENDKEYS;
          }
#endif
          if (nextState == ST_START && IsNewHidReportDue(now)) {
            // if we are not leaving the start state and
            // we are waiting here for more than the update rate,
            // keep the timestamp for the last sent package nearby
            lastHIDsentRep = now - HIDUPDATERATE_MS;
          }
        }
        break;
      case ST_SENDTRANS:
        // send translation data, if the 8 ms from the last hid report have past
        if (IsNewHidReportDue(now)) {
          uint8_t trans[6] = { (byte)(x & 0xFF), (byte)(x >> 8), (byte)(y & 0xFF), (byte)(y >> 8), (byte)(z & 0xFF), (byte)(z >> 8) };

#ifdef ADV_HID_JIGGLE
          jiggleValues(trans, toggleValue);  // jiggle the non-zero values, if toggleValue is true
                                             // the toggleValue is toggled after sending the rotations, down below
#endif
          HID.SendReport(1, trans, 6);  // send new translational values
          lastHIDsentRep += HIDUPDATERATE_MS;
          hasSentNewData = true;  // return value

          // if only zeros where send, increment zero counter, otherwise reset it
          if (x == 0 && y == 0 && rz == 0) {
            countTransZeros++;
          } else {
            countTransZeros = 0;
          }
          nextState = ST_SENDROT;
        }
        break;
      case ST_SENDROT:
        // send rotational data, if the 8 ms from the last hid report have past
        if (IsNewHidReportDue(now)) {
          uint8_t rot[6] = { (byte)(rx & 0xFF), (byte)(rx >> 8), (byte)(ry & 0xFF), (byte)(ry >> 8), (byte)(rz & 0xFF), (byte)(rz >> 8) };

#ifdef ADV_HID_JIGGLE
          jiggleValues(rot, toggleValue);  // jiggle the non-zero values, if toggleValue is true
          toggleValue ^= true;             // toggle the indicator to jiggle only every second report send
#endif

          HID.SendReport(2, rot, 6);
          lastHIDsentRep += HIDUPDATERATE_MS;
          hasSentNewData = true;  // return value
          // if only zeros where send, increment zero counter, otherwise reset it
          if (rx == 0 && ry == 0 && rz == 0) {
            countRotZeros++;
          } else {
            countRotZeros = 0;
          }
// check if the next state should be keys
#if (NUMKEYS > 0)
          if (memcmp(keyData, prevKeyData, HIDMAXBUTTONS / 8) != 0)
          // compare key data to previous key data
          {
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
          HID.SendReport(3, keyData, HIDMAXBUTTONS / 8);
          lastHIDsentRep += HIDUPDATERATE_MS;
          memcpy(prevKeyData, keyData, HIDMAXBUTTONS / 8);  // copy actual keyData to previous keyData
          hasSentNewData = true;                            // return value
          nextState = ST_START;                             // go back to start
        }
        break;
#endif
      default:
        nextState = ST_START;  // go back to start in error?!
        // send nothing if all data is zero
        break;
    }
    return hasSentNewData;
  }

  // check if a new HID report shall be send
  bool IsNewHidReportDue(unsigned long now) {
    // calculate the difference between now and the last time it was sent
    // such a difference calculation is safe with regard to integer overflow after 48 days
    return (now - lastHIDsentRep >= HIDUPDATERATE_MS);
  }

#ifdef ADV_HID_JIGGLE
  // function to add jiggle to the values, if they are not zero.
  // jiggle means to set the last bit to zero or one, depending on the parameter lastBit
  // lastBit shall be toggled between true and false between repeating calls
  bool jiggleValues(uint8_t val[6], bool lastBit) {
    for (uint8_t i = 0; i < 6; i = i + 2) {
      if ((val[i] != 0 || val[i + 1] != 0) && lastBit) {
        // value is not zero, set last bit to one
        val[i] = val[i] | 1;
      } else {
        // value is already zero and needs not jiggling, or the last bit shall be forced to zero
        val[i] = val[i] & (0xFE);
      }
    }
    return true;
  }

#endif

#if (NUMKEYS > 0)
  // Takes the data in keys and sort them into the bits of keyData
  // Which key from keyData should belong to which byte is defined in bitNumber = BUTTONLIST see config.h
  void prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug) {
    for (int i = 0; i < HIDMAXBUTTONS / 8; i++)  // init or empty this array
    {
      keyData[i] = 0;
    }

    for (int i = 0; i < NUMHIDKEYS; i++) {
      // check for every key if it is pressed
      if (keys[i]) {
        // set the according bit in the data bytes
        // byte no.: bitNumber[i] / 8
        // bit no.:  bitNumber[i] modulo 8
        keyData[(bitNumber[i] / 8)] = (1 << (bitNumber[i] % 8));
        if (debug == 8) {
          // debug the key board outputs
          Serial.print("bitnumber: ");
          Serial.print(bitNumber[i]);
          Serial.print(" -> keyData[");
          Serial.print((bitNumber[i] / 8));
          Serial.print("] = ");
          Serial.print("0x");
          Serial.println(keyData[(bitNumber[i] / 8)], HEX);
        }
      }
    }
  }
#endif
};
CustomHIDDevice UsbDevice;

void usbSetup() {
  // Check if the HID device is ready to send reports
  if (!HID.ready()) {
    delay(1);
    return;
  }

  USB.VID(0x256f);
  USB.PID(0xc635);
  USB.productName("SpaceMouse Compact");
  USB.manufacturerName("BANNA");

  UsbDevice.begin();
  USB.begin();
}

// the debug mode can be set during runtime via the serial interface. See config.h for a description of the different debug modes.
int debug = STARTDEBUG;

// stores the raw analog values from the joysticks
int rawReads[8];

// Centerpoints store the zero position of the joysticks
int centerPoints[8];

// stores the values from the joysticks after zeroing and mapping
int centered[8];

// store raw value of the keys, without debouncing
int keyVals[NUMKEYS];

// key event, after debouncing. It is 1 only for a single sample
uint8_t keyOut[NUMKEYS];
// state of the key, which stays 1 as long as the key is pressed
uint8_t keyState[NUMKEYS];

// Resulting calculated velocities / movements
// int16_t to match what the HID protocol expects.
int16_t velocity[6];

int tmpInput;  // store the value, the user might input over the serial


void setup() {
  usbSetup();

// setup the keys e.g. to internal pull-ups
#if NUMKEYS > 0
  setupKeys();
#endif

  // Begin Seral for debugging
  Serial.begin(250000);
  delay(100);
  Serial.setTimeout(2);  // the serial interface will look for new debug values and it will only wait 2ms
  // Read idle/centre positions for joysticks.

#ifdef HALLEFFECT
    // Set the ADC referencevoltage to 5V if debug=1, 2.56V otherwise.
  // It is important the reference Voltage is set before the Zeroing of the
  // sensors is executed.
  setAnalogReferenceVoltage();
#endif

  // zero the joystick position 500 times (takes approx. 480 ms)
  // during setup() we are not interested in the debug output: debugFlag = false
  busyZeroing(centerPoints, 500, false);

#if ROTARY_AXIS > 0 or ROTARY_KEYS > 0
  initEncoderWheel();
#endif
#ifdef LEDpin
  initLEDring();
#endif
}

void loop() {
  // check if the user entered a debug mode via serial interface
  if (Serial.available()) {
    tmpInput = Serial.parseInt();  // Read from serial interface, if a new debug value has been sent. Serial timeout has been set in setup()
    if (tmpInput != 0) {
      debug = tmpInput;
      if (tmpInput == -1) {
        Serial.println(F("Please enter the debug mode now or while the script is reporting."));
      }
#ifdef HALLEFFECT
      // Debug is updated check if the ADC referencevoltage has to be changed.
      setAnalogReferenceVoltage();
#endif
    }
  }

  // Joystick values are read. 0-1023
  readAllFromJoystick(rawReads);

#if NUMKEYS > 0
  // LivingTheDream added reading of key presses
  readAllFromKeys(keyVals);
#endif
  // Report back 0-1023 raw ADC 10-bit values if enabled
  if (debug == 1) {
    debugOutput1(rawReads, keyVals);
  }

  if (debug == 11) {
    // calibrate the joystick
    // As this is called in the debug=11, we do more iterations.
    busyZeroing(centerPoints, 2000, true);
    debug = -1;  // this only done once
  }

  // Subtract centre position from measured position to determine movement.
  for (int i = 0; i < 8; i++) {
    centered[i] = rawReads[i] - centerPoints[i];
  }

  if (debug == 20) {
    calcMinMax(centered);  // debug=20 to calibrate MinMax values
  }

  // Report centered joystick values if enabled. Values should be approx -500 to +500, jitter around 0 at idle
  if (debug == 2) {
    debugOutput2(centered);
  }

  FilterAnalogReadOuts(centered);

  // Report centered joystick values. Filtered for deadzone. Approx -350 to +350, locked to zero at idle
  if (debug == 3) {
    debugOutput2(centered);
  }

  calculateKinematic(centered, velocity);

#if (ROTARY_AXIS > 0) && ROTARY_AXIS < 7
  // If an encoder wheel is used, calculate the velocity of the wheel and replace one of the former calculated velocities
  calcEncoderWheel(velocity, debug);
#endif

#if NUMKEYS > 0
  evalKeys(keyVals, keyOut, keyState);
#endif

#if ROTARY_KEYS > 0
  // The encoder wheel shall be treated as a key
  calcEncoderAsKey(keyState, debug);
#endif

  if (debug == 4) {
    debugOutput4(velocity, keyOut);
    // Report translation and rotation values if enabled.
  }
  if (debug == 5) {
    debugOutput5(centered, velocity);
  }

  // if the kill-key feature is enabled, rotations or translations are killed=set to zero
#if (NUMKILLKEYS == 2)
  if (keyVals[KILLROT] == LOW) {
    // check for the raw keyVal and not keyOut, because keyOut is only 1 for a single iteration. keyVals has inverse Logic due to pull-ups
    // kill rotation
    velocity[ROTX] = 0;
    velocity[ROTY] = 0;
    velocity[ROTZ] = 0;
  }
  if (keyVals[KILLTRANS] == LOW) {
    // kill translation
    velocity[TRANSX] = 0;
    velocity[TRANSY] = 0;
    velocity[TRANSZ] = 0;
  }
#endif

  // report velocity and keys after possible kill-key feature
  if (debug == 6) {
    debugOutput4(velocity, keyOut);
  }

#if SWITCHYZ > 0
  switchYZ(velocity);
#endif

#ifdef EXCLUSIVEMODE
  // exclusive mode
  // rotation OR translation, but never both at the same time
  // to avoid issues with classics joysticks
  exclusiveMode(velocity);
#endif

  // report velocity and keys after Switch or ExclusiveMode
  if (debug == 61) {
    debugOutput4(velocity, keyOut);
  }

  // get the values to the USB HID driver to send if necessary
  UsbDevice.send_command(velocity[ROTX], velocity[ROTY], velocity[ROTZ], velocity[TRANSX], velocity[TRANSY], velocity[TRANSZ], keyState, debug);

  if (debug == 7) {
    // update and report the at what frequency the loop is running
    updateFrequencyReport();
  }

#ifdef LEDpin
  processLED(velocity, bool led_on);
#endif

}  // end loop()



#ifdef HALLEFFECT
/**
 * @brief Set the analog reference to 5V for debug 1 and to 2.56V otherwise
 */
void setAnalogReferenceVoltage() {
  if (debug == 1) {
    // Set the reference voltage for the AD Convertor to 5V only for the first calibration step (pinout/inversion calibration).
    analogReadResolution(analogReadResolution);
    Serial.println(F("Setting analogReadResolution to 12"));
  } else {
    // Set the reference voltage for the AD Convertor to 2.56V in order to get larger sensitivity.
    analogReadResolution(analogReadResolution - 8);
    Serial.println(F("Setting analogReadResolution to 8"));
  }

  // The first measurements after changing the reference voltage can be wrong. So take 100ms to let the voltage stabilize and
  // take some measurements afterwards just to be sure. Performancewise this shouldn't be a problem due to the debug/setup
  // nature of this function.
  delay(100);
  int tempReads[8];
  for (int i = 0; i <= 8; i++) {
    readAllFromJoystick(tempReads);
  }
}
#endif