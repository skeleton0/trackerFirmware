#include "Sim7kInterface.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface();
  sim7k->setLogStream(&Serial);
  
  sim7k->turnOff();
  sim7k->turnOn();
}

void loop() {
  sim7k->tick();
}
