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
// Reset the Menu
void resetMenu() {
  menuPage = 1;
  menu = 0;
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
    maxPages = 6;
    switch (menuPage) {
      case 1:
        printDisplay(SELECT_OPTION, 0, DISPLAY_MODE_ON, 0);
        break;
      case 2:
        printDisplay(DISPLAY_MODE_OFF, 0, DISPLAY_KEYPRESS_ON, 0);
        break;
      case 3:
        printDisplay(DISPLAY_KEYPRESS_OFF, 0, DISPLAY_STATUS_ON, 0);
        break;
      case 4:
        printDisplay(DISPLAY_STATUS_OFF, 0, BUZZER_STATUS_ON, 0);
        break;
      case 5:
        printDisplay(BUZZER_STATUS_OFF, 0, HOUR_CHIRP_ON, 0);
        break;
      case 6:
        printDisplay(HOUR_CHIRP_OFF, 0, DISPLAY_RUNTIME_ON, 0);
        break;
    }
    delay(DEBOUNCE);
  } else {
    getDateTime();
    printDisplay(getDate + "      " + getTime, 0, MENU, 0);
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
  if (menu == 1 && menuPage == 3) { // Display Off
    toggleBacklight();
  }
  if (menu == 1 && menuPage == 4) { // Buzzer Status
    toggleBuzzerStatus();
  }
  if (menu == 1 && menuPage == 5) { // Hour Chirp
    toggleHourChirp();
  }
  if (menu == 1 && menuPage == 6) { // Display Runtime
    displayRuntime();
  }
}

//
// Toggle Display Mode: Show Playstation or Wheel button labels
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

//
// Toggle Display Keypress: Show or hide the label on each keypress
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

//
// Turn the Display off 
void toggleBacklight() {
  printDisplay(DISPLAY_STATUS_OFF, 2);
  delay(MESSAGE_DURATION);
  lcd.noDisplay();
  lcd.noBacklight();
  lcd.clear();
  DISPLAY_STATUS = 0;
  EEPROM.write(3, DISPLAY_STATUS);
  resetMenu();
}

//
// Toggle the buzzzer
void toggleBuzzerStatus() {
  if (BUZZER_STATUS == 1) {
    BUZZER_STATUS = 0;
    printDisplay(BUZZER_OFF, 2);
  } else {
    BUZZER_STATUS = 1;
    printDisplay(BUZZER_ON, 3);
    buzzer();
  }
  EEPROM.write(2, BUZZER_STATUS);
  delay(MESSAGE_DURATION);
  showMenu();
}

//
// Toggle the Hour Chirp
void toggleHourChirp() {
  if (HOUR_CHIRP == 1) {
    HOUR_CHIRP = 0;
    printDisplay(CHIRP_OFF);
  } else {
    HOUR_CHIRP = 1;
    printDisplay(CHIRP_ON);
  }
  EEPROM.write(4, HOUR_CHIRP);
  delay(MESSAGE_DURATION);
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
  int getCABAction() {
    CAB_ACTION = analogRead(6);
    if (CAB_ACTION >= 770 && CAB_ACTION <= 820) {
      triggerCAB = 0;
    } else if (CAB_ACTION >= 640 && CAB_ACTION <= 690) {
      triggerCAB = 1;
    } else if (CAB_ACTION >= 510 && CAB_ACTION <= 550) {
      triggerCAB = 2;
    }
    return triggerCAB;
  }
  int getCABSteps() {
    CAB_STEPS = analogRead(7);
    if (CAB_STEPS >= 510 && CAB_STEPS <= 550) {
      triggerSteps = 1;
    } else if (CAB_STEPS >= 390 && CAB_STEPS <= 420) {
      triggerSteps = 2;
    } else if (CAB_STEPS >= 240 && CAB_STEPS <= 280) {
      triggerSteps = 3;
    } else if (CAB_STEPS >= 110 && CAB_STEPS <= 150) {
      triggerSteps = 4;
    } else if (CAB_STEPS >= 0 && CAB_STEPS <= 60) {
      triggerSteps = 5;
    } else if (CAB_STEPS >= 910 && CAB_STEPS <= 950) {
      triggerSteps = 6;
    } else if (CAB_STEPS >= 770 && CAB_STEPS <= 820) {
      triggerSteps = 7;
    } else if (CAB_STEPS >= 640 && CAB_STEPS <= 690) {
      triggerSteps = 8;
    }
    return triggerSteps;
  }
#else
  int getCABAction() {
    CAB_ACTION = analogRead(6);
    if (CAB_ACTION >= 580 && CAB_ACTION <= 620) {
      triggerCAB = 0;
    } else if (CAB_ACTION >= 480 && CAB_ACTION <= 520) {
      triggerCAB = 1;
    } else if (CAB_ACTION >= 380 && CAB_ACTION <= 420) {
      triggerCAB = 2;
    }
    return triggerCAB;
  }
  int getCABSteps() {
    CAB_STEPS = analogRead(7);
    if (CAB_STEPS >= 380 && CAB_STEPS <= 420) {
      triggerSteps = 1;
    } else if (CAB_STEPS >= 280 && CAB_STEPS <= 320) {
      triggerSteps = 2;
    } else if (CAB_STEPS >= 180 && CAB_STEPS <= 220) {
      triggerSteps = 3;
    } else if (CAB_STEPS >= 80 && CAB_STEPS <= 120) {
      triggerSteps = 4;
    } else if (CAB_STEPS >= 00 && CAB_STEPS <= 40) {
      triggerSteps = 5;
    } else if (CAB_STEPS >= 680 && CAB_STEPS <= 720) {
      triggerSteps = 6;
    } else if (CAB_STEPS >= 580 && CAB_STEPS <= 620) {
      triggerSteps = 7;
    } else if (CAB_STEPS >= 480 && CAB_STEPS <= 520) {
      triggerSteps = 8;
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
