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

//
// the default wheel state
void resetWheelState() {
  // Default Wheel State for the Thrustmaster F1
  wheelState[0] = B11001111;
  wheelState[1] = B11111111;
  wheelState[2] = B11111111;
  wheelState[3] = B11111111;
  wheelState[4] = B00000000;
  wheelState[5] = B00000000;
  wheelState[6] = B01100000;
  wheelState[7] = B01000000;
  buttonValue = 0;
}

//
// The encoder's buttonValue. These will trigger the appropriate actions.
void encoderBB_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 1100;
}
void encoderBB_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 1000;
}
void encoderABS_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 3000;
}
void encoderABS_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 3100;
}
void encoderTC_decrement(i2cEncoderMiniLib* obj) {
  buttonValue = 2000;
}
void encoderTC_increment(i2cEncoderMiniLib* obj) {
  buttonValue = 2100;
}

//
// We scan the Button matrix
void scanButtonMatrix() {
  for (int rowCounter=0; rowCounter<rowSize; rowCounter++) {
    // Set the active rowPin to LOW
    digitalWrite(rowPin[rowCounter], LOW);
    // Then we scan all cols in the col[] array
    for (int colCounter=0; colCounter<colSize; colCounter++) {
      // Scan each pin in column and if it's LOW
      if(digitalRead(colPin[colCounter]) == LOW){
        // Using a Cantor Pairing function we create unique number
        buttonValue = (((rowPin[rowCounter]+colPin[colCounter]) * (rowPin[rowCounter]+colPin[colCounter]+1)) / 2 + colPin[colCounter]);
      }
    }
    // Set the active rowPin back to HIGH
    digitalWrite(rowPin[rowCounter], HIGH);
  }
}

//
// Used for updating the display
void printDisplay(String line_1="", int pos_1=0, String line_2="", int pos_2=0) {
  if (prev_line_1 != line_1 || prev_line_2 != line_2) {
    lcd.clear();
    lcd.setCursor(pos_1, 0);
    lcd.print(line_1);
    lcd.setCursor(pos_2, 1);
    lcd.print(line_2);
    prev_line_1 = line_1;
    prev_line_2 = line_2;
    lastDisplayTime = millis();
  }
}

//
// What menu to display?
void showMenu() {
  if (menu == 1) { // Main Menu
    maxPages = 7;
    switch (menuPage) {
      case 1:
        printDisplay(SELECT_OPTION, 1,
                     ON_ICON+DISPLAY_MODE_MENU + ((DISPLAY_MODE) ? MODE_PS : MODE_GT), 0);
        break;
      case 2:
        printDisplay(DISPLAY_MODE_MENU + ((DISPLAY_MODE) ? MODE_PS : MODE_GT), 1,
                     ON_ICON+DISPLAY_KEYPRESS_MENU + ((DISPLAY_KEYS) ? ON : OFF), 0);
        break;
      case 3:
        printDisplay(DISPLAY_KEYPRESS_MENU + ((DISPLAY_KEYS) ? ON : OFF), 1, 
                     ON_ICON+DISPLAY_CLOCK_MENU + ((CLOCK_STATUS) ? ON : OFF), 0);
        break;
      case 4:
        printDisplay(DISPLAY_CLOCK_MENU + ((CLOCK_STATUS) ? ON : OFF), 1,
                     ON_ICON+BUZZER_STATUS_MENU + ((BUZZER_STATUS) ? ON : OFF), 0);
        break;
      case 5:
        printDisplay(BUZZER_STATUS_MENU + ((BUZZER_STATUS) ? ON : OFF), 1, 
                     ON_ICON+HOUR_CHIRP_MENU + ((HOUR_CHIRP) ? ON : OFF), 0);
        break;
      case 6:
        printDisplay(HOUR_CHIRP_MENU + ((HOUR_CHIRP) ? ON : OFF), 1, 
                     ON_ICON+DISPLAY_STATUS_MENU, 0);
        break;
      case 7:
        printDisplay(DISPLAY_STATUS_MENU, 1, 
                     ON_ICON+DISPLAY_RUNTIME_MENU, 0);
        break;
    }
  } else {
    if (CLOCK_STATUS) {
      getDateTime();
      printDisplay(getDate + "      " + getTime, 0, MENU, 0);
    } else {
      printDisplay("", 0, MENU, 0);
    }
  }
};

