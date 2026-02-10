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

CRGB leds[100];

// 함수 선언
void campfireMode();
void christmasMode();
void normalMode();
void warmLightMode();
void beatsinMode();
void updateDisplay();
const char* getModeText();

// 모드 정의
enum Mode {
  NORMAL_MODE = 0,
  CAMPFIRE_MODE = 1,
  CHRISTMAS_MODE = 2,
  WARMLIGHT_MODE = 3,
  BEATSIN_MODE = 4
};
Mode currentMode = WARMLIGHT_MODE;  // 부팅 시 기본값: 웜라이트 모드

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
  DisplaySetup();  // 기존 DisplaySetup 사용
  GetExternalIP();
  
  delay(2000);  // IP 정보 표시 시간
  
  // 현재 상태 표시
  updateDisplay();

  FastLED.addLeds<WS2812B, LEDSPIN, GRB>(leds, NUMPIXELS);
  FastLED.setBrightness(50);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500); // Set power limit of LED strip to 5V, 1500mA
  FastLED.clear();

}

void loop()
{
  EVERY_N_SECONDS(5)
  {
    // ledData 읽기 (색상 및 밝기)
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
    
    // ledSequence 읽기 (모드 선택)
    FirebaseData modeData;
    if (Firebase.getInt(modeData, "/Controller/" + macID + "/ledSequence"))
    {
      int modeValue = modeData.intData();
      if (modeValue >= 0 && modeValue <= 4)
      {
        Mode oldMode = currentMode;
        currentMode = (Mode)modeValue;
        Serial.print("모드 변경: ");
        Serial.println(modeValue);
        
        // 모드가 변경되었을 때만 디스플레이 업데이트
        if (oldMode != currentMode)
        {
          updateDisplay();
        }
      }
    }
  }

  // 현재 모드에 따른 동작
  switch (currentMode)
  {
    case NORMAL_MODE:
      normalMode();
      break;
    case CAMPFIRE_MODE:
      campfireMode();
      break;
    case CHRISTMAS_MODE:
      christmasMode();
      break;
    case WARMLIGHT_MODE:
      warmLightMode();
      break;
    case BEATSIN_MODE:
      beatsinMode();
      break;
  }
}

// 노말 모드 (단순 LED 켜짐)
void normalMode()
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    leds[i] = CRGB(mg, mr, mb);
  }
  FastLED.show();
}

// Beatsin 모드 (흐르는 효과)
void beatsinMode()
{
  uint16_t sinBeat = beatsin16(20, 0, NUMPIXELS - 1, 0, 0);
  leds[sinBeat] = CRGB(mg, mr, mb);
  fadeLightBy(leds, NUMPIXELS, 10);
  FastLED.show();
}

// 모닥불 모드
void campfireMode()
{
  static unsigned long lastUpdate = 0;
  static byte firePixels[100];   // 각 픽셀의 현재 불꽃 강도
  static byte targetPixels[100]; // 각 픽셀의 목표 강도
  static bool initialized = false;
  
  // 초기화
  if (!initialized)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      firePixels[i] = random(50, 200);
      targetPixels[i] = firePixels[i];
    }
    initialized = true;
  }
  
  if (millis() - lastUpdate > 70)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      // 15% 확률로 새로운 목표값 설정
      if (random(0, 100) < 15)
      {
        targetPixels[i] = random(40, 220);
      }
      
      // 현재 값을 목표값으로 부드럽게 이동
      int diff = (int)targetPixels[i] - (int)firePixels[i];
      firePixels[i] += diff / 10;
      
      // 인근 픽셀들의 영향 추가 (불꽃 확산 효과)
      if (i > 0 && i < NUMPIXELS - 1)
      {
        int neighborAvg = ((int)firePixels[i-1] + (int)firePixels[i+1]) / 2;
        firePixels[i] = ((int)firePixels[i] * 4 + neighborAvg) / 5;
      }
      
      // 밝기 조절
      int intensity = firePixels[i];
      
      // 모닥불 색상: 주로 빨간색, 약간의 주황색
      int red = intensity;
      int green = intensity / 5;
      int blue = 0;
      
      // 5% 확률로 더 밝은 불꽃 효과
      if (random(0, 100) < 5)
      {
        red = min(255, red + (int)random(20, 50));
        green = min(60, green + (int)random(5, 15));
      }
      
      // 최소 밝기 보장
      red = max(red, 10);
      green = max(green, 5);
      
      leds[i] = CRGB(red, green, blue);
    }
    
    FastLED.show();
    lastUpdate = millis();
  }
}

