// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Please use an Arduino IDE 1.6.8 or greater

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"
#include "RedundantCheck.h"
SSD1306  display(0x3c, 4, 15);
#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6

#include "config.h"

static bool messagePending = false;
static bool messageSending = true;

static char *connectionString;
static char *ssid;
static char *pass;

static int interval = INTERVAL;

int counter = 0;
uint16_t msgMaxLength = 0;
uint16_t prevLength;
bool messageFlag = false;
bool sendOnceFlag = false;
uint32_t messageTimer = 0;

RedundantChecker checker;


void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %ld.\r\n", epochTime);
            break;
        }
    }
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
void setup()
{
    pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH);
  
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  Serial.begin(115200);
  while (!Serial); //if just the the basic function, must connect to a computer
  delay(1000);
 
  Serial.println("LoRa Receiver"); 
  display.drawString(5,5,"LoRa Receiver"); 
  display.display();
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  
  if (!LoRa.begin(BAND)) {
    display.drawString(5,25,"Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!");
  display.drawString(5,25,"LoRa Initializing OK!");
  display.display();
    pinMode(LED_PIN, OUTPUT);

    initSerial();
    delay(2000);
    readCredentials();

    initWifi();
    initTime();
    // initSensor();
    
    /*
    * Break changes in version 1.0.34: AzureIoTHub library removed AzureIoTClient class.
    * So we remove the code below to avoid compile error.
    */
    // initIoThubClient();

    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);
    
}

static int messageCount = 1;
void loop()
{
  String payload = "";
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
      // received a packets
      Serial.println("Received packet. ");

     // read packet
     while (LoRa.available()) {
       String msg = LoRa.readString();
       Serial.println(msg);
       bool isRedundant = checker.check(msg,prevLength);
       Serial.println("Prev Length = " + String(prevLength));
       Serial.println("Redundant? = " + String(isRedundant));
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
               payload = payload +"}";

               if (!messagePending && messagesSending)
                {
                  // String msg;
                  char messagePayload[MESSAGE_MAX_LEN];
                  // readMessage(messageCount, msg);

                  payload.toCharArray(messagePayload,payload.length());
                  sendMessage(iotHubClientHandle, messagePayload, false);
                  // uint32_t len = msg.length();
                  // messageCount++;
                  // delay(interval);
                }
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
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    delay(10);
}
