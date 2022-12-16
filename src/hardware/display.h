#pragma once

#include <cstdint>

#include <esp_lcd_panel_io.h>

namespace hardware
{
    class display
    {
    public:
        using transfer_done_callback_t = void (*)(void *);

        enum class brightness_level
        {
            min = 0,
            max = 0xff,
        };

        static display &get() { return s_instance; };

        ~display();

        display(const display &) = delete;
        display(display &&) = delete;
        display &operator=(const display &) = delete;
        display &operator=(display &&) = delete;

        uint16_t width();
        uint16_t height();

        void set_backlight(brightness_level level);
        void set_transfer_done_callback(transfer_done_callback_t on_transfer_done, void *user_data);
        void set_bitmap(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *data);

    private:
        static display s_instance;

        display();

        esp_lcd_i80_bus_handle_t m_bus_handle = nullptr;
        esp_lcd_panel_io_handle_t m_io_handle = nullptr;
        esp_lcd_panel_handle_t m_panel_handle = nullptr;

        transfer_done_callback_t m_on_transfer_done_callback = nullptr;
        void *m_on_transfer_done_user_data = nullptr;
    };
}