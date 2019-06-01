/*
 * Smart Sensor Simulator 2
 * 
 * Arduino Sketch to define the Smart Sensor Simulator 2 Functions
 * Uses the Teensy 3.6
 * This file needs to be included in the main program.
 * 
 * Written By Dr. Jeremy S. Daily
 * The University of Tulsa
 * Department of Mechanical Engineering
 * 
 * 06 May 2017
 * 
 * Released under the MIT License
 *
 * Copyright (c) 2017        Jeremy S. Daily
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * All libraried and packages are available by installing the following: 
 * Arduino v1.8 https://www.arduino.cc/en/Main/Software
 * Teensyduino 1.35 https://www.pjrc.com/teensy/td_download.html
 * 
 * Libraries not included in the Arduino and Teensyduino environment include:
 * Adafruit_MCP23017 - https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
 * OneButton - https://github.com/mathertel/OneButton
 * FlexCAN - https://github.com/collin80/FlexCAN_Library
 * Thread - https://github.com/ivanseidel/ArduinoThread
 * TeensyID - https://github.com/sstaub/TeensyID
*/
#define ENCODER_OPTIMIZE_INTERRUPTS

#include <SPI.h>
#include <i2c_t3.h>
#include <Encoder.h>
#include "OneButton.h"
#include "Adafruit_MCP23017.h" 
#include <EEPROM.h>
#include <FlexCAN.h>
#include <TimeLib.h>
#include <TeensyID.h>
#include "Thread.h"
#include "ThreadController.h"


Adafruit_MCP23017 ConfigExpander; //U21
Adafruit_MCP23017 PotExpander; //U33

IntervalTimer CANTimer;


//The Unique ID variable that comes from the chip
uint32_t uid[4];

uint8_t terminationSettings;


String commandPrefix;
String commandString;

#define numSettings  85
uint8_t source_address = 0xFA; 

int comp_id_index = 0;

/****************************************************************/
/*              Setup millisecond timers and intervals          */
//Declare a millisecond timer to execute the switching of the LEDs on a set time
elapsedMillis RXCAN0timer;
elapsedMillis RXCAN1orJ1708timer;
elapsedMillis analog_tx_timer;
elapsedMillis analogMillis;
elapsedMicros J1708RXtimer;
elapsedMicros microsecondsPerSecond;
elapsedMillis canComponentIDtimer;


/****************************************************************/
/*                 Binary Control Variables                       */

boolean ADJUST_MODE_ON  = 0;
boolean SAFE_TO_ADJUST = 0;
boolean displayCAN0 = 0;
boolean displayCAN1 = 0;
boolean displayCAN2 = 0;
boolean CAN0baudNotDetected = true;
boolean CAN1baudNotDetected = true;
boolean CAN2baudNotDetected = true;
boolean TXCAN = true;
boolean enableSendComponentInfo = true;
boolean sendA21voltage = false;
boolean showJ1708 = false;
boolean firstJ1708 = true;
boolean send_voltage = false;
uint16_t currentSetting = 0;
bool newJ1708Char = false;

uint8_t J1708RXbuffer[256];
uint8_t J1708_index=0;

uint32_t shortest_period = 10;

const int allFFs[8] = {255,255,255,255,255,255,255,255};
/****************************************************************/
/*                    Quadrature Knob Setup                     */

Encoder knob(encoderAPin,encoderBPin);
int32_t currentKnob = 0;

int knobLowLimit = 0;
int knobHighLimit = 255;
int knobJump = 1;

elapsedMicros LINtimer;
int LINbaud = 19200;
uint32_t LINbitTime = 52;
uint16_t LINsyncPause = 13*LINbitTime; //microseconds
bool LINbreak;
bool readyForLINsync;
bool readyForLINid;
bool readyForLINdata;
bool LINfinished;
bool readyForLINchecksum;
bool readyToTransmitShifter;
uint8_t LINindex=0;
uint8_t LINbuffer[8];
uint8_t LINchecksum;
uint8_t LINaddress;
uint8_t LINlength;
uint8_t LIN_ID;
uint8_t outByte[256]; 
uint8_t kounter;
uint8_t checksumValue;
bool sendLIN = true;
bool printLIN;

//void displayLIN(){
//  if (commandString.toInt() > 0){
//    printLIN = true;
//    
//    Serial.println("SET Stream LIN data on."); 
//  }
//  else {
//    Serial.println("SET Stream LIN data off.");
//    printLIN  = false;
//  }
//}
//void sendLINselect(){
//  if (commandString.toInt() > 0){
//    sendLIN = true;
//    
//    Serial.println("SET Turn on LIN data."); 
//  }
//  else {
//    Serial.println("SET Turn off LIN data.");
//    sendLIN  = false;
//  }
//}
//
//void determineSync(){
//  if(LINtimer > LINsyncPause) {
//    detachInterrupt(linRXpin);
//    LIN.begin(LINbaud,SERIAL_8N1);
//    readyForLINsync=true;
//  }
//}
//
//void resetLINtimer(){
//  LINtimer = 0;
//  attachInterrupt(linRXpin,determineSync,RISING);
//}
//
//void sendLINResponse(){
//  if( LIN.available() ){
//    
//    if (readyForLINsync){
//      readyForLINsync=false;
//      uint8_t LIN_Sync = 0; 
//      while (LIN_Sync != 0x55 && LINtimer < 4000){
//        LIN_Sync = LIN.read();
//      }
//      if (LIN_Sync == 0x55){
//        readyForLINid = true;
//        //if (printLIN) Serial.printf("LIN Sync: %02X\n",LIN_Sync);
//      }
//      else{
//        LINfinished = true;
//        //if (printLIN) Serial.printf("ERROR: LIN Sync: %02X\n",LIN_Sync);
//      }
//    }
//    else if (readyForLINid ){
//      LIN_ID = LIN.read();
//      readyForLINdata = true;
//      readyForLINid = false;
//      LINindex = 0;
//       
//      bool ID0 = (LIN_ID & 0b00000001) >> 0;
//      bool ID1 = (LIN_ID & 0b00000010) >> 1; 
//      bool ID2 = (LIN_ID & 0b00000100) >> 2;
//      bool ID3 = (LIN_ID & 0b00001000) >> 3; 
//      bool ID4 = (LIN_ID & 0b00010000) >> 4; 
//      bool ID5 = (LIN_ID & 0b00100000) >> 5; 
//      bool LINparity0 = (LIN_ID & 0b01000000) >> 6;
//      bool LINparity1 = (LIN_ID & 0b10000000) >> 7;
//      LINaddress = (LIN_ID & 0x0F) >> 0;
//
//      if (ID4 && ID5) LINlength = 8;
//      else if (!ID4 && ID5) LINlength = 4;
//      else if (ID4 && !ID5) LINlength = 2;
//      else if (!ID4 && !ID5) LINlength = 2;
//      
//      
//      if (LIN_ID == 0x20){
//        outByte[0] = 0x14;
//        outByte[1] = (kounter << 4) + 0x01;
//        kounter++;
//        outByte[2] = 0x02;
//        outByte[3] = 0x0D;
//        int calculatedChecksum = 0;
//        calculatedChecksum += 0x20;
//        for (int i = 0; i<4;i++){
//          calculatedChecksum +=outByte[i];
//          if (sendLIN) LIN.write(outByte[i]);
//        }
//        outByte[4] = uint8_t(~(calculatedChecksum % 255) );
//        if (sendLIN) LIN.write(outByte[4]);
//      }
//      LINfinished = true; 
//    }
//    else if (readyForLINdata){
//      
//      LINbuffer[LINindex] = LIN.read();
//      LINindex++;
//      if (LINindex == LINlength){
//        readyForLINdata = false;
//        readyForLINchecksum = true;
//         LINindex = 0;
//      }
//    }
//    else if (readyForLINchecksum ){
//      LINchecksum = LIN.read();
//      int calculatedChecksum = 0;
//      calculatedChecksum += LIN_ID & 0x3F;
//      //the inverted module-256 checksum
//      for (LINindex = 0; LINindex < LINlength; LINindex++){
//        calculatedChecksum += LINbuffer[LINindex];
//      }
//      checksumValue = uint8_t(~(calculatedChecksum % 255) );
//     
//      readyForLINchecksum = false;
//      LINfinished = true;
//      if (printLIN) Serial.printf("LIN %lu,%02X,",micros(),LIN_ID);
//      for (LINindex = 0; LINindex < LINlength; LINindex++){
//        if (printLIN) Serial.printf("%02X,",LINbuffer[LINindex]);
//      }
//      if (printLIN) Serial.printf("%02X,%02X\n",LINchecksum,checksumValue);
//      
//    }
//    else {
//      if (printLIN) Serial.printf("LIN -1,%02X\n",LIN.read());
//      else LIN.read();
//    }
//  }
//
//  //Reset the LIN responder after 20 ms of no messages
//  if( LINtimer > 50000) LINfinished = true;
//   
//  if (LINfinished && LINtimer > 9000){
//    pinMode(linRXpin,INPUT);
//    attachInterrupt(linRXpin,resetLINtimer,FALLING);
//    LINfinished = false;
//    readyForLINchecksum = false;
//    readyForLINdata = false;
//    readyForLINsync = false;
//    readyForLINid = false;
//  } 
//}

void displayVoltage(){
  analogMillis = 0;
  if (commandString.toInt() > 0){
    send_voltage = true;
    
    Serial.println("SET Stream analog in data on."); 
  }
  else {
    Serial.println("SET Stream analog In data off.");
    send_voltage = false;
  }
}


