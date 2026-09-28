#include <SPI.h>
#include <XPT2046_Touchscreen.h>
// Animates white pixels to simulate flying through a star field
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <HardwareSerial.h>
#define RX2 16
#define TX2 17
#define CS_PIN 21
XPT2046_Touchscreen ts(CS_PIN);
#define SDCS_PIN 13
// Temporary calibration values.
// Fine-tune these after measuring all four corners.
#define TS_MINX 200
#define TS_MAXX 3800
#define TS_MINY 200
#define TS_MAXY 3800
struct ImageSpan {
  uint16_t x;
  uint16_t y;
  uint16_t length;
  uint32_t pixelOffset;
};

struct CompressedBMP {
  uint16_t width = 0;
  uint16_t height = 0;

  ImageSpan *spans = nullptr;
  uint16_t *pixels = nullptr;

  uint32_t spanCount = 0;
  uint32_t pixelCount = 0;
};
CompressedBMP arm1 = {};
//Second SPI
int sck = 14;
int miso = 27;
int mosi = 26;
int cs = 13;
SPIClass SPI2(HSPI);
TFT_eSPI tft = TFT_eSPI();
HardwareSerial SerialSecond(2);
uint8_t bmpBuffer[320 * 3 + 4];
uint16_t lineBuffer[320];
const char *images[] = {
  "/Arm1.bmp",
  "/Arm2.bmp",
  "/Arm3.bmp",
  "/Arm4.bmp",
  "/Arm5.bmp",
  "/Arm6.bmp",
  "/calibrationPose.bmp"
};
uint8_t backgroundR = 0;
uint8_t backgroundG = 0;
uint8_t backgroundB = 0;
void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23);
  SPI2.begin(sck, miso, mosi, cs);
  // Landscape
  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(false);
  Serial.print("Breite: ");
  Serial.println(tft.width());

  Serial.print("Hoehe: ");
  Serial.println(tft.height());

  tft.fillScreen(TFT_BLACK);
  ts.begin();
  ts.setRotation(1);

  SerialSecond.begin(
    9600,  // baud rate
    SERIAL_8N1,
    RX2,
    TX2);
  if (!SD.begin(SDCS_PIN, SPI2, 1000000)) {
    Serial.println("SD mount failed");
    return;
  }

  File root = SD.open("/");
  File entry = root.openNextFile();
  while (entry) {
    Serial.println(entry.name());
    entry = root.openNextFile();
  }
  Serial.println(
  "Loading Arm1 into RAM..."
);

if (
  loadBMPCompressed(
    "/Arm1.bmp",
    arm1
  )
) {

  Serial.println(
    "Arm1 loaded"
  );
}
else {

  Serial.println(
    "Arm1 load failed"
  );
}
  drawBMP("/Arm1.bmp", 0, 0);
}

void loop() {
  if (!ts.touched())
    return;

  TS_Point p = ts.getPoint();

  Serial.print("RAW X=");
  Serial.print(p.x);
  Serial.print(" Y=");
  Serial.print(p.y);
  Serial.print(" Z=");
  Serial.println(p.z);

  // Reject only clearly invalid data
  if (p.x < 20 || p.x > 4200 || p.y < 20 || p.y > 4200) {
    return;
  }

  int x = map(
    p.x,
    TS_MINX,
    TS_MAXX,
    0,
    tft.width() - 1);

  int y = map(
    p.y,
    TS_MINY,
    TS_MAXY,
    0,
    tft.height() - 1);

  x = constrain(x, 0, tft.width() - 1);
  y = constrain(y, 0, tft.height() - 1);

  Serial.print("Touch at: ");
  Serial.print(x);
  Serial.print(", ");
  Serial.println(y);

  tft.fillCircle(x, y, 4, TFT_RED);

  if (SerialSecond.available()) {

    int nummer = SerialSecond.parseInt();

    // Restliche Zeichen wie \n / \r entfernen
    while (SerialSecond.available()) {
      SerialSecond.read();
    }

    switch (nummer) {

      case 1:
        drawBMP("/Arm1.bmp", 0, 0);
        Serial.println("Bild 1");
        break;

      case 2:
        drawBMP("/Arm2.bmp", 0, 0);
        Serial.println("Bild 2");
        break;

      case 3:
        drawBMP("/Arm3.bmp", 0, 0);
        Serial.println("Bild 3");
        break;

      case 4:
        drawBMP("/Arm4.bmp", 0, 0);
        Serial.println("Bild 4");
        break;

      case 5:
        drawBMP("/Arm5.bmp", 0, 0);
        Serial.println("Bild 5");
        break;

      case 6:
        drawBMP("/Arm6.bmp", 0, 0);
        Serial.println("Bild 6");
        break;

      default:
        Serial.println("Ungueltige Nummer. Bitte 1 bis 6 senden.");
        break;
    }
  }
}

