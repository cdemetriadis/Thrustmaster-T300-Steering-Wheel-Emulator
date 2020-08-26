/*
Author: Danny van den Brande, Arduinosensors.nl. BlueCore Tech.
In this example code i gave all the buttons a function, Because i
can not find any projects with this keypad, i made a example.
If you make a project with it such as a alarm or whatever.
Please share it with me @ contact@arduinosensor.nl.
With this code way we know which value is assigned to which key,
you can also see the values in the serial monitor.
you can modify those parts as you wish to give it other functions.
*/

  #define DEBUG true

  int rowCounter = 0; // Row counter
  int colCounter = 0; // Column counter
  int foundColumn = 0; // Active column
  boolean foundCol = false;
  int keyValue = 0;
  int noKey = 0;
  boolean readKey = false;
  int debounce = 1000; // set this to the lowest value that gives the best result
  volatile byte pos;
  byte wheelState[8]; // local push-buttons state saved here

  // Button Grid
  int row[] = {3, 4, 5, 6, 7, 8}; // Set pins for rows
  int col[] = {A0, A1, A2, A3}; // Set pins for columns
  int rowSize = sizeof(row)/sizeof(row[0]);
  int colSize = sizeof(col)/sizeof(col[0]);

void setup(){
  
  wheelState[0] = B11001111; // F1 wheel specific, and 5 Button
  wheelState[1] = B11111111; // 8 Buttons
  wheelState[2] = B11111111; // 8 Buttons
  wheelState[3] = B11111111; // DIFF and CHRG flags
  wheelState[4] = B00000000; // DIFF steps
  wheelState[5] = B00000000; // CHRG steps
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  
  Serial.begin(9600);    // Arduino debug console
  pinMode(MISO, OUTPUT); // Arduino is a slave device
  SPCR |= _BV(SPE);      // Enables the SPI when 1
  SPCR |= _BV(SPIE);     // Enables the SPI interrupt when 1
  
  // Interrupt for SS rising edge.
  attachInterrupt (0, ss_rising, RISING);
  
  #if DEBUG
    Serial.println("Thrustmaster Custom Wheel Emulator v1.0");
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
  // Maybe move to DDR & PORT?
  // PORT B: 8-13
  // PORT C: A0-A7
  // Port D: 0-7
  for (int r = 0; r <= rowSize-1; r++) {
    pinMode(row[r], OUTPUT);
  
    if (row[r] >= 8 && row[r] <= 13) { // PORT B
      DDRB  &= B11110000;
      PORTB |= B00001011;
    } else if (row[r] >= 0 && row[r] <= 7) { // PORT D
      DDRD  &= B11000000;
      PORTD |= B00000011;
    } else if (row[r] >= 14 && row[r] <= 20) { // PORT C
      DDRC  &= B00100000;
      PORTC |= B11011011;
    }

    
    #if DEBUG
      Serial.print("Pin: ");
      Serial.print(row[r]);
      Serial.print(" is set as OUTPUT on row: ");
      Serial.println(r);
    #endif
  }
  
  // Column setup
  //
  // Set columns as INPUT_PULLUP
  for (int c = 0; c <= colSize-1; c++) {
    pinMode(col[c], INPUT_PULLUP);
    #if DEBUG
      Serial.print("Pin: ");
      Serial.print(col[c]);
      Serial.print(" is set as INPUT_PULLUP on col: ");
      Serial.println(c);
    #endif
  }

}

void loop(){

  // Check to see if any keys are pressed
  //
  // noKey is set in the readColumn() function.
  // It increments 1 for every button that is not pressed.
  // If no keys are pressed, then it sets readKey to true.
  if(noKey == (rowSize * colSize)) { 
    readKey = true;  // Keyboard is ready to accept a new keypress
  }
  noKey = 0; // Reset noKey value

  // Search for a key press
  //
  // First we scan all rows in the row[] array
  for (rowCounter=0; rowCounter<=rowSize-1; rowCounter++) {
    scanRow(); // switch on one row at a time

    // Then we scan all cols in the col[] array
    for (colCounter=0; colCounter<=colSize-1; colCounter++) {
      readColumn(); // read the switch pressed
      if (foundCol == true){
        keyValue = (row[rowCounter] * col[colCounter]); // We set a keyValue, unique to each key press
      }
    }
  }


  // Execute on the key press
  //
  // If we're ready to read a key and a key has been pressed
  if (readKey == true && noKey == (rowSize * colSize - 1)) {

    // Do things here
    if (keyValue == 42) { // Triangle
        wheelState[0] = wheelState[0] & B10111111;
    }
    
    if (keyValue == 45) { // Square
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("Square");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 48) { // Cross
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("Cross");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 51) { // Circle
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("Circle");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 56) { // L1
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("L1");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 60) { // L2
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("L2");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 64) { // R1
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("R1");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 68) { // R2
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("R2");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 70) { // Share
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("Share");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 75) { // PS
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("PS");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    if (keyValue == 80) { // Cross (5w)
        #if DEBUG
          Serial.print("Button: ");
          Serial.print("R1");
          Serial.print(" (");
          Serial.print(keyValue);
          Serial.println(")");
        #endif
    }
    
    // Reset all
    readKey = false; // Reset the flag
    delay(debounce); // Delay the next read
  
  }

  #if DEBUG
    Serial.println();
    for(int i = 0; i < 8; i++)
    {
      Serial.print(wheelState[i], BIN);
      Serial.print(" ");
    }
  #endif
  
}

void scanRow() {
  // Set all pins in row to HIGH
  for(int j = 0; j <= rowSize-1; j++){
    digitalWrite(row[j], HIGH);
  }
  // Set the active pin to LOW
  digitalWrite(row[rowCounter], LOW);
}

void readColumn() {
  // Scan each pin in column
  foundColumn = digitalRead(col[colCounter]);
  if(foundColumn == 0){
    foundCol = true; // Set founCol to true if we find an active pin
  } else {
    foundCol = false;
    noKey++; // Counter for number of empty columns
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
