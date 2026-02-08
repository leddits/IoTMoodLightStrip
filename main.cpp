#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// 함수 선언
void encoderISR();
void handleEncoder();
void handleButton();
void setAllPixels(uint8_t r, uint8_t g, uint8_t b);
void campfireMode();
void christmasMode();

// 핀 정의
#define LED_PIN       5    // NeoPixel 데이터 핀 (D5)
#define ENCODER_CLK   2    // 로터리 엔코더 CLK 핀 (D2)
#define ENCODER_DT    3    // 로터리 엔코더 DT 핀 (D3)
#define ENCODER_SW    4    // 로터리 엔코더 버튼 핀 (D4)

// NeoPixel 설정
#define NUM_PIXELS    146  // LED 개수 (146개 전체)
#define SKIP_PIXELS   18   // 앞쪽 18개는 항상 꺼진 상태로 유지
// 색상 순서 옵션들 (녹색이 보이면 GRB가 맞는 것 같음)
#define PIXEL_TYPE    (NEO_GRB + NEO_KHZ800)  // GRB 순서로 다시 변경
//#define PIXEL_TYPE    (NEO_RGB + NEO_KHZ800)  // RGB 순서
//#define PIXEL_TYPE    (NEO_BRG + NEO_KHZ800)  // BRG 순서
//#define PIXEL_TYPE    (NEO_BGR + NEO_KHZ800)  // BGR 순서

// NeoPixel 객체 생성
Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, PIXEL_TYPE);

// 변수들 (메모리 최적화)
volatile bool encoderChanged = false;
volatile int encoderPos = 0;
int lastEncoderPos = 0;
byte lastCLK = HIGH;         // int 대신 byte 사용
byte lastDT = HIGH;          // int 대신 byte 사용
byte brightness = 40;        // 초기 밝기 (안전을 위해 낮게 설정)
const byte maxBrightness = 150;     // 최대 밝기 150으로 조정
const byte minBrightness = 0;       // const로 메모리 절약
const byte brightnessStep = 25;     // const로 메모리 절약

unsigned long lastEncoderTime = 0;

// 모드 및 버튼 변수들
enum Mode {
  NORMAL_MODE = 0,
  CAMPFIRE_MODE = 1,
  CHRISTMAS_MODE = 2
};
Mode currentMode = NORMAL_MODE;

byte lastButtonState = HIGH;               // bool 대신 byte
unsigned long lastButtonPress = 0;
const unsigned int buttonDebounce = 200;   // long 대신 int

void setup() {
  Serial.begin(9600);
  
  // 핀 모드 설정
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  
  // 초기 상태 읽기
  lastCLK = digitalRead(ENCODER_CLK);
  lastDT = digitalRead(ENCODER_DT);
  
  // 인터럽트 설정 (CLK와 DT 둘 다)
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);
  
  // NeoPixel 초기화
  strip.begin();
  strip.show(); // 모든 픽셀 OFF로 초기화
  strip.setBrightness(brightness);
  
  // 초기 색상 설정 (흰색)
  setAllPixels(255, 255, 255);
  
  Serial.println("NeoPixel 밝기 조절기 시작!");
  Serial.print("초기 밝기: ");
  Serial.println(brightness);
}

void loop() {
  // 로터리 엔코더 변화 감지
  if (encoderChanged) {
    handleEncoder();
    encoderChanged = false;
  }
  
  // 버튼 처리
  handleButton();
  
  // 현재 모드에 따른 동작
  if (currentMode == CAMPFIRE_MODE) {
    campfireMode();
  } else {
    christmasMode();
  }
  
  delay(2); // 메모리 최적화를 위해 약간 늘린 지연
}

// 로터리 엔코더 인터럽트 서비스 루틴
void encoderISR() {
  // 더 빠른 디바운싱을 위한 시간 체크
  unsigned long currentTime = millis();
  if (currentTime - lastEncoderTime > 2) {
    int currentCLK = digitalRead(ENCODER_CLK);
    int currentDT = digitalRead(ENCODER_DT);
    
    // CLK가 falling edge일 때만 처리 (더 안정적이고 빠름)
    if (lastCLK == HIGH && currentCLK == LOW) {
      if (currentDT == HIGH) {
        encoderPos++;  // 시계방향
      } else {
        encoderPos--;  // 반시계방향
      }
      encoderChanged = true;
      lastEncoderTime = currentTime;
    }
    
    lastCLK = currentCLK;
    lastDT = currentDT;
  }
}

