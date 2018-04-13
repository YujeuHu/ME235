#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"
#include "RedundantCheck.h"
SSD1306  display(0x3c, 4, 15);

//OLED pins to ESP32 GPIOs via this connection:
//OLED_SDA -- GPIO4
//OLED_SCL -- GPIO15
//OLED_RST -- GPIO16


// WIFI_LoRa_32 ports

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

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

void setup() {
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
}

void loop() {
  // try to parse packet
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
            }
          }
        }else if ((msg.length() - prevLength) < 0){
          Serial.println("Checker Reset!");
          checker.reset(msg);

        }  
      }else{
        messageFlag =false;
      }
    }
  }else{
    sendOnceFlag = false;
  }
}
