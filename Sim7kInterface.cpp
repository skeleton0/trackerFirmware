#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface() : 
mLog(nullptr),
mUartStream(10, 11), 
mModemIsOn(false)
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
    return;
  }

  const size_t bufferSize{50};
  char responseBuffer[bufferSize];

  if (readLineFromUart(responseBuffer, bufferSize))
  {
    handleUnsolicitedResponse(responseBuffer); 
  }
}

void Sim7kInterface::turnModemOn()
{
  digitalWrite(6, LOW);
  delay(200);
  digitalWrite(6, HIGH);
  delay(4000);
}

void Sim7kInterface::turnModemOff()
{
  digitalWrite(6, LOW);
  delay(1400);
  digitalWrite(6, HIGH);
  delay(1400);
}

bool Sim7kInterface::modemIsOn()
{
  return mModemIsOn;
}

bool Sim7kInterface::turnOnGnss()
{
  if (!mModemIsOn)
  {
    return false;
  }

  mUartStream.write("AT+CGNSPWR=1\r\n");

  const size_t responseBuffer{50};
  char response[responseBuffer];
  if (readLineFromUart(response, responseBuffer))
  {
    if (strcmp(response, "OK"))
    {
      return true; 
    }
  }

  return false;
}

//responses from modem are in the form <CR><LF><msg><CR><LF>
//goal is to return just <msg>
bool Sim7kInterface::readLineFromUart(char* response, const size_t bufferSize)
{
  bool foundLineFeed{false};

  for (int i{0}; i < bufferSize; i++)
  {
   char nextByte = mUartStream.read();

    //need this loop because arduino will read faster than uart stream transmits
    while (nextByte == -1)
    {
      nextByte = mUartStream.read();
    }

    response[i] = nextByte;

    switch (response[i])
    {
      case '\n':
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

      break;
      
      case '\r':
      case '\0':
      i--;
      break;
    }
  }

  //read to buffer failed, so finish flushing the line from the uart stream
  char nextByte{0};
  while (!foundLineFeed && nextByte != '\n')
  {
    if (nextByte == '\n')
    {
      foundLineFeed = true;
    }
    
    nextByte = mUartStream.read();
  }

  return false;
}

bool Sim7kInterface::sendCommand(const char* command, char* response, const size_t bufferSize)
{
  if (!mModemIsOn)
  {
    return false;
  }
  
  mUartStream.write(command);
  
  return readLineFromUart(response, bufferSize);
}

void Sim7kInterface::handleUnsolicitedResponse(const char* response)
{
  bool handled{false};

  writeToLog(response);
  
  if (strcmp(response, "SMS Ready") == 0)
  {
    mModemIsOn = true;
    handled = true;
  }
  else if (strcmp(response, "NORMAL POWER DOWN") == 0)
  {
    mModemIsOn = false;
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
}

void Sim7kInterface::writeToLog(const char* msg)
{
  if (mLog)
  {
    mLog->write("Log - ");
    mLog->println(msg);
  }
}