void drawBMP(const char *filename, int16_t x, int16_t y) {

  File bmpFile = SD.open(filename, FILE_READ);

  if (!bmpFile) {
    Serial.print("Could not open: ");
    Serial.println(filename);
    return;
  }

  // BMP signature
  if (read16(bmpFile) != 0x4D42) {
    Serial.println("Not a BMP");
    bmpFile.close();
    return;
  }

  read32(bmpFile);  // file size
  read32(bmpFile);  // reserved

  uint32_t imageOffset = read32(bmpFile);

  read32(bmpFile);  // DIB header size

  int32_t bmpWidth = (int32_t)read32(bmpFile);
  int32_t bmpHeight = (int32_t)read32(bmpFile);

  uint16_t planes = read16(bmpFile);
  uint16_t depth = read16(bmpFile);

  uint32_t compression = read32(bmpFile);

  // Only 24-bit uncompressed BMP
  if (planes != 1 || depth != 24 || compression != 0) {

    Serial.println("Only 24-bit uncompressed BMP supported");
    bmpFile.close();
    return;
  }

  bool flip = true;

  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  // Row is padded to multiple of 4 bytes
  uint32_t rowSize =
    (bmpWidth * 3 + 3) & ~3;

  if (bmpWidth > 320) {
    Serial.println("BMP too wide");
    bmpFile.close();
    return;
  }

  int drawWidth = bmpWidth;

  if (x + drawWidth > tft.width())
    drawWidth = tft.width() - x;

  int drawHeight = bmpHeight;

  if (y + drawHeight > tft.height())
    drawHeight = tft.height() - y;

  if (drawWidth <= 0 || drawHeight <= 0) {
    bmpFile.close();
    return;
  }

  // Start one long TFT transaction
  tft.startWrite();

  tft.setAddrWindow(
    x,
    y,
    drawWidth,
    drawHeight);

  for (int row = 0; row < drawHeight; row++) {

    uint32_t position;

    if (flip) {
      position =
        imageOffset + (bmpHeight - 1 - row) * rowSize;
    } else {
      position =
        imageOffset + row * rowSize;
    }

    bmpFile.seek(position);

    // ------------------------------
    // ONE SD READ FOR ENTIRE ROW
    // ------------------------------

    int bytesNeeded = drawWidth * 3;

    int bytesRead =
      bmpFile.read(
        bmpBuffer,
        bytesNeeded);

    if (bytesRead != bytesNeeded) {
      Serial.println("BMP read error");
      break;
    }

    // ------------------------------
    // BGR888 -> RGB565
    // ------------------------------

    uint8_t *src = bmpBuffer;

    for (int col = 0; col < drawWidth; col++) {

      uint8_t b = *src++;
      uint8_t g = *src++;
      uint8_t r = *src++;

      // Faster than calling color565()
      lineBuffer[col] =
        ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // ------------------------------
    // Send whole row to TFT
    // ------------------------------

    tft.pushPixels(
      lineBuffer,
      drawWidth);
  }

  tft.endWrite();

  bmpFile.close();
}

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}


bool isTransparent(uint8_t r, uint8_t g, uint8_t b) {
  const int tolerance = 15;

  return abs((int)r - backgroundR) <= tolerance &&
         abs((int)g - backgroundG) <= tolerance &&
         abs((int)b - backgroundB) <= tolerance;
}

