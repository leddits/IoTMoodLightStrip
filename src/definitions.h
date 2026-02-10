// version
String version = "2.0";

// timing count
int count = 0;
int term = 1500;

// wifi setting (AP mode)
const char *ssid_ap = "AIMoodLight";
const char *password_ap = "0405";

// check mac address
ESP8266WiFiClass Wifi8266;
String macID = Wifi8266.macAddress();

// neopixel setting
#define LEDSPIN 14  // D5 (GPIO 14)
#define MAX_LEDS 200  // 최대 LED 개수 (배열 크기용)                                                           
int NUMPIXELS = 173;  // 실제 사용할 LED 개수
// Adafruit_NeoPixel pixels(NUMPIXELS, LEDSPIN, NEO_RGB + NEO_KHZ800);  // FastLED 사용으로 주석 처리
int mr = 0;
int mg = 0;
int mb = 0;

// OLED setting
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);