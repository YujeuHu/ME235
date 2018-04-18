#include <ArduinoJson.h>
#include <DHT.h>
#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"
#include "RedundantCheck.h"

#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

int counter = 0;
uint16_t msgMaxLength = 0;
uint16_t prevLength;
bool messageFlag = false;
bool sendOnceFlag = false;
uint32_t messageTimer = 0;

RedundantChecker checker;

#if SIMULATED_DATA

void initSensor()
{
    // use SIMULATED_DATA, no sensor need to be inited
}

float readTemperature()
{
    return random(20, 30);
}

float readHumidity()
{
    return random(30, 40);
}

#else

static DHT dht(DHT_PIN, DHT_TYPE);
void initSensor()
{
    dht.begin();
}

float readTemperature()
{
    return dht.readTemperature();
}

float readHumidity()
{
    return dht.readHumidity();
}

#endif

void readMessage(int messageId, String &payload)
{
  payload = "";
    int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packets
    Serial.println("Received packet. ");

    // read packet
    while (LoRa.available()) {
      String msg = LoRa.readString();
      //Serial.println(msg);
      bool isRedundant = checker.check(msg,prevLength);
      if(isRedundant){
        //Serial.println("already Received!");
        //Serial.println("PrevLength = " + String(prevLength));
        //Serial.println("CurrentLength = " + String(msg.length()));
        if((msg.length() - prevLength) == 0){//msg is ready to send
          if (messageFlag == false){
            messageFlag = true;
            messageTimer = millis();
          }
          if ((millis() - messageTimer) > 5*1000){
            if (sendOnceFlag == false){
              sendOnceFlag = true;
              Serial.println("Complete Msg:");
              Serial.println(msg);
              payload = msg;
            }
          }
        }else if (((int)msg.length() - (int)prevLength) < 0){
          Serial.println("Checker Reset!");
          checker.reset(msg);
        }  
      }else{
        messageFlag =false;
        sendOnceFlag = false;
      }
    }
  }
}

void parseTwinMessage(char *message)
{
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(message);
    if (!root.success())
    {
        Serial.printf("Parse %s failed.\r\n", message);
        return;
    }

    if (root["desired"]["interval"].success())
    {
        interval = root["desired"]["interval"];
    }
    else if (root.containsKey("interval"))
    {
        interval = root["interval"];
    }
}
