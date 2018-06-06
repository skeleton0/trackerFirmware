#include <SoftwareSerial.h>

class HardwareSerial;

class Sim7kInterface
{
  public:
  Sim7kInterface();

  void setLogStream(HardwareSerial* log);
  void tick();
  void turnModemOn();
  void turnModemOff();
  bool modemIsOn();
  bool turnOnGnss();
  
  private:
  bool readLineFromUart(char* response, const size_t bufferSize);
  bool sendCommand(const char* command, char* response, const size_t bufferSize);
  void handleUnsolicitedResponse(const char* response);
  void writeToLog(const char* msg);
  
  SoftwareSerial mUartStream;
  bool mModemIsOn;
  HardwareSerial* mLog;
};

