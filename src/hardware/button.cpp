#include "button.h"

#include <Arduino.h>

namespace hardware
{
    std::vector<button> button::s_buttons;
    std::vector<button::key_event> button::s_key_events;

    void button::add(uint8_t pin, uint32_t id)
    {
        for (auto &btn : s_buttons)
            if (btn.m_pin == pin)
            {
                btn.m_id = id;

                return;
            }

        s_buttons.push_back({pin, id});
    }

    void button::remove(uint8_t pin)
    {
        auto predicate = [&pin](const button &btn)
        {
            return btn.m_pin == pin;
        };

        s_buttons.erase(std::remove_if(s_buttons.begin(), s_buttons.end(), predicate), s_buttons.end());
    }

    void button::tick()
    {
        for (auto &btn : s_buttons)
            if (btn.m_last_state == digitalRead(btn.m_pin))
            {
                btn.m_last_state = !btn.m_last_state;

                s_key_events.push_back({btn.m_id, btn.m_last_state, millis()});
            }
    }

    const button::key_event button::get_data()
    {
        button::tick();

        key_event data{.id = 0, .state = false};

        if (s_key_events.size() && (millis() - s_key_events.front().timestamp > 100))
        {
            auto same_state_it = s_key_events.begin() + 1;
            data = s_key_events.front();

            while (same_state_it != s_key_events.end() && same_state_it->state == data.state)
                data.id |= same_state_it++->id;

            s_key_events.erase(s_key_events.begin(), same_state_it);
        }

        return std::move(data);
    }

    button::button(uint8_t pin, uint32_t id) : m_pin(pin),
                                               m_id(id),
                                               m_last_state(false)
    {
        pinMode(m_pin, INPUT_PULLUP);
    }

    button::~button()
    {
        pinMode(m_pin, INPUT);
    }
}