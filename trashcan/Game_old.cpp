#include "Game.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <random>
#include <stdexcept>
#include <cstring> // Для strlen

// --- Глобальні константи та дані спрайтів (винесені з main) ---
// Ці дані залишаються тут, оскільки вони є константами і не змінюються протягом гри.

// Розміри буфера
const int BUFFER_WIDTH = 640;
const int BUFFER_HEIGHT = 480;

// Ігрові константи
#define GAME_MAX_PLAYER_BULLETS 128
#define GAME_MAX_ALIEN_BULLETS 16
#define GAME_NUM_BUNKERS 4
#define MAX_LEVELS 3
#define ALIENS_ROWS 5
#define ALIENS_COLS 11

// Дані спрайтів (повністю скопійовано з вашого файлу)
const uint8_t alien_a_frame1_data[] = {0,0,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,1,0};
Sprite alien_a_frame1 = {8, 8, alien_a_frame1_data};
const uint8_t alien_a_frame2_data[] = {0,0,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,1,1,1,1,1,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,0,0,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1};
Sprite alien_a_frame2 = {8, 8, alien_a_frame2_data};

const uint8_t alien_b_frame1_data[] = {0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,1,0,1,1,0,0,0};
Sprite alien_b_frame1 = {11, 8, alien_b_frame1_data};
const uint8_t alien_b_frame2_data[] = {0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0};
Sprite alien_b_frame2 = {11, 8, alien_b_frame2_data};

const uint8_t alien_c_frame1_data[] = {0,0,0,0,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0,1,1,0,1,1,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1};
Sprite alien_c_frame1 = {12, 8, alien_c_frame1_data};
const uint8_t alien_c_frame2_data[] = {0,0,0,0,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,1,1,0,0,0,0,1,1,0,0};
Sprite alien_c_frame2 = {12, 8, alien_c_frame2_data};

