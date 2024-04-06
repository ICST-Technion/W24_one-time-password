#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char welcomeText[] = "Welcome!";

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void loop() {
  static int scrollPosition = SCREEN_WIDTH;

  display.clearDisplay();
  display.setCursor(scrollPosition, 10);
  display.print(welcomeText);

  scrollPosition--;

  if (scrollPosition < -display.width()) {
    scrollPosition = SCREEN_WIDTH;
  }

  display.display();
  delay(10);
}