#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface(HardwareSerial* log) :
mLog{log},
mUartStream{10, 11} {
  //init gnss cache
  mGnssCache.mLatitude[0] = '\0';
  mGnssCache.mLongitude[0] = '\0';
  mGnssCache.mTimestamp[0] = '\0';
  mGnssCache.mSpeedOverGround[0] = '\0';
  mGnssCache.mCourseOverGround[0] = '\0';
  
  //init rx buffer with empty string
  mRxBuffer[0] = '\0';
  
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  
  mUartStream.begin(4800);

  sendCommand("AT");
  if (checkNextResponse("ATOK") || checkLastResponse("OK")) { //ATOK when echo mode is enabled
    writeToLog(F("Modem is initially on."));
    sendInitialSettings();
  }
  else {
    writeToLog(F("Modem is initially off.")); 
  }
  
}

bool Sim7kInterface::turnOn() {
  if (isOn()) {
    return true;
  }

  auto turnOnViaPin = [this](const uint8_t pin) {
    digitalWrite(pin, LOW);
    delay(300);
    digitalWrite(pin, HIGH);
    delay(10000);

    flushUart();

    sendCommand("AT");
    if (checkNextResponse("ATOK") || checkLastResponse("OK")) {
      sendInitialSettings();
      return true;
    }

    return false;
  };

  if (turnOnViaPin(6)) {
    writeToLog(F("Turned on modem via pin 6."));
    return true;
  }
  else {
    writeToLog(F("Failed to start modem via pin 6."));
    writeToLog(F("Attempting to start modem with emergency reset via pin 7."));

    if(turnOnViaPin(7)) {
      writeToLog(F("Emergency reset successful."));
      return true;
    }

    writeToLog(F("Emergency reset failed."));
  }
  
  return false;
}

bool Sim7kInterface::turnOff() {
  if (!isOn()) {
    return true;
  }
  
  sendCommand("AT+CPOWD=1");
  return checkNextResponse("NORMAL POWER DOWN");
}

bool Sim7kInterface::isOn() {
  writeToLog(F("Checking if modem is on..."));
  sendCommand("AT");
  return checkNextResponse("OK");
}

bool Sim7kInterface::turnOnGnss() {
  sendCommand("AT+CGNSPWR=1");

  return checkNextResponse("OK");
}

bool Sim7kInterface::checkPositionChange() {
  sendCommand("AT+CGNSINF");

  if (!readLineFromUart()) {
    writeToLog(F("Failed to read response from AT+CGNSINF."));
    return false;
  }

  strtok(mRxBuffer, ","); //GNSS run status

  if (strcmp(strtok(nullptr, ","), "1") != 0) { //Fix status
     writeToLog(F("GNSS doesn't have fix position."));
     flushUart();
     return false;
  }

  strncat(mGnssCache.mTimestamp, strtok(nullptr, ","), TIMESTAMP_SIZE - 1);
  strncat(mGnssCache.mLatitude, strtok(nullptr, ","), LAT_SIZE - 1);
  strncat(mGnssCache.mLongitude, strtok(nullptr, ","), LONG_SIZE - 1);
  strtok(nullptr, ","); //altitude (we don't care about it... for now)
  strncat(mGnssCache.mSpeedOverGround, strtok(nullptr, ","), SOG_SIZE - 1);
  strncat(mGnssCache.mCourseOverGround, strtok(nullptr, ","), COG_SIZE - 1);

  float speedOverGround = atof(mGnssCache.mSpeedOverGround);

  if (checkNextResponse("OK") && speedOverGround > 0.0f) {
    writeToLog(F("Speed over ground is greater than 0, so we've got a valid position change."));
    return true;
  }

  writeToLog(F("Speed over ground is 0."));
  return false;
}

