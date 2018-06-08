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

  if (mUartStream.available())
  {
    //set initial settings
    sendCommand("AT+IPR=4800");
    sendCommand("ATE0");
    flushUart();
  }
  
  return isOn();
}

bool Sim7kInterface::turnOff()
{
  if (!isOn())
  {
    return true;
  }
  
  sendCommand("AT+CPOWD=0");
  return checkResponse("NORMAL POWER DOWN");
}

bool Sim7kInterface::isOn()
{
  sendCommand("AT");
  return checkResponse("OK");
}

bool Sim7kInterface::turnOnGnss()
{
  sendCommand("AT+CGNSPWR=1");

  return checkResponse("OK");
}

void Sim7kInterface::sendCommand(const char* command)
{
  writeToLog("Sending to modem:");
  writeToLog(command);
  mUartStream.write(command);
  mUartStream.write("\r\n");
}

//responses from modem are in the form <CR><LF><response><CR><LF>
//if successful, places just <response> in the mRxBuffer
bool Sim7kInterface::readLineFromUart(const uint32_t timeout)
{
  if (!mUartStream.available())
  {
    return false;
  }
  
  bool foundLineFeed{false};

  for (int i{0}; i < RX_BUFFER_SIZE; i++)
  {
   char nextByte = mUartStream.read();

    //need this loop because arduino will read faster than uart stream transmits
    const uint32_t startTimer = millis();
    while (nextByte == -1)
    {
      if (millis() - startTimer > timeout)
      {
        writeToLog("readLineFromUart() timed out.");
        mRxBuffer[0] = '\0';
        return false; 
      }
      
      nextByte = mUartStream.read();
    }

    switch (nextByte)
    {
      case '\n':
      if (foundLineFeed)
      {
          mRxBuffer[i] = '\0';
          writeToLog("Received response from modem:");
          writeToLog(mRxBuffer);
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

  writeToLog("Failed to read line from UART buffer.");

  //set buffer to empty string
  mRxBuffer[0] = '\0';

  //read to buffer failed, so finish flushing the line from the uart stream to avoid leaving a partially consumed response on the buffer
  const uint32_t startTimer = millis();
  char nextByte{0};
  while (!foundLineFeed && nextByte != '\n')
  {
    if (millis() - startTimer > timeout)
    {
      writeToLog("readLineFromUart() timed out while flushing buffer after read failure.");
      return false;
    }
    
    if (nextByte == '\n')
    {
      foundLineFeed = true;
    }
    
    nextByte = mUartStream.read();
  }
  
  return false;
}

void Sim7kInterface::flushUart()
{
  while (mUartStream.available())
  {
    readLineFromUart();
  }
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
    mLog->write("Sim7k log - ");
    mLog->println(msg);
  }
}

