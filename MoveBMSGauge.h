#ifndef MOVEBMSGAUGE_H
#define MOVEBMSGAUGE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

#define DAC_PIN 25

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

  bool loadVehicleConfig(const char* vehicleName);

  bool connectBMS();

  void outputDAC(uint8_t soc);

  static MoveBMSGauge* instance;

  static void notifyCallback(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t len,
    bool isNotify
  );
};

#endif