bool Sim7kInterface::cstt(const char* apn) {
  const size_t maxApnLen{50};
  if (strnlen(apn, maxApnLen) == maxApnLen) {
    writeToLog(F("Apn is too long."));
    return false;
  }
  
  char command[60] = "AT+CSTT=\"";
  strcat(command, apn);
  strcat(command, "\"");

  sendCommand(command);
  
  return checkNextResponse("OK");
}

bool Sim7kInterface::ciicr() {
  sendCommand("AT+CIICR");

  return checkNextResponse("OK", 85000);
}

bool Sim7kInterface::cipshut() {
  sendCommand("AT+CIPSHUT");

  return checkNextResponse("SHUT OK");
}

bool Sim7kInterface::cifsr() {
  sendCommand("AT+CIFSR");

  return !checkNextResponse("ERROR");
}

bool Sim7kInterface::cipstart(const char* protocol, const char* address, const char* port) {
  const size_t MAX_PROTO_LEN{4};
  if (strnlen(protocol, MAX_PROTO_LEN) == MAX_PROTO_LEN) {
    writeToLog(F("Argument 'protocol' passed to cipstart is too long."));
    return false;
  }

  const size_t MAX_ADDR_LEN{25};
  if (strnlen(address, MAX_ADDR_LEN) == MAX_ADDR_LEN) {
    writeToLog(F("Argument 'address' passed to cipstart is too long."));
    return false;
  }

  const size_t MAX_PORT_LEN{6};
  if (strnlen(port, MAX_PORT_LEN) == MAX_PORT_LEN) {
    writeToLog(F("Argument 'port' passed to cipstart is too long."));
    return false;
  }

  char command[60] = "AT+CIPSTART=\"";
  strcat(command, protocol);
  strcat(command, "\",\"");
  strcat(command, address);
  strcat(command, "\",");
  strcat(command, port);

  sendCommand(command);

  return checkNextResponse("OK") && checkNextResponse("CONNECT OK", 75);
}

bool Sim7kInterface::sendGnssUpdate(const char* id) {
  sendCommand("AT+CIPSEND");

  if (!checkNextResponse(">")) {
    writeToLog(F("AT+CIPSEND didn't return '>' prompt."));
    return false;
  }

  //build json msg in format of {"id":"0","t":"yyyyMMddhhmmss.sss","lat":"+dd.dddddd","lon":"+ddd.dddddd","sog":"999.99","cog","360.00"}
  char jsonMsg[105] = "{\"id\":\"";
  strncat(jsonMsg, id, 1);
  strcat(jsonMsg, "\",\"t\":\"");
  strcat(jsonMsg, mGnssCache.mTimestamp);
  strcat(jsonMsg, "\",\"lat\":\"");
  strcat(jsonMsg, mGnssCache.mLatitude);
  strcat(jsonMsg, "\",\"lon\":\"");
  strcat(jsonMsg, mGnssCache.mLongitude);
  strcat(jsonMsg, "\",\"sog\":\"");
  strcat(jsonMsg, mGnssCache.mSpeedOverGround);
  strcat(jsonMsg, "\",\"cog\":\"");
  strcat(jsonMsg, mGnssCache.mCourseOverGround);
  strcat(jsonMsg, "\"}");

  sendCommand(jsonMsg);

  mUartStream.write(0x1A); //communicates end of msg to sim7k

  return checkNextResponse("SEND OK", 30); 
}

