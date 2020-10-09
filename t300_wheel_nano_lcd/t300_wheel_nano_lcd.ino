// Connect to Thrustmaster T300
//
// Arduino GND                        -> T300 Blue wire (2)
// Arduino pin 12                     -> T300 White wire (3) (data from wheel to base)
// Arduino pin 10 + pin 2 (SS)        -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
// Arduino pin 13 (SCK)               -> T300 Red wire (5)
// Arduino +5V                        -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#include      <EEPROM.h>                              // Load EEPROM library for storing settings
#include      <LiquidCrystal_I2C.h>                   // Load the Liquid Crystal Display library
#include      <i2cEncoderMiniLib.h>                   // Load I2C Encoder Library
#include      <TimeLib.h>                             // Load Time Library
#include      <DS1307RTC.h>                           // Load the DS1307 RTC Library
 
#define       DEBUG_SETUP false                       // Debug Setup information
#define       DEBUG_KEYS false                        // Debug the button presses
#define       DEBUG_WHEEL false                       // Debug wheel output
#define       MESSAGE_DURATION 750                    // Duration of the messages on the screen
#define       DEBOUNCE 50                             // Set this to the lowest value that gives the best result

#define       LOADING "Loading..."
#define       SELECT_OPTION " Select Option:"
#define       CAB_ON "~CAB"
#define       CAB_OFF " CAB"
#define       DISPLAY_MODE_ON "~Display Mode"
#define       DISPLAY_MODE_OFF " Display Mode"
#define       DISPLAY_KEYPRESS_ON "~Disp.Keypress"
#define       DISPLAY_KEYPRESS_OFF " Disp.Keypress"
#define       DISPLAY_STATUS_ON "~Display Off"
#define       DISPLAY_STATUS_OFF " Display Off"
#define       DISPLAY_RUNTIME_ON "~Display Runtime"
#define       ACTION_OFF " Action:     "
#define       ACTION_ON "~Action:     "
#define       STEPS_ON "~Steps:       "
#define       STEPS_OFF " Steps:       "
#define       KEYPRESS_ON "Keypress: On"
#define       KEYPRESS_OFF "Keypress: Off"
#define       MODE_PS "Mode: PS"
#define       MODE_WHEEL "Mode: Wheel"

// Setup EEPROM
bool          DISPLAY_MODE = EEPROM.read(0);          // Retrieve the display output mode. 'true:1' for Playstation, 'false:0' for Wheel
bool          DISPLAY_KEYS = EEPROM.read(1);          // Retrieve the display keypress option. 'true:1' on, 'false:0' off
bool          DISPLAY_STATUS = EEPROM.read(3);        // Retrieve the display status. 'true:1' on, 'false:0' off
int           CAB_ACTION = EEPROM.read(4);            // Retrieve the CAB action
int           CAB_STEPS = EEPROM.read(5);             // Retrieve the CAB Steps


// Initialize Encoders
const int     IntPin = 3;
i2cEncoderMiniLib encoderBB(0x20);
//i2cEncoderMiniLib encoderTC(0x20); <-- SETUP ADDRESS
//i2cEncoderMiniLib encoderABS(0x20); <-- SETUP ADDRESS


// Setup LCD & vars
LiquidCrystal_I2C lcd(0x27, 16, 2);                   // Connects to 3.3V, GND, A4 (SDA), A5 (SCL)
int           curValue = -100;
String        prevLine_1;
String        prevLine_2;
void          printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0);
unsigned long displayTime = millis();                 // Used for delaying the screen message


// Setup Real Time Clock
tmElements_t  tm;
String        getTime;
String        getDate;


// Menu Navigation Setup
int           menu = 0;
int           menuPage = 1;
int           maxPages = 1;


// CAB Setup - Combined action buttons
int           triggerCAB;
int           triggerStepsIncrease;
int           triggerStepsDecrease;


