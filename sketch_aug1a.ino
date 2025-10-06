#include <Adafruit_Fingerprint.h>

// Use hardware Serial2 on Mega for ESP32 link (RX=17, TX=16)
#define arduinoSerial Serial2


const int ledPins[] = {
  53, 52, 51, 50, 49
}; // kitchen = 53, living = 52, bed = 51, dining = 50, cr = 49;
int led[5] = {0,0,0,0,0};
const int fanPins[] = {
  48, 47, 46
}; // living = 48, bed = 47, dining = 46;
int fan[3] = {0,0,0};
String EspFan[3];
const int irPins[] = {
  45, 44, 43, 42
}; // kitchen = 45, living = 44, bed = 43, dining = 42;
int ir[4];
String EspIR[4];
const int smokePins[] = {
  41, 40, 39
}; // kitchen = 41, living = 40, bed = 39; 
int smoke[3];
String EspSmoke[3];
const int Gkit = 38;
int Gas;
const int buzz = 37, valve = 36;
const int relay[] = {
  35, 34
}; // magnetic lock = 35, outlet = 34;
int rel[2];
const int ledpinCount = 5;
const int fanpinCount = 3;
const int irpinCount = 4;
const int smokepinCount = 3;
const int relaypinCount = 2;

String data1, data2, data3;
String ledd, fann, relayy;
String ledvalue, fanvalue, relayvalue;

char charledArray[6];
char charfanArray[4];
char charrelay;


String command, value;


// --------------------------- fingerprint ---------------------------------------------------------------

// Use hardware serial on Mega: Serial1 (pins 18/19)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

static bool fingerIsDown = false;  // edge-detect to avoid duplicate matches on same touch

// ----- Helpers -----
static void printFpCode(const char* label, uint8_t code) {
  Serial.print(label);
  Serial.print(": 0x"); Serial.print(code, HEX);
  Serial.print(" (");
  switch (code) {
    case FINGERPRINT_OK: Serial.print("FINGERPRINT_OK"); break;
    case FINGERPRINT_NOFINGER: Serial.print("FINGERPRINT_NOFINGER"); break;
    case FINGERPRINT_PACKETRECIEVEERR: Serial.print("FINGERPRINT_PACKETRECIEVEERR"); break;
    case FINGERPRINT_PACKETRESPONSEFAIL: Serial.print("FINGERPRINT_PACKETRESPONSEFAIL"); break;
    case FINGERPRINT_IMAGEFAIL: Serial.print("FINGERPRINT_IMAGEFAIL"); break;
    case FINGERPRINT_IMAGEMESS: Serial.print("FINGERPRINT_IMAGEMESS"); break;
    case FINGERPRINT_FEATUREFAIL: Serial.print("FINGERPRINT_FEATUREFAIL"); break;
    case FINGERPRINT_INVALIDIMAGE: Serial.print("FINGERPRINT_INVALIDIMAGE"); break;
    case FINGERPRINT_NOTFOUND: Serial.print("FINGERPRINT_NOTFOUND"); break;
    case FINGERPRINT_TIMEOUT: Serial.print("FINGERPRINT_TIMEOUT"); break;
    default: Serial.print("UNKNOWN"); break;
  }
  Serial.println(")");
}

static void showHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("  e  -> Enroll a new fingerprint (you will be asked for ID 1..127)"));
  Serial.println(F("  t  -> Show template count"));
  Serial.println(F("  h  -> Help (this message)\n"));
}

// Blocking read of an integer ID from Serial
static uint8_t readnumber() {
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available()) { /* wait */ }
    num = (uint8_t)Serial.parseInt();
  }
  return num;
}

// One non-blocking scan step; returns status or matched ID
static uint8_t scanStep() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) {
    fingerIsDown = false;
    return p;
  }
  if (p != FINGERPRINT_OK) {
    printFpCode("getImage", p);
    return p;
  }

  // Avoid duplicate matches while the same finger remains on the sensor
  if (fingerIsDown) {
    return p;
  }
  fingerIsDown = true;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) { printFpCode("image2Tz", p); return p; }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print(F("Found ID #")); Serial.print(finger.fingerID);
    Serial.print(F(" with confidence ")); Serial.println(finger.confidence);
    return finger.fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println(F("Did not find a match"));
    return p;
  } else {
    printFpCode("fingerFastSearch", p);
    return p;
  }
}

