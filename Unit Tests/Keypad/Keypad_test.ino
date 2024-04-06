#include "Adafruit_Keypad.h"
#include "vector"

const byte ROWS = 4; // rows
const byte COLS = 4; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 25, 16, 26}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 18, 19, 21}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
std::vector<char> courseNumberVector = {'2', '3', '6', '3', '3', '3'};
std::vector<char> inputNumberVector;
int digitCount = 6;

void setup() {
  Serial.begin(9600);
  customKeypad.begin();
  Serial.print("Enter the course number\n");
}

void loop() {
  customKeypad.tick();

  while(customKeypad.available()){
    keypadEvent e = customKeypad.read();
    if(e.bit.EVENT == KEY_JUST_PRESSED) {
      Serial.print((char)e.bit.KEY);
      inputNumberVector.push_back((char)e.bit.KEY);
      if (inputNumberVector.size() == digitCount) {
        if (inputNumberVector == courseNumberVector) {
          Serial.print(" Correct number\n");
        } else {
          Serial.print(" Wrong number\n");
        }
        inputNumberVector.clear();
      }
    }
  }

  delay(10);
}