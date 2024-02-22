#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

char ssid[50];     // Array to hold SSID
char password[50]; // Array to hold password

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Enter your WiFi credentials:");
  while (!Serial.available()) {
    // Wait for user input
  }
  
  // Read SSID from Serial Monitor
  String inputSSID = Serial.readStringUntil('\n');
  inputSSID.trim();
  inputSSID.toCharArray(ssid, sizeof(ssid));

  Serial.println("Enter your WiFi password:");
  while (!Serial.available()) {
    // Wait for user input
  }
  
  // Read password from Serial Monitor
  String inputPassword = Serial.readStringUntil('\n');
  inputPassword.trim();
  inputPassword.toCharArray(password, sizeof(password));

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  timeClient.begin();
}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  delay(1000); // Update every second
}