// 로터리 엔코더 처리
void handleEncoder() {
  if (encoderPos != lastEncoderPos) {
    int direction = encoderPos - lastEncoderPos;
    
    if (direction > 0) {
      // 시계방향 회전 - 밝기 증가 (오버플로우 방지)
      if (brightness <= maxBrightness - brightnessStep) {
        brightness += brightnessStep;
      } else {
        brightness = maxBrightness;  // 최대값에서 정지
      }
    } else if (direction < 0) {
      // 반시계방향 회전 - 밝기 감소 (언더플로우 방지)
      if (brightness >= minBrightness + brightnessStep) {
        brightness -= brightnessStep;
      } else {
        brightness = minBrightness;  // 최소값에서 정지
      }
    }
    
    // 밝기 적용
    strip.setBrightness(brightness);
    
    // 모든 모드에서 밝기는 각 모드 함수에서 처리됨
    
    // 시리얼 모니터에 현재 밝기 출력
    Serial.print("밝기: ");
    Serial.print(brightness);
    Serial.print(" (");
    Serial.print((brightness * 100) / 255);
    Serial.print("%)");
    if (brightness == 0) {
      Serial.println(" - LED 꺼짐");
    } else {
      Serial.println();
    }
    
    lastEncoderPos = encoderPos;
  }
}

// 버튼 처리 함수
void handleButton() {
  bool currentButtonState = digitalRead(ENCODER_SW);
  
  if (currentButtonState != lastButtonState) {
    if (millis() - lastButtonPress > buttonDebounce) {
      if (currentButtonState == LOW && lastButtonState == HIGH) {
        // 버튼이 눌렸을 때 모드 토글
        if (currentMode == CAMPFIRE_MODE) {
          currentMode = CHRISTMAS_MODE;
          Serial.println("모드 변경: 크리스마스 모드");
        } else {
          currentMode = CAMPFIRE_MODE;
          Serial.println("모드 변경: 모닥불 모드");
        }
        lastButtonPress = millis();
      }
    }
  }
  lastButtonState = currentButtonState;
}