const uint8_t player_data[] = {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
Sprite player_sprite_data = {11, 7, player_data};

const uint8_t bullet_data[] = {1, 1, 1};
Sprite bullet_sprite_data = {1, 3, bullet_data};

const uint8_t alien_death_data[] = {0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,1,0};
Sprite alien_death_sprite_data = {13, 7, alien_death_data};

const uint8_t bunker_base_data[] = {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
Sprite bunker_base_sprite_data = {22, 16, bunker_base_data};
// ... і так далі для всіх спрайтів, включаючи шрифт і серце ...

   // --- Спрайт-лист для шрифта (5x7 пикселей на символ) ---
    // Начинается с ASCII 32 (' ') и заканчивается ASCII 96 ('`'). Всего 65 символов.
    // Важно: данные должны соответствовать порядку ASCII символов от 32 до 96.
    
    const uint8_t font_5x7_data[] = {
        // Пробел (ASCII 32)
        0,0,0,0,0, 
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // ! (ASCII 33)
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0,

        // " (ASCII 34)
        0,1,0,1,0,
        0,1,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // # (ASCII 35)
        0,1,0,1,0,
        1,1,1,1,1,
        0,1,0,1,0,
        1,1,1,1,1,
        0,1,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // $ (ASCII 36)
        0,0,1,0,0,
        0,1,1,0,0,
        1,0,1,0,0,
        0,1,1,1,0,
        0,0,1,0,1,
        0,0,1,1,0,
        0,0,1,0,0,

        // % (ASCII 37)
        1,1,0,0,0,
        1,1,0,0,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,0,0,1,1,
        0,0,0,1,1,

        // & (ASCII 38)
        0,1,1,0,0,
        1,0,0,1,0,
        0,1,1,0,0,
        1,0,1,1,0,
        1,0,1,0,1,
        0,1,1,0,1,
        0,1,0,1,0,

        // ' (ASCII 39)
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // ( (ASCII 40)
        0,0,0,1,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,1,0,

        // ) (ASCII 41)
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,0,0,0,

        // * (ASCII 42)
        0,0,0,0,0,
        0,1,0,1,0,
        0,0,1,0,0,
        0,1,1,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        0,0,0,0,0,

        // + (ASCII 43)
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        1,1,1,1,1,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,

        // , (ASCII 44)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,0,0,0,

        // - (ASCII 45)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        1,1,1,1,1,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // . (ASCII 46)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,

        // / (ASCII 47)
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        1,0,0,0,0,

        // 0 (ASCII 48)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // 1 (ASCII 49)
        0,0,1,0,0,
        0,1,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,1,1,0,

        // 2 (ASCII 50)
        0,1,1,1,0,
        1,0,0,0,1,
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        1,1,1,1,1,

        // 3 (ASCII 51)
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,1,0,
        0,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // 4 (ASCII 52)
        0,0,0,1,0,
        0,0,1,1,0,
        0,1,0,1,0,
        1,0,0,1,0,
        1,1,1,1,1,
        0,0,0,1,0,
        0,0,0,1,0,

        // 5 (ASCII 53)
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,0,
        0,0,0,0,1,
        0,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // 6 (ASCII 54)
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // 7 (ASCII 55)
        1,1,1,1,1,
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,

        // 8 (ASCII 56)
        0,1,1,1,0,
        1,0,0,0,1,
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // 9 (ASCII 57)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,

        // : (ASCII 58)
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,

        // ; (ASCII 59)
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,0,0,0,

        // < (ASCII 60)
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        1,0,0,0,0,
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,

        // = (ASCII 61)
        0,0,0,0,0,
        1,1,1,1,1,
        0,0,0,0,0,
        1,1,1,1,1,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // > (ASCII 62)
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,

        // ? (ASCII 63)
        0,1,1,1,0,
        1,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,

        // @ (ASCII 64)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,1,1,0,
        1,0,0,0,0,
        0,1,1,1,0,

        // A (ASCII 65)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,

        // B (ASCII 66)
        1,1,1,1,0,
        1,0,0,0,1,
        1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,0,

        // C (ASCII 67)
        0,1,1,1,1,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        0,1,1,1,1,

        // D (ASCII 68)
        1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,0,

        // E (ASCII 69)
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,1,1,1,1,

        // F (ASCII 70)
        1,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,

        // G (ASCII 71)
        0,1,1,1,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,1,

        // H (ASCII 72)
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,

        // I (ASCII 73)
        1,1,1,1,1,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        1,1,1,1,1,

        // J (ASCII 74)
        0,0,0,0,1,
        0,0,0,0,1,
        0,0,0,0,1,
        0,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // K (ASCII 75)
        1,0,0,0,1,
        1,0,0,1,0,
        1,0,1,0,0,
        1,1,0,0,0,
        1,0,1,0,0,
        1,0,0,1,0,
        1,0,0,0,1,

        // L (ASCII 76)
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,1,1,1,1,

        // M (ASCII 77)
        1,0,0,0,1,
        1,1,0,1,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,

        // N (ASCII 78)
        1,0,0,0,1,
        1,1,0,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        1,0,0,1,1,
        1,0,0,0,1,
        1,0,0,0,1,

        // O (ASCII 79)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // P (ASCII 80)
        1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,

        // Q (ASCII 81)
        0,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,1,0,1,
        1,0,0,1,0,
        0,1,1,1,1,

        // R (ASCII 82)
        1,1,1,1,0,
        1,0,0,0,1,
        1,0,0,0,1,
        1,1,1,1,0,
        1,0,0,1,0,
        1,0,0,0,1,
        1,0,0,0,1,

        // S (ASCII 83)
        0,1,1,1,1,
        1,0,0,0,0,
        1,1,1,1,0,
        0,0,0,0,1,
        0,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // T (ASCII 84)
        1,1,1,1,1,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,

        // U (ASCII 85)
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,1,1,0,

        // V (ASCII 86)
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,0,1,0,
        0,1,0,1,0,
        0,0,1,0,0,

        // W (ASCII 87)
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,0,0,1,
        1,0,1,0,1,
        1,0,1,0,1,
        0,1,0,1,0,
        0,1,0,1,0,

        // X (ASCII 88)
        1,0,0,0,1,
        1,0,0,0,1,
        0,1,0,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        1,0,0,0,1,
        1,0,0,0,1,

        // Y (ASCII 89)
        1,0,0,0,1,
        0,1,0,1,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,

        // Z (ASCII 90)
        1,1,1,1,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        1,0,0,0,0,
        1,0,0,0,0,
        1,1,1,1,1,

        // [ (ASCII 91)
        0,1,1,1,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,1,1,0,

        // \ (ASCII 92)
        1,0,0,0,0,
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,0,1,
        0,0,0,0,1,

        // ] (ASCII 93)
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,1,0,

        // ^ (ASCII 94)
        0,0,1,0,0,
        0,1,0,1,0,
        1,0,0,0,1,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,

        // _ (ASCII 95)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        1,1,1,1,1,

        // ` (ASCII 96)
        1,0,0,0,0,
        0,1,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0
    };
    Sprite font_5x7_sprite = {5, 7, font_5x7_data}; // Ширина 5, высота 7 для каждого символа

// --- Реалізація класу Game ---

Game::Game(int width, int height) :
    width(width),
    height(height),
    window(nullptr),
    state(GAME_RUNNING),
    score(0),
    current_level(1),
    player_move_dir(0),
    fire_pressed(false),
    switcher_super_bullet(0)
{
    buffer.width = width;
    buffer.height = height;
    buffer.data = nullptr; // Виділимо пам'ять в Init

    // Ініціалізація генератора випадкових чисел
    rng.seed(std::random_device{}());
}

Game::~Game() {
    delete[] buffer.data;

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Game::Init() {
    // --- Ініціалізація GLFW ---
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, "Space Invaders OOP", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- Ініціалізація GLEW ---
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // --- Налаштування обробників вводу ---
    glfwSetWindowUserPointer(window, this); // Зберігаємо вказівник на цей об'єкт Game
    glfwSetKeyCallback(window, Game::KeyCallback); // Встановлюємо статичний обробник

    // --- Створення буфера ---
    buffer.data = new uint32_t[width * height];

    // --- Створення гравця ---
    player = std::make_unique<Player>((width - player_sprite_data.width) / 2, 32, player_sprite_data);

    // --- Ініціалізація анімацій ---
    // Анимация для ALIEN_TYPE_A
    const Sprite* alien_a_frames[] = {&alien_a_frame1, &alien_a_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_A)] = {true, 2, 30, 0, alien_a_frames};
    // ... і так далі для інших типів прибульців ...

    // Анимация для ALIEN_TYPE_B
    const Sprite* alien_b_frames[] = {&alien_b_frame1, &alien_b_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_B)] = {true, 2, 30, 0, alien_b_frames};

    // Анимация для ALIEN_TYPE_C
    const Sprite* alien_c_frames[] = {&alien_c_frame1, &alien_c_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_C)] = {true, 2, 30, 0, alien_c_frames};

    // --- Перший запуск рівня ---
    ResetLevel();

    // --- Налаштування OpenGL (шейдери, текстури) ---
     // --- Настройка OpenGL ---
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_id, 1, &vertex_shader, NULL);
    glCompileShader(vertex_shader_id);

    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader_id, 1, &fragment_shader, NULL);
    glCompileShader(fragment_shader_id);

    GLuint shader_id = glCreateProgram();
    glAttachShader(shader_id, vertex_shader_id);
    glAttachShader(shader_id, fragment_shader_id);
    glLinkProgram(shader_id);
    if (!validate_program(shader_id)) return -1;

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    glUseProgram(shader_id);
    glBindVertexArray(vao);
}

