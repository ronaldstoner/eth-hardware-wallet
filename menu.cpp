#include "TFT_eSPI.h"
#include "menu.h"
#include "bitcoinfuncs.h"

TFT_eSPI tft = TFT_eSPI();

void drawMenu(MenuOption currentOption) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  
  tft.setTextColor(TFT_ORANGE);
  tft.println();
  //tft.println("   --- STONER WALLET ---");
  printCentered("STONER WALLET", tft.getCursorY(), 12);
  tft.setTextColor(TFT_WHITE);
  //tft.println("    no network detected");
  tft.println();
  printCentered("no network detected", tft.getCursorY(), 12);
  tft.println();
  tft.println();

  if (currentOption == SIGN) {
    tft.setTextColor(TFT_GREEN);
    printCentered("--- Sign ---", tft.getCursorY(), 12);
  } else {
    tft.setTextColor(TFT_WHITE);
    printCentered("Sign", tft.getCursorY(), 12);
  }
  //tft.println("Sign");
  tft.println();

  if (currentOption == RECEIVE) {
    tft.setTextColor(TFT_GREEN);
    printCentered("--- Receive ---", tft.getCursorY(), 12);
  } else {
    tft.setTextColor(TFT_WHITE);
    printCentered("Receive", tft.getCursorY(), 12);
  }
  //tft.println("Receive");
  tft.println();

  if (currentOption == MNEMONIC) { 
    tft.setTextColor(TFT_GREEN);
  printCentered("--- Mnemonic ---", tft.getCursorY(), 12);  
  } else {
    tft.setTextColor(TFT_WHITE);
    printCentered("Mnemonic", tft.getCursorY(), 12);
  }
  //tft.println("Mnemonic");
  tft.println();

  if (currentOption == LOCK) {
    tft.setTextColor(TFT_GREEN);
    printCentered("--- Lock Device ---", tft.getCursorY(), 12);
  } else {
    tft.setTextColor(TFT_WHITE); 
    printCentered("Lock Device", tft.getCursorY(), 12);
  }
  //tft.println("Lock Device");
  tft.println();
}