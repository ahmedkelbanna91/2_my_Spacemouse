/* Calibration instructions
============================
Follow this file from top to bottom to calibrate your space mouse.
You can find some pictures for the calibration process here:
https://github.com/AndunHH/spacemouse/wiki/Ergonomouse-Build#calibration

Debugging Instructions
=========================
To activate one of the following debugging modes, you can either:
- Change STARTDEBUG here in the code and compile again or
- Compile and upload your program. Change to the serial monitor and type the number and hit enter to select the debug mode.

Debug Modes:
------------
-1: Debugging off. Set to this once everything is working.
0:  Nothing...

1:  Output raw joystick values. 0-analogMax_Resolution raw ADC 10-bit values
11: Calibrate / Zero the Spacemouse and get a dead-zone suggestion (This is also done on every startup in the setup())

2:  Output centered joystick values. Values should be approximately -500 to +500, jitter around 0 at idle.
20: semi-automatic min-max calibration. (Replug/reset the mouse, to enable the semi-automatic calibration for a second time.)

3:  Output centered joystick values. Filtered for deadzone. Approximately -350 to +350, locked to zero at idle, modified with a function.

4:  Output translation and rotation values. Approximately -350 to +350 depending on the parameter.
5:  Output debug 4 and 5 side by side for direct cause and effect reference.
6:  Report velocity and keys after possible kill-key feature
61: Report velocity and keys after kill-switch or ExclusiveMode
7:  Report the frequency of the loop() -> how often is the loop() called in one second?
8:  Report the bits and bytes send as button codes
9:  Report details about the encoder wheel, if ROTARY_AXIS > 0 or ROTARY_KEYS>0
*/
#define STARTDEBUG 0  // Can also be set over the serial interface, while the program is running!

// Send a HID report every 8 ms
#define HIDUPDATERATE_MS 8


/* First Calibration: Hall effect sensors pin assignment
==============================================
Default assembly when looking from above on top of the space mouse
 *
 *    back(USB)     resulting axis (not from the single sensors)
 *
 *      7   6              Y+
 *        |                .
 *   8    |    3           .
 *     ---+---        X-...Z+...X+
 *   9    |    2           .
 *        |                .
 *      0   1              Y-
 *

Each sensor is affected by the position of the magnet. If the magnet moves closer to the sensor the output values should decrease.
If you have mounted the magnets upside-down, the values will be inverted.

1. Try to write down the each HES sensor with the corresponding pin number you chose. If you use the PCB version, the pins are shown
   on the silk screen.
2. Compile the script, type 1 into the serial interface and hit enter to enable debug output 1.
3. With the spacemouse in front of you (ie. USB connection at the backside ie. @ north), move the knob from south to north and observe the debug output:
  3.a) HES0 & HES1 both increase when moving the knob north (magnets further away) and decrease when moving the knob south -> Everything is correct.
  3.b) HES0 & HES1 both decrease when moving the knob south (magnets closer by) -> Everything is correct.
  3.c) The sensorpair acts in the opposite direction (ie. increasing when the magnets get closer by ). You probably have mixed up the poles of the magnets.
       Invert the sensorpair in the INVERTLIST.
  3.c) Another output is showing movement: Swap the pins in the PINLIST. This shouldn't happen when the PCB is used. Debugging is rather difficult when
       the spacemouse is assembled, due to the fact that all sensors act on the same movement.

4. Continue with the other sensor pairs. Moving the magnet closer to the sensor should decrease the value.

5. Optimally when not moving the knob, all sensors should output approximately the same value. You can adjuist the values a little bit
   by adjusting the height of the sensor plate using the spacernuts.

5. Repeat this with every axis and every sensor pair until you have a valid PINLIST and maybe an INVERTLIST
*/

// HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9

// --- Sensor Pin Definitions ---
// ---------M---------
// -------7---6-------
// --7-------------5--
// M-----------------M
// --8-------------4--
// -------1---2-------
// ---------M---------

#define HES1_PIN 6   // Bottom Pair, Sensor 1, left
#define HES2_PIN 7   // Bottom Pair, Sensor 2, right
#define HES3_PIN 8   // Right Pair, Sensor 3, bottom
#define HES4_PIN 9   // Right Pair, Sensor 4, top
#define HES5_PIN 10  // Top Pair, Sensor 5, right
#define HES6_PIN 11  // Top Pair, Sensor 6, left
#define HES7_PIN 12  // Left Pair, Sensor 7, top
#define HES8_PIN 13  // Left Pair, Sensor 8, bottom

#define analogRead_Resolution 12
#define analogMax_Resolution 4096

// AX, AY, BX, BY, CX, CY, DX, DY
#define PINLIST \
  { HES1_PIN, HES2_PIN, HES3_PIN, HES4_PIN, HES5_PIN, HES6_PIN, HES7_PIN, HES8_PIN }
