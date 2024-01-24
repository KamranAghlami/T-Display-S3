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

    auto &display = hardware::display::get();

    lv_init();

    void *buffer1 = heap_caps_malloc(sizeof(lv_color_t) * display.width() * 16, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buffer2 = heap_caps_malloc(sizeof(lv_color_t) * display.width() * 16, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    lv_disp_drv_init(&m_disp_drv);
    lv_disp_draw_buf_init(&m_draw_buf, buffer1, buffer2, display.width() * 16);

    auto flush_cb = [](lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
    {
        auto display = static_cast<hardware::display *>(disp->user_data);

        display->set_bitmap(area->x1, area->x2, area->y1, area->y2, (uint16_t *)&color_p->full);
    };

    m_disp_drv.hor_res = display.width();
    m_disp_drv.ver_res = display.height();
    m_disp_drv.flush_cb = flush_cb;
    m_disp_drv.draw_buf = &m_draw_buf;
    m_disp_drv.user_data = &display;

    lv_disp_drv_register(&m_disp_drv);

    auto on_transfer_done = [](void *user_data)
    {
        lv_disp_flush_ready(static_cast<lv_disp_drv_t *>(user_data));
    };

    display.set_transfer_done_callback(on_transfer_done, &m_disp_drv);

    lv_indev_drv_init(&m_indev_drv);

    auto read_cb = [](lv_indev_drv_t *drv, lv_indev_data_t *data)
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

    m_indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    m_indev_drv.read_cb = read_cb;

    lv_indev_drv_register(&m_indev_drv);

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
    free(m_disp_drv.draw_buf->buf1);
    free(m_disp_drv.draw_buf->buf2);

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