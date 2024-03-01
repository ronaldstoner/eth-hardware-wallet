#include "bitcoinfuncs.h"
#include "Bitcoin.h"
#include "qrcoder.h"
#include "menu.h"

extern TFT_eSPI tft;

void displayQRCode(String text) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text.c_str());

  // Calculate the width of the QR code (qrcode.size * 5 in this case)
  int qrWidth = qrcode.size * 5;

  // Calculate the starting position of the QR code
  int startX = (tft.width() - qrWidth) / 2;

  // Draw QR code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + x*5, y*5, 5, 5, TFT_WHITE);
      } else {
        tft.fillRect(startX + x*5, y*5, 5, 5, TFT_BLACK);
      }
    }
  }
}

void printCentered(String text, int y, int z) {
  int textWidth = text.length() * z; // 6 pixels for text size 1, 12 pixels per character when text size is 2
  int x = max(0, (tft.width() - textWidth) / 2);
  tft.setCursor(x, y);
  tft.print(text);
}

void PrintWrapSeed(const char* str, uint16_t color) {
  String seedPhrase = String(str);
  int wordCount = 0;
  String line = " ";

  for (int i = 0; i < seedPhrase.length(); i++) {
    if (seedPhrase.charAt(i) == ' ') {
      wordCount++;
      if (wordCount % 3 == 0) {
        tft.setTextColor(color);
        tft.println(line);
        line = "";
      }
    }
    line += seedPhrase.charAt(i);
  }
  tft.setTextColor(color);
  tft.println(line);
}

void PrintWrapLeft(String text, uint16_t color) {
  // Calculate the maximum number of characters that can fit in the half of the screen space
  // Subtract 1 from maxCharCount for padding
  int maxCharCount = (tft.width() / 2 / tft.textWidth(" ")) - 1; // assuming fixed width font

  // Set the text color
  tft.setTextColor(color);

  // Print characters one by one, adding a newline if the next character would exceed maxCharCount
  int currentCharCount = 0;
  for (int i = 0; i < text.length(); i++) {
    if (currentCharCount + 1 > maxCharCount) {
      tft.println();
      currentCharCount = 0;
    }
    tft.print(text[i]);
    currentCharCount++;
  }
  tft.setCursor(0, tft.getCursorY()); // reset cursor's x position to 0
}

void printHD(String mnemonic, String password) {

  HDPrivateKey hd(mnemonic, password);

  if(!hd){ // check if it is valid
    Serial.println("Invalid xpub");
    return;
  }
  //Serial.println("Mnemonic:" + mnemonic);
  //tft.println("Mnemonic:" + mnemonic);

  // BIP84 (Non-Hardened)
  //Serial.println("\nFirst 5 addresses: ");
  //tft.println("\nFirst 5 addresses: ");
  HDPrivateKey account84 = hd.derive("m/84'/0'/0'/");
  //for(int i=0; i<1; i++){
      //tft.println("#" + String(i) + " - " + account84.derive("m/0/" + String(i)).address());
  //}

  // BIP32 (Coinomi/Ledger, Non-Hardended)
  //Serial.println("First 5 legacy addresses: ");
  //tft.println("First 5 legacy addresses: ");
  //HDPrivateKey account32 = hd.derive("m/44'/0'/0'/");
  //for(int j=0; j<1; j++){
  //  tft.println("#" + String(j) + " - " + account32.derive("m/" + String(j)).address());
  //}  

  // QRTest
  // Replace the hardcoded 0 with addressIndex
  String QRTEST_STRING = account84.derive("m/0/" + String(addressIndex)).address();
  
  
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, QRTEST_STRING.c_str());

  tft.setCursor(0, 0); // Reset the cursor
  // Set the font size to be larger
  tft.setTextSize(2); // Change the number to increase/decrease the font size
  // Replace the hardcoded 1 with addressIndex + 1
  PrintWrapLeft("Receiving   Address #" + String(addressIndex + 1), TFT_GREEN);
  tft.println(); // Add a newline after printing the first string
  tft.println(); // Add a newline after printing the first string
  tft.setTextSize(1);
  PrintWrapLeft("m/84'/0'/0'/0/" + String(addressIndex + 1), TFT_WHITE);
  tft.println(); // Add a newline after printing the first string
  tft.println(); // Add a newline after printing the first string
  tft.println(); // Add a newline after printing the first string
  tft.setTextSize(2);
  PrintWrapLeft(QRTEST_STRING, TFT_ORANGE);

  // Calculate the width of the QR code (qrcode.size * 5 in this case)
  int qrWidth = qrcode.size * 5;

  // Draw QR code to the right of the text
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // Modify these values to change the size and position of the QR code
        // Add (tft.width() - qrWidth) + tft.textWidth(" ") to x*5 to move QR code to the right of the screen
        tft.fillRect((tft.width() - qrWidth) + (tft.textWidth(" ") - 16)+ x*5, y*5, 5, 5, TFT_WHITE);
      } else {
        tft.fillRect((tft.width() - qrWidth) + (tft.textWidth(" ") - 16)+ x*5, y*5, 5, 5, TFT_BLACK);
      }
    }
  }
}