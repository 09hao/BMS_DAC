#include "MoveBMSGauge.h"

#define DAC_PIN 25

BLEUUID serviceUUID("0000FFE0-0000-1000-8000-00805F9B34FB");
BLEUUID charUUID("0000FFE1-0000-1000-8000-00805F9B34FB");

uint8_t loginCmd[] = {
  0xA5, 0x0B, 0x00, 0x58, 0x58,
  0x19, 0x0A, 0x1E,
  0x0E, 0x28, 0x0D
};

MoveBMSGauge* MoveBMSGauge::instance = nullptr;

MoveBMSGauge::MoveBMSGauge(BLEAddress address)
  : bmsAddr(address),
    pClient(nullptr),
    pChar(nullptr),
    connected(false)
{
  instance = this;
}

void MoveBMSGauge::begin()
{
  pinMode(DAC_PIN, OUTPUT);
  dacWrite(DAC_PIN, 130);

  BLEDevice::init("");

  connectBMS();
}

void MoveBMSGauge::loop()
{
  if (pClient && !pClient->isConnected())
  {
    connected = false;

    Serial.println("Disconnected");

    dacWrite(DAC_PIN, 130);

    delay(2000);

    connectBMS();
  }

  delay(1000);
}

bool MoveBMSGauge::connectBMS()
{
  Serial.println("Connecting...");

  pClient = BLEDevice::createClient();

  if (!pClient->connect(bmsAddr))
  {
    Serial.println("Connect fail");
    return false;
  }

  Serial.println("Connected");

  BLERemoteService* service = pClient->getService(serviceUUID);

  if (service == nullptr)
  {
    Serial.println("No FFE0");
    pClient->disconnect();
    return false;
  }

  pChar = service->getCharacteristic(charUUID);

  if (pChar == nullptr)
  {
    Serial.println("No FFE1");
    pClient->disconnect();
    return false;
  }

  pChar->registerForNotify(notifyCallback);

  delay(500);

  Serial.println("Send Login");

  pChar->writeValue(loginCmd, sizeof(loginCmd), true);

  connected = true;

  Serial.println("Ready");

  return true;
}

void MoveBMSGauge::notifyCallback(
  BLERemoteCharacteristic* c,
  uint8_t* data,
  size_t len,
  bool isNotify
)
{
  if (instance)
  {
    instance->handleData(data, len);
  }
}

void MoveBMSGauge::handleData(uint8_t* data, size_t len)
{
  if (len < 12) return;
  if (data[0] != 0xA5) return;

  uint8_t soc = data[11];

  soc = constrain(soc, 0, 100);

  Serial.print("SOC=");
  Serial.print(soc);
  Serial.println("%");

  outputDAC(soc);
}

void MoveBMSGauge::outputDAC(uint8_t soc)
{
  int dacValue = map(soc, 0, 100, 127, 155);

  dacWrite(DAC_PIN, dacValue);

  Serial.print("DAC=");
  Serial.print(dacValue);
  Serial.print(" V=");
  Serial.println(dacValue * 3.3 / 255.0, 2);
}