/****************************************************************/
/*                    CAN Setup                                 */
//Setup messages
#define num_default_messages  21
String default_messages[num_default_messages] = {
 "DDEC MCM 01,                   1,1,0,1,  10,   0,0,1, 8FF0001,8, 0, 0, 0, 0, 0, 0, 0, 0", //DDEC 13 MCM message, CAN1
 "DDEC TCM 01,                   2,1,0,1,  10,   0,0,1, CF00203,8, 0, 0, 0, 0, 0, 0, 0, 0", //DDEC13 Transmission controler message, CAN1
 "DDEC TCM 02,                   3,1,0,1,  10,   0,0,1, 8FF0303,8, 0, 0, 0, 0, 0, 0, 0, 0", //DDEC 13 TCM message, CAN1
 "DDEC TCM 03,                   4,1,0,1, 100,   0,0,1,18F00503,8, 0, 0, 0, 0, 0, 0, 0, 0", //Transmission on DDEC 13
 "HRW from Brake Controller,     5,1,0,0,  20,   0,0,1, CFE6E0B,8, 0, 0, 0, 0, 0, 0, 0, 0", //High Resolution wheel speed message from SA=11 (brake controller)
 "EBC1 from Cab Controller,      6,1,0,0, 100,   0,0,1,18F00131,8, 0, 0, 0, 0, 0, 0, 0, 0", // Electronic Brake Controller from SA=49
 "EBC1 from Brake Controller,    7,1,0,0, 100,   0,0,1,18F0010B,8, 0, 0, 0, 0, 0, 0, 0, 0", // Electronic Brake Controller from SA=11
 "CCVS1 from Instrument Cluster, 8,1,0,0, 100,   0,0,1,18FEF117,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise Control/Vehicle Speed from SA=23
 "CCVS1 from Cab Display 1,      9,1,0,0, 100,   0,0,1,18FEF128,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise Control/Vehicle Speed from SA=40
 "CCVS1 from Body Controller,   10,1,0,0, 100,   0,0,1,18FEF121,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise Control/Vehicle Speed from SA=33
 "CCVS1 from Cab Controller,    11,1,0,0, 100,   0,0,1,18FEF131,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise Control/Vehicle Speed from SA=49
 "CM1 from Instrument Cluster,  12,1,0,0, 100,   0,0,1,18E00017,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cab Message 1 from SA=23
 "CM1 from Climate Control 1,   13,1,0,0, 100,   0,0,1,18E00019,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cab Message 1 from SA=25
 "CM1 from Body Controller,     14,1,0,0, 100,   0,0,1,18E00021,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cab Message 1 from SA=33
 "CM1 from Cab Display,         15,1,0,0, 100,   0,0,1,18E00028,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cab Message 1 from SA=40
 "CM1 from Cab Controller,      16,1,0,0, 100,   0,0,1,18E00031,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cab Message 1 from SA=49
 "PTO from Instrument Cluster,  17,1,0,0, 100,   0,0,1,18FEF017,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise and PTO setup from SA=23
 "PTO from Body Controller,     18,1,0,0, 100,   0,0,1,18FEF021,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise and PTO setup from SA=33
 "PTO from Cab Display,         19,1,0,0, 100,   0,0,1,18FEF028,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise and PTO setup from SA=40
 "PTO from Cab Controller,      20,1,0,0, 100,   0,0,1,18FEF031,8, 0, 0, 0, 0, 0, 0, 0, 0", //Cruise and PTO setup from SA=49
 "AMB from Body Controller,     21,1,0,0,1000,   0,0,1,18FEF521,8, 0, 0, 0, 0, 0, 0, 0, 0" //Ambient Conditions
};




//Set up the CAN data structures
static CAN_message_t rxmsg;
static CAN_message_t txmsg;
static CAN_message_t temp_txmsg;

//set up a counter for each received message
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

int CANWaitTimeout = 200;
uint32_t BAUDRATE0 = 250000;
uint32_t BAUDRATE1 = 250000;
const uint32_t baudRateList[5] = {250000,500000,666666,125000,1000000};
uint8_t baudRateIndex0 = 0;
uint8_t baudRateIndex1 = 0;


CAN_filter_t allPassFilter;

// Create a thread controller class
ThreadController can_thread_controller = ThreadController();

void runCANthreads(){
  can_thread_controller.run();
}


class CanThread: public Thread
{
public:
  uint32_t stop_after_count;
  uint32_t transmit_number = 0;
  boolean ok_to_send = true;
  uint32_t loop_cycles = 0; 
  uint32_t cycle_count = 0;
  
  uint8_t channel = 0;
 
  CAN_message_t txmsg;
    
  uint8_t num_messages = 1; 
  uint8_t message_index = 0;
  uint8_t message_list[256][8] = {};
  uint32_t id_list[256]={};
  String ThreadName;
  
  
  bool shouldRun(unsigned long time){
    if (stop_after_count > 0){
      if (transmit_number >= stop_after_count) enabled = false;
    }
    return Thread::shouldRun(time);
  }
  
  void run(){
    //Set the CAN message data to the next one in the list.
    txmsg.id = id_list[message_index];
    memcpy(txmsg.buf,message_list[message_index],8);

    //Write the data to the CAN bus.
    if (ok_to_send && ignitionCtlState){
      if      (channel == 0) Can0.write(txmsg);
      else if (channel == 1) Can1.write(txmsg);
      
      transmit_number++;
      message_index++;
    }
    cycle_count++;
    if (message_index >= num_messages ) {
      message_index = 0; 
    }
    if (cycle_count >= num_messages) {
      ok_to_send = false;
    }
    if (cycle_count*interval >= loop_cycles){
      cycle_count = 0;
      ok_to_send = true;
    }
  
    
    return Thread::run();
  }
};

//Setup a container to keep track of the individual CAN threads
CanThread* can_messages[MAX_THREADS] ={};

void set_shortest_period(){
  if (commandString.length() > 0){
    shortest_period = commandString.toInt();
    Serial.printf("SET Shortest CAN Broadcast Period to %lu milliseconds.\n",shortest_period);
  }
  else
     Serial.printf("INFO Shortest CAN Broadcast Period is %lu milliseconds.\n",shortest_period);
}

void getThreadName(){
  int index = commandString.toInt();
  Serial.printf("NAME of CAN Thread %d: ",index); 
  Serial.println(can_messages[index]->ThreadName);
}
void getAllThreadNames(){
  int threadSize =  can_thread_controller.size(false);
  for(int i = 0; i<threadSize; i++){
    Serial.printf("NAME of CAN Thread %d: ",i); 
    Serial.println(can_messages[i]->ThreadName);
  }
  
}


void getThreadSize(){
  int threadSize =  can_thread_controller.size(false);
  Serial.printf("INFO Size of CAN Thread = %d\n",threadSize); 
}

int setupPeriodicCANMessage(){
  CANTimer.end();
  CanThread* can_message;
  int index;
  int sub_index;
  int channel;
  uint32_t tx_period;
  uint32_t tx_delay;
  uint8_t num_messages=1;
  uint32_t stop_after_count;
   
  int threadSize =  can_thread_controller.size(false);
  char commandBytes[256];
  commandString.toCharArray(commandBytes,256);
  char delimiter[] = ",";
  char* commandValues;

  commandValues = strtok(commandBytes, delimiter);
  String threadName = commandValues;
  
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    index = constrain(atoi(commandValues),0,threadSize);
  }
  else {
    Serial.println(F("ERROR SM command is missing arguments.")); 
    return -1;
  }

  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    num_messages = constrain(atoi(commandValues),1,255);
  }
  else {
    Serial.println(F("ERROR SM command not able to determine the number of sub messages.")); 
    return -2;
  }


  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
     //constrain the sub_index to be one larger than the 
     sub_index = constrain(atoi(commandValues),0,num_messages-1);
  }
  else {
    Serial.println(F("ERROR SM command missing sub_index.")); 
    return -3;
  }
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    channel = constrain(atoi(commandValues),0,1);
  }
  else {
    Serial.println(F("ERROR SM command not able to set CAN Channel.")); 
    return -4;
  }
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    tx_period = constrain(strtoul(commandValues,NULL,10),shortest_period,0xFFFFFFFF);
  }
  else {
    Serial.println(F("ERROR SM command not able to set period information.")); 
    return -5;
  }

  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    tx_delay = strtoul(commandValues,NULL,10);
  }
  else {
    Serial.println(F("ERROR SM command not able to set delay information.")); 
    return -6;
  }
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    stop_after_count = strtoul(commandValues,NULL,10);
  }
  else {
    Serial.println(F("ERROR SM command not able to set the total number count.")); 
    return -7;
  }
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    temp_txmsg.ext = constrain(atoi(commandValues),0,1);
  }
  else {
    Serial.println(F("ERROR SM command not able to set extended ID flag.")); 
    temp_txmsg.ext = 1;
  }

  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    temp_txmsg.id = strtol(commandValues,NULL,16);
  }
  else {
    Serial.println(F("WARNING SM command not able to set CAN ID information."));
    temp_txmsg.id = 0x3FFFFFFF ;
  }
  
  commandValues = strtok(NULL, delimiter);
  if (commandValues != NULL) {
    temp_txmsg.len = constrain(atoi(commandValues),0,8);
  }
  else {
    Serial.println(F("WARNING SM command not able to set CAN data length code."));
    temp_txmsg.len = 8;
  }
 
  //memset(temp_txmsg.buf,allFFs,8);
  for (int i = 0; i < temp_txmsg.len; i++){
    commandValues = strtok(NULL, delimiter);
    if (commandValues != NULL) {
      temp_txmsg.buf[i] = constrain(strtol(commandValues,NULL,16),0,255);
    }
    else {
      temp_txmsg.buf[i] = 0xFF;
      Serial.printf("WARNING SM command not able to set CAN data byte in position %d.\n",i);
    }
  }
  char threadNameChars[256]; 
  threadName.toCharArray(threadNameChars,threadName.length()+1);
  
  Serial.printf("SET CAN name=%s, i=%d, n=%d, j=%d, c=%d, p=%d, d=%d, t=%d, e=%d, ID=%08X, DLC=%d, DATA=[",
                 threadNameChars,index, num_messages, sub_index, channel, tx_period, tx_delay, stop_after_count, temp_txmsg.ext, temp_txmsg.id,temp_txmsg.len);
  for (int i = 0; i < temp_txmsg.len-1; i++){
    Serial.printf("%02X, ",temp_txmsg.buf[i]);
  }
  Serial.printf("%02X]\n",temp_txmsg.buf[temp_txmsg.len-1]);
  
  if (index == threadSize) { //Create a new entry
    Serial.printf("THREAD %lu, %s (NEW)\n",index,threadNameChars); 
    CanThread* can_message = new CanThread(); 
    can_thread_controller.add(can_message);
    can_messages[index] = can_message;
    can_messages[index]->enabled = false;
  }
  else{
   Serial.printf("THREAD %lu, %s (EXISTING)\n",index,threadNameChars); 
  }

  can_messages[index]->channel = channel;
  can_messages[index]->txmsg.ext = temp_txmsg.ext;
  can_messages[index]->txmsg.len = temp_txmsg.len;
  can_messages[index]->id_list[sub_index] = temp_txmsg.id;
  for (int i = 0; i < temp_txmsg.len; i++) {
    can_messages[index]->message_list[sub_index][i] = temp_txmsg.buf[i];
  }
  can_messages[index]->stop_after_count = stop_after_count;
  can_messages[index]->transmit_number = 0;
  can_messages[index]->cycle_count = 0;
  can_messages[index]->message_index = 0;
  can_messages[index]->num_messages = num_messages;  
  can_messages[index]->setInterval(tx_period);
  can_messages[index]->loop_cycles =  tx_delay ;
  can_messages[index]->ThreadName = threadName;
  
  CANTimer.begin(runCANthreads, 500);
  return index;
}

