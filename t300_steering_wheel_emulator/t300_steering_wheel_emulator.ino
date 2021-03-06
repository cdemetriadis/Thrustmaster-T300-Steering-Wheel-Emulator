//
// Thrustmaster T300 Steering Wheel Emulator
//
// Arduino Nano based emulator for an advanced Console Sim-Racing
// experience on a Thrustmaster T300 base.
// 
// This build is intended for use with a Thrustmaster T300 Base
// (direct plugin), Playstation 4 Pro and Assetto Corsa Competizione. But,
// having said that, it should work with any compatible Thrustmaster Base
// and Console, since it's emulating the F1 Steering Wheel (Advanced Mode 
// for PC not supported yet).
//
// Find out more about this build, instructions and parts list here:
// https://github.com/cdemetriadis/Thrustmaster-T300-Steering-Wheel-Emulator
//
// GNU General Public License v3.0 or later
// Constantinos Demetriadis (https://github.com/cdemetriadis/)
//

#include      <EEPROM.h>                              // Load EEPROM library for storing settings
#include      <LiquidCrystal_I2C.h>                   // Load the Liquid Crystal Display library
#include      <i2cEncoderMiniLib.h>                   // Load I2C encoderBB Library
#include      <TimeLib.h>                             // Load Time Library
#include      <DS1307RTC.h>                           // Load the DS1307 RTC Library

//
// Debugging & Setup 
#define       DEBUG_SETUP false                       // Debug Setup information
#define       DEBUG_KEYS false                        // Debug the button presses
#define       DEBUG_WHEEL false                       // Debug wheel output
#define       DEBUG_ROTARY_SWITCHES false             // Debug Rotary Switches: Display the values returned by the Rotary Switches
#define       DEBUG_LATENCY false                     // Debug response
#define       Rotary_Switch_T300 true                 // Select the values for the Rotary Switches. 'true:T300', 'false:USB'
#define       MESSAGE_DURATION 750                    // Duration of the messages on the screen
#define       DEBOUNCE 90                             // Set this to the lowest value that gives the best result

//
// Display Strings
const String  VERSION = "v";
const String  VERSION_NUMBER = "0.5.0 beta";

const String  ON_ICON = "~";
const String  MENU = "~Ready";
const String  LOADING = "Loading...";
const String  SELECT_OPTION = "Select Option";
const String  DISPLAY_MODE_MENU = "Disp. Mode";
const String  DISPLAY_MODE_PS = ":PS";
const String  DISPLAY_MODE_GT = ":GT";
const String  WHEEL_MODE_MENU = "Wheel Mode";
const String  WHEEL_MODE_PS = ":PS";
const String  WHEEL_MODE_PC = ":PC";
const String  DISPLAY_KEYPRESS_MENU = "Keypress";
const String  DISPLAY_STATUS_MENU = "Display Off";
const String  BUZZER_STATUS_MENU = "Buzzer";
const String  DISPLAY_CLOCK_MENU = "Date/Time";
const String  HOUR_CHIRP_MENU = "Hour Chirp";
const String  DISPLAY_RUNTIME_MENU = "Runtime";
const String  ON = ":On";
const String  OFF = ":Off";

//
// Setup EEPROM for saving various states
bool          DISPLAY_MODE = EEPROM.read(0);          // Retrieve the display output mode. 'true:1' for Playstation, 'false:0' for Wheel
bool          DISPLAY_KEYS = EEPROM.read(1);          // Retrieve the display keypress option. 'true:1' on, 'false:0' off
bool          BUZZER_STATUS = EEPROM.read(2);         // Retrieve the buzzer status. 'true:1' on, 'false:0' off
bool          DISPLAY_STATUS = EEPROM.read(3);        // Retrieve the display status. 'true:1' on, 'false:0' off
bool          HOUR_CHIRP = EEPROM.read(4);            // Retrieve the hour chirp status. 'true:1' on, 'false:0' off
bool          CLOCK_STATUS = EEPROM.read(5);          // Retrieve the clock status. 'true:1' on, 'false:0' off
bool          WHEEL_MODE = EEPROM.read(6);            // Retrieve the wheel mode. 'true:1' for Playstation, 'false:0' for PC

unsigned long startLoop;

//
// Initialize Encoders
i2cEncoderMiniLib encoderBB(0x20);
i2cEncoderMiniLib encoderABS(0x21);
i2cEncoderMiniLib encoderTC(0x22);
const int     ENC_INTERRUPT_PIN = 3;
int           encoderIncCount = 0;
int           encoderDecCount = 0;

