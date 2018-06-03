#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface() : 
mLog(nullptr),
mUartStream(10, 11), 
mIsOn(false)
{
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  mUartStream.begin(4800);
}

void Sim7kInterface::setLogStream(HardwareSerial* log)
{
  mLog = log;
}

void Sim7kInterface::tick()
{  
  if (mUartStream.available())
  { 
    const uint8_t bufferSize = 50;
    char msgBuffer[bufferSize];
    bool foundFirstLf = false;

    //messages come in the form <CR><LF><Msg><CR><LF>
    for (int i = 0; (i < bufferSize); i++)
    {      
      int nextByte = mUartStream.read();

      //this is because I think on occasion the arduino reads faster than the sim7k sends
      while (nextByte == -1)
      {
        nextByte = mUartStream.read();
      }
      
      msgBuffer[i] = nextByte;

      if (msgBuffer[i] == '\n')
      { 
        if (foundFirstLf)
        {
          msgBuffer[i] = '\0';
          handleUnsolicitedMsg(msgBuffer);
          break;
        }
        else
        {
          foundFirstLf = true;
          i--;
        }
      }
      else if (msgBuffer[i] == '\r')
      {
        i--;
      }
    }
  }
}

void Sim7kInterface::turnOn()
{
  digitalWrite(6, LOW);
  delay(200);
  digitalWrite(6, HIGH);
  delay(4000);
}

void Sim7kInterface::turnOff()
{
  digitalWrite(6, LOW);
  delay(1400);
  digitalWrite(6, HIGH);
  delay(1400);
}

bool Sim7kInterface::isOn()
{
  return mIsOn;
}

bool Sim7kInterface::sendCommand(const char* command, char* response, const size_t bufferSize)
{
  if (!mIsOn)
  {
    return false;
  }
  
  mUartStream.print(command);

  bool foundLineFeed{false};
  
  for (int i{0}; i < bufferSize; i++)
  {
    int nextByte = mUartStream.read();
    while (nextByte == -1)
    {
      nextByte = mUartStream.read();
    }

    response[i] = nextByte;

    if (response[i] == '\n')
    {
      if (foundLineFeed)
      {
        response[i] = '\0';
        return true;
      }
      else
      {
        foundLineFeed = true;
        i--;
      }
    }
    else if (response[i] == '\r')
    {
      i--;
    }
  }

  return false;
}

bool Sim7kInterface::handleUnsolicitedMsg(const char* msg)
{
  bool handled{false};

  writeToLog(msg);
  
  if (strcmp(msg, "SMS Ready") == 0)
  {
    mIsOn = true;
    handled = true;
  }
  else if (strcmp(msg, "NORMAL POWER DOWN") == 0)
  {
    mIsOn = false;
    handled = true;
  }

  if (handled)
  {
    writeToLog("Unsolicited msg was handled.");
  }
  else
  {
    writeToLog("Unsolicited msg was not handled.");
  }

  return handled;
}

void Sim7kInterface::writeToLog(const char* msg)
{
  if (mLog)
  {
    mLog->write("Log - ");
    mLog->println(msg);
  }
}

