#include "Sim7kInterface.h"

#include <Arduino.h>

Sim7kInterface::Sim7kInterface(HardwareSerial* log) :
mLog(log),
mUartStream(10, 11) {
  //init buffer with empty string
  mRxBuffer[0] = '\0';
  
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  
  mUartStream.begin(4800);

  sendCommand("AT");
  if (checkNextResponse("ATOK") || checkLastResponse("OK")) { //ATOK when echo mode is enabled
    writeToLog("Modem is initially on.");
    sendInitialSettings();
  }
  else {
    writeToLog("Modem is initially off."); 
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
    writeToLog("Turned on modem via pin 6.");
    return true;
  }
  else {
    writeToLog("Failed to start modem via pin 6.");
    writeToLog("Attempting to start modem with emergency reset via pin 7.");

    if(turnOnViaPin(7)) {
      writeToLog("Emergency reset successful.");
      return true;
    }

    writeToLog("Emergency reset failed.");
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
  writeToLog("Checking if modem is on...");
  sendCommand("AT");
  return checkNextResponse("OK");
}

bool Sim7kInterface::turnOnGnss() {
  sendCommand("AT+CGNSPWR=1");

  return checkNextResponse("OK");
}

bool Sim7kInterface::cstt(const char* apn) {
  const size_t maxApnLen{50};
  if (strnlen(apn, maxApnLen) == maxApnLen) {
    writeToLog("Apn is too long.");
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
  const int MAX_PROTO_LEN{4};
  if (strnlen(protocol, MAX_PROTO_LEN) == MAX_PROTO_LEN) {
    writeToLog("Argument 'protocol' passed to cipstart is too long.");
    return false;
  }

  const int MAX_ADDR_LEN{25};
  if (strnlen(address, MAX_ADDR_LEN) == MAX_ADDR_LEN) {
    writeToLog("Argument 'address' passed to cipstart is too long.");
    return false;
  }

  const int MAX_PORT_LEN{6};
  if (strnlen(port, MAX_PORT_LEN) == MAX_PORT_LEN) {
    writeToLog("Argument 'port' passed to cipstart is too long.");
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

  return ConnectionState::UNDEFINED;
}

void Sim7kInterface::sendCommand(const char* command) {
  writeToLog("Sending to modem:");
  writeToLog(command);
  mUartStream.write(command);
  mUartStream.write("\r\n");
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
        writeToLog("readLineFromUart() timed out.");
        mRxBuffer[0] = '\0';
        return false; 
      }
      
      nextByte = mUartStream.read();
    }

    switch (nextByte) {
      case '\n':
      if (foundLineFeed) {
          mRxBuffer[i] = '\0';
          writeToLog("Received response from modem:");
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

  writeToLog("Failed to read line from UART buffer.");

  //set buffer to empty string
  mRxBuffer[0] = '\0';

  //read to buffer failed, so finish flushing the line from the uart stream to avoid leaving a partially consumed response on the buffer
  const uint32_t startTimer = millis();
  char nextByte{0};
  while (!foundLineFeed && nextByte != '\n') {
    if (millis() - startTimer > timeout) {
      writeToLog("readLineFromUart() timed out while flushing buffer after read failure.");
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
  writeToLog("Flushing UART stream.");
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

void Sim7kInterface::writeToLog(const char* msg) {
  if (mLog) {
    mLog->write("Sim7k log - ");
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