//
// Nexgt page on the Display menu
void displayNext() {
  (menuPage >= maxPages) ? menuPage = 1 : menuPage++;
  delay(DEBOUNCE);
  showMenu();
}

//
// Actions for the Display menu
void displaySelect() {
  if (menu == 1 && menuPage == 1) { // Display Mode
    toggleDisplayMode();
  }
  if (menu == 1 && menuPage == 2) { // Display Keypress
    toggleDisplayKeypress();
  }
  if (menu == 1 && menuPage == 3) { // Display Clock
    toggleClockStatus();
  }
  if (menu == 1 && menuPage == 4) { // Buzzer Status
    toggleBuzzerStatus();
  }
  if (menu == 1 && menuPage == 5) { // Hour Chirp
    toggleHourChirp();
  }
  if (menu == 1 && menuPage == 6) { // Display Off
    turnDisplayOff();
  }
  if (menu == 1 && menuPage == 7) { // Display Runtime
    displayRuntime();
  }
}

//
// Toggle Display Mode: Show Playstation or Wheel button labels
void toggleDisplayMode() {
  if (DISPLAY_MODE == 1) {
    DISPLAY_MODE = 0;
  } else {
    DISPLAY_MODE = 1;
  }
  EEPROM.write(0, DISPLAY_MODE);
  delay(DEBOUNCE);
  showMenu();
}

//
// Toggle Display Keypress: Show or hide the label on each keypress
void toggleDisplayKeypress() {
  if (DISPLAY_KEYS == 1) {
    DISPLAY_KEYS = 0;
  } else {
    DISPLAY_KEYS = 1;
  }
  EEPROM.write(1, DISPLAY_KEYS);
  delay(DEBOUNCE);
  showMenu();
}

//
// Toggle the buzzzer
void toggleBuzzerStatus() {
  if (BUZZER_STATUS == 1) {
    BUZZER_STATUS = 0;
  } else {
    BUZZER_STATUS = 1;
    buzzer();
  }
  EEPROM.write(2, BUZZER_STATUS);
  delay(DEBOUNCE);
  showMenu();
}

//
// Toggle the Hour Chirp
void toggleHourChirp() {
  if (HOUR_CHIRP == 1) {
    HOUR_CHIRP = 0;
  } else {
    HOUR_CHIRP = 1;
  }
  EEPROM.write(4, HOUR_CHIRP);
  delay(DEBOUNCE);
  showMenu();
}

//
// Turn the Display off 
void turnDisplayOff() {
  printDisplay(DISPLAY_STATUS_MENU, 2);
  delay(MESSAGE_DURATION);
  lcd.noDisplay();
  lcd.noBacklight();
  lcd.clear();
  DISPLAY_STATUS = 0;
  EEPROM.write(3, DISPLAY_STATUS);
  menuPage = 1;
  menu = 0;
}

//
// Toggle the buzzzer
void toggleClockStatus() {
  if (CLOCK_STATUS == 1) {
    CLOCK_STATUS = 0;
  } else {
    CLOCK_STATUS = 1;
  }
  EEPROM.write(5, CLOCK_STATUS);
  delay(DEBOUNCE);
  showMenu();
}

