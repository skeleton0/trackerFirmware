#include "Sim7kInterface.h"
#include "Config.h"

Sim7kInterface* sim7k;
Sim7kInterface::ConnectionState state;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);
  state = Sim7kInterface::ConnectionState::UNDEFINED;
}

void loop() {  
  switch (state)
  {
    case Sim7kInterface::ConnectionState::UNDEFINED:
    sim7k->turnOn();
    sim7k->turnOnGnss();
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_INITIAL:
    sim7k->cstt("hologram");
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_START:
    sim7k->ciicr();
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_CONFIG:
    sim7k->cifsr();
    state = sim7k->queryConnectionState();
    break;
  }
}

void writeToLog(const char* msg) {
  Serial.print("Main log - ");
  Serial.println(msg);
}

