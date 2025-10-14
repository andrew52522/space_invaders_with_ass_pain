#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <random>
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"

struct GLFWwindow; // Предверительное определение

// Структура для буфера экрана
struct Buffer
{
    size_t width, height;
    uint32_t *data;
};

// Структура для анимации
struct SpriteAnimation
{
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    const Sprite **frames;
};

// Структура для укрытий
struct Bunker
{
    int x, y;
    std::vector<uint8_t> pixel_data;
    size_t width, height;
};

class Game
{
public:
    Game(int width, int height);
    ~Game();

    // Запрещаем копирование и разрешаем перемещение (Правило пяти)
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;
    Game(Game &&) = default;
    Game &operator=(Game &&) = default;

    // Публичные методы, доступные для main.cpp
    void Init();
    void Run();
    void CheckGLError();
    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

private:
    void Update();
    void Render();
    void ResetLevel();
    void CheckCollisions();

    // Приватные хелперы
    uint32_t RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void DrawSprite(const Sprite &sprite, size_t x, size_t y, uint32_t color, bool is_super = false);
    void DrawBunker(const Bunker &bunker, uint32_t color);
    void DrawText(const Sprite &font, const char *text, size_t x, size_t y, uint32_t color);
    void DrawNumber(const Sprite &font, size_t number, size_t x, size_t y, uint32_t color);
    bool SpriteOverlapCheck(const Sprite &sp_a, size_t x_a, size_t y_a, const Sprite &sp_b, size_t x_b, size_t y_b);
    bool BulletBunkerOverlapCheck(const Bullet &bullet, const Bunker &bunker, size_t &hit_x, size_t &hit_y);
    bool validate_program(unsigned int program);

    // Оконные и графические ресурсы
    int width, height;
    GLFWwindow *window;
    Buffer buffer;
    unsigned int shader_id;
    unsigned int buffer_texture_id;
    unsigned int vao_id;

    // Игровые объекты
    std::unique_ptr<Player> player;
    std::vector<Enemy> enemies;
    std::vector<Bullet> player_bullets;
    std::vector<Bullet> alien_bullets;
    Bunker bunkers[4]; // Массив укрытий

    // Состояние игры
    enum GameState
    {
        GAME_RUNNING,
        GAME_OVER_WIN,
        GAME_OVER_LOST
    };
    GameState state;
    size_t score;
    int current_level;

    // Переменные для управления вводом
    int player_move_dir;
    bool fire_pressed;
    int switcher_super_bullet;

    // Переменные для логики пришельцев
    int alien_move_dir;
    int alien_speed;
    size_t alien_move_timer;
    size_t alien_frames_per_move;
    size_t alien_drop_amount;
    int alien_drop_count;
    std::vector<uint8_t> death_counters;

    // Ресурсы для анимации
    SpriteAnimation alien_animations[4];

    // Генератор случайных чисел
    std::mt19937 rng;
};

#endif // GAME_H