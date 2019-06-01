#include <FlexCAN.h>
#include "OneButton.h"

const int8_t buttonPin = 24;
OneButton button(buttonPin, 1);


boolean isTx = true; //if node sending messages. false if receiver
boolean isTxMode = true; //if tx node in tx mode. alternates between
//transmitting and printing counter with button press

CAN_message_t msg; //msg buf to send
CAN_message_t inmsg; //msg buf to recv

uint32_t txCount = 0;
uint32_t RXCount0 = 0; 
uint32_t RXCount1 = 0;
uint32_t timeStampMillis;

void myClickFunction(){
  Serial.println("Changing TX Mode!");
  isTxMode = !isTxMode;
}

void printFrame(CAN_message_t rxmsg, uint8_t channel, uint32_t RXCount)
{
  char CANdataDisplay[50];
  sprintf(CANdataDisplay, "%d %12lu %12lu %08X %d %d", channel, RXCount, micros(), rxmsg.id, rxmsg.ext, rxmsg.len);
  Serial.print(CANdataDisplay);
  for (uint8_t i = 0; i < rxmsg.len; i++) {
    char CANBytes[4];
    sprintf(CANBytes, " %02X", rxmsg.buf[i]);
    Serial.print(CANBytes);
  }
  Serial.println();
}

void setup(void)
{
  delay(1000);
  Serial.println("Blake's Test");
  Can0.begin(1000000);

  pinMode(2,OUTPUT);
  pinMode(35, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(35, HIGH);

  //const int8_t buttonPin = 24;
  pinMode(buttonPin, INPUT_PULLUP);
  //OneButton button(buttonPin, true);
  button.attachClick(myClickFunction);

  msg.ext = 1;
  msg.id = 0x00111111;
  msg.len = 8;

  memcpy(&msg.buf[0], &txCount,4);
  timeStampMillis = millis();
  memcpy(&msg.buf[4], &timeStampMillis, 4);
}



void loop() {
  button.tick();
  if (isTx){
    if (isTxMode){
      if(Can0.blockWrite(msg)){
      //delayMicroseconds(1400);
      txCount++;
      memcpy(&msg.buf[0], &txCount, 4);
      timeStampMillis = millis();
      memcpy(&msg.buf[4], &timeStampMillis, 4);
      }
    }
    else{
      Serial.print("TXCount0: "); Serial.print(txCount); Serial.print('\n');
    }
  }
  else{
    if(Can0.read(inmsg)){
      //printFrame(inmsg, 0, RXCount0++);
      RXCount0++;
      Serial.print("RX Count 0:"); Serial.println(RXCount0);
    }
    if(Can1.read(inmsg)){
      //printFrame(inmsg, 1, RXCount1++);
      RXCount1++;
      Serial.print("RX Count 1:"); Serial.println(RXCount1);
    }
  }

}
