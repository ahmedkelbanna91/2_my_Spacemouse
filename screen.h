#include <Wire.h>
#include <U8g2lib.h>


#define I2C_SCL_PIN 2
#define I2C_SDA_PIN 3

#define SCREEN_REFRESH_DELAY 10

// --- OLED DISPLAY CONFIGURATION ---
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
uint8_t screenWidth = u8g2.getDisplayWidth();
uint8_t screenHeight = u8g2.getDisplayHeight();
#define INFO_SCREEN_DELAY 1

void setupDisplay() {
  // Initialize OLED Display
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  u8g2.begin();

  u8g2.clearBuffer();                  // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
  String bootscreen = "3D Mouse Booting...";
  u8g2.drawStr((screenWidth - u8g2.getStrWidth(bootscreen.c_str())) / 2, (screenHeight + u8g2.getAscent() - u8g2.getDescent()) / 2 - 1, bootscreen.c_str());
  u8g2.sendBuffer();

  delay(1000);
}

void displayScreen(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z, uint8_t* keys) {
  static unsigned long lastDisplayUpdateTime = 0;
  char buffer[32];

  if (millis() - lastDisplayUpdateTime > SCREEN_REFRESH_DELAY) {  // 10Hz update rate
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x8_tr);
    uint8_t line = 0;
    uint8_t TRlineY = 7;

    line += TRlineY;
    u8g2.drawStr(2, line, "T");
    snprintf(buffer, sizeof(buffer), "%5d %5d %5d", x, y, z);
    u8g2.drawStr(13, line, buffer);

    line += TRlineY;
    u8g2.drawStr(2, line, "R");
    snprintf(buffer, sizeof(buffer), "%5d %5d %5d", rx, ry, rz);
    u8g2.drawStr(13, line, buffer);

    u8g2.drawVLine(100, 0, line + 2);
    u8g2.drawVLine(70, 0, line + 2);
    u8g2.drawVLine(40, 0, line + 2);
    u8g2.drawVLine(10, 0, line + 2);
    u8g2.drawHLine(0, line + 3, screenWidth);
    // u8g2.drawStr(110, line - TRlineY, "Btn");
    // u8g2.drawStr(110, line, keys.c_str());

    u8g2.sendBuffer();
    lastDisplayUpdateTime = millis();
  }
}