//  {A0, A1, A2, A3, A6, A7, A8, A9}
// Check the correct wiring with the debug output=1

// Set to 1 to invert one hall sensor.
// Values should decrease when the magnet is nearing the sensor, but if the magnet is placed with the poles reversed, you can
// invert the value. Usually the inversion should be configured by HES-pair (ie. 0 & 1, 2 & 3, 6 & 7, 8 & 9)
#define INVERTLIST \
  { 0, 0, 0, 0, 0, 0, 0, 0 }
// HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9

/* Second calibration: Tune Deadzone
====================================
Deadzone to filter out unintended movements. Increase if the mouse has small movements when it should be idle or the mouse is too sensitive to subtle movements.

Semi-automatic: Set debug = 11. Don't touch the mouse and observe the automatic output.
Manual:         Set debug = 2.  Don't touch the mouse but observe the values. They should be nearly to zero.
                                Every value around zero which is noise or should be neglected afterwards is in the following deadzone.
*/
#define ANALOG_DEADZONE 10
#define DEADZONE 10  //15  // Recommended to have this as small as possible to allow full range of motion.

// a dead zone above the following value will be warned
#define DEADZONEWARNING 50
// The centerpoint of the Hall effect mouse is not in the center of the ADC range, due to the hardware nature.
// According to the height of the base plate, the centerpoint is shifted up or downwards.
// a centerpoint below or above those values will be warned (512 +/- 128)
#define CENTERPOINTWARNINGMIN (1400 - 128)
#define CENTERPOINTWARNINGMAX (1400 + 128)

// The Hall effect sensors aren't centered around zero, due to the nature of the hardware.
// In my version of the Spacemouse, the values vary between -425 and 285, the centerpoint is thus around -70
// The MIN and MAX warning levels have to be shifted accordingly.
#define MINMAX_MINWARNING (100 - centerPoint)
#define MINMAX_MAXWARNING (100 + centerPoint)

/* Third calibration: Getting MIN and MAX values
================================================
Can be done manual (debug = 2) or semi-automatic (debug = 20)

Semi-automatic (debug=20)
------------------------
1. Compile the sketch and upload it.
2. Go to the serial monitor type 20 and hit enter. -> debug is set to 20.
3. Move the Spacemouse around for 15s to get a min and max value.
// TODO - minMax are different for the HES sensors
4. Verify, that the minimums are around -400 to -520 and the maxVals around +400 to +520.
   (repeat or check again, if you have too small values!)
5. Copy the output from the console into your config.h below.

Manual min/max calibration (debug = 2)
--------------------------------------
Recommended calibration procedure for min/max ADC levels
1. Compile the sketch and upload it. Go to the serial monitor type 2 and hit enter. -> debug is set to 2.
2. Get a piece of paper and write the following chart:
 Chart:
 maxVals      | minVals
-------------------------------
 HES0+:         | HES0-:
 HES1+:         | HES1-:
 HES2+:         | HES2-:
 HES3+:         | HES3-:
 HES6+:         | HES6-:
 HES7+:         | HES7-:
 HES8+:         | HES8-:
 HES9+:         | HES9-:

3. (a) Start out with HES0 (positive Values)
   (b) Start moving the your Spacemouse and try increasing the Value of HES0 till you can't get a higher value out of it.
   (c) this is your positive maximum value for HES0 so write it down for HES0
4. Do the same for HES1,HES2,HES3,....HES9
5. Do the same for your negative Values to populate the minVals
6. Write all the positive Values starting from the top into the Array maxValues
7. Write all the negative Values starting from the top into the Array minValues
8. You finished calibrating.

Insert measured Values like this: {HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9}
*/


// #define MINVALS  { -500, -527, -447, -462, -583, -436, -490, -531 }
// #define MAXVALS  { 546, 485, 528, 565, 481, 549, 488, 582 }

#define MINVALS  { -500, -500, -500, -500, -500, -500, -500, -500 }
#define MAXVALS  { 500, 500, 500, 500, 500, 500, 500, 500 }

// Ranges are: {1046, 1012, 975, 1027, 1064, 985, 978, 1113}

