#include "MoveBMSGauge.h"

MoveBMSGauge* MoveBMSGauge::instance = nullptr;

uint8_t loginCmd[] =
{
  0xA5, 0x0B, 0x00, 0x58, 0x58,
  0x19, 0x0A, 0x1E,
  0x0E, 0x28, 0x0D
};

MoveBMSGauge::MoveBMSGauge()
  : pClient(nullptr),
    pChar(nullptr),
    addr(nullptr),
    serviceUUID("0000FFE0-0000-1000-8000-00805F9B34FB"),
    charUUID("0000FFE1-0000-1000-8000-00805F9B34FB"),
    socFiltered(-1),
    lastPacketTime(0)
{
  instance = this;
}

bool MoveBMSGauge::loadVehicleConfig(const char* vehicleName)
{
  if (strcmp(vehicleName, "ISB") == 0)
  {
    vehicle.name = "ISB";

    vehicle.dacInit = 130;

    vehicle.dacMin = 127;

    vehicle.dacMax = 155;

    vehicle.socIndex = 11;

    return true;
  }

  if (strcmp(vehicleName, "ATN") == 0)
  {
    vehicle.name = "ATN";

    vehicle.dacInit = 50;

    vehicle.dacMin = 62;

    vehicle.dacMax = 72;

    vehicle.socIndex = 11;

    return true;
  }

  if (strcmp(vehicleName, "007") == 0)
  {
    vehicle.name = "007";

    vehicle.dacInit = 50;

    vehicle.dacMin = 59;

    vehicle.dacMax = 73;

    vehicle.socIndex = 11;

    return true;
  }

  return false;
}

void MoveBMSGauge::begin(
  const char* vehicleName,
  const char* bmsAddr
)
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();

  Serial.println("Move BMS Gauge");

  addr = bmsAddr;

  if (!loadVehicleConfig(vehicleName))
  {
    Serial.println("ERROR: Unknown vehicle");

    while (1)
    {
      delay(1000);
    }
  }

  Serial.print("Vehicle: ");
  Serial.println(vehicle.name);

  Serial.print("BMS Address: ");
  Serial.println(addr);

  pinMode(DAC_PIN, OUTPUT);

  dacWrite(DAC_PIN, vehicle.dacInit);

  BLEDevice::init("");

  connectBMS();
}

bool MoveBMSGauge::connectBMS()
{
  Serial.print("Connecting to BMS: ");

  Serial.println(addr);

  BLEAddress bmsAddr(addr);

  pClient = BLEDevice::createClient();

  if (!pClient->connect(bmsAddr))
  {
    Serial.println("Connect fail");

    return false;
  }

  Serial.println("Connected");

  BLERemoteService* service =
    pClient->getService(serviceUUID);

  if (service == nullptr)
  {
    Serial.println("No FFE0 service");

    pClient->disconnect();

    return false;
  }

  pChar =
    service->getCharacteristic(charUUID);

  if (pChar == nullptr)
  {
    Serial.println("No FFE1 characteristic");

    pClient->disconnect();

    return false;
  }

  pChar->registerForNotify(notifyCallback);

  delay(500);

  Serial.println("Send login command");

  pChar->writeValue(
    loginCmd,
    sizeof(loginCmd),
    true
  );

  Serial.println("Ready");

  lastPacketTime = millis();

  return true;
}

void MoveBMSGauge::notifyCallback(
  BLERemoteCharacteristic* c,
  uint8_t* data,
  size_t len,
  bool isNotify
)
{
  if (instance == nullptr) return;

  if (len <= instance->vehicle.socIndex) return;

  // kiểm tra packet đầu
  if (data[0] != 0xA5) return;

  // kiểm tra packet cuối
  if (data[len - 1] != 0x0D) return;

  uint8_t socRaw =
    data[instance->vehicle.socIndex];

  if (socRaw > 100) return;

  // lọc SOC chống nhảy
  if (instance->socFiltered < 0)
  {
    instance->socFiltered = socRaw;
  }
  else
  {
    instance->socFiltered =
      instance->socFiltered * 0.90 +
      socRaw * 0.10;
  }

  instance->lastPacketTime = millis();

  instance->outputDAC(
    (uint8_t)instance->socFiltered
  );
}

void MoveBMSGauge::outputDAC(uint8_t soc)
{
  soc = constrain(soc, 0, 100);

  int dacValue = map(
    soc,
    0,
    100,
    vehicle.dacMin,
    vehicle.dacMax
  );

  dacWrite(DAC_PIN, dacValue);

  Serial.print("Vehicle=");
  Serial.print(vehicle.name);

  Serial.print("  SOC=");
  Serial.print(soc);

  Serial.print("%  DAC=");
  Serial.print(dacValue);

  Serial.print("  V=");
  Serial.println(
    dacValue * 3.3 / 255.0,
    2
  );
}

void MoveBMSGauge::loop()
{
  // mất BLE
  if (pClient && !pClient->isConnected())
  {
    Serial.println("Disconnected");

    // fail-safe
    dacWrite(DAC_PIN, vehicle.dacMin);

    delay(2000);

    connectBMS();
  }

  // timeout không có packet
  if (
    lastPacketTime > 0 &&
    millis() - lastPacketTime > 5000
  )
  {
    Serial.println("No BMS data timeout");

    // fail-safe
    dacWrite(DAC_PIN, vehicle.dacMin);

    lastPacketTime = 0;
  }

  delay(1000);
}
