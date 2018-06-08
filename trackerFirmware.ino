#include "Sim7kInterface.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);

  if (sim7k->turnOn())
  {
    Serial.println("Modem is on.");

    if (sim7k->turnOnGnss())
    {
      Serial.println("GNSS is on.");
    }
    else
    {
      Serial.println("Failed to turn GNSS on.");
    }

    sim7k->turnOff();
  }
  else
  {
    Serial.println("Failed to turn modem on.");
  }
}

void loop() {
}
