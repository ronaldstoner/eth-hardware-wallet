#include "bitcoinfuncs.h"
#include "menu.h"
#include "globals.h"

#include <stdint.h>
#include <stdlib.h>
#include <WiFi.h>
#include <WebServer.h>

#define BUTTON_1 0
#define BUTTON_2 14

#define FAILED_PIN_TIMER 30   //Seconds to lock on failed PIN 
#define DECODE_RESULT_LEN 100

typedef enum { NONE, STRING, LIST } seq_type;

typedef struct {
  uint8_t **data;
  uint8_t capacity;
  uint8_t size;
  // decode_result * next; // TODO: 这个字段可以用来实现，保存嵌套的list
} decode_result;

unsigned long failedPinTime = 0;

// Webserver stuff
const char* ssid = "hacktest"; // Replace YOUR_SSID with your WiFi network name
const char* password = "testhack";  // Replace YOUR_PASSWORD with your WiFi password
WebServer server(80); // Object to represent the HTTP server

int pincode = 1111;
int userPincode = 0;
int digitPosition = 0;
int digits[4] = {random(10), random(10), random(10), random(10)}; // Initialize with random numbers for security purposes

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

  parseTx("0x");

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


// BEGIN RLP

uint64_t get_index_of_signs(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return -1;
}

uint64_t get_decode_length(uint8_t *seq, int seq_len, int *decoded_len, seq_type *type) {
  uint8_t first_byte = *seq;
  uint64_t item_bytes_len = 0;
  if (first_byte <= 0x7f) {
    item_bytes_len = 1;
    *type = STRING;
    *decoded_len = 0;
  } else if (first_byte <= 0xb7 &&
             seq_len >
                 (first_byte - 0x80)) // 第二个条件是为了防止以后读取数据时越界
  {
    item_bytes_len = first_byte - 0x80;
    *type = STRING;
    *decoded_len = 1;
  } else if (first_byte <= 0xbf && seq_len > (first_byte - 0xb7)) {
    uint8_t len = first_byte - 0xb7;
    uint8_t buffer_len[len];
    uint8_t hex_len[len * 2 + 1];
    hex_len[len * 2] = '\0';
    *decoded_len = 1;
    memcpy(buffer_len, seq + *decoded_len, len);
    *decoded_len += 1;
    buffer_to_hex(buffer_len, len, hex_len, len * 2);
    item_bytes_len = hex2dec((char*)hex_len);
    *type = STRING;
  } else if (first_byte <= 0xf7 && seq_len > (first_byte - 0xc0)) {
    item_bytes_len = first_byte - 0xc0;
    *type = LIST;
    *decoded_len = 1;
  } else if (first_byte <= 0xff && seq_len > (first_byte - 0xf7)) {
    uint8_t len = first_byte - 0xf7;
    uint8_t buffer_len[len];
    uint8_t hex_len[len * 2 + 1];
    hex_len[len * 2] = '\0';

    *decoded_len = 1;
    memcpy(buffer_len, seq + *decoded_len, len);
    *decoded_len += 1;
    buffer_to_hex(buffer_len, len, hex_len, len * 2);
    item_bytes_len = hex2dec((char*)hex_len);
    *type = LIST;
  } else {
    perror("sequence don't conform RLP encoding form");
  }

  return item_bytes_len;
}

