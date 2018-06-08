#include <SoftwareSerial.h>

#define RX_BUFFER_SIZE 256

class HardwareSerial;

class Sim7kInterface
{
  public:
  Sim7kInterface();

  void setLogStream(HardwareSerial* log);
  bool turnOn();
  bool turnOff();
  bool isOn();
  bool turnOnGnss();
  
  private:
  void sendCommand(const char* command);
  bool readLineFromUart(const uint32_t timeout = 5000);
  void flushUart();
  bool checkResponse(const char* expectedResponse);
  void writeToLog(const char* msg);
  void sendInitialSettings();
  
  SoftwareSerial mUartStream;
  char mRxBuffer[RX_BUFFER_SIZE];
  HardwareSerial* mLog;
};