/* Fourth calibration: Sensitivity
==================================
Use debug mode 4 or use for example your CAD program to verify changes.

The sensitivity values are used in a division, thus
  - Use a fraction to make the axis MORE sensitive. F.e. 0.5 makes the axis twice as sensitive.
  - Use a value larger than 1 to make it LESS sensitive. F.e. 5 makes the axis five times less sensitive.

Recommended calibration procedure for sensitivity
-------------------------------------------------
1. Make sure modFunc is on level 0, see below. Upload the sketch. Then open serial monitor, type 4 for and hit enter.
   You will see Values TX, TY, TZ, RX, RY, RZ, the configured keys and the current status of the sensitivity parameters.
2. Start moving your Spacemouse. You will notice values changing.
3. Starting with TX try increasing this value as much as possible by moving your Spacemouse around. If you get around 350 thats great.
   If not change TRANSX_SENSITIVITY. Repeat until it is around 350 for maximum motion.
4. Repeat steps 3 for TY, TZ, RX, RY, RZ
5. Verification: Move the Joystick in funny ways. All you should get for either TX,TX,TZ,RX,RY,RZ should be approximately between -350 to 350.
6. You have finished sensitivity calibration. You can now test your Spacemouse with your favorite program (e.g. Cad software, Slicer)
7. Aftermath: You notice the movements are hard to control. Try using Modification Functions [Suggestion: ModFunc level 3]
*/

#define TRANSX_SENSITIVITY 1.0
#define TRANSY_SENSITIVITY 1.0
#define POS_TRANSZ_SENSITIVITY 1.0
#define NEG_TRANSZ_SENSITIVITY 1.0
#define GATE_NEG_TRANSZ 20  // gate value, which negative z movements will be ignored (like an additional deadzone for -z).
#define GATE_ROTX 20        // Value under which rotX values will be forced to zero
#define GATE_ROTY 20        // Value under which roty values will be forced to zero
#define GATE_ROTZ 20        // Value under which rotz values will be forced to zero

#define ROTX_SENSITIVITY 1.0
#define ROTY_SENSITIVITY 1.0
#define ROTZ_SENSITIVITY 1.0

/* Fifth calibration: Modifier Function
=======================================
Modify resulting behaviour of Spacemouse outputs to suppress small movements around zero and enforce big movements even more.

Check the README.md for more details and a plot of the different functions.
(This function is applied on the resulting velocities and not on the direct input from the joysticks)

This should be at level 0 when starting the calibration!
0: linear y = x [Standard behaviour: No modification]
1: squared function y = x^2*sign(x) [altered squared function working in positive and negative direction]
2: tangent function: y = tan(x) [Results in a linear curve near zero but increases the more you are away from zero]
3: squared tangent function: y = tan(x^2*sign(X)) [Results in a flatter curve near zero but increases a lot the more you are away from zero]
4: cubed tangent function: y = tan(x^3) [Results in a very flat curve near zero but increases drastically the more you are away from zero]

Recommendation after tuning: MODFUNC 3
*/
#define MODFUNC 3  // Used as default value as long as the data hasn't been saved in the EEPROM

/* Sixth Calibration: Direction
===============================
Modify the direction of translation/rotation depending on the CAD program you are using on your PC.
This should be done, when you are done with the pin assignment!

If all defines are set to 0 the resulting X, Y and Z axis correspond to the pictures shown in the README.md.
The suggestion in the comments for "3Dc" are often needed on windows PCs with 3dconnexion driver to get expected behavior.
*/
#define INVX 1   // pan left/right  // 3Dc: 0
#define INVY 1   // pan up/down     // 3Dc: 1
#define INVZ 1   // zoom in/out     // 3Dc: 1
#define INVRX 1  // Rotate around X axis (tilt front/back)  // 3Dc: 0
#define INVRY 1  // Rotate around Y axis (tilt left/right)  // 3Dc: 1
#define INVRZ 1  // Rotate around Z axis (twist left/right) // 3Dc: 1

// Switch Zoom direction with Up/Down Movement
#define SWITCHYZ 0  // change to 1 to switch Y and Z axis






/* Key Support
===============
If you attached keys to your Spacemouse, configure them here.
You can use the keys to report them via USB HID to the PC (either classically pressed or emulated with an encoder) or as kill-keys (described below).

How many classic keys are there in total? (0=no keys, feature disabled)
*/

#define RIGHT_BTN_PIN 1
#define LEFT_BTN_PIN 4
#define CAL_BUTTON_PIN 0

#define NUMKEYS 3  // 0

// Define the PINS for the classic keys on the Arduino
// The first pins from KEYLIST may be reported via HID
#define KEYLIST \
  { LEFT_BTN_PIN, RIGHT_BTN_PIN, CAL_BUTTON_PIN }

/* Report KEYS over USB HID to the PC
 ----------------------------------
How many keys reported? Classical + ROTARY_KEYS in total.
*/
#define NUMHIDKEYS 2  // 0

// In order to define which key is assigned to which button, the following list must be entered in the BUTTONLIST below

// compact
#define SM_LEFT 0
#define SM_RIGHT 1

