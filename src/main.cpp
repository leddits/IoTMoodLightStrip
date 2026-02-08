// library import
#include "FirebaseESP8266.h"
#include <WiFiManager.h>       // For WiFi config AP
#include <ESP8266WiFi.h>       // For check MacAddress
#include <WebSocketsServer.h>  // For WebSocketComm
#include <SPI.h>               // For OLED
#include <Wire.h>              // For OLED
#include <Adafruit_SSD1306.h>  // For OLED
#include <Adafruit_NeoPixel.h> // For Neopixel
#include "credential.h"
#include "definitions.h"
#include "externalFunc.h"
#include <FastLED.h>

CRGB leds[18];

void setup()
{
  Serial.begin(9600);

  pinMode(LEDSPIN, OUTPUT);

  wifiManager.autoConnect(ssid_ap, password_ap); // wifi connect
  // waiting wifi connect
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }

  FirebaseSetup();
  DisplaySetup();
  GetExternalIP();

  FastLED.addLeds<WS2812B, LEDSPIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(50);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500); // Set power limit of LED strip to 5V, 1500mA
  FastLED.clear();

}

void loop()
{
  EVERY_N_SECONDS(5)
  {
    if (Firebase.getString(ledData, "/Controller/" + macID + "/ledData"))
    {
      if (ledData.stringData())
      {
        mg = ledData.stringData().substring(1, 4).toInt();
        mr = ledData.stringData().substring(4, 7).toInt();
        mb = ledData.stringData().substring(7, 10).toInt();
        uint8_t brightness = ledData.stringData().substring(10, 13).toInt();
        FastLED.setBrightness(brightness);
      }
    }
  }

  uint16_t sinBeat = beatsin16(20, 0, NUMPIXELS - 1, 0, 0);

  leds[sinBeat] = CRGB(mg, mr, mb);

  fadeLightBy(leds, NUMPIXELS, 10);
  FastLED.show();
}
