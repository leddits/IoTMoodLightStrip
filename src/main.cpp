// library import
#include <ESP8266WebServer.h>  // For Web Server
#include <ESP8266WiFi.h>       // For WiFi AP mode
#include <SPI.h>               // For OLED
#include <Wire.h>              // For OLED
#include <Adafruit_SSD1306.h>  // For OLED
#include "credential.h"
#include "definitions.h"
#include "externalFunc.h"
#include <FastLED.h>
#include <EEPROM.h>            // For saving mode to internal storage

ESP8266WebServer server(80);  // 웹 서버 (포트 80)

CRGB leds[MAX_LEDS];  // 최대 크기로 배열 선언, 실제는 NUMPIXELS만큼 사용

// EEPROM 설정
#define EEPROM_SIZE 12
#define EEPROM_MODE_ADDR 0
#define EEPROM_RED_ADDR 1
#define EEPROM_GREEN_ADDR 2
#define EEPROM_BLUE_ADDR 3
#define EEPROM_BRIGHTNESS_ADDR 4
// Warm Light 설정 주소
#define EEPROM_WARM_COLORTEMP_ADDR 5
#define EEPROM_WARM_CHANCE_ADDR 6
#define EEPROM_WARM_MIN_ADDR 7
#define EEPROM_WARM_MAX_ADDR 8
#define EEPROM_WARM_SPEED_ADDR 9
#define EEPROM_WARM_SMOOTH_ADDR 10

// 모드 정의
enum Mode {
  NORMAL_MODE = 0,
  CAMPFIRE_MODE = 1,
  CHRISTMAS_MODE = 2,
  WARMLIGHT_MODE = 3,
  BEATSIN_MODE = 4
};
Mode currentMode;  // EEPROM에서 불러온 값으로 초기화됨

// Warm Light 모드 설정
int warmColorTemp = 3000;  // 색온도 (2000, 3000, 4000, 5000, 6000)
int warmChangeChance = 20;  // 밝기 변화 확률 (0-100%)
int warmMinBrightness = 0;  // 최소 밝기 (0-255)
int warmMaxBrightness = 255;  // 최대 밝기 (0-255)
int warmUpdateSpeed = 50;  // 업데이트 속도 (ms)
int warmSmoothness = 8;  // 전환 부드러움 (1-20, 낮을수록 빠름)

// 함수 선언
void campfireMode();
void christmasMode();
void normalMode();
void warmLightMode();
void beatsinMode();
void updateDisplay();
const char* getModeText();
void saveModeToEEPROM(Mode mode);
Mode loadModeFromEEPROM();
void saveColorToEEPROM(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
void loadColorFromEEPROM();
void saveWarmConfigToEEPROM();
void loadWarmConfigFromEEPROM();
void setupWebServer();
void handleRoot();
void handleStatus();
void handleSetMode();
void handleSetColor();
void handleSetBrightness();
void handleSetWarmConfig();
void handleGetWarmConfig();

void setup()
{
  Serial.begin(115200);

  pinMode(LEDSPIN, OUTPUT);
  
  // EEPROM 초기화 및 저장된 모드/색상 불러오기
  EEPROM.begin(EEPROM_SIZE);
  currentMode = loadModeFromEEPROM();
  loadColorFromEEPROM();
  loadWarmConfigFromEEPROM();
  Serial.print("저장된 모드 불러오기: ");
  Serial.print(currentMode);
  Serial.print(" (");
  Serial.print(getModeText());
  Serial.println(")");
  Serial.print("저장된 RGB: ");
  Serial.print(mr); Serial.print(", ");
  Serial.print(mg); Serial.print(", ");
  Serial.println(mb);
  Serial.print("저장된 밝기: ");
  Serial.println(FastLED.getBrightness());

  // AP 모드 설정
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, password_ap);
  
  Serial.println("\nAP 모드 시작!");
  Serial.print("SSID: ");
  Serial.println(ssid_ap);
  Serial.print("IP 주소: ");
  Serial.println(WiFi.softAPIP());
  
  DisplaySetup();  // OLED 디스플레이 설정
  delay(2000);  // IP 정보 표시 시간
  
  // 현재 상태 표시
  updateDisplay();

  // 웹 서버 설정
  setupWebServer();
  server.begin();
  Serial.println("웹 서버 시작됨 (포트 80)");

  FastLED.addLeds<WS2812B, LEDSPIN, GRB>(leds, NUMPIXELS);
  // FastLED.setBrightness()는 loadColorFromEEPROM()에서 이미 설정됨
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000); // 170개 LED용: 5V, 10000mA (10A)
  FastLED.clear();
}

