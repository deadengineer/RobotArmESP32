#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <HardwareSerial.h>

#define RX2 16
#define TX2 17
HardwareSerial SerialSecond(2);

#define TOUCH_CS 21
XPT2046_Touchscreen ts(TOUCH_CS);

#define TS_MINX 200
#define TS_MAXX 3800
#define TS_MINY 200
#define TS_MAXY 3800

#define SD_SCK 14
#define SD_MISO 27
#define SD_MOSI 26
#define SD_CS 13
SPIClass SPI2(HSPI);

TFT_eSPI tft = TFT_eSPI();

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240
#define BUFFER_LINES 16
uint16_t imageBuffer[IMAGE_WIDTH * BUFFER_LINES];

const char *images[] = {
  "/Arm1.rgb565",
  "/Arm2.rgb565",
  "/Arm3.rgb565",
  "/Arm4.rgb565",
  "/Arm5.rgb565",
  "/Arm6.rgb565",
  "/calibrationPose.rgb565"
};

constexpr int IMAGE_COUNT = sizeof(images) / sizeof(images[0]);

int currentImage = -1;
bool touchHandled = false;

bool SHOW_DEBUG_UI = true;
bool SHOW_TOUCH_DOT = true;

bool drawRGB565(const char *filename, int16_t x, int16_t y, uint16_t width, uint16_t height);
void showImage(int imageNumber);
bool insideButton(int touchX, int touchY, int x, int y, int w, int h);

bool calib = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("Starting...");

  SPI.begin(18, 19, 23);

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);

  Serial.print("Width: ");
  Serial.println(tft.width());
  Serial.print("Height: ");
  Serial.println(tft.height());

  ts.begin();
  ts.setRotation(1);

  SerialSecond.begin(9600, SERIAL_8N1, RX2, TX2);
  SerialSecond.setTimeout(50);

  SPI2.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  Serial.println("Mounting SD...");
  if (!SD.begin(SD_CS, SPI2, 10000000)) {
    Serial.println("SD mount failed");
    return;
  }

  Serial.println("SD mounted");

  File root = SD.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      Serial.print(entry.name());
      if (!entry.isDirectory()) {
        Serial.print("  ");
        Serial.print(entry.size());
        Serial.println(" bytes");
      } else {
        Serial.println();
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }

  showImage(0);
  tft.fillRect(10, 200, 80, 30, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Calibration", 20, 210);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Debug", 230, 20);
}

void loop() {
  bool touching = ts.touched();

  if (touching && !touchHandled) {
    touchHandled = true;

    TS_Point p = ts.getPoint();

    if (p.x >= 20 && p.x <= 4200 && p.y >= 20 && p.y <= 4200) {
      int x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width() - 1);
      int y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height() - 1);

      x = constrain(x, 0, tft.width() - 1);
      y = constrain(y, 0, tft.height() - 1);

      Serial.print("Touch: ");
      Serial.print(x);
      Serial.print(", ");
      Serial.println(y);

      if (SHOW_TOUCH_DOT) {
        tft.fillCircle(x, y, 4, TFT_RED);
      }
      if (insideButton(x, y, 220, 10, 40, 30) && SHOW_DEBUG_UI == true) {
        SHOW_DEBUG_UI = false;
        SHOW_TOUCH_DOT = false;
        showImage(currentImage);
        tft.setTextColor(TFT_BLACK);
        tft.drawString("Debug", 230, 20);
      } else if (insideButton(x, y, 220, 10, 40, 30) && SHOW_DEBUG_UI == false) {
        SHOW_DEBUG_UI = true;
        SHOW_TOUCH_DOT = true;
        showImage(currentImage);
        tft.setTextColor(TFT_GREEN);
        tft.drawString("Debug", 230, 20);
      }
      if (!calib) {
        if (insideButton(x, y, 120, 140, 60, 50)) {
          Serial.println("Button 1");
          showImage(0);
        } else if (insideButton(x, y, 90, 80, 60, 50)) {
          Serial.println("Button 2");
          showImage(1);
        } else if (insideButton(x, y, 115, 190, 80, 40)) {
          Serial.println("Button 3");
          showImage(2);
        } else if (insideButton(x, y, 140, 40, 30, 40)) {
          Serial.println("Button 4");
          showImage(3);
        } else if (insideButton(x, y, 170, 30, 30, 60)) {
          Serial.println("Button 5");
          showImage(4);
        }

        else if (insideButton(x, y, 200, 30, 80, 60)) {
          Serial.println("Button 6");
          showImage(5);

        } else if (insideButton(x, y, 10, 200, 80, 30)) {
          Serial.println("Calibration");
          showImage(6);
          calib = true;
          tft.fillRect(220, 200, 80, 30, TFT_WHITE);
          tft.setTextColor(TFT_BLACK);
          tft.drawString("Done", 230, 210);
        }
      } else if (insideButton(x, y, 220, 200, 80, 30)) {
        Serial.println("Return");
        showImage(0);
        calib = false;
        tft.fillRect(10, 200, 80, 30, TFT_WHITE);
        tft.setTextColor(TFT_BLACK);
        tft.drawString("Calibration", 20, 210);
      }
      if (SHOW_DEBUG_UI) { void drawGrid(); }
      if (SHOW_TOUCH_DOT) { void drawDebug(); }
    }
  }

  if (!touching) {
    touchHandled = false;
  }

  if (SerialSecond.available()) {
    int nummer = SerialSecond.parseInt();

    while (SerialSecond.available()) {
      SerialSecond.read();
    }

    if (nummer >= 1 && nummer <= 6) {
      Serial.print("Displaying image ");
      Serial.println(nummer);
      showImage(nummer - 1);
    } else {
      Serial.println("Invalid number. Send 1-6.");
    }
  }
}

