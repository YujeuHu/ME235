#include <ArduinoJson.h>
#include "RedundantCheck.h"

RedundantChecker checker;

void setup() {

  Serial.begin(9600);
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> anotherJsonBuffer;
  DynamicJsonBuffer restoreJsonBuffer;

  JsonObject& anotherRoot = anotherJsonBuffer.createObject();
  anotherRoot["DeviceID"] = 1234;
  anotherRoot["TempData"] = 12;
  String anotherRootString;
  anotherRoot.printTo(anotherRootString);
  Serial.println("String from Object:");
  Serial.println(anotherRootString);

  bool isRedundant = checker.check(anotherRootString); // first call : 0
  Serial.println(isRedundant);

  JsonObject& anotherRootRestored = restoreJsonBuffer.parseObject(anotherRootString);

  Serial.println("Object from String:");
  anotherRootRestored.prettyPrintTo(Serial);

  JsonObject& root = jsonBuffer.createObject(); // This one has the smae ID as the previous one

  root["DeviceID"] = 1234;
  root["time"] = 1351824120;
  JsonArray& data = root.createNestedArray("data");
  data.add(48.756080);
  data.add(2.302038);
  // root.prettyPrintTo(Serial);

  String rootString;
  root.printTo(rootString);
  Serial.println();
  checker.reset(); // Here resets the checker
  isRedundant = checker.check(rootString);
  Serial.println(isRedundant); // 0 (becasue of reset)
  
  
}

void loop(){
  //
}