void loop()
{
  // 웹 서버 요청 처리
  server.handleClient();

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
  static byte firePixels[MAX_LEDS];   // 각 픽셀의 현재 불꽃 강도
  static byte targetPixels[MAX_LEDS]; // 각 픽셀의 목표 강도
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

// 웜라이트 모드
void warmLightMode()
{
  static unsigned long lastUpdate = 0;
  static byte warmPixels[MAX_LEDS];   // 각 픽셀의 현재 밝기
  static byte targetPixels[MAX_LEDS]; // 각 픽셀의 목표 밝기
  static bool initialized = false;
  
  // 색온도에 따른 RGB 값 (근사값)
  int baseRed, baseGreen, baseBlue;
  
  switch(warmColorTemp) {
    case 2000:  // 촛불 색
      baseRed = 255; baseGreen = 147; baseBlue = 41;
      break;
    case 3000:  // 따뜻한 백열등
      baseRed = 255; baseGreen = 180; baseBlue = 107;
      break;
    case 4000:  // 중성 백색
      baseRed = 255; baseGreen = 209; baseBlue = 163;
      break;
    case 5000:  // 주광색
      baseRed = 255; baseGreen = 228; baseBlue = 206;
      break;
    case 6000:  // 차가운 백색
      baseRed = 255; baseGreen = 243; baseBlue = 239;
      break;
    default:  // 기본값 3000K
      baseRed = 255; baseGreen = 180; baseBlue = 107;
  }
  
  // 초기화
  if (!initialized)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      warmPixels[i] = random(50, 200);
      targetPixels[i] = warmPixels[i];
    }
    initialized = true;
  }
  
  if (millis() - lastUpdate > warmUpdateSpeed)
  {
    for (int i = 0; i < NUMPIXELS; i++)
    {
      // 설정된 확률로 새로운 목표값 설정
      if (random(0, 100) < warmChangeChance)
      {
        targetPixels[i] = random(warmMinBrightness, warmMaxBrightness + 1);
      }
      
      // 현재 값을 목표값으로 부드럽게 이동
      int diff = (int)targetPixels[i] - (int)warmPixels[i];
      warmPixels[i] += diff / warmSmoothness;
      
      // 목표값이 낮을 때(50 이하)는 인근 영향 무시
      if (targetPixels[i] > 50 && i > 0 && i < NUMPIXELS - 1)
      {
        int neighborAvg = ((int)warmPixels[i-1] + (int)warmPixels[i+1]) / 2;
        warmPixels[i] = ((int)warmPixels[i] * 9 + neighborAvg) / 10;
      }
      
      // 밝기 조절
      float intensity = warmPixels[i] / 255.0;
      
      // 색온도 적용
      int red = baseRed * intensity;
      int green = baseGreen * intensity;
      int blue = baseBlue * intensity;
      
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
      return "Warm Light";
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

// EEPROM에 모드 저장
void saveModeToEEPROM(Mode mode)
{
  EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)mode);
  EEPROM.commit();
  Serial.print("EEPROM에 모드 저장: ");
  Serial.println(mode);
}

// EEPROM에서 모드 불러오기
Mode loadModeFromEEPROM()
{
  uint8_t savedMode = EEPROM.read(EEPROM_MODE_ADDR);
  
  // 유효한 모드 값인지 확인 (0-4)
  if (savedMode <= 4)
  {
    return (Mode)savedMode;
  }
  else
  {
    // 유효하지 않으면 기본값(모닥불 모드) 반환
    Serial.println("유효하지 않은 EEPROM 값, 기본 모드 사용");
    return CAMPFIRE_MODE;
  }
}

