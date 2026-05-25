#include <MoveBMSGauge.h>

MoveBMSGauge gauge;

#define VEHICLE_NAME "007"
#define BMS_ADDR     "55:c3:63:00:bf:04"
// Chỉ cần sửa 2 dòng này
//#define VEHICLE_NAME "ISB"
//#define BMS_ADDR     "7a:b4:9d:68:07:f4"

void setup()
{
  gauge.begin(VEHICLE_NAME, BMS_ADDR);
}

void loop()
{
  gauge.loop();
}
