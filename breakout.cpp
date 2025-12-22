#include "assets.h"
#include "ball.h"
#include "game.h"
#include "graphics.h"
#include "level.h"
#include "paddle.h"
#include "raylib.h"
#include "rlgl.h"

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float size;
    bool active;
};

const int MAX_PARTICLES = 800;
Particle particles[MAX_PARTICLES];
float fireworkTimer = 0.0f;
Texture2D backgroundTexture;
GameState currentState = MENU;
int menuSelection = 0;
float blockMoveTimer = 0.0f;
const float blockMoveInterval = 10.0f;
float invasionTimer = 0.0f;
float invasionSpeed = 5.0f;
float levelYOffset = 0.0f;

void update_powerups() {
    for (int i = 0; i < 10; i++) {
        if (power_ups[i].active) {
            power_ups[i].position.y += 0.05f;

            if (is_colliding_with_paddle(power_ups[i].position, ball_size)) {
                power_ups[i].active = false;


                if (power_ups[i].type == EXTEND_PADDLE) {
                    paddle_size.x += 0.5f;
                }
                if (power_ups[i].type == EXTRA_LIFE) {
                    lives++;
                }
            }

            if (power_ups[i].position.y > 25.0f) power_ups[i].active = false;
        }
    }
}

void draw_powerups() {
    for (int i = 0; i < 10; i++) {
        if (power_ups[i].active) {

            Vector2 screenPos = {
                power_ups[i].position.x * 40.0f + 20.0f,
                power_ups[i].position.y * 40.0f + 20.0f
            };

            Color mainColor = (power_ups[i].type == EXTEND_PADDLE) ? BLUE : GREEN;
            Color glowColor = Fade(WHITE, 0.5f);

            DrawCircleV(screenPos, 15, glowColor);

            DrawCircleV(screenPos, 12, mainColor);

            DrawCircle(screenPos.x - 4, screenPos.y - 4, 4, Fade(WHITE, 0.7f));

            if (power_ups[i].type == EXTEND_PADDLE) {
                DrawRectangle(screenPos.x - 6, screenPos.y - 1, 12, 2, WHITE);

                DrawTriangle(
                    {screenPos.x - 8, screenPos.y},
                    {screenPos.x - 4, screenPos.y - 3},
                    {screenPos.x - 4, screenPos.y + 3}, WHITE);
                DrawTriangle(
                    {screenPos.x + 8, screenPos.y},
                    {screenPos.x + 4, screenPos.y + 3},
                    {screenPos.x + 4, screenPos.y - 3}, WHITE);
            }
            else if (power_ups[i].type == EXTRA_LIFE) {
                DrawRectangle(screenPos.x - 6, screenPos.y - 2, 12, 4, WHITE); // Горизонтальная часть
                DrawRectangle(screenPos.x - 2, screenPos.y - 6, 4, 12, WHITE); // Вертикальная часть
            }
        }
    }
}
int pauseSelection = 0;


void InitFireworks() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
    fireworkTimer = 0.0f;
}

void SpawnFireworkBurst(Vector2 center) {
    Color colors[] = { GOLD, ORANGE, PINK, SKYBLUE, LIME, VIOLET };
    Color burstColor = colors[GetRandomValue(0, 5)];

    int particlesCreated = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].position = center;

            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float speed = (float)GetRandomValue(20, 70) / 10.0f;

            particles[i].velocity = { cosf(angle) * speed, sinf(angle) * speed };
            particles[i].color = burstColor;
            particles[i].alpha = 1.0f;
            particles[i].size = (float)GetRandomValue(2, 5);

            particlesCreated++;
            if (particlesCreated >= 100) break;
        }
    }
}

void UpdateFireworks() {
    fireworkTimer += GetFrameTime();
    if (fireworkTimer > 0.5f) {
        SpawnFireworkBurst({ (float)GetRandomValue(100, 1180), (float)GetRandomValue(100, 400) });
        fireworkTimer = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        particles[i].position.x += particles[i].velocity.x;
        particles[i].position.y += particles[i].velocity.y;

        particles[i].velocity.y += 0.05f;

        particles[i].alpha -= 0.015f;

        if (particles[i].alpha <= 0.0f) particles[i].active = false;
    }
}

void DrawFireworks() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        DrawCircleV(particles[i].position, particles[i].size, Fade(particles[i].color, particles[i].alpha));
    }
}

void update() {
switch (currentState) {
        case MENU:
            if (IsKeyPressed(KEY_DOWN)) menuSelection = (menuSelection + 1) % 3;
            if (IsKeyPressed(KEY_UP)) menuSelection = (menuSelection - 1 + 3) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (menuSelection == 0) {
                    load_level();
                    spawn_ball();
                    spawn_paddle();
                    paddle_size.x = 3.0f;
                    lives = 3;
                    currentState = PLAYING;
                }
                if (menuSelection == 1) currentState = INSTRUCTIONS;
                if (menuSelection == 2) CloseWindow();
            }
            break;

        case INSTRUCTIONS:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) currentState = MENU;
            break;
case PLAYING:
    invasionTimer += GetFrameTime();
    if (invasionTimer >= invasionSpeed) {
        levelYOffset += 0.5f;
        invasionTimer = 0.0f;

        rlTranslatef(0, (float)GetRandomValue(-5, 5), 0);

        if (invasionSpeed > 1.0f) invasionSpeed -= 0.1f;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentState = PAUSED;
        pauseSelection = 0;
    }

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move_paddle(-paddle_speed);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move_paddle(paddle_speed);