// EEPROM에 RGB 색상 및 밝기 저장
void saveColorToEEPROM(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
  EEPROM.write(EEPROM_RED_ADDR, r);
  EEPROM.write(EEPROM_GREEN_ADDR, g);
  EEPROM.write(EEPROM_BLUE_ADDR, b);
  EEPROM.write(EEPROM_BRIGHTNESS_ADDR, brightness);
  EEPROM.commit();
  Serial.print("EEPROM에 색상 저장: R=");
  Serial.print(r); Serial.print(", G=");
  Serial.print(g); Serial.print(", B=");
  Serial.print(b); Serial.print(", Brightness=");
  Serial.println(brightness);
}

// EEPROM에서 RGB 색상 및 밝기 불러오기
void loadColorFromEEPROM()
{
  uint8_t r = EEPROM.read(EEPROM_RED_ADDR);
  uint8_t g = EEPROM.read(EEPROM_GREEN_ADDR);
  uint8_t b = EEPROM.read(EEPROM_BLUE_ADDR);
  uint8_t brightness = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
  
  // 0xFF는 초기화되지 않은 EEPROM 값
  if (r == 0xFF || g == 0xFF || b == 0xFF || brightness == 0xFF)
  {
    // 기본값 설정
    mr = 255;
    mg = 255;
    mb = 255;
    FastLED.setBrightness(50);
    Serial.println("기본 색상 사용: 흰색, 밝기 50");
  }
  else
  {
    mr = r;
    mg = g;
    mb = b;
    FastLED.setBrightness(brightness);
  }
}

// Warm 설정 EEPROM에 저장
void saveWarmConfigToEEPROM()
{
  // 색온도 저장 (2바이트: 2000~6000)
  EEPROM.write(EEPROM_WARM_COLORTEMP_ADDR, (uint8_t)(warmColorTemp / 100));
  
  EEPROM.write(EEPROM_WARM_CHANCE_ADDR, (uint8_t)warmChangeChance);
  EEPROM.write(EEPROM_WARM_MIN_ADDR, (uint8_t)warmMinBrightness);
  EEPROM.write(EEPROM_WARM_MAX_ADDR, (uint8_t)warmMaxBrightness);
  EEPROM.write(EEPROM_WARM_SPEED_ADDR, (uint8_t)warmUpdateSpeed);
  EEPROM.write(EEPROM_WARM_SMOOTH_ADDR, (uint8_t)warmSmoothness);
  
  EEPROM.commit();
  Serial.println("Warm Light 설정 EEPROM 저장 완료");
}

// Warm 설정 EEPROM에서 불러오기
void loadWarmConfigFromEEPROM()
{
  // 색온도 로드
  uint8_t temp = EEPROM.read(EEPROM_WARM_COLORTEMP_ADDR);
  if (temp != 0xFF && temp >= 20 && temp <= 60) {
    warmColorTemp = temp * 100;
  }
  
  uint8_t chance = EEPROM.read(EEPROM_WARM_CHANCE_ADDR);
  uint8_t minBr = EEPROM.read(EEPROM_WARM_MIN_ADDR);
  uint8_t maxBr = EEPROM.read(EEPROM_WARM_MAX_ADDR);
  uint8_t speed = EEPROM.read(EEPROM_WARM_SPEED_ADDR);
  uint8_t smooth = EEPROM.read(EEPROM_WARM_SMOOTH_ADDR);
  
  if (chance != 0xFF) warmChangeChance = chance;
  if (minBr != 0xFF) warmMinBrightness = minBr;
  if (maxBr != 0xFF) warmMaxBrightness = maxBr;
  if (speed != 0xFF) warmUpdateSpeed = speed;
  if (smooth != 0xFF) warmSmoothness = smooth;
  
  Serial.println("Warm Light 설정 EEPROM 로드 완료");
}

// 웹 서버 설정
void setupWebServer()
{
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/setMode", handleSetMode);
  server.on("/setColor", handleSetColor);
  server.on("/setBrightness", handleSetBrightness);
  server.on("/setWarmConfig", handleSetWarmConfig);
  server.on("/getWarmConfig", handleGetWarmConfig);
}