void stopCAN(){
  int threadSize =  can_thread_controller.size(false);
  for (int i = 0; i < threadSize; i++) {
    can_messages[i]->enabled = false;
    }
  Serial.println(F("INFO Stopped all CAN transmission."));
}

void clearCAN(){
  int threadSize =  can_thread_controller.size(false);
  for (int i = 0; i < sizeof(can_messages); i++) {
    delete can_messages[i] ;
    }
  can_thread_controller.clear();
  Serial.println(F("INFO Cleared the CAN transmission thread. All messages must be reloaded."));
}

void goCAN(){
  int threadSize =  can_thread_controller.size(false);
  for (int i = 0; i < threadSize; i++) {
    can_messages[i]->enabled = true;
    can_messages[i]->transmit_number = 0;
    can_messages[i]->message_index = 0;
    can_messages[i]->cycle_count = 0;
  }
  Serial.println(F("INFO Started all CAN transmissions."));
}

void startCAN (){
  int threadSize =  can_thread_controller.size(false);
  if (threadSize > 0){
    char commandBytes[10];
    commandString.toCharArray(commandBytes,10);
    char delimiter[] = ",";
    char* commandValues;
    commandValues = strtok(commandBytes, delimiter);
    int index = constrain(atoi(commandValues),0,threadSize-1);
    
    commandValues = strtok(NULL, delimiter);
    int setting = atoi(commandValues);
    
    if (setting > 0) {
      can_messages[index]->enabled = true;
      can_messages[index]->transmit_number = 0;
      can_messages[index]->message_index = 0;
      can_messages[index]->cycle_count = 0;
      Serial.printf("SET CAN message %d with ID 0x%08X on.\n",index,can_messages[index]->id_list[0]); 
    }
    else  {
      can_messages[index]->enabled = false;
      Serial.printf("SET CAN message %d with ID 0x%08X off.\n",index,can_messages[index]->id_list[0]); 
    }
  }
  else
  {
     Serial.println("ERROR No CAN Messages Setup to turn on.");
  }
}

class settingsData {
public:
  uint32_t knobLowLimit = 0;
  uint32_t knobHighLimit = 255;
  uint32_t knobJump = 1;
  String settingName = "";
  String settingPort = "";
  
};



void connectionString(boolean switchState) {
  //memset(displayBuffer,0,sizeof(displayBuffer));
  Serial.print(switchState);
  if (switchState) strcpy(displayBuffer, " Connected");
  else strcpy(displayBuffer, " Open");
}

void terminalString(uint8_t setting) {
  switch (setting) {
    case TCON_B_ONLY:
      strcpy(displayBuffer, " 1 (TCON_B_ONLY)");
      break;
    case TCON_WIPER_ONLY:
      strcpy(displayBuffer, "2 (TCON_WIPER_ONLY)");
      break;
    case TCON_WIPER_AND_B:
      strcpy(displayBuffer, "3 (TCON_WIPER_AND_B)");
      break;
    case TCON_A_ONLY:
      strcpy(displayBuffer, "4 (TCON_A_ONLY)");
      break;
    case TCON_A_AND_B :
      strcpy(displayBuffer, "5 (TCON_A_AND_B)");
      break;
    case TCON_WIPER_AND_A :
      strcpy(displayBuffer, "6 (TCON_WIPER_AND_A)");
      break;
    case TCON_CONNECT_ALL:
      strcpy(displayBuffer, "7 (TCON_CONNECT_ALL)");
      break;
    default:
      strcpy(displayBuffer, "0 Nothing connected");
      break;
  }
}


/**********************************************************************/
/*   Utility functions to manipulate Ports                            */

uint8_t setTerminationSwitches() {
  //Set the termination Switches of U29 on Rev 3

  uint8_t terminationSettings =  uint8_t( CAN0term | CAN1term << 1 | CAN2term << 2 |  LINmaster << 3 | 
                                 PWM1Out << 4 | PWM2Out << 5 | PWM3Out << 6 | PWM4Out << 7);
  if (revision.equals("02")){ //Termination switches are on U21 on Rev 2
      terminationSettings = U1U2P0ASwitch | U3U4P0ASwitch << 1 | U5U6P0ASwitch << 2 | U7U8P0ASwitch << 3 |
                            CAN0term << 4 | CAN1term << 5 | CAN2term << 6 | LINmaster <<7;
  digitalWrite(CStermPin, LOW);
  delay(1);
  SPI1.transfer(terminationSettings);
  delay(1);
  digitalWrite(CStermPin, HIGH);
  return terminationSettings;
  }
  digitalWrite(CStermPin, LOW);
  delay(1);
  SPI.transfer(terminationSettings);
  delay(1);
  digitalWrite(CStermPin, HIGH);
  return terminationSettings;
}

void getTerminationSwitches(uint8_t terminationSettings) {
  //Set the termination Switches for U21
  CAN0term  = (terminationSettings & 0b00000001) >> 0;
  CAN1term  = (terminationSettings & 0b00000010) >> 1;
  CAN2term  = (terminationSettings & 0b00000100) >> 2;
  LINmaster = (terminationSettings & 0b00001000) >> 3;
  PWM1Out   = (terminationSettings & 0b00010000) >> 4;
  PWM2Out   = (terminationSettings & 0b00100000) >> 5;
  PWM3Out   = (terminationSettings & 0b01000000) >> 6;
  PWM4Out   = (terminationSettings & 0b10000000) >> 7;
}


uint16_t setConfigSwitches() {
  //Set the termination Switches of U21 on Rev 3 based on the boolean values of the variables representing the GPIO pins of U21
  if (revision.equals("02")){
    uint8_t configSwitchSettings = PWM1Out | PWM2Out << 1 | CAN2Switch << 2 | CAN2Switch << 3 | LIN1Switch << 4 | !LIN1Switch << 5 | LIN2Switch << 6 | !LIN2Switch << 7;
    digitalWrite(CSVoutPin, LOW);
    delay(1);
    SPI1.transfer(configSwitchSettings);
    delay(1);
    digitalWrite(CSVoutPin, HIGH);
  }
  uint16_t configSwitchSettings =  
              LIN1Switch | LIN2Switch  << 1 | P10or19Switch << 2 |  P15or18Switch << 3 | !U1though8Enable << 4 | !U9though16Enable << 5 |
              CAN1Switch << 6 | CAN2Switch << 7 | U1U2P0ASwitch << 8 |  U3U4P0ASwitch  << 9 |  U5U6P0ASwitch  << 10 | U7U8P0ASwitch << 11 |
              U9U10P0ASwitch << 12 | U11U12P0ASwitch << 13 | U13U14P0ASwitch << 14 | U15U16P0ASwitch << 15;
  ConfigExpander.writeGPIOAB(configSwitchSettings);
  return configSwitchSettings;
}

void getConfigSwitches(uint16_t configSwitchSettings) {
  //get the termination Switches for U21 from the config switch settings. 
  //Requires a 16 bit number that represents the GPIO Pins on chip U21
  //the Booleans must be previously declared
  
  LIN1Switch        = (configSwitchSettings & 0b0000000000000001) >> 0;
  LIN2Switch        = (configSwitchSettings & 0b0000000000000010) >> 1;
  P10or19Switch     = (configSwitchSettings & 0b0000000000000100) >> 2;
  P15or18Switch     = (configSwitchSettings & 0b0000000000001000) >> 3;
  U1though8Enable   = (configSwitchSettings & 0b0000000000010000) >> 4;
  U9though16Enable  = (configSwitchSettings & 0b0000000000100000) >> 5;
  CAN1Switch        = (configSwitchSettings & 0b0000000001000000) >> 6;
  CAN2Switch        = (configSwitchSettings & 0b0000000010000000) >> 7;
  U1U2P0ASwitch     = (configSwitchSettings & 0b0000000100000000) >> 9;
  U3U4P0ASwitch     = (configSwitchSettings & 0b0000001000000000) >> 9;
  U5U6P0ASwitch     = (configSwitchSettings & 0b0000010000000000) >> 10;
  U7U8P0ASwitch     = (configSwitchSettings & 0b0000100000000000) >> 11;
  U9U10P0ASwitch    = (configSwitchSettings & 0b0001000000000000) >> 12;
  U11U12P0ASwitch   = (configSwitchSettings & 0b0010000000000000) >> 13;
  U13U14P0ASwitch   = (configSwitchSettings & 0b0100000000000000) >> 14;
  U15U16P0ASwitch   = (configSwitchSettings & 0b1000000000000000) >> 15;
}


/****************************************************************/
/*   Begin Function Calls for Digital Potentiometer             */
  
uint8_t MCP41HVExtender_SetTerminals(uint8_t pin, uint8_t TCON_Value) {
  if(revision.equals("02")){
    digitalWrite(CSpotsPin, LOW);
    SPI1.transfer(ExtenderOpCode);
    uint8_t latReg = 0x0A;
    if (pin >= 8){
      pin -= 8;
      latReg = 0x1A;
    }
    SPI1.transfer(latReg);
    SPI1.transfer(bitPositions[pin]);
    digitalWrite(CSpotsPin, HIGH);
    delay(1);

    SPI1.transfer(0x40);
    SPI1.transfer(TCON_Value + 8);
    SPI1.transfer(0x4C);
    uint8_t result = SPI.transfer(0xFF);

    delay(1);
    digitalWrite(CSpotsPin, LOW);
    SPI1.transfer(ExtenderOpCode);
    SPI1.transfer(latReg);
    SPI1.transfer(0xFF);
    digitalWrite(CSpotsPin, HIGH);
    delay(1);
    return result & 0x07;
  }
  PotExpander.writeGPIOAB(~(1 << pin));
  delay(1);
  SPI.transfer(0x40); //Write to TCON Register
  SPI.transfer(TCON_Value + 8);
  SPI.transfer(0x4C); //Read Command
  uint8_t result = SPI.transfer(0xFF); //Read Terminal Connection (TCON) Register
  PotExpander.writeGPIOAB(0xFF);
  return result & 0x07;
}



