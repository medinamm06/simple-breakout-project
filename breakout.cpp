#include "assets.h"
#include "ball.h"
#include "game.h"
#include "graphics.h"
#include "level.h"
#include "paddle.h"
#include "raylib.h"
#include "rlgl.h"
Texture2D backgroundTexture; // <- Добавь вот эту строку
GameState currentState = MENU;
int menuSelection = 0;
float blockMoveTimer = 0.0f;
const float blockMoveInterval = 10.0f;
float invasionTimer = 0.0f;
float invasionSpeed = 5.0f; // Каждые 5 секунд блоки опускаются ниже
float levelYOffset = 0.0f;  // На сколько единиц сетки опустился уровень

void update_powerups() {
    for (int i = 0; i < 10; i++) {
        if (power_ups[i].active) {
            power_ups[i].position.y += 0.05f;

            if (is_colliding_with_paddle(power_ups[i].position, ball_size)) {
                power_ups[i].active = false;
                // PlaySound(bonus_sound); // Можно раскомментировать, если есть звук

                if (power_ups[i].type == EXTEND_PADDLE) {
                    paddle_size.x += 0.5f; // Увеличиваем ширину ракетки
                }
                if (power_ups[i].type == EXTRA_LIFE) {
                    lives++; // Добавляем жизнь
                }
            }

            // Удаляем бонус, если он улетел за нижнюю границу
            if (power_ups[i].position.y > 25.0f) power_ups[i].active = false;
        }
    }
}

void draw_powerups() {
    for (int i = 0; i < 10; i++) {
        if (power_ups[i].active) {
            // 1. Вычисляем центр (добавляем +20, чтобы было в центре клетки 40x40)
            Vector2 screenPos = {
                power_ups[i].position.x * 40.0f + 20.0f,
                power_ups[i].position.y * 40.0f + 20.0f
            };

            Color mainColor = (power_ups[i].type == EXTEND_PADDLE) ? BLUE : GREEN;
            Color glowColor = Fade(WHITE, 0.5f); // Полупрозрачный белый для свечения

            // 2. Рисуем "свечение" (внешний круг побольше)
            DrawCircleV(screenPos, 15, glowColor);

            // 3. Рисуем основное тело бонуса
            DrawCircleV(screenPos, 12, mainColor);

            // 4. Рисуем "блик" (маленький светлый кружок сверху слева для объема)
            DrawCircle(screenPos.x - 4, screenPos.y - 4, 4, Fade(WHITE, 0.7f));

            // 5. Рисуем ИКОНКУ вместо текста
            if (power_ups[i].type == EXTEND_PADDLE) {
                // Рисуем символ расширения: <->
                // Горизонтальная палка
                DrawRectangle(screenPos.x - 6, screenPos.y - 1, 12, 2, WHITE);
                // Левая стрелка (треугольник)
                DrawTriangle(
                    {screenPos.x - 8, screenPos.y},
                    {screenPos.x - 4, screenPos.y - 3},
                    {screenPos.x - 4, screenPos.y + 3}, WHITE);
                // Правая стрелка (треугольник)
                DrawTriangle(
                    {screenPos.x + 8, screenPos.y},
                    {screenPos.x + 4, screenPos.y + 3},
                    {screenPos.x + 4, screenPos.y - 3}, WHITE);
            }
            else if (power_ups[i].type == EXTRA_LIFE) {
                // Рисуем плюсик: +
                DrawRectangle(screenPos.x - 6, screenPos.y - 2, 12, 4, WHITE); // Горизонтальная часть
                DrawRectangle(screenPos.x - 2, screenPos.y - 6, 4, 12, WHITE); // Вертикальная часть
            }
        }
    }
}
int pauseSelection = 0;
// --- ГЛАВНАЯ ЛОГИКА ОБНОВЛЕНИЯ ---
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

        // ЭФФЕКТ ТРЯСКИ (Визуально дергаем экран вверх-вниз при шаге блоков)
        rlTranslatef(0, (float)GetRandomValue(-5, 5), 0);

        if (invasionSpeed > 1.0f) invasionSpeed -= 0.1f;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        currentState = PAUSED;
        pauseSelection = 0;
    }

    // 1. Движение ракетки
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move_paddle(-paddle_speed);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move_paddle(paddle_speed);

    // 2. ЛОГИКА ВТОРЖЕНИЯ
