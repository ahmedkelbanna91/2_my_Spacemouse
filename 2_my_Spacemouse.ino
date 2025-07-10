
#include "Arduino.h"
#include "config.h"

#include "usbSpaceHID.h"
#include "kinematics.h"
#include "calibration.h"
#include "spaceKeys.h"
#include "screen.h"


#ifdef LEDpin
#include "ledring.h"
#endif


// the debug mode can be set during runtime via the serial interface. See config.h for a description of the different debug modes.
int debug = STARTDEBUG;
bool doOnce = true;

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
  setupDisplay();
  setupkalmanFilters();
  setAnalogResolution();

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
  delay(100);

  // Begin Seral for debugging
  Serial.begin(115200);
  delay(100);
  Serial.setTimeout(2);  // the serial interface will look for new debug values and it will only wait 2ms


// setup the keys e.g. to internal pull-ups
#if NUMKEYS > 0
  setupKeys();
#endif
#ifdef LEDpin
  initLEDring();
#endif
}

void loop() {
  if (doOnce) {
    doOnce = false;
    // Read idle/centre positions for joysticks.
    setAnalogResolution();

    // zero the joystick position 500 times (takes approx. 480 ms)
    // during setup() we are not interested in the debug output: debugFlag = false
    busyZeroing(centerPoints, 3000, true);
  }

  // check if the user entered a debug mode via serial interface
  if (Serial.available()) {
    tmpInput = Serial.parseInt();  // Read from serial interface, if a new debug value has been sent. Serial timeout has been set in setup()
    if (tmpInput != 0) {
      debug = tmpInput;
      if (tmpInput == -1) {
        Serial.println(F("Please enter the debug mode now or while the script is reporting."));
      }
      // Debug is updated check if the ADC referencevoltage has to be changed.
      setAnalogResolution();
    }
  }

  // Joystick values are read. 0-1023
  readAllFromSensors(rawReads);

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
    busyZeroing(centerPoints, 3000, true);
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
    debugOutput2(centered, keyVals);
  }

  FilterAnalogReadOuts(centered);

  // Report centered joystick values. Filtered for deadzone. Approx -350 to +350, locked to zero at idle
  if (debug == 3) {
    debugOutput2(centered, keyVals);
  }

  calculateKinematic(centered, velocity);


#if NUMKEYS > 0
  evalKeys(keyVals, keyOut, keyState);
#endif

  // Report translation and rotation values if enabled.
  if (debug == 4) {
    debugOutput4(velocity, keyOut);
  }
  if (debug == 5) {
    debugOutput5(centered, velocity);
  }

  // if the kill-key feature is enabled, rotations or translations are killed=set to zero
#if (NUMKILLKEYS == 2)
  // check for the raw keyVal and not keyOut, because keyOut is only 1 for a single iteration. keyVals has inverse Logic due to pull-ups
  // kill rotation
  if (keyVals[KILLROT] == LOW) {
    velocity[ROTX] = 0;
    velocity[ROTY] = 0;
    velocity[ROTZ] = 0;
  }
  // kill translation
  if (keyVals[KILLTRANS] == LOW) {
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
  send_command(velocity[ROTX], velocity[ROTY], velocity[ROTZ], velocity[TRANSX], velocity[TRANSY], velocity[TRANSZ], keyState, debug);

  // update and report the at what frequency the loop is running
  if (debug == 7) {
    updateFrequencyReport();
  }

#ifdef LEDpin
  processLED(velocity, UsbDevice.getLedState());
#endif

  displayScreen(velocity[ROTX], velocity[ROTY], velocity[ROTZ], velocity[TRANSX], velocity[TRANSY], velocity[TRANSZ], keyState);

}  // end loop()



/**
 * @brief Set the analog reference to 5V for debug 1 and to 2.56V otherwise
 */
void setAnalogResolution() {
  analogReadResolution(analogRead_Resolution);
  Serial.println(F("Setting analogReadResolution to 12"));

  // The first measurements after changing the reference voltage can be wrong. So take 100ms to let the voltage stabilize and
  // take some measurements afterwards just to be sure. Performancewise this shouldn't be a problem due to the debug/setup
  // nature of this function.
  delay(100);
  for (int i = 0; i <= 8; i++) {
    readAllFromSensors(rawReads);
  }
}
