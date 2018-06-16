#include "Sim7kInterface.h"
#include "Config.h"

Sim7kInterface* sim7k;
Sim7kInterface::ConnectionState state;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);
  state = sim7k->queryConnectionState();
}

void loop() {
  //finite state machine  
  switch (state)
  {
    case Sim7kInterface::ConnectionState::MODEM_OFF:
    sim7k->turnOn();
    sim7k->turnOnGnss();
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_INITIAL:
    sim7k->cstt(APN);
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_START:
    sim7k->ciicr();
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_GPRSACT:
    sim7k->cifsr();
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::IP_STATUS:
    case Sim7kInterface::ConnectionState::TCP_CLOSED:
    sim7k->cipstart("TCP", SERVER_ADDR, SERVER_PORT);
    state = sim7k->queryConnectionState();
    break;

    case Sim7kInterface::ConnectionState::CONNECT_OK:
    if (sim7k->checkPositionChange()) {
      if (!sim7k->sendGnssUpdate(DEVICE_ID)) {
        state = sim7k->queryConnectionState();
      }
    }
    break;

    case Sim7kInterface::ConnectionState::PDP_DEACT:
    sim7k->cipshut();
    state = sim7k->queryConnectionState();
    break;

    default:
    state = sim7k->queryConnectionState();
    break;
  }
}

void writeToLog(const char* msg) {
  Serial.print(F("Main log - "));
  Serial.println(msg);
}

