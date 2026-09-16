#include <SPI.h>
#include <XPT2046_Touchscreen.h>
// Animates white pixels to simulate flying through a star field
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "Arm1.h"
#include "Arm2.h"
#include "Arm3.h"
#include "Arm4.h"
#include "Arm5.h"
#include "Arm6.h"
#define RX2 16
#define TX2 17

TFT_eSPI tft = TFT_eSPI();
HardwareSerial SerialSecond(2);
void setup() {
  Serial.begin(115200);

  tft.init();

  // Landscape
  tft.setRotation(3);

  Serial.print("Breite: ");
  Serial.println(tft.width());

  Serial.print("Hoehe: ");
  Serial.println(tft.height());

  tft.fillScreen(TFT_BLACK);

  SerialSecond.begin(
    9600,           // baud rate
    SERIAL_8N1,
    RX2,
    TX2
  );

  // Fuer die erzeugten RGB565-Bilder
  tft.setSwapBytes(true);

  tft.pushImage(
    0,
    0,
    ARM1_WIDTH,
    ARM1_HEIGHT,
    Arm1
  );
}

void loop() {
  if (SerialSecond.available()) {

    int nummer = SerialSecond.parseInt();

    // Restliche Zeichen wie \n / \r entfernen
    while (SerialSecond.available()) {
      SerialSecond.read();
    }

    switch (nummer) {

      case 1:
        tft.pushImage(0, 0, 320, 240, Arm1);
        Serial.println("Bild 1");
        break;

      case 2:
        tft.pushImage(0, 0, 320, 240, Arm2);
        Serial.println("Bild 2");
        break;

      case 3:
        tft.pushImage(0, 0, 320, 240, Arm3);
        Serial.println("Bild 3");
        break;

      case 4:
        tft.pushImage(0, 0, 320, 240, Arm4);
        Serial.println("Bild 4");
        break;

      case 5:
        tft.pushImage(0, 0, 320, 240, Arm5);
        Serial.println("Bild 5");
        break;

      case 6:
        tft.pushImage(0, 0, 320, 240, Arm6);
        Serial.println("Bild 6");
        break;

      default:
        Serial.println("Ungueltige Nummer. Bitte 1 bis 6 senden.");
        break;
    }
  }
}