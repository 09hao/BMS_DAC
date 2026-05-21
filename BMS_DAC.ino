#include <MoveBMSGauge.h>

MoveBMSGauge gauge;

// Chỉ cần sửa 2 dòng này
#define VEHICLE_NAME "ISB"
#define BMS_ADDR     "7a:b4:9d:68:07:f4"

void setup()
{
  gauge.begin(VEHICLE_NAME, BMS_ADDR);
}

void loop()
{
  gauge.loop();
}