//
// Setup LCD & vars
LiquidCrystal_I2C lcd(0x27, 16, 2);                   // Connects to 3.3V, GND, A4 (SDA), A5 (SCL)
int           curValue = -100;
void          printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0);
unsigned long lastDisplayTime;                        // Used for delaying the screen message
String        line_1;
String        line_2;
String        prev_line_1;
String        prev_line_2;
int           lcdDebug = 0;

//
// Setup Real Time Clock
tmElements_t  tm;
String        getTime;
String        getDate;
bool          chirp_played = true;

//
// Setup Buzzer 
const int     BUZZER_PIN = 4;
unsigned long buzzerTime;

//
// Menu Navigation Setup
int           menu = 0;
int           menuPage = 1;
int           maxPages = 1;

//
// CAB Setup - Combined action buttons
int           triggerCAB;
int           triggerSteps = 0;
int           triggerStepsIncrease = 0;
int           triggerStepsDecrease = 0;
int           CAB_ACTION;
int           CAB_STEPS;
unsigned long cabTrigger;

//
// Button Matrix
const int     MATRIX_INTERRUPT_PIN = 2;
int           foundColumn = 0;
int           buttonValue = 0;
byte          wheelState[8];
volatile byte pos;

//
// Setup Button Matrix
int           rowPin[] = {5, 6, 7, 8, 9};                   // Set pins for rows > OUTPUT
int           colPin[] = {A0, A1, A2, A3, 11};              // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int           rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int           colSize = sizeof(colPin)/sizeof(colPin[0]);


void setup() {
  
  resetWheelState();
  
  #if DEBUG_SETUP || DEBUG_KEYS || DEBUG_WHEEL || DEBUG_LATENCY || DEBUG_ROTARY_SWITCHES
     Serial.begin(115200);    // Arduino debug console
  #endif

  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer pin as OUTPUT
  
  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(MATRIX_INTERRUPT_PIN), ss_rising, RISING); // Interrupt for Button Matrix
  
  // Setup Encoders
  pinMode(ENC_INTERRUPT_PIN, INPUT);
  encoderBB.begin(i2cEncoderMiniLib::IPUP_ENABLE);
  encoderBB.onIncrement = encoderBB_increment;
  encoderBB.onDecrement = encoderBB_decrement;
  encoderABS.begin(i2cEncoderMiniLib::IPUP_ENABLE);
  encoderABS.onIncrement = encoderABS_increment;
  encoderABS.onDecrement = encoderABS_decrement;
  encoderABS.onButtonLongPush = turnDisplayOff;
  encoderABS.writeDoublePushPeriod(50);
  encoderTC.begin(i2cEncoderMiniLib::IPUP_ENABLE);
  encoderTC.onIncrement = encoderTC_increment;
  encoderTC.onDecrement = encoderTC_decrement;
  encoderBB.autoconfigInterrupt();
  encoderABS.autoconfigInterrupt();
  encoderTC.autoconfigInterrupt();

  
  // Setup Display & Init message
  lcd.init();
  
  if (DISPLAY_STATUS) {
    lcd.backlight();
    lcd.clear();
    printDisplay(LOADING);
  } else {
    lcd.noDisplay();
    lcd.noBacklight();
    lcd.clear();
  }


  #if DEBUG_SETUP
    Serial.println("Steering Wheel Emulator");
    Serial.println();
    Serial.print("Setup ");
    Serial.print(rowSize);
    Serial.print("x");
    Serial.print(colSize);
    Serial.println(" Button Matrix");
  #endif


  //
  // Row setup
  // Set rows as OUTPUT
  for (int i=0; i<rowSize; i++) {
    pinMode(rowPin[i], OUTPUT);
    #if DEBUG_SETUP
      Serial.print("Pin: ");
      Serial.print(rowPin[i]);
      Serial.print(" is set as OUTPUT on row: ");
      Serial.println(i);
    #endif
    digitalWrite(rowPin[i], HIGH);
  }

  //
  // Column setup
  // Set columns as INPUT_PULLUP
  for (int i=0; i<colSize; i++) {
    pinMode(colPin[i], INPUT_PULLUP);
    #if DEBUG_SETUP
      Serial.print("Pin: ");
      Serial.print(colPin[i]);
      Serial.print(" is set as INPUT_PULLUP on col: ");
      Serial.println(i);
    #endif
  }
  
  if (BUZZER_STATUS) {
    buzzerStartup();
  }

}