int rlp_decode(decode_result *my_result, uint8_t *seq, int seq_len) {
  if (seq_len == 0)
    return 0;
  seq_type type = NONE; // 保存此次将要处理的数据的类型
  int decoded_len;      // 保存此次解码过的长度
  uint64_t item_num = get_decode_length(seq, seq_len, &decoded_len,
                                        &type); // 保存需要解码的长度
  Serial.println("rlp_decode: 1");
  uint8_t *start_ptr = seq + decoded_len;       // 解码的起始地址
  int need_decode_len = seq_len - decoded_len;

  if (type == STRING) {
    Serial.println("rlp_decode: 2");

    if (my_result->capacity < my_result->size) {
      Serial.println("in todo");
      // TODO:申请更大的内存空间，并拷贝旧数据，释放旧空间
    }
    char tmp[item_num];
    memcpy(tmp, start_ptr, item_num);
    uint8_t *buf1 = (uint8_t *)malloc((size_t)(sizeof(char) * item_num * 2 + 1));
    buffer_to_hex((const uint8_t*)tmp, item_num, buf1, item_num * 2);
    buf1[item_num * 2] = '\0';
    my_result->data[my_result->size++] = buf1;
    rlp_decode(my_result, start_ptr + item_num, need_decode_len - item_num);
  } else if (type == LIST) {
    Serial.println("rlp_decode: 3");

    rlp_decode(my_result, start_ptr, need_decode_len);
  }
}

static uint8_t convert_hex_to_digital(uint8_t c) {
  if (c >= '0' && c <= '9') {
    c -= '0';
  } else if (c >= 'A' && c <= 'F') {
    c = c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    c = c - 'a' + 10;
  } else {
    return 0xff;
  }

  return c;
}

int hex_to_buffer(const uint8_t *hex, size_t hex_len, uint8_t *out,
                  size_t out_len) {
  int padding = hex_len % 2;
  uint8_t last_digital = 0;
  int i;
  for (i = 0; i < hex_len && i < out_len * 2; i++) {
    uint8_t d = convert_hex_to_digital(hex[i]);
    if (d > 0x0f) {
      break;
    }

    if ((i + padding) % 2) {
      out[(i + padding) / 2] = ((last_digital << 4) & 0xf0) | (d & 0x0f);
    } else {
      last_digital = d;
    }
  }

  return (i + padding) / 2;
}

int buffer_to_hex(const uint8_t *buffer, size_t buffer_len, uint8_t *out,        size_t out_len) {
  uint8_t map[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  int i;
  for (i = 0; i < buffer_len && i < out_len / 2; i++) {
    uint8_t b = (buffer[i] >> 4) & 0x0f;
    uint8_t l = buffer[i] & 0x0f;
    out[i * 2] = map[b];
    out[i * 2 + 1] = map[l];
  }

  return i * 2;
}

/* 十六进制数转换为十进制数 */
uint64_t hex2dec(char *source) {
  uint64_t sum = 0;
  uint64_t t = 1;
  int i, len = 0;

  len = strlen(source);
  for (i = len - 1; i >= 0; i--) {
    uint64_t j = get_index_of_signs(*(source + i));
    sum += (t * j);
    t *= 16;
  }

  return sum;
}
// END RLP

String parseTx(String txdata) {
  Serial.println("parsing tx");
  uint8_t seq[] =
      "f8aa018504e3b2920083030d409486fa049857e0209aa7d9e616f7eb3b3b78ecfdb080b8"
      "44a9059cbb0000000000000000000000001febdb3539341a3005f3c5851854db7720a037"
      "fe00000000000000000000000000000000000000000000000002c68af0bb1400001ba0fe"
      "13bc4be8dc46e804f5b7eb6182d1f6feecdc2574eafface0ecc7f7be3516f4a042586cf9"
      "62a3896e607214eede1a5cde727e13b62f86678efb980d9047a1017e";
  uint8_t buffer[(sizeof(seq)) / 2] = {0};
  hex_to_buffer(seq, sizeof(seq) - 1, buffer, (sizeof(seq) - 1) / 2);

  decode_result my_resut;
  char no_use;
  my_resut.data = (uint8_t**) malloc(sizeof(&no_use) * DECODE_RESULT_LEN);
  my_resut.capacity = DECODE_RESULT_LEN;
  my_resut.size = 0;

  Serial.println("before decode");

  rlp_decode(&my_resut, buffer, sizeof(buffer) / sizeof(buffer[0]));
  Serial.println("after decode");

  for (size_t i = 0; i < my_resut.size; i++) {
    Serial.printf("index:%d,data:%s\n", i, my_resut.data[i]);
  }
  Serial.println("done parsing");
  return "";
}
