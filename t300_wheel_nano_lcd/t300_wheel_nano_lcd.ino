// Connect to Thrustmaster T300
//
// Arduino GND                        -> T300 Blue wire (2)
// Arduino pin 12                     -> T300 White wire (3) (data from wheel to base)
// Arduino pin 10 + pin 2 (SS)        -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
// Arduino pin 13 (SCK)               -> T300 Red wire (5)
// Arduino +5V                        -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#include      <EEPROM.h>              // Load EEPROM library for storing settings
#include      <LiquidCrystal_I2C.h>   // Load the Liquid Crystal Display library

#define       DEBUG_SETUP false       // Debug Setup information
#define       DEBUG_KEYS false        // Debug the button presses
#define       DEBUG_WHEEL false       // Debug wheel output

String        defaultLine1 = "GT Challenge";
String        defaultLine2 = ">Menu";

// Setup LCD & vars
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Connects to 5V, GND, A4 (SDA), A5 (SCL)
bool          lcdBacklight = true;    // Used for the LCD backlight toggle
int           curValue = -100;
int           messageDuration = 750;  // Duration of the messages on the screen
String        prevLine_1;
String        prevLine_2;
void          printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0);
unsigned long displayTime = millis(); // Used for delaying the screen message

// Setup EEPROM 
bool          DISPLAY_MODE = EEPROM.read(0); // Set the display outout. 'true:1' for Playstation, 'false:0' for Wheel
bool          DISPLAY_KEYS = EEPROM.read(1); // Set the display keypress option.
int           DISPLAY_MESSAGE = EEPROM.read(2); // Set the display default message.
bool          DISPLAY_STATUS = EEPROM.read(3); // Set the display status (on or off)
int           LEFT_BUTTON_ACTION = EEPROM.read(4); // Set the left button action
int           LEFT_BUTTON_REPEAT = EEPROM.read(5); // Set the left button action repeat
int           RIGHT_BUTTON_ACTION = EEPROM.read(6); // Set the right button action
int           RIGHT_BUTTON_REPEAT = EEPROM.read(7); // Set the right button action repeat

// Menu Navigation Setup
bool          mainMenu = false;
bool          leftButtonMenu = false;
bool          rightButtonMenu = false;
int           menuPage = 1;
int           maxPages = 1;

// Button Matrix
int         foundColumn = 0;
int         buttonValue = 0;
int         debounce = 100;           // Set this to the lowest value that gives the best result
byte        wheelState[8];
volatile    byte pos;

// Setup Button Matrix
int     rowPin[] = {4, 5, 6, 7, 8, 9}; // Set pins for rows > OUTPUT
int     colPin[] = {A0, A1, A2, A3, 11}; // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int     rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int     colSize = sizeof(colPin)/sizeof(colPin[0]);

// Button Matrix
//      Cols  |  0              1               2               4                 5
// Rows Pins  |  14/A0          15/A1           16/A2           17/A3             11 (Display)
// ------------------------------------------------------------------------------------------------
// 0    4     |  185 Triangle   205 Circle      226 Up          248 TC- (Up)      131 Done
// 1    5     |  204 Square     225 Cross       247 Down        270 TC+ (Down)    147 Select
// 2    6     |  224 L1         246 R1          269 left        293 BB- (Left)    164 Next
// 3    7     |  245 L2         268 R2          292 Right       317 BB+ (Right)   182 
// 4    8     |  267 Share      291 Options     316 PS          342 ABS- (L3)     201 
// 5    9     |  290 CAB-L      315 CAB-R       341 Center (X)  368 ABS+ (R3)     221 

void setup() {

  resetVars();

  Serial.begin(9600);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING); // Interrupt for Button Matrix
  
  // Setup Display & Init message
  lcd.init();
  lcd.backlight();
  lcd.clear();
  printDisplay("Loading...", 3);

  #if DEBUG_SETUP
    Serial.println("Thrustmaster Custom Wheel Emulator v1.0");
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