bool loadBMPCompressed(
  const char *filename,
  CompressedBMP &img) {

  File bmpFile = SD.open(filename, FILE_READ);

  if (!bmpFile) {
    Serial.print("Could not open: ");
    Serial.println(filename);
    return false;
  }

  // ----------------------------------
  // BMP header
  // ----------------------------------

  if (read16(bmpFile) != 0x4D42) {
    Serial.println("Not a BMP");
    bmpFile.close();
    return false;
  }

  read32(bmpFile);  // file size
  read32(bmpFile);  // reserved

  uint32_t imageOffset = read32(bmpFile);

  read32(bmpFile);  // DIB header

  int32_t bmpWidth =
    (int32_t)read32(bmpFile);

  int32_t bmpHeight =
    (int32_t)read32(bmpFile);

  uint16_t planes = read16(bmpFile);
  uint16_t depth = read16(bmpFile);

  uint32_t compression = read32(bmpFile);

  if (
    planes != 1 || depth != 24 || compression != 0) {

    Serial.println(
      "BMP must be 24-bit uncompressed");

    bmpFile.close();
    return false;
  }

  bool flip = true;

  if (bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip = false;
  }

  img.width = bmpWidth;
  img.height = bmpHeight;

  uint32_t rowSize =
    (bmpWidth * 3 + 3) & ~3;

  // Temporary row buffer
  uint8_t *rowBuffer =
    (uint8_t *)malloc(rowSize);

  if (!rowBuffer) {
    Serial.println(
      "Could not allocate row buffer");

    bmpFile.close();
    return false;
  }

  // ==================================================
  // PASS 1
  // Count visible pixels and spans
  // ==================================================

  uint32_t totalPixels = 0;
  uint32_t totalSpans = 0;

  for (
    int row = 0;
    row < bmpHeight;
    row++) {

    uint32_t position;

    if (flip) {
      position =
        imageOffset + (bmpHeight - 1 - row) * rowSize;
    } else {
      position =
        imageOffset + row * rowSize;
    }

    bmpFile.seek(position);

    if (
      bmpFile.read(
        rowBuffer,
        rowSize)
      != rowSize) {

      Serial.println(
        "BMP read error");

      free(rowBuffer);
      bmpFile.close();

      return false;
    }

    bool insideSpan = false;

    for (
      int x = 0;
      x < bmpWidth;
      x++) {

      uint8_t *p =
        &rowBuffer[x * 3];

      uint8_t b = p[0];
      uint8_t g = p[1];
      uint8_t r = p[2];

      bool transparent =
        isTransparent(r, g, b);

      if (!transparent) {

        totalPixels++;

        if (!insideSpan) {
          totalSpans++;
          insideSpan = true;
        }
      } else {
        insideSpan = false;
      }
    }
  }

  Serial.print("Visible pixels: ");
  Serial.println(totalPixels);

  Serial.print("Spans: ");
  Serial.println(totalSpans);

  // ==================================================
  // Allocate exact amount of memory
  // ==================================================

  img.pixels =
    (uint16_t *)imageMalloc(
      totalPixels * sizeof(uint16_t));

  img.spans =
    (ImageSpan *)imageMalloc(
      totalSpans * sizeof(ImageSpan));

  if (
    img.pixels == nullptr || img.spans == nullptr) {

    Serial.println(
      "Image RAM allocation failed");

    if (img.pixels)
      free(img.pixels);

    if (img.spans)
      free(img.spans);

    img.pixels = nullptr;
    img.spans = nullptr;

    free(rowBuffer);
    bmpFile.close();

    return false;
  }

  img.pixelCount = totalPixels;
  img.spanCount = totalSpans;

  // ==================================================
  // PASS 2
  // Build compressed representation
  // ==================================================

  uint32_t pixelIndex = 0;
  uint32_t spanIndex = 0;

  for (
    int row = 0;
    row < bmpHeight;
    row++) {

    uint32_t position;

    if (flip) {
      position =
        imageOffset + (bmpHeight - 1 - row) * rowSize;
    } else {
      position =
        imageOffset + row * rowSize;
    }

    bmpFile.seek(position);

    bmpFile.read(
      rowBuffer,
      rowSize);

    bool insideSpan = false;

    uint16_t spanStart = 0;
    uint16_t spanLength = 0;
    uint32_t spanPixelStart = 0;

    for (
      int x = 0;
      x < bmpWidth;
      x++) {

      uint8_t *p =
        &rowBuffer[x * 3];

      uint8_t b = p[0];
      uint8_t g = p[1];
      uint8_t r = p[2];

      bool transparent =
        isTransparent(r, g, b);

      if (!transparent) {

        // Start new span
        if (!insideSpan) {

          insideSpan = true;

          spanStart = x;
          spanLength = 0;

          spanPixelStart =
            pixelIndex;
        }

        // Convert directly to RGB565
        img.pixels[pixelIndex++] =
          ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

        spanLength++;
      }

      // End of current span
      if (
        insideSpan && (transparent || x == bmpWidth - 1)) {

        ImageSpan &span =
          img.spans[spanIndex++];

        span.x = spanStart;
        span.y = row;
        span.length = spanLength;

        span.pixelOffset =
          spanPixelStart;

        insideSpan = false;
      }
    }
  }

  free(rowBuffer);
  bmpFile.close();

  // ----------------------------------
  // Stats
  // ----------------------------------

  uint32_t pixelRAM =
    img.pixelCount * sizeof(uint16_t);

  uint32_t spanRAM =
    img.spanCount * sizeof(ImageSpan);

  Serial.println();
  Serial.print("Compressed RAM: ");
  Serial.print(
    pixelRAM + spanRAM);
  Serial.println(" bytes");

  Serial.print("Pixel data: ");
  Serial.print(pixelRAM);
  Serial.println(" bytes");

  Serial.print("Span data: ");
  Serial.print(spanRAM);
  Serial.println(" bytes");

  return true;
}

void drawCompressedBMP(
  const CompressedBMP &img,
  int16_t xOffset,
  int16_t yOffset) {

  tft.startWrite();

  for (
    uint32_t i = 0;
    i < img.spanCount;
    i++) {

    const ImageSpan &span =
      img.spans[i];

    int16_t x =
      xOffset + span.x;

    int16_t y =
      yOffset + span.y;

    // Skip spans outside display
    if (
      y < 0 || y >= tft.height()) {
      continue;
    }

    if (
      x >= tft.width() || x + span.length <= 0) {
      continue;
    }

    tft.pushImage(
      x,
      y,
      span.length,
      1,
      &img.pixels[span.pixelOffset]);
  }

  tft.endWrite();
}
void *imageMalloc(size_t bytes) {

  if (psramFound()) {
    return ps_malloc(bytes);
  }

  return malloc(bytes);
}