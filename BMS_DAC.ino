#include "MoveBMSGauge.h"

MoveBMSGauge gauge;

void setup()
{
  gauge.begin(
    //"007",
    //"55:c3:63:00:bf:04"
    "ISB",
    "7a:b4:9d:68:07:f4"
  );
}

void loop()
{
  gauge.loop();
}
