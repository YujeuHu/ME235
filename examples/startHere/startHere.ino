//************************************************************
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a message to every node on the mesh 
// 4. prints anything it recieves to Serial.print
// 5. extend sleep duration as specified
// 6. averaging sensor reading to eliminate error
//************************************************************

#include <painlessMesh.h>
#include "DHT.h"
#include "ArduinoJson.h"
extern "C" {
  #include "user_interface.h"
}

#define   LED1             D3 
#define   LED2             2
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE, 15);

#define   BLINK_PERIOD    2000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

//--------RTC Init------------
uint32_t SleepInterval = 30*1000000;
uint32_t initializer;
uint32_t FinalSleep;
uint32_t SleepTime = 20*1000000;
uint32_t UpdatedSleepTime;

//--------Flag Init-----------
int SensorFlag = 1;
int totaltemp = 0;
int totalhumi = 0;
int32_t offset = 0;
int32_t OffsetTime = 0;
bool exeOnceBroadcast = true;
bool onFlag = false;
unsigned long broadcastStartingTime = 0;
unsigned long noConnectionStartTime = 0;
bool exeOnceConnection = true;
String msg="";

//--------Time Spec----------
unsigned long broadcastTimeout = 10 * 1000;//milli
//unsigned long sleepInterval = 4294967295; //micro
unsigned long noConnectionTimeout = 9 * 1000;//milli

//--------Time Spec----------
painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;

//--------Task Init----------
void sendMessage(); // Prototype
void obtainMessage();// Prototype
Task taskSendMessage( TASK_SECOND * 2, TASK_FOREVER, &sendMessage ); // start with a one second interval
Task obtainSensorData(TASK_SECOND * 2, TASK_FOREVER, &obtainMessage);// obtain the reading from sensors
Task blinkNoNodes;// Task to blink the number of nodes


void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(5, INPUT);
  pinMode(LED2, OUTPUT);
  
//--------------------Extend Sleep Time If > 71 Mins By Accessing RTC Memory---------------
  ESP.rtcUserMemoryRead(64, &initializer, sizeof(uint32_t));
  ESP.rtcUserMemoryRead(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
  Serial.println("Works Fine!");
  if (initializer != 123){
    Serial.println("first time");
    initializer = 123;
    UpdatedSleepTime = 0;
    ESP.rtcUserMemoryWrite(64, &initializer, sizeof(uint32_t));
    ESP.rtcUserMemoryWrite(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
  }else{
    UpdatedSleepTime = UpdatedSleepTime - SleepInterval;
    if (UpdatedSleepTime >0){
      if (UpdatedSleepTime/(SleepInterval) > 0){
        Serial.print("Remaining Sleep Time: ");
        Serial.println(UpdatedSleepTime);        
        ESP.rtcUserMemoryWrite(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
        ESP.deepSleep(SleepInterval);
      }else{
        Serial.print("Remaining Sleep Time: ");
        Serial.println(UpdatedSleepTime); 
        FinalSleep = UpdatedSleepTime;
        UpdatedSleepTime = 0;
        ESP.rtcUserMemoryWrite(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
        ESP.deepSleep(FinalSleep);
      }
    }
  }
//----------------------------------------------------------------------------------------
  dht.begin();
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  
  mesh.scheduler.addTask( obtainSensorData );
  obtainSensorData.enable();
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

  randomSeed(analogRead(A0));
}

void loop() {
  mesh.update();
  digitalWrite(LED2, !onFlag);
}

void obtainMessage() {
  int h = dht.readHumidity();
  int t = dht.readTemperature();
  totaltemp += random(10,30);
  totalhumi += random(0,100);
  SensorFlag += 1;
  if (SensorFlag == 3){
    if ((totaltemp/3 > 1000 )||( totalhumi/3 >1000)) {
      msg += "NaN";
    }else{
      msg += "Test ";
      msg += mesh.getNodeId();
      msg += " Temperature: ";
      msg += String(floor(totaltemp/3));
      msg += "Humidity: ";
      msg += String(floor(totalhumi/3));
      msg += " myFreeMemory: " + String(ESP.getFreeHeap());
      msg += " noTasks: " + String(mesh.scheduler.size());  
    }
    SensorFlag =0;
  }
  obtainSensorData.setInterval(TASK_SECOND * 2);
}


void sendMessage(){
  OffsetTime = abs(mesh.getNodeTime()-millis()*1000);
  if (mesh.getNodeList().size()>0 ) {    
    if (msg !=""){
      if (exeOnceBroadcast) {
        exeOnceBroadcast = false;
        broadcastStartingTime = millis();
      }
      bool error = mesh.sendBroadcast(msg);
      unsigned long currentTime = millis();
      if (currentTime - broadcastStartingTime > broadcastTimeout) {
        UpdatedSleepTime = SleepTime-OffsetTime*1000+random(0,500000);
        ESP.rtcUserMemoryWrite(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
        if (UpdatedSleepTime > SleepInterval){
          ESP.deepSleep(SleepInterval);
        }else{
          ESP.deepSleep(UpdatedSleepTime);
        }
      }
    Serial.printf("Sending message: %s\n", msg.c_str()); 
    }
  }else{
    if (exeOnceConnection){
      noConnectionStartTime = millis();
      exeOnceConnection = false;
    }
    Serial.printf("No connection time: %.2f\n",(millis() - noConnectionStartTime)/1000.0);
    Serial.printf("Current time: %.2f\n", millis()/1000.0);
    if (millis() - noConnectionStartTime > noConnectionTimeout) {
        UpdatedSleepTime = SleepTime-OffsetTime*1000+random(0,500000);
        ESP.rtcUserMemoryWrite(64+sizeof(uint32_t), &UpdatedSleepTime, sizeof(uint32_t));
        if (UpdatedSleepTime > SleepInterval){
          ESP.deepSleep(SleepInterval);
        }else{
          ESP.deepSleep(UpdatedSleepTime);
        }
    }
  }
}

//=====================buildin tasks to keep mesh network================
void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
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