// Button Matrix
int           foundColumn = 0;
int           buttonValue = 0;
byte          wheelState[8];
volatile      byte pos;


// Setup Button Matrix
int           rowPin[] = {4, 5, 6, 7, 8, 9};            // Set pins for rows > OUTPUT
int           colPin[] = {A0, A1, A2, A3, 11};          // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int           rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int           colSize = sizeof(colPin)/sizeof(colPin[0]);


// Button Matrix
//      Cols  |  0              1               2               4                 5
// Rows Pins  |  14/A0          15/A1           16/A2           17/A3             11 (Display)
// ------------------------------------------------------------------------------------------------
// 0    4     |  185 Triangle   205 Circle      226 Up          248 BB+ (Up)      131 Done
// 1    5     |  204 Square     225 Cross       247 Down        270 BB- (Down)    147 Select
// 2    6     |  224 L1         246 R1          269 left        293 TC- (Left)    164 Next
// 3    7     |  245 L2         268 R2          292 Right       317 TC+ (Right)   182
// 4    8     |  267 Share      291 Options     316 PS          342 ABS- (L3)     201
// 5    9     |  290 CAB-       315 CAB+        341 Center (X)  368 ABS+ (R3)     221

void setup() {

  resetVars();

  #if DEBUG_SETUP || DEBUG_KEYS || DEBUG_WHEELS
    Serial.begin(9600);    // Arduino debug console
  #endif

  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING); // Interrupt for Button Matrix

  
  // Setup Encoders
  pinMode(IntPin, INPUT);
//  encoderBB.reset();
//  encoderBB.begin(i2cEncoderMiniLib::RMOD_X1 );
  encoderBB.onIncrement = encoderBB_increment;
  encoderBB.onDecrement = encoderBB_decrement;
//  encoderTC.reset();
//  encoderTC.begin(i2cEncoderMiniLib::RMOD_X1 );
//  encoderTC.onIncrement = encoderTC_increment;
//  encoderTC.onDecrement = encoderTC_decrement;
//  encoderABS.reset();
//  encoderABS.begin(i2cEncoderMiniLib::RMOD_X1 );
//  encoderABS.onIncrement = encoderABS_increment;
//  encoderABS.onDecrement = encoderABS_decrement;
  encoderBB.autoconfigInterrupt();


  // Setup Display & Init message
  lcd.init();
  
  if (DISPLAY_STATUS) {
    lcd.backlight();
    lcd.clear();
  } else {
    lcd.noDisplay();
    lcd.noBacklight();
    lcd.clear();
  }

  printDisplay(LOADING);

  #if DEBUG_SETUP
    Serial.println("Thrustmaster Wheel Emulator v1.0");
    Serial.println();
    Serial.print("Setup ");
    Serial.print(rowSize);
    Serial.print("x");
    Serial.print(colSize);
    Serial.println(" Button Matrix");
  #endif

  // Row setup
  //
  // Set rows as OUTPUT
  for (int i=0; i<rowSize; i++) {
    pinMode(rowPin[i], OUTPUT);
    #if DEBUG_SETUP
      Serial.print("Pin: ");
      Serial.print(rowPin[i]);
      Serial.print(" is set as OUTPUT on row: ");
      Serial.println(i);
    #endif
  }

  // Column setup
  //
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
  " BB",
  " TC",
  "ABS"
};

String CABStepsMap[] = {
  "x1",
  "x2",
  "x3",
  "x4",
  "x5"
};

int CABActionGuide[3][2][3] = {
  { // BB
    {270, 3, B10111111},   // D-Pad Down - wheelState[3] = wheelState[3] & B10111111;
    {248, 3, B11110111}    // D-Pad Up - wheelState[3] = wheelState[3] & B11110111;
  },
  { // TC
    {293, 3, B11011111},   // D-Pad Left - wheelState[3] = wheelState[3] & B11011111;
    {317, 3, B11101111}    // D-Pad Right - wheelState[3] = wheelState[3] & B11101111;
  },
  { // ABS
    {342, 1, B11111101},  // L3 - wheelState[1] = wheelState[1] & B11111101;
    {368, 1, B11111110}   // R3 - wheelState[1] = wheelState[1] & B11111110;
  }
};

