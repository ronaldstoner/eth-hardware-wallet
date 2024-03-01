#ifndef BITCOIN_H
#define BITCOIN_H

#include <Arduino.h>
#include "globals.h" // Include the shared header file

void printHD(String mnemonic, String password = "");
void PrintWrapLeft(String text, uint16_t color);
void PrintWrapSeed(const char* str, uint16_t color);
void printCentered(String text, int y, int z);
void displayQRCode(String text);
#endif