// 메인 HTML 페이지
void handleRoot()
{
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>IoT Mood Light</title><style>";
  html += "body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;background:#f0f0f0}";
  html += "h1{text-align:center;color:#333}";
  html += ".panel{background:white;padding:20px;margin:15px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".mode-btn{display:inline-block;padding:12px 20px;margin:5px;background:#4CAF50;color:white;border:none;border-radius:5px;cursor:pointer;font-size:14px}";
  html += ".mode-btn:hover{background:#45a049}";
  html += ".mode-btn.active{background:#FF9800}";
  html += ".slider-container{margin:15px 0}";
  html += ".slider-label{display:flex;justify-content:space-between;margin-bottom:5px;color:#555}";
  html += "input[type=range],select{width:100%;height:8px;border-radius:5px;outline:none}";
  html += "select{height:35px;padding:5px;font-size:14px}";
  html += ".status{padding:10px;background:#e3f2fd;border-left:4px solid #2196F3;margin:15px 0;border-radius:4px}";
  html += "#colorPreview{width:100%;height:50px;border-radius:5px;margin-top:10px;border:2px solid #ddd}";
  html += "</style></head><body>";
  html += "<h1>IoT Mood Light</h1>";
  
  html += "<div class='panel'><h3>Status</h3><div class='status'>";
  html += "<div>Mode: <strong id='mode'>-</strong></div>";
  html += "<div>Brightness: <strong id='brightness'>-</strong></div>";
  html += "<div>RGB: (<span id='r'>-</span>, <span id='g'>-</span>, <span id='b'>-</span>)</div>";
  html += "</div></div>";
  
  html += "<div class='panel'><h3>Mode</h3>";
  html += "<button class='mode-btn' onclick='setMode(0)'>Normal</button>";
  html += "<button class='mode-btn' onclick='setMode(1)'>Campfire</button>";
  html += "<button class='mode-btn' onclick='setMode(2)'>Christmas</button>";
  html += "<button class='mode-btn' onclick='setMode(3)'>Warm Light</button>";
  html += "<button class='mode-btn' onclick='setMode(4)'>Beatsin</button>";
  html += "</div>";
  
  html += "<div class='panel'><h3>Brightness</h3>";
  html += "<div class='slider-container'><div class='slider-label'><span>Brightness</span><span id='bVal'>50</span></div>";
  html += "<input type='range' id='bSlider' min='0' max='255' value='50' oninput='setBright(this.value)'>";
  html += "</div></div>";
  
  html += "<div class='panel'><h3>Color (Normal Mode)</h3>";
  html += "<div class='slider-container'><div class='slider-label'><span>Red</span><span id='rVal'>255</span></div>";
  html += "<input type='range' id='rSlider' min='0' max='255' value='255' oninput='setColor()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Green</span><span id='gVal'>255</span></div>";
  html += "<input type='range' id='gSlider' min='0' max='255' value='255' oninput='setColor()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Blue</span><span id='bSlider2'>255</span></div>";
  html += "<input type='range' id='blSlider' min='0' max='255' value='255' oninput='setColor()'></div>";
  html += "<div id='preview'></div></div>";
  
  html += "<div class='panel' id='warmPanel' style='display:none'><h3>Warm Light Settings</h3>";
  html += "<div class='slider-container'><div class='slider-label'><span>Color Temperature</span></div>";
  html += "<select id='wtempSelect' onchange='setWarmConfig()'>";
  html += "<option value='2000'>2000K (Candlelight)</option>";
  html += "<option value='3000' selected>3000K (Warm)</option>";
  html += "<option value='4000'>4000K (Neutral)</option>";
  html += "<option value='5000'>5000K (Daylight)</option>";
  html += "<option value='6000'>6000K (Cool)</option>";
  html += "</select></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Change Rate (%)</span><span id='wcVal'>20</span></div>";
  html += "<input type='range' id='wcSlider' min='1' max='100' value='20' oninput='setWarmConfig()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Min Brightness</span><span id='wminVal'>0</span></div>";
  html += "<input type='range' id='wminSlider' min='0' max='255' value='0' oninput='setWarmConfig()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Max Brightness</span><span id='wmaxVal'>255</span></div>";
  html += "<input type='range' id='wmaxSlider' min='0' max='255' value='255' oninput='setWarmConfig()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Speed (ms)</span><span id='wsVal'>50</span></div>";
  html += "<input type='range' id='wsSlider' min='20' max='200' value='50' oninput='setWarmConfig()'></div>";
  html += "<div class='slider-container'><div class='slider-label'><span>Smoothness</span><span id='wsmVal'>8</span></div>";
  html += "<input type='range' id='wsmSlider' min='1' max='20' value='8' oninput='setWarmConfig()'></div>";
  html += "</div>";
  
  html += "<script>";
  html += "var modes=['Normal','Campfire','Christmas','Warm Light','Beatsin'];";
  html += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('mode').textContent=modes[d.mode];";
  html += "document.getElementById('brightness').textContent=d.brightness;";
  html += "document.getElementById('r').textContent=d.red;";
  html += "document.getElementById('g').textContent=d.green;";
  html += "document.getElementById('b').textContent=d.blue;";
  html += "document.getElementById('rSlider').value=d.red;";
  html += "document.getElementById('gSlider').value=d.green;";
  html += "document.getElementById('blSlider').value=d.blue;";
  html += "document.getElementById('bSlider').value=d.brightness;";
  html += "document.getElementById('rVal').textContent=d.red;";
  html += "document.getElementById('gVal').textContent=d.green;";
  html += "document.getElementById('bSlider2').textContent=d.blue;";
  html += "document.getElementById('bVal').textContent=d.brightness;";
  html += "updatePreview();highlightMode(d.mode);";
  html += "}).catch(err=>console.error(err));}";
  html += "function highlightMode(m){var btns=document.querySelectorAll('.mode-btn');";
  html += "btns.forEach((btn,i)=>{btn.classList.toggle('active',i===m);});";
  html += "document.getElementById('warmPanel').style.display=m===3?'block':'none';";
  html += "if(m===3)loadWarmConfig();}";
  html += "function loadWarmConfig(){fetch('/getWarmConfig').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('wtempSelect').value=d.temp;";
  html += "document.getElementById('wcSlider').value=d.chance;";
  html += "document.getElementById('wminSlider').value=d.minBright;";
  html += "document.getElementById('wmaxSlider').value=d.maxBright;";
  html += "document.getElementById('wsSlider').value=d.speed;";
  html += "document.getElementById('wsmSlider').value=d.smooth;";
  html += "document.getElementById('wcVal').textContent=d.chance;";
  html += "document.getElementById('wminVal').textContent=d.minBright;";
  html += "document.getElementById('wmaxVal').textContent=d.maxBright;";
  html += "document.getElementById('wsVal').textContent=d.speed;";
  html += "document.getElementById('wsmVal').textContent=d.smooth;";
  html += "}).catch(err=>console.error(err));}";
  html += "function setWarmConfig(){var temp=document.getElementById('wtempSelect').value;";
  html += "var c=document.getElementById('wcSlider').value;";
  html += "var min=document.getElementById('wminSlider').value;";
  html += "var max=document.getElementById('wmaxSlider').value;";
  html += "var s=document.getElementById('wsSlider').value;";
  html += "var sm=document.getElementById('wsmSlider').value;";
  html += "document.getElementById('wcVal').textContent=c;";
  html += "document.getElementById('wminVal').textContent=min;";
  html += "document.getElementById('wmaxVal').textContent=max;";
  html += "document.getElementById('wsVal').textContent=s;";
  html += "document.getElementById('wsmVal').textContent=sm;";
  html += "fetch('/setWarmConfig?temp='+temp+'&c='+c+'&min='+min+'&max='+max+'&s='+s+'&sm='+sm);}";
  html += "function updatePreview(){var r=document.getElementById('rSlider').value;";
  html += "var g=document.getElementById('gSlider').value;";
  html += "var b=document.getElementById('blSlider').value;";
  html += "document.getElementById('preview').style.backgroundColor='rgb('+r+','+g+','+b+')';}";
  html += "function setMode(m){fetch('/setMode?mode='+m).then(()=>setTimeout(updateStatus,100));}";
  html += "function setColor(){var r=document.getElementById('rSlider').value;";
  html += "var g=document.getElementById('gSlider').value;";
  html += "var b=document.getElementById('blSlider').value;";
  html += "document.getElementById('rVal').textContent=r;";
  html += "document.getElementById('gVal').textContent=g;";
  html += "document.getElementById('bSlider2').textContent=b;";
  html += "updatePreview();fetch('/setColor?r='+r+'&g='+g+'&b='+b);}";
  html += "function setBright(v){document.getElementById('bVal').textContent=v;";
  html += "fetch('/setBrightness?value='+v);}";
  html += "updateStatus();setInterval(updateStatus,3000);";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

// 현재 상태 반환 (JSON)
void handleStatus()
{
  String json = "{";
  json += "\"mode\":" + String((int)currentMode) + ",";
  json += "\"red\":" + String(mr) + ",";
  json += "\"green\":" + String(mg) + ",";
  json += "\"blue\":" + String(mb) + ",";
  json += "\"brightness\":" + String(FastLED.getBrightness());
  json += "}";
  
  server.send(200, "application/json", json);
}

// 모드 변경
void handleSetMode()
{
  if (server.hasArg("mode"))
  {
    int modeValue = server.arg("mode").toInt();
    if (modeValue >= 0 && modeValue <= 4)
    {
      currentMode = (Mode)modeValue;
      saveModeToEEPROM(currentMode);
      updateDisplay();
      Serial.print("웹에서 모드 변경: ");
      Serial.println(modeValue);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid mode");
}

// 색상 변경
void handleSetColor()
{
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b"))
  {
    mr = server.arg("r").toInt();
    mg = server.arg("g").toInt();
    mb = server.arg("b").toInt();
    
    saveColorToEEPROM(mr, mg, mb, FastLED.getBrightness());
    
    Serial.print("웹에서 색상 변경: R=");
    Serial.print(mr); Serial.print(" G=");
    Serial.print(mg); Serial.print(" B=");
    Serial.println(mb);
    
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Invalid color");
}

// 밝기 변경
void handleSetBrightness()
{
  if (server.hasArg("value"))
  {
    int brightness = server.arg("value").toInt();
    if (brightness >= 0 && brightness <= 255)
    {
      FastLED.setBrightness(brightness);
      saveColorToEEPROM(mr, mg, mb, brightness);
      
      Serial.print("웹에서 밝기 변경: ");
      Serial.println(brightness);
      
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid brightness");
}

// Warm Light 설정 가져오기
void handleGetWarmConfig()
{
  String json = "{";
  json += "\"temp\":" + String(warmColorTemp) + ",";
  json += "\"chance\":" + String(warmChangeChance) + ",";
  json += "\"minBright\":" + String(warmMinBrightness) + ",";
  json += "\"maxBright\":" + String(warmMaxBrightness) + ",";
  json += "\"speed\":" + String(warmUpdateSpeed) + ",";
  json += "\"smooth\":" + String(warmSmoothness);
  json += "}";
  
  server.send(200, "application/json", json);
}

// Warm Light 설정 변경
void handleSetWarmConfig()
{
  if (server.hasArg("temp") && server.hasArg("c") && server.hasArg("min") && 
      server.hasArg("max") && server.hasArg("s") && server.hasArg("sm"))
  {
    int temp = server.arg("temp").toInt();
    if (temp == 2000 || temp == 3000 || temp == 4000 || temp == 5000 || temp == 6000)
    {
      warmColorTemp = temp;
    }
    
    warmChangeChance = constrain(server.arg("c").toInt(), 1, 100);
    warmMinBrightness = constrain(server.arg("min").toInt(), 0, 255);
    warmMaxBrightness = constrain(server.arg("max").toInt(), 0, 255);
    warmUpdateSpeed = constrain(server.arg("s").toInt(), 20, 200);
    warmSmoothness = constrain(server.arg("sm").toInt(), 1, 20);
    
    saveWarmConfigToEEPROM();  // EEPROM에 저장
    
    Serial.println("Warm Light 설정 변경:");
    Serial.print("  색온도: "); Serial.print(warmColorTemp); Serial.println("K");
    Serial.print("  변화 확률: "); Serial.println(warmChangeChance);
    Serial.print("  밝기 범위: "); Serial.print(warmMinBrightness); 
    Serial.print(" - "); Serial.println(warmMaxBrightness);
    Serial.print("  속도: "); Serial.println(warmUpdateSpeed);
    Serial.print("  부드러움: "); Serial.println(warmSmoothness);
    
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Invalid config");
}
