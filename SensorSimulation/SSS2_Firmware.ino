/*
   Smart Sensor Simulator 2

   Arduino Sketch to enable the functions for the SSS2

   Written By Dr. Jeremy S. Daily
   The University of Tulsa
   Department of Mechanical Engineering

   06 May 2017

   Released under the MIT License

   Copyright (c) 2017        Jeremy S. Daily

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.

*/

#include "SSS2_board_defs_rev_2.h"
#include "SSS2_functions.h"

//softwareVersion
String softwareVersion = "SSS2*REV" + revision + "*0.91b*master*7c1ec40e272233b5c69eb16a293705007af312bd"; //Hash of the previous git commit

void listSoftware() {
  Serial.print("FIRMWARE ");
  Serial.println(softwareVersion);
}

void setup() {
  Serial.begin(9600);
  //  LIN.begin(19200);
  SPI1.begin();

  commandString.reserve(256);
  commandPrefix.reserve(20);

  kinetisUID(uid);

  analogWriteResolution(12);

  setSyncProvider(getTeensy3Time);
  setSyncInterval(1);

  setPinModes();

  Wire.begin();
  Wire.setDefaultTimeout(200000); // 200ms

  //Initialize the MCP Extender
  digitalWrite(CSpotsPin, LOW);
  SPI1.transfer(ExtenderOpCode);
  SPI1.transfer(IOCONA);
  SPI1.transfer(0b10111100);
  digitalWrite(CSpotsPin, HIGH);
  digitalWrite(CSpotsPin, LOW);
  SPI1.transfer(ExtenderOpCode);
  SPI1.transfer(IODIRA);
  SPI1.transfer(0x00);
  digitalWrite(CSpotsPin, HIGH);
  digitalWrite(CSpotsPin, LOW);
  SPI1.transfer(ExtenderOpCode);
  SPI1.transfer(IODIRB);
  SPI1.transfer(0x00);
  digitalWrite(CSpotsPin, HIGH);
  digitalWrite(CSpotsPin, LOW);
  SPI1.transfer(ExtenderOpCode);
  SPI1.transfer(OLATA);
  SPI1.transfer(0xFF);
  digitalWrite(CSpotsPin, HIGH);
  digitalWrite(CSpotsPin, LOW);
  SPI1.transfer(ExtenderOpCode);
  SPI1.transfer(OLATB);
  SPI1.transfer(0xFF);
  digitalWrite(CSpotsPin, HIGH);
  //  PotExpander.begin(potExpanderAddr);  //U33
  //  ConfigExpander.begin(configExpanderAddr); //U21
  //  for (uint8_t i = 0; i<16; i++){
  //    PotExpander.pinMode(i,OUTPUT);
  //    ConfigExpander.pinMode(i,OUTPUT);
  //  }
  //  PotExpander.writeGPIOAB(0xFFFF);
  //  ConfigExpander.writeGPIOAB(0xFFFF);

  SPI.begin();
  terminationSettings = setTerminationSwitches();

  Serial.print("Configration Switches (U21): ");
  uint16_t configSwitchSettings = setConfigSwitches();
  Serial.println(configSwitchSettings, BIN);

  button.attachClick(myClickFunction);
  button.attachDoubleClick(myDoubleClickFunction);
  button.attachLongPressStart(longPressStart);
  button.attachLongPressStop(longPressStop);
  button.attachDuringLongPress(longPress);
  button.setPressTicks(2000);
  button.setClickTicks(250);

  initializeDACs(Vout2address);

  listInfo();
  for (int i = 1; i < numSettings; i++) {
    currentSetting = setSetting(i, -1, DEBUG_OFF);
    setSetting(i, currentSetting , DEBUG_ON);
  }

  Can0.begin(BAUDRATE0);
  Can1.begin(BAUDRATE1);

//  Can0.startStats();
  //Can1.startStats();

  //leave the first 4 mailboxes to use the default filter. Just change the higher ones
  allPassFilter.id = 0;
  allPassFilter.ext = 1;
  allPassFilter.rtr = 0;
  for (uint8_t filterNum = 4; filterNum < 16; filterNum++) {
    Can0.setFilter(allPassFilter, filterNum);
    Can1.setFilter(allPassFilter, filterNum);
  }

  txmsg.ext = 1;
  txmsg.len = 8;

  setConfigSwitches();

  //  LIN.begin(19200,SERIAL_8N2);

  J1708.begin(9600);
  J1708.clear();

  CANTimer.begin(runCANthreads, 1000); // Run can threads on an interrupt. Produces very little jitter.

  currentSetting = 1;
  knobLowLimit = 1;
  knobHighLimit = numSettings - 1;

  LINfinished = true;
  getCompIdEEPROMdata();

  commandString = "1";
  setEnableComponentInfo();
  reloadCAN();
}