// Interrupt0 (external, pin 2) - prepare to start the transfer
void ss_rising () {
  SPDR = wheelState[0]; // load first byte into SPI data register
  pos = 1;
}


// SPI interrupt routine
ISR (SPI_STC_vect) {
  SPDR = wheelState[pos++]; // load the next byte to SPI output register and return.
}


String CABActionMap[] = {
  "BB",
  "ABS",
  "TC"
};

int CABActionGuide[3][2][2] = {
  { // BB
    {2, B10111111},   // D-Pad Down
    {2, B11110111}    // D-Pad Up
  },
  { // ABS
    {1, B11111101},  // L3
    {1, B11111110}   // R3
  },
  { // TC
    {2, B11101111},   // D-Pad Left
    {2, B11011111}    // D-Pad Right
  }
};

int CABActionGuidePC[3][2][2] = {
  { // BB
    {2, B10111111},   // D-Pad Down
    {2, B11110111}    // D-Pad Up
  },
  { // ABS
    {2, B01111111},  // L3
    {2, B11111011}   // R3
  },
  { // TC
    {2, B11101111},   // D-Pad Left
    {2, B11011111}    // D-Pad Right
  }
};

void loop() {
  
  #if DEBUG_LATENCY
    startLoop = micros();
  #endif

  #if DEBUG_ROTARY_SWITCHES
    #define MODE_LABEL "CAB Mode: "
    #define STEPS_LABEL "CAB Steps: "
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(MODE_LABEL + String(analogRead(6)));
    lcd.setCursor(0, 1);
    lcd.print(MODE_LABEL + String(analogRead(7)));
    delay(MESSAGE_DURATION);
  #endif
  
  #if !DEBUG_ROTARY_SWITCHES // Ignore if debugging Rotary Switches
    // 
    // Read Rotary Encoders
    if (digitalRead(ENC_INTERRUPT_PIN) == LOW) {
      encoderBB.updateStatus();
      encoderABS.updateStatus();
      encoderTC.updateStatus();
    }
    
    //
    // Initiate Button Matrix
    // This functions returns a buttonValue for each button
    scanButtonMatrix();
  #endif

  //
  // Set the action based on the buttonValue
  switch (buttonValue) {

    case 204:
      if (WHEEL_MODE) { // PS Triangle
        wheelState[0] = wheelState[0] & B11111101;
      } else { // PC 3
        wheelState[0] = wheelState[0] & B11111101;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Triangle", 4) : printDisplay("Pit Limiter", 2);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224:
      if (WHEEL_MODE) { // PS Square
        wheelState[0] = wheelState[0] & B11111110;
      } else { // PC 4
        wheelState[0] = wheelState[0] & B11111110;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Square", 5) : printDisplay("MFD", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // Menu (Display)
      buzzer();
      if (DISPLAY_STATUS) {
        if (menu == 1) {
          menuPage = 1;
          menu = 0;
        } else {
          menuPage = 1;
          menu = 1;
        }
        delay(DEBOUNCE*2);
        showMenu();
      } else {
        lcd.display();
        lcd.backlight();
        DISPLAY_STATUS = 1;
        EEPROM.write(3, DISPLAY_STATUS);
      }
      
      #if DEBUG_KEYS
        Serial.print("Done ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 267: // Next (Display)
      buzzer();
      lcd.display();
      lcd.backlight();
      DISPLAY_STATUS = 1;
      EEPROM.write(3, DISPLAY_STATUS);
      displayNext();
      #if DEBUG_KEYS
        Serial.print("Next ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 290: // Select (Display)
      buzzer();
      lcd.display();
      lcd.backlight();
      DISPLAY_STATUS = 1;
      EEPROM.write(3, DISPLAY_STATUS);
      displaySelect();
      #if DEBUG_KEYS
        Serial.print("Select("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;




    case 225:
      if (WHEEL_MODE) { // PS Circle
        wheelState[0] = wheelState[0] & B01111111;
      } else { // PC 5
        wheelState[0] = wheelState[0] & B01111111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Circle", 5) : printDisplay("Flash", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 246:
      if (WHEEL_MODE) { // PS Cross
        wheelState[1] = wheelState[1] & B10111111;
      } else { // PC 6
        wheelState[1] = wheelState[1] & B10111111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("HUD", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 268:
      if (WHEEL_MODE) { // PS Options
        wheelState[1] = wheelState[1] & B11101111;
      } else { // PC 8
        wheelState[1] = wheelState[1] & B11101111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Options", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 291: 
      if (WHEEL_MODE) { // PS Playstation
        wheelState[1] = wheelState[1] & B01111111;
      } else { // PC 13
        wheelState[1] = wheelState[1] & B01111111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("PlayStation", 2);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("PlayStation ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 315:
      if (WHEEL_MODE) { // PS Share
        wheelState[1] = wheelState[1] & B11011111;
      } else { // PC 7
        wheelState[1] = wheelState[1] & B11011111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Share", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;




    case 247:
      if (WHEEL_MODE) { // PS D-Pad Up
        wheelState[2] = wheelState[2] & B11110111;
      } else { // PC Up
        wheelState[2] = wheelState[2] & B11110111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Up", 7);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 269:
      if (WHEEL_MODE) { // PS D-Pad Down
        wheelState[2] = wheelState[2] & B10111111;
      } else { // PC Down
        wheelState[2] = wheelState[2] & B10111111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Down", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 292:
      if (WHEEL_MODE) { // PS D-Pad Left
        wheelState[2] = wheelState[2] & B11101111;
      } else { // PC Left
        wheelState[2] = wheelState[2] & B11101111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Left", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 316:
      if (WHEEL_MODE) { // PS D-Pad Right
        wheelState[2] = wheelState[2] & B11011111;
      } else { // PC Right
        wheelState[2] = wheelState[2] & B11011111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay("Right", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 270:
      if (WHEEL_MODE) { // PS L2
        wheelState[1] = wheelState[1] & B11111011;
      } else { // PC 10
        wheelState[1] = wheelState[1] & B11111011;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L2", 7) : printDisplay("Wipers", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 293:
      if (WHEEL_MODE) { // PS R2
        wheelState[1] = wheelState[1] & B11110111;
      } else { // PC 9
        wheelState[1] = wheelState[1] & B11110111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R2", 7) : printDisplay("Lights", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 317: // 290 CAB-L Combined Action Button Left
      buzzer();

      triggerCAB = getCABMode();
      triggerStepsDecrease = getCABSteps();

      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay(CABActionMap[triggerCAB]+" -"+triggerStepsDecrease, 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("CAB- ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 342: // 315 CAB-R + Combined Action Button Right
      buzzer();
      
      triggerCAB = getCABMode();
      triggerStepsIncrease = getCABSteps();

      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        printDisplay(CABActionMap[triggerCAB]+" +"+triggerStepsIncrease, 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("CAB+ ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



      
    case 147:
      if (WHEEL_MODE) { // PS L1
        wheelState[0] = wheelState[0] & B11110111;
      } else { // PC 2/23
        wheelState[0] = wheelState[0] & B11110111;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L1", 7) : printDisplay("Shift Down", 3);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 164:
      if (WHEEL_MODE) { // PS R1
        wheelState[0] = wheelState[0] & B11111011;
      } else { // PC 1/22
        wheelState[0] = wheelState[0] & B11111011;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R1", 7) : printDisplay("Shift Up", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
      
    case 182:
      if (WHEEL_MODE) { // PS L3
        wheelState[1] = wheelState[1] & B11111101;
      } else { // PC 11
        wheelState[1] = wheelState[1] & B11111101;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L3", 7) : printDisplay("ABS-", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 201:
      if (WHEEL_MODE) { // PS R3
        wheelState[1] = wheelState[1] & B11111110;
      } else { // PC 12
        wheelState[1] = wheelState[1] & B11111110;
      }
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R3", 7) : printDisplay("ABS+", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("R3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    //
    // Encoder Functions
    case 1000:
      if (WHEEL_MODE) { // PS D-Pad Up
        wheelState[2] = wheelState[2] & B11110111;
      } else { // PC 17 CHRG+
        // wheelState[3] = wheelState[3] & B11110111;
        wheelState[2] = wheelState[2] & B11110111;
      }
      encoderIncCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Up x"+String(encoderIncCount), 5) : printDisplay("BB +"+String(encoderIncCount), 5); 
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 1100:
      if (WHEEL_MODE) { // PS D-Pad Down
        wheelState[2] = wheelState[2] & B10111111;
      } else { // PC 16 CHRG-
        // wheelState[3] = wheelState[3] & B10111111;
        wheelState[2] = wheelState[2] & B10111111;
      }
      encoderDecCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Down x"+String(encoderDecCount), 4) : printDisplay("BB -"+String(encoderDecCount), 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 2000:
      if (WHEEL_MODE) { // PS D-Pad Left
        wheelState[2] = wheelState[2] & B11101111;
      } else { // PC 14 DIF IN+
        // wheelState[3] = wheelState[3] & B11011111;
        wheelState[2] = wheelState[2] & B11101111;
      }
      encoderDecCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Left x"+String(encoderDecCount), 4) : printDisplay("TC -"+String(encoderDecCount), 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 2100: // D-Pad Right
      if (WHEEL_MODE) { // PS D-Pad Right
        wheelState[2] = wheelState[2] & B11011111;
      } else { // PC 15 DIF IN-
        //wheelState[3] = wheelState[3] & B11101111;
        wheelState[2] = wheelState[2] & B11011111;
      }
      encoderIncCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Right x"+String(encoderIncCount), 4) : printDisplay("TC +"+String(encoderIncCount), 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 3000: // L3
      if (WHEEL_MODE) { // PS L3
        wheelState[1] = wheelState[1] & B11111101;
      } else { // PC 21
        wheelState[2] = wheelState[2] & B01111111;
      }
      encoderDecCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L3 x"+String(encoderDecCount), 5) : printDisplay("ABS -"+String(encoderDecCount), 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 3100:
      if (WHEEL_MODE) { // PS R3
        wheelState[1] = wheelState[1] & B11111110;
      } else { // PC 20
        wheelState[2] = wheelState[2] & B11111011;
      }
      encoderIncCount++;
      buzzer();
      if (DISPLAY_STATUS && DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R3 x"+String(encoderIncCount), 5) : printDisplay("ABS +"+String(encoderIncCount), 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("R3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    default: // Reset if nothing is pressed
      break;
  }

  // CAB Trigger 
  if (triggerStepsIncrease >= 1 && ((millis()-cabTrigger) > DEBOUNCE*2)) {
    if (WHEEL_MODE) {
      wheelState[CABActionGuide[triggerCAB][1][0]] = wheelState[CABActionGuide[triggerCAB][1][0]] & CABActionGuide[triggerCAB][1][1];
    } else {
      wheelState[CABActionGuidePC[triggerCAB][1][0]] = wheelState[CABActionGuidePC[triggerCAB][1][0]] & CABActionGuidePC[triggerCAB][1][1];
    }
    triggerStepsIncrease--;
    cabTrigger = millis();
  }
  if (triggerStepsDecrease >= 1 && ((millis()-cabTrigger) > DEBOUNCE*2)) {
    if (WHEEL_MODE) {
      wheelState[CABActionGuide[triggerCAB][0][0]] = wheelState[CABActionGuide[triggerCAB][0][0]] & CABActionGuide[triggerCAB][0][1];
    } else {
      wheelState[CABActionGuidePC[triggerCAB][0][0]] = wheelState[CABActionGuidePC[triggerCAB][0][0]] & CABActionGuidePC[triggerCAB][0][1];
    }
    triggerStepsDecrease--;
    cabTrigger = millis();
  }
  
  #if DEBUG_WHEEL
    for (int i = 0; i < 8; i++) {
      Serial.print(wheelState[i], BIN);
      Serial.print(" ");
    }
    Serial.println();
  #endif

  #if !DEBUG_ROTARY_SWITCHES // Ignore if debugging Rotary Switches
  // Reset Display if nothing is pressed for the MESSAGE_DURATION
  if (menu == 0 && DISPLAY_STATUS && (millis()-lastDisplayTime) > MESSAGE_DURATION) {
    showMenu();
    curValue = buttonValue;
    encoderIncCount = 0;
    encoderDecCount = 0;
  }
  #endif 
  
  if (HOUR_CHIRP) {
    if (tm.Minute == 0 && chirp_played == false) {
      buzzerHour();
      chirp_played = true;
    } else if (tm.Minute != 0 && chirp_played == true) {
      chirp_played = false;
    }
  }

  #if DEBUG_LATENCY
    Serial.println(micros()-startLoop);
  #endif

  
  // Set a debounce delay
  delay(DEBOUNCE);
  
  resetWheelState();

}