void loop() {

  //
  // Initiate Button Matrix
  // This functions returns a buttonValue for each button
  scanButtonMatrix();

  //
  // Set the action based on the buttonValue
  switch (buttonValue) {
    
    case 185: // Pit Limiter (Triangle)
      wheelState[0] = wheelState[0] & B11111101;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Triangle", 4) : printDisplay("Pit Limiter", 2);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 204: // PIT (Square)
      wheelState[0] = wheelState[0] & B11111110;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Square", 5) : printDisplay("Lights", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224: // Shift Down (L1)
      wheelState[0] = wheelState[0] & B11110111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L1", 7) : printDisplay("Shift Down", 3);
      }
      #if DEBUG_KEYS
        Serial.print("Button: L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // Wipers (L2)
      wheelState[1] = wheelState[1] & B11111011;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L2", 7) : printDisplay("Wipers", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 267: // Share
      wheelState[1] = wheelState[1] & B11011111;
      if (DISPLAY_KEYS) {
        printDisplay("Share", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 290: // CAB-L Combined Action Button Left
      // Do something here

      if (DISPLAY_KEYS) {
        printDisplay("CAB-L", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: CAB-L ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 205: // Flash (Circle)
      wheelState[0] = wheelState[0] & B01111111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Circle", 5) : printDisplay("Flash", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 225: // HUD (Cross)
      wheelState[1] = wheelState[1] & B10111111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("HUD", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 246: // Shift Up (R1)
      wheelState[0] = wheelState[0] & B11111011;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R1", 7) : printDisplay("Shift Up", 4);
      }
      #if DEBUG_KEYS
        Serial.print("Button: R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 268: // Dash (R2)
      wheelState[1] = wheelState[1] & B11110111;
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R2", 7) : printDisplay("Dash", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 291: // Options
      wheelState[1] = wheelState[1] & B11101111;
      if (DISPLAY_KEYS) {
        printDisplay("Options", 4);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 315: // CAB-R Combined Action Button Right
      // Do something here
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("CAB-R", 5) : printDisplay("Combo Button 2", 1);
      }
      #if DEBUG_KEYS
        Serial.print("Button: CAB-R ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 226: // D-Pad Up
      wheelState[2] = wheelState[2] & B11110111; // DP-Up
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Up", 4) : printDisplay("Up", 7);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 247: // D-Pad Down
      wheelState[2] = wheelState[2] & B10111111; // DP-Down
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Down", 3) : printDisplay("Down", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 269: // D-Pad Left
      wheelState[2] = wheelState[2] & B11101111; // DP-Left
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Left", 3) : printDisplay("Left", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 292: // D-Pad Right
      wheelState[2] = wheelState[2] & B11011111; // DP-Right
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Right", 2) : printDisplay("Right", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 316: // Playstation
      wheelState[1] = wheelState[1] & B01111111;
      if (DISPLAY_KEYS) {
        printDisplay("Playstation", 2);
      }
      #if DEBUG_KEYS
        Serial.print("Button: PS ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 341: // Cross
      wheelState[1] = wheelState[1] & B10111111; // Cross
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("Confirm", 4);
      }
      #if DEBUG_KEYS
        Serial.print("Button: Center ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 248: // TC- (D-Pad Up)
      wheelState[3] = wheelState[3] & B11110111; // CHRG+
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Up", 4) : printDisplay("TC-", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 270: // TC+ (D-Pad Down)
      wheelState[3] = wheelState[3] & B10111111; // CHRG-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Down", 3) : printDisplay("TC+", 5);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 293: // BB- (D-Pad Left)
      wheelState[3] = wheelState[3] & B11011111; // DIF IN+
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Left", 3) : printDisplay("BB-", 4);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 317: // BB+ (D-Pad Right)
      wheelState[3] = wheelState[3] & B11101111; // DIF IN-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("D-Pad Right", 3) : printDisplay("BB+", 4);
      }
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 342: // ABS- (L3)
      wheelState[1] = wheelState[1] & B11111101; // Pump
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("L3", 7) : printDisplay("ABS-", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 368: // ABS+ (R3)
      wheelState[1] = wheelState[1] & B11111110; // 1-
      if (DISPLAY_KEYS) {
        (DISPLAY_MODE) ? printDisplay("R3", 7) : printDisplay("ABS+", 6);
      }
      #if DEBUG_KEYS
        Serial.print("Button: R3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 131: // Done (Display)
      lcd.display();
      lcd.backlight();
      if (mainMenu || leftButtonMenu || rightButtonMenu) {
        resetMenu();
      } else {
        menuPage = 1;
        mainMenu = true;
        leftButtonMenu = false;
        rightButtonMenu = false;
      }
      showMenu();
      #if DEBUG_KEYS
        Serial.print("Display: Done ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 147: // Select (Display)
      lcd.display();
      lcd.backlight();
      displaySelect();
      #if DEBUG_KEYS
        Serial.print("Display: Select("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 164: // Next (Display)
      lcd.display();
      lcd.backlight();
      displayNext();
      #if DEBUG_KEYS
        Serial.print("Display: Next ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
      
    default: // Reset if nothing is pressed
      break;
  }

  #if DEBUG_WHEEL
    for (int i = 0; i < 8; i++) {
      Serial.print(wheelState[i], BIN);
      Serial.print(" ");
    }
    Serial.println();
  #endif

  // Reset Display if nothing is pressed for the messageDuration
  if (!mainMenu && !leftButtonMenu && !rightButtonMenu) {
    if ((millis()-displayTime) > messageDuration) {
      printDisplay(defaultLine1, 0, defaultLine2, 0);
      curValue = buttonValue;
    }
  }

  // Set a delay and reset the keyValue to something that will never match an exisitng keyValue
  delay(debounce);
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
  if (!mainMenu && !leftButtonMenu && !rightButtonMenu) {
    displayRuntime();
  }
}

void displayRuntime() {
  int runtimeColumn = 8;
  int runtimeRow = 1;
  
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

String keyActionMap[] = {
  " BB",
  " TC",
  "ABS"
};

String keyRepeatMap[] = {
  "x1",
  "x2",
  "x3",
  "x4",
  "x5"
};

void showMenu() {
  
  if (mainMenu) {
    maxPages = 7;
    switch (menuPage) {
      case 0:
        menuPage = 1;
        break;
      case 1:
        printDisplay(" Select Option", 0, ">Left CAB", 0);
        break;
      case 2:
        printDisplay(" Left CAB", 0, ">Right CAB", 0);
        break;
      case 3:
        printDisplay(" Right CAB", 0, ">Default Message", 0);
        break;
      case 4:
        printDisplay(" Default Message", 0, ">Display Mode", 0);
        break;
      case 5:
        printDisplay(" Display Mode", 0, ">Disp. Keypress", 0);
        break;
      case 6:
        printDisplay(" Disp. Keypress", 0, ">Display Off ", 0);
        break;
      case 7:
        printDisplay(" Display Off", 0, ">Restart Wheel", 0);
        break;
    }
  } else if (leftButtonMenu) {
    maxPages = 3;
    switch (menuPage) {
      case 0:
        menuPage = 1;
        break;
      case 1:
        printDisplay(" Action:", 0, " Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[LEFT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[LEFT_BUTTON_REPEAT]);
        break;
      case 2:
        printDisplay(">Action:", 0, " Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[LEFT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[LEFT_BUTTON_REPEAT]);
        break;
      case 3:
        printDisplay(" Action:", 0, ">Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[LEFT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[LEFT_BUTTON_REPEAT]);
        break;
    }
  } else if (rightButtonMenu) {
    maxPages = 3;
    switch (menuPage) {
      case 0:
        menuPage = 1;
        break;
      case 1:
        printDisplay(" Action:", 0, " Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[RIGHT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[RIGHT_BUTTON_REPEAT]);
        break;
      case 2:
        printDisplay(">Action:", 0, " Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[RIGHT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[RIGHT_BUTTON_REPEAT]);
        break;
      case 3:
        printDisplay(" Action:", 0, ">Reps:", 0);
        lcd.setCursor(13, 0);
        lcd.print(keyActionMap[RIGHT_BUTTON_ACTION]);  
        lcd.setCursor(14, 1);
        lcd.print(keyRepeatMap[RIGHT_BUTTON_REPEAT]);
        break;
    }
  } else {
    printDisplay(defaultLine1, 0, defaultLine2, 0);
  }
  delay(debounce);
};

void displayNext() {
  if (menuPage >= maxPages) {
    menuPage = 1;
  } else {
    menuPage++;
  }
  delay(debounce);
  showMenu();
}

void displaySelect() {

  if (mainMenu && menuPage == 1) { // Left Button Actions
    mainMenu = false;
    leftButtonMenu = true;
    rightButtonMenu = false;
    menuPage = 1;
    showMenu();
  }
  
  if (mainMenu && menuPage == 2) { // Right Button Actions
    mainMenu = false;
    leftButtonMenu = false;
    rightButtonMenu = true;
    menuPage = 1;
    showMenu();
  }
  
  if (mainMenu && menuPage == 4) { // Display Mode
    toggleDisplayMode();
  }
  
  if (mainMenu && menuPage == 5) { // Display Keypress
    toggleDisplayKeypress();
  }

  if (mainMenu && menuPage == 6) { // Display Off
    toggleBacklight();
    resetMenu();
  }

  if (leftButtonMenu && menuPage == 2) { // Left Button Action Select
    setLeftButtonAction();
    showMenu();
  }

  if (leftButtonMenu && menuPage == 3) { // Left Button Reps Select
    setLeftButtonRepeat();
    showMenu();
  }

  if (rightButtonMenu && menuPage == 2) { // Right Button Action Select
    setRightButtonAction();
    showMenu();
  }

  if (rightButtonMenu && menuPage == 3) { // Right Button Reps Select
    setRightButtonRepeat();
    showMenu();
  }

}

//char keyActionGuide[3][2][4] = {
//  {
//    {293, "BB-", 3, B11011111},   // wheelState[3] = wheelState[3] & B11011111;
//    {317, "BB+", 3, B11101111}    // wheelState[3] = wheelState[3] & B11101111;
//  },
//  {
//    {248, "TC-", 3, B11110111},   // wheelState[3] = wheelState[3] & B11110111;
//    {270, "TC+", 3, B10111111}    // wheelState[3] = wheelState[3] & B10111111;
//  },
//  {
//    {342, "ABS-", 1, B11111101},  // wheelState[1] = wheelState[1] & B11111101;
//    {368, "ABS+", 1, B11111110}   // wheelState[1] = wheelState[1] & B11111110;
//  }
//};

void setLeftButtonAction() {
  if (LEFT_BUTTON_ACTION >= 2) {
    LEFT_BUTTON_ACTION = 0;
  } else {
    LEFT_BUTTON_ACTION++;
  }
  lcd.setCursor(13, 0);
  lcd.print(keyActionMap[LEFT_BUTTON_ACTION]);
  EEPROM.write(4, LEFT_BUTTON_ACTION);  
  delay(debounce);
}

void setLeftButtonRepeat () {
  if (LEFT_BUTTON_REPEAT >= 4) {
    LEFT_BUTTON_REPEAT = 0;
  } else {
    LEFT_BUTTON_REPEAT++;
  }
  lcd.setCursor(14, 1);
  lcd.print(keyRepeatMap[LEFT_BUTTON_REPEAT]);
  EEPROM.write(5, LEFT_BUTTON_REPEAT);     
  delay(debounce);
}

  
void setRightButtonAction() {
  if (RIGHT_BUTTON_ACTION >= 2) {
    RIGHT_BUTTON_ACTION = 0;
  } else {
    RIGHT_BUTTON_ACTION++;
  }
  lcd.setCursor(13, 0);
  lcd.print(keyActionMap[RIGHT_BUTTON_ACTION]);
  EEPROM.write(6, RIGHT_BUTTON_ACTION);  
  delay(debounce);
}

void setRightButtonRepeat () {
  if (RIGHT_BUTTON_REPEAT >= 4) {
    RIGHT_BUTTON_REPEAT = 0;
  } else {
    RIGHT_BUTTON_REPEAT++;
  }
  lcd.setCursor(14, 1);
  lcd.print(keyRepeatMap[RIGHT_BUTTON_REPEAT]);
  EEPROM.write(7, RIGHT_BUTTON_REPEAT);     
  delay(debounce);
}

void  toggleBacklight() {
  printDisplay("Display Off", 2);
  delay(messageDuration);
  lcd.noDisplay();
  lcd.noBacklight();
  lcdBacklight = false;
  showMenu();
}

void toggleDisplayKeypress() {
  if (DISPLAY_KEYS == 1) {
    DISPLAY_KEYS = 0;
    printDisplay("Keypress: Off", 1);
  } else {
    DISPLAY_KEYS = 1;
    printDisplay("Keypress: On", 2);
  }
  EEPROM.write(1, DISPLAY_KEYS);
  delay(messageDuration);
  showMenu();
}

void toggleDisplayMode() {
  if (DISPLAY_MODE == 1) {
    DISPLAY_MODE = 0;
    printDisplay("Mode: Wheel", 2);
  } else {
    DISPLAY_MODE = 1;
    printDisplay("Mode: PS", 4);
  }
  EEPROM.write(0, DISPLAY_MODE);
  delay(messageDuration);
  showMenu();
}

void resetMenu() {
  menuPage = 1;
  mainMenu = false;
  leftButtonMenu = false;
  rightButtonMenu = false;
}

//String defaultMessage[20] = {
//  {"GT Challenge"},
//  {"Lexus RC F GT3"},
//  {"BMW M6 GT3"},
//  {"Lambohrghini Huracan GT3 EVO"},
//  {"Nissan GT-R Nismo GT3"},
//  {"Audi R8 LMS GT3 Evo"},
//  {"Bentley Continental GT3"},
//  {"McLaren 720S GT3"},
//  {"Porsche 911 II GT3 R"},
//  {"Mercedes AMG GT3"},
//  {"Aston Martin V8 Vantage GT3"},
//  {"Ferrari 488 GT3"},
//  {"Ginetta G55 GT4"},
//  {"Chevrolet Camaro GT4"},
//  {"Mercedes AMG GT4"},
//  {"Aston Martin V8 Vantage GT4"},
//  {"BMW M4 GT4"},
//  {"Porsche 718 Cayman GT4"}
//};
