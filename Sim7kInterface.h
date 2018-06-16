#include <SoftwareSerial.h>

#define RX_CACHE_SIZE 256
#define TIMESTAMP_SIZE 19 //yyyyMMddhhmmss.sss
#define LAT_SIZE 11
#define LON_SIZE 12
#define SOG_SIZE 7
#define COG_SIZE 7

class HardwareSerial;

struct GnssData {
  char mLatitude[LAT_SIZE];
  char mLongitude[LON_SIZE];
  char mTimestamp[TIMESTAMP_SIZE];
  char mSpeedOverGround[SOG_SIZE];
  char mCourseOverGround[COG_SIZE];
};

class Sim7kInterface {
  public:
  Sim7kInterface(HardwareSerial* log = nullptr);

  enum class ConnectionState {
    IP_INITIAL,
    IP_START,
    IP_CONFIG,
    IP_GPRSACT,
    IP_STATUS,
    TCP_CONNECTING,
    CONNECT_OK,
    TCP_CLOSED,
    PDP_DEACT,
    MODEM_OFF,
  };

  bool turnOn();
  bool turnOff();
  bool isOn();
  bool turnOnGnss();
  bool checkPositionChange();
  bool cstt(const char* apn);
  bool ciicr();
  bool cipshut();
  bool cifsr();
  bool cipstart(const char* protocol, const char* address, const char* port);
  bool sendGnssUpdate(const char* id, bool hologramCloudMode = false);
  ConnectionState queryConnectionState();
  
  private:
  void sendCommand(const char* command);
  bool readLineFromUart(const uint32_t timeout = 5000);
  void flushUart();
  bool checkNextResponse(const char* expectedResponse, const uint32_t timeout = 5000);
  bool checkLastResponse(const char* expectedResponse);
  void writeToLog(const __FlashStringHelper* msg);
  void writeToLog(const char* msg);
  void sendInitialSettings();
  
  SoftwareSerial mUartStream;
  char mRxCache[RX_CACHE_SIZE];
  GnssData mGnssCache;
  HardwareSerial* mLog;
};

