#include <Arduino.h>

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPIFlash.h>
#include "Ticker.h"

#define NETWORKID 100

#define FREQUENCY RF69_915MHZ
#define ENCRYPTKEY "sampleEncryptKey"
#define IS_RFM69HW_HCW

#define SERIAL_BAUD 115200
#define SLAVE_ID 2

String buff((char *)0);
boolean newData = false;
RFM69 radio;
// Mode 0 undefined, 1 Master, 2 Slave
int mode = 0;

void recvUntilNewLine();
void parseData();
void startShow();
void sendSyncCMD();

void setup()
{
  buff.reserve(20);
  Serial.begin(SERIAL_BAUD);

  // Wait to find out if we're the master radio or slave radio.
  while (mode == 0)
  {
    recvUntilNewLine();
    parseData();
    if (mode > 0)
    {
      Serial.print("Mode set to: ");
      Serial.println(mode);
    }
  }

  // Set our radio ID based on which mode we're in. 1<- is the master node and 2 are the reciever nodes.
  radio.initialize(FREQUENCY, mode, NETWORKID);
  radio.setHighPower();
}

void loop()
{

  // If we are the master mode we will be parsing input from the PI to know when to sync up the show.
  if (mode == 1)
  {
    recvUntilNewLine();
    parseData();
  }
  else if (mode == 2)
  {
    recvUntilNewLine();
    //Wait to recieve messages from the master radio then we will output to the PI requesting it start the show
    if (radio.receiveDone())
    {
      for (uint8_t i = 0; i < radio.DATALEN; i++)
      {
        // Serial.print((char)radio.DATA[i]);
        buff += (char)radio.DATA[i];
      }
      newData = true;
    }
    parseData();
  }
}

void recvUntilNewLine()
{
  char newLine('\n');
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();
    if (rc != newLine)
    {
      buff += rc;
    }
    else
    {
      newData = true;
    }
  }
}

void parseData()
{
  if (newData == true)
  {
    buff.trim();
    if (buff.equals("MST"))
    {
      Serial.println("Master Mode ");
      mode = 1;
    }
    else if (buff.equals("SLV"))
    {
      Serial.println("Slave Mode ");
      mode = 2;
    }
    else if (buff.equals("SYNC") && mode == 1)
    {
      sendSyncCMD();
    }
    else if (buff.equals("VERSION"))
    {
      Serial.println(F("V: " __DATE__ " " __TIME__));
    }
    else if (buff.equals("SYNC") && mode == 2)
    {
      Serial.println("SYNC");
    }
    else
    {
      Serial.print("Command not understood: ");
      Serial.println(buff);
    }
    buff = (char *)0;
    newData = false;
  }
}

void sendSyncCMD()
{
  radio.send(SLAVE_ID, "SYNC", 4);
}