uint8_t MCP41HVExtender_SetWiper(uint8_t pin, uint8_t potValue)
{
  if(revision.equals("02")){ //extender chip uses SPI
    digitalWrite(CSpotsPin, LOW);//select this CS
    SPI1.transfer(ExtenderOpCode);//write
    uint8_t latReg = 0x0A;//pins 1-8
    if (pin >= 8) {
      pin -= 8;
      latReg = 0x1A;//pins 9-16
    }
    SPI1.transfer(latReg); //to this register
    SPI1.transfer(bitPositions[pin]); //set CS pin for potentiometer
    digitalWrite(CSpotsPin, HIGH);
    delay(1);

    SPI1.transfer(0x00); //write to wiper register
    SPI1.transfer(potValue);//this value
    uint8_t result = SPI1.transfer(0xFF);

    digitalWrite(CSpotsPin, LOW);
    SPI1.transfer(ExtenderOpCode);//write
    SPI1.transfer(latReg);//the same register
    SPI1.transfer(0xFF);//reset all pins to high
    digitalWrite(CSpotsPin, HIGH);
    delay(1);
    return potValue; //reading register isn't working
  }
  //otherwise the chip uses I2C
  PotExpander.writeGPIOAB(~(1 << pin));
  delay(1);
  SPI.transfer(0x00); //Write to wiper Register
  SPI.transfer(potValue);
  SPI.transfer(0x0C); //Read command
  uint8_t result = SPI.transfer(0xFF); //Read Wiper Register
  PotExpander.writeGPIOAB(0xFF);
  return result;
}

uint8_t MCP41HV_SetWiper(uint8_t pin, uint8_t potValue)
{
  digitalWrite(CShvAdjPin, LOW);
  delay(1);
  SPI1.transfer(0x00);
  SPI1.transfer(potValue);
  digitalWrite(CShvAdjPin, HIGH);
  delay(1);
  return 0;
}


uint8_t MCP41HVI2C_SetTerminals(uint8_t addr, uint8_t TCON_Value) {
  
  Wire.beginTransmission(addr); 
  Wire.write(byte(0x40));            // sends instruction byte  
  Wire.write(0xF8 | TCON_Value);             // sends potentiometer value byte  
  Wire.endTransmission();

  Wire.requestFrom(addr,2);    // request 6 bytes from slave device #8
  uint8_t result = Wire.read(); //Read Wiper Register
  result = Wire.read(); //Read Wiper Register
  return result & 0x07;
}

uint8_t MCP41HVI2C_SetWiper(uint8_t addr, uint8_t potValue)
{
  Wire.beginTransmission(addr); 
  Wire.write(byte(0x00));            // sends instruction byte  
  Wire.write(potValue);             // sends potentiometer value byte  
  Wire.endTransmission();

  Wire.requestFrom(addr,2);    // request 6 bytes from slave device #8
  uint8_t result = Wire.read(); //Read Wiper Register, first byt is all 0
  result = Wire.read(); //Read Wiper Register
  return result;
} 

/*   End Function Calls for Digital Potentiometer               */
/****************************************************************/

/****************************************************************/
/*   Begin Function Calls for DAC                               */

void initializeDACs(uint8_t address) {
  Serial.print("Setting DAC Internal Reference register with address of 0x");
  Serial.println(address, HEX);
  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b10000000);
  Wire.write(0x00);
  Wire.write(0x10);
  Wire.endTransmission();

  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b10000000);
  Wire.endTransmission();

  Wire.requestFrom(address, 2);
  char highC = Wire.read();
  char lowC = Wire.read();
  Serial.print("Internal Reference Register: ");
  Serial.println(lowC);         // print the character

  Serial.println("Setting LDAC register.");
  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b01100000);
  Wire.write(0xFF);
  Wire.write(0xFF);
  Wire.endTransmission();

  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b01100000);
  Wire.endTransmission();

  Wire.requestFrom(address, 2);
  highC = Wire.read();
  lowC = Wire.read();
  Serial.print("LDAC Register: ");
  Serial.println(lowC, HEX);        // print the character

  Serial.println("Setting DAC Power Down register to On.");
  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b01000000);
  Wire.write(0b00011111);
  Wire.write(0xFF);
  Wire.endTransmission();

  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b01000000);
  Wire.endTransmission();

  Wire.requestFrom(address, 2);
  highC = Wire.read();
  lowC = Wire.read();
  Serial.print("Power Register: ");
  Serial.println(lowC, BIN);        // print the character

  Serial.print(F("Done with DAC at address 0x"));
  Serial.println(address, HEX);
}

uint16_t setDAC(uint16_t setting, uint8_t DACchannel, uint8_t address) {
  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b00100000 + DACchannel);
  Wire.write((setting & 0x0FF0) >> 4);
  Wire.write((setting & 0x000F) << 4);
  Wire.endTransmission();

  Wire.beginTransmission(address);   // Slave address
  Wire.write(0b00010000 + DACchannel);
  Wire.endTransmission();

  Wire.requestFrom(address, 2);
  uint8_t highC = Wire.read();
  uint8_t lowC =  Wire.read();
  uint16_t result = (highC << 4) + (lowC >> 4);
  return result;
}

/*   End Function Calls for DAC                              */
/****************************************************************/

/********************************************************************/
/*               Begin Define Settings                              */
/*
 * To add another setting, update the numSettings, populate the settingNames and settingPins array and add 
 * new knob limits. The knob limits are used for value checking. These are for numerical Settings.
 */


char settingNames[numSettings][40] = {
  "Nothing Selected", //0
  "Digital Potentiometer  1 Wiper", //1
  "Digital Potentiometer  2 Wiper",
  "Digital Potentiometer  3 Wiper",
  "Digital Potentiometer  4 Wiper",
  "Digital Potentiometer  5 Wiper",
  "Digital Potentiometer  6 Wiper",
  "Digital Potentiometer  7 Wiper",
  "Digital Potentiometer  8 Wiper",
  "Digital Potentiometer  9 Wiper",
  "Digital Potentiometer 10 Wiper",
  "Digital Potentiometer 11 Wiper",
  "Digital Potentiometer 12 Wiper",
  "Digital Potentiometer 13 Wiper",
  "Digital Potentiometer 14 Wiper",
  "Digital Potentiometer 15 Wiper",
  "Digital Potentiometer 16 Wiper", //16

  "Vout2-A", //17
  "Vout2-B",
  "Vout2-C",
  "Vout2-D",
  "Vout2-E",
  "Vout2-F",
  "Vout2-G",
  "Vout2-H", //24

  "U1 & U2 P0A", //25
  "U3 & U4 P0A", 
  "U5 & U5 P0A", 
  "U7 & U2 P0A", 
  "U9 & U10 P0A", 
  "U11 & U12 P0A", 
  "U13 & U14 P0A", 
  "U15 & U16 P0A", //32
  
  "PWM 1", //33
  "PWM 2",
  "PWM 3",
  "PWM 4", //36

  "Port 10 or 19", //37
  "Port 15 or 18",
  "CAN1 or J1708",
  "Port 31 & 32 or CAN2",
  "CAN0 Termination Resistor",
  "CAN1 Termination Resistor",
  "CAN2 Termination Resistor",
  "LIN Master Pullup Resistor", //44

  "12V Out 1 (H-Bridge)", //45
  "12V Out 2 (H-Bridge)",
  "Ground Out 1 (H-Bridge)",
  "Ground Out 2 (H-Bridge)", //48

  "High Voltage Adjustable Output", //49
  "Ignition Relay",
  
  "Dig. Pot.  1 Terminal Connect", //51
  "Dig. Pot.  2 Terminal Connect",
  "Dig. Pot.  3 Terminal Connect",
  "Dig. Pot.  4 Terminal Connect",
  "Dig. Pot.  5 Terminal Connect",
  "Dig. Pot.  6 Terminal Connect",
  "Dig. Pot.  7 Terminal Connect",
  "Dig. Pot.  8 Terminal Connect",
  "Dig. Pot.  9 Terminal Connect",
  "Dig. Pot. 10 Terminal Connect",
  "Dig. Pot. 11 Terminal Connect",
  "Dig. Pot. 12 Terminal Connect",
  "Dig. Pot. 13 Terminal Connect",
  "Dig. Pot. 14 Terminal Connect",
  "Dig. Pot. 15 Terminal Connect",
  "Dig. Pot. 16 Terminal Connect", //66

  "PWM1 Connect", //67
  "PWM2 Connect",
  "PWM3 Connect",
  "PWM4 Connect",
  "LIN to Shield Connect", //U21 GPA0
  "LIN to Port 16 Connect", //U21 GPA1
  "U28 (U1-U8)  P0A Enable",
  "U31 (U9-U16) P0A Enable", //74
  
  "Digital Potentiometer 28 Wiper", //75
  "Digital Potentiometer 29 Wiper",
  "Digital Potentiometer 30 Wiper",
  
  "Dig. Pot. 28 Terminal Connect",
  "Dig. Pot. 29 Terminal Connect",
  "Dig. Pot. 30 Terminal Connect", //80

  "PWM1 Frequency",
  "PWM2 Frequency",
  "PWM3 Frequency",
  "PWM4 Frequency"
  
};

