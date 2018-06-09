#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface(HardwareSerial* log) :
mLog(log),
mUartStream(10, 11)
{
  //init buffer with empty string
  mRxBuffer[0] = '\0';
  
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  
  mUartStream.begin(4800);

  sendCommand("AT");
  if (checkNextResponse("ATOK") || checkLastResponse("OK")) //ATOK when echo mode is enabled
  {
    writeToLog("Modem is initially on.");
    sendInitialSettings();
  }
  else
  {
    writeToLog("Modem is initially off."); 
  }
  
}

bool Sim7kInterface::turnOn()
{
  if (isOn())
  {
    return true;
  }

  auto turnOnViaPin = [this](const uint8_t pin) -> bool
  {
    digitalWrite(pin, LOW);
    delay(300);
    digitalWrite(pin, HIGH);
    delay(10000);

    flushUart();

    sendCommand("AT");
    if (checkNextResponse("ATOK") || checkLastResponse("OK"))
    {
      sendInitialSettings();
      return true;
    }

    return false;
  };

  if (turnOnViaPin(6))
  {
    writeToLog("Turned on modem via pin 6.");
    return true;
  }
  else
  {
    writeToLog("Failed to start modem via pin 6.");
    writeToLog("Attempting to start modem with emergency reset via pin 7.");

    if(turnOnViaPin(7))
    {
      writeToLog("Emergency reset successful.");
      return true;
    }

    writeToLog("Emergency reset failed.");
  }
  
  return false;
}

bool Sim7kInterface::turnOff()
{
  if (!isOn())
  {
    return true;
  }
  
  sendCommand("AT+CPOWD=1");
  return checkNextResponse("NORMAL POWER DOWN");
}

bool Sim7kInterface::isOn()
{
  writeToLog("Checking if modem is on...");
  sendCommand("AT");
  return checkNextResponse("OK");
}

bool Sim7kInterface::turnOnGnss()
{
  sendCommand("AT+CGNSPWR=1");

  return checkNextResponse("OK");
}

bool Sim7kInterface::setApn(const char* apn)
{
  const size_t maxApnLen{50};
  if (strlen(apn) == maxApnLen)
  {
    writeToLog("Apn is too long.");
    return false;
  }

  //check if APN has already been set to this APN
  sendCommand("AT+CSTT?");

  //response comes in the form: +CSTT: "apn","","" 
  char expectedResponse[65] = "+CSTT: \"";
  strcat(expectedResponse, apn);
  strcat(expectedResponse, "\",\"\",\"\"");
  
  if (checkNextResponse(expectedResponse) && checkNextResponse("OK"))
  {
    writeToLog("APN was already set.");
  }
  else
  {
    writeToLog("APN was not already set.");
    
    char command[60] = "AT+CSTT=\"";
    strcat(command, apn);
    strcat(command, "\"");

    sendCommand(command);

    if (!checkNextResponse("OK"))
    {
      writeToLog("Failed to set APN.");
    }
  }

  //set APN in bearer
  char command[71] = "AT+SAPBR=3,1,\"APN\",\"";
  strcat(command, apn);
  strcat(command, "\"");

  sendCommand(command);

  if (!checkNextResponse("OK"))
  {
    writeToLog("Failed to set APN in bearer settings.");
    return false;
  }

  writeToLog("Set APN in bearer settings.");
  return true;
}

bool Sim7kInterface::bringUpGprsConnection()
{
  sendCommand("AT+CIICR");

  return checkNextResponse("OK");
}

bool Sim7kInterface::openBearer()
{
  sendCommand("AT+SAPBR=1,1");

  return checkNextResponse("OK");
}

void Sim7kInterface::sendCommand(const char* command)
{
  writeToLog("Sending to modem:");
  writeToLog(command);
  mUartStream.write(command);
  mUartStream.write("\r\n");
  delay(1000); //give modem time to respond
}

//responses from modem are in the form <CR><LF><response><CR><LF>
//if successful, places just <response> in the mRxBuffer
bool Sim7kInterface::readLineFromUart(const uint32_t timeout)
{ 
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
  writeToLog("Flushing UART stream.");
  while (mUartStream.available())
  {
    readLineFromUart();
  }
}

bool Sim7kInterface::checkNextResponse(const char* expectedResponse)
{
  if (readLineFromUart())
  {
    return strcmp(mRxBuffer, expectedResponse) == 0;
  }

  return false;
}

bool Sim7kInterface::checkLastResponse(const char* expectedResponse)
{
  return strcmp(mRxBuffer, expectedResponse) == 0;
}

void Sim7kInterface::writeToLog(const char* msg)
{
  if (mLog)
  {
    mLog->write("Sim7k log - ");
    mLog->println(msg);
  }
}

void Sim7kInterface::sendInitialSettings()
{
  //set initial settings
  sendCommand("AT+IPR=4800"); //set baud rate to 4800
  sendCommand("ATE0");        //disable echo mode
  sendCommand("AT+CNMP=38");  //use LTE only
  sendCommand("AT+CMNB=1");   //use CAT-M only

  flushUart();
}