void loop() {
  //Serial.println(fakePwmFrequencies[0]);
  //Serial.println(fakePwmTimeouts[0]);
  //  fakePwm(17,fakePwmFrequencies[0]);
  // fakePwm(18,fakePwmFrequencies[1]);
  // fakePwm(19,fakePwmFrequencies[2]);
  //  Check CAN messages
  if (Can0.available()) {
    Can0.read(rxmsg);
    //parseJ1939(rxmsg);
    RXCount0++;
    RXCAN0timer = 0;
    if (displayCAN0) printFrame(rxmsg, -1, 0, RXCount0);
    redLEDstate = !redLEDstate;
    digitalWrite(redLEDpin, redLEDstate);
  }
  if (Can1.available()) {
    Can1.read(rxmsg);
    RXCount1++;
    RXCAN1orJ1708timer = 0;
    if (displayCAN1) printFrame(rxmsg, -1, 1, RXCount1);
    if (ignitionCtlState) {
      greenLEDstate = !greenLEDstate;
      digitalWrite(greenLEDpin, greenLEDstate);
    }
  }

  //Check J1708
  if (J1708.available()) {
    J1708RXbuffer[J1708_index] = J1708.read();
    J1708RXtimer = 0;
    newJ1708Char = true;

    J1708_index++;
    if (J1708_index > sizeof(J1708RXbuffer)) J1708_index = 0;
  }
  if (showJ1708 && newJ1708Char && J1708RXtimer > 1150) { //At least 11 bit times must pass
    //Check to see if this is the first displayed message. If so, discard and start showing subsequent messages.
    if (firstJ1708) firstJ1708 = false;
    else {
      uint8_t j1708_checksum = 0;
      Serial.printf("J1708 %10lu.%06lu ", now(), uint32_t(microsecondsPerSecond));
      for (int i = 0; i < J1708_index; i++) {
        j1708_checksum += J1708RXbuffer[i];
        Serial.printf("%02X ", J1708RXbuffer[i]);
      }
      if (j1708_checksum == 0) Serial.println("OK");
      else Serial.println("Checksum Failed.");
    }
    J1708_index = 0;
    newJ1708Char = false;
  }

  if (send_voltage) {
    if (analog_tx_timer >= analog_display_period ) {
      analog_tx_timer = 0;
      Serial.print("ANALOG");
      for (uint8_t j = 0; j < numADCs; j++) {
        Serial.printf(" %lu:%d", uint32_t(analogMillis), analogRead(analogInPins[j]));
      }
      Serial.print("\n");
    }
  }

  if (enableSendComponentInfo) {
    if (canComponentIDtimer > 5000) {
      canComponentIDtimer = 0;
      can_messages[comp_id_index]->enabled = true;
      can_messages[comp_id_index]->transmit_number = 0;
      can_messages[comp_id_index]->ok_to_send = true;
      can_messages[comp_id_index]->loop_cycles = 0;
      can_messages[comp_id_index]->cycle_count = 0;
      can_messages[comp_id_index]->message_index = 0;
    }
  }

  //  sendLINResponse();

  /****************************************************************/
  /*            Begin Serial Command Processing                   */
  if (Serial.available() >= 2 && Serial.available() < 256) {
    commandPrefix = Serial.readStringUntil(',');
    commandString = Serial.readStringUntil('\n');

    if      (commandPrefix.toInt() > 0)                   fastSetSetting();
    else if (commandPrefix.equalsIgnoreCase("ACC"))       accelerate();
    else if (commandPrefix.equalsIgnoreCase("AI"))        displayVoltage();
    else if (commandPrefix.equalsIgnoreCase("B0"))        autoBaud0();
    else if (commandPrefix.equalsIgnoreCase("B1"))        autoBaud1();
    else if (commandPrefix.equalsIgnoreCase("DB"))        displayBaud();
    else if (commandPrefix.equalsIgnoreCase("CANCOMP"))   setEnableComponentInfo();
    else if (commandPrefix.equalsIgnoreCase("ID"))        print_uid();
    else if (commandPrefix.equalsIgnoreCase("C0"))        startStopCAN0Streaming();
    else if (commandPrefix.equalsIgnoreCase("C1"))        startStopCAN1Streaming();
    else if (commandPrefix.equalsIgnoreCase("C2"))        startStopCAN2Streaming();
    else if (commandPrefix.equalsIgnoreCase("GO"))        startCAN();
    else if (commandPrefix.equalsIgnoreCase("SP"))        set_shortest_period();
    else if (commandPrefix.equalsIgnoreCase("STOPCAN"))   stopCAN();
    else if (commandPrefix.equalsIgnoreCase("STARTCAN"))  goCAN();
    else if (commandPrefix.equalsIgnoreCase("CLEARCAN"))  clearCAN();
//    else if (commandPrefix.equalsIgnoreCase("STATS"))     displayStats();
 //   else if (commandPrefix.equalsIgnoreCase("CLEARSTATS"))clearStats();
    else if (commandPrefix.equalsIgnoreCase("CI"))        changeComponentID();
    else if (commandPrefix.equalsIgnoreCase("LS"))        listSettings();
    else if (commandPrefix.equalsIgnoreCase("OK"))        checkAgainstUID();
    else if (commandPrefix.equalsIgnoreCase("CANNAME"))   getThreadName();
    else if (commandPrefix.equalsIgnoreCase("CANSIZE"))   getThreadSize();
    else if (commandPrefix.equalsIgnoreCase("THREADS"))   getAllThreadNames();
    else if (commandPrefix.equalsIgnoreCase("SOFT"))      listSoftware();
    else if (commandPrefix.equalsIgnoreCase("J1708"))     displayJ1708();
    else if (commandPrefix.equalsIgnoreCase("SM"))        setupPeriodicCANMessage();
    else if (commandPrefix.equalsIgnoreCase("CANSEND"))   sendMessage();
    else if (commandPrefix.equalsIgnoreCase("RELOAD"))    reloadCAN();
    else if (commandPrefix.equalsIgnoreCase("TIME"))      displayTime();
    else if (commandPrefix.equalsIgnoreCase("GETTIME"))   Serial.printf("INFO Timestamp: %D\n", now());
    //    else if (commandPrefix.equalsIgnoreCase("LIN"))      ;// displayLIN();
    //    else if (commandPrefix.equalsIgnoreCase("SENDLIN"))   ;//sendLINselect();
    else {
      Serial.println(F("ERROR Unrecognized Command Characters. Use a comma after the command."));
      Serial.clear();
      //Serial.println(F("INFO Known commands are setting numbers, GO, SP, J1708, STOPCAN, STARTCAN, B0, B1, C0, C1, C2, DS, SW, OK, ID, STATS, CLEAR, MK, LI, LS, CI, CS, SA, SS, or SM."));
    }
  }
  /*              End Serial Command Processing                   */
  /****************************************************************/

  /****************************************************************/
  /*            Begin Quadrature Knob Processing                  */
  button.tick(); //check for presses
  int32_t newKnob = knob.read(); //check for turns
  if (newKnob != currentKnob) {
    if (newKnob >= knobHighLimit) {  //note: knob limits are for each input parameter
      knob.write(knobHighLimit);
      currentKnob = knobHighLimit;
    }
    else if (newKnob <= knobLowLimit) {
      knob.write(knobLowLimit);
      currentKnob = knobLowLimit;
    }
    else
    {
      currentKnob = newKnob;
    }
    //Place function calls to execute when the knob turns.
    if (ADJUST_MODE_ON) {
      setSetting(currentSetting, currentKnob, DEBUG_ON);
    }
    else {
      currentSetting = currentKnob;
      setSetting(currentSetting, -1, DEBUG_ON);
    }
  }
  /*            End Quadrature Knob Processing                    */
  /****************************************************************/


  /****************************************************************/
  /*           Begin LED Indicators for messages                  */
  /*
    /*Reset the greenLED after a timeout in case the last CAN message was on the wrong state*/
  if (RXCAN0timer >= 200) {
    RXCAN0timer = 0;
    redLEDstate = true;
    digitalWrite(redLEDpin, redLEDstate); //Use red because it is the power button.
  }

  /*Reset the greenLED after a timeout in case the last CAN message was on the wrong state*/
  if (RXCAN1orJ1708timer >= 200) {
    RXCAN1orJ1708timer = 0;
    if (ignitionCtlState) greenLEDstate = true;
    else greenLEDstate = false;
    digitalWrite(greenLEDpin, greenLEDstate);
  }
  /*             End LED Indicators for messages                        */
  /**********************************************************************/
}