char settingPins[numSettings][40] = {
  "\n",
  "Port  1 (J24- 1)",
  "Port  2 (J24- 2)",
  "Port  3 (J24- 3)",
  "Port  4 (J24- 4)",
  "Port  5 (J24- 5)",
  "Port  6 (J24- 6)",
  "Port  7 (J24- 7)",
  "Port  8 (J24- 8)",
  "Port  9 (J24- 9)",
  "Port 10 (J24-10)",
  "Port 11 (J24-11)",
  "Port 12 (J24-12)",
  "Port 13 (J18-11)",
  "Port 14 (J18-12)",
  "Port 15 (J24-15)",
  "Port 16 (J24-16)",

  "Port 18 (J18- 2)",
  "Port 19 (J18- 3)",
  "Port 20 (J18- 4)",
  "Port 21 (J18- 5)",
  "Port 22 (J18- 6)",
  "Port 23 (J18- 7)",
  "Port 24 (J18- 8)",
  "Port 25 (J18- 9)",

  "Ports  1 and 2",
  "Ports  3 and 4",
  "Ports  5 and 6",
  "Ports  7 and 8",
  "Ports  9 and 10",
  "Ports 11 and 12",
  "Ports 13 and 14",
  "Ports 15 and 16",

  "Ports 13 (J24-13) and 31 (J18-15)",
  "Ports 14 (J24-14) and 32 (J18-16)",
  "Port 27 (J18-10)",
  "Port 17 (J18-1)",
  
  "(J24-10)",
  "(J24-15)",
  "(J24-17 & J24-18)",
  "(J18-15 and J18-16)",
  "R44",
  "R45",
  "R46",
  "R59",

  "Port 26 (J18-10)",
  "Port 11 (J24-11)",
  "Port 17 (J18- 1)",
  "Port 12 (J24-12)",

  "(J24-19 and J18-11)",
  "(J24-20)",
  
  "Port  1 (J24- 1)",
  "Port  2 (J24- 2)",
  "Port  3 (J24- 3)",
  "Port  4 (J24- 4)",
  "Port  5 (J24- 5)",
  "Port  6 (J24- 6)",
  "Port  7 (J24- 7)",
  "Port  8 (J24- 8)",
  "Port  9 (J24- 9)",
  "Port 10 (J24-10)",
  "Port 11 (J24-11)",
  "Port 12 (J24-12)",
  "Port 13 (J18-11)",
  "Port 14 (J18-12)",
  "Port 15 (J24-15)",
  "Port 16 (J24-16)",

  "Port 13 (J24-13)",
  "Port 14 (J24-14)",
  "Port 27 (J18-10)",
  "Port 17 (J18- 1)",
  "(J10- 5)",
  "Port 16 (J24-16)",
  "(J24-1 to J24-8)",
  "(J24-9 to J24-16)",
  
  "Port 28 (J18-12)",
  "Port 29 (J18-13)",
  "Port 30 (J18-14)",
  
  "Port 28 (J18-12)",
  "Port 29 (J18-13)",
  "Port 30 (J18-14)",

  "Port 13 (J24-13)",
  "Port 14 (J24-14)",
  "Port 27 (J18-10)",
  "Port 17 (J18-1)"
};


void setLimits(uint8_t settingNum) {
  if (settingNum > 0 && settingNum <= 16) {
    knobLowLimit = 0;
    knobHighLimit = 255;
    knobJump = 1;
  }
  else if (settingNum > 16 && settingNum <= 24) {
    knobLowLimit = 0;
    knobHighLimit = 4095;
    knobJump = 20;
  }
  else if (settingNum > 24 && settingNum <= 32) {
    knobLowLimit = 0;
    knobHighLimit = 1;
    knobJump = 1;
  }
  else if (settingNum > 32 && settingNum <= 36) {
    knobLowLimit = 0;
    knobHighLimit = 4096;
    knobJump = 1;
  }
  else if (settingNum > 36 && settingNum <= 48) {
    knobLowLimit = 0;
    knobHighLimit = 1;
    knobJump = 1;
  }
  else if (settingNum == 49) {
    knobLowLimit = 0;
    knobHighLimit = 255;
    knobJump = 1;
  }
  else if (settingNum == 50) {
    knobLowLimit = 0;
    knobHighLimit = 1;
    knobJump = 1;
  }
 
  else if (settingNum >= 51 && settingNum <= 66) {
    knobLowLimit = 0;
    knobHighLimit = 7;
    knobJump = 1;
  }
  else if (settingNum >= 67 && settingNum <= 74) {
    knobLowLimit = 0;
    knobHighLimit = 1;
    knobJump = 1;
  }
  else if (settingNum > 74 && settingNum <= 77) {
    knobLowLimit = 0;
    knobHighLimit = 255;
    knobJump = 1;
  }
  else if (settingNum >= 77 && settingNum <= 80) {
    knobLowLimit = 0;
    knobHighLimit = 7;
    knobJump = 1;
  }
  else if (settingNum >= 81 && settingNum <= 84) {
    knobLowLimit = 0;
    knobHighLimit = 32768;
    knobJump = 1;
  }
  else {
    knobLowLimit = 1;
    knobHighLimit = numSettings-1;
    knobJump = 1;
  }

  // Serial.print("Low Limit: ");
  // Serial.println(knobLowLimit);
  // Serial.print("High Limit: ");
  // Serial.println(knobHighLimit);
}
int16_t fakePwm(int settingNum, float freq){
    float period = 1./freq;
    Serial.println(period/2. * 1000 - 0.9);
    float floatTimeout = period/2. * 1000 - 1.05;
    uint32_t timeout = (period/2.)*1000 - 1.05;
    uint32_t microTimeout = (floatTimeout - timeout)*1000;
    Serial.println(floatTimeout);
    Serial.println(timeout);
    Serial.println(microTimeout);
    setDAC(4095, settingNum - 17, Vout2address);
    delay(timeout);
    delayMicroseconds(microTimeout);
    setDAC(0, settingNum - 17, Vout2address);
    delay(timeout);
    delayMicroseconds(microTimeout);
}

