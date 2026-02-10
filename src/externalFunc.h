
void DisplaySetup()
{
  // LCD begin
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.display();
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(32, 22);
  display.print(F("IP Address: "));
  display.setCursor(32, 32);
  display.print(WiFi.softAPIP());
  display.setCursor(32, 42);
  display.print(F("Port: 80"));
  display.setCursor(32, 52);
  display.print(F("MAC: "));
  display.print(macID.substring(12)); // MAC 주소 마지막 5자리만 표시
  display.startscrollleft(0x00, 0x07);
  display.display();
}
