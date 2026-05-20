#ifndef MOVE_BMS_GAUGE_H
#define MOVE_BMS_GAUGE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

class MoveBMSGauge
{
public:
  MoveBMSGauge(BLEAddress address);

  void begin();
  void loop();

private:
  BLEAddress bmsAddr;

  BLEClient* pClient;
  BLERemoteCharacteristic* pChar;

  bool connected;

  static MoveBMSGauge* instance;

  static void notifyCallback(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t len,
    bool isNotify
  );

  void handleData(uint8_t* data, size_t len);
  bool connectBMS();
  void outputDAC(uint8_t soc);
};

#endif
