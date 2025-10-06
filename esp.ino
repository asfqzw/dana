#include <HardwareSerial.h>

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// Network and Firebase credentials
#define WIFI_SSID "wifi."
#define WIFI_PASSWORD "dejesus1922"

#define Web_API_KEY "AIzaSyCoa7wOqmxYz9k6jgx8aIXTNtctZyi3QRc"
#define DATABASE_URL "https://safesmarthome-9b759-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "2020106241@pampangstateu.edu.ph"
#define USER_PASS "Group4DaBEST"

String house = "House_1";

// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

unsigned long lastReadTimeFB = 0;
const unsigned long readIntervalFB = 10000;

String data1, data2, data3;
String ir, smoke, gas;
String irvalue, smokevalue, gasvalue;

char irArray[5];
char smokeArray[4];
char gass;

int kitchenLed;
int livingLed;
int bedLed;
int diningLed;
int crLed;
int livingFan;
int bedFan;
int diningFan;
int relay;
String LedArduino;
String FanArduino;
String Relay;



HardwareSerial espSerial(2);

void setup() {
  Serial.begin(115200);
  espSerial.begin(9600, SERIAL_8N1, 27, 26); // RX=27, TX=26
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  delay(2000);
}

void loop() {
  static unsigned long lastReadTime = 0;
  if (millis() - lastReadTime >= 5000) {
    receivearduinocomm();
    lastReadTime = millis();
  }

  app.loop();
  // Check if authentication is ready
  if (app.ready()){
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      // Update the last send time
      lastSendTime = currentTime;

      // kitchen
      int Ksmoke = smokeArray[0] - '0';
      int Kir = irArray[0] - '0';
      int Kgas = gass - '0';
      Database.set<int>(aClient, "/" + house + "/Kitchen/Smoke", Ksmoke, processData, "RTDB_Send_Int");
      Database.set<int>(aClient, "/" + house + "/Kitchen/IR", Kir, processData, "RTDB_Send_Int");
      Database.set<int>(aClient, "/" + house + "/Kitchen/Gas", Kgas, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Kitchen/Led", 0, processData, "RTDB_Send_Int");
      
      // living
      int Lsmoke = smokeArray[1] - '0';
      int Lir = irArray[1] - '0';
      Database.set<int>(aClient, "/" + house + "/Living/Smoke", Lsmoke, processData, "RTDB_Send_Int");
      Database.set<int>(aClient, "/" + house + "/Living/IR", Lir, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Living/Fan", 0, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Living/Led", 0, processData, "RTDB_Send_Int");
      
      // bed 
      int Bsmoke = smokeArray[2] - '0';
      int Bir = irArray[2] - '0';
      Database.set<int>(aClient, "/" + house + "/Bed/Smoke", Bsmoke, processData, "RTDB_Send_Int");
      Database.set<int>(aClient, "/" + house + "/Bed/IR", Bir, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Bed/Fan", 0, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Bed/Led", 0, processData, "RTDB_Send_Int");

      // dining 
      int Dir = irArray[3] - '0';
      Database.set<int>(aClient, "/" + house + "/Dining/IR", Dir, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Dining/Fan", 0, processData, "RTDB_Send_Int");
      // Database.set<int>(aClient, "/" + house + "/Dining/Led", 0, processData, "RTDB_Send_Int");

      // cr
    }
  }

  if (app.ready()) {
    unsigned long currentTime = millis();
    if (currentTime - lastReadTimeFB >= readIntervalFB) {
      lastReadTimeFB = currentTime;
      // Retrieve Kitchen Led, Living Fan, Bed Status (change to your actual data structure as needed)
      Database.get(aClient, "/" + house + "/Kitchen/Led", processData, false, "GetKitchenLed");
      Database.get(aClient, "/" + house + "/Living/Led", processData, false, "GetLivingLed");
      Database.get(aClient, "/" + house + "/Bed/Led", processData, false, "GetBedLed");
      Database.get(aClient, "/" + house + "/Dining/Led", processData, false, "GetDiningLed");
      Database.get(aClient, "/" + house + "/Cr/Led", processData, false, "GetCrLed");
      Database.get(aClient, "/" + house + "/Living/Fan", processData, false, "GetLivingFan");
      Database.get(aClient, "/" + house + "/Bed/Fan", processData, false, "GetBedFan");
      Database.get(aClient, "/" + house + "/Dining/Fan", processData, false, "GetDiningFan");
      Database.get(aClient, "/" + house + "/Relay", processData, false, "GetRelay");
    }
  }
  LedArduino = String(kitchenLed) + String(livingLed) + String(bedLed) + String(diningLed) + String(crLed);
  FanArduino = String(livingFan) + String(bedFan) + String(diningFan);
  Relay = String(relay);
  sendarduinocomm();

}

void receivearduinocomm(){
  if (espSerial.available()) {
    String received = espSerial.readStringUntil('\n');
    received.trim();
    Serial.println("ESP32 received: " + received);

    // Split into three parts
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
        ir = data1.substring(0, sep);
        irvalue = data1.substring(sep + 1);
        irvalue.toCharArray(irArray, 5);
      }

      sep = data2.indexOf('#');
      if (sep != -1) {
        smoke = data2.substring(0, sep);
        smokevalue = data2.substring(sep + 1);
        smokevalue.toCharArray(smokeArray, 4);
      }

      sep = data3.indexOf('#');
      if (sep != -1) {
        gas = data3.substring(0, sep);
        gasvalue = data3.substring(sep + 1);
        gass = gasvalue.charAt(0);
      }

    } else {
      Serial.println("Received message format invalid.");
    }
  }
}

void sendarduinocomm(){
  espSerial.println("Led#" + LedArduino + " Fan#" + FanArduino + " Relay#" + Relay);
  delay(100);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult()){
    return;
  }
  if (aResult.isEvent()){
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  }
  if (aResult.isDebug()){
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }
  if (aResult.isError()){
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }
  if (aResult.available()){
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    String payload = aResult.c_str();

    // Save retrieved values based on uid
    if (aResult.uid() == "GetKitchenLed") {
      kitchenLed = payload.toInt();
    } else if (aResult.uid() == "GetLivingLed") {
      livingLed = payload.toInt();
    } else if (aResult.uid() == "GetBedLed") {
      bedLed = payload.toInt();
    } else if (aResult.uid() == "GetDiningLed") {
      diningLed = payload.toInt();
    } else if (aResult.uid() == "GetCrLed") {
      crLed = payload.toInt();
    } else if (aResult.uid() == "GetLivingFan") {
      livingFan = payload.toInt();
    } else if (aResult.uid() == "GetBedFan") {
      bedFan = payload.toInt();
    } else if (aResult.uid() == "GetDiningFan") {
      diningFan = payload.toInt();
    } else if (aResult.uid() == "GetRelay") {
      relay = payload.toInt();
    }
  }
}