void Game::Run() {
    while (!glfwWindowShouldClose(window)) {
        Update();
        Render();
        glfwPollEvents();
    }
}

void Game::Update() {
    if (state != GAME_RUNNING) {
        return; // Якщо гра закінчена, нічого не оновлювати
    }

    // Оновлення гравця
    player->Move(player_move_dir);
    player->Update();
    player_move_dir = 0; // Скидаємо напрямок після обробки

    // Стрільба гравця
    if (fire_pressed) {
        if (player_bullets.size() < GAME_MAX_PLAYER_BULLETS) { // Проста перевірка
             player_bullets.emplace_back(
                player->GetX() + (player->GetSprite().width / 2),
                player->GetY() + player->GetSprite().height,
                4, // direction
                (switcher_super_bullet == 1),
                bullet_sprite_data);
        }
        fire_pressed = false;
    }

    // Оновлення куль гравця
    for (auto& bullet : player_bullets) {
        bullet.Update();
    }
    // Видалення неактивних куль
    player_bullets.erase(
        std::remove_if(player_bullets.begin(), player_bullets.end(), [](const Bullet& b) {
            return !b.IsActive();
        }),
        player_bullets.end());

    // Оновлення прибульців (рух рою)
    // Обновление анимации пришельцев
    for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
        SpriteAnimation& current_anim = alien_animations[type_idx];
        ++current_anim.time;
        if (current_anim.time >= current_anim.num_frames * current_anim.frame_duration) {
            if (current_anim.loop) current_anim.time = 0;
        }
    }
    std::cout << "main game loop cycle update animation\n";
    // Обновление таймеров смерти пришельцев
    for (size_t i = 0; i < game.num_aliens; ++i) {
        if (game.aliens[i].type == ALIEN_DEAD && death_counters[i] > 0) {
            --death_counters[i];
        }
    }

    // Движение толпы пришельцев
            game.alien_move_timer++;
            if (game.alien_move_timer >= game.alien_frames_per_move) {
                game.alien_move_timer = 0;
                bool should_drop = false;

        // Проверка границ
        for (size_t i = 0; i < game.num_aliens; ++i) {
            if (game.aliens[i].type == ALIEN_DEAD) continue;

            const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
            int alien_right = game.aliens[i].x + sprite.width;
            int alien_left = game.aliens[i].x;

            if ((game.alien_move_dir == 1 && alien_right + game.alien_speed > game.width) ||
                (game.alien_move_dir == -1 && alien_left - game.alien_speed < 0)) {
                should_drop = true;
                break;
            }
        }

        if (should_drop) {
            // Увеличиваем счетчик спусков
            game.alien_drop_count++;

            // Проверка на 6 спусков (поражение)
            if (game.alien_drop_count >= 10) {
                game.state = GAME_OVER_LOST;
            }

            game.alien_move_dir *= -1; // Меняем направление

            // Опускаем всех пришельцев вниз
            for (size_t i = 0; i < game.num_aliens; ++i) {
                if (game.aliens[i].type != ALIEN_DEAD) {
                    game.aliens[i].y -= game.alien_drop_amount;
                    
                    // Проверка достижения игрока (дополнительное условие поражения)
                    //int alien_bottom_y = game.aliens[i].y + 8;
                    //int bunker_top_y = game.bunkers[0].y;
                    //if (alien_bottom_y >= bunker_top_y - 5) 
                    {
                        //game.state = GAME_OVER_LOST;
                    }
                }
            }
        }

    // Двигаем пришельцев в текущем направлении
    for (size_t i = 0; i < game.num_aliens; ++i) {
        if (game.aliens[i].type != ALIEN_DEAD) {
            game.aliens[i].x += game.alien_move_dir * game.alien_speed;
            
            // Ограничение в пределах экрана
            const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
            if (game.aliens[i].x < 0) game.aliens[i].x = 0;
            if (game.aliens[i].x + sprite.width > game.width) {
                game.aliens[i].x = game.width - sprite.width;
            }
        }
    }
}


    std::cout << "main game loop cycle update movement crowd aliens\n";

    
    // Стрільба прибульців
    // Стрельба пришельцев
            std::uniform_int_distribution<size_t> alien_dist(0, game.num_aliens - 1);
            std::uniform_int_distribution<int> fire_chance(0, 100); // 5% шанс выстрела

            // Разрешаем до GAME_MAX_ALIEN_BULLETS активных пуль
            for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
                if (fire_chance(rng) < 5 && game.num_alien_bullets < GAME_MAX_ALIEN_BULLETS) { // 5% шанс
                    size_t shooter_idx = alien_dist(rng);
                    if (game.aliens[shooter_idx].type != ALIEN_DEAD) {
                        // Находим свободный слот для пули
                        for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                            if (!game.alien_bullets[i].is_active) {
                                game.alien_bullets[i].x = game.aliens[shooter_idx].x + (alien_a_frame1.width / 2);
                                game.alien_bullets[i].y = game.aliens[shooter_idx].y;
                                game.alien_bullets[i].dir = -2;
                                game.alien_bullets[i].is_active = true;
                                ++game.num_alien_bullets;
                                break;
                            }
                        }
                    }
                }
            }
            std::cout << "main game loop cycle bullets of aliens\n";

            // Максимум 3 пули от пришельцев одновременно
            if (active_alien_bullets < 3) {
                for (int count = 0; count < (3 - active_alien_bullets); ++count) { // Попытка выстрелить столько раз, сколько не хватает до 3х пуль
                    if (fire_chance(rng) < 1) { // 1% шанс для каждого выстрела
                        size_t shooter_idx = alien_dist(rng);
                        if (game.aliens[shooter_idx].type != ALIEN_DEAD) {
                            // Находим свободный слот для пули пришельца
                            size_t bullet_idx_to_use = -1;
                            for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                                if (!game.alien_bullets[i].is_active) {
                                    bullet_idx_to_use = i;
                                    break;
                                }
                            }
                            if (bullet_idx_to_use != (size_t)-1) {
                                game.alien_bullets[bullet_idx_to_use].x = game.aliens[shooter_idx].x + (alien_a_frame1.width / 2); // Берем ширину спрайта A для центрирования
                                game.alien_bullets[bullet_idx_to_use].y = game.aliens[shooter_idx].y;
                                game.alien_bullets[bullet_idx_to_use].dir = -2; // Скорость пули (вниз)
                                game.alien_bullets[bullet_idx_to_use].is_active = true;
                                game.num_alien_bullets++; // Увеличиваем счетчик активных пуль пришельцев
                            }
                        }
                    }
                }
            }
            std::cout << "main game loop cycle max bullets of group aliens\n";

    // Оновлення куль прибульців
    // --- Обновление пуль пришельцев ---
                for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                    if (game.alien_bullets[i].is_active) {
                        game.alien_bullets[i].y += game.alien_bullets[i].dir;

                        if (game.alien_bullets[i].y < 0) {
                            game.alien_bullets[i].is_active = false;
                            --game.num_alien_bullets;
                        }
                    }
                }
            std::cout << "main game loop cycle update bullets aliens\n";

    // Перевірка зіткнень
    CheckCollisions();

    // Перевірка умов перемоги/поразки
    if (player->GetLife() == 0) {
        state = GAME_OVER_LOST;
    }
    
    bool all_aliens_dead = true;
    for (const auto& enemy : enemies) {
        if (enemy.IsActive()) {
            all_aliens_dead = false;
            break;
        }
    }
    if (all_aliens_dead) {
        current_level++;
        if (current_level > MAX_LEVELS) {
            state = GAME_OVER_WIN;
        } else {
            ResetLevel();
        }
    }
}

