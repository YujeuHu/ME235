#include <ArduinoJson.h>
// #include <DHT.h>
//#include <SPI.h>
#include <LoRa.h>
//#include "SSD1306.h"
//#include "RedundantCheck.h"

#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

//int counter = 0;
//uint16_t msgMaxLength = 0;
//uint16_t prevLength;
//bool messageFlag = false;
//bool sendOnceFlag = false;
//uint32_t messageTimer = 0;
//
//RedundantChecker checker;


//void readMessage(int messageId, String &payload)
//{
//  //payload = "{\"DeviceID\":2758734973,\"airtemp\":0,\"airhumi\":5,\"ID3893651586\":{\"T\":7,\"H\":42},\"ID3163568818\":{\"T\":1,\"H\":36},\"ID2138597866\":{\"T\":6,\"H\":43},\"time\":\"2018:04:18T09:11:01\"}";
//  payload = "";
//  int packetSize = LoRa.parsePacket();
//  if (packetSize) {
//      // received a packets
//      Serial.println("Received packet. ");
//
//     // read packet
//     while (LoRa.available()) {
//       String msg = LoRa.readString();
//       Serial.println(msg);
//       bool isRedundant = checker.check(msg,prevLength);
//       if(isRedundant){
//         //Serial.println("already Received!");
//         //Serial.println("PrevLength = " + String(prevLength));
//        //Serial.println("CurrentLength = " + String(msg.length()));
//         if((msg.length() - prevLength) == 0){//msg is ready to send
//           if (messageFlag == false){
//             messageFlag = true;
//             messageTimer = millis();
//           }
//           if ((millis() - messageTimer) > 5*1000){
//             if (sendOnceFlag == false){
//               sendOnceFlag = true;
//               Serial.println("Complete Msg:");
//               Serial.println(msg);
//               payload = msg;
//               payload = payload +"}";
//             }
//           }
//         }else if (((int)msg.length() - (int)prevLength) < 0){
//           Serial.println("Checker Reset!");
//           checker.reset(msg);
//         }  
//       }else{
//         messageFlag =false;
//         sendOnceFlag = false;
//       }
//     }
//   }
//}

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