// Enrollment flow (blocking). Returns true on success.
static bool enrollFinger(uint8_t id) {
  if (id < 1 || id > 127) {
    Serial.println(F("ID must be 1..127"));
    return false;
  }

  int p = -1;
  Serial.print(F("Waiting for valid finger to enroll as #")); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER)        { Serial.print('.'); delay(50); }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) { Serial.println(F("\nCommunication error")); }
    else if (p == FINGERPRINT_IMAGEFAIL)  { Serial.println(F("\nImaging error")); }
    else if (p != FINGERPRINT_OK)         { Serial.println(F("\nUnknown error")); }
  }
  Serial.println(F("\nImage taken"));

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_IMAGEMESS) Serial.println(F("Image too messy"));
    else if (p == FINGERPRINT_PACKETRECIEVEERR) Serial.println(F("Communication error"));
    else if (p == FINGERPRINT_FEATUREFAIL || p == FINGERPRINT_INVALIDIMAGE) Serial.println(F("Could not find fingerprint features"));
    else Serial.println(F("Unknown error"));
    return false;
  }
  Serial.println(F("Image converted"));

  Serial.println(F("Remove finger"));
  do { p = finger.getImage(); } while (p != FINGERPRINT_NOFINGER);
  delay(300);

  Serial.println(F("Place the same finger again"));
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER)        { Serial.print('.'); delay(50); }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) { Serial.println(F("\nCommunication error")); }
    else if (p == FINGERPRINT_IMAGEFAIL)  { Serial.println(F("\nImaging error")); }
    else if (p != FINGERPRINT_OK)         { Serial.println(F("\nUnknown error")); }
  }
  Serial.println(F("\nImage taken"));

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_IMAGEMESS) Serial.println(F("Image too messy"));
    else if (p == FINGERPRINT_PACKETRECIEVEERR) Serial.println(F("Communication error"));
    else if (p == FINGERPRINT_FEATUREFAIL || p == FINGERPRINT_INVALIDIMAGE) Serial.println(F("Could not find fingerprint features"));
    else Serial.println(F("Unknown error"));
    return false;
  }
  Serial.println(F("Image converted"));

  Serial.print(F("Creating model for #")); Serial.println(id);
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_PACKETRECIEVEERR) Serial.println(F("Communication error"));
    else if (p == FINGERPRINT_ENROLLMISMATCH) Serial.println(F("Fingerprints did not match"));
    else Serial.println(F("Unknown error"));
    return false;
  }
  Serial.println(F("Prints matched!"));

  Serial.print(F("Storing model at ID ")); Serial.println(id);
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_PACKETRECIEVEERR) Serial.println(F("Communication error"));
    else if (p == FINGERPRINT_BADLOCATION) Serial.println(F("Could not store in that location"));
    else if (p == FINGERPRINT_FLASHERR) Serial.println(F("Error writing to flash"));
    else Serial.println(F("Unknown error"));
    return false;
  }
  Serial.println(F("Stored!"));

  // Ensure the next scan starts fresh
  fingerIsDown = false;
  return true;
}

// --------------------------- fingerprint ---------------------------------------------------------------


void setup() {

  Serial1.begin(57600);  // Mega's Serial1 for sensor
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println(F("Found fingerprint sensor!"));
  } else {
    Serial.println(F("Did not find fingerprint sensor :("));
    while (1) { delay(1); }
  }
  finger.getParameters();

  finger.getTemplateCount();

  Serial.begin(115200);
  arduinoSerial.begin(9600);
  arduinoSerial.setTimeout(50);
  delay(2000);
  setPins();
}

void loop() {
  
  turnLedOn();

  turnFanOn();

  checkIR();

  checkSmoke();

  turnValveOn();

  turnRelayOn();

  sendesp32comm();

  receiveesp32comm();

  fingerprint();
}