// pro
#define SM_MENU 0   // Key "Menu"
#define SM_FIT 1    // Key "Fit"
#define SM_T 2      // Key "Top"
#define SM_R 4      // Key "Right"
#define SM_F 5      // Key "Front"
#define SM_RCW 8    // Key "Roll 90°CW"
#define SM_1 12     // Key "1"
#define SM_2 13     // Key "2"
#define SM_3 14     // Key "3"
#define SM_4 15     // Key "4"
#define SM_ESC 22   // Key "ESC"
#define SM_ALT 23   // Key "ALT"
#define SM_SHFT 24  // Key "SHIFT"
#define SM_CTRL 25  // Key "CTRL"
#define SM_ROT 26   // Key "Rotate"

// uint8_t button_bits[] = { 12, 13, 14, 15, 22, 25, 23, 24, 0, 1, 2, 4, 5, 8, 26 };




// BUTTONLIST must have at least as many elements as NUMHIDKEYS
// The keys from KEYLIST or ROTARY_KEYS are assigned to buttons here:
#define BUTTONLIST \
  { SM_LEFT, SM_RIGHT }

/* Exclusive mode
=================
Exclusive mode only permit to send translation OR rotation, but never both at the same time.
This can solve issues with classic joysticks where you get unwanted translation or rotation at the same time.

it choose to send the one with the biggest absolute value.
*/
// #define EXCLUSIVEMODE

/* Kill-Key Feature
--------------------
Are there buttons to set the translation or rotation to zero?
How many kill keys are there? (disabled: 0; enabled: 2)
*/
#define NUMKILLKEYS 0
// usually you take the last two buttons from KEYLIST as kill-keys
// Index of the kill key for rotation
#define KILLROT 3
// Index of the kill key for translation
#define KILLTRANS 3
// Note: Technically you can report the kill-keys via HID as "usual" buttons, but that doesn't make much sense...

/*  Example for NO KEYS
 *  There are zero keys in total:  NUMKEYS 0
 *  KEYLIST { }
 *  NUMHIDKEYS 0
 *  BUTTONLIST { }
 *  NUMKILLKEYS 0
 *  KILLROT and KILLTRANS don't matter... KILLROT 0 and KILLTRANS 0
 */

/*  Example for three usual buttons and no kill-keys
 *  There are three keys in total:  NUMKEYS 3
 *  The keys which shall be reported to the pc are connected to pin 15, 14 and 16
 *  KEYLIST {15, 14, 16}
 *  Therefore, the first three pins from the KEYLIST apply for the HID: NUMHIDKEYS 3
 *  Those three Buttons shall be "FIT", "T" and "R": BUTTONLIST {SM_FIT, SM_T, SM_R}
 *  No keys for kill-keys NUMKILLKEYS 0
 *  KILLROT and KILLTRANS don't matter... KILLROT 0 and KILLTRANS 0
 */

/*
 *  Example for two usual buttons and two kill-keys:
 *  There are four keys in total:  NUMKEYS 4
 *  The keys which shall be reported to the pc are connected to pin 15 and 14
 *  The keys which shall be used to kill translation or rotation are connected to pin 16 and 10
 *  KEYLIST {15, 14, 16, 10}
 *  Therefore, the first two pins from the KEYLIST apply for the HID: NUMHIDKEYS 2
 *  Those two Buttons shall be "3", "4": BUTTONLIST {SM_3, SM_4}
 *  Two keys are used as kill-keys: NUMKILLKEYS 2
 *  The first kill key has the third position in the KEYLIST and due to zero-based counting third-1 => KILLROT 2
 *  The second kill key has the last position in the KEYLIST with index 3 -> KILLTRANS 3
 */

// Some simple tests for the definition of the keys
#if (NUMKILLKEYS > NUMKEYS)
#error "Number of Kill Keys can not be larger than total number of keys"
#endif
#if (NUMKILLKEYS > 0 && ((KILLROT > NUMKEYS) || (KILLTRANS > NUMKEYS)))
#error "Index of killkeys must be smaller than the total number of keys"
#endif

// time in ms which is needed to allow a new button press
#define DEBOUNCE_KEYS_MS 200

/* LED support
===============
*/
// #define LEDpin 5
// #define LEDRING 4

// The LEDs light up, if a certain movement is reached:
#define VelocityDeadzoneForLED 15

// About how many LEDs must the ring by turned to align?
#define LEDclockOffset 0

// how often shall the LEDs be updated
#define LEDUPDATERATE_MS 150
/* Advanced debug output settings
=================================
The following settings allow customization of debug output behavior 
*/

// Generate a debug line only every DEBUGDELAY ms
#define DEBUGDELAY 100

// The standard behavior "\r" for the debug output is, that the values are always written into the same line to get a clean output. Easy readable for the human.
// #define DEBUG_LINE_END "\r"
// If you need to report some debug outputs to trace errors, you can change the debug output to "\r\n" to get a newline with each debug output. (old behavior)
#define DEBUG_LINE_END "\r\n"
