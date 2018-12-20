
#include "Joystick.h"
#include <stdint.h>

// 255 = 100% throttle = 2378 watts to conform to the DaSH simulator
#define DEFAULT_MULTIPLIER (255.0f / 2378.0f)

void printHex(uint8_t val) {
  if (val < 16) {
    Serial.write('0');
  }
  Serial.print(val, HEX);
}

#define START_VAL 0xFFFF
#define END_VAL 0xBEEF
typedef struct {
  uint16_t startVal;
  uint16_t watts;
  uint16_t checksum;
  uint16_t endVal;
} Packet, *PacketPtr;
// Returns a pointer to the current wattage read from the serial port, if there's no valid packet it returns NULL
uint16_t *readPacket() {
  static uint8_t buf[sizeof(Packet)];
  const static PacketPtr packet = (PacketPtr) (&buf);
  static int8_t debug_i = sizeof(Packet);
  static uint16_t ret_watts = 0;

  while (Serial1.available()) {
    // Shift bytes down the buffer, shifting in the new Serial1 byte
    if (debug_i == 0 && Serial) printHex(buf[0]); // write out bytes that weren't part of a packet
    else debug_i--; // ignore bytes that were in a packet
    for (uint8_t i = 0; i < sizeof(Packet) - 1; i++) {
      buf[i] = buf[i + 1];
    }
    buf[sizeof(Packet) - 1] = Serial1.read();

    // Check if the packet is valid
    if (packet->startVal == START_VAL && packet->endVal == END_VAL) {
      if ((packet->watts + packet->checksum) == 0) {
        // packet is valid, return the data
        ret_watts = packet->watts;
        debug_i = sizeof(Packet);
        return &ret_watts;
      }
    }
  }
  // Didn't find a valid packet to return and we've run out of Serial1 data
  return NULL;
}

uint16_t *fakePacket() {
  static uint16_t watts = 0;
  static unsigned long lastTime = 0;
  if (millis() > lastTime + 500) {
    watts = random(0, 2300);
    lastTime = millis();
    return &watts;
  } else {
    return NULL;
  }
}


static float multiplier = DEFAULT_MULTIPLIER;
static bool enabled = true;
static bool fakeDataMode = false;


// Print Joystick output status
void pStatJoy() {
  if (enabled) Serial.println("Joystick output is enabled.");
  else Serial.println("Joystick output is DISABLED! ('e' to enable)");
}
// Print Fake Data Mode status
void pStatFake() {
  if (fakeDataMode) Serial.println("Fake data mode is ENABLED. ('r' to disable)");
  else Serial.println("Fake data mode is disabled. Data is real.");
}
// Print Multiplier value
void pStatMult() {
  Serial.print("Multiplier is ");
  Serial.println(multiplier, 10);
}
// Print statuses and values
void printStatus() {
  pStatJoy();
  pStatFake();
  pStatMult();
}

// Print welcome/version and instructions
void printInfo() {
  Serial.println("SUMPAC pedal joystick v18. (type 'h' for help)");
  Serial.println("Joystick output: 'e' to enable, 'd' to disable.");
  Serial.println("Fake Data Mode: 'f' (for 'fake') to enable, 'r' (for 'real') to disable.");
  Serial.println("Multiplier: '-' for default (2378 Watts = max throttle),");
  Serial.println("    '=' for 1.0 (255 W max throt.), '+' for 2.0 (127 W max throt.).");
  Serial.println("Manually set throttle (overwritten by data): \"t123z\" (throttle = 123).");
  printStatus();
}
void serialWelcome() {
  static bool didWelcome = false;
  if (didWelcome) return;

  if (Serial) {
    printInfo();
    didWelcome = true;
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  Joystick.begin(false);
  serialWelcome();
}

void setThrottle(uint8_t value) {
  Joystick.setThrottle(value);
  Joystick.sendState();
}

uint8_t watts2throttle(uint16_t watts) {
  float tmp = (float) watts;
  tmp = tmp * multiplier;
  if (tmp >= 255.0f) return 255;
  else return (uint8_t) tmp;
}

void loop() {
  static char cmd[50];
  static char cmdi = 0;
  serialWelcome();

  // Handle commands
  if (Serial) {
    int c = Serial.read();
    if (c == 'e' || c == 'd') {
      enabled = c == 'e';
      pStatJoy();
    } else if (c == 'f' || c == 'r') {
      fakeDataMode = c == 'f';
      pStatFake();
    } else if (c == 's') {
      printStatus();
    } else if (c == 'h') {
      printInfo();
    } else if (c == '-' || c == '=' || c == '+') {
      if (c == '-') multiplier = DEFAULT_MULTIPLIER;
      if (c == '=') multiplier = 1.0f;
      if (c == '+') multiplier = 2.0f;
      pStatMult();
    } else if (c == 't') { // Command 't' start
      cmd[0] = c; cmdi = 1;
    } else if ('0' <= c && c <= '9') { // Place numbers in command buffer
      cmd[cmdi++] = c;
    } else if (c == 'z') { // End command
      // Parse command
      if (cmd[0] == 't') { // Throttle set command
        uint8_t setVal = (cmd[1] - '0')*100 + (cmd[2] - '0')*10 + (cmd[3] - '0')*1;
        setThrottle(setVal);
        Serial.print("Throttle set to "); Serial.println(setVal, DEC);
      } else { // Invalid command
        cmd[cmdi++] = 'z';
        cmd[cmdi] = 0;
        Serial.print("Uknown command: ");
        Serial.println(cmd);
      }
    }
  }

  // Handle fake data or incoming data from RPi
  uint16_t *watts;
  if (fakeDataMode) watts = fakePacket();
  else watts = readPacket();
  if (watts != NULL) {
    uint8_t throttle = watts2throttle(*watts);
    if (enabled) setThrottle(throttle);
    if (Serial) {
      Serial.print(*watts, DEC);
      if (fakeDataMode) Serial.print(" fakeWatts = ");
      else Serial.print(" watts = ");
      Serial.print(throttle, DEC);
      Serial.println("/255");
    }
  }

  // Blink button 0
  static bool buttonState = false;
  static unsigned long lastChange = 0;
  // Every second, toggle state of button 0 joystick output only if enabled
  // Also press button 1 if Fake Data Mode is enabled.
  if (millis() > lastChange + 1000) {
    buttonState = !buttonState;
    if (enabled) {
      Joystick.setButton(0, buttonState);
      Joystick.setButton(1, fakeDataMode);
      Joystick.sendState();
    }
    lastChange = millis();
  }
}