Sim7kInterface::ConnectionState Sim7kInterface::queryConnectionState() {
  sendCommand("AT+CIPSTATUS");

  if (checkNextResponse("OK")) {
    if (checkNextResponse("STATE: IP INITIAL")) {
      return ConnectionState::IP_INITIAL;
    }
    else if (checkLastResponse("STATE: IP START")) {
      return ConnectionState::IP_START;
    }
    else if (checkLastResponse("STATE: IP CONFIG")) {
      return ConnectionState::IP_CONFIG;
    }
    else if (checkLastResponse("STATE: IP GPRSACT")) {
      return ConnectionState::IP_GPRSACT;
    }
    else if (checkLastResponse("STATE: IP STATUS")) {
      return ConnectionState::IP_STATUS;
    }
    else if (checkLastResponse("STATE: TCP CONNECTING")) {
      return ConnectionState::TCP_CONNECTING;
    }
    else if (checkLastResponse("STATE: CONNECT OK")) {
      return ConnectionState::CONNECT_OK;
    }
    else if (checkLastResponse("STATE: TCP CLOSED")) {
      return ConnectionState::TCP_CLOSED;
    }
    else if (checkLastResponse("STATE: PDP DEACT")) {
      return ConnectionState::PDP_DEACT;
    }
  }

  return ConnectionState::MODEM_OFF;
}

void Sim7kInterface::sendCommand(const char* command) {
  writeToLog(F("Sending to modem:"));
  writeToLog(command);
  mUartStream.write(command);
  mUartStream.write("\r");
  delay(1000); //give modem time to respond
}

//responses from modem are in the form <CR><LF><response><CR><LF>
//if successful, places just <response> in the mRxBuffer
bool Sim7kInterface::readLineFromUart(const uint32_t timeout) { 
  bool foundLineFeed{false};

  for (int i{0}; i < RX_BUFFER_SIZE; i++) {
   char nextByte = mUartStream.read();

    //need this loop because arduino will read faster than uart stream transmits
    const uint32_t startTimer = millis();
    while (nextByte == -1) {
      if (millis() - startTimer > timeout) {
        writeToLog(F("readLineFromUart() timed out."));
        mRxBuffer[0] = '\0';
        return false; 
      }
      
      nextByte = mUartStream.read();
    }

    switch (nextByte) {
      case '\n':
      if (foundLineFeed) {
          mRxBuffer[i] = '\0';
          writeToLog(F("Received response from modem:"));
          writeToLog(mRxBuffer);
          return true;
      }
      else {
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

  writeToLog(F("Failed to read line from UART buffer."));

  //set buffer to empty string
  mRxBuffer[0] = '\0';

  //read to buffer failed, so finish flushing the line from the uart stream to avoid leaving a partially consumed response on the buffer
  const uint32_t startTimer = millis();
  char nextByte{0};
  while (!foundLineFeed && nextByte != '\n') {
    if (millis() - startTimer > timeout) {
      writeToLog(F("readLineFromUart() timed out while flushing buffer after read failure."));
      return false;
    }
    
    if (nextByte == '\n') {
      foundLineFeed = true;
    }
    
    nextByte = mUartStream.read();
  }
  
  return false;
}

void Sim7kInterface::flushUart() {
  writeToLog(F("Flushing UART stream."));
  while (mUartStream.available()) {
    readLineFromUart();
  }
}

bool Sim7kInterface::checkNextResponse(const char* expectedResponse, const uint32_t timeout) {
  if (readLineFromUart(timeout)) {
    return strcmp(mRxBuffer, expectedResponse) == 0;
  }

  return false;
}

bool Sim7kInterface::checkLastResponse(const char* expectedResponse) {
  return strcmp(mRxBuffer, expectedResponse) == 0;
}

void Sim7kInterface::writeToLog(const __FlashStringHelper* msg) {
  if (mLog) {
    mLog->print(F("Sim7k log - "));
    mLog->println(msg);
  }
}

void Sim7kInterface::writeToLog(const char* msg) {
  if (mLog) {
    mLog->print(F("Sim7k log - "));
    mLog->println(msg);
  }
}

void Sim7kInterface::sendInitialSettings() {
  //set initial settings
  sendCommand("AT+IPR=4800"); //set baud rate to 4800
  sendCommand("ATE0");        //disable echo mode
  sendCommand("AT+CNMP=38");  //use LTE only
  sendCommand("AT+CMNB=1");   //use CAT-M only

  flushUart();
}

