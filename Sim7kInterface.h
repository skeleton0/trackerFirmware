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

  private:
  bool handleUnsolicitedMsg(const char* const msg);
  void writeToLog(const char* const msg);
  
  SoftwareSerial mUartStream;
  bool mIsOn;
  HardwareSerial* mLog;
};

