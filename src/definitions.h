// version
String version = "2.0";

// timing count
int count = 0;
int term = 1500;

// wifi setting
const char *ssid_ap = "AIMoodLight";
const char *password_ap = "";
WiFiManager wifiManager;

// check mac address
ESP8266WiFiClass Wifi8266;
String macID = Wifi8266.macAddress();

// firebase setting
FirebaseData firebaseData;
FirebaseData ledData;

// neopixel setting
#define LEDSPIN 2                                                                  
int NUMPIXELS = 18;
Adafruit_NeoPixel pixels(NUMPIXELS, LEDSPIN, NEO_RGB + NEO_KHZ800);
int mr = 0;
int mg = 0;
int mb = 0;

// OLED setting
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);