// Connect to Thrustmaster T300
//
// Arduino GND                  -> T300 Blue wire (2)
// Arduino pin 12               -> T300 White wire (3) (data from wheel to base)
// Arduino pin 10 + pin 2 (SS)  -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
// Arduino pin 13 (SCK)         -> T300 Red wire (5)
// Arduino +5V                  -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#define     DEBUG_SETUP false   // Debug Setup information
#define     DEBUG_KEYS false    // Debug the button presses
#define     DEBUG_WHEEL false   // Debug wheel output
#define     DISPLAY_LCD false    // If you're using an 16x2 LCD
#define     DISPLAY_MODE false   // Set the display outout. 'true' for Playstation keys, 'false' for Thrustmaster F1 Wheel

#if DISPLAY_LCD
  #include <LiquidCrystal_I2C.h>
  // Setup LCD & vars
  LiquidCrystal_I2C lcd(0x27, 16, 2); // Connects to 5V, GND, A4 (SDA), A5 (SCL)
  bool        lcdBacklight = true; // Used for the LCD backlight toggle
  void        printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0);
  int         curValue = -100;
  long        displayTime = millis(); // Used for delaying the screen message
  long        lcdTime = millis(); // Used for delaying the screen light toggle
  String      prevLine_1;
  String      prevLine_2;
#endif

// Button Matrix
int         foundColumn = 0;
int         buttonValue = 0;
int         debounce = 50;      // Set this to the lowest value that gives the best result
byte        wheelState[8];      // local push-buttons state saved here
volatile    byte pos;

// Setup Button Matrix
int     rowPin[] = {4, 5, 6, 7, 8, 9}; // Set pins for rows > OUTPUT
int     colPin[] = {A0, A1, A2, A3}; // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int     rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int     colSize = sizeof(colPin)/sizeof(colPin[0]);

// Button Matrix
//      Cols  |  0              1               2               4
// Rows Pins  |  14/A0          15/A1           16/A2           17/A3
// ------------------------------------------------------------------------------
// 0    4     |  185 Triangle   205 Circle      226 Up          248 TC- (Up)
// 1    5     |  204 Square     225 Cross       247 Down        270 TC+ (Down)
// 2    6     |  224 L1         246 R1          269 left        293 BB- (Left)
// 3    7     |  245 L2         268 R2          292 Right       317 BB+ (Right)
// 4    8     |  267 Share      291 Options     316 PS          342 ABS- (L3)
// 5    9     |  290 CAB-L      315 CAB-R       341 Center (X)  368 ABS+ (R3)

void setup() {

  resetVars();

  Serial.begin(9600);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING); // Interrupt for Button Matrix
  
  #if DISPLAY_LCD
    // Setup Display & Init message
    lcd.init();
    lcd.backlight();
    lcd.clear();
    printDisplay("Loading...", 3);
  #endif

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
    
    case 185: // N (Triangle)
      wheelState[0] = wheelState[0] & B11111101;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Triangle", 4) : printDisplay("N", 7);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 204: // PIT (Square)
      wheelState[0] = wheelState[0] & B11111110;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Square", 5) : printDisplay("PIT", 5);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224: // Shift Down (L1)
      wheelState[0] = wheelState[0] & B11110111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("L1", 7) : printDisplay("Shift Down", 3);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // K (L2)
      wheelState[1] = wheelState[1] & B11111011;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("L2", 7) : printDisplay("K", 7);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 267: // BO (Share)
      wheelState[1] = wheelState[1] & B11011111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Share", 5) : printDisplay("BO", 7);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 290: // CAB-L Combined Action Button Left
      // Do something here
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("CAB-L", 5) : printDisplay("Combo Button 1", 1);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: CAB-L ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 205: // DRS (Circle)
      wheelState[0] = wheelState[0] & B01111111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Circle", 5) : printDisplay("DRS", 6);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 225: // 10+ (Cross)
      wheelState[1] = wheelState[1] & B10111111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("10+", 6);
       #endif
      #if DEBUG_KEYS
        Serial.print("Button: Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 246: // Shift Up (R1)
      wheelState[0] = wheelState[0] & B11111011;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("R1", 7) : printDisplay("Shift Up", 4);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 268: // PL (R2)
      wheelState[1] = wheelState[1] & B11110111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("R2", 7) : printDisplay("PL", 7);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 291: // WET (Options)
      wheelState[1] = wheelState[1] & B11101111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Options", 4) : printDisplay("WET", 6);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 315: // CAB-R Combined Action Button Right
      // Do something here
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("CAB-R", 5) : printDisplay("Combo Button 2", 1);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: CAB-R ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 226: // D-Pad Up
      wheelState[2] = wheelState[2] & B11110111; // DP-Up
      #if DISPLAY_LCD
        printDisplay("D-Pad Up", 4);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 247: // D-Pad Down
      wheelState[2] = wheelState[2] & B10111111; // DP-Down
      #if DISPLAY_LCD
        printDisplay("D-Pad Down", 3);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 269: // D-Pad Left
      wheelState[2] = wheelState[2] & B11101111; // DP-Left
      #if DISPLAY_LCD
        printDisplay("D-Pad Left", 3);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 292: // D-Pad Right
      wheelState[2] = wheelState[2] & B11011111; // DP-Right
      #if DISPLAY_LCD
        printDisplay("D-Pad Right", 2);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 316: // Start (Playstation)
      wheelState[1] = wheelState[1] & B01111111;
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Playstation", 2) : printDisplay("Start", 5);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: PS ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 341: // 10+ (Cross)
      wheelState[1] = wheelState[1] & B10111111; // Cross
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("Cross", 5) : printDisplay("10+", 6);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: Center ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 248: // CRHG+ (D-Pad Up)
      wheelState[3] = wheelState[3] & B11110111; // CHRG+
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("D-Pad Up", 4) : printDisplay("CHRG+", 5);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 270: // CHRG- (D-Pad Down)
      wheelState[3] = wheelState[3] & B10111111; // CHRG-
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("D-Pad Down", 3) : printDisplay("CHRG-", 5);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 293: // DIF IN+ (D-Pad Left)
      wheelState[3] = wheelState[3] & B11011111; // DIF IN+
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("D-Pad Left", 3) : printDisplay("DIF IN+", 4);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 317: // DIF IN- (D-Pad Right)
      wheelState[3] = wheelState[3] & B11101111; // DIF IN-
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("D-Pad Right", 3) : printDisplay("DIF IN-", 4);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 342: // Pump (L3)
      wheelState[1] = wheelState[1] & B11111101; // Pump
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("L3", 7) : printDisplay("Pump", 6);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 368: // 1- (R3)
      wheelState[1] = wheelState[1] & B11111110; // 1-
      #if DISPLAY_LCD
        (DISPLAY_MODE) ? printDisplay("R3", 7) : printDisplay("1-", 6);
      #endif
      #if DEBUG_KEYS
        Serial.print("Button: R3 ("); Serial.print(buttonValue); Serial.println(") ");
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

  #if DISPLAY_LCD
    // Reset Display if nothing is pressed for 750 millis
    if ((millis()-displayTime) > 750) {
      printDisplay("Custom GT3 Wheel", 0, "v1.0", 6);
      curValue = buttonValue;
    }
  #endif

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

#if DISPLAY_LCD
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
  
  void lcdBacklightToggle() {
    if ((millis()-lcdTime) > 750) {
      if (lcdBacklight == false) {
        lcd.backlight();
        lcdBacklight = true;
      } else {
        lcd.noBacklight();
        lcdBacklight = false;
      }
    }
    lcdTime = millis();
  }
#endif
