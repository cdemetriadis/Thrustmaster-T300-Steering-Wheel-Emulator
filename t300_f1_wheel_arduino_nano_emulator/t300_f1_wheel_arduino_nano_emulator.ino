#include <CommonBusEncoders.h>

//Arduino GND                 -> T300 Blue wire (2)
//Arduino pin 12              -> T300 White wire (3) (data from wheel to base)
//Arduino pin 10 + pin 2 (SS) -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
//Arduino pin 13 (SCK)        -> T300 Red wire (5)
//Arduino +5V                 -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#define     DEBUG false
#define     DEBUG_WHEEL false

// Button Matrix
int         foundColumn = 0;
int         buttonValue = 0;
int         encoderValue = 0;
int         debounce = 50; // Set this to the lowest value that gives the best result
byte        wheelState[8]; // local push-buttons state saved here
volatile    byte pos;

// Setup Button Matrix
int     rowPin[] = {3, 4, 5, 6, 7}; // Set pins for rows
int     colPin[] = {8, 9, 11}; // Set pins for columns
int     rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int     colSize = sizeof(colPin)/sizeof(colPin[0]);

// Button Matrix
//      Cols  |  0              1               2
// Rows Pins  |  8              9               11
// -----------------------------------------------------------
// 0    3     |  74 Triangle    87 Circle       116 Up
// 1    4     |  86 Square      100 Cross       131 Down
// 2    5     |  99 L1          114 R1          147 left
// 3    6     |  113 L2         129 R2          164 Right
// 4    7     |  128 Share      145 Options     182 PS

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
  encoderValue = 0;
}

// Setup Encoders
CommonBusEncoders encoders(A0, A1, A2, 3); // (Pin A Bus, Pin B Bus, Button Bus, Number of encoders)

void setup(){
  
  resetVars();
  
  Serial.begin(250000);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1
  
  // Interrupt for SS rising edge
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING); // Interrupt for Button Matrix
  
  #if DEBUG
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
    #if DEBUG
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
    #if DEBUG
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
  // Initiate Encoders
  encoderValue = encoders.readAll();

  //
  // Set the encoder action based on the code
  switch (encoderValue) {

    case 700: // Encoder 1 CW "TC+"
      wheelState[2] = wheelState[2] & B11111011;
      #if DEBUG
        Serial.print("Encoder: TC+ ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 701: // Encoder 1 CCW "TC-"
      wheelState[2] = wheelState[2] & B11110111;
      #if DEBUG
        Serial.print("Encoder: TC- ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 799: // Encoder 1 Button
      #if DEBUG
        Serial.print("Encoder: TC Button ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

    case 800: // Encoder 2 CW "BB+"
      wheelState[2] = wheelState[2] & B11101111;
      #if DEBUG
        Serial.print("Encoder: BB+ ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 801: // Encoder 2 CCW "BB-"
      wheelState[2] = wheelState[2] & B11111101;
      #if DEBUG
        Serial.print("Encoder: BB- ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 899: // Encoder 2 Button
      #if DEBUG
        Serial.print("Encoder: BB Button ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

    case 900: // Encoder 3 CW "R3 ABS+"
      wheelState[1] = wheelState[1] & B01111111;
      #if DEBUG
        Serial.print("Encoder: ABS+ ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 901: // Encoder 3 CCW "L3 ABS-"
      wheelState[1] = wheelState[1] & B10111111;
      #if DEBUG
        Serial.print("Encoder: ABS- ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;

     case 999: // Encoder 3 Button
      #if DEBUG
        Serial.print("Encoder: ABS Button ("); Serial.print(encoderValue); Serial.println(") ");
      #endif
      break;
      
  }

  //
  // Initiate Button Matrix
  scanButtonMatrix();

  //
  // Set the button action based on the keyValue
  switch (buttonValue) {
  
    case 74: // Triangle
      wheelState[0] = wheelState[0] & B10111111;
      #if DEBUG
        Serial.print("Button: Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 86: // Square
      wheelState[0] = wheelState[0] & B01111111;
      #if DEBUG
        Serial.print("Button: Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 99: // L1
      wheelState[0] = wheelState[0] & B11101111;
      #if DEBUG
        Serial.print("Button: L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 113: // L2
      wheelState[1] = wheelState[1] & B11011111;
      #if DEBUG
        Serial.print("Button: L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 128: // Share
      wheelState[1] = wheelState[1] & B11111011;
      #if DEBUG
        Serial.print("Button: Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

      
    case 87: // Circle
      wheelState[0] = wheelState[0] & B11111110;
      #if DEBUG
        Serial.print("Button: Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
      
    case 100: // Cross
      wheelState[1] = wheelState[1] & B11111101;
      #if DEBUG
        Serial.print("Button: Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 114: // R1
      wheelState[0] = wheelState[0] & B11011111;
      #if DEBUG
        Serial.print("Button: R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 129: // R2
      wheelState[1] = wheelState[1] & B11101111;
      #if DEBUG
        Serial.print("Button: R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 145: // Options
      wheelState[1] = wheelState[1] & B11110111;
      #if DEBUG
        Serial.print("Button: Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    
    case 116: // Up
      wheelState[2] = wheelState[2] & B11101111;
      #if DEBUG
        Serial.print("Button: Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 131: // Down
      wheelState[2] = wheelState[2] & B11111101;
      #if DEBUG
        Serial.print("Button: Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 147: // Left
      wheelState[2] = wheelState[2] & B11110111;
      #if DEBUG
        Serial.print("Button: Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 164: // Right
      wheelState[2] = wheelState[2] & B11111011;
      #if DEBUG
        Serial.print("Button: Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
    
    case 182: // PS
      wheelState[1] = wheelState[1] & B11111110;
      #if DEBUG
        Serial.print("Button: PS ("); Serial.print(buttonValue); Serial.println(") ");
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

  // Set a delay and reset the keyValue to something that will never match an exisitng keyValue
  delay(debounce);
  resetVars();
  
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
