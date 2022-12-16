#pragma once

#include <cstdint>
#include <vector>

namespace hardware
{
    class button
    {
    public:
        struct key_event
        {
            uint32_t id;
            bool state;
            unsigned long timestamp;
        };

        static void add(uint8_t pin, uint32_t id);
        static void remove(uint8_t pin);
        static const key_event get_data();

        ~button();

    private:
        static std::vector<button> s_buttons;
        static std::vector<key_event> s_key_events;

        static void tick();

        button(uint8_t pin, uint32_t id);

        uint8_t m_pin;
        uint32_t m_id;

        bool m_last_state;
    };
}