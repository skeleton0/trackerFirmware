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

  if (sim7k->isOn())
  {
    const size_t bufferSize{100};
    char responseBuffer[bufferSize];
    sim7k->sendCommand("AT+CSQ\r\n", responseBuffer, bufferSize);

    Serial.print("response from sim7k: ");
    Serial.println(responseBuffer);
    delay(1000);
  }
}
