//************************************************************
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a message to every node on the mesh 
// 4. prints anything it recieves to Serial.print
// 5. extend sleep duration as specified
// 6. averaging sensor reading to eliminate error
//************************************************************

#include <painlessMesh.h>
#include "ArduinoJson.h"
#include "sleepTimer.h"

sleepTimer Timer;

#define   LED1             D3 
#define   LED2             2
#define   BLINK_PERIOD    2000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

//--------RTC Init------------
uint32_t SleepTime = 90*1000000;
uint32_t UpdatedSleepTime;

//--------Flag Init-----------
int8_t SensorFlag = 1;
int totaltemp = 0;
int totalhumi = 0;
//----------Broadcast Init-----------
uint8_t SensorDataSize = 3;
int32_t offset = 0;
int32_t OffsetTime = 0;
bool exeOnceBroadcast = true;
bool onFlag = false;
unsigned long broadcastStartingTime = 0;
bool exeOnceConnection = true;
String msg="";
int sensor_pin = A0; 
int output_value ;
//--------Time Spec----------
unsigned long broadcastTimeout = 30 * 1000;//milli
unsigned long noConnectionTimeout = 9 * 1000;//milli
//--------Time Spec----------
painlessMesh  mesh;
bool calc_delay = false;
SimpleList<uint32_t> nodes;
//--------Task Init----------
void sendMessage(); // Prototype
void obtainMessage();// Prototype
Task taskSendMessage( TASK_SECOND * 2, TASK_FOREVER, &sendMessage ); // start with a one second interval
Task obtainSensorData(TASK_SECOND * 0.1, TASK_FOREVER, &obtainMessage);// obtain the reading from sensors
Task blinkNoNodes;// Task to blink the number of nodes
// ----- JSON Init ------
DynamicJsonBuffer jsonBuffer;
JsonObject& Root = jsonBuffer.createObject();

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(5, INPUT);
  pinMode(LED2, OUTPUT);
  pinMode(A0,INPUT);
//--------------------Extend Sleep Time If > 71 Mins By Accessing RTC Memory---------------
  Timer.startSleeping();
//----------------------------------------------------------------------------------------
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  //mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION | COMMUNICATION);  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
    
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
  mesh.scheduler.addTask( obtainSensorData );
  obtainSensorData.enable();
  mesh.scheduler.addTask( taskSendMessage );
  taskSendMessage.enable() ;
}

void loop() {
  mesh.update();
  digitalWrite(LED2, !onFlag);
}

void obtainMessage() {
  output_value= analogRead(sensor_pin);
  output_value = map(output_value,770,360,0,100);
  totaltemp += random(0,30);
  totalhumi += output_value;
  SensorFlag += 1;

  if (SensorFlag == SensorDataSize){
    if ((totaltemp/SensorDataSize > 1000 )||( totalhumi/SensorDataSize >1000)) {
      Root["DeviceID"] = mesh.getNodeId();
      Root["Temp"] =random(0,30);
      Root["Humi"] =random(0,100);
    }else{
      Root["DeviceID"] = mesh.getNodeId();
      Root["T"] = round(totaltemp/3.0*100)/100;
      Root["H"] = round(totalhumi/3.0*100)/100;
    }
    Root.printTo(msg);
  }
  obtainSensorData.setInterval(TASK_SECOND * 3);
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
    if (UpdatedSleepTime > 4000000000){
      UpdatedSleepTime = 1;
    } 
    Timer.setSleepTime((uint32_t)UpdatedSleepTime); 
    Timer.startSleeping();
   }
  if (mesh.getNodeList().size()>0 ) { 
    if (msg != ""){
//      bool error = mesh.sendBroadcast(msg,false);
      uint32_t destID = 2758734973;
      bool error = mesh.sendSingle(destID, msg);
//      if (calc_delay) {
//        SimpleList<uint32_t>::iterator node = nodes.begin();
//        while (node != nodes.end()) {
//          mesh.startDelayMeas(*node);
//          node++;
//        }
//        calc_delay = false;
//      }
      Serial.printf("Sending message: %s\n", msg.c_str()); 
    }
  }
  taskSendMessage.setInterval(TASK_SECOND*2);
}


//=====================buildin tasks to keep mesh network================
void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
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
