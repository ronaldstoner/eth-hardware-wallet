#include "bitcoinfuncs.h"
#include "menu.h"
#include "globals.h"
#include <WiFi.h>
#include <WebServer.h>

// eth libraries
#include <ethers.h>
#include <uECC.h>

#define BUTTON_1 0
#define BUTTON_2 14

#define FAILED_PIN_TIMER 30   //Seconds to lock on failed PIN 

unsigned long failedPinTime = 0;

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

const struct uECC_Curve_t * curve = uECC_secp256k1();

String walletAddress = "";
uint8_t privateKey[32] = {0x07, 0x86, 0xA6, 0x16, 0x14, 0xD9, 0x6A, 0xDB, 0x71, 0x13, 0x41, 0x7C, 0x6C, 0xC3, 0x8A, 0xEB, 0x7A, 0xDC, 0x74, 0xDA, 0x5D, 0x0D, 0xBE, 0x5B, 0x8E, 0xE2, 0x5B, 0x21, 0x84, 0x6A, 0xEC, 0x63};

String convertBytesKeyToHexKey(uint8_t bytesKey[], int size) {
  String str = String();

  for (byte i = 0; i < size; i = i + 1) {
    String int2Hex = String(bytesKey[i], HEX);
    if (int2Hex.length() ==  1) {
      int2Hex = "0" + int2Hex;
    }
    str.concat(int2Hex);
  }

  return str;
};

String convertBytesToUtf8String(byte* bytes, unsigned int length) {
    return String((char*)bytes);
}

// Webserver stuff
const char* ssid = "Hacktest"; // Replace YOUR_SSID with your WiFi network name
const char* password = "testhack";  // Replace YOUR_PASSWORD with your WiFi password
WebServer server(80); // Object to represent the HTTP server

int pincode = 1111;
int userPincode = 0;
int digitPosition = 0;
int digits[4] = {random(10), random(10), random(10), random(10)}; // Initialize with random numbers for security purposes

void handleRoot() {
  // HTML content with a form
  String htmlContent = R"(
  <!DOCTYPE html>
  <html>
  <head>
      <title>ESP32 Form Submission</title>
  </head>
  <body>
      <h2>Submit Transaction Data</h2>
      <form action="/process" method="POST">
          <input type="text" name="txdata" placeholder="Enter transaction data" style="width:300px;" required>
          <input type="submit" value="Process">
      </form>
  </body>
  </html>
  )";
  server.send(200, "text/html", htmlContent);
}

void handleProcess() {
  if (server.hasArg("txdata")) { // Check if the textbox named 'txdata' has data
    String txData = server.arg("txdata"); // Get the data from 'txdata'
    Serial.println("Data received: " + txData); // Print the data to the Serial monitor
    server.send(200, "text/plain", "Data received: " + txData); // Send confirmation back to client
  } else {
    server.send(200, "text/plain", "Error: No data received");
  }
}

void viewAddress() {
  String pk;
  uint8_t pub[64];
  uint8_t address[20];

  pk = String(convertBytesKeyToHexKey(privateKey, 32));
  Serial.println("priv key: " + pk);
  delay(2000);

  // Compute public key based off private key using ECDSA
  uECC_compute_public_key(privateKey, pub, curve);
  String pubStr = convertBytesKeyToHexKey(pub, 64);
  Serial.println("Public Key: " + pubStr);
  delay(2000);

  // Keccak-256 hash of public key
  uint8_t hashed[32];
  ethers_keccak256(pub, 64, hashed);
  String hashStr = convertBytesKeyToHexKey(hashed, 64);
  Serial.println("Keccak-256 of Pubkey: " + hashStr);
  delay(2000);

  // Extract the last 20 bytes (160 bits) from the hashed array
  uint8_t last20Bytes[20];
  memcpy(last20Bytes, &hashed[12], 20);
  String last20BytesStr = convertBytesKeyToHexKey(last20Bytes, 20);
  Serial.println("Last 20 bytes of Keccak-256 hash of Pubkey: " + last20BytesStr);
  delay(2000);
   
  // Convert the extracted bytes to lowercase
  for (int i = 0; i < 20; i++) {
    last20Bytes[i] = tolower(last20Bytes[i]);
  } 

  // Checksum calculation - Keccak-256 hash of the lowercase representation of the last 20 bytes
  uint8_t checksumHash[32];
  ethers_keccak256(last20Bytes, 20, checksumHash);

  // Convert the checksum hash to a hexadecimal string
  String checksumHashStr = convertBytesKeyToHexKey(checksumHash, 32);
  Serial.println("Keccak-256 of lowercase representation of last 20 bytes (Checksum Hash): " + checksumHashStr);
  delay(2000);

  //walletAddress =  "0x" + String(convertBytesKeyToHexKey(address, 20));
  //Serial.println("wallet address: " + walletAddress);
  
}

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
  randomSeed(analogRead(0));
  //uECC_set_rng(&RNG);
  viewAddress();

  delay(100000);

  Serial.println("Scanning for Wi-Fi networks...");

  int n = WiFi.scanNetworks(); // Start scanning for networks
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    for (int i = 0; i < n; ++i) {
      // Print SSID for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(WiFi.SSID(i));
      delay(10);
    }
  }
  Serial.println("Connecting to Wi-Fi...");

  WiFi.begin(ssid, password); // Connect to the WiFi network

  while (WiFi.status() != WL_CONNECTED) { // Wait for the connection to complete
    delay(500);
    Serial.print(".");
  }



  WiFi.begin(ssid, password); // Connect to the WiFi network

  while (WiFi.status() != WL_CONNECTED) { // Wait for the connection to complete
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Print the local IP address to the Serial monitor

  server.on("/", HTTP_GET, handleRoot); // Handle the root URL
  server.on("/process", HTTP_POST, handleProcess); // Handle the form action URL

  server.begin(); // Start the server
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

  server.handleClient(); // Handle client requests

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