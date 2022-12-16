#pragma once

#include <cinttypes>

namespace hardware
{
    class battery
    {
    public:
        static battery &get() { return s_instance; };

        ~battery();

        battery(const battery &) = delete;
        battery(battery &&) = delete;
        battery &operator=(const battery &) = delete;
        battery &operator=(battery &&) = delete;

        uint32_t voltage_level();

    private:
        static battery s_instance;

        battery();
    };
}