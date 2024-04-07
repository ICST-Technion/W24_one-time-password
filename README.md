## One Time Password Door Entry Project by : Pierre Khamis, Mahmoud Diab & Tasneem Nassar.
In this project, we've developed a one-time door entry password system aimed at enhancing security and convenience for users. Our system provide a smartphone application that allows admin to customize various aspects of their door entry password. Admin can set parameters such as password length, frequency of password changes, and establish master passwords for added security. The system comprises two integral units: the keypad unit, responsible for securely collecting user-generated passwords and granting or denying access based on authentication, and the password generation unit, which autonomously generates unique passwords and seamlessly transmits them to the keypad unit for door entry. Additionally, our system boasts intuitive interfaces and real-time notifications, ensuring users and the admin are always in control and aware of their door access status.

## Our Project in details :
The application enables the admin to configure password settings, including length, frequency of changes, and a master password for anytime use. Additionally, it offers the admin visibility into the current password and provides statistics on daily and hourly entries. To access these features, users must activate Bluetooth on their device and press the Bluetooth icon within the app to establish a connection.
The OLED unit generates passwords based on these settings, displaying them on the screen and transmitting them to the keypad unit when a new password is required.
The keypad unit receives user-entered passwords and grants access upon authentication. It also communicates with the other unit to update statistics. If there are three consecutive failed attempts, the unit locks the door for 30 seconds before allowing another try.
This project involves ESP Now communications and Bluetooth communications.

## Folder description :
* ESP32: source code for the esp side (firmware).
* Documentation: wiring diagram + system diagram +  basic operating instructions
* Unit Tests: tests for individual hardware components (input / output devices)
* flutter_app : dart code for our Flutter app.
* Parameters: contains description of configurable parameters 

## Arduino/ESP32 libraries used in this project:
* Adafruit Keypad - version 1.3.2
* Time - version 1.6.1
* Adafruit GFX Library - version 1.11.9
* Adafruit BusIO - version 1.15.1
* Adafruit SSD1306 - version 2.5.9
* ArduinoJson - version 7.0.4

## Hardware used in this project:
* Esp32 CH9102 - number 2
* 1.3 inch MONOCHROME OLED display 128*64 with alternate address - number 2
* 4X4 matrix keypad - number 1

## Circuit diagrams:
### Oled unit circuit (Admin):
![Oled unit circuit (Admin)](https://github.com/mahmouddiab2000/one-time-password/blob/main/Documentation/Connection%20diagram/Oled%20unit%20circuit%20(Admin).png)

### Keypad unit circuit:
![Keypad Unit Circuit](https://github.com/mahmouddiab2000/one-time-password/blob/main/Documentation/Connection%20diagram/Keypad%20unit%20circuit.png)

## Project Poster:
TBD

This project is part of ICST - The Interdisciplinary Center for Smart Technologies, Taub Faculty of Computer Science, Technion
https://icst.cs.technion.ac.il/