//
// Calculate and show the time since the wheel booted
void displayRuntime() {
  
  lcd.clear();

  unsigned long allSeconds=millis()/1000;
  int runHours= allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;

  char runtime_buf [10];
  sprintf(runtime_buf, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
  String showRuntime = String(runtime_buf);
  printDisplay(showRuntime, 4);
  
  delay(MESSAGE_DURATION);
  showMenu();
}

//
// depending on the voltage, the actual values we receive from
// the Analog pins on the Rotary Switches are different. So, depending
// where we're connected, we adapt the values.
//
// USB: 5V - T300:3.3V
#if Rotary_Switch_T300
  //
  // Change the following values for the Thrustmaster T300 base
  int getCABMode() {
    CAB_ACTION = analogRead(6);
    if (CAB_ACTION >= 770 && CAB_ACTION <= 820) {
      triggerCAB = 0; // BB Positin
    } else if (CAB_ACTION >= 640 && CAB_ACTION <= 690) {
      triggerCAB = 1; // ABS Position
    } else if (CAB_ACTION >= 510 && CAB_ACTION <= 550) {
      triggerCAB = 2; // TC1 Position
    }
    return triggerCAB;
  }
  int getCABSteps() {
    CAB_STEPS = analogRead(7);
    if (CAB_STEPS >= 510 && CAB_STEPS <= 550) {
      triggerSteps = 1; // 1x Position
    } else if (CAB_STEPS >= 390 && CAB_STEPS <= 420) {
      triggerSteps = 2; // 2x Position
    } else if (CAB_STEPS >= 240 && CAB_STEPS <= 280) {
      triggerSteps = 3; // 3x Position
    } else if (CAB_STEPS >= 110 && CAB_STEPS <= 150) {
      triggerSteps = 4; // 4x Position
    } else if (CAB_STEPS >= 0 && CAB_STEPS <= 60) {
      triggerSteps = 5; // 5x Position
    } else if (CAB_STEPS >= 910 && CAB_STEPS <= 950) {
      triggerSteps = 6; // 6x Position
    } else if (CAB_STEPS >= 770 && CAB_STEPS <= 820) {
      triggerSteps = 7; // 7x Position
    } else if (CAB_STEPS >= 640 && CAB_STEPS <= 690) {
      triggerSteps = 8; // 8x Position
    }
    return triggerSteps;
  }
#else
  int getCABMode() {
    CAB_ACTION = analogRead(6);
    if (CAB_ACTION >= 580 && CAB_ACTION <= 620) {
      triggerCAB = 0; // BB Position
    } else if (CAB_ACTION >= 480 && CAB_ACTION <= 520) {
      triggerCAB = 1; // ABS Position
    } else if (CAB_ACTION >= 380 && CAB_ACTION <= 420) {
      triggerCAB = 2; // TC1 Position
    }
    return triggerCAB;
  }
  int getCABSteps() {
    CAB_STEPS = analogRead(7);
    if (CAB_STEPS >= 380 && CAB_STEPS <= 420) {
      triggerSteps = 1; // 1x Position
    } else if (CAB_STEPS >= 280 && CAB_STEPS <= 320) {
      triggerSteps = 2; // 2x Position
    } else if (CAB_STEPS >= 180 && CAB_STEPS <= 220) {
      triggerSteps = 3; // 3x Position
    } else if (CAB_STEPS >= 80 && CAB_STEPS <= 120) {
      triggerSteps = 4; // 4x Position
    } else if (CAB_STEPS >= 00 && CAB_STEPS <= 40) {
      triggerSteps = 5; // 5x Position
    } else if (CAB_STEPS >= 680 && CAB_STEPS <= 720) {
      triggerSteps = 6; // 6x Position
    } else if (CAB_STEPS >= 580 && CAB_STEPS <= 620) {
      triggerSteps = 7; // 7x Position
    } else if (CAB_STEPS >= 480 && CAB_STEPS <= 520) {
      triggerSteps = 8; // 8x Position
    }
    return triggerSteps;
  }
#endif

//
// Beep-beep!
void buzzer() {
  if (BUZZER_STATUS) {
    tone(BUZZER_PIN, 2500, 50);
    buzzerTime = millis();
  }
}

//
// Startup tune
void buzzerStartup() {
  tone(BUZZER_PIN, 1500, 80);
  delay(100);
  tone(BUZZER_PIN, 1800, 80);
  delay(100);
  tone(BUZZER_PIN, 2400, 80);
  delay(100);
  tone(BUZZER_PIN, 3500, 80);
}

//
// Hour Chirp tune
void buzzerHour() {
  tone(BUZZER_PIN, 3000, 80);
  delay(200);
  tone(BUZZER_PIN, 3000, 80);
}

//
// Get Date & Time and place them in strings to use
void getDateTime() {

  if (RTC.read(tm)) {
    char getDate_buf [10];
    char getTime_buf [10];
    sprintf(getDate_buf, "%02d/%02d", tm.Day, tm.Month);
    sprintf(getTime_buf, "%02d:%02d", tm.Hour, tm.Minute);
    getDate = String(getDate_buf);
    getTime = String(getTime_buf);
  } else {
    getDate = "Clock error!";
  }
 
}
