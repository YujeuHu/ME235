// Import required libraries
#include "ESP8266WiFi.h"
#include "DHT.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


// WiFi parameters
const char* ssid = "Kun";
const char* password = "yangyangyang";
//const char* ssid = "LinksysWiFi";
//const char* password = "DingisDog";

const int LED_PIN = D3;
int soilpin = A0;
int soilsensor = 0;
int soilpercent = 0;
int number = 1;
bool Flag = 0;
String msg;

DynamicJsonBuffer jsonBuffer;


// Pin
#define DHTPIN 5

// Use DHT11 sensor
#define DHTTYPE DHT11

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);

// Host
const char* host = "api.oparp.com";

void setup() {
  
  // Start Serial
  Serial.begin(115200);
  delay(10);
  
  // Init DHT 
  dht.begin();
  pinMode(A0, INPUT);
  pinMode(D3, OUTPUT);
  pinMode(5, INPUT);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
 
  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }else{
    if (Flag == 0){
      Flag =1;
      JsonObject& totalData = jsonBuffer.createObject();
      totalData["Device1ID"] = 123123;
      totalData["airTemp"] = 12;
      totalData["airHumi"] = 23;
      totalData["Device2ID"] = 456456;
      totalData["temp2"] = 45;
      totalData["humi2"] = 56;
      totalData["Device3ID"] = 789789;
      totalData["temp3"] = 78;
      totalData["humi3"] = 89;
      totalData["Device4ID"] = 111213;
      totalData["temp4"] = 11;
      totalData["humi4"] = 12;
      totalData["Device5ID"] = 141516;
      totalData["temp5"] = 14;
      totalData["humi5"] = 15;
      totalData.printTo(msg);
      totalData.prettyPrintTo(Serial);
      Serial.println();
//      msg.toCharArray(shadow,256);
     }
    client.println("POST /iot HTTP/1.1");
    client.println("Host:  api.oparp.com");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(msg.length());
    client.println();
    client.println(msg);
    Serial.println(msg);
//    client.println();
  }

  soilsensor = analogRead(soilpin);
  soilpercent = map(soilsensor,1000,410,0,100);
  
  // Reading temperature and humidity
  int h = dht.readHumidity();
  // Read temperature as Celsius
  int t = dht.readTemperature();
  //Serial.print(soilsensor);
  
  // This will send the request to the server
//  client.print(String("GET /iot?temperature=") + String(17) + "&humidity=" + String(86) + "&soilhumidity=" + String(78)+" HTTP/1.1\r\n" +
//               "Host: " + host + "\r\n" + 
//               "Connection: keep-alive\r\n\r\n");
               
//  Serial.print(String("temperature=") + String(17) + "&humidity=" + String(86)+ "&soilhumidity=" + String(78)  );
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
//    if (soilpercent <= 50){
//     digitalWrite(D3, HIGH);
//     delay(3000);
//      }
//     else {
//           digitalWrite(D3, LOW);
//          number = number +1;
  //        if (number==6){
    //      number =1;
      //    Serial.println();
        //  Serial.println("Sleep");
          //ESP.deepSleep(2*1000000);
          //delay(100);
 // }
//     }  
  Serial.println();
  //Serial.println("closing connection");
  // Repeat every 10 seconds
  delay(2000);

  
}

