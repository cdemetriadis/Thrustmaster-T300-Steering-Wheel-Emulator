#define     DEBUG true
#define     DEBUG_WHEEL false

int         foundColumn = 0; // Active column
int         keyValue = 0;
int         debounce = 300; // Set this to the lowest value that gives the best result
byte        wheelState[8]; // local push-buttons state saved here
volatile    byte pos;

// Setup Button Grid
int     rowPin[] = {4, 5, 6, 7, 8, 9}; // Set pins for rows
int     colPin[] = {A0, A1, A2, A3}; // Set pins for columns
int     rowSize = sizeof(rowPin)/sizeof(rowPin[0]);
int     colSize = sizeof(colPin)/sizeof(colPin[0]);

//      Cols  |  0              1               2             3
// Rows Pins  |  A0 (14)        A1 (15)         A2 (16)       A3 (17)
// -------------------------------------------------------------------------
// 0    4     |  185 Triangle   205 Square      226 Cross     248 Circle
// 1    5     |  204 L1         225 L2          247 R2        270 R1
// 2    6     |  224 Share      246 PS          269 Options   293 -
// 3    7     |  245 Up         268 Down        292 Left      317 Right
// 4    8     |  267 Enc Up     291 Enc Left    316 -         342 Enc L3
// 5    9     |  290 Enc Down   315 Enc Right   341 -         368 Enc R3

void setup(){
  
  wheelState[0] = B11001111; // F1 wheel specific, and 5 Button
  wheelState[1] = B11111111; // 8 Buttons
  wheelState[2] = B11111111; // 8 Buttons
  wheelState[3] = B11111111; // DIFF and CHRG flags
  wheelState[4] = B00000000; // DIFF steps
  wheelState[5] = B00000000; // CHRG steps
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  
  Serial.begin(115200);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1
  
  // Interrupt for SS rising edge.
  attachInterrupt (digitalPinToInterrupt(2), ss_rising, RISING);
  
  #if DEBUG
    Serial.println("Thrustmaster Custom F1 Wheel Emulator v1.0");
    Serial.println();
    Serial.print("Setup ");
    Serial.print(rowSize);
    Serial.print("x");
    Serial.print(colSize);
    Serial.println(" Button Grid");
    Serial.println();
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
  
  // Search for a key press
  //
  // scanButtonMatrix() returns a unique keyValue
  scanButtonMatrix();
  
  // Search for an encoder rotation
  //
  // scanEncoderMatrix() returns a unique keyValue for each rotation (left or right)
//  scanEncoderMatrix();

  //
  // Set the button action based on the keyValue
  switch (keyValue) {
  
    case 185: // Triangle
      wheelState[0] = wheelState[0] & B10111111;
      #if DEBUG
        Serial.print("Button: Triangle ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 205: // Square
      wheelState[0] = wheelState[0] & B01111111;
      #if DEBUG
        Serial.print("Button: Square ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 226: // Cross
      wheelState[1] = wheelState[1] & B11111101;
      #if DEBUG
        Serial.print("Button: Cross ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 248: // Circle
      wheelState[0] = wheelState[0] & B11111110;
      #if DEBUG
        Serial.print("Button: Circle ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 204: // L1
      wheelState[0] = wheelState[0] & B11101111;
      #if DEBUG
        Serial.print("Button: L1 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 225: // L2
      wheelState[1] = wheelState[1] & B11011111;
      #if DEBUG
        Serial.print("Button: L2 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 247: // R1
      wheelState[0] = wheelState[0] & B11011111;
      #if DEBUG
        Serial.print("Button: R1 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 270: // R2
      wheelState[1] = wheelState[1] & B11101111;
      #if DEBUG
        Serial.print("Button: R2 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 224: // Share
      wheelState[1] = wheelState[1] & B11111011;
      #if DEBUG
        Serial.print("Button: Share ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 246: // PS
      wheelState[1] = wheelState[1] & B11111110;
      #if DEBUG
        Serial.print("Button: PS ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 269: // Options
      wheelState[1] = wheelState[1] & B11110111;
      #if DEBUG
        Serial.print("Button: Options ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 245: // Up
      wheelState[2] = wheelState[2] & B11101111;
      #if DEBUG
        Serial.print("Button: Up ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 268: // Down
      wheelState[2] = wheelState[2] & B11111101;
      #if DEBUG
        Serial.print("Button: Down ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 292: // Left
      wheelState[2] = wheelState[2] & B11110111;
      #if DEBUG
        Serial.print("Button: Left ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 317: // Right
      wheelState[2] = wheelState[2] & B11111011;
      #if DEBUG
        Serial.print("Button: Right ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;


    
    case 267: // Encoder Up
      wheelState[2] = wheelState[2] & B11101111;
      #if DEBUG
        Serial.print("Encoder: Up ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 290: // Encoder Down
      wheelState[2] = wheelState[2] & B11111101;
      #if DEBUG
        Serial.print("Encoder: Down ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 291: // Encoder Left
      wheelState[2] = wheelState[2] & B11110111;
      #if DEBUG
        Serial.print("Encoder: Left ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 315: // Encoder Right
      wheelState[2] = wheelState[2] & B11111011;
      #if DEBUG
        Serial.print("Encoder: Right ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 342: // Encoder L3
      wheelState[1] = wheelState[1] & B10111111;
      #if DEBUG
        Serial.print("Button: L3 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
    
    case 368: // Encoder R3
      wheelState[1] = wheelState[1] & B01111111;
      #if DEBUG
        Serial.print("Button: R3 ("); Serial.print(keyValue); Serial.println(") ");
      #endif
      break;
        
    default: // Reset if nothing is pressed
      wheelState[0] = B11001111; // F1 wheel specific, and 5 Button
      wheelState[1] = B11111111; // 8 Buttons
      wheelState[2] = B11111111; // 8 Buttons
      wheelState[3] = B11111111; // DIFF and CHRG flags
      wheelState[4] = B00000000; // DIFF steps
      wheelState[5] = B00000000; // CHRG steps
      wheelState[6] = B01100000;
      wheelState[7] = B01000000;
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
  keyValue = 0;
  
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
      // If we find a COL:
      // We set a keyValue, unique to each key press rowPin x colPin
      if(foundColumn==0){
        //
        // Using a Cantor Pairing function we create unique numbers
        keyValue = ( ( (rowPin[rowCounter]+colPin[colCounter]) * (rowPin[rowCounter]+colPin[colCounter]+1) ) / 2 + colPin[colCounter]);
      }
    }
  }
}