void setPins(){
  for (int thisPin = 0; thisPin < ledpinCount; thisPin++){
    pinMode(ledPins[thisPin], OUTPUT);  
  }
  for (int thisPin = 0; thisPin < fanpinCount; thisPin++){
    pinMode(fanPins[thisPin], OUTPUT);  
  }
  for (int thisPin = 0; thisPin < irpinCount; thisPin++){
    pinMode(irPins[thisPin], INPUT);  
  }
  for (int thisPin = 0; thisPin < smokepinCount; thisPin++){
    pinMode(smokePins[thisPin], INPUT);  
  }
  pinMode(buzz, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(Gkit, INPUT);
}

void turnLedOn() {
  for (int thisledPin = 0; thisledPin < ledpinCount; thisledPin++){
    if(led[thisledPin] == 1){
      digitalWrite(ledPins[thisledPin], HIGH);      
    } else {
      digitalWrite(ledPins[thisledPin], LOW);
    }
  }
}

void turnFanOn(){
  for (int thisfanPin = 0; thisfanPin < fanpinCount; thisfanPin++){
    if(fan[thisfanPin] == 1){
      digitalWrite(fanPins[thisfanPin], HIGH);      
    } else {
      digitalWrite(fanPins[thisfanPin], LOW);
    }
  }
}

void checkIR(){
  for (int thisirPin = 0; thisirPin < irpinCount; thisirPin++){
    if(digitalRead(irPins[thisirPin]) == LOW){
      ir[thisirPin] = 1;      
    } else {
      ir[thisirPin] = 0;
    }
  }
}

void checkSmoke(){
for (int thissmokePin = 0; thissmokePin < smokepinCount; thissmokePin++){
    if(digitalRead(smokePins[thissmokePin]) == LOW){
      smoke[thissmokePin] = 1;      
    } else {
      smoke[thissmokePin] = 0;
    }
  }
  if(digitalRead(Gkit) == LOW){
    Gas = 1;
  }
  else{
    Gas = 0;
  }
}

void turnValveOn(){
  if(smoke[0] == 1){ // kitchen & dining
    if(ir[0] == 1 || ir[1] == 1){ // kitchen  or dining
      digitalWrite(valve, HIGH);
      digitalWrite(buzz, HIGH);
      delay(5000);     
    } 
  }
  else if(smoke[1] == 1){ // dining & living 
    if(ir[1] == 1 || ir[2] == 1){ // dining or living
      digitalWrite(valve, HIGH);
      digitalWrite(buzz, HIGH);
      delay(5000);     
    }
  } 
  else if(smoke[2] == 1){ // bed
    if(ir[3] == 1){ // bed
      digitalWrite(valve, HIGH);
      digitalWrite(buzz, HIGH); 
      delay(5000);    
    }
  } 
    else{
      digitalWrite(valve, LOW);
      digitalWrite(buzz, LOW);
    }
}

void turnRelayOn(){
  for (int thisrelayPin = 0; thisrelayPin < relaypinCount; thisrelayPin++){
    if(rel[thisrelayPin] == 1){
      digitalWrite(relay[thisrelayPin], HIGH);
    } else{
      digitalWrite(relay[thisrelayPin], LOW);
    }   
  }
}

void sendesp32comm(){
  for (int thisirPin = 0; thisirPin < irpinCount; thisirPin++){
    EspIR[thisirPin] = String(ir[thisirPin]);
  }
  for (int thissmokePin = 0; thissmokePin < smokepinCount; thissmokePin++){
    EspSmoke[thissmokePin] = String(smoke[thissmokePin]);
  }
  
  String EspSendIR = EspIR[0] + EspIR[1] + EspIR[2] + EspIR[3]; 
  String EspSendSmoke = EspSmoke[0] + EspSmoke[1] + EspSmoke[2];

  arduinoSerial.println("IR#" + EspSendIR + " Smoke#" + EspSendSmoke + " Gas#" + String(Gas));
  delay(100);
}


void receiveesp32comm(){

  if (arduinoSerial.available()) {
    int firstChar = arduinoSerial.peek();
    if (firstChar == 'e' || firstChar == 'E') {
      return; // let fingerprint() handle commands like e-<id>
    }
    String received = arduinoSerial.readStringUntil('\n');
    received.trim();
    Serial.println("Arduino received: " + received);

    int firstSpace = received.indexOf(' ');
    int secondSpace = received.indexOf(' ', firstSpace + 1);

    if (firstSpace != -1 && secondSpace != -1) {
      data1 = received.substring(0, firstSpace);
      data2 = received.substring(firstSpace + 1, secondSpace);
      data3 = received.substring(secondSpace + 1);

      // Extract command#value from each data item
      int sep;

      sep = data1.indexOf('#');
      if (sep != -1) {
        ledd = data1.substring(0, sep);
        ledvalue = data1.substring(sep + 1);
        ledvalue.toCharArray(charledArray, 6);
        for(int thisledPin = 0; thisledPin < ledpinCount; thisledPin++){
          led[thisledPin] = charledArray[thisledPin] - '0';
        } 
      }

      sep = data2.indexOf('#');
      if (sep != -1) {
        fann = data2.substring(0, sep);
        fanvalue = data2.substring(sep + 1);
        fanvalue.toCharArray(charfanArray, 4);
        for (int thisfanPin = 0; thisfanPin < fanpinCount; thisfanPin++) {
          fan[thisfanPin] = charfanArray[thisfanPin] - '0';
        }
      }

      sep = data3.indexOf('#');
      if (sep != -1) {
        relayy = data3.substring(0, sep);
        relayvalue = data3.substring(sep + 1);
        charrelay = relayvalue.charAt(0);
        int outlet = charrelay - '0';
        rel[1] = outlet;
      }

    } else {
      Serial.println("Received message format invalid.");
    }
  }
}

void fingerprint(){
  if (arduinoSerial.available()) {
    int firstChar = arduinoSerial.peek();
    if (firstChar != 'e' && firstChar != 'E') {
      return; // ignore non-fingerprint commands on this channel
    }
    String received = arduinoSerial.readStringUntil('\n');
    received.trim();  // Remove newline and extra spaces
    int separatorIndex = received.indexOf('-');
    if (separatorIndex != -1) {
        command = received.substring(0, separatorIndex);
        value = received.substring(separatorIndex + 1);

    } else {
        Serial.println("Invalid format (no '-')");
      }
    if (command == "e" || command == "E") {
      value = 5;
      int myint = value.toInt();

      uint8_t id = myint;
      if (id) {
        bool ok = enrollFinger(id);
        Serial.println(ok ? F("Enroll done.") : F("Enroll failed."));
        finger.getTemplateCount();
        Serial.print(F("Templates: ")); Serial.println(finger.templateCount);
      }
      Serial.println(F("Resuming continuous scanning..."));
    } 
}

  // Continuous verification scan
  scanStep();
  delay(60);  // light debounce to reduce UART chatter
}