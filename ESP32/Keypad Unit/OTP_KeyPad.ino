/**********************************************************************************************************************
 *  Libraries
 *********************************************************************************************************************/
#include "Adafruit_Keypad.h"
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Time.h>
#include <TimeLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/**********************************************************************************************************************
 *  Constants
 *********************************************************************************************************************/
static const char *defaultPasswordKey = "default_pass";
static const char *numberOfFailsInRowKey = "fails_in_row";

/**********************************************************************************************************************
 * Global Variables
 *********************************************************************************************************************/
Preferences preferences;
bool ended = false;
bool first = false;
time_t otpReceiveDate = 0;
int otpDuration = 0;
String currentOtp = "";
String defaultPassword = "";
int maxNumberOfFailedAttempts = 3;
int passwordTypeTime = 20;
int lockTime = 30;

/**********************************************************************************************************************
 * Keypad Initialization
 *********************************************************************************************************************/
const byte ROWS = 4; // rows
const byte COLS = 4; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 18, 19, 23}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/**********************************************************************************************************************
 * Oled Initialization
 *********************************************************************************************************************/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/**********************************************************************************************************************
 * Esp-Now Initialization
 *********************************************************************************************************************/
uint8_t receiverMacAddress[] = {0xa8, 0x42, 0xe3, 0x45, 0x94, 0x68};
esp_now_peer_info_t peerInfo;

// Callback function to handle received data
void OnDataRecv(const uint8_t *macAddr, const uint8_t *data, int len) 
{
  String receivedMessage = String((const char*)data);
  Serial.print("Received message: ");
  Serial.println(receivedMessage);

  uint8_t senderData[] = "Ack";
  esp_now_send(receiverMacAddress, senderData, sizeof(senderData));
  if (receivedMessage != "Test") {
    otpReceiveDate = now();
    Serial.print( "Receive date: ");
    String delimiter = "|"; 
    size_t pos = 0;
    // Extract the first substring
    pos = receivedMessage.indexOf(delimiter);
    currentOtp = receivedMessage.substring(0, pos);
    receivedMessage.remove(0, pos + delimiter.length());
    // Extract the second substring
    pos = receivedMessage.indexOf(delimiter);
    otpDuration = (receivedMessage.substring(0, pos)).toInt();
    receivedMessage.remove(0, pos + delimiter.length());
    // The remaining part of the string is the third substring
    preferences.putString(defaultPasswordKey, defaultPassword);
    defaultPassword = receivedMessage;

    Serial.print("New password: ");
    Serial.println(currentOtp);
    Serial.print("Password duration: ");
    Serial.print(otpDuration);
    Serial.print("Default password: ");
    Serial.print(defaultPassword);
  }
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

  customKeypad.begin();

  preferences.begin("saved-data", false);

  delay(100);
}

void loop() 
{
  time_t currentTime = now();
  int diffMinutes = (currentTime - otpReceiveDate) / 60;
  diffMinutes = max(diffMinutes, 0);

  // Handle reboot case
  if (currentOtp == "") {
    Serial.print("Requesting a new password: ");
    uint8_t senderData[] = "New Password";
    esp_now_send(receiverMacAddress, senderData, sizeof(senderData));
  }

  if ((currentOtp == "")  || (diffMinutes >= otpDuration)) {
    currentOtp = preferences.getString(defaultPasswordKey, "");
  }

  Serial.print("Current password: ");
  Serial.println(currentOtp);
  Serial.print("Password length: ");
  Serial.println(currentOtp.length());
  Serial.print("Password duration: ");
  Serial.print(otpDuration);

  String receivedPassword = "";   
  if (currentOtp != "") {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Enter the \npassword:");
    display.display();
    currentTime = now();
    time_t startTime = now();
    String stars = "";   
    while (!ended && 
           (receivedPassword.length() < 7) && 
           (currentTime < (startTime + passwordTypeTime))) {
      customKeypad.tick();
      if (customKeypad.available()) {
        keypadEvent e = customKeypad.read();
        if (e.bit.EVENT == KEY_JUST_PRESSED) {
          char key = (char)e.bit.KEY;
          Serial.println(key);
          if (key == '#') {
            ended = true;
            Serial.println("\nEnd input sequence");
          } else if (key == 'D') {
            if (receivedPassword.length() != 0) {
              receivedPassword = receivedPassword.substring(0, receivedPassword.length()-1);
              stars = stars.substring(0, stars.length()-1);;
            }
          } else {
            if (!first) {
              first = true;
              startTime = now();
            }
            receivedPassword += key;
            stars += "*";
          }
          display.clearDisplay();
          int16_t x = ((SCREEN_WIDTH - (6 * stars.length())) / 2) - 10;
          int16_t y = (SCREEN_HEIGHT - 8) / 2;
          display.setCursor(x, y);
          display.println(stars);
          display.display();
        }
      }
      delay(100);
      currentTime = now();
    }

    if (receivedPassword.length() != 0 && ended) {
        String df = preferences.getString(defaultPasswordKey, "");
        Serial.println(df);
        if ((receivedPassword == currentOtp) || (receivedPassword == df)) {
          Serial.println("Access");
          preferences.putInt(numberOfFailsInRowKey, 0);
          uint8_t senderData[] = "Access";
          esp_now_send(receiverMacAddress, senderData, sizeof(senderData));
          display.clearDisplay();
          int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
          x = 25;
          int16_t y = (SCREEN_HEIGHT - 8) / 2;
          display.setCursor(x, y);
          display.println("Access");
          display.display();
          delay(1000);
      } else {
        Serial.println("Deny");
        uint8_t senderData[] = "Deny";
        esp_now_send(receiverMacAddress, senderData, sizeof(senderData));
        display.clearDisplay();
        int16_t x = ((SCREEN_WIDTH - 6 * 6) / 2) - 1;
        int16_t y = (SCREEN_HEIGHT - 8) / 2;
        display.setCursor(x, y);
        display.println("Deny");
        display.display();
        delay(1000);
        int numberOfFailsInRow = preferences.getInt(numberOfFailsInRowKey, 0);
        numberOfFailsInRow++;
        if (numberOfFailsInRow >= maxNumberOfFailedAttempts) {
          uint8_t senderData[] = "Potential Attack";
          esp_now_send(receiverMacAddress, senderData, sizeof(senderData));
          preferences.putInt(numberOfFailsInRowKey, 0);
          display.clearDisplay();
          display.setCursor(23, y);
          display.println("Locked:");
          display.display();
          delay(1000);
          for (int i = lockTime; i > 0; i--) {
            display.clearDisplay();
            display.setCursor(54, y);
            display.println(i);
            display.display();
            delay(1000);
          }
        } else {
          preferences.putInt(numberOfFailsInRowKey, numberOfFailsInRow);
        }
      }
    }
  } else {
    display.clearDisplay();
    int16_t x = (SCREEN_WIDTH - 6 * 6) / 2;
    int16_t y = (SCREEN_HEIGHT - 8) / 2;
    display.setCursor(x, y);
    display.println("Error");
    display.display();
  }

  ended = false;
  first = false;
  delay(100);
}