void loop() {


  if (digitalRead(IntPin) == LOW) {
    /* Check the status of the encoder and call the callback */
    encoderBB.updateStatus();
  }

  //
  // Initiate Button Matrix
  // This functions returns a buttonValue for each button
  scanButtonMatrix();

  //
  // Set the action based on the buttonValue
  switch (buttonValue) {

    case 185: // Triangle
      wheelState[0] = wheelState[0] & B11111101;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Triangle", 4) : printDisplay("Pit Limiter", 2);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 204: // Square
      wheelState[0] = wheelState[0] & B11111110;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Square", 5) : printDisplay("Lights", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224: // L1
      wheelState[0] = wheelState[0] & B11110111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L1", 7) : printDisplay("Shift Down", 3);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // L2
      wheelState[1] = wheelState[1] & B11111011;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L2", 7) : printDisplay("Wipers", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 267: // Share
      wheelState[1] = wheelState[1] & B11011111;
      if (DISPLAY_KEYS) {
        printDisplay("Share", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 290: // CAB-L Combined Action Button Left
      
      triggerCAB = CAB_ACTION; // 0 for BB, 1 for TC, 2 for ABS
      triggerStepsDecrease = CAB_STEPS+1; // 4 for x5

      if (DISPLAY_KEYS) {
        printDisplay("CAB-L: "+CABActionMap[CAB_ACTION]+"- "+CABStepsMap[CAB_STEPS], 1);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: CAB-L ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 205: // Circle
      wheelState[0] = wheelState[0] & B01111111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Circle", 5) : printDisplay("Flash", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 225: // Cross
      wheelState[1] = wheelState[1] & B10111111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("HUD", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 246: // R1
      wheelState[0] = wheelState[0] & B11111011;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R1", 7) : printDisplay("Shift Up", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 268: // R2
      wheelState[1] = wheelState[1] & B11110111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R2", 7) : printDisplay("Dash", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 291: // Options
      wheelState[1] = wheelState[1] & B11101111;
      if (DISPLAY_KEYS) {
        printDisplay("Options", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 315: // CAB-R Combined Action Button Right
      
      triggerCAB = CAB_ACTION; // 0 for BB, 1 for TC, 2 for ABS
      triggerStepsIncrease = CAB_STEPS+1; // 4 for x5

      if (DISPLAY_KEYS) {
        printDisplay("CAB-R: "+CABActionMap[CAB_ACTION]+"+ "+CABStepsMap[CAB_STEPS], 1);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: CAB-R ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 226: // D-Pad Up
      wheelState[2] = wheelState[2] & B11110111; // DP-Up
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Up", 4) : printDisplay("Up", 7);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 247: // D-Pad Down
      wheelState[2] = wheelState[2] & B10111111; // DP-Down
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Down", 3) : printDisplay("Down", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 269: // D-Pad Left
      wheelState[2] = wheelState[2] & B11101111; // DP-Left
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Left", 3) : printDisplay("Left", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 292: // D-Pad Right
      wheelState[2] = wheelState[2] & B11011111; // DP-Right
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Right", 2) : printDisplay("Right", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 316: // Playstation
      wheelState[1] = wheelState[1] & B01111111;
      if (DISPLAY_KEYS) {
        printDisplay("Playstation", 2);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: PS ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 341: // Cross
      wheelState[1] = wheelState[1] & B10111111; // Cross
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("Confirm", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: Center ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 248: // D-Pad Up
      wheelState[3] = wheelState[3] & B11110111; // CHRG+
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Up", 4) : printDisplay("BB+", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 270: // D-Pad Down
      wheelState[3] = wheelState[3] & B10111111; // CHRG-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Down", 3) : printDisplay("BB-", 5);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 293: // D-Pad Left
      wheelState[3] = wheelState[3] & B11011111; // DIF IN+
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Left", 3) : printDisplay("TC-", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 317: // D-Pad Right
      wheelState[3] = wheelState[3] & B11101111; // DIF IN-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Right", 3) : printDisplay("TC+", 4);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 342: // L3
      wheelState[1] = wheelState[1] & B11111101; // Pump
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L3", 7) : printDisplay("ABS-", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 368: // R3
      wheelState[1] = wheelState[1] & B11111110; // 1-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R3", 7) : printDisplay("ABS+", 6);
        menu = 0;
      }
      #if DEBUG_KEYS
        Serial.print("Button: R3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 131: // Menu (Display)
      if (DISPLAY_STATUS) {
        if (menu == 1) {
          resetMenu();
        } else if (menu == 2) {
          menu = 1;
          menuPage = 1;
        } else if (menu == 3) {
          menu = 1;
          menuPage = 2;
        } else {
          menuPage = 1;
          menu = 1;
        }
        showMenu();
      } else {
        lcd.display();
        lcd.backlight();
        DISPLAY_STATUS = 1;
        EEPROM.write(3, DISPLAY_STATUS);
      }

      #if DEBUG_KEYS
        Serial.print("Display: Done ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 147: // Select (Display)
      lcd.display();
      lcd.backlight();
      DISPLAY_STATUS = 1;
      EEPROM.write(3, DISPLAY_STATUS);
      displaySelect();
      #if DEBUG_KEYS
        Serial.print("Display: Select("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 164: // Next (Display)
      lcd.display();
      lcd.backlight();
      DISPLAY_STATUS = 1;
      EEPROM.write(3, DISPLAY_STATUS);
      displayNext();
      #if DEBUG_KEYS
        Serial.print("Display: Next ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    default: // Reset if nothing is pressed
      break;
  }


  // CAB Trigger 
  if (triggerStepsIncrease > 0) {
    wheelState[CABActionGuide[CAB_ACTION][1][1]] = wheelState[CABActionGuide[CAB_ACTION][1][1]] & CABActionGuide[CAB_ACTION][1][2];
    triggerStepsIncrease--;
  }
  if (triggerStepsDecrease > 0) {
    wheelState[CABActionGuide[CAB_ACTION][0][1]] = wheelState[CABActionGuide[CAB_ACTION][0][1]] & CABActionGuide[CAB_ACTION][0][2];
    triggerStepsDecrease--;
  }

  #if DEBUG_WHEEL
    for (int i = 0; i < 8; i++) {
      Serial.print(wheelState[i], BIN);
      Serial.print(" ");
    }
    Serial.println();
  #endif

  
  // Reset Display if nothing is pressed for the MESSAGE_DURATION
  if (menu == 0) {
    if ((millis()-displayTime) > MESSAGE_DURATION) {
      showMenu();
      curValue = buttonValue;
    }
  }
  
  // Set a delay and reset the keyValue to something that will never match an exisitng keyValue
  delay(DEBOUNCE);
  resetVars();

}

void resetVars() {
  wheelState[0] = B11001111; // F1 wheel specific, and 5 Button
  wheelState[1] = B11111111; // 8 Buttons
  wheelState[2] = B11111111; // 8 Buttons
  wheelState[3] = B11111111; // DIFF and CHRG flags
  wheelState[4] = B00000000; // DIFF steps
  wheelState[5] = B00000000; // CHRG steps
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  buttonValue = -100;
}

void scanButtonMatrix() {
  for (int rowCounter=0; rowCounter<rowSize; rowCounter++) {
    // Set all rowPins to HIGH
    for(int i=0; i<rowSize; i++) {
      digitalWrite(rowPin[i], HIGH);
    }
    // Set the active rowPin to LOW
    digitalWrite(rowPin[rowCounter], LOW);
    // Then we scan all cols in the col[] array
    for (int colCounter=0; colCounter<colSize; colCounter++) {
      // Scan each pin in column
      foundColumn = digitalRead(colPin[colCounter]);
      // If we find a column
      if(foundColumn==0){
        // Using a Cantor Pairing function we create unique number
        buttonValue = (((rowPin[rowCounter]+colPin[colCounter]) * (rowPin[rowCounter]+colPin[colCounter]+1)) / 2 + colPin[colCounter]);
      }
    }
  }
}

void printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0) {
  if (prevLine_1 != line_1 || prevLine_2 != line_2) {
    lcd.clear();
    lcd.setCursor(pos_1, 0);
    lcd.print(line_1);
    lcd.setCursor(pos_2, 1);
    lcd.print(line_2);
    prevLine_1 = line_1;
    prevLine_2 = line_2;
    displayTime = millis();
  }
}

void encoderBB_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 270;
}
void encoderBB_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 248;
}
void encoderTC_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 293;
}
void encoderTC_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 317;
}
void encoderABS_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 342;
}
void encoderABS_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 368;
}

void displayRuntime() {
  
  lcd.clear();
  int runtimeColumn = 4;
  int runtimeRow = 0;
  
  prevLine_1 = " ";
  prevLine_2 = " ";
  displayTime = millis();

  unsigned long allSeconds=millis()/1000;
  int runHours= allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;

  if (runHours<10) {
    lcd.setCursor(runtimeColumn, runtimeRow);
    lcd.print(0);
    lcd.setCursor(runtimeColumn+1, runtimeRow);
    lcd.print(runHours);
  } else {
    lcd.setCursor(runtimeColumn, runtimeRow);
    lcd.print(runHours);
  }

  lcd.setCursor(runtimeColumn+2, runtimeRow);
  lcd.print(":");

  if (runMinutes<10) {
    lcd.setCursor(runtimeColumn+3, runtimeRow);
    lcd.print(0);
    lcd.setCursor(runtimeColumn+4, runtimeRow);
    lcd.print(runMinutes);
  } else {
    lcd.setCursor(runtimeColumn+3, runtimeRow);
    lcd.print(runMinutes);
  }

  lcd.setCursor(runtimeColumn+5, runtimeRow);
  lcd.print(":");

  if (runSeconds<10) {
    lcd.setCursor(runtimeColumn+6, runtimeRow);
    lcd.print(0);
    lcd.setCursor(runtimeColumn+7, runtimeRow);
    lcd.print(runSeconds);
  } else {
    lcd.setCursor(runtimeColumn+6, runtimeRow);
    lcd.print(runSeconds);
  }
}


void showMenu() {

  if (menu == 1) { // Main Menu
    maxPages = 5;
    switch (menuPage) {
      case 1:
        printDisplay(SELECT_OPTION, 0, CAB_ON, 0);
        break;
      case 2:
        printDisplay(CAB_OFF, 0, DISPLAY_MODE_ON, 0);
        break;
      case 3:
        printDisplay(DISPLAY_MODE_OFF, 0, DISPLAY_KEYPRESS_ON, 0);
        break;
      case 4:
        printDisplay(DISPLAY_KEYPRESS_OFF, 0, DISPLAY_STATUS_ON, 0);
        break;
      case 5:
        printDisplay(DISPLAY_STATUS_OFF, 0, DISPLAY_RUNTIME_ON, 0);
        break;
    }
  } else if (menu == 2) { // CAB Menu
    maxPages = 3;
    switch (menuPage) {
      case 1:
        printDisplay(ACTION_OFF + CABActionMap[CAB_ACTION], 0, STEPS_OFF + CABStepsMap[CAB_STEPS], 0);
        break;
      case 2:
        printDisplay(ACTION_ON + CABActionMap[CAB_ACTION], 0, STEPS_OFF + CABStepsMap[CAB_STEPS], 0);
        break;
      case 3:
        printDisplay(ACTION_OFF + CABActionMap[CAB_ACTION], 0, STEPS_ON + CABStepsMap[CAB_STEPS], 0);
        break;
    }
  } else {
    getDateTime();
    String message_line = "~Menu      " + getTime;
    printDisplay(getDate, 0, message_line, 0);
  }
  delay(DEBOUNCE);
};

void displayNext() {
  (menuPage >= maxPages) ? menuPage = 1 : menuPage++;
  delay(DEBOUNCE);
  showMenu();
}

void displaySelect() {

  if (menu == 1 && menuPage == 1) { // CAB
    menu = 2;
    menuPage = 1;
    showMenu();
  }

  if (menu == 1 && menuPage == 2) { // Display Mode
    toggleDisplayMode();
  }

  if (menu == 1 && menuPage == 3) { // Display Keypress
    toggleDisplayKeypress();
  }

  if (menu == 1 && menuPage == 4) { // Display Off
    toggleBacklight();
    resetMenu();
  }

  if (menu == 1 && menuPage == 5) { // Display Runtime
    displayRuntime();
    delay(MESSAGE_DURATION);
    showMenu();
  }

  if (menu == 2 && menuPage == 2) { // CAB Action Select
    setCABAction();
    showMenu();
  }

  if (menu == 2 && menuPage == 3) { // CAB Steps Select
    setCABSteps();
    showMenu();
  }

}

void setCABAction() {
  if (CAB_ACTION >= 2) {
    CAB_ACTION = 0;
  } else {
    CAB_ACTION++;
  }
  EEPROM.write(4, CAB_ACTION);
  delay(DEBOUNCE);
}

void setCABSteps () {
  if (CAB_STEPS >= 4) {
    CAB_STEPS = 0;
  } else {
    CAB_STEPS++;
  }
  EEPROM.write(5, CAB_STEPS);
  delay(DEBOUNCE);
}

void toggleBacklight() {
  printDisplay(DISPLAY_STATUS_OFF, 2);
  delay(MESSAGE_DURATION);
  lcd.noDisplay();
  lcd.noBacklight();
  DISPLAY_STATUS = 0;
  EEPROM.write(3, DISPLAY_STATUS);
  showMenu();
}

void toggleDisplayKeypress() {
  if (DISPLAY_KEYS == 1) {
    DISPLAY_KEYS = 0;
    printDisplay(KEYPRESS_OFF, 1);
  } else {
    DISPLAY_KEYS = 1;
    printDisplay(KEYPRESS_ON, 2);
  }
  EEPROM.write(1, DISPLAY_KEYS);
  delay(MESSAGE_DURATION);
  showMenu();
}

void toggleDisplayMode() {
  if (DISPLAY_MODE == 1) {
    DISPLAY_MODE = 0;
    printDisplay(MODE_WHEEL, 2);
  } else {
    DISPLAY_MODE = 1;
    printDisplay(MODE_PS, 4);
  }
  EEPROM.write(0, DISPLAY_MODE);
  delay(MESSAGE_DURATION);
  showMenu();
}

void resetMenu() {
  menuPage = 1;
  menu = 0;
}

void getDateTime() {
  
  String months[13] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  String days[8] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  String tm_hour; 
  String tm_minute;
  String tm_date = String(tm.Day);
  
  if (RTC.read(tm)) {
    if (tm.Hour<10) {
      tm_hour = "0" + String(tm.Hour);
    } else {
      tm_hour = String(tm.Hour);
    }
    if (tm.Minute<10) {
      tm_minute = "0" + String(tm.Minute);
    } else {
      tm_minute = String(tm.Minute);
    }

    getDate = days[tm.Wday] + " " + months[tm.Month] + " " + tm_date;
    getTime = tm_hour + ":" + tm_minute;
  } else {
    getDate = "Clock error!";
    getTime = "00:00";
  }
 
}
