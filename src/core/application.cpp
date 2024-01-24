#include "application.h"

#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#include "hardware/config.h"
#include "hardware/storage.h"
#include "hardware/button.h"
#include "hardware/display.h"

application *application::s_instance = nullptr;

application::application()
{
    assert(s_instance == nullptr);

    s_instance = this;

    hardware::storage::mount(hardware::storage::type::internal, LV_FS_POSIX_PATH);

    hardware::button::add(PIN_BUTTON_1, 0b00000001);
    hardware::button::add(PIN_BUTTON_2, 0b00000010);

    lv_init();

    auto tick_get_cb = []() -> uint32_t
    {
        return esp_timer_get_time() / 1000ULL;
    };

    lv_tick_set_cb(tick_get_cb);

    auto &display = hardware::display::get();

    mp_draw_buf_1 = heap_caps_malloc((LV_COLOR_DEPTH / 8) * display.width() * 16, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    mp_draw_buf_2 = heap_caps_malloc((LV_COLOR_DEPTH / 8) * display.width() * 16, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    mp_display = lv_display_create(display.width(), display.height());

    lv_display_set_buffers(mp_display, mp_draw_buf_1, mp_draw_buf_2, (LV_COLOR_DEPTH / 8) * display.width() * 16, LV_DISPLAY_RENDER_MODE_PARTIAL);

    auto on_transfer_done = [](void *user_data)
    {
        lv_display_flush_ready(static_cast<lv_display_t *>(user_data));
    };

    display.set_transfer_done_callback(on_transfer_done, mp_display);

    auto flush_cb = [](lv_display_t *disp, const lv_area_t *area, uint8_t *pix_map)
    {
        lv_draw_sw_rgb565_swap(pix_map, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));

        auto display = static_cast<hardware::display *>(lv_display_get_user_data(disp));

        display->set_bitmap(area->x1, area->x2, area->y1, area->y2, (uint16_t *)pix_map);
    };

    lv_display_set_flush_cb(mp_display, flush_cb);
    lv_display_set_user_data(mp_display, &display);

    mp_indev = lv_indev_create();

    auto read_cb = [](lv_indev_t *indev, lv_indev_data_t *data)
    {
        auto event = hardware::button::get_data();

        switch (event.id)
        {
        case 0b00000001:
            data->key = LV_KEY_UP;
            break;

        case 0b00000010:
            data->key = LV_KEY_DOWN;
            break;

        case 0b00000011:
            data->key = LV_KEY_ENTER;
            break;

        default:
            break;
        }

        data->state = event.state ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    };

    lv_indev_set_type(mp_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(mp_indev, read_cb);

    auto on_create = [](void *userdata)
    {
        static_cast<application *>(userdata)->on_create();
    };

    lv_async_call(on_create, this);

    auto on_update = [](lv_timer_t *timer)
    {
        static const auto app = static_cast<application *>(timer->user_data);

        auto now = esp_timer_get_time();
        auto timestep = now - app->m_previous_timestamp;
        app->m_previous_timestamp = now;

        if (timestep < 0.0f)
            timestep = 0.0f;

        app->on_update(timestep / 1000000.0f);
    };

    lv_timer_create(on_update, 33, this);

    display.set_backlight(hardware::display::brightness_level::max);
}

application::~application()
{
    lv_indev_delete(mp_indev);
    lv_display_delete(mp_display);
    free(mp_draw_buf_2);
    free(mp_draw_buf_1);

    lv_deinit();

    hardware::storage::unmount(hardware::storage::type::internal);
}

application *create_application();

void setup()
{
    // wait for initialization to finish.
    // this is required when running on core 1.
    vTaskDelay(pdMS_TO_TICKS(200));

    create_application();
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(lv_timer_handler()));
}