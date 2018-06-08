#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface() : 
mLog(nullptr),
mUartStream(10, 11)
{
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  mUartStream.begin(4800);
}

void Sim7kInterface::setLogStream(HardwareSerial* log)
{
  mLog = log;
}

bool Sim7kInterface::turnOn()
{
  if (isOn())
  {
    return true;
  }
  
  digitalWrite(6, LOW);
  delay(200);
  digitalWrite(6, HIGH);
  delay(4000);

  flushUart();

  return isOn();
}

bool Sim7kInterface::turnOff()
{
  if (!isOn())
  {
    return true;
  }
  
  mUartStream.write("AT+CPOWD=0\r\n");
  return checkResponse("NORMAL POWER DOWN");
}

bool Sim7kInterface::isOn()
{
  mUartStream.write("AT\r\n");
  return checkResponse("OK");
}

bool Sim7kInterface::turnOnGnss()
{
  mUartStream.write("AT+CGNSPWR=1\r\n");

  return checkResponse("OK");
}

void Sim7kInterface::flushUart()
{
  while (mUartStream.available())
  {
    readLineFromUart();
  }
}

//responses from modem are in the form <CR><LF><msg><CR><LF>
//goal is to return just <msg>
bool Sim7kInterface::readLineFromUart()
{
  bool foundLineFeed{false};

  for (int i{0}; i < RX_BUFFER_SIZE; i++)
  {
   char nextByte = mUartStream.read();

    //need this loop because arduino will read faster than uart stream transmits
    while (nextByte == -1)
    {
      nextByte = mUartStream.read();
    }

    switch (nextByte)
    {
      case '\n':
      if (foundLineFeed)
      {
          mRxBuffer[i] = '\0';
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

      default:
      mRxBuffer[i] = nextByte;
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

  //set buffer to empty string
  mRxBuffer[0] = '\0';

  return false;
}

bool Sim7kInterface::checkResponse(const char* expectedResponse)
{
  if (readLineFromUart())
  {
    return strcmp(mRxBuffer, expectedResponse) == 0;
  }

  return false;
}

void Sim7kInterface::writeToLog(const char* msg)
{
  if (mLog)
  {
    mLog->write("Log - ");
    mLog->println(msg);
  }
}

