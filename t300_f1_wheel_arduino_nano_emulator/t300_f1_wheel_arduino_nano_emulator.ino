#include <LiquidCrystal_I2C.h>

//Arduino GND                 -> T300 Blue wire (2)
//Arduino pin 12              -> T300 White wire (3) (data from wheel to base)
//Arduino pin 10 + pin 2 (SS) -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
//Arduino pin 13 (SCK)        -> T300 Red wire (5)
//Arduino +5V                 -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#define     DEBUG false        // Debug the button presses
#define     DEBUG_SETUP false // Debug Setup information
#define     DEBUG_WHEEL false // Debug wheel output

// Setup LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Button Matrix
int         foundColumn = 0;
int         buttonValue = 0;
int         debounce = 50; // Set this to the lowest value that gives the best result
byte        wheelState[8]; // local push-buttons state saved here
volatile    byte pos;

void        printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0);
int         curValue = 9999;
String      prevLine_1;
String      prevLine_2;

// Setup Button Matrix
int     rowPin[] = {3, 4, 5, 6, 7, A1}; // Set pins for rows > OUTPUT
int     colPin[] = {8, 9, 11, A0}; // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int     rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int     colSize = sizeof(colPin)/sizeof(colPin[0]);

// Button Matrix
//      Cols  |  0              1               2             4
// Rows Pins  |  8              9               11            14/A0
// -------------------------------------------------------------------------
// 0    3     |  74 Triangle    87 Circle       116 Up        167 TC- (Up)
// 1    4     |  86 Square      100 Cross       131 Down      185 TC+ (Down)
// 2    5     |  99 L1          114 R1          147 left      204 BB- (Left)
// 3    6     |  113 L2         129 R2          164 Right     224 BB+ (Right)
// 4    7     |  128 Share      145 Options     182 PS        245 ABS- (L3)
// 5    15/A1 |  284 CB-L       309 CB-R        362 Center    449 ABS+ (R3)


// Setup Encoders
CommonBusEncoders encoders(A0, A1, A2, 3); // (Pin A Bus, Pin B Bus, Button Bus, Number of encoders)


void resetVars() {
  wheelState[0] = B11001111; // F1 wheel specific, and 5 Button
  wheelState[1] = B11111111; // 8 Buttons
  wheelState[2] = B11111111; // 8 Buttons
  wheelState[3] = B11111111; // DIFF and CHRG flags
  wheelState[4] = B00000000; // DIFF steps
  wheelState[5] = B00000000; // CHRG steps
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  buttonValue = 0;
}

