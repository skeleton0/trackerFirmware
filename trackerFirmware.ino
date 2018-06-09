#include "Sim7kInterface.h"
#include "Config.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);

  if (!sim7k->turnOn())
  {
    Serial.println("Failed to turn modem on.");
    return;
  }

  Serial.println("Modem is on.");
  
  if (!sim7k->turnOnGnss())
  {
    Serial.println("Failed to turn GNSS on.");
    return;
  }

  Serial.println("GNSS is on.");

  if (!sim7k->setApn(APN))
  {
    Serial.println("Failed to set APN.");
    return;
  }

  Serial.println("Set APN.");
}

void loop() {
}
