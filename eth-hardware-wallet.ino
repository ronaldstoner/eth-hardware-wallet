#include "bitcoinfuncs.h"
#include "menu.h"
#include "globals.h"

#define BUTTON_1 0
#define BUTTON_2 14

#define FAILED_PIN_TIMER 30   //Seconds to lock on failed PIN 

unsigned long failedPinTime = 0;


int pincode = 1234;
int userPincode = 0;
int digitPosition = 0;
int digits[4] = {random(10), random(10), random(10), random(10)}; // Initialize with random numbers

bool RECEIVE_SHOWN = false;

enum AppState {
  PIN, 
  MENU,
  SIGN_TX,
  RECEIVE_ADDRESS,
  MNEMONIC_DISPLAY,
};

enum ButtonPress {
  SINGLE_BUTTON_1,
  SINGLE_BUTTON_2,
  DOUBLE_BUTTON
};

void displayPinCode(int digits[4], int pos) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.println();
  printCentered("Enter PIN", 20, 12);
  tft.setTextSize(4);

  String pinDisplay = "";
  for (int i = 0; i < 4; i++) {
    if (i == pos) {
      pinDisplay += String(digits[i]);
    } else {
      pinDisplay += "-";
    }
    if (i < 3) pinDisplay += " ";
  }
  printCentered(pinDisplay, tft.height()/2-10, 24); // 24 pixels per character when text size is 4
  
  // Display lock message if failedPinTime is set
  if (failedPinTime > 0) {
    int remainingTime = FAILED_PIN_TIMER - ((millis() - failedPinTime) / 1000);
    if (remainingTime > 0) {
      tft.setTextColor(TFT_RED);
      tft.setTextSize(2);
      String lockMessage = "Bad PIN - Locked for " + String(remainingTime) + "s";
      printCentered(lockMessage, tft.height()-40, 12);
      //tft.setTextSize(2);
    } else {
      failedPinTime = 0;  // Reset failedPinTime
    }
  }
}

bool verifyPin() {
  userPincode = digits[0]*1000 + digits[1]*100 + digits[2]*10 + digits[3];
  return userPincode == pincode;
}

void handlePinState(AppState &state, ButtonPress buttonPress) {
  if (failedPinTime > 0 && ((millis() - failedPinTime) / 1000) < FAILED_PIN_TIMER) {
    // If timer is active, do not accept button input
    return;
  }
  switch (buttonPress) {
    case SINGLE_BUTTON_1:
      digits[digitPosition] = (digits[digitPosition] + 1) % 10;
      displayPinCode(digits, digitPosition);
      break;
    case SINGLE_BUTTON_2:
      digits[digitPosition] = (digits[digitPosition] - 1) % 10;
      if (digits[digitPosition] < 0) digits[digitPosition] = 9;
      displayPinCode(digits, digitPosition);
      break;
    case DOUBLE_BUTTON:
      digitPosition++;
      if (digitPosition >= 4) {
        if (verifyPin()) {
          state = MENU;
          drawMenu(SIGN);
        } else {
          digitPosition = 0;
          for (int i = 0; i < 4; i++) digits[i] = random(10); // Reset to random numbers
          failedPinTime = millis();  // Start the timer
        }
        displayPinCode(digits, digitPosition);
      }
      break;
  }
}

void handleMenuState(AppState &state, MenuOption &currentOption, ButtonPress buttonPress) {
  switch (buttonPress) {
    case SINGLE_BUTTON_1:
      currentOption = (MenuOption)((((int)currentOption - 1) + NUM_OPTIONS) % NUM_OPTIONS);
      drawMenu(currentOption);
      break;
    case SINGLE_BUTTON_2:
      currentOption = (MenuOption)(((int)currentOption + 1) % NUM_OPTIONS);
      drawMenu(currentOption);
      break;
    case DOUBLE_BUTTON:
      switch (currentOption) {
        case SIGN:
          state = SIGN_TX;
          break;
        case RECEIVE:
          state = RECEIVE_ADDRESS;
          break;
        case MNEMONIC:
          state = MNEMONIC_DISPLAY;
          break;
        case LOCK:
          state = PIN;
          digitPosition = 0;  // Reset PIN screen 
          // TODO: move PIN reset to after verification and remove userpin from memory for security 
          for (int i = 0; i < 4; i++) digits[i] = random(10); // Reset to random numbers
          displayPinCode(digits, digitPosition);
          break;
      }
      break;
  }
}