int16_t setSetting(uint8_t settingNum, int settingValue, bool debugDisplay) {
  
  
  if (debugDisplay){
    Serial.print(settingNum);
    Serial.print(", ");
    Serial.print(settingNames[settingNum]);
    Serial.print(", ");
    Serial.print(settingPins[settingNum]);
    Serial.print(" = ");
  }
  if (settingNum > 0 && settingNum <= 16){
    if (settingValue > -1) SPIpotWiperSettings[settingNum - 1] = settingValue; 
    uint8_t w_position = MCP41HVExtender_SetWiper(settingNum - 1, 
                                                SPIpotWiperSettings[settingNum - 1]);
    if (debugDisplay) {
        Serial.print(w_position);
        Serial.print(", ");
        Serial.println(SPIpotWiperSettings[settingNum - 1]);
    }
    return w_position;
  }
  else if (settingNum > 16 && settingNum <= 19){ //make a psuedo pwm signal
    Serial.print("SETTING: ");
    Serial.println(settingValue);
    if (settingValue > -1){
      fakePwmFrequencies[settingNum - 17] = settingValue;
//      float period = 1./fakePwmFrequencies[settingNum - 17] - 0.0018;
//      fakePwmTimeouts[settingNum - 17] = settingValue;//period/2.;
    }
    if (debugDisplay) Serial.println(fakePwmFrequencies[settingNum - 17]); 
    return fakePwmFrequencies[settingNum - 17];
  }
  else if (settingNum > 19 && settingNum <= 24) {
    if (settingValue > -1) DAC2value[settingNum - 17] = settingValue;
    setDAC(DAC2value[settingNum - 17], settingNum - 17, Vout2address);
    if (debugDisplay) Serial.println(DAC2value[settingNum - 17]); 
    return DAC2value[settingNum - 17];
  }
  else if (settingNum == 25){
    if (settingValue > -1) U1U2P0ASwitch = boolean(settingValue);
    if (revision.equals("02")) setTerminationSwitches();
    else setConfigSwitches();
    if (debugDisplay) {
        connectionString(U1U2P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U1U2P0ASwitch;
  }
  else if (settingNum == 26){
    if (settingValue > -1) U3U4P0ASwitch = boolean(settingValue);
    if (revision.equals("02")) setTerminationSwitches();
    else setConfigSwitches();
    if (debugDisplay) {
        connectionString(U3U4P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U3U4P0ASwitch;
  }
  else if (settingNum == 27){
    if (settingValue > -1) U5U6P0ASwitch = boolean(settingValue);
    if (revision.equals("02")) setTerminationSwitches();
    else setConfigSwitches();
    if (debugDisplay) {
        connectionString(U5U6P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U5U6P0ASwitch;
  }
  else if (settingNum == 28){
    if (settingValue > -1) U7U8P0ASwitch = boolean(settingValue);
    if (revision.equals("02")) setTerminationSwitches();
    else setConfigSwitches();
    if (debugDisplay) {
        connectionString(U7U8P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U7U8P0ASwitch;
  } 
  else if (settingNum == 29){
    if (settingValue > -1) U9U10P0ASwitch = boolean(settingValue);
    if (revision.equals("02")){
      if (settingValue > 0) DAC3value[settingNum - 25] = 4095;
      else DAC3value[settingNum - 25] = 0;
      setDAC(DAC3value[settingNum - 25], settingNum - 25, Vout3address);
    }
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U9U10P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U9U10P0ASwitch;
  }  
  else if (settingNum == 30){
    if (settingValue > -1) U11U12P0ASwitch = boolean(settingValue);
    if (revision.equals("02")){
      if (settingValue > 0) DAC3value[settingNum - 25] = 4095;
      else DAC3value[settingNum - 25] = 0;
      setDAC(DAC3value[settingNum - 25], settingNum - 25, Vout3address);
    }
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U11U12P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U11U12P0ASwitch;
  }  
  else if (settingNum == 31){
    if (settingValue > -1) U13U14P0ASwitch = boolean(settingValue);
    if (revision.equals("02")){
      if (settingValue > 0) DAC3value[settingNum - 25] = 4095;
      else DAC3value[settingNum - 25] = 0;
      setDAC(DAC3value[settingNum - 25], settingNum - 25, Vout3address);
    }
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U13U14P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U13U14P0ASwitch;
  }   
  else if (settingNum == 32){
    if (settingValue > -1) U15U16P0ASwitch = boolean(settingValue);
    if (revision.equals("02")){
      if (settingValue > 0) DAC3value[settingNum - 25] = 4095;
      else DAC3value[settingNum - 25] = 0;
      setDAC(DAC3value[settingNum - 25], settingNum - 25, Vout3address);
    }
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U15U16P0ASwitch);
        Serial.println(displayBuffer);
    }
    return U15U16P0ASwitch;
  }
  else if (settingNum >= 33 && settingNum <= 36 ){
    if (settingValue > -1) pwmValue[settingNum - 33] = uint16_t(settingValue);
    analogWrite(PWMPins[settingNum - 33],pwmValue[settingNum - 33]);
    if (debugDisplay) Serial.println(pwmValue[settingNum - 33]);
    return pwmValue[settingNum - 33];
  }
  else if (settingNum == 37){
    if (settingValue > -1) P10or19Switch = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(P10or19Switch);
        Serial.println(displayBuffer);
    }
    return P10or19Switch;
  }
  else if (settingNum == 38){
    if (settingValue > -1) P15or18Switch = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(P15or18Switch);
        Serial.println(displayBuffer);
    }
    return P15or18Switch;
  } 
  else if (settingNum == 39){
    if (settingValue > -1) CAN1Switch = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(CAN1Switch);
        Serial.println(displayBuffer);
    }
    return CAN1Switch;
  }  
  else if (settingNum == 40){
    if (settingValue > -1) CAN2Switch = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(CAN2Switch);
        Serial.println(displayBuffer);
    }
    return CAN2Switch;
  }
  else if (settingNum == 41){
    if (settingValue > -1) CAN0term = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(CAN0term);
        Serial.println(displayBuffer);
    }
    return CAN0term;
  } 
  else if (settingNum == 42) {
    if (settingValue > -1) CAN1term = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(CAN1term);
        Serial.println(displayBuffer);
    }
    return CAN1term;
  } 
  else if (settingNum == 43){
    if (settingValue > -1) CAN2term = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(CAN2term);
        Serial.println(displayBuffer);
    }
    return CAN2term;
  }  
  else if (settingNum == 44) {
    if (settingValue > -1) LINmaster = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(LINmaster);
        Serial.println(displayBuffer);
    }
    return LINmaster;
  }  
  else if (settingNum == 45)  {
    if (settingValue > -1) IH1State = boolean(settingValue);
    if (IH1State) PWM3Out = false;
    else PWM3Out = true;
    
    setTerminationSwitches();
    digitalWrite(IH1Pin,IH1State);
    if (debugDisplay) {
        connectionString(IH1State);
        Serial.println(displayBuffer);
    }
    return IH1State;
  }  
  else if (settingNum == 46) {
    if (settingValue > -1) IH2State = boolean(settingValue);
    if (IH2State) MCP41HVExtender_SetTerminals(11, 0); //Turn off all terminals on Pot 11
    else MCP41HVExtender_SetTerminals(11, SPIpotTCONSettings[10]); //Reset all terminals on Pot 11
    digitalWrite(IH2Pin,IH2State);
    if (debugDisplay) {
        connectionString(IH2State);
        Serial.println(displayBuffer);
    }
    return IH2State;
  }
  else if (settingNum == 47){
    if (settingValue > -1) IL1State = boolean(settingValue);
    if (IL1State) PWM4Out = false;
    else PWM4Out = true; 
    setTerminationSwitches(); //Turn off PWM4
    digitalWrite(IL1Pin,IL1State);
    if (debugDisplay) {
        connectionString(IL1State);
        Serial.println(displayBuffer);
    }
    return IL1State;
  }
  else if (settingNum == 48) {
    if (settingValue > -1) IL2State = boolean(settingValue);
    if (IL2State) MCP41HVExtender_SetTerminals(12, 0); //Turn off all terminals on Pot 12
    else MCP41HVExtender_SetTerminals(12, SPIpotTCONSettings[11]); //Reset all terminals 
    digitalWrite(IL2Pin,IL2State);
    if (debugDisplay) {
        connectionString(IL2State);
        Serial.println(displayBuffer);
    }
    return IL2State;
  }
  else if (settingNum == 49) {
    if (settingValue > -1) HVoutAdjValue = uint8_t(settingValue);
    if (debugDisplay) {
//        Serial.print(position);
        Serial.print(", ");
        Serial.println(HVoutAdjValue);
    }
    if (revision.equals("02")){
      MCP41HV_SetWiper(CShvAdjPin, HVoutAdjValue);
      return HVoutAdjValue;
    }
    uint8_t position = MCP41HVI2C_SetWiper(HVoutAdjAddr,HVoutAdjValue);
    return position;
  }
  else if (settingNum == 50){
    if (settingValue > -1) ignitionCtlState = boolean(settingValue);
    digitalWrite(ignitionCtlPin,ignitionCtlState);
    if (debugDisplay) {
        connectionString(ignitionCtlState);
        Serial.println(displayBuffer);
    }
    return ignitionCtlState;
  } 
  else if (settingNum >  50 && settingNum <= 66) {
    if (settingValue > -1) SPIpotTCONSettings[settingNum - 51] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVExtender_SetTerminals(settingNum - 51, SPIpotTCONSettings[settingNum - 51]);
    if (debugDisplay) {
        Serial.print(SPIpotTCONSettings[settingNum - 51]);
        Serial.print(", ");
        terminalString(terminalConnection);
        Serial.println(displayBuffer);
    }
    return terminalConnection;
  }
  else if (settingNum == 67){
    if (settingValue > -1) PWM1Out = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(PWM1Out);
        Serial.println(displayBuffer);
    }
    return PWM1Out;
  } 
  else if (settingNum == 68){
    if (settingValue > -1) PWM2Out = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(PWM2Out);
        Serial.println(displayBuffer);
    }
    return PWM2Out;
  }  
  else if (settingNum == 69){
    if (settingValue > -1) PWM3Out = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(PWM3Out);
        Serial.println(displayBuffer);
    }
    return PWM3Out;
  }  
  else if (settingNum == 70) {
    if (settingValue > -1) PWM4Out = boolean(settingValue);
    setTerminationSwitches();
    if (debugDisplay) {
        connectionString(PWM4Out);
        Serial.println(displayBuffer);
    }
    return PWM4Out;
  } 
  else if (settingNum == 71) {
    if (settingValue > -1) LIN1Switch = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(LIN1Switch);
        Serial.println(displayBuffer);
    }
    return LIN1Switch;
  }
  else if (settingNum == 72) {
    if (settingValue > -1) LIN2Switch = boolean(settingValue);
    setConfigSwitches();
    if (LIN2Switch) MCP41HVExtender_SetTerminals(15, 0);
    else MCP41HVExtender_SetTerminals(15, SPIpotTCONSettings[15]);
    if (debugDisplay) {
        connectionString(LIN2Switch);
        Serial.println(displayBuffer);
    }
    return LIN2Switch;
  }
  else if (settingNum == 73) {
    if (settingValue > -1) U1though8Enable = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U1though8Enable);
        Serial.println(displayBuffer);
    }
    return U1though8Enable;
  } 
  else if (settingNum == 74){
    if (settingValue > -1) U9though16Enable = boolean(settingValue);
    setConfigSwitches();
    if (debugDisplay) {
        connectionString(U9though16Enable);
        Serial.println(displayBuffer);
    }
    return U9though16Enable;
  } 
  else if (settingNum == 75){
    if (settingValue > -1) I2CpotWiperSettings[0] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetWiper(I2CpotAddr[0],I2CpotWiperSettings[0]);
    if (debugDisplay) {
        Serial.print(I2CpotWiperSettings[0]);
        Serial.print(", ");
        Serial.println(terminalConnection);
    }
    return terminalConnection;
  }
  else if (settingNum == 76){
    if (settingValue > -1) I2CpotWiperSettings[1] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetWiper(I2CpotAddr[1],I2CpotWiperSettings[1]);
    if (debugDisplay) {
        Serial.print(I2CpotWiperSettings[1]);
        Serial.print(", ");
        Serial.println(terminalConnection);
    }
    return terminalConnection;
  }
  else if (settingNum == 77){
    if (settingValue > -1) I2CpotWiperSettings[2] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetWiper(I2CpotAddr[2],I2CpotWiperSettings[2]);
    if (debugDisplay) {
        Serial.print(I2CpotWiperSettings[2]);
        Serial.print(", ");
        Serial.println(terminalConnection);
    }
    return terminalConnection;
  }
  else if (settingNum == 78){
    if (settingValue > -1) I2CpotTCONSettings[0] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetTerminals(I2CpotAddr[0],I2CpotTCONSettings[0]);
    if (debugDisplay) {
        Serial.print(I2CpotTCONSettings[0]);
        Serial.print(", ");
        terminalString(terminalConnection);
        Serial.println(displayBuffer);
    }
    return terminalConnection;
  }
  else if (settingNum == 79){
    if (settingValue > -1) I2CpotTCONSettings[1] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetTerminals(I2CpotAddr[1],I2CpotTCONSettings[1]);
    if (debugDisplay) {
        Serial.print(I2CpotTCONSettings[1]);
        Serial.print(", ");
        terminalString(terminalConnection);
        Serial.println(displayBuffer);
    }
    return terminalConnection;
  }
  else if (settingNum == 80){
    if (settingValue > -1) I2CpotTCONSettings[2] = uint8_t(settingValue);
    uint8_t terminalConnection = MCP41HVI2C_SetTerminals(I2CpotAddr[2],I2CpotTCONSettings[2]);
    if (debugDisplay) {
        Serial.print(I2CpotTCONSettings[2]);
        Serial.print(", ");
        terminalString(terminalConnection);
        Serial.println(displayBuffer);
    }
    return terminalConnection;
  }

  else if (settingNum >= 81 && settingNum <= 84 ){
    if (settingValue > -1)  pwmFrequency[settingNum-81] = uint16_t(settingValue);
    analogWriteFrequency(PWMPins[settingNum-81], pwmFrequency[settingNum-81]);
    for (uint8_t i = 0; i<numPWMs; i++) analogWrite(PWMPins[i],pwmValue[i]);
    if (debugDisplay) {
        Serial.println(pwmFrequency[settingNum-81]);
    }
    return pwmFrequency[settingNum-81];
  }

  else return -1;
}
/*               End Define Settings                              */
/******************************************************************/

void setCompIdEEPROMdata () {
  char id[256];
  componentID.toCharArray(id,255);
  EEPROM.put(componentIDAddress,id);
  Serial.print("SAVED ");
  Serial.println(componentID);
}


void getCompIdEEPROMdata () {
  char id[256];
  EEPROM.get(componentIDAddress, id);
  if (id[0]==255) {
    setCompIdEEPROMdata ();
  }
  else{
    componentID = String(id);
    Serial.print("LOADED ");
    Serial.println(componentID);
  }
}


void displayJ1708(){
  showJ1708 = bool(commandString.toInt());
  //clear the RX buffer on start. 
  J1708.clear();
  J1708_index = 0;
  newJ1708Char = false;
  firstJ1708 = true;
}

void  adjustError() {
  Serial.println(F("INFO SS - Condition not met. Turn adjust mode on by typing AO, then select a setting with CS"));
}



/********************************************************************************************/
/*                       Begin Function Calls for Serial Commands                           */

void turnOnAdjustMode() {
  ADJUST_MODE_ON = 1;
  Serial.println(F("INFO AO - Turned Adjustment mode on. Type AF or click to turn off. Type SS,XXXX  or scroll knob and click to set settings."));
  Serial.print(F("INFO Current Setting for Adjustement is "));
  Serial.print(currentSetting);
  Serial.print(" - ");
  Serial.println(settingNames[currentSetting]);
  knob.write(setSetting(currentSetting,-1,DEBUG_OFF));
  setLimits(currentSetting);

}

