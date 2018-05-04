//************************************************************
//
// 1. Receive message from mesh
// 2. broadcast by lora
//
//
//************************************************************
#include <painlessMesh.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include "RedundantCheck.h"
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
//-------------------------------------------------
#define DHTPIN 17
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
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

String packet;
int airtemp;
int airhumi;
int msgsize = 0;
String totalDataString;
String totalDataStringCP;

//--------RTC Init------------
uint32_t SleepTime = 75*1000000;
uint32_t UpdatedSleepTime;

//--------Flag Init-----------
bool airDataFlag = 0;
int32_t OffsetTime = 0;
bool exeOnceBroadcast = true;
bool onFlag = false;
unsigned long broadcastStartingTime = 0;
bool exeOnceConnection = true;
bool sendingFlag = false;
String msg="";
int h;
int t;

//--------Time Spec----------
unsigned long broadcastTimeout = 45 * 1000;//milli

void sendMessage();
Task taskSendMessage( TASK_SECOND * 2, TASK_FOREVER, &sendMessage ); // start with a one second interval
void obtainMessage();
Task obtainSensorData(TASK_SECOND * 2, TASK_FOREVER, &obtainMessage);// obtain the reading from sensors

StaticJsonBuffer<512> totalBuffer;
StaticJsonBuffer<512> restoreJsonBuffer;
JsonObject& totalData = totalBuffer.createObject();

painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

RedundantChecker checker;

void setup() {
// ---------------Lora-------------------
  dht.begin();
  pinMode(17, INPUT); // DHT Pin
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
}

void loop() {
  mesh.update();
}
//------------------------ MergeJSON ---------------------------
void mergeJSON(JsonObject& destination, JsonObject& source) {
  String  nameofSource = String(source.begin() -> value.as<uint32_t>());
  nameofSource = "ID" + nameofSource;
  JsonObject& src = destination.createNestedObject(nameofSource);
  for (JsonObject::iterator it = ++source.begin(); it!=source.end(); ++it) {
//    if (it->key == "DeviceID"){
//      continue;
//    }
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
void obtainMessage() {
  h = dht.readHumidity();
  t = dht.readTemperature();
}

void sendMessage(){
  OffsetTime = abs((uint64_t)mesh.getNodeTime()-millis()*1000);
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
    
    esp_sleep_enable_timer_wakeup((uint64_t)UpdatedSleepTime);
    if (sendingFlag == false){
    esp_deep_sleep_start();
    }
   }
}
//=====================buildin tasks to keep mesh network================
void receivedCallback(uint32_t from, String & msg) {
  sendingFlag = true;
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  if (airDataFlag == 0){
    airDataFlag = 1;
    airtemp = t;
    airhumi = h;
    totalData["DeviceID"] = mesh.getNodeId();
    totalData["airtemp"] = airtemp;
    totalData["airhumi"] = airhumi;
  }
  bool isRedundant = checker.check( msg );
  if (!isRedundant){
    JsonObject& otherData = restoreJsonBuffer.parseObject(msg);
    mergeJSON(totalData, otherData);
    msgsize++;
    totalData.printTo(totalDataString);    
    totalDataStringCP = totalDataString;
    totalDataString = "";
  }
  if (msgsize > 0){
    Serial.println("Total Data String:");
    Serial.println(totalDataStringCP);
    // send packet
    LoRa.beginPacket();
    LoRa.print(totalDataStringCP);
    LoRa.endPacket();
  }
  sendingFlag = false;
}

void newConnectionCallback(uint32_t nodeId) {
  onFlag = false;
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections %s\n", mesh.subConnectionJson().c_str());
  onFlag = false;
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