void Game::Render() {
    // Очищення буфера
    uint32_t clear_color = RgbaToUint32(0, 0, 0, 255);
    for (size_t i = 0; i < buffer.width * buffer.height; ++i) {
        buffer.data[i] = clear_color;
    }

    // Малювання гравця
    DrawSprite(player->GetSprite(), player->GetX(), player->GetY(), RgbaToUint32(0, 255, 0, 255));

    // Малювання прибульців
    for (const auto& enemy : enemies) {
        if (enemy.IsActive()) {
             // Колір залежить від типу, як у вашому коді
            uint32_t color;
            switch(enemy.GetType()) {
                case AlienType::TYPE_A: color = RgbaToUint32(255, 0, 0, 255); break;
                case AlienType::TYPE_B: color = RgbaToUint32(0, 130, 125, 255); break;
                case AlienType::TYPE_C: color = RgbaToUint32(0, 0, 255, 255); break;
                default: color = RgbaToUint32(255, 255, 255, 255);
            }
            // Потрібно додати логіку анімації
            DrawSprite(enemy.GetSprite(), enemy.GetX(), enemy.GetY(), color);
        }
    }

    // Малювання куль гравця
    for (const auto& bullet : player_bullets) {
         uint32_t color = bullet.IsSuper() ? 
                            RgbaToUint32(255, 0, 255, 255) :
                            RgbaToUint32(255, 255, 0, 255);
        DrawSprite(bullet.GetSprite(), bullet.GetX(), bullet.GetY(), color, bullet.IsSuper());
    }

    // Малювання куль прибульців
    for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
            if (game.alien_bullets[i].is_active) {
                buffer_sprite_draw(&buffer, bullet_sprite, game.alien_bullets[i].x, game.alien_bullets[i].y, rgba_to_uint32(255, 255, 255, 255)); // Белые пули
            }
        }

    // Малювання укриттів
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
            buffer_bunker_draw(&buffer, game.bunkers[i], rgba_to_uint32(0, 255, 0, 255)); // Зеленые укрытия
        }

    // Малювання інтерфейсу (рахунок, життя)
    // Рисуем счет
        buffer_draw_text(&buffer, font_5x7_sprite, "SCORE", 4, 10, rgba_to_uint32(255, 255, 255, 255));
        buffer_draw_number(&buffer, font_5x7_sprite, game.score, 60, 10, rgba_to_uint32(255, 255, 255, 255));
        

        buffer_draw_number(
            &buffer,
            font_5x7_sprite, game.score,
            4 + (font_5x7_sprite.width + 1) * 6, game.height - font_5x7_sprite.height - 7,
            rgba_to_uint32(128, 0, 0, 255)
        );

        // Рисуем жизни игрока в виде сердец
        // Рисуем жизни игрока (ИСПРАВЛЕНО: смещено на 10 пикселей ниже)
        size_t lives_y_pos = 10 + font_5x7_sprite.height + 10;
        buffer_draw_text(
            &buffer, font_5x7_sprite, 
            "LIVES", game.width - 100,
            10, rgba_to_uint32(255, 255, 255, 255));
        
            for (size_t i = 0; i < game.player.life; ++i) {
            buffer_sprite_draw(
                &buffer, heart_sprite, 
                game.width - 40 + i * (heart_sprite.width + 2),
                 10, rgba_to_uint32(255, 0, 0, 255));
        }

        // Рисуем "CREDIT 00" внизу по центру
        const char* credit_text = "CREDIT 00";
        size_t credit_text_width = (font_5x7_sprite.width + 1) * 9 - 1;
        buffer_draw_text(
            &buffer,
            font_5x7_sprite, credit_text,
            (game.width - credit_text_width) / 2, 7,
            rgba_to_uint32(128, 0, 0, 255)
        );

        // Рисуем горизонтальную линию над текстом кредита
        uint32_t line_color = rgba_to_uint32(128, 0, 0, 255);
        size_t line_y_from_bottom = 16;
        for (size_t i = 0; i < game.width; ++i){    
            size_t actual_buffer_y = buffer.height - 1 - line_y_from_bottom;
            if (actual_buffer_y < buffer.height) {
                buffer.data[actual_buffer_y * game.width + i] = line_color;
            }
        }

    // Екран Game Over / Win
    if (state == GAME_OVER_LOST) {
        const char* game_over_text = "GAME OVER";
            size_t text_width = (font_5x7_sprite.width + 1) * strlen(game_over_text);
            buffer_draw_text(&buffer, font_5x7_sprite, game_over_text, (game.width - text_width) / 2, game.height / 2, rgba_to_uint32(255, 0, 0, 255));
    } else if (state == GAME_OVER_WIN) {
       const char* win_text = "YOU WIN";
            size_t text_width = (font_5x7_sprite.width + 1) * strlen(win_text);
            buffer_draw_text(&buffer, font_5x7_sprite, win_text, (game.width - text_width) / 2, game.height / 2, rgba_to_uint32(0, 255, 0, 255));
    }


    // --- Виведення на екран ---
    // --- Вывод на экран ---
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
}

