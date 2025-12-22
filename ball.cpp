#include "ball.h"
#include "assets.h"
#include "level.h"
#include "paddle.h"
#include "game.h"
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
    Vector2 next_ball_pos = {
        ball_pos.x + ball_vel.x,
        ball_pos.y + ball_vel.y
    };


    Vector2 logical_pos = next_ball_pos;
    logical_pos.y -= levelYOffset;


    if (next_ball_pos.x < 0 || next_ball_pos.x > current_level.columns - 1) {
        ball_vel.x *= -1;
        next_ball_pos.x = ball_pos.x;
    }
    if (next_ball_pos.y < 0) {
        ball_vel.y *= -1;
        next_ball_pos.y = ball_pos.y;
    }


    if (is_colliding_with_level_cell(logical_pos, ball_size, WALL)) {
        if (is_colliding_with_level_cell({logical_pos.x, ball_pos.y - levelYOffset}, ball_size, WALL)) ball_vel.x *= -1;
        if (is_colliding_with_level_cell({ball_pos.x, logical_pos.y}, ball_size, WALL)) ball_vel.y *= -1;
    }
    else if (is_colliding_with_level_cell(logical_pos, ball_size, BLOCKS)) {
        char& cell = get_colliding_level_cell(logical_pos, ball_size, BLOCKS);

        if (is_colliding_with_level_cell({logical_pos.x, ball_pos.y - levelYOffset}, ball_size, BLOCKS)) ball_vel.x *= -1;
        if (is_colliding_with_level_cell({ball_pos.x, logical_pos.y}, ball_size, BLOCKS)) ball_vel.y *= -1;

        cell = VOID;
        --current_level_blocks;

        if (GetRandomValue(1, 5) == 1) {
            for (int i = 0; i < 10; i++) {
                if (!power_ups[i].active) {
                    power_ups[i].active = true;
                    power_ups[i].position = next_ball_pos;
                    power_ups[i].type = (PowerUpType)GetRandomValue(0, 1);
                    break;
                }
            }
        }
    }
    else if (is_colliding_with_paddle(next_ball_pos, ball_size)) {
        ball_vel.y = -std::abs(ball_vel.y);
    }

    ball_pos = next_ball_pos;
}
bool is_ball_inside_level()
{
    return is_inside_level(static_cast<int>(ball_pos.y), static_cast<int>(ball_pos.x));
}