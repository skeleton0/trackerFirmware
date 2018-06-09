#include "Sim7kInterface.h"
#include "Config.h"

Sim7kInterface* sim7k;

void setup() {
  Serial.begin(4800);
  sim7k = new Sim7kInterface(&Serial);

  if (!sim7k->turnOn())
  {
    writeToLog("Failed to turn modem on.");
    return;
  }

  writeToLog("Modem is on.");
  
  if (!sim7k->turnOnGnss())
  {
    writeToLog("Failed to turn GNSS on.");
    return;
  }

  writeToLog("GNSS is on.");

  if (!sim7k->setApn(APN))
  {
    writeToLog("Failed to set APN.");
    return;
  }

  writeToLog("Set APN.");
}

void loop() {
}

void writeToLog(const char* msg)
{
  Serial.print("Main log - ");
  Serial.println(msg);
}

