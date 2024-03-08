/**********************************************************************************************************************
 * Include Libraries
 *********************************************************************************************************************/
#include "Adafruit_Keypad.h"
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>
#include <vector>
#include <sstream>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <SPIFFS.h>

/**********************************************************************************************************************
 *  Constants
 *********************************************************************************************************************/
static const char *otpDurationKey = "otp_duration"  ;
static const char *defaultPasswordKey = "default_pass";
static const char *wifiSSIDKey = "wifi_ssid";
static const char *wifiPasswordKey = "wifi_password";
static const char *passwordLengthKey = "password_length";
const int NUM_DAYS = 7;
const int NUM_HOURS = 24;

/**********************************************************************************************************************
 * Global Variables
 *********************************************************************************************************************/
Preferences preferences;
WiFiUDP ntpUDP;
bool newPassword = false;
BluetoothSerial SerialBT;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
time_t otpSendDate = 0;
int otpDuration = 0;
String currentOtp = "";
String defaultPassword = "";
String wifiSSID = "";
String wifiPassword = "";
int passwordLength = 0;
int customerCounts[NUM_DAYS][NUM_HOURS] = {{0}};
bool ack = false;

/**********************************************************************************************************************
 * Oled Initialization
 *********************************************************************************************************************/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/**********************************************************************************************************************
 * Esp-Now Initialization
 *********************************************************************************************************************/
uint8_t receiverMacAddress[] = {0xcc, 0xdb, 0xa7, 0x5a, 0x5b, 0x1c};
esp_now_peer_info_t peerInfo;

// Callback function to handle received data
void OnDataRecv(const uint8_t *macAddr, const uint8_t *data, int len) 
{
  time_t currTime = now();
  String receivedMessage = String((const char*)data);
  Serial.print("Received message: ");
  Serial.println(receivedMessage);
  if (receivedMessage == "Access") {
    // TBD 
    // Print Access on Oled and update statics 
    display.clearDisplay();
    int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(x, y);
    display.println("Access");
    display.display();
    delay(1000);
    printPassword();
  } else if (receivedMessage == "New Password") {
    newPassword = true;
  } else if (receivedMessage == "Ack") {
    ack = true;
  } else {
    // TBD Send notification to user
    display.clearDisplay();
    int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(x, y);
    display.println("Multiple Failed Attempts");
    display.display();
    delay(30000);
    printPassword();
  }
}

/**********************************************************************************************************************
 * Helper functions
 *********************************************************************************************************************/
String generatePassword(int length) 
{
    String otp = "";
    for (int i = 0; i < length; i++) {
      otp += String(random(10));
    }
    return otp;
}

void stringToUint8(const String& password, uint8_t senderData[]) 
{
    int length = password.length();
    for (int i = 0; i < length; ++i) {
        senderData[i] = static_cast<uint8_t>(password[i]);
    }
    senderData[length] = static_cast<uint8_t>('\0');
}

void checkBluetoothSerial()
{
  if (SerialBT.available()) {
    String received = SerialBT.readString();
    Serial.print("Received message: ");
    Serial.println(received);
    if (received.indexOf("|") != -1) {
      newPassword = true;
      std::vector<String> substrings; // Vector to store 5 substrings

      // Find the positions of '|' in the received string
      size_t pos = 0;
      size_t found;

      for (int i = 0; i < 4; ++i) {
          found = received.indexOf('|', pos);
          substrings.push_back(received.substring(pos, found));
          pos = found + 1;
      }

      // Last substring goes from the last '|' to the end of the string
      substrings.push_back(received.substring(pos));

      preferences.putString(wifiSSIDKey, substrings[0]);
      preferences.putString(wifiPasswordKey, substrings[1]);
      preferences.putInt(passwordLengthKey, substrings[2].toInt());
      preferences.putLong(otpDurationKey, substrings[3].toInt());
      preferences.putString(defaultPasswordKey, substrings[4]);

      Serial.print("Wifi SSID: ");
      Serial.println(substrings[0]);
      Serial.print("Wifi password: ");
      Serial.println(substrings[1]);
      Serial.print("OTP length: ");
      Serial.println(substrings[2]);
      Serial.print("OTP duration: ");
      Serial.println(substrings[3]);
      Serial.print("Default password: ");
      Serial.print(substrings[4]);
    }
  }
}

void printPassword()
{
    display.clearDisplay();

    // Calculate the starting position for "Password" message
    int textWidth = strlen("OTP") * 8 * 2; // 8 pixels per character, multiplied by text size
    int startX = (SCREEN_WIDTH - textWidth) / 2;
    display.setCursor(startX, 0);
    display.print("OTP:");

    // Calculate the starting position for squares
    int otpLength = currentOtp.length();
    int startXOTP = (SCREEN_WIDTH - (otpLength * 16)) / 2;

    // Display the OTP numbers inside squares
    for (int i = 0; i < otpLength; i++) {
      display.drawRect(startXOTP + i * 16, 30, 14, 22, WHITE); // Draw square
      display.setCursor(startXOTP + i * 16 + 3, 35); // Adjust the position for numbers
      display.print(currentOtp.charAt(i)); // Print number inside the square
    }

    // Display the changes
    display.display();
}

/**********************************************************************************************************************
 * Setup and Loop
 *********************************************************************************************************************/
void setup() 
{
  Serial.begin(115200);
  delay(100); // Delay to allow Serial communication to stabilize

  WiFi.mode(WIFI_STA);

    // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  preferences.begin("saved-data", false);

  /*if (!SPIFFS.begin(true)) {
    Serial.println("Error mounting SPIFFS");
    return;
  }*/

  SerialBT.begin("ESP32");
  delay(1000);
}

void loop() 
{
  checkBluetoothSerial();

  otpDuration = preferences.getInt(otpDurationKey, 0);
  time_t currentTime = now();
  int diffMinutes = (currentTime - otpSendDate) / 60;

  Serial.println(currentTime);
  passwordLength = preferences.getInt(passwordLengthKey, 0);
  Serial.println(passwordLength);
  if (passwordLength) {
    if ((currentOtp == "" )|| (diffMinutes >= otpDuration) || newPassword) {
      currentOtp = generatePassword(passwordLength);
      defaultPassword = preferences.getString(defaultPasswordKey, "");

      String toSend = currentOtp + "|" + String(otpDuration) + "|" + defaultPassword;
      uint8_t senderData[toSend.length() + 1];
      stringToUint8(toSend, senderData);

      time_t startTime = now();
      while ((esp_now_send(receiverMacAddress, senderData, sizeof(senderData)) != ESP_OK) && (currentTime < startTime + 5)) {
        currentTime = now();
        delay(100);
      }

      startTime = now();
      while (!ack && (currentTime < startTime + 5)) {
        currentTime = now();
        delay(100);
      }

      if (!ack) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Disconnected");
        display.display();
      } else {
        printPassword();
        otpSendDate = currentTime;
        ack = false;
        newPassword = false;
      }
    }
  }

  delay(1000);
}