// 크리스마스 모드
void christmasMode()
{
  static unsigned long lastUpdate = 0;
  static unsigned long patternStart = 0;
  static int phase = 0; // 0: 빨간색 켜짐, 1: 초록색 켜짐, 2: 둘 다 반짝임
  static bool sparkleState = false;
  
  if (millis() - lastUpdate > 250)
  {
    // 3초마다 패턴 변경
    if (millis() - patternStart > 3000)
    {
      phase = (phase + 1) % 3;
      patternStart = millis();
      Serial.print("크리스마스 패턴: ");
      if (phase == 0) Serial.println("빨간색");
      else if (phase == 1) Serial.println("초록색");
      else Serial.println("반짝임");
    }
    
    for (int i = 0; i < NUMPIXELS; i++)
    {
      int red = 0, green = 0, blue = 0;
      
      if (phase == 0)
      {
        // 빨간색 위주, 가끔 초록색
        if (i % 4 == 0 || i % 4 == 1)
        {
          red = 255;
          green = 0;
        }
        else
        {
          red = 0;
          green = 255;
        }
      }
      else if (phase == 1)
      {
        // 초록색 위주, 가끔 빨간색
        if (i % 4 == 0 || i % 4 == 1)
        {
          red = 0;
          green = 255;
        }
        else
        {
          red = 255;
          green = 0;
        }
      }
      else
      {
        // 둘 다 반짝임
        sparkleState = !sparkleState;
        if (sparkleState)
        {
          if (i % 2 == 0)
          {
            red = 255;
            green = 0;
          }
          else
          {
            red = 0;
            green = 255;
          }
        }
        else
        {
          // 약간 어둡게
          if (i % 2 == 0)
          {
            red = 100;
            green = 0;
          }
          else
          {
            red = 0;
            green = 100;
          }
        }
      }
      
      // 3% 확률로 흰색 반짝임 추가 (별 효과)
      if (random(0, 100) < 3)
      {
        red = 255;
        green = 255;
        blue = 200;
      }
      
      leds[i] = CRGB(red, green, blue);
    }
    
    FastLED.show();
    lastUpdate = millis();
  }
}

// 웜라이트 3000K 모드
void warmLightMode()
{
  static unsigned long lastUpdate = 0;
  static byte warmPixels[100];   // 각 픽셀의 현재 밝기
  static byte targetPixels[100]; // 각 픽셀의 목표 밝기
  static bool initialized = false;
  
  // 3000K 색온도 기본 RGB 값
  const int baseRed = 255;
  const int baseGreen = 180;
  const int baseBlue = 107;
  
  // 초기화
  if (!initialized)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      warmPixels[i] = random(200, 240);
      targetPixels[i] = warmPixels[i];
    }
    initialized = true;
  }
  
  if (millis() - lastUpdate > 80)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      // 5% 확률로 새로운 목표값 설정 (매우 미세한 변화)
      if (random(0, 100) < 5)
      {
        targetPixels[i] = random(220, 255);
      }
      
      // 현재 값을 목표값으로 부드럽게 이동
      int diff = (int)targetPixels[i] - (int)warmPixels[i];
      warmPixels[i] += diff / 15; // 더 부드러운 변화
      
      // 인근 픽셀들의 영향 추가 (균일한 조명 효과)
      if (i > 0 && i < NUMPIXELS - 1)
      {
        int neighborAvg = ((int)warmPixels[i-1] + (int)warmPixels[i+1]) / 2;
        warmPixels[i] = ((int)warmPixels[i] * 5 + neighborAvg) / 6;
      }
      
      // 밝기 조절
      float intensity = warmPixels[i] / 255.0;
      
      // 3000K 색온도 적용
      int red = baseRed * intensity;
      int green = baseGreen * intensity;
      int blue = baseBlue * intensity;
      
      // 매우 작은 확률로 미세한 flicker 효과 (1%)
      if (random(0, 100) < 1)
      {
        int flicker = random(-5, 5);
        red = constrain(red + flicker, 0, 255);
        green = constrain(green + flicker, 0, 255);
        blue = constrain(blue + flicker, 0, 255);
      }
      
      leds[i] = CRGB(red, green, blue);
    }
    
    FastLED.show();
    lastUpdate = millis();
  }
}

// 현재 모드 텍스트 반환
const char* getModeText()
{
  switch (currentMode)
  {
    case NORMAL_MODE:
      return "Normal";
    case CAMPFIRE_MODE:
      return "Campfire";
    case CHRISTMAS_MODE:
      return "Christmas";
    case WARMLIGHT_MODE:
      return "Warm 3000K";
    case BEATSIN_MODE:
      return "Beatsin";
    default:
      return "Unknown";
  }
}

// OLED 디스플레이 업데이트
void updateDisplay()
{
  display.stopscroll();
  display.clearDisplay();
  
  // 흰색 배경으로 채우기
  display.fillScreen(WHITE);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.cp437(true);
  
  // WiFi 상태 표시 (1줄)
  display.setCursor(10, 20);
  display.print(F("WiFi: "));
  if (WiFi.status() == WL_CONNECTED)
  {
    display.print(F("Connected"));
  }
  else
  {
    display.print(F("Disconnected"));
  }
  
  // 현재 모드 표시 (2줄)
  display.setCursor(10, 40);
  display.print(F("Mode: "));
  display.print(getModeText());
  
  display.display();
}