invasionTimer += GetFrameTime();
if (invasionTimer >= invasionSpeed) {
    levelYOffset += 0.5f;
    invasionTimer = 0.0f;

    // --- ПРОВЕРКА УЛЕТЕВШИХ БЛОКОВ ---
    for (int r = 0; r < current_level.rows; r++) {
        for (int c = 0; c < current_level.columns; c++) {
            if (get_level_cell(r, c) == BLOCKS) {
                // Если блок ушел ниже 14-й строки (ниже ракетки)
                if ((float)r + levelYOffset > 14.0f) {
                    set_level_cell(r, c, VOID); // Считаем его уничтоженным
                    current_level_blocks--;     // Уменьшаем счетчик, чтобы уровень завершился
                }
            }
        }
    }
    // --------------------------------
}
    // 3. Проверка: не коснулись ли блоки ракетки?
    // Если блоки опустились ниже 18.0f (примерная высота ракетки), то конец
    if (levelYOffset > 15.0f) {
        currentState = GAMEOVER;
    }

    move_ball();
    update_powerups();

    // 4. Проверка проигрыша мяча
    if (!is_ball_inside_level()) {
        lives--;
        PlaySound(lose_sound);
        if (lives <= 0) {
            currentState = GAMEOVER;
        } else {
            spawn_ball();
        }
    }
    // 5. Проверка победы
    else if (current_level_blocks == 0) {
        if (current_level_index >= 2) {
            currentState = VICTORY;
        } else {
            levelYOffset = 0.0f; // Сброс смещения для нового уровня
            load_level(1);
            spawn_ball();
            PlaySound(win_sound);
        }
    }
    break;


        case PAUSED:
            // --- НОВАЯ ЛОГИКА ПАУЗЫ ---
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)) {
                pauseSelection = (pauseSelection + 1) % 2; // Переключаем 0 или 1
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (pauseSelection == 0) currentState = PLAYING; // Продолжить
                else currentState = MENU;                       // В меню
            }

            if (IsKeyPressed(KEY_ESCAPE)) currentState = PLAYING; // Быстрый выход из паузы
            break;

        case GAMEOVER:
        case VICTORY:
            if (IsKeyPressed(KEY_ENTER)) currentState = MENU;
            break;
    };
    }


// --- ГЛАВНАЯ ЛОГИКА ОТРИСОВКИ ---
void draw() {
    Color bgColor = WHITE;

    // Начинаем паниковать, когда блоки прошли больше 10 клеток (из 15 допустимых)
    if (levelYOffset > 10.0f) {
        // Вычисляем интенсивность (от 0.0 до 1.0)
        float dangerLevel = (levelYOffset - 10.0f) / 5.0f;

        // Частота пульсации (чем ближе блоки, тем быстрее пульс)
        float speed = 2.0f + (dangerLevel * 10.0f);
        float pulse = (sinf(GetTime() * speed) + 1.0f) / 2.0f; // Значение от 0 до 1

        // Смешиваем белый с красным в зависимости от пульса и уровня опасности
        unsigned char redTint = (unsigned char)(255 * dangerLevel * pulse);
        bgColor = { 255, (unsigned char)(255 - redTint), (unsigned char)(255 - redTint), 255 };
    }
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(
    backgroundTexture,
    { 0.0f, 0.0f, (float)backgroundTexture.width, (float)backgroundTexture.height },
    { 0.0f, 0.0f, 1280.0f, 720.0f },
    { 0.0f, 0.0f }, 0.0f, bgColor // Используем вычисленный цвет
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
        rlTranslatef(0, levelYOffset * 40.0f, 0); // Смещаем всю сетку блоков вниз
        draw_level();
        rlPopMatrix();

            draw_paddle();
            draw_ball();
            draw_powerups();
            draw_ui();
            // Отображение жизней
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
            DrawRectangle(0, 0, 1280, 720, DARKBLUE);
            DrawText("YOU WIN!", 500, 300, 60, GOLD);
            DrawText("Press ENTER for Menu", 510, 400, 25, WHITE);
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