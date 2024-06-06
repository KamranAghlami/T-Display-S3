#include "core/application.h"

#include <vector>
#include <cmath>

#include "hardware/display.h"
#include "hardware/battery.h"

constexpr size_t initial_balls = 25;

struct ball
{
    lv_obj_t *obj_handle;

    struct
    {
        float x;
        float y;
    } position;

    struct
    {
        float x;
        float y;
    } velocity;
};

class sandbox : public application
{
public:
    sandbox() : m_width(hardware::display::get().width()),
                m_height(hardware::display::get().height()),
                m_group(lv_group_create()),
                m_screen(lv_scr_act())
    {
        lv_indev_t *indev = nullptr;

        while ((indev = lv_indev_get_next(indev)))
            if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD)
                lv_indev_set_group(indev, m_group);

        lv_group_add_obj(m_group, m_screen);

        auto on_key = [](lv_event_t *e)
        {
            auto app = static_cast<sandbox *>(lv_event_get_user_data(e));
            auto key = lv_event_get_key(e);

            switch (key)
            {
            case LV_KEY_UP:
                app->add_ball();
                break;
            case LV_KEY_DOWN:
                app->remove_ball();
                break;
            case LV_KEY_ENTER:
                app->reset_balls();
                break;
            default:
                break;
            }
        };

        lv_obj_add_event_cb(m_screen, on_key, LV_EVENT_KEY, this);
    }

    ~sandbox()
    {
        while (m_balls.size())
            remove_ball();

        lv_group_del(m_group);
    }

private:
    void on_create() override
    {
        lv_obj_clear_flag(m_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(m_screen, lv_color_black(), LV_STATE_DEFAULT);

        m_battery_voltage = lv_label_create(lv_layer_top());

        lv_obj_set_style_text_color(m_battery_voltage, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_align(m_battery_voltage, LV_ALIGN_BOTTOM_LEFT, 4, -22);

        m_ball_count = lv_label_create(lv_layer_top());

        lv_obj_set_style_text_color(m_ball_count, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_align(m_ball_count, LV_ALIGN_BOTTOM_LEFT, 4, -4);

        m_balls.reserve(initial_balls);

        reset_balls();

        for (size_t i = 0; i < 10; i++)
            m_voltage_level = 0.9f * m_voltage_level + 0.1f * hardware::battery::get().voltage_level();

        auto hud_update = [](lv_timer_t *timer)
        {
            static_cast<sandbox *>(timer->user_data)->update_hud();
        };

        lv_timer_create(hud_update, 200, this);
    }

    void on_update(float timestep) override
    {
        const auto max_x = m_width - 30;
        const auto max_y = m_height - 30;

        for (const auto ball : m_balls)
        {
            const auto flip_vx = (ball->position.x < 0 && ball->velocity.x < 0) || (ball->position.x > max_x && ball->velocity.x > 0);
            const auto flip_vy = (ball->position.y < 0 && ball->velocity.y < 0) || (ball->position.y > max_y && ball->velocity.y > 0);

            ball->velocity.x = flip_vx ? -ball->velocity.x : ball->velocity.x;
            ball->velocity.y = flip_vy ? -ball->velocity.y : ball->velocity.y;

            for (const auto b : m_balls)
            {
                if (b == ball)
                    continue;

                const auto dx = ball->position.x - b->position.x;
                const auto dy = ball->position.y - b->position.y;

                if ((dx * dx + dy * dy) <= 1024)
                    resolve_collision(*b, *ball);
            }

            ball->position.x += ball->velocity.x * timestep;
            ball->position.y += ball->velocity.y * timestep;

            lv_obj_set_pos(ball->obj_handle, ball->position.x, ball->position.y);
        }
    }

    void add_ball()
    {
        auto b = static_cast<ball *>(lv_malloc(sizeof(ball)));

        b->obj_handle = lv_image_create(m_screen);

        b->position.x = (m_width / 2) - 15;
        b->position.y = (m_height / 2) - 15;
        b->velocity.x = lv_rand(50, 150);
        b->velocity.y = lv_rand(50, 150);

        if (lv_rand(0, 1))
            b->velocity.x = -b->velocity.x;

        if (lv_rand(0, 1))
            b->velocity.y = -b->velocity.y;

        lv_obj_set_size(b->obj_handle, 30, 30);
        lv_obj_set_pos(b->obj_handle, b->position.x, b->position.y);

        lv_obj_set_style_radius(b->obj_handle, 15, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(b->obj_handle, 0, LV_STATE_DEFAULT);

        char path[] = "F:/balls/ball_0.png";

        path[14] = '0' + lv_rand(0, 7);

        lv_image_set_src(b->obj_handle, path);

        m_balls.push_back(b);
    }

    void remove_ball()
    {
        if (!m_balls.size())
            return;

        auto b = m_balls.back();

        m_balls.pop_back();

        lv_obj_del(b->obj_handle);
        lv_free(b);
    }

    void reset_balls()
    {
        if (m_timer)
            return;

        auto timer_cb = [](lv_timer_t *timer)
        {
            auto app = static_cast<sandbox *>(timer->user_data);

            if (app->m_balls.size() == initial_balls)
            {
                lv_timer_del(app->m_timer);

                app->m_timer = nullptr;

                return;
            }

            if (app->m_balls.size() < initial_balls)
                app->add_ball();
            else
                app->remove_ball();
        };

        m_timer = lv_timer_create(timer_cb, 100, this);
    }

    void update_hud()
    {
        m_voltage_level = 0.9f * m_voltage_level + 0.1f * hardware::battery::get().voltage_level();

        lv_label_set_text_fmt(m_battery_voltage, "Battery: %lumv", m_voltage_level);
        lv_label_set_text_fmt(m_ball_count, "Balls: %zu", m_balls.size());
    }

    void resolve_collision(ball &b1, ball &b2)
    {
        const float dx = b2.position.x - b1.position.x;
        const float dy = b2.position.y - b1.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const float penetration_depth = (15 + 15) - distance;
        const float normal_x = dx / distance;
        const float normal_y = dy / distance;
        const float resolution_distance = penetration_depth / 2;

        b1.position.x -= normal_x * resolution_distance;
        b1.position.y -= normal_y * resolution_distance;
        b2.position.x += normal_x * resolution_distance;
        b2.position.y += normal_y * resolution_distance;

        const float relative_vx = b2.velocity.x - b1.velocity.x;
        const float relative_vy = b2.velocity.y - b1.velocity.y;
        const float v_along_normal = relative_vx * normal_x + relative_vy * normal_y;

        if (v_along_normal > 0)
            return;

        const float j = -(1 + 0.99f) * v_along_normal / 2;
        const float impulse_x = j * normal_x;
        const float impulse_y = j * normal_y;

        b1.velocity.x -= impulse_x;
        b1.velocity.y -= impulse_y;
        b2.velocity.x += impulse_x;
        b2.velocity.y += impulse_y;
    }

    const uint16_t m_width;
    const uint16_t m_height;

    lv_group_t *m_group;
    lv_obj_t *m_screen;

    lv_obj_t *m_battery_voltage;
    lv_obj_t *m_ball_count;

    lv_timer_t *m_timer = nullptr;

    std::vector<ball *> m_balls;
    uint32_t m_voltage_level = 0;
};

application *create_application()
{
    return new sandbox();
}