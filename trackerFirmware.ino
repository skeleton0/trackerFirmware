#include "Sim7kInterface.h"
#include "Config.h"

enum class ModemState
{
  OFF,
  ON,
  IP_INITIAL,
  IP_START,
  IP_CONFIG,
  IP_GPRSACT,
  IP_STATUS,
  TCP_CONNECTING,
  CONNECT_OK,
  TCP_CLOSED,
  PDP_DEACT,
  UNDEFINED
};

ModemState state {ModemState::OFF};
Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);
}

void loop() {
  
}

void writeToLog(const char* msg)
{
  Serial.print("Main log - ");
  Serial.println(msg);
}