void setup(){

  resetVars();

  Serial.begin(115200);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1

  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING); // Interrupt for Button Matrix

  lcd.init();
  lcd.backlight();
  lcd.clear();
  printDisplay("Custom GT3 Wheel", 0, "v1.0", 6);

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


  // Setup Encoders
  //
  // We are using CommonBusEncoder.h Library
  encoders.addEncoder(1, 2, A3, 1, 700, 799); // (Encoder ID, Type: 2 or 4 step, Common Pin, Mode, Code, Button Code)
  encoders.addEncoder(2, 2, A4, 1, 800, 899);
  encoders.addEncoder(3, 2, A5, 1, 900, 999);
  #if DEBUG
    Serial.println();
    Serial.println("Setup 3 Encoders");
    Serial.println("Encoder 1 on A:A0, B:A1, S:A2, C:A3");
    Serial.println("Encoder 2 on A:A0, B:A1, S:A2, C:A4");
    Serial.println("Encoder 3 on A:A0, B:A1, S:A2, C:A5");
  #endif

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
  // Set the encoder action based on the code
  switch (buttonValue) {

    case 74: // Triangle
      wheelState[0] = wheelState[0] & B10111111;
      printDisplay("Triangle", 4);
      #if DEBUG
        Serial.print("Button: Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 86: // Square
      wheelState[0] = wheelState[0] & B01111111;
      printDisplay("Square", 5);
      #if DEBUG
        Serial.print("Button: Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 99: // L1
      wheelState[0] = wheelState[0] & B11011111;
      printDisplay("L1", 7);
      #if DEBUG
        Serial.print("Button: L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 113: // L2
      wheelState[1] = wheelState[1] & B11011111;
      printDisplay("L2", 7);
      #if DEBUG
        Serial.print("Button: L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 128: // Share
      wheelState[1] = wheelState[1] & B11111011;
      printDisplay("Share", 5);
      #if DEBUG
        Serial.print("Button: Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 87: // Circle
      wheelState[0] = wheelState[0] & B11111110;
      printDisplay("Circle", 5);
      #if DEBUG
        Serial.print("Button: Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 100: // Cross
      wheelState[1] = wheelState[1] & B11111101;
      printDisplay("Cross", 5);
      #if DEBUG
        Serial.print("Button: Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 114: // R1
      wheelState[0] = wheelState[0] & B11101111;
      printDisplay("R1", 7);
      #if DEBUG
        Serial.print("Button: R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 129: // R2
      wheelState[1] = wheelState[1] & B11101111;
      printDisplay("R2", 7);
      #if DEBUG
        Serial.print("Button: R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 145: // Options
      wheelState[1] = wheelState[1] & B11110111;
      printDisplay("Options", 4);
      #if DEBUG
        Serial.print("Button: Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 116: // Up
      wheelState[2] = wheelState[2] & B11101111; // DP-Up
      printDisplay("Up", 7);
      #if DEBUG
        Serial.print("Button: Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 131: // Down
      wheelState[2] = wheelState[2] & B11111101; // DP-Down
      printDisplay("Down", 6);
      #if DEBUG
        Serial.print("Button: Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 147: // Left
      wheelState[2] = wheelState[2] & B11110111; // DP-Left
      printDisplay("Left", 6);
      #if DEBUG
        Serial.print("Button: Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 164: // Right
      wheelState[2] = wheelState[2] & B11111011; // DP-Right
      printDisplay("Right", 5);
      #if DEBUG
        Serial.print("Button: Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 182: // PS
      wheelState[1] = wheelState[1] & B11111110;
      printDisplay("PS", 7);
      #if DEBUG
        Serial.print("Button: PS ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 362: // Center
      wheelState[1] = wheelState[1] & B11111101; // Cross
      printDisplay("Center", 5);
      #if DEBUG
        Serial.print("Button: Center ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 167: // TC- (Up)
//      wheelState[2] = wheelState[2] & B11101111; // DP-Up
      wheelState[3] = wheelState[3] & B11101111; // CHRG+
      printDisplay("TC-", 7);
      #if DEBUG
        Serial.print("Button: TC- (Up) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 185: // TC+ (Down)
//      wheelState[2] = wheelState[2] & B11111101; // DP-Down
      wheelState[3] = wheelState[3] & B11111101; // CHRG-
      printDisplay("TC+", 7);
      #if DEBUG
        Serial.print("Button: TC+ (Down) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 204: // BB- (Left)
//      wheelState[2] = wheelState[2] & B11110111; // DP-Left
      wheelState[3] = wheelState[3] & B11111011; // DIF IN+
      printDisplay("BB-", 7);
      #if DEBUG
        Serial.print("Button: BB- (Left) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224: // BB+ (Right)
//      wheelState[2] = wheelState[2] & B11111011; // DP-Right
      wheelState[3] = wheelState[3] & B11110111; // DIF IN-
      printDisplay("BB+", 7);
      #if DEBUG
        Serial.print("Button: BB+ (Right) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // ABS- (L3)
      wheelState[1] = wheelState[1] & B10111111; // Pump
      printDisplay("ABS-", 6);
      #if DEBUG
        Serial.print("Button: ABS- (L3) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 449: // ABS+ (R3)
      wheelState[1] = wheelState[1] & B01111111; // 1-
      printDisplay("ABS+", 6);
      #if DEBUG
        Serial.print("Button: ABS+ (R3) ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;


    case 284: // CAB-L Combined Action Button
      printDisplay("CAB-L");
      #if DEBUG
        Serial.print("Button: CAB-L ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 309: // CAB-R Combined Action Button
      printDisplay("CAB-R");
      #if DEBUG
        Serial.print("Button: CAB-R ("); Serial.print(buttonValue); Serial.println(") ");
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

  if (curValue != buttonValue) {
    printDisplay("Custom GT3 Wheel", 0, "v1.0", 6);
    curValue = buttonValue;
  }

  // Set a delay and reset the keyValue to something that will never match an exisitng keyValue
  delay(debounce);
  resetVars();

}

void printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0) {
  if (prevLine_1 != line_1 || prevLine_2 != line_2) {
    lcd.clear();
  }
  lcd.setCursor(pos_1, 0);
  lcd.print(line_1);
  lcd.setCursor(pos_2, 1);
  lcd.print(line_2);
  prevLine_1 = line_1;
  prevLine_2 = line_2;
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