void Game::CheckCollisions() {
    // Куля гравця vs Прибульці
    for (auto& bullet : player_bullets) {
        if (!bullet.IsActive()) continue;

        for (auto& enemy : enemies) {
            if (!enemy.IsActive()) continue;

            // Проста перевірка прямокутників
            if (SpriteOverlapCheck(bullet.GetSprite(), bullet.GetX(), bullet.GetY(),
                                  enemy.GetSprite(), enemy.GetX(), enemy.GetY()))
            {
                enemy.TakeDamage(1);
                if (!bullet.IsSuper()) {
                    bullet.SetActive(false);
                }
                if (!enemy.IsActive()) {
                    score += 10; // Простий підрахунок очок
                }
                break; // Куля вразила одного ворога
            }
        }
    }
    
    // Куля прибульця vs Гравець
    // Обработка столкновений пуль пришельцев с игроком
    for (size_t bi = 0; bi < GAME_MAX_ALIEN_BULLETS;) {
        if (!game.alien_bullets[bi].is_active) {
            ++bi;
            continue;
        }

        bool bullet_removed = false;
        if (sprite_overlap_check(bullet_sprite, game.alien_bullets[bi].x, game.alien_bullets[bi].y,
                                    player_sprite, game.player.x, game.player.y)) {
            game.player.life--;
            game.alien_bullets[bi].is_active = false;
            bullet_removed = true;
            if (game.player.life == 0) {
                game.state = GAME_OVER_LOST;
            }
        }
        if (!bullet_removed) ++bi;
    }
    
    // Кулі vs Укриття
    // Обработка столкновений пуль пришельцев с укрытиями
    for (size_t bi = 0; bi < game.num_alien_bullets; ++bi) {
        if (!game.alien_bullets[bi].is_active) continue;
        for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
            size_t hit_x, hit_y;
            if (bullet_bunker_overlap_check(game.alien_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                int radius = 2; // Небольшой радиус поражения
                for (int y_offset = -radius; y_offset <= radius; ++y_offset) {
                    for (int x_offset = -radius; x_offset <= radius; ++x_offset) {
                        if (x_offset * x_offset + y_offset * y_offset > radius * radius) continue;
                        int px = hit_x + x_offset;
                        int py = hit_y + y_offset;
                        if (px >= 0 && px < game.bunkers[bui].width && py >= 0 && py < game.bunkers[bui].height) {
                            size_t idx = py * game.bunkers[bui].width + px;
                            if (game.bunkers[bui].pixel_data[idx] > 0) {
                                game.bunkers[bui].pixel_data[idx]--;
                            }
                        }
                    }
                }
                game.alien_bullets[bi].is_active = false;
                break;
            }
        }
    }
    // Обработка столкновений пуль игрока с укрытиями
    for (size_t bi = 0; bi < game.num_player_bullets; ++bi) {
        if (!game.player_bullets[bi].is_active) continue;
        for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
            size_t hit_x, hit_y;
            if (bullet_bunker_overlap_check(game.player_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                int radius = 2; // Небольшой радиус поражения
                for (int y_offset = -radius; y_offset <= radius; ++y_offset) {
                    for (int x_offset = -radius; x_offset <= radius; ++x_offset) {
                        if (x_offset * x_offset + y_offset * y_offset > radius * radius) continue;
                        int px = hit_x + x_offset;
                        int py = hit_y + y_offset;
                        if (px >= 0 && px < game.bunkers[bui].width && py >= 0 && py < game.bunkers[bui].height) {
                            size_t idx = py * game.bunkers[bui].width + px;
                            if (game.bunkers[bui].pixel_data[idx] > 0) {
                                game.bunkers[bui].pixel_data[idx]--;
                            }
                        }
                    }
                }
                game.player_bullets[bi].is_active = false;
                break;
            }
        }
    }
    std::cout << "main game loop cycle update bullet of gamer into bunker\n";


    // Прибульці vs Укриття / Гравець
    // Обработка столкновений пули игрока с пришельцами
            const Sprite* current_anim_frames_for_collision[4];
            for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
                const SpriteAnimation& anim = alien_animations[type_idx];
                current_anim_frames_for_collision[type_idx] = anim.frames[anim.time / anim.frame_duration];
            }

            // 5. Модифицируем обработку столкновений пуль с пришельцами
            for (size_t bi = 0; bi < game.num_player_bullets;) {
                if (!game.player_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                bool is_super = game.player_bullets[bi].is_super;
                
                for (size_t ai = 0; ai < game.num_aliens; ++ai) {
                    if (game.aliens[ai].type == ALIEN_DEAD) continue;

                    const Alien& alien = game.aliens[ai];
                    const Sprite& alien_sprite = *current_anim_frames_for_collision[alien.type];

                    if (sprite_overlap_check(bullet_sprite, game.player_bullets[bi].x, game.player_bullets[bi].y,
                                            alien_sprite, alien.x, alien.y)) {
                        // Для супер-пули: уничтожаем всю колонку
                        if (is_super) {
                            // Находим всех пришельцев в этой колонке
                            int column_x = alien.x;
                            for (size_t col_ai = 0; col_ai < game.num_aliens; ++col_ai) {
                                if (game.aliens[col_ai].type != ALIEN_DEAD && 
                                    abs(game.aliens[col_ai].x - column_x) < 10) { // Пороговое значение
                                    game.aliens[col_ai].hp = 0;
                                    game.aliens[col_ai].type = ALIEN_DEAD;
                                    game.score += 10 * (4 - game.aliens[col_ai].type);
                                    death_counters[col_ai] = 15;
                                }
                            }
                            // Супер-пуля продолжает полет
                        } 
                        else { // Обычная пуля
                            game.aliens[ai].hp--;
                            if (game.aliens[ai].hp <= 0) {
                                game.aliens[ai].type = ALIEN_DEAD;
                                game.score += 10 * (4 - alien.type);
                                death_counters[ai] = 15;
                                game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite.width) / 2;
                            }
                            bullet_removed = true;
                            break;
                        }
                    }
                }
                
                if (bullet_removed) {
                    game.player_bullets[bi].is_active = false;
                }
                
                if (game.player_bullets[bi].is_active) {
                    ++bi;
                } else {
                    // Удаляем пулю
                    game.player_bullets[bi] = game.player_bullets[game.num_player_bullets - 1];
                    game.player_bullets[game.num_player_bullets - 1].is_active = false;
                    --game.num_player_bullets;
                }
            }
            std::cout << "main game loop cycle bullet of gamer into alien\n";

}