void handleReceiveAddressState(AppState &state, ButtonPress buttonPress) {
  // This will run only once when the state changes to RECEIVE ADDRESS
  if (RECEIVE_SHOWN == false) {
    tft.fillScreen(TFT_BLACK);
    printHD(SEED_PHRASE, SEED_PASS);
    RECEIVE_SHOWN = true;
  }

  if (RECEIVE_SHOWN == true) {
    switch (buttonPress) {
      case SINGLE_BUTTON_1:
        if (addressIndex > 0 and RECEIVE_SHOWN == true) {
          addressIndex--;
          tft.fillScreen(TFT_BLACK);
          printHD(SEED_PHRASE, SEED_PASS);
        }
        break;
      case SINGLE_BUTTON_2:
        if (RECEIVE_SHOWN == true) {
          addressIndex++;
          tft.fillScreen(TFT_BLACK);
          printHD(SEED_PHRASE, SEED_PASS);
        }
        break;
      case DOUBLE_BUTTON:
        if (RECEIVE_SHOWN == true) {
          state = MENU;
        }
        break;
      }
  }
}

void handleMnemonicDisplayState(AppState &state, ButtonPress buttonPress) {
  tft.setCursor(0,0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_ORANGE);
  tft.println("   Mnemonic Seed Phrase");
  tft.println();
  PrintWrapSeed(SEED_PHRASE, TFT_WHITE);
  state = MNEMONIC_DISPLAY;
  switch (buttonPress) {
    case SINGLE_BUTTON_1:
    case SINGLE_BUTTON_2:
    case DOUBLE_BUTTON:
      state = MENU;
      break;
  }
}

void processButtonPress(AppState &state, MenuOption &currentOption, ButtonPress buttonPress) {
  switch (state) {
    case PIN:
      handlePinState(state, buttonPress);
      break;
    case MENU:
      handleMenuState(state, currentOption, buttonPress);
      break;
    case SIGN_TX:
      // handle SIGN_TX state
      break;
    case RECEIVE_ADDRESS:
      handleReceiveAddressState(state, buttonPress);
      break;
    case MNEMONIC_DISPLAY:
      handleMnemonicDisplayState(state, buttonPress);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  displayPinCode(digits, digitPosition);
}

void loop() {
  static AppState state = PIN; // Start with PIN state
  static MenuOption currentOption = SIGN;
  static bool button1Prev = HIGH;
  static bool button2Prev = HIGH;
  static unsigned long lastUpdateTime = 0;

  bool button1 = digitalRead(BUTTON_1);
  bool button2 = digitalRead(BUTTON_2);

  if (button1 == LOW && button1Prev == HIGH) {
    delay(200);
    if (digitalRead(BUTTON_2) == LOW) {
      processButtonPress(state, currentOption, DOUBLE_BUTTON);
    } else {
      processButtonPress(state, currentOption, SINGLE_BUTTON_1);
    }
  } else if (button2 == LOW && button2Prev == HIGH) {
    delay(200);
    if (digitalRead(BUTTON_1) == LOW) {
      processButtonPress(state, currentOption, DOUBLE_BUTTON);
    } else {
      processButtonPress(state, currentOption, SINGLE_BUTTON_2);
    }
  }

  // Update the PIN display every second if failedPinTime is set
  if (failedPinTime > 0 && millis() - lastUpdateTime >= 1000) {
    lastUpdateTime = millis();
    displayPinCode(digits, digitPosition);
  }

  button1Prev = button1;
  button2Prev = button2;
}