bool drawRGB565(const char *filename, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  uint32_t startTime = millis();

  Serial.print("Opening ");
  Serial.println(filename);

  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.print("Could not open ");
    Serial.println(filename);
    return false;
  }

  const uint32_t expectedSize = (uint32_t)width * height * sizeof(uint16_t);
  const uint32_t actualSize = file.size();

  if (actualSize != expectedSize) {
    Serial.print("Wrong file size for ");
    Serial.println(filename);
    Serial.print("Actual: ");
    Serial.println(actualSize);
    Serial.print("Expected: ");
    Serial.println(expectedSize);
    file.close();
    return false;
  }

  uint16_t currentY = 0;
  bool success = true;

  while (currentY < height) {
    uint16_t lines = min((uint16_t)BUFFER_LINES, (uint16_t)(height - currentY));
    size_t pixelCount = (size_t)width * lines;
    size_t bytesNeeded = pixelCount * sizeof(uint16_t);

    size_t bytesRead = file.read((uint8_t *)imageBuffer, bytesNeeded);
    if (bytesRead != bytesNeeded) {
      Serial.println("RGB565 read error");
      success = false;
      break;
    }

    tft.pushImage(x, y + currentY, width, lines, imageBuffer);
    currentY += lines;
  }

  file.close();

  uint32_t elapsed = millis() - startTime;
  Serial.print("Image displayed in ");
  Serial.print(elapsed);
  Serial.println(" ms");

  return success;
}

void showImage(int imageNumber) {
  if (imageNumber < 0 || imageNumber >= IMAGE_COUNT) {
    Serial.println("Invalid image index");
    return;
  }

  if (imageNumber == currentImage) {
    return;
  }

  Serial.print("Changing image to ");
  Serial.println(imageNumber);

  if (drawRGB565(images[imageNumber], 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT)) {
    currentImage = imageNumber;

    if (SHOW_DEBUG_UI) {
      drawDebug();
    }
  }
}

void drawDebug() {
  drawGrid();

  tft.drawRect(120, 140, 60, 50, TFT_RED);
  tft.drawRect(90, 80, 60, 50, TFT_RED);
  tft.drawRect(115, 190, 80, 40, TFT_RED);
  tft.drawRect(140, 40, 30, 40, TFT_RED);
  tft.drawRect(170, 30, 30, 60, TFT_RED);
  tft.drawRect(200, 30, 80, 60, TFT_RED);
  tft.drawRect(10, 200, 80, 30, TFT_RED);
}

void drawGrid() {
  const int spacing = 10;

  for (int x = 0; x < tft.width(); x += spacing) {
    tft.drawFastVLine(x, 0, tft.height(), TFT_DARKGREY);
  }

  for (int y = 0; y < tft.height(); y += spacing) {
    tft.drawFastHLine(0, y, tft.width(), TFT_DARKGREY);
  }

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);

  for (int x = 0; x < tft.width(); x += 20) {
    tft.setCursor(x + 2, 2);
    tft.print(x);
  }

  for (int y = 0; y < tft.height(); y += 20) {
    tft.setCursor(2, y + 2);
    tft.print(y);
  }
}

bool insideButton(int touchX, int touchY, int x, int y, int w, int h) {
  return (
    touchX > x && touchX < (x + w - 1) && touchY > y && touchY < (y + h - 1));
}