void Game::ResetLevel() {
    enemies.clear();
    player_bullets.clear();
    alien_bullets.clear();
    
    // Скидаємо позицію гравця
    player->SetPosition((width - player->GetSprite().width) / 2, 32);

    // Створення рою прибульців (логіка з вашого `reset_level` та `main`)
    const size_t spacing_x = 16;
    const size_t spacing_y = 16;
    const size_t start_y = height - 150;
    const size_t total_width = (ALIENS_COLS - 1) * spacing_x + 12; // 12 - ширина alien_c
    const size_t start_x = (width - total_width) / 2;

    for (size_t yi = 0; yi < ALIENS_ROWS; ++yi) {
        for (size_t xi = 0; xi < ALIENS_COLS; ++xi) {
            int current_x = start_x + xi * spacing_x;
            int current_y = start_y - yi * spacing_y;
            AlienType type;
            Sprite sprite;

            if (yi == 0) {
                type = AlienType::TYPE_C;
                sprite = alien_c_frame1;
            } else if (yi < 3) {
                type = AlienType::TYPE_B;
                sprite = alien_b_frame1;
            } else {
                type = AlienType::TYPE_A;
                sprite = alien_a_frame1;
            }
            enemies.emplace_back(current_x, current_y, type, sprite);
        }
    }
    
    // Сброс укрытий
    const size_t bunker_spacing = (game.width - GAME_NUM_BUNKERS * bunker_base_sprite.width) / (GAME_NUM_BUNKERS + 1);
    const size_t bunker_start_y = game.player.y + 20;
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
        game.bunkers[i].x = bunker_spacing * (i + 1) + i * bunker_base_sprite.width;
        game.bunkers[i].y = bunker_start_y;
        game.bunkers[i].width = bunker_base_sprite.width;
        game.bunkers[i].height = bunker_base_sprite.height;
        game.bunkers[i].pixel_data.resize(bunker_base_sprite.width * bunker_base_sprite.height);
        for (size_t p_idx = 0; p_idx < bunker_base_sprite.width * bunker_base_sprite.height; ++p_idx) {
            if (bunker_base_sprite.data[p_idx] == 1) {
                game.bunkers[i].pixel_data[p_idx] = 5; // 5 HP
            } else {
                game.bunkers[i].pixel_data[p_idx] = 0;
            }
        }
    }
}


