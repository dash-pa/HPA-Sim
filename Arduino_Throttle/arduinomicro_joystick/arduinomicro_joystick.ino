
#include "Joystick.h"
#include <stdint.h>
#include <Keyboard.h>

// Pin for the view angle potentiometer, comment out to disable this functionality
#define PIN_VIEWPOT A2
// Calibration values
#define VIEWPOT_MAX 1023
#define VIEWPOT_MIN 0
// Uncomment to reverse the potentiometer direction (software alternative to swapping the power wires of the pot)
//#define VIEWPOT_REVERSE
// Which joystick buttons are bound to view left and right in the simulator
#define JOY_BTN_VIEWRIGHT 10
#define JOY_BTN_VIEWLEFT 11


// Pin for the temporary boost button, comment out to disable this functionality
#define PIN_BOOST 15
// Uncomment if this input is active-low
#define PIN_BOOST_ACTIVELOW
// Boost button duration in milliseconds
#define BOOST_DURATION 10000L
// Set to 2.0f for 2x boost, 3.0f for 3x boost, etc
#define BOOST_MULTIPLIER 2.0f


// Pin for the reset simulation button
#define PIN_RESET A1
// Uncomment if this input is active-low
#define PIN_RESET_ACTIVELOW
// The code to run when the button changes state to active
// - see https://www.arduino.cc/en/Reference/KeyboardModifiers
#define BTN_RESET_PRESS Keyboard.press(KEY_LEFT_SHIFT);Keyboard.press(KEY_ESC)
// The code to run when the button changes state to inactive
#define BTN_RESET_UNPRESS Keyboard.release(KEY_ESC);Keyboard.release(KEY_LEFT_SHIFT)

// Pin for the view change button
#define PIN_VIEWCHANGE A0
// Uncomment if this input is active-low
#define PIN_VIEWCHANGE_ACTIVELOW
// The code to run when the button changes state to active
// - see https://www.arduino.cc/en/Reference/KeyboardModifiers
#define BTN_VIEWCHANGE_PRESS Keyboard.press('v')
// The code to run when the button changes state to inactive
#define BTN_VIEWCHANGE_UNPRESS Keyboard.release('v')

// Called whenever we get power data from the pedals, used to prevent the computer from going into screensaver mode
// Left bracket is flaps up, which does nothing on the current SUMPAC sim
#define KEEP_AWAKE Keyboard.write('[')
// Minimum milliseconds between keep awake events
#define KEEP_AWAKE_PERIOD 5000L
// Watts reading needs to be greater than this number to trigger keep-awake
#define KEEP_AWAKE_THRESHOLD 3

// Display heartbeat on this joystick button output, comment out to disable this functionality
#define JOY_BTN_BLINK 0
// Display fake data state on a joystick button output, comment out to disable this functionality
#define JOY_BTN_FAKEDATA 1

// 255 = 100% throttle = 2378 watts to conform to the DaSH simulator
#define DEFAULT_MULTIPLIER (255.0f / 2378.0f)

// Print a byte as a 2-character hex value
void printHex(uint8_t val) {
  if (val < 16) {
    Serial.write('0');
  }
  Serial.print(val, HEX);
}

// Code that handles getting wattage data from the serial
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

// Return a new fake value every 0.5 seconds
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

// Values changeable from the USB command interface
// throttle = watts * multiplier. Gets modified by boost button (if applicable)
static float multiplier = DEFAULT_MULTIPLIER;
// Doesn't output any joystick data while false
static bool enabled = true;
// Doesn't output any keyboard data while false
static bool kbenabled = true;
// Whether or not to use the fakePacket function instead of readPacket
static bool fakeDataMode = false;


// Print Joystick output status
void pStatJoy() {
  if (enabled) Serial.println("Joystick output is enabled.");
  else Serial.println("Joystick output is DISABLED! ('e' to enable)");
}
// Print Keyboard output status
void pStatKB() {
  if (kbenabled) Serial.println("Keyboard output is enabled.");
  else Serial.println("Keyboard output is DISABLED! ('k' to enable)");
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
  pStatKB();
  pStatFake();
  pStatMult();
}

