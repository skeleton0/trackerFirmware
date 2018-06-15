#include <SoftwareSerial.h>

#define RX_BUFFER_SIZE 256

class HardwareSerial;

class Sim7kInterface
{
  public:
  Sim7kInterface(HardwareSerial* log = nullptr);

  enum class ConnectionState
  {
    IP_INITIAL,
    IP_START,
    IP_CONFIG,
    IP_GPRSACT,
    IP_STATUS,
    TCP_CONNECTING,
    CONNECT_OK,
    TCP_CLOSED,
    PDP_DEACT,
    UNDEFINED,
  };

  bool turnOn();
  bool turnOff();
  bool isOn();
  bool turnOnGnss();
  ConnectionState getConnectionState();
  bool setApn(const char* apn);
  bool bringUpGprsConnection();
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