invasionTimer += GetFrameTime();
if (invasionTimer >= invasionSpeed) {
    levelYOffset += 0.5f;
    invasionTimer = 0.0f;

    for (int r = 0; r < current_level.rows; r++) {
        for (int c = 0; c < current_level.columns; c++) {
            if (get_level_cell(r, c) == BLOCKS) {
                if ((float)r + levelYOffset > 14.0f) {
                    set_level_cell(r, c, VOID);
                    current_level_blocks--;
                }
            }
        }
    }
}

    if (levelYOffset > 15.0f) {
        currentState = GAMEOVER;
    }

    move_ball();
    update_powerups();

    if (!is_ball_inside_level()) {
        lives--;
        PlaySound(lose_sound);
        if (lives <= 0) {
            currentState = GAMEOVER;
        } else {
            spawn_ball();
        }
    }
    else if (current_level_blocks == 0) {
        if (current_level_index >= 4) {
            InitFireworks();
            currentState = VICTORY;
        } else {
            levelYOffset = 0.0f;
            current_level_index++;
            load_level();
            spawn_ball();
            PlaySound(win_sound);
        }
    }
    break;


        case PAUSED:

            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)) {
                pauseSelection = (pauseSelection + 1) % 2;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (pauseSelection == 0) currentState = PLAYING;
                else currentState = MENU;
            }

            if (IsKeyPressed(KEY_ESCAPE)) currentState = PLAYING;
            break;

        case GAMEOVER:
        case VICTORY:
         UpdateFireworks();
            if (IsKeyPressed(KEY_ENTER)) currentState = MENU;
            break;
    };
    }


void draw() {
    Color bgColor = WHITE;

    if (levelYOffset > 10.0f) {
        float dangerLevel = (levelYOffset - 10.0f) / 5.0f;

        float speed = 2.0f + (dangerLevel * 10.0f);
        float pulse = (sinf(GetTime() * speed) + 1.0f) / 2.0f;

        unsigned char redTint = (unsigned char)(255 * dangerLevel * pulse);
        bgColor = { 255, (unsigned char)(255 - redTint), (unsigned char)(255 - redTint), 255 };
    }
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(
    backgroundTexture,
    { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height },
    { 0.0f, 0.0f, 1280.0f, 720.0f },
    { 0.0f, 0.0f }, 0.0f, bgColor
);


    switch (currentState) {
        case MENU:
            DrawText("SUPER BREAKOUT", 380, 150, 60, GOLD);
            DrawText("START GAME", 530, 350, 30, (menuSelection == 0) ? YELLOW : WHITE);
            DrawText("INSTRUCTIONS", 515, 410, 30, (menuSelection == 1) ? YELLOW : WHITE);
            DrawText("QUIT", 585, 470, 30, (menuSelection == 2) ? YELLOW : WHITE);
            break;

        case INSTRUCTIONS:
            DrawText("HOW TO PLAY", 500, 100, 40, WHITE);
            DrawText("- Move paddle with A/D or Arrows", 400, 250, 20, LIGHTGRAY);
            DrawText("- Destroy blocks to drop bonuses", 400, 300, 20, LIGHTGRAY);
            DrawText("- BLUE 'P' = Wider Paddle", 400, 350, 20, BLUE);
            DrawText("- GREEN 'P' = Extra Life", 400, 400, 20, GREEN);
            DrawText("Press ENTER to return", 520, 550, 20, YELLOW);
            break;

        case PLAYING:
        rlPushMatrix();
        rlTranslatef(0, levelYOffset * 40.0f, 0);
        draw_level();
        rlPopMatrix();

            draw_paddle();
            draw_ball();
            draw_powerups();
            draw_ui();
            DrawText(TextFormat("LIVES: %i", lives), 20, 20, 25, RED);
            break;

        case PAUSED:
            draw_level();
        rlPushMatrix();
        rlTranslatef(0, levelYOffset * 40.0f, 0);
        draw_level();
        rlPopMatrix();
            DrawRectangle(0, 0, 1280, 720, Fade(BLACK, 0.7f));
            DrawText("PAUSED", 550, 320, 50, PURPLE);
            DrawText("Press ESC to Resume", 520, 390, 20, LIGHTGRAY);
            break;

        case GAMEOVER:
            DrawRectangle(0, 0, 1280, 720, RED);
            DrawText("GAME OVER", 480, 300, 60, WHITE);
            DrawText("Press ENTER for Menu", 510, 400, 25, LIGHTGRAY);
            break;
    case VICTORY:
        DrawRectangle(0, 0, 1280, 720, Fade(BLACK, 0.85f));

        DrawFireworks();

        DrawText("VICTORY!", 395, 255, 100, Fade(BLACK, 0.5f));
        DrawText("YOU SAVED THE GALAXY!", 355, 385, 40, Fade(BLACK, 0.5f));

        DrawText("VICTORY!", 390, 250, 100, GOLD);
        DrawText("YOU SAVED THE GALAXY!", 350, 380, 40, SKYBLUE);

        if ((int)(GetTime() * 2) % 2 == 0) {
            DrawText("Press ENTER for Menu", 510, 500, 25, WHITE);
        }
        break;
    }

    EndDrawing();
}

int main() {
    InitWindow(1280, 720, "Breakout Ultimate");


    SetExitKey(0);

    InitAudioDevice();
    SetTargetFPS(60);

    load_fonts();
    load_textures();
    backgroundTexture = LoadTexture("data/images/background01.png");
    load_level();
    load_sounds();

    while (!WindowShouldClose()) {
        update();
        draw();
    }

    unload_sounds();
    unload_level();
    unload_textures();
    unload_fonts();
    UnloadTexture(backgroundTexture);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}