void turnOffAdjustMode() {
  ADJUST_MODE_ON = 0;
  Serial.println(F("INFO AF - Turned Setting Adjustment mode off. Type AO to turn on. Scroll knob to select a setting."));
  knob.write(currentSetting);
  knobLowLimit = 1;
  knobHighLimit = numSettings - 1;
}

void fastSetSetting(){
  int returnval;
  currentSetting = commandPrefix.toInt();
  if (currentSetting > 0 && currentSetting < numSettings){
    setLimits(currentSetting);
    if (commandString.length() > 0){ 
      long settingValue = constrain(commandString.toInt(), knobLowLimit, knobHighLimit);
      returnval = setSetting(currentSetting, settingValue,DEBUG_OFF);
    }
    else{
      returnval = setSetting(currentSetting, -1, DEBUG_OFF);
    }
    Serial.print("SET ");
    Serial.print(currentSetting);
    Serial.print(",");
    Serial.println(returnval);  
  }
  else Serial.println(F("ERROR in setting value."));
  
}

void changeSetting() {
  Serial.println(F("INFO CS - Change or Select Setting."));
  if (commandString.length() > 0) {
    currentSetting = constrain(commandString.toInt(), 0, numSettings);
    
  }
  //listSetting(currentSetting);
  if (ADJUST_MODE_ON){
    setLimits(currentSetting);
    knob.write(setSetting(currentSetting,-1,DEBUG_OFF));
  }
  else{
    if (knob.read() == currentSetting){
      Serial.print("INFO ");
      setSetting(currentSetting,-1,DEBUG_ON);
    }
    else knob.write(currentSetting); //automatic listSetting if knob changes
    knobLowLimit = 1;
    knobHighLimit = numSettings - 1;
  }
}

void listSettings(){
  Serial.println(F("INFO LS - List Settings. "));
  for (int i = 1; i < numSettings; i++) {
    Serial.print("INFO ");
    setSetting(i,-1,DEBUG_ON);
  }
}

void changeValue(){
  //Set value from Serial commands
  if (ADJUST_MODE_ON && currentSetting != 0) {
    Serial.println(F("INFO SS - Set Setting."));
    int adjustmentValue = constrain(commandString.toInt(), knobLowLimit, knobHighLimit);
    currentKnob = setSetting(currentSetting, adjustmentValue,DEBUG_ON);
    knob.write(currentKnob);
  }
  else
  {
    adjustError();
  }
}


/********************************************************************************************/
/*                         Begin Function Calls for Knob Buttons                            */

OneButton button(buttonPin, true);

void longPressStart() {
  ignitionCtlState = !ignitionCtlState;
  commandPrefix = "50";
  if (ignitionCtlState) commandString = "1";
  else commandString = "0";
  fastSetSetting();
}

void myClickFunction() {
  turnOffAdjustMode();
  }
void myDoubleClickFunction() {}
void longPress() {} //Do nothing at this time.
void longPressStop() {} 
/*                         End Function Calls for Knob Buttons                            */
/********************************************************************************************/

void accelerate(){
  //pinnumber, duration, and ending frequency
  const uint8_t MIN_PIN_FREQ = 14;
  uint8_t firstIndex = commandString.indexOf(",");
  uint8_t secondIndex = commandString.indexOf(",", firstIndex + 1);
  uint8_t thirdIndex = commandString.indexOf(",", secondIndex + 1);

  uint8_t setting = commandString.substring(0, firstIndex).toInt();
  int32_t finalFreq = commandString.substring(firstIndex + 1, secondIndex).toInt();
  float duration = commandString.substring(secondIndex + 1, thirdIndex).toFloat();
  bool debug = (bool) commandString.substring(thirdIndex + 1).toInt();
  uint16_t firstFreq = pwmFrequency[setting - 81];
  int16_t freqChange = finalFreq - firstFreq;
  

  uint16_t num_reps = duration * 10;
  float secretcounter = (float) firstFreq;
  float stepAmt = abs(freqChange)/(float)num_reps;
  float oldcounter;
  if(finalFreq > firstFreq){
    for(uint16_t i = 0; i < num_reps; i++){
      oldcounter = secretcounter;
      secretcounter += stepAmt;
      if((int)oldcounter < (int)secretcounter){
        analogWriteFrequency(PWMPins[setting-81],(int)secretcounter);
        pwmFrequency[setting - 81] = (int)secretcounter;
        if(debug){
          Serial.print(setting);
          Serial.print(",");
          Serial.println((int)secretcounter);
        }
      }
      delay(100);
    }
  }
  else if(finalFreq < firstFreq){
    Serial.println("DECELERATE");
    for(uint16_t i = 0; i < num_reps; i++){
      oldcounter = secretcounter;
      secretcounter -= stepAmt;
      if((int)oldcounter > (int)secretcounter && (int)secretcounter >= MIN_PIN_FREQ){ //added =
        analogWriteFrequency(PWMPins[setting-81], (int)secretcounter);
        pwmFrequency[setting - 81] = (int)secretcounter;
        if(debug){
          Serial.print(setting);
          Serial.print(",");
          Serial.println((int)secretcounter);
        }
      }
      if((int)secretcounter <= MIN_PIN_FREQ){
        if(debug){
          Serial.print("Reached Minimum Frequency ");
          Serial.println((int)secretcounter);
        }
        break;
      }
      delay(100);
    }
  }


//  for(uint16_t i = 0; i < abs(freqChange); i++){
//    if(freqChange > 0){
//      firstFreq += 1;
//    }
//    else if(freqChange < 0){
//      firstFreq -= 1;
//    }
//    Serial.println(setting);
//    Serial.println(firstFreq);
//    analogWriteFrequency(PWMPins[setting-81],firstFreq);
//    uint32_t milliDelay = duration/(abs(freqChange)*1000);
//    uint32_t microDelay = (duration/abs(freqChange))%1000;
//    delay(milliDelay);
//    delayMicroseconds(microDelay);
//    pwmFrequency[setting - 81] = firstFreq;
//    if(firstFreq <= 14) break;
//  }
  Serial.println("MADE IT");

  
}

void startStopCAN0Streaming(){
  if (commandString.toInt() > 0) displayCAN0 = true;
  else  displayCAN0 = false;
}

void startStopCAN1Streaming(){
  if (commandString.toInt() > 0) displayCAN1 = true;
  else  displayCAN1 = false;
}

void startStopCAN2Streaming(){
  if (commandString.toInt() > 0) displayCAN2 = true;
  else  displayCAN2 = false;
}





void sendMessage(){
  /* Sends a CAN message from the following string fields:
   *  channel,id,data
   *  channel is 0 for CAN0 and 1 for CAN1
   *  id is the CAN ID. if the ID is less than 11 bits and the first bit is set, then it will be transmitted as an extended id
   *  data length code is determined by the length of the data
   *  data is the hex representation in ascii with no spaces. there will be 16 hex characters for 8 bytes. Data longer than 8 bytes will be ignored. 
   */
  boolean goodID = false;
  boolean goodData = false;
  
  //Serial.println(F("CANSEND - Send Message."));
  //Serial.println(commandString);
  char commandCharBuffer[100];
  char IdCharBuffer[9];
  char dataCharBuffer[17];
  char *endptr;
  if (commandString.length() > 0) {
    commandString.toCharArray(commandCharBuffer,commandString.length());
    int channel;
    if (commandCharBuffer[0]=='1') channel = 1;
    else if (commandCharBuffer[0]=='0') channel = 0;
    else channel = -1;
    
    //Serial.print(channel);  
    
    int commaIndex0 = commandString.indexOf(',');
    int commaIndex1 = commandString.indexOf(',',commaIndex0+1);
    if (commaIndex0 > 0 && commaIndex1 > 1){
      String idString = commandString.substring(commaIndex0+1,commaIndex1);
      //Serial.print("idString = ");
      //Serial.println(idString);
      idString.toCharArray(IdCharBuffer,9);
      for (uint8_t i = 0; i < strlen(IdCharBuffer) ; i++){
        //Serial.print(IdCharBuffer[i]);
        if (isxdigit(IdCharBuffer[i])) goodID = true; 
        else { goodID = false; break; }
      }
      if (goodID){
        uint32_t tempID = strtoul(IdCharBuffer,&endptr,16);
        //Serial.print(" ");
        //Serial.print(tempID,HEX);
        if (  (tempID >> 11) > 0){
          txmsg.ext = 1;
          txmsg.id = (0x3FFFFFFF & tempID); //29 bit ID
        }
        else {
          txmsg.ext = 0;
          txmsg.id = (0x7FF & tempID); //11 bit ID
        }
        //Serial.print(" ");
        //Serial.print(txmsg.id,HEX);
      }
      else Serial.println("ERROR Invalid ID format");
      
      String dataString = commandString.substring(commaIndex1+1);
      
      dataString.toCharArray(dataCharBuffer,17);
      txmsg.len = strlen(dataCharBuffer)/2;
      //Serial.print(" ");
      //Serial.print(txmsg.len,HEX);
      // Serial.print(" ");
      for (uint8_t i = 0; i < txmsg.len*2 ; i++){
        if (isxdigit(dataCharBuffer[i])) goodData = true; 
        else { goodData = false; Serial.println("ERROR Non Hex Characters or Odd number of nibbles or ID is too long"); break; }
      } 
      if (goodData){
        for (int i = 0; i <  txmsg.len ; i++){
          char byteStringChars[3] = {dataCharBuffer[2*i],dataCharBuffer[2*i+1],0x00};
          txmsg.buf[i] = strtol(byteStringChars,&endptr,16);
          //Serial.print(txmsg.buf[i],HEX);
          //Serial.print(" ");
        }
      }
    }
    
    if (goodData && goodID ){
      if (channel == 0) {
        Can0.write(txmsg);
      }
      else if (channel == 1){
        Can1.write(txmsg);
      }
      else Serial.println("ERROR Invalid Channel for CANSEND.");
    }
    
    else
      Serial.println(F("ERROR Invalid input data for CANSEND. Input should be using hex characters with no spaces in the form SM,channel,ID,data/n"));
  }
  else
  {
    Serial.println(F("ERROR Missing or invalid data to send."));
  }
  txmsg.ext = 1; //set default
  txmsg.len = 8;
}


