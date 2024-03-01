#ifndef MENU_H
#define MENU_H

#include <TFT_eSPI.h>

extern TFT_eSPI tft;

enum MenuOption {
  SIGN,
  RECEIVE,
  MNEMONIC,
  LOCK,
  NUM_OPTIONS
};

void drawMenu(MenuOption currentOption);

#endif