#include "battery.h"

#include <Arduino.h>

#include "config.h"

namespace hardware
{
    battery battery::s_instance;

    battery::battery()
    {
        pinMode(PIN_BATTERY, INPUT);
    }

    battery::~battery()
    {
    }

    uint32_t battery::voltage_level()
    {
        return analogReadMilliVolts(PIN_BATTERY) * 2;
    }
}
