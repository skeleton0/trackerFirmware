#include <SoftwareSerial.h>

#define RX_BUFFER_SIZE 256

class HardwareSerial;

class Sim7kInterface
{
  public:
  Sim7kInterface(HardwareSerial* log = nullptr);

  bool turnOn();
  bool turnOff();
  bool isOn();
  bool turnOnGnss();
  bool setApn(const char* apn);
  bool bringUpGprsConnection();
  bool openBearer();
  bool isInDeactState();
  bool deactGprs();
  bool isAssignedIp();
  
  private:
  void sendCommand(const char* command);
  bool readLineFromUart(const uint32_t timeout = 5000);
  void flushUart();
  bool checkNextResponse(const char* expectedResponse, const uint32_t timeout = 5000);
  bool checkLastResponse(const char* expectedResponse);
  void writeToLog(const char* msg);
  void sendInitialSettings();
  
  SoftwareSerial mUartStream;
  char mRxBuffer[RX_BUFFER_SIZE];
  HardwareSerial* mLog;
};

