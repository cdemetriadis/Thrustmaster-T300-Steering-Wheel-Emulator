// Connect to Thrustmaster T300
//
// Arduino GND                        -> T300 Blue wire (2)
// Arduino pin 12                     -> T300 White wire (3) (data from wheel to base)
// Arduino pin 10 + pin 2 (SS)        -> T300 Orange wire pin (4) (yes, on Arduino it's wired to two pins. 10 - SS, 2 - INT0)
// Arduino pin 13 (SCK)               -> T300 Red wire (5)
// Arduino +5V                        -> T300 Black wire (6) (it gives 3.3V, but must be connected into +5V socket on arduino uno side)

#include      <Wire.h>                                // Wire lib used for I2C Encoders
#include      <i2cEncoderMiniLib.h>                   // Load I2C Encoder Library

#define       DEBUG_SETUP false                       // Debug Setup information
#define       DEBUG_KEYS false                        // Debug the button presses
#define       DEBUG_WHEEL false                       // Debug wheel output

#define       CAB_ACTION 0                            // Set the CAB action. 0 for BB, 1 for TC, 2 for ABS
#define       CAB_STEPS 3                             // Set the CAB Steps. 

// CAB Setup - Combined action buttons
int           triggerCAB;
int           triggerStepsIncrease;
int           triggerStepsDecrease;


// Initialize Encoders
const int     IntPin = 3;
i2cEncoderMiniLib encoderBB(0x20);
//i2cEncoderMiniLib encoderTC(0x20); <-- SETUP ADDRESS
//i2cEncoderMiniLib encoderABS(0x20); <-- SETUP ADDRESS


// Button Matrix
int           foundColumn = 0;
int           buttonValue = 0;
int           debounce = 100;                           // Set this to the lowest value that gives the best result
byte          wheelState[8];
volatile      byte pos;

// Setup Button Matrix
int           rowPin[] = {5, 6, 7, 8, 9};            // Set pins for rows > OUTPUT
int           colPin[] = {A0, A1, A2, A3, 111};              // Set pins for columns, could also use Analog pins > INPUT_PULLUP
int           rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int           colSize = sizeof(colPin)/sizeof(colPin[0]);


// Button Matrix
//      Cols  |  0              1               2               4               5
// Rows Pins  |  14/A0          15/A1           16/A2           17/A3           11
// ---------------------------------------------------------------------------------------------
// 0    5     |  204 Triangle   225 Circle      247 Up          270 L1          147 
// 1    6     |  224 Square     246 Cross       269 Down        293 R1          164 Share
// 2    7     |  245 L2         268 R2          292 left        317 CAB-L
// 3    8     |  267            291 Options     316 Right       342 CAB-R
// 4    9     |  290            315 PS          341 Confirm     368

