//************************************************************
//
// 1. Receive message from mesh
// 2. broadcast by lora
//
//
//************************************************************
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include "RedundantCheck.h"


//-------------Lora------------------------
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
//#include "images.h"
//-------------------------------------------------

//#define   LED1             D3 
#define   LED2             2
#define   BLINK_PERIOD    2000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

//-------------Lora------------------------
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

//-------------Lora------------------------
unsigned int counter = 0;

SSD1306 display(0x3c, 4, 15);
String rssi = "RSSI --";
String packSize = "--";
String packet ;
int airtemp;
int airhumi;
int msgsize = 0;
String totalDataString;

//--------RTC Init------------
uint32_t SleepInterval = 30*1000000;
uint32_t SleepTime = 20*1000000;
uint32_t UpdatedSleepTime;

//--------Flag Init-----------
int32_t OffsetTime = 0;
bool exeOnceBroadcast = true;
bool onFlag = false;
unsigned long broadcastStartingTime = 0;
unsigned long noConnectionStartTime = 0;
bool exeOnceConnection = true;
bool sendingFlag = false;
String msg="";

//--------Time Spec----------
unsigned long broadcastTimeout = 20 * 1000;//milli
//unsigned long sleepInterval = 4294967295; //micro
unsigned long noConnectionTimeout = 9 * 1000;//milli

void sendMessage();
Task taskSendMessage( TASK_SECOND * 2, TASK_FOREVER, &sendMessage ); // start with a one second interval

  
StaticJsonBuffer<512> totalBuffer;
DynamicJsonBuffer restoreJsonBuffer;

JsonObject& totalData = totalBuffer.createObject();

bool airDataFlag = 0;
//---------------------------------------------

painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

RedundantChecker checker;

Task blinkNoNodes;// Task to blink the number of nodes


void setup() {
// ---------------Lora-------------------
  pinMode(16,OUTPUT);
  pinMode(25,OUTPUT);
  
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  //pinMode(LED1, OUTPUT);
  pinMode(5, INPUT);
  pinMode(LED2, OUTPUT);
  randomSeed(analogRead(A0));
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Sender Test");
  
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("init ok");
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
  randomSeed(analogRead(0));
  
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  //mesh.setDebugMsgTypes(ERROR);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  mesh.scheduler.addTask( taskSendMessage );
  taskSendMessage.enable() ;
  
    blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
      // If on, switch off, else switch on
      if (onFlag)
        onFlag = false;
      else
        onFlag = true;
      blinkNoNodes.delay(BLINK_DURATION);

      if (blinkNoNodes.isLastIteration()) {
        // Finished blinking. Reset task for next run 
        // blink number of nodes (including this node) times
        blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
        // Calculate delay based on current mesh time and BLINK_PERIOD
        // This results in blinks between nodes being synced
        blinkNoNodes.enableDelayed(BLINK_PERIOD - 
            (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
      }
  });
  mesh.scheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();
}

void loop() {
  mesh.update();
  digitalWrite(LED2, !onFlag);
}
//------------------------ MergeJSON ---------------------------
void mergeJSON(JsonObject& destination, JsonObject& source, String nameofSource) {
  JsonObject& src = destination.createNestedObject(nameofSource);
  for (JsonObject::iterator it=source.begin(); it!=source.end(); ++it) {
    
    if (it->value.is<char*>()){
      src[it->key] = it->value.as<String>();
    } else if (it->value.is<int>()) {
      src[it->key] = it->value.as<uint32_t>();
    } else if (it->value.is<double>()){
      src[it->key] = it->value.as<double>();
    } else if (it->value.is<JsonArray>()){
    src[it->key] = it->value.as<JsonArray>();
    }
  }
}

void sendMessage(){
  OffsetTime = abs(mesh.getNodeTime()-millis()*1000);
  Serial.println("OffsetTime");
  Serial.println(OffsetTime);
  
  if (exeOnceBroadcast) {
    exeOnceBroadcast = false;
    broadcastStartingTime = millis();
  }
  unsigned long currentTime = millis();
  if (currentTime - broadcastStartingTime > broadcastTimeout) {
    UpdatedSleepTime = SleepTime - OffsetTime-random(-500000,500000);
    if (UpdatedSleepTime > 3720000000){
      UpdatedSleepTime = 1;
    }
    Serial.println("Updated Sleep Time");
    Serial.println(UpdatedSleepTime);

    
    esp_sleep_enable_timer_wakeup(UpdatedSleepTime);
    if (sendingFlag == false){
    esp_deep_sleep_start();
    }
   }
}
//-----------------------------------------------------------------

//=====================buildin tasks to keep mesh network================
void receivedCallback(uint32_t from, String & msg) {
  sendingFlag = true;
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  if (airDataFlag == 0){
    airDataFlag = 1;
    airtemp = random(30);
    airhumi = random(100);
    totalData["DeviceID"] = mesh.getNodeId();
    totalData["airtemp"] = airtemp;
    totalData["airhumi"] = airhumi;
  }
  bool isRedundant = checker.check( msg );
  Serial.println(isRedundant);
  if (!isRedundant){
//    Serial.println("--------------------");
//    Serial.println("No Redundancy Found!");
//    Serial.println("--------------------");
    JsonObject& otherData = restoreJsonBuffer.parseObject(msg);
//    Serial.println("Other Data:");
//    otherData.prettyPrintTo(Serial);
    //String deviceName = "Sensor " + String(msgsize);
    mergeJSON(totalData, otherData,String(msgsize));
    msgsize++;
    Serial.println("Total Data:");
    totalData.prettyPrintTo(Serial);
    totalData.printTo(totalDataString);    
  }
  if (msgsize > 0){
    
    //airDataFlag == 0;
    //msgsize=0;
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
  
    display.drawString(0, 0, "Sending packet: ");
    display.drawString(90, 0, String(counter));
    display.display();
    // send packet
    LoRa.beginPacket();
    LoRa.print(totalDataString);
    LoRa.print(counter);
    LoRa.endPacket();
    //pkgCreated = true;
    counter++;
  }
  sendingFlag = false;
//------------------------------------------------  

//--------------------------------------------------------------------------------------------
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
 
  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
