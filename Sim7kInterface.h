#include <SoftwareSerial.h>

class HardwareSerial;

class Sim7kInterface
{
  public:
  Sim7kInterface();

  void setLogStream(HardwareSerial* log);
  void tick();
  void turnOn();
  void turnOff();
  bool isOn();
  bool sendCommand(const char* command, char* response, const size_t bufferSize);

  private:
  bool readLineFromUart(char* response, const size_t bufferSize);
  void handleUnsolicitedResponse(const char* response);
  void writeToLog(const char* msg);
  
  SoftwareSerial mUartStream;
  bool mIsOn;
  HardwareSerial* mLog;
};

