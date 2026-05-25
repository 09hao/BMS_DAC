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
    lastPacketTime(0),
    cellMinMV(0),
    cellMaxMV(0),
    cellDeltaMV(0),
    detectedCellStart(0),
    detectedCellCount(0),
    cellWarn(false),
    cellFault(false)
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
  Serial.println("Move BMS Gauge + Auto Cell Count");

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

void MoveBMSGauge::printRawPacket(uint8_t* data, size_t len)
{
#if DEBUG_RAW_PACKET
  Serial.print("LEN=");
  Serial.print(len);
  Serial.print(" DATA=");

  for (size_t i = 0; i < len; i++)
  {
    Serial.printf("%02X ", data[i]);
  }

  Serial.println();
#endif
}

bool MoveBMSGauge::autoDetectCells(uint8_t* data, size_t len)
{
  uint8_t bestStart = 0;
  uint8_t bestCount = 0;
  uint16_t bestDelta = 65535;

  for (uint8_t start = 0; start + 1 < len; start++)
  {
    uint8_t count = 0;
    uint16_t minMV = 5000;
    uint16_t maxMV = 0;

    for (uint8_t i = 0; i < MAX_CELL_COUNT; i++)
    {
      uint16_t index = start + i * 2;

      if (index + 1 >= len)
      {
        break;
      }

      // BMS gửi little-endian: D5 0C = 0x0CD5 = 3285mV
      uint16_t mv =
        ((uint16_t)data[index + 1] << 8) |
        data[index];

      if (
        mv >= CELL_MIN_VALID_MV &&
        mv <= CELL_MAX_VALID_MV
      )
      {
        count++;

        if (mv < minMV)
        {
          minMV = mv;
        }

        if (mv > maxMV)
        {
          maxMV = mv;
        }
      }
      else
      {
        break;
      }
    }

    if (count >= MIN_CELL_COUNT)
    {
      uint16_t delta = maxMV - minMV;

      if (
        count > bestCount ||
        (count == bestCount && delta < bestDelta)
      )
      {
        bestStart = start;
        bestCount = count;
        bestDelta = delta;
      }
    }
  }

  if (bestCount < MIN_CELL_COUNT)
  {
    detectedCellStart = 0;
    detectedCellCount = 0;

    return false;
  }

  detectedCellStart = bestStart;
  detectedCellCount = bestCount;

  return true;
}

bool MoveBMSGauge::readDetectedCells(uint8_t* data, size_t len)
{
  if (detectedCellCount == 0)
  {
    return false;
  }

  if (
    detectedCellStart + detectedCellCount * 2 > len
  )
  {
    return false;
  }

  cellMinMV = 5000;
  cellMaxMV = 0;

  for (uint8_t i = 0; i < detectedCellCount; i++)
  {
    uint16_t index =
      detectedCellStart + i * 2;

    uint16_t mv =
      ((uint16_t)data[index + 1] << 8) |
      data[index];

    cellMV[i] = mv;

    if (mv < cellMinMV)
    {
      cellMinMV = mv;
    }

    if (mv > cellMaxMV)
    {
      cellMaxMV = mv;
    }
  }

  cellDeltaMV = cellMaxMV - cellMinMV;

  return true;
}

void MoveBMSGauge::checkCellFault()
{
  cellWarn = false;
  cellFault = false;

  if (cellDeltaMV >= CELL_WARN_DELTA_MV)
  {
    cellWarn = true;
  }

  if (cellDeltaMV >= CELL_FAULT_DELTA_MV)
  {
    cellFault = true;
  }

  if (cellMinMV <= CELL_LOW_FAULT_MV)
  {
    cellFault = true;
  }
}

void MoveBMSGauge::printCellInfo()
{
  Serial.println("====== CELL INFO ======");

  Serial.print("Detected Cell Start = ");
  Serial.println(detectedCellStart);

  Serial.print("Detected Cell Count = ");
  Serial.println(detectedCellCount);

  for (uint8_t i = 0; i < detectedCellCount; i++)
  {
    Serial.print("Cell ");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.print(cellMV[i]);
    Serial.println(" mV");
  }

  Serial.print("Cell Min = ");
  Serial.print(cellMinMV);
  Serial.println(" mV");

  Serial.print("Cell Max = ");
  Serial.print(cellMaxMV);
  Serial.println(" mV");

  Serial.print("Cell Delta = ");
  Serial.print(cellDeltaMV);
  Serial.println(" mV");

  if (cellFault)
  {
    Serial.println("PIN LOI: LECH CELL / CELL QUA THAP");
  }
  else if (cellWarn)
  {
    Serial.println("CANH BAO: LECH CELL");
  }

  Serial.println("=======================");
}

void MoveBMSGauge::notifyCallback(
  BLERemoteCharacteristic* c,
  uint8_t* data,
  size_t len,
  bool isNotify
)
{
  if (instance == nullptr)
  {
    return;
  }

  instance->printRawPacket(data, len);

  if (len <= instance->vehicle.socIndex)
  {
    return;
  }

  if (data[0] != 0xA5)
  {
    return;
  }

  if (data[len - 1] != 0x0D)
  {
    return;
  }

  uint8_t socRaw =
    data[instance->vehicle.socIndex];

  if (socRaw > 100)
  {
    return;
  }

  bool hasCells =
    instance->autoDetectCells(data, len);

  if (hasCells)
  {
    hasCells =
      instance->readDetectedCells(data, len);
  }

  if (hasCells)
  {
    instance->checkCellFault();
    instance->printCellInfo();

    if (instance->cellFault)
    {
      instance->lastPacketTime = millis();

      instance->outputCellFault();

      return;
    }
  }
  else
  {
    Serial.println("No valid cell data in this packet");
  }

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

  uint8_t socDisplay =
    (uint8_t)(instance->socFiltered + 0.5);

  if (
    instance->cellWarn &&
    socDisplay > 30
  )
  {
    socDisplay = 30;
  }

  instance->lastPacketTime = millis();

  instance->outputDAC(socDisplay);
}

void MoveBMSGauge::outputDAC(uint8_t soc)
{
  soc = constrain(soc, 0, 100);

  int dacValue =
    map(
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

void MoveBMSGauge::outputCellFault()
{
  dacWrite(DAC_PIN, vehicle.dacMin);

  Serial.print("PIN LOI - DAC MIN=");
  Serial.print(vehicle.dacMin);

  Serial.print("  Delta=");
  Serial.print(cellDeltaMV);

  Serial.print("mV  CellMin=");
  Serial.print(cellMinMV);

  Serial.print("mV  CellCount=");
  Serial.println(detectedCellCount);
}

void MoveBMSGauge::loop()
{
  if (pClient && !pClient->isConnected())
  {
    Serial.println("Disconnected");

    dacWrite(DAC_PIN, vehicle.dacMin);

    delay(2000);

    connectBMS();
  }

  if (
    lastPacketTime > 0 &&
    millis() - lastPacketTime > 5000
  )
  {
    Serial.println("No BMS data timeout");

    dacWrite(DAC_PIN, vehicle.dacMin);

    lastPacketTime = 0;
  }

  delay(1000);
}