// 모닥불 모드 (메모리 극도 최적화)
void campfireMode() {
  static unsigned long lastUpdate = 0;
  static byte firePixels[NUM_PIXELS]; // 각 픽셀의 현재 불꽃 강도 (byte로 최적화: 1바이트)
  static byte targetPixels[NUM_PIXELS]; // 각 픽셀의 목표 강도 (byte로 최적화: 1바이트)
  static bool initialized = false;
  
  // 초기화 (앞쪽 18개는 제외하고 나머지만)
  if (!initialized) {
    // 앞쪽 18개는 0으로 초기화
    for (int i = 0; i < SKIP_PIXELS; i++) {
      firePixels[i] = 0;
      targetPixels[i] = 0;
    }
    // 나머지는 랜덤 값으로 초기화
    for (int i = SKIP_PIXELS; i < NUM_PIXELS; i++) {
      firePixels[i] = random(50, 200);
      targetPixels[i] = firePixels[i];
    }
    initialized = true;
  }
  
  if (millis() - lastUpdate > 70) { // 70ms마다 업데이트 (메모리 최적화를 위해 속도 조정)
    
    // 앞쪽 18개는 건드리지 않고 나머지에만 모닥불 효과 적용
    for (int i = SKIP_PIXELS; i < NUM_PIXELS; i++) {
      
      // 가끔씩 새로운 목표값 설정 (15% 확률)
      if (random(0, 100) < 15) {
        targetPixels[i] = random(40, 220);
      }
      
      // 현재 값을 목표값으로 부드럽게 이동 (스무딩)
      int diff = (int)targetPixels[i] - (int)firePixels[i];
      firePixels[i] += diff / 10; // 1/10씩 천천히 변화
      
      // 인근 픽셀들의 영향 추가 (불꽃 확산 효과)
      if (i > 0 && i < NUM_PIXELS - 1) {
        int neighborAvg = ((int)firePixels[i-1] + (int)firePixels[i+1]) / 2;
        firePixels[i] = ((int)firePixels[i] * 4 + neighborAvg) / 5; // 20% 인근 영향
      }
      
      // 밝기 설정에 따른 전체 강도 조절
      int scaledIntensity = map(firePixels[i], 0, 255, 0, brightness);
      
      // 모닥불 색상: 주로 빨간색, 약간의 주황색
      int red = scaledIntensity;
      int green = scaledIntensity / 5; // 빨간색의 1/5만 사용
      int blue = 0; // 파란색은 완전히 제거
      
      // 가끔씩 더 밝은 불꽃 효과 (5% 확률로 줄임)
      if (random(0, 100) < 5) {
        red = min(255, red + random(20, 50));
        green = min(60, green + random(5, 15)); // 녹색 더 제한
      }
      
      // 최소 밝기 보장 (완전히 꺼지지 않게)
      red = max(red, brightness / 10);
      green = max(green, brightness / 20);
      
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    
    strip.show();
    lastUpdate = millis();
  }
}

// 크리스마스 모드
void christmasMode() {
  static unsigned long lastUpdate = 0;
  static unsigned long patternStart = 0;
  static int phase = 0; // 0: 빨간색 켜짐, 1: 초록색 켜짐, 2: 둘 다 반짝임
  static bool sparkleState = false;
  
  if (millis() - lastUpdate > 250) { // 250ms마다 업데이트 (메모리 최적화)
    
    // 3초마다 패턴 변경
    if (millis() - patternStart > 3000) {
      phase = (phase + 1) % 3;
      patternStart = millis();
      Serial.print("크리스마스 패턴: ");
      if (phase == 0) Serial.println("빨간색");
      else if (phase == 1) Serial.println("초록색");
      else Serial.println("반짝임");
    }
    
    // 앞쪽 18개는 항상 꺼진 상태로 유지
    for (int i = 0; i < SKIP_PIXELS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    
    // 나머지 LED들에 크리스마스 패턴 적용
    for (int i = SKIP_PIXELS; i < NUM_PIXELS; i++) {
      int red = 0, green = 0, blue = 0;
      
      if (phase == 0) {
        // 빨간색 위주, 가끔 초록색
        if (i % 4 == 0 || i % 4 == 1) {
          red = map(brightness, 0, 255, 0, 255);   // 빨간색
          green = 0;
        } else {
          red = 0;
          green = map(brightness, 0, 255, 0, 255); // 초록색
        }
      } 
      else if (phase == 1) {
        // 초록색 위주, 가끔 빨간색
        if (i % 4 == 0 || i % 4 == 1) {
          red = 0;
          green = map(brightness, 0, 255, 0, 255); // 초록색
        } else {
          red = map(brightness, 0, 255, 0, 255);   // 빨간색
          green = 0;
        }
      }
      else {
        // 둘 다 반짝임
        sparkleState = !sparkleState;
        if (sparkleState) {
          if (i % 2 == 0) {
            red = map(brightness, 0, 255, 0, 255);
            green = 0;
          } else {
            red = 0;
            green = map(brightness, 0, 255, 0, 255);
          }
        } else {
          // 약간 어둡게
          if (i % 2 == 0) {
            red = map(brightness, 0, 255, 0, 100);
            green = 0;
          } else {
            red = 0;
            green = map(brightness, 0, 255, 0, 100);
          }
        }
      }
      
      // 가끔씩 흰색 반짝임 추가 (별 효과)
      if (random(0, 100) < 3) {
        red = map(brightness, 0, 255, 0, 255);
        green = map(brightness, 0, 255, 0, 255);
        blue = map(brightness, 0, 255, 0, 200);
      }
      
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    
    strip.show();
    lastUpdate = millis();
  }
}

// 앞쪽 18개는 건드리고 나머지 픽셀을 지정된 색상으로 설정
void setAllPixels(uint8_t r, uint8_t g, uint8_t b) {
  // 앞쪽 18개는 항상 꺼진 상태
  for (int i = 0; i < SKIP_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  // 나머지 LED들은 지정된 색상으로 설정
  for (int i = SKIP_PIXELS; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}