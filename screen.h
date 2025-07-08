// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// #define I2C_SCL_PIN 2
// #define I2C_SDA_PIN 3

// void setupDisplay() {
//   // Initialize OLED Display
//   Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
//   if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
//     Serial.println(F("SSD1306 allocation failed"));
//     for (;;)
//       ;
//   }
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(SSD1306_WHITE);
//   display.setCursor(10, 12);
//   display.print("3D Mouse Booting...");
//   display.display();

//   delay(1000);
// }

// void calibrateSensors() {
//   CALIBRATION = true;
//   showled(CRGB::Yellow, 250);

//   // Perform calibration
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     long sum = 0;
//     for (int j = 0; j < CALIBRATION_SAMPLES; j++) {
//       sum += analogRead(SENSOR_PINS[i]);
//       delay(CALIBRATION_DELAY_MS);
//     }
//     zeroValues[i] = sum / CALIBRATION_SAMPLES;

//     Serial.print("Sensor ");
//     Serial.print(i + 1);  // Print sensor number starting from 1
//     Serial.print(" Zero Value: ");
//     Serial.println(zeroValues[i]);

//     display.clearDisplay();
//     display.setTextSize(1);
//     display.setTextColor(SSD1306_WHITE);
//     display.setCursor(1, 15);
//     display.print("Calibrating");
//     display.setCursor(1, 25);
//     display.print("Sensor ");
//     display.print(i + 1);  // Print sensor number starting from 1
//     display.print(" Zero: ");
//     display.println(zeroValues[i]);
//     display.display();
//   }
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(SSD1306_WHITE);
//   display.setCursor(1, 15);
//   display.print("Calibration DONE!");
//   display.display();

//   showled(CRGB::Green, 250);
//   delay(1000);  // Final delay
//   CALIBRATION = false;
// }

// void updateDisplay() {
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(SSD1306_WHITE);

//   // Line 1: Translation Axes
//   display.setCursor(0, 5);
//   display.print("T:");
//   display.setCursor(12, 5);
//   display.printf("%4d", D_X);
//   display.setCursor(47, 5);
//   display.printf("%4d", D_Y);
//   display.setCursor(82, 5);
//   display.printf("%4d", D_Z);

//   // Line 2: Rotation Axes
//   display.setCursor(0, 15);
//   display.print("R:");
//   display.setCursor(12, 15);
//   display.printf("%4d", R_X);
//   display.setCursor(47, 15);
//   display.printf("%4d", R_Y);
//   display.setCursor(82, 15);
//   display.printf("%4d", R_Z);

//   // Line 3: Button Status with visual delay
//   display.setCursor(0, 25);
//   display.print("B:");
//   display.setCursor(12, 25);
//   display.print((millis() - lastLeftPressTime < BUTTON_DISPLAY_DELAY) ? "L" : "-");
//   display.setCursor(22, 25);
//   display.print((millis() - lastRightPressTime < BUTTON_DISPLAY_DELAY) ? "R" : "-");

//   // display.fillRect(0, 0, display.width(), display.height(), SSD1306_INVERSE);
//   display.display();
// }