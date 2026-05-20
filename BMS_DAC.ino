#include "MoveBMSGauge.h"

BLEAddress addr("7a:b4:9d:68:07:f4");//THAY ĐỔI ĐỊA CHỈ VỚI MỖI CỤC PIN KHÁC NHAU

MoveBMSGauge gauge(addr);

void setup()
{
  Serial.begin(115200);
  gauge.begin();
}

void loop()
{
  gauge.loop();
}
