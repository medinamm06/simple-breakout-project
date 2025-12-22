#include "ball.h"
#include "assets.h"
#include "level.h"
#include "paddle.h"
#include "game.h" // Добавили, чтобы мяч "знал" о бонусах

#include "raylib.h"

#include <cmath>
#include <numbers>
extern float levelYOffset;

void spawn_ball()
{
    for (int column = 0; column < current_level.columns; column++) {
        for (int row = 0; row < current_level.rows; row++) {
            if (get_level_cell(row, column) == BALL) {
                set_level_cell(row, column, VOID);
                ball_pos = { static_cast<float>(column), static_cast<float>(row) };
                constexpr float ball_launch_angle_radians = ball_launch_angle_degrees * (std::numbers::pi_v<float> / 180.0f);
                ball_vel.y = -ball_launch_vel_mag * std::sin(ball_launch_angle_radians);
                ball_vel.x = (rand() % 2 == 0) ? ball_launch_vel_mag * std::cos(ball_launch_angle_radians) : -ball_launch_vel_mag * std::cos(ball_launch_angle_radians);
                goto outer_loop_end;
            }
        }
    }
outer_loop_end:;
}

void move_ball()
{
    // 1. Рассчитываем, куда мяч ХОЧЕТ наступить (в мировых координатах)
    Vector2 next_ball_pos = {
        ball_pos.x + ball_vel.x,
        ball_pos.y + ball_vel.y
    };

    // 2. СОЗДАЕМ ЛОГИЧЕСКУЮ ПОЗИЦИЮ
    // Это позиция мяча относительно движущейся сетки уровня
    Vector2 logical_pos = next_ball_pos;
    logical_pos.y -= levelYOffset; // Вычитаем смещение уровня

    // --- ПРОВЕРКА ГРАНИЦ ЭКРАНА (Чтобы не улетал в дыры) ---
    if (next_ball_pos.x < 0 || next_ball_pos.x > current_level.columns - 1) {
        ball_vel.x *= -1;
        next_ball_pos.x = ball_pos.x; // Откат
    }
    if (next_ball_pos.y < 0) { // Потолок
        ball_vel.y *= -1;
        next_ball_pos.y = ball_pos.y;
    }

    // --- СТОЛКНОВЕНИЕ СО СТЕНАМИ И БЛОКАМИ (Через логическую позицию) ---

    // Сначала проверяем стены (WALL)
    if (is_colliding_with_level_cell(logical_pos, ball_size, WALL)) {
        // Если ударились в стену, которая уехала вниз
        if (is_colliding_with_level_cell({logical_pos.x, ball_pos.y - levelYOffset}, ball_size, WALL)) ball_vel.x *= -1;
        if (is_colliding_with_level_cell({ball_pos.x, logical_pos.y}, ball_size, WALL)) ball_vel.y *= -1;
    }
    // Затем проверяем кирпичи (BLOCKS)
    else if (is_colliding_with_level_cell(logical_pos, ball_size, BLOCKS)) {
        char& cell = get_colliding_level_cell(logical_pos, ball_size, BLOCKS);

        // Определяем сторону удара
        if (is_colliding_with_level_cell({logical_pos.x, ball_pos.y - levelYOffset}, ball_size, BLOCKS)) ball_vel.x *= -1;
        if (is_colliding_with_level_cell({ball_pos.x, logical_pos.y}, ball_size, BLOCKS)) ball_vel.y *= -1;

        // Разрушаем блок
        cell = VOID;
        --current_level_blocks;

        // Шанс бонуса (позиция бонуса тоже должна учитывать смещение!)
        if (GetRandomValue(1, 5) == 1) {
            for (int i = 0; i < 10; i++) {
                if (!power_ups[i].active) {
                    power_ups[i].active = true;
                    power_ups[i].position = next_ball_pos; // Спавним в мировых координатах
                    power_ups[i].type = (PowerUpType)GetRandomValue(0, 1);
                    break;
                }
            }
        }
        // PlaySound(hit_sound);
    }
    // Столкновение с ракеткой (она не движется по Y, проверяем обычные координаты)
    else if (is_colliding_with_paddle(next_ball_pos, ball_size)) {
        ball_vel.y = -std::abs(ball_vel.y);
    }

    // Обновляем реальную позицию мяча
    ball_pos = next_ball_pos;
}
bool is_ball_inside_level()
{
    return is_inside_level(static_cast<int>(ball_pos.y), static_cast<int>(ball_pos.x));
}