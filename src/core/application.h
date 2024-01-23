#pragma once

#include <cstdint>

#include <lvgl.h>

class application
{
public:
    application();
    virtual ~application();

    application(const application &) = delete;
    application(application &&) = delete;
    application &operator=(const application &) = delete;
    application &operator=(application &&) = delete;

protected:
    virtual void on_create() = 0;
    virtual void on_update(float timestep) = 0;

private:
    static application *s_instance;

    void *mp_draw_buf_1;
    void *mp_draw_buf_2;
    lv_display_t *mp_display;
    // lv_indev_drv_t m_indev_drv;

    int64_t m_previous_timestamp = 0;
};