// Print welcome/version and instructions
void printInfo() {
  Serial.print("SUMPAC pedal joystick v30");
#ifdef JOY_BTN_BLINK
  Serial.write('H'); // heartbeat
#endif
#ifdef JOY_BTN_FAKEDATA
  Serial.write('F');
#endif
#ifdef PIN_VIEWPOT
  Serial.write('V');
#endif
#ifdef PIN_BOOST
  Serial.write('B');
#endif
#ifdef PIN_RESET
  Serial.write('R');
#endif
#ifdef PIN_VIEWCHANGE
  Serial.write('C');
#endif
#ifdef KEEP_AWAKE
  Serial.write('A');
#endif
  Serial.println(". (type 'h' for help)");
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

#ifdef PIN_BOOST
#ifdef PIN_BOOST_ACTIVELOW
  pinMode(PIN_BOOST, INPUT_PULLUP);
#else
  pinMode(PIN_BOOST, INPUT); // needs external pulldown resistor
#endif
#endif

#ifdef PIN_RESET
#ifdef PIN_RESET_ACTIVELOW
  pinMode(PIN_RESET, INPUT_PULLUP);
#else
  pinMode(PIN_RESET, INPUT); // needs external pulldown resistor
#endif
#endif

#ifdef PIN_VIEWCHANGE
#ifdef PIN_VIEWCHANGE_ACTIVELOW
  pinMode(PIN_VIEWCHANGE, INPUT_PULLUP);
#else
  pinMode(PIN_VIEWCHANGE, INPUT); // needs external pulldown resistor
#endif
#endif

  
#ifdef PIN_VIEWPOT
  pinMode(PIN_VIEWPOT, INPUT);
#endif
  
  Joystick.begin(false);
  Keyboard.begin();
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

// Read in any waiting characters from the USB command interface and execute any valid commands
void doCommands() {
  static char cmd[50];
  static char cmdi = 0;
  if (Serial) {
    int c = Serial.read();
    
    if (c == 'e' || c == 'd') { // enable, disable
      enabled = c == 'e';
      pStatJoy();
    } else if (c == 'k') { // toggle keyboard enable
      kbenabled = !kbenabled;
      pStatKB();
    } else if (c == 'f' || c == 'r') { // fakeData, realData
      fakeDataMode = c == 'f';
      pStatFake();
#ifdef JOY_BTN_FAKEDATA
      // Display fake data state on a joystick button output
      Joystick.setButton(JOY_BTN_FAKEDATA, fakeDataMode);
#endif
    } else if (c == 's') { // status
      printStatus();
    } else if (c == 'h') { // help
      printInfo();
    } else if (c == '-' || c == '=' || c == '+') {
      if (c == '-') multiplier = DEFAULT_MULTIPLIER;
      if (c == '=') multiplier = 1.0f;
      if (c == '+') multiplier = 2.0f;
      pStatMult();
    } else if (c == 't') { // Command 't' (aka throttle set) start
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
}

void doView();
void doBoost();
void loop() {
  serialWelcome();
  
  doCommands();

  // Handle fake data or incoming data from RPi
  uint16_t *watts;
  if (fakeDataMode) watts = fakePacket();
  else watts = readPacket();
#ifdef KEEP_AWAKE
  static unsigned long keepAwakeTimer = 0L;
#endif
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
#ifdef KEEP_AWAKE
    if (kbenabled && (watts > KEEP_AWAKE_THRESHOLD) && (millis() >= keepAwakeTimer)) {
      KEEP_AWAKE;
      keepAwakeTimer = millis() + KEEP_AWAKE_PERIOD;
    }
#endif
  }

  // Increments by 1 every 0.1 seconds
  static unsigned char timer = 0;
  // Every 0.1 seconds sample the buttons
  static unsigned long lastBtnUpdate = 0;
  if (millis() > lastBtnUpdate + 100L) {
    lastBtnUpdate = millis(); timer++;
    
#ifdef JOY_BTN_BLINK
    // Every second, toggle state of button 0 joystick output
    if (timer % 10 == 0) Joystick.setButton(JOY_BTN_BLINK, timer % 20);
#endif
    
#ifdef PIN_BOOST
    doBoost();
    Joystick.setButton(3, !digitalRead(PIN_BOOST)); // NOT NEEDED
#endif
#ifdef PIN_RESET
    doBtnReset();
    Joystick.setButton(2, !digitalRead(PIN_RESET)); // NOT NEEDED
#endif    
#ifdef PIN_VIEWCHANGE
    doBtnViewChange();
#endif    
    
#ifdef PIN_VIEWPOT
    doViewPot();
#endif
    
    if (enabled) Joystick.sendState();
  }
}

#ifdef PIN_RESET
void doBtnReset() {
  static bool lastVal = false;
#ifdef PIN_RESET_ACTIVELOW
  bool btn = !digitalRead(PIN_RESET);
#else
  bool btn = digitalRead(PIN_RESET);
#endif
  
  if (btn != lastVal) {
    if (kbenabled) {
      if (btn) {
	BTN_RESET_PRESS;
      } else {
	BTN_RESET_UNPRESS;
      }
    }
    lastVal = btn;
  }
}
#endif
#ifdef PIN_VIEWCHANGE
void doBtnViewChange() {
  static bool lastVal = false;
#ifdef PIN_VIEWCHANGE_ACTIVELOW
  bool btn = !digitalRead(PIN_VIEWCHANGE);
#else
  bool btn = digitalRead(PIN_VIEWCHANGE);
#endif
  
  if (btn != lastVal) {
    if (kbenabled) {
      if (btn) {
	BTN_VIEWCHANGE_PRESS;
      } else {
	BTN_VIEWCHANGE_UNPRESS;
      }
    }
    lastVal = btn;
  }
}
#endif

#ifdef PIN_BOOST
// Change the multiplier based off of a temporary power boost button to make it easier to take off
void doBoost() {
#ifdef PIN_BOOST_ACTIVELOW
  bool btn = !digitalRead(PIN_BOOST);
#else
  bool btn = digitalRead(PIN_BOOST);
#endif

  static unsigned long boostExpiry = 0;
  if (btn) {
    if (multiplier != DEFAULT_MULTIPLIER * BOOST_MULTIPLIER) {
      multiplier = DEFAULT_MULTIPLIER * BOOST_MULTIPLIER;
      pStatMult();
    }
    boostExpiry = millis() + BOOST_DURATION;
  } else if (millis() > boostExpiry && multiplier != DEFAULT_MULTIPLIER) {
    multiplier = DEFAULT_MULTIPLIER;
    pStatMult();
  }
}
#endif

#ifdef PIN_VIEWPOT
// Change the view direction based off of the VIEWPOT potentiometer position
void doViewPot() {
  int a = analogRead(PIN_VIEWPOT);
  a -= VIEWPOT_MIN;
#ifdef VIEWPOT_REVERSE
  a = (VIEWPOT_MAX - VIEWPOT_MIN) - a;
#endif
  // Takes 5 button clicks to get all the way from center to one side * 2 sides + center = 11 positions
  int desiredView = (a * 11) / (VIEWPOT_MAX - VIEWPOT_MIN);
  desiredView -= 5; // center
  if (desiredView > 5) desiredView = 5; // cut off top

  // Go to the desired view
  // Counter for current view position. Negative is left, positive is right
  static int currentView = 0;
  static unsigned char wait = 0;
  if (wait) {
    if (wait == 1) {
      Joystick.setButton(JOY_BTN_VIEWRIGHT, false);
      Joystick.setButton(JOY_BTN_VIEWLEFT, false);
    }
    wait--; // wait for 0.1 seconds (this function is called every 0.1s)
  } else {
    Joystick.setButton(JOY_BTN_VIEWRIGHT, false);
    Joystick.setButton(JOY_BTN_VIEWLEFT, false);
    if (desiredView > currentView) {
      // Set the VIEWRIGHT button to true for 0.1 seconds to go right by 1
      Joystick.setButton(JOY_BTN_VIEWRIGHT, true);
      currentView++;
      wait = 2;
    } else if (desiredView < currentView) {
      // Set the VIEWLEFT button to true for 0.1 seconds to go left by 1
      Joystick.setButton(JOY_BTN_VIEWLEFT, true);
      currentView--;
      wait = 2;
    }
  }

  static int debugCounter = 0;
  if (debugCounter % 5 == 0) {
    //Serial.print("a = "); Serial.println(a, DEC);
    if (currentView != desiredView) {
      Serial.print("Going from view ");
      Serial.print(currentView, DEC);
      Serial.print(" to ");
      Serial.println(desiredView, DEC);
    }
  }
  debugCounter++;
}
#endif
