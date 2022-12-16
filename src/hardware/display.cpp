#include "display.h"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <Arduino.h>

#include "config.h"

#define LCD_PIXELS_WIDTH 320
#define LCD_PIXELS_HEIGHT 170

namespace hardware
{
    display display::s_instance;

    display::display()
    {
        pinMode(PIN_LCD_POWER, OUTPUT);
        digitalWrite(PIN_LCD_POWER, HIGH);

        pinMode(PIN_LCD_RD, OUTPUT);
        digitalWrite(PIN_LCD_RD, HIGH);

        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = PIN_LCD_DC,
            .wr_gpio_num = PIN_LCD_WR,
            .clk_src = LCD_CLK_SRC_PLL160M,
            .data_gpio_nums =
                {
                    PIN_LCD_D0,
                    PIN_LCD_D1,
                    PIN_LCD_D2,
                    PIN_LCD_D3,
                    PIN_LCD_D4,
                    PIN_LCD_D5,
                    PIN_LCD_D6,
                    PIN_LCD_D7,
                },
            .bus_width = 8,
            .max_transfer_bytes = LCD_PIXELS_WIDTH * LCD_PIXELS_HEIGHT * sizeof(uint16_t),
        };

        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &m_bus_handle));

        auto on_transfer_done = [](esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
        {
            auto disp = static_cast<display *>(user_ctx);

            disp->m_on_transfer_done_callback(disp->m_on_transfer_done_user_data);

            return false;
        };

        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = PIN_LCD_CS,
            .pclk_hz = 10 * 1000 * 1000,
            .trans_queue_depth = 20,
            .on_color_trans_done = on_transfer_done,
            .user_ctx = this,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .dc_levels = {
                .dc_idle_level = 0,
                .dc_cmd_level = 0,
                .dc_dummy_level = 0,
                .dc_data_level = 1,
            },
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(m_bus_handle, &io_config, &m_io_handle));

        esp_lcd_panel_dev_config_t device_config = {
            .reset_gpio_num = PIN_LCD_RES,
            .color_space = ESP_LCD_COLOR_SPACE_RGB,
            .bits_per_pixel = 16,
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(m_io_handle, &device_config, &m_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(m_panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(m_panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(m_panel_handle, false, true));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(m_panel_handle, 0, 35));

        pinMode(PIN_LCD_BACKLIGHT, OUTPUT);
        set_backlight(brightness_level::min);
    }

    display::~display()
    {
        pinMode(PIN_LCD_BACKLIGHT, INPUT);

        ESP_ERROR_CHECK(esp_lcd_panel_del(m_panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_io_del(m_io_handle));
        ESP_ERROR_CHECK(esp_lcd_del_i80_bus(m_bus_handle));

        pinMode(PIN_LCD_RD, INPUT);
        pinMode(PIN_LCD_POWER, INPUT);
    }

    uint16_t display::width()
    {
        return LCD_PIXELS_WIDTH;
    }

    uint16_t display::height()
    {
        return LCD_PIXELS_HEIGHT;
    }

    void display::set_backlight(brightness_level level)
    {
        analogWrite(PIN_LCD_BACKLIGHT, static_cast<int>(level));
    }

    void display::set_transfer_done_callback(transfer_done_callback_t on_transfer_done, void *user_data)
    {
        m_on_transfer_done_callback = on_transfer_done;
        m_on_transfer_done_user_data = user_data;
    }

    void display::set_bitmap(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *data)
    {
        esp_lcd_panel_draw_bitmap(m_panel_handle, x1, y1, x2 + 1, y2 + 1, data);
    }
}