void setup() {

  resetVars();

  #if DEBUG_SETUP || DEBUG_KEYS || DEBUG_WHEELS
    Serial.begin(115200);    // Arduino debug console
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


int CABActionGuide[3][2][2] = {
  { // BB
    {3, B10111111},   // D-Pad Down - wheelState[3] = wheelState[3] & B10111111;
    {3, B11110111}    // D-Pad Up - wheelState[3] = wheelState[3] & B11110111;
  },
  { // TC
    {3, B11011111},   // D-Pad Left - wheelState[3] = wheelState[3] & B11011111;
    {3, B11101111}    // D-Pad Right - wheelState[3] = wheelState[3] & B11101111;
  },
  { // ABS
    {1, B11111101},  // L3 - wheelState[1] = wheelState[1] & B11111101;
    {1, B11111110}   // R3 - wheelState[1] = wheelState[1] & B11111110;
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


    case 204: // Triangle
      wheelState[0] = wheelState[0] & B11111101;
      #if DEBUG_KEYS
        Serial.print("Triangle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 224: // Square
      wheelState[0] = wheelState[0] & B11111110;
      #if DEBUG_KEYS
        Serial.print("Square ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 245: // L2
      wheelState[1] = wheelState[1] & B11111011;
      #if DEBUG_KEYS
        Serial.print("L2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    case 225: // Circle
      wheelState[0] = wheelState[0] & B01111111;
      #if DEBUG_KEYS
        Serial.print("Circle ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 246: // Cross
      wheelState[1] = wheelState[1] & B10111111;
      #if DEBUG_KEYS
        Serial.print("Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 268: // R2
      wheelState[1] = wheelState[1] & B11110111;
      #if DEBUG_KEYS
        Serial.print("R2 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 291: // Options
      wheelState[1] = wheelState[1] & B11101111;
      #if DEBUG_KEYS
        Serial.print("Options ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 315: // Playstation
      wheelState[1] = wheelState[1] & B01111111;
      #if DEBUG_KEYS
        Serial.print("Playstation ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 164: // Share
      wheelState[1] = wheelState[1] & B11011111;
      #if DEBUG_KEYS
        Serial.print("Share ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;




    case 247: // D-Pad Up
      wheelState[2] = wheelState[2] & B11110111; // DP-Up
      #if DEBUG_KEYS
        Serial.print("D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 269: // D-Pad Down
      wheelState[2] = wheelState[2] & B10111111; // DP-Down
      #if DEBUG_KEYS
        Serial.print("D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 292: // D-Pad Left
      wheelState[2] = wheelState[2] & B11101111; // DP-Left
      #if DEBUG_KEYS
        Serial.print("D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 316: // D-Pad Right
      wheelState[2] = wheelState[2] & B11011111; // DP-Right
      #if DEBUG_KEYS
        Serial.print("D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 341: // D-Pad Confirm
      wheelState[1] = wheelState[1] & B10111111; // Cross
      #if DEBUG_KEYS
        Serial.print("Cross ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;




    case 270: // L1
      wheelState[0] = wheelState[0] & B11110111;
      #if DEBUG_KEYS
        Serial.print("L1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 293: // R1
      wheelState[0] = wheelState[0] & B11111011;
      #if DEBUG_KEYS
        Serial.print("R1 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 317: // 290 CAB-L Combined Action Button Left

      triggerCAB = CAB_ACTION; // 0 for BB, 1 for TC, 2 for ABS
      triggerStepsDecrease = triggerStepsDecrease+CAB_STEPS; // 4 for x5

      #if DEBUG_KEYS
        Serial.print("CAB-L ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 342: // 315 CAB-R Combined Action Button Right

      triggerCAB = CAB_ACTION; // 0 for BB, 1 for TC, 2 for ABS
      triggerStepsIncrease = triggerStepsIncrease+CAB_STEPS; // 4 for x5

      #if DEBUG_KEYS
        Serial.print("CAB-R ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;



    //
    // Encoder Functions

    case 1000: // D-Pad Up
      wheelState[3] = wheelState[3] & B11110111; // CHRG+
      #if DEBUG_KEYS
        Serial.print("D-Pad Up ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 1100: // D-Pad Down
      wheelState[3] = wheelState[3] & B10111111; // CHRG-
      #if DEBUG_KEYS
        Serial.print("D-Pad Down ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 2000: // D-Pad Left
      wheelState[3] = wheelState[3] & B11011111; // DIF IN+
      #if DEBUG_KEYS
        Serial.print("D-Pad Left ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 2100: // D-Pad Right
      wheelState[3] = wheelState[3] & B11101111; // DIF IN-
      #if DEBUG_KEYS
        Serial.print("D-Pad Right ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 3000: // L3
      wheelState[1] = wheelState[1] & B11111101; // Pump
      #if DEBUG_KEYS
        Serial.print("L3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;

    case 3100: // R3
      wheelState[1] = wheelState[1] & B11111110; // 1-
      #if DEBUG_KEYS
        Serial.print("R3 ("); Serial.print(buttonValue); Serial.println(") ");
      #endif
      break;
   

    default: // Reset if nothing is pressed
      break;
    
   
  }
  
  
  // CAB Trigger 
  if (triggerStepsIncrease > 0) {
    wheelState[CABActionGuide[CAB_ACTION][1][0]] = wheelState[CABActionGuide[CAB_ACTION][1][0]] & CABActionGuide[CAB_ACTION][1][1];
    triggerStepsIncrease--;
  }
  if (triggerStepsDecrease > 0) {
    wheelState[CABActionGuide[CAB_ACTION][0][0]] = wheelState[CABActionGuide[CAB_ACTION][0][0]] & CABActionGuide[CAB_ACTION][0][1];
    triggerStepsDecrease--;
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


void encoderBB_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 1100;
}
void encoderBB_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 1000;
}
void encoderTC_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 2000;
}
void encoderTC_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 2100;
}
void encoderABS_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 3000;
}
void encoderABS_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 3100;
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