// --- Статичний обробник вводу ---
void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Отримуємо вказівник на наш об'єкт Game, який ми зберегли раніше
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    if (game->state == GAME_RUNNING) {
        int move_change = 0;
        if (key == GLFW_KEY_RIGHT) move_change = 1;
        if (key == GLFW_KEY_LEFT) move_change = -1;

        if (action == GLFW_PRESS) {
            game->player_move_dir += move_change;
            if (key == GLFW_KEY_S) {
                 game->switcher_super_bullet = !game->switcher_super_bullet;
            }
        } else if (action == GLFW_RELEASE) {
            game->player_move_dir -= move_change;
            if (key == GLFW_KEY_SPACE) {
                game->fire_pressed = true;
            }
        }
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// --- Приватні методи-хелпери для малювання та іншого ---
uint32_t Game::RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void Game::DrawSprite(const Sprite& sprite, size_t x, size_t y, uint32_t color, bool is_super) {
     void buffer_sprite_draw(
    Buffer* buffer, const Sprite& sprite,
    size_t x, size_t y, uint32_t color, bool is_super = false
) {
    if (is_super) {
        // Рисуем длинный вертикальный лазер
        size_t laser_height = buffer->height - y;
        for (size_t yi = 0; yi < laser_height; ++yi) {
            for (size_t xi = 0; xi < sprite.width; ++xi) {
                size_t sy = y + yi;
                size_t sx = x + xi;
                if (sx < buffer->width && sy < buffer->height) {
                    buffer->data[sy * buffer->width + sx] = color;
                }
            }
        }
    } else {
        // Стандартная отрисовка
        for (size_t yi = 0; yi < sprite.height; ++yi) {
            for (size_t xi = 0; xi < sprite.width; ++xi) {
                size_t sy = y + (sprite.height - 1 - yi);
                size_t sx = x + xi;
                if (sprite.data[yi * sprite.width + xi] &&
                    sx < buffer->width && sy < buffer->height) {
                    buffer->data[sy * buffer->width + sx] = color;
                }
            }
        }
    }
}
}

bool Game::SpriteOverlapCheck(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b) {
    return (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
            y_a < y_b + sp_b.height && y_a + sp_a.height > y_b);
}