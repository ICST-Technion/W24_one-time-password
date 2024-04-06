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
#include <ArduinoJson.h>
#include <stdio.h>

/**********************************************************************************************************************
 *  Constants
 *********************************************************************************************************************/
static const char *otpDurationKey = "otp_duration";
static const char *defaultPasswordKey = "default_pass";
static const char *passwordLengthKey = "password_length";
const int NUM_DAYS = 7;
const int NUM_HOURS = 24;

/**********************************************************************************************************************
 * Global Variables
 *********************************************************************************************************************/
Preferences preferences;
bool newPassword = false;
BluetoothSerial SerialBT;
time_t otpSendDate = 0;
int otpDuration = 0;
String currentOtp = "";
String defaultPassword = "";
int passwordLength = 0;
int customerCounts[NUM_DAYS][NUM_HOURS] = {{0}};
volatile bool ack = false;
bool newSettings = false;
time_t lastCheckDate = 0;
bool isDisconnected = false;
time_t updatedTime = 0;

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
  String receivedMessage = String((const char *)data);
  Serial.print("Received message: ");
  Serial.println(receivedMessage);
  if (receivedMessage == "Access") {
    display.clearDisplay();
    int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
    x = 25;
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(x, y);
    display.println("Access");
    if (updatedTime) {
      updateArray(customerCounts, NUM_DAYS, NUM_HOURS, "/statistics.txt");
    }
    display.display();
    delay(1000);
    printPassword();
  } else if (receivedMessage == "Deny") {
    display.clearDisplay();
    int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(x, y);
    display.println("Deny");
    display.display();
    delay(1000);
    printPassword();
  } else if (receivedMessage == "New Password") {
    newPassword = true;
  } else if (receivedMessage == "Ack") {
    ack = true;
  } else {
    SerialBT.print("lock");
    display.clearDisplay();
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(25, y);
    display.println("Locked:");
    display.display();
    delay(1000);
    for (int i = 30; i > 0; i--)
    {
      display.clearDisplay();
      int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
      int16_t y = (SCREEN_HEIGHT - 8) / 2;
      display.setCursor(x, y);
      display.println(i);
      display.display();
      delay(1000);
    }
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

void stringToUint8(const String &password, uint8_t senderData[])
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
      newSettings = true;
      std::vector<String> substrings;

      // Find the positions of '|' in the received string
      size_t pos = 0;
      size_t found;

      for (int i = 0; i < 2; ++i) {
        found = received.indexOf('|', pos);
        substrings.push_back(received.substring(pos, found));
        pos = found + 1;
      }

      // Last substring goes from the last '|' to the end of the string
      substrings.push_back(received.substring(pos));

      preferences.putInt(passwordLengthKey, substrings[0].toInt());
      preferences.putLong(otpDurationKey, substrings[1].toInt());
      preferences.putString(defaultPasswordKey, substrings[2]);

      Serial.print("OTP length: ");
      Serial.println(substrings[0]);
      Serial.print("OTP duration: ");
      Serial.println(substrings[1]);
      Serial.print("Default password: ");
      Serial.print(substrings[2]);
    } else if (received == "statistics") {
      readArrayFromJSON(customerCounts, NUM_DAYS, NUM_HOURS, "/statistics.txt");
      StaticJsonDocument<200> doc;

      // Serialize the array
      for (int i = 0; i < NUM_DAYS; i++) {
        JsonArray row = doc.createNestedArray();
        for (int j = 0; j < NUM_HOURS; j++) {
          row.add(customerCounts[i][j]);
        }
      }

      // Convert JSON object to string
      String jsonString;
      serializeJson(doc, jsonString);
      jsonString = "{" + jsonString + "}";
      Serial.println(jsonString);
      SerialBT.print(jsonString);
    } else if (received == "password") {
      if (currentOtp) {
        SerialBT.print(currentOtp);
      }
    } else {
      int year, month, day, hour, minute, second;
      float fraction;
      sscanf(received.c_str(), "%d-%d-%dT%d:%d:%d.%f", &year, &month, &day, &hour, &minute, &second, &fraction);
      setTime(hour, minute, second, day, month, year);
      updatedTime = now();
    }
  }
}

void printPassword()
{
  display.clearDisplay();

  int textWidth = strlen("OTP") * 8 * 2; // 8 pixels per character, multiplied by text size
  int startX = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(startX, 1);
  display.print("OTP:");

  // Calculate the starting position for squares
  int otpLength = currentOtp.length();
  int startXOTP = (SCREEN_WIDTH - (otpLength * 16)) / 2;

  // Display the OTP numbers inside squares
  for (int i = 0; i < otpLength; i++) {
    display.drawRect(startXOTP + i * 16, 30, 14, 22, WHITE); // Draw square
    display.setCursor(startXOTP + i * 16 + 3, 35);           // Adjust the position for numbers
    display.print(currentOtp.charAt(i));                     // Print number inside the square
  }

  // Display the changes
  display.display();
}

void saveArrayToJSON(int array[][NUM_HOURS], int rows, int cols, const char *filename)
{
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  StaticJsonDocument<512> doc;
  JsonArray root = doc.to<JsonArray>();

  for (int i = 0; i < rows; i++) {
    JsonArray row = root.createNestedArray();
    for (int j = 0; j < cols; j++) {
      row.add(array[i][j]);
    }
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }

  file.close();
}

void readArrayFromJSON(int array[][NUM_HOURS], int rows, int cols, const char *filename)
{
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(error.c_str());
    return;
  }

  for (int i = 0; i < rows; i++) {
    JsonArray row = doc[i];
    for (int j = 0; j < cols; j++) {
      array[i][j] = row[j];
    }
  }

  file.close();
}

void updateArray(int array[][NUM_HOURS], int rows, int cols, const char *filename)
{
  readArrayFromJSON(array, rows, cols, filename);
  int currHour = ::hour();
  int currDay = ::weekday();
  array[currDay - 1][currHour] += 1;
  saveArrayToJSON(array, rows, cols, filename);
}

void resetArray(int array[][NUM_HOURS], int rows, int cols, const char *filename)
{
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      array[i][j] = 0;
    }
  }
  saveArrayToJSON(array, rows, cols, filename);
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
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Register for a callback function that will be called when data is received

  esp_now_register_recv_cb(OnDataRecv);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  preferences.begin("saved-data", false);

  if (!SPIFFS.begin(true)) {
    Serial.println("Error mounting SPIFFS");
    return;
  }

  SerialBT.begin("ESP32");
  delay(1000);
}

void loop()
{
  time_t currentTime = now();
  if (!updatedTime) {
    SerialBT.print("time");
  }

  int week = 60*60*24*7;
  if (currentTime > (updatedTime + week)) {
    resetArray(customerCounts, NUM_DAYS, NUM_HOURS, "/statistics.txt");
    updatedTime = now();
  }

  checkBluetoothSerial();
  otpDuration = preferences.getInt(otpDurationKey, 0);
  currentTime = now();
  int diffMinutes = (currentTime - otpSendDate) / 60;

  passwordLength = preferences.getInt(passwordLengthKey, 0);
  if (passwordLength) {
    if ((currentOtp == "") || (diffMinutes >= otpDuration) || newPassword || newSettings) {
      passwordLength = preferences.getInt(passwordLengthKey, 0);
      currentOtp = generatePassword(passwordLength);
      defaultPassword = preferences.getString(defaultPasswordKey, "");

      ack = false;
      String toSend = currentOtp + "|" + String(otpDuration) + "|" + defaultPassword;
      uint8_t senderData[toSend.length() + 1];
      stringToUint8(toSend, senderData);

      time_t startTime = now();
      while ((esp_now_send(receiverMacAddress, senderData, sizeof(senderData)) != ESP_OK) && (currentTime < startTime + 1)) {
        currentTime = now();
        delay(10);
      }

      startTime = now();
      while (!ack && (currentTime < startTime + 12)) {
        currentTime = now();
        delay(10);
      } 

      if (ack) {
        //Serial.print("BBB");
        printPassword();
        otpSendDate = currentTime;
        ack = false;
        if (newSettings) {
          SerialBT.print("ack");
        }
        newSettings = false;
        newPassword = false;
        /*if (isDisconnected) {
          SerialBT.print("reconnected");
        }*/
        isDisconnected = false;
        lastCheckDate = now();
      } else {
        //Serial.print("AAA");
        display.clearDisplay();
        display.setCursor(0, 10);
        display.print("Connecting\nTo Keypad");
        display.display();
        isDisconnected = true;
      }
    } else {
      time_t currentTime = now();
      if (currentTime > (lastCheckDate + 30)) {
        lastCheckDate = now();
        ack = false;
        uint8_t senderData[] = "Test";
        esp_now_send(receiverMacAddress, senderData, sizeof(senderData));

        time_t startTime = now();
        currentTime = now();
        while (!ack && (currentTime < startTime + 12)) {
          currentTime = now();
          delay(100);
        }

        if (ack) {
          ack = false;
          printPassword(); 
          /*if (isDisconnected) {
            SerialBT.print("reconnected");
          }*/
          isDisconnected = false;
        } else {
          SerialBT.print("disconnected");
          display.clearDisplay();
          display.setCursor(0, 10);
          display.print("Connecting\nToKeypad");
          display.display();
          isDisconnected = true;
        }
      }
    }
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Set \nSettings");
    display.display();
  }

  delay(1000);
}