#include <FlexCAN.h>

CAN_message_t msg;

uint32_t txCount = 0;
uint32_t timeStampMillis;

void setup(void)
{
  delay(1000);
  Serial.println("Blake's Test");
  Can0.begin(250000);

  pinMode(2,OUTPUT);
  pinMode(35, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(35, HIGH);

  msg.ext = 1;
  msg.id = 0x00111111;
  msg.len = 8;

  memcpy(&msg.buf[0], &txCount,4);
  timeStampMillis = millis();
  memcpy(&msg.buf[4], &timeStampMillis, 4);
}

void loop() {
  if(Can0.write(msg)){
      txCount++;
      memcpy(&msg.buf[0], &txCount, 4);
      timeStampMillis = millis();
      memcpy(&msg.buf[4], &timeStampMillis, 4);
  }
}
