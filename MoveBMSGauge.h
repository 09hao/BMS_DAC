#ifndef MOVEBMSGAUGE_H
#define MOVEBMSGAUGE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

#define DAC_PIN 25

#define DEBUG_RAW_PACKET 0

#define MAX_CELL_COUNT 24
#define MIN_CELL_COUNT 6

#define CELL_MIN_VALID_MV 2500
#define CELL_MAX_VALID_MV 3700

#define CELL_WARN_DELTA_MV 80
#define CELL_FAULT_DELTA_MV 150
#define CELL_LOW_FAULT_MV 2800

struct VehicleConfig
{
  const char* name;
  uint8_t dacInit;
  uint8_t dacMin;
  uint8_t dacMax;
  uint8_t socIndex;
};

class MoveBMSGauge
{
public:
  MoveBMSGauge();

  void begin(const char* vehicleName, const char* bmsAddr);
  void loop();

private:
  BLEClient* pClient;
  BLERemoteCharacteristic* pChar;

  const char* addr;

  VehicleConfig vehicle;

  BLEUUID serviceUUID;
  BLEUUID charUUID;

  float socFiltered;

  unsigned long lastPacketTime;

  uint16_t cellMV[MAX_CELL_COUNT];
  uint16_t cellMinMV;
  uint16_t cellMaxMV;
  uint16_t cellDeltaMV;

  uint8_t detectedCellStart;
  uint8_t detectedCellCount;

  bool cellWarn;
  bool cellFault;

  bool loadVehicleConfig(const char* vehicleName);
  bool connectBMS();

  void printRawPacket(uint8_t* data, size_t len);

  bool autoDetectCells(uint8_t* data, size_t len);
  bool readDetectedCells(uint8_t* data, size_t len);

  void checkCellFault();
  void printCellInfo();

  void outputDAC(uint8_t soc);
  void outputCellFault();

  static MoveBMSGauge* instance;

  static void notifyCallback(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t len,
    bool isNotify
  );
};

#endif
