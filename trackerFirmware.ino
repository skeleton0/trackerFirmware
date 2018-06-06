#include "Sim7kInterface.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface();
  sim7k->setLogStream(&Serial);
  
  sim7k->turnModemOff();
  sim7k->turnModemOn();
}

void loop() {
  sim7k->tick();
}
