#include <ArduinoJson.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> anotherJsonBuffer;
  DynamicJsonBuffer restoreJsonBuffer;

  JsonObject& anotherRoot = anotherJsonBuffer.createObject();
  anotherRoot["anotherSensor"] = "TempSensor";
  anotherRoot["TempData"] = 12;
  String anotherRootString;
  anotherRoot.printTo(anotherRootString);
  Serial.println("String from Object:");
  Serial.println(anotherRootString);

  JsonObject& anotherRootRestored = restoreJsonBuffer.parseObject(anotherRootString);

  Serial.println("Object from String:");
  anotherRootRestored.prettyPrintTo(Serial);

  JsonObject& root = jsonBuffer.createObject();

  root["sensor"] = "gps";
  root["time"] = 1351824120;
  JsonArray& data = root.createNestedArray("data");
  data.add(48.756080);
  data.add(2.302038);
  // root.prettyPrintTo(Serial);
  Serial.println();
  // mergeJSON(root, anotherRoot, "WTF");
  mergeJSON(anotherRootRestored, root, "WTF");
  anotherRootRestored.prettyPrintTo(Serial);
  Serial.println();
}

void mergeJSON(JsonObject& destination, JsonObject& source, String nameofSource) {
  JsonObject& src = destination.createNestedObject(nameofSource);
  for (JsonObject::iterator it=source.begin(); it!=source.end(); ++it) {
    
    if (it->value.is<char*>()){
      src[it->key] = it->value.as<String>();
    } else if (it->value.is<int>()) {
      src[it->key] = it->value.as<int>();
    } else if (it->value.is<double>()){
      src[it->key] = it->value.as<double>();
    } else if (it->value.is<JsonArray>()){
    src[it->key] = it->value.as<JsonArray>();
    }
  }
}



  // root.prettyPrintTo(Serial);
void loop() {
  // put your main code here, to run repeatedly:

}