void setupComponentInfo(){
   char byteEntry[4];
   uint8_t old_shortest_period = shortest_period;
   shortest_period = 1;
   uint16_t id_length = constrain(componentID.length(),0,7*256-1);
   char id[7*256];
   componentID.toCharArray(id,id_length+1);
   uint8_t num_frames = id_length/7;
   if (id_length % 7 > 0) num_frames++;
   char bytes_to_send[4]; 
   sprintf(bytes_to_send,"%02X",id_length);
   char frames_to_send[3];
   sprintf(frames_to_send,"%02X",num_frames);
   commandString = "CI from SSS2,0,1,0,0,1,0,1,1,18ECFF";
   sprintf(byteEntry,"%02X,",source_address);
   commandString += byteEntry; 
   commandString += "8,20,";
   commandString += bytes_to_send;
   commandString += ",0,";
   commandString += frames_to_send;
   commandString += ",FF,EB,FE,00";
   Serial.println(commandString);
   
   setupPeriodicCANMessage();
   
   
   for (int i = 0; i < num_frames; i++){
     commandString = "CI from SSS2,0,";
     sprintf(byteEntry,"%d,",num_frames+1);
     commandString += byteEntry; 
     sprintf(byteEntry,"%d,",i+1);
     commandString += byteEntry; 
     commandString += "0,1,0,";
     sprintf(byteEntry,"%d,",num_frames+1);
     commandString += byteEntry; 
     commandString += "1,18EBFF";
     sprintf(byteEntry,"%02X,",source_address);
     commandString += byteEntry; 
     commandString += "8,";
     sprintf(byteEntry,"%02X,",i+1);
     commandString += byteEntry; 
     for (int j = 7*i; j < 7*i+7;j++){
       if (j < id_length) sprintf(byteEntry,"%02X,",id[j]);
       else sprintf(byteEntry,"%02X,",0xFF);
       commandString += byteEntry;
     }
     Serial.println(commandString);
   
     comp_id_index = setupPeriodicCANMessage();
   }
   shortest_period = old_shortest_period;
  
    
}       

void reloadCAN(){
  setupComponentInfo();
        
  for (int i = 0; i < num_default_messages; i++){
    commandString = default_messages[i];
    setupPeriodicCANMessage();
    delay(1);
  }
  commandString = "1";
  goCAN();
}
void parseJ1939(CAN_message_t &rxmsg ){
  uint32_t ID = rxmsg.id;
  uint8_t DLC = rxmsg.len;
  uint8_t SA = (ID & 0xFF);
  uint8_t PF = (ID & 0x03FF0000) >> 16;
  uint8_t PRIORITY = (ID & 0x3C000000) >> 26;
  uint8_t DA = 0xFF;
  uint32_t PGN;
  if (PF >= 240){
     PGN = (ID & 0x03FFFF00) >> 8;
  }
  else{
    //pdu1 format
    PGN = (ID & 0x03FF0000) >> 8;
    DA = (ID & 0x00000FF00);
  }
  
  if (PGN == 0xEA00 && DA == 0xFF){
    //request message
    if (rxmsg.buf[0] == 0xEB && rxmsg.buf[1] == 0xFE){
      if (enableSendComponentInfo){
        if (canComponentIDtimer >5000){
          canComponentIDtimer = 0;
          can_messages[comp_id_index]->enabled = true;
          can_messages[comp_id_index]->transmit_number = 0;
          can_messages[comp_id_index]->ok_to_send = true;
          can_messages[comp_id_index]->loop_cycles = 0; 
          can_messages[comp_id_index]->cycle_count = 0;
          can_messages[comp_id_index]->message_index = 0;
        }
  
      }
    }
  }
}



//A generic CAN Frame print function for the Serial terminal
void printFrame(CAN_message_t rxmsg, int mailbox, uint8_t channel, uint32_t RXCount)
{ 

 Serial.printf("CAN%d %10lu.%06lu %08X %d %d %02X %02X %02X %02X %02X %02X %02X %02X\n",
          channel,now(),uint32_t(microsecondsPerSecond),rxmsg.id,rxmsg.ext,rxmsg.len,
          rxmsg.buf[0],rxmsg.buf[1],rxmsg.buf[2],rxmsg.buf[3],
          rxmsg.buf[4],rxmsg.buf[5],rxmsg.buf[6],rxmsg.buf[7]);
}


time_t getTeensy3Time(){
  microsecondsPerSecond = 0;
  return Teensy3Clock.get();
}

const uint32_t DEFAULT_TIME = 10; 
char timeStamp[45];

uint32_t processSyncMessage() {
  char timeChar[15];
  memset(timeChar,0,15);
  commandString.toCharArray(timeChar,15);
  time_t pctime = atol(timeChar);
  if (pctime > DEFAULT_TIME){
    return pctime;
  }
  else if (pctime == 0) {
    Serial.printf("INFO Time is %04d-%02d-%02d %02d:%02d:%02d\n",year(),month(),day(),hour(),minute(),second());
    return 0L;
  }
  else{
    Serial.println("ERROR Time value is invalid.");
    return 0L;
  }
}

void displayTime() {
  time_t t = processSyncMessage();
  if (t != 0) {
    Teensy3Clock.set(t); // set the RTC
    setTime(t);
    Serial.printf("SET Time to %04d-%02d-%02d %02d:%02d:%02d\n",year(),month(),day(),hour(),minute(),second());
  }
}

void print_uid()  {  
  Serial.printf("ID: %s\n", kinetisUID());
}
 
/*void printStats(CAN_stats_t currentStats,int channel){
  Serial.printf("STATS for CAN%d: Enabled:%d, RingRXHighWater: %ul, ringRXFramesLost: %lu, ringTXHighWater: %lu, mailbox use count:[%lu",
                            channel,currentStats.enabled,currentStats.ringRxHighWater,currentStats.ringRxFramesLost,currentStats.ringTxHighWater,currentStats.mb[0].refCount);
  for (int i=1;i<16;i++){
    Serial.printf(", %lu",currentStats.mb[i].refCount); 
  }
  Serial.printf("], overrunCount: [%lu",currentStats.mb[0].overrunCount);
  for (int i=1; i<16; i++){
    Serial.printf(", %lu",currentStats.mb[i].overrunCount); 
  }
  Serial.println("]");
}

void displayStats(){
  CAN_stats_t currentStats = Can0.getStats();
  printStats(currentStats,0);
  currentStats = Can1.getStats();
  printStats(currentStats,1);
}


void clearStats(){
  Can0.clearStats();
  Can1.clearStats();
  
  Can0.startStats();
  Can1.startStats();
  
  Serial.println("INFO Cleared CAN Statisitics");
}*/

void listInfo() {
  Serial.print("INFO SSS2 Component ID (Make*Model*Serial*Unit): ");
  Serial.println(componentID);
  
}

void changeComponentID() {
  if (commandString.length() < 12) {
    Serial.print(F("INFO SSS2 Component ID: "));
    Serial.println(componentID);
  }
  else{
    componentID = commandString;
    setCompIdEEPROMdata();
    Serial.print(F("SET SSS2 Component ID: "));
    Serial.println(componentID);
    setupComponentInfo();
  } 
}


void autoBaud0(){
  Serial.println(F("B0 - Set the baudrate for CAN 0 or select AutoBaud"));
  char baudstring[9];
  if (commandString.length() > 0){
    commandString.toCharArray(baudstring,9);
    BAUDRATE0 = strtoul(baudstring,0,10);
    Serial.print("SET CAN0 baudrate set to ");
    Serial.println(BAUDRATE0);
    for (uint8_t baudRateIndex = 0; baudRateIndex<sizeof(baudRateList); baudRateIndex++){
      if (BAUDRATE0 == baudRateList[baudRateIndex]){
        Can0.begin(BAUDRATE0);
        for (uint8_t filterNum = 4; filterNum < 16;filterNum++) Can0.setFilter(allPassFilter,filterNum);
        CAN0baudNotDetected = false;
        break; 
      }
      else{
        CAN0baudNotDetected = true;
      }
      
    }
  }
  else {
    BAUDRATE0 = 0;
  } 
  if (BAUDRATE0 == 0){
    CAN0baudNotDetected = true;
    Serial.println("INFO CAN0 set to automatically set baudrate.");
  }
}

void autoBaud1(){
  Serial.println(F("INFO B1 - Set the baudrate for CAN 1 or select AutoBaud"));
  char baudstring[9];
  if (commandString.length() > 0){
    commandString.toCharArray(baudstring,9);
    BAUDRATE1 = strtoul(baudstring,0,10);
    Serial.print("INFO CAN1 baudrate set to ");
    Serial.println(BAUDRATE1);
    for (uint8_t baudRateIndex = 0; baudRateIndex<sizeof(baudRateList); baudRateIndex++){
      if (BAUDRATE1 == baudRateList[baudRateIndex]){
          Can1.begin(BAUDRATE1);
          for (uint8_t filterNum = 4; filterNum < 16;filterNum++) Can1.setFilter(allPassFilter,filterNum);
          CAN1baudNotDetected = false;
          break;
        }
        else{
         CAN1baudNotDetected = true;
        }
      }
  }
  else {
    BAUDRATE1 = 0;
  } 
  if (BAUDRATE1 == 0){
    CAN1baudNotDetected = true;
    Serial.println(F("INFO CAN1 set to automatically set baudrate."));
  }
}

void displayBaud(){
  Serial.print("SET CAN0 Baudrate");
  Serial.println(BAUDRATE0);
  Serial.print("SET CAN1 Baudrate");
  Serial.println(BAUDRATE1);  
}
void setEnableComponentInfo(){
  if (commandString.toInt() > 0){
    setupComponentInfo();
    enableSendComponentInfo = true;
    Serial.println(F("SET Enable CAN transmission of Component ID"));
    if (canComponentIDtimer >5000){
      canComponentIDtimer = 0;
      can_messages[comp_id_index]->enabled = true;
      can_messages[comp_id_index]->transmit_number = 0;
      can_messages[comp_id_index]->ok_to_send = true;
      can_messages[comp_id_index]->loop_cycles = 0; 
      can_messages[comp_id_index]->cycle_count = 0;
      can_messages[comp_id_index]->message_index = 0;
    }
  
  }
  else{
    enableSendComponentInfo = false;
    Serial.println(F("SET Disable CAN transmission of Component ID"));  
    can_messages[comp_id_index]->enabled = false;
  }
  
}

void checkAgainstUID(){
  String secret = kinetisUID();
  if(commandString==secret) Serial.println("OK:Authenticated");
  else Serial.println("OK:Denied");
}
