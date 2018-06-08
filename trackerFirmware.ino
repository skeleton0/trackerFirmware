#include "Sim7kInterface.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface();
  sim7k->setLogStream(&Serial);

  if (sim7k->turnOn())
  {
    Serial.println("Modem is on.");
  }
  else
  {
    Serial.println("Failed to turn modem on.");
  }
}

void loop() {
}
