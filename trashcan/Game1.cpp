#include "Game.h"
#include <GL/glew.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <vector>

// --- Глобальные константы ---
#define GAME_NUM_BUNKERS 4
#define MAX_LEVELS 3
#define ALIENS_ROWS 5
#define ALIENS_COLS 11
#define NUM_ALIENS (ALIENS_ROWS * ALIENS_COLS)

// --- ВСТАВЬТЕ ДАННЫЕ ВАШИХ СПРАЙТОВ ЗДЕСЬ ---
// Вам нужно скопировать все ваши const uint8_t массивы и определения Sprite сюда.
// Я оставил несколько для примера.

// --- Спрайты пришельцев (из гайда) ---
    const uint8_t alien_a_frame1_data[] = {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,0,0,0,0,0,1, // @......@
        0,1,0,0,0,0,1,0  // .@....@.
    };
    Sprite alien_a_frame1 = {8, 8, alien_a_frame1_data};

    const uint8_t alien_a_frame2_data[] = {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,0,1,0,0,1,0,0, // ..@..@..
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,1,0,0,1,0,1  // @.@..@.@
    };
    Sprite alien_a_frame2 = {8, 8, alien_a_frame2_data};

    const uint8_t alien_b_frame1_data[] = {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };
    Sprite alien_b_frame1 = {11, 8, alien_b_frame1_data};

    const uint8_t alien_b_frame2_data[] = {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };
    Sprite alien_b_frame2 = {11, 8, alien_b_frame2_data};

    const uint8_t alien_c_frame1_data[] = {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
        0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
        1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
    };
    Sprite alien_c_frame1 = {12, 8, alien_c_frame1_data};

    const uint8_t alien_c_frame2_data[] = {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
        0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
        0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
    };
    Sprite alien_c_frame2 = {12, 8, alien_c_frame2_data};

    const uint8_t player_data[] = {
        0,0,0,0,0,1,0,0,0,0,0,
        0,0,0,0,1,1,1,0,0,0,0,
        0,0,0,0,1,1,1,0,0,0,0,
        0,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1
    };
    Sprite player_sprite_data = {11, 7, player_data};

    const uint8_t bullet_data[] = {1, 1, 1};
    Sprite bullet_sprite_data = {1, 3, bullet_data};

    const uint8_t alien_death_data[] = {
        0,1,0,0,1,0,0,0,1,0,0,1,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,1,1,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,1,0,0,1,0,0,0,1,0,0,1,0
    };
    Sprite alien_death_sprite_data = {13, 7, alien_death_data};

    // Спрайт сердца для индикации здоровья
    const uint8_t heart_data[] = {
        0,1,1,0,1,1,0,
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,
        0,1,1,1,1,1,0,
        0,0,1,1,1,0,0,
        0,0,0,1,0,0,0
    };
    Sprite heart_sprite_data = {7, 6, heart_data}; // Размеры сердца

    // Спрайт бункера (основа)
    const uint8_t bunker_base_data[] = {
        0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0, // 22x16
        0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
        0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
        0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1, // Нижние "ножки"
        1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
    };
    Sprite bunker_base_sprite_data = {22, 16, bunker_base_data}; // Размеры базового спрайта бункера


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
    Sprite font_5x7_sprite_data = {5, 7, font_5x7_data}; // Ширина 5, высота 7 для каждого символа

// --- Шейдеры ---
// --- ИСПРАВЛЕННЫЕ ШЕЙДЕРЫ ---
const char* vertex_shader_source = R"(
    #version 330 core
    
    // Координаты вершин для двух треугольников, образующих прямоугольник
    const vec2 pos[6] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0,  1.0),
        vec2( 1.0, -1.0), vec2(1.0,  1.0), vec2(-1.0,  1.0)
    );
    
    // Соответствующие им текстурные координаты
    const vec2 tex[6] = vec2[](
        vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
        vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)
    );

    // Выходная переменная, которая будет передана во фрагментный шейдер
    out vec2 TexCoord;

    void main() {
       // Берем позицию и текстурную координату по ID вершины
       gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
       TexCoord = tex[gl_VertexID];
    }
)";

const char* fragment_shader_source = R"(
    #version 330 core
    uniform sampler2D buffer;
    in vec2 TexCoord;
    out vec4 outColor;
    void main() {
       outColor = texture(buffer, TexCoord);
    }
)";


// --- Вспомогательная функция для проверки ошибок компиляции шейдера ---
bool validate_shader(GLuint shader_id, const std::string& type) {
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> log(log_length);
        glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
        std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n" << log.data() << std::endl;
        return false;
    }
    return true;
}


// --- Реализация класса Game ---

Game::Game(int w, int h) :
    width(w), height(h), window(nullptr), state(GAME_RUNNING), score(0), current_level(1),
    player_move_dir(0), fire_pressed(false), switcher_super_bullet(0),
    alien_move_dir(1), alien_speed(10), alien_move_timer(0), alien_frames_per_move(60),
    alien_drop_amount(10), alien_drop_count(0)
{
    buffer.width = width;
    buffer.height = height;
    buffer.data = nullptr;
    rng.seed(std::random_device{}());
    death_counters.resize(NUM_ALIENS, 0);
}

Game::~Game() {
    delete[] buffer.data;
    glDeleteProgram(shader_id);
    glDeleteTextures(1, &buffer_texture_id);
    glDeleteVertexArrays(1, &vao_id);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Game::CheckGLError() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

void Game::Init() {
    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
    std::cout << "GLFW initialized" << std::endl;
    CheckGLError();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, "Space Invaders OOP", NULL, NULL);
    std::cout << "Window created" << std::endl;
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) throw std::runtime_error("Failed to initialize GLEW");
    std::cout << "GLEW initialized" << std::endl;
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, Game::KeyCallback);

    buffer.data = new uint32_t[width * height];

    player = std::make_unique<Player>((width - player_sprite_data.width) / 2, 32, player_sprite_data);

    const Sprite* alien_a_frames[] = {&alien_a_frame1, &alien_a_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_A)] = {true, 2, 30, 0, alien_a_frames};
    const Sprite* alien_b_frames[] = {&alien_b_frame1, &alien_b_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_B)] = {true, 2, 30, 0, alien_b_frames};
    const Sprite* alien_c_frames[] = {&alien_c_frame1, &alien_c_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_C)] = {true, 2, 30, 0, alien_c_frames};

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    std::cout << "Shader1 compiled" << std::endl;

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    std::cout << "Shader2 compiled" << std::endl;

    shader_id = glCreateProgram();
    glAttachShader(shader_id, vertex_shader);
    glAttachShader(shader_id, fragment_shader);
    glLinkProgram(shader_id);
    std::cout << "Shader linked" << std::endl;

    CheckGLError();

    if (!validate_program(shader_id)) {
        throw std::runtime_error("Shader program validation failed");
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    

    // Активируем шейдерную программу (достаточно сделать это один раз при инициализации)
    glUseProgram(shader_id); 
    
    // Генерация и привязка VAO все равно необходимы для совместимости с Core Profile, 
    // даже если мы не используем VBO
    glGenVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id); 
    
    // --- 4. Текстура для буфера кадра ---
    glGenTextures(1, &buffer_texture_id);
    glBindTexture(GL_TEXTURE_2D, buffer_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // glUseProgram(0); // Опционально, можно оставить программу активной
    glBindVertexArray(0); // Отвязываем VAO

    ResetLevel();
    CheckGLError();
}

void Game::Run() {
    while (!glfwWindowShouldClose(window)) {
        std::cout << "Loop iteration" << std::endl;
        if (state == GAME_RUNNING) {
            Update();
        }
        Render();
        glfwPollEvents();
    }
}
// std::cout << "Exiting main loop" << std::endl;



void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    
    if (game->state == GAME_RUNNING) {
        int move_change = 0;
        if (key == GLFW_KEY_LEFT) move_change = -1;
        if (key == GLFW_KEY_RIGHT) move_change = 1;
        
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
}

void Game::Update() {
    player->Move(player_move_dir);
    player->Update();
    
    if (fire_pressed) {
        bool can_fire = true;
        for(const auto& b : player_bullets) {
            if (b.IsActive()) { can_fire = false; break; }
        }
        if (can_fire) {
             player_bullets.emplace_back(
                player->GetX() + (player_sprite_data.width / 2),
                player->GetY() + player_sprite_data.height,
                4, (switcher_super_bullet == 1), bullet_sprite_data);
        }
        fire_pressed = false;
    }

    for(auto& b : player_bullets) b.Update();
    player_bullets.erase(std::remove_if(player_bullets.begin(), player_bullets.end(), 
        [](const Bullet& b){ return !b.IsActive(); }), player_bullets.end());

    for(auto& b : alien_bullets) b.Update();
    alien_bullets.erase(std::remove_if(alien_bullets.begin(), alien_bullets.end(), 
        [](const Bullet& b){ return !b.IsActive(); }), alien_bullets.end());

    for (size_t type_idx = 1; type_idx <= 3; ++type_idx) {
        SpriteAnimation& anim = alien_animations[type_idx];
        ++anim.time;
        if (anim.time >= anim.num_frames * anim.frame_duration) {
            if (anim.loop) anim.time = 0;
        }
    }

    for (size_t i = 0; i < NUM_ALIENS; ++i) {
        if (death_counters[i] > 0) --death_counters[i];
    }
    
    alien_move_timer++;
    if (alien_move_timer >= alien_frames_per_move) {
        alien_move_timer = 0;
        bool should_drop = false;
        for (size_t i=0; i < enemies.size(); ++i) {
            if (!enemies[i].IsActive()) continue;
            int x = enemies[i].GetX();
            int w = enemies[i].GetSprite().width;
            if ((alien_move_dir > 0 && x + w + alien_speed > width) || (alien_move_dir < 0 && x - alien_speed < 0)) {
                should_drop = true;
                break;
            }
        }
        if (should_drop) {
            alien_move_dir *= -1;
            alien_drop_count++;
            if (alien_drop_count >= 10) state = GAME_OVER_LOST;
            for (auto& enemy : enemies) {
                if(enemy.IsActive()) enemy.SetPosition(enemy.GetX(), enemy.GetY() - alien_drop_amount);
            }
        } else {
            for (auto& enemy : enemies) {
                if(enemy.IsActive()) enemy.Move(alien_move_dir * alien_speed, 0);
            }
        }
    }

    std::uniform_int_distribution<int> fire_chance(0, 200);
    if (fire_chance(rng) < 5 && alien_bullets.size() < 3) {
        std::vector<int> active_aliens;
        for(int i = 0; i < enemies.size(); ++i) if(enemies[i].IsActive()) active_aliens.push_back(i);
        if(!active_aliens.empty()) {
            std::uniform_int_distribution<size_t> dist(0, active_aliens.size() - 1);
            const auto& shooter = enemies[active_aliens[dist(rng)]];
            alien_bullets.emplace_back(shooter.GetX() + shooter.GetSprite().width/2, shooter.GetY(), -2, false, bullet_sprite_data);
        }
    }
    CheckCollisions();
    
    if (player->GetLife() == 0) state = GAME_OVER_LOST;
    
    bool all_aliens_dead = true;
    for (const auto& enemy : enemies) {
        if (enemy.IsActive()) { all_aliens_dead = false; break; }
    }
    if (all_aliens_dead) {
        current_level++;
        if (current_level > MAX_LEVELS) state = GAME_OVER_WIN;
        else ResetLevel();
    }
}

void Game::CheckCollisions() {
    // Пули игрока vs (Пришельцы | Укрытия)
    for (auto& bullet : player_bullets) {
        if (!bullet.IsActive()) continue;
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i].IsActive()) continue;
            if (SpriteOverlapCheck(bullet.GetSprite(), bullet.GetX(), bullet.GetY(), enemies[i].GetSprite(), enemies[i].GetX(), enemies[i].GetY())) {
                enemies[i].TakeDamage(1);
                bullet.SetActive(false);
                if (!enemies[i].IsActive()) {
                    score += 10 * (4 - static_cast<int>(enemies[i].GetType()));
                    death_counters[i] = 15;
                }
                goto next_player_bullet;
            }
        }
        for (auto& bunker : bunkers) {
            size_t hit_x, hit_y;
            if (BulletBunkerOverlapCheck(bullet, bunker, hit_x, hit_y)) {
                int r=2; for(int yo=-r;yo<=r;++yo) for(int xo=-r;xo<=r;++xo) if(xo*xo+yo*yo<=r*r) {
                    int px=hit_x+xo, py=hit_y+yo; if(px>=0&&px<bunker.width&&py>=0&&py<bunker.height) {
                        size_t idx=py*bunker.width+px; if(bunker.pixel_data[idx]>0) bunker.pixel_data[idx]--;
                }}
                bullet.SetActive(false);
                goto next_player_bullet;
            }
        }
        next_player_bullet:;
    }

    // Пули пришельцев vs (Игрок | Укрытия)
    for (auto& bullet : alien_bullets) {
        if (!bullet.IsActive()) continue;
        if (SpriteOverlapCheck(bullet.GetSprite(), bullet.GetX(), bullet.GetY(), player->GetSprite(), player->GetX(), player->GetY())) {
            player->DecreaseLife();
            bullet.SetActive(false);
            continue;
        }
        for (auto& bunker : bunkers) {
            size_t hit_x, hit_y;
            if (BulletBunkerOverlapCheck(bullet, bunker, hit_x, hit_y)) {
                int r=2; for(int yo=-r;yo<=r;++yo) for(int xo=-r;xo<=r;++xo) if(xo*xo+yo*yo<=r*r) {
                    int px=hit_x+xo, py=hit_y+yo; if(px>=0&&px<bunker.width&&py>=0&&py<bunker.height) {
                        size_t idx=py*bunker.width+px; if(bunker.pixel_data[idx]>0) bunker.pixel_data[idx]--;
                }}
                bullet.SetActive(false);
                goto next_alien_bullet;
            }
        }
        next_alien_bullet:;
    }
}


void Game::Render() {
    uint32_t clear_color = RgbaToUint32(0, 0, 0, 255);
    std::fill(buffer.data, buffer.data + width * height, clear_color);

    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto& enemy = enemies[i];
        if (enemy.IsActive()) {
            const auto& anim = alien_animations[static_cast<int>(enemy.GetType())];
            size_t frame_idx = (anim.time / anim.frame_duration) % anim.num_frames;
            const Sprite& s = *anim.frames[frame_idx];
            uint32_t color;
            switch (enemy.GetType()) {
                case AlienType::TYPE_A: color = RgbaToUint32(255, 0, 0, 255); break;
                case AlienType::TYPE_B: color = RgbaToUint32(0, 130, 125, 255); break;
                case AlienType::TYPE_C: color = RgbaToUint32(0, 0, 255, 255); break;
                default: color = RgbaToUint32(255, 255, 255, 255);
            }
            DrawSprite(s, enemy.GetX(), enemy.GetY(), color);
        } else if (death_counters[i] > 0) {
            DrawSprite(alien_death_sprite_data, enemies[i].GetX(), enemies[i].GetY(), RgbaToUint32(255, 255, 0, 255));
        }

        
    }

    for (const auto& b : player_bullets) if(b.IsActive()) {
        uint32_t color = b.IsSuper() ? RgbaToUint32(255,0,255,255) : RgbaToUint32(255,255,0,255);
        DrawSprite(b.GetSprite(), b.GetX(), b.GetY(), color, b.IsSuper());
    }
    for (const auto& b : alien_bullets) if(b.IsActive()) DrawSprite(b.GetSprite(), b.GetX(), b.GetY(), RgbaToUint32(255,255,255,255));
    
    DrawSprite(player->GetSprite(), player->GetX(), player->GetY(), RgbaToUint32(0, 255, 0, 255));

    for (const auto& b : bunkers) DrawBunker(b, RgbaToUint32(0, 255, 0, 255));
    
    DrawText(font_5x7_sprite_data, "SCORE", 4, 10, RgbaToUint32(255,255,255,255));
    DrawNumber(font_5x7_sprite_data, score, 60, 10, RgbaToUint32(255,255,255,255));
    DrawText(font_5x7_sprite_data, "LIVES", width - 100, 10, RgbaToUint32(255,255,255,255));
    for (size_t i = 0; i < player->GetLife(); ++i) {
        DrawSprite(heart_sprite_data, width - 40 + i * (heart_sprite_data.width + 2), 10, RgbaToUint32(255,0,0,255));
    }
    
    if (state == GAME_OVER_LOST) {
        DrawText(font_5x7_sprite_data, "GAME OVER", width/2 - 40, height/2, RgbaToUint32(255,0,0,255));
    } else if (state == GAME_OVER_WIN) {
        DrawText(font_5x7_sprite_data, "YOU WIN", width/2 - 30, height/2, RgbaToUint32(0,255,0,255));
    }

    // 1. Обновляем данные текстуры
    glBindTexture(GL_TEXTURE_2D, buffer_texture_id); // Убедимся, что текстура привязана
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    CheckGLError(); 
    
    // --- ГРАФИЧЕСКИЙ РЕНДЕРИНГ НА GPU ---

    // 2. Активируем шейдерную программу, которая будет рендерить квад с текстурой
    glUseProgram(shader_id); 

    // 3. Привязываем VAO, который содержит данные вершин полноэкранного квада
    glBindVertexArray(vao_id);

    // 4. Собственно отрисовка квада (6 вершин, 2 треугольника)
    glDrawArrays(GL_TRIANGLES, 0, 6); // !!! ИСПРАВЛЕНО: Должно быть 6 вершин, а не 3!

    glBindVertexArray(0); // Отвязываем VAO (хорошая практика)
    glUseProgram(0);      // Отвязываем программу (хорошая практика)
    
    CheckGLError(); // Проверка после отрисовки

    // 5. Показываем результат и чистим буфер
    glfwSwapBuffers(window);

    // Очистка буфера экрана (важно делать это ПОСЛЕ отрисовки кадра, но ДО рендеринга следующего)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Чёрный фон
    glClear(GL_COLOR_BUFFER_BIT);
}

void Game::ResetLevel() {
    enemies.clear();
    player_bullets.clear();
    alien_bullets.clear();
    
    alien_frames_per_move = 60 - (current_level - 1) * 15;
    // player->SetPosition((width - player->GetSprite().width) / 2, 32);
    player->SetLife(3);
    alien_drop_count = 0;

    const size_t spacing_x = 16, spacing_y = 16;
    const size_t start_y = height - 150;
    const size_t total_width = (ALIENS_COLS - 1) * spacing_x + 12; // 12 for alien_c width
    const size_t start_x = (width - total_width) / 2;

    for (size_t yi = 0; yi < ALIENS_ROWS; ++yi) {
        for (size_t xi = 0; xi < ALIENS_COLS; ++xi) {
            int cur_x = start_x + xi * spacing_x;
            int cur_y = start_y - yi * spacing_y;
            AlienType type;
            Sprite sprite;
            if (yi == 0) { type = AlienType::TYPE_C; sprite = alien_c_frame1; }
            else if (yi < 3) { type = AlienType::TYPE_B; sprite = alien_b_frame1; }
            else { type = AlienType::TYPE_A; sprite = alien_a_frame1; }
            enemies.emplace_back(cur_x, cur_y, type, sprite);
        }
    }
    
    const size_t bunker_spacing = (width - GAME_NUM_BUNKERS * bunker_base_sprite_data.width) / (GAME_NUM_BUNKERS + 1);
    const size_t bunker_start_y = player->GetY() + player_sprite_data.height + 20;
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
        bunkers[i].x = bunker_spacing * (i + 1) + i * bunker_base_sprite_data.width;
        bunkers[i].y = bunker_start_y;
        bunkers[i].width = bunker_base_sprite_data.width;
        bunkers[i].height = bunker_base_sprite_data.height;
        bunkers[i].pixel_data.resize(bunker_base_sprite_data.width * bunker_base_sprite_data.height);
        for (size_t p_idx = 0; p_idx < bunker_base_sprite_data.width * bunker_base_sprite_data.height; ++p_idx) {
            if (bunker_base_sprite_data.data[p_idx] == 1) {
                bunkers[i].pixel_data[p_idx] = 5; // 5 HP
            } else {
                bunkers[i].pixel_data[p_idx] = 0;
            }
        }
    }
}

// --- Приватные хелперы ---

uint32_t Game::RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void Game::DrawSprite(const Sprite& sprite, size_t x, size_t y, uint32_t color, bool is_super) {
    if (is_super) {
        size_t laser_h = height-y; for(size_t yi=0;yi<laser_h;++yi) for(size_t xi=0;xi<sprite.width;++xi) {
            size_t sx=x+xi, sy=y+yi; if(sx<width&&sy<height) buffer.data[sy*width+sx]=color;
        }
    } else {
        for (size_t yi = 0; yi < sprite.height; ++yi) {
            for (size_t xi = 0; xi < sprite.width; ++xi) {
                size_t sx = x + xi, sy = y + yi;
                if (sprite.data[yi * sprite.width + xi] && sx < width && sy < height) {
                    buffer.data[sy * width + sx] = color;
                }
            }
        }
    }
}

void Game::DrawBunker(const Bunker& bunker, uint32_t color) {
    for (size_t yi = 0; yi < bunker.height; ++yi) {
        for (size_t xi = 0; xi < bunker.width; ++xi) {
            size_t sx = bunker.x + xi, sy = bunker.y + yi;
            if (bunker.pixel_data[yi * bunker.width + xi] > 0 && sx < width && sy < height) {
                buffer.data[sy * width + sx] = color;
            }
        }
    }
}

void Game::DrawText(const Sprite& font, const char* text, size_t x, size_t y, uint32_t color) {
    size_t xp = x;
    size_t stride = font.width * font.height;
    Sprite char_sprite = font;
    for (const char* cp = text; *cp != '\0'; ++cp) {
        char c = *cp; if (c < 32 || c > 126) continue;
        char_sprite.data = font_5x7_sprite_data.data + (c - 32) * stride;
        for (size_t yi = 0; yi < char_sprite.height; ++yi) {
            for (size_t xi = 0; xi < char_sprite.width; ++xi) {
                size_t sx = xp + xi, sy = y + yi;
                if (char_sprite.data[yi*char_sprite.width+xi] && sx<width && sy<height) {
                    buffer.data[sy * width + sx] = color;
                }
            }
        }
        xp += char_sprite.width + 1;
    }
}

void Game::DrawNumber(const Sprite& font, size_t number, size_t x, size_t y, uint32_t color) {
    char text_buffer[11];
    snprintf(text_buffer, sizeof(text_buffer), "%zu", number);
    DrawText(font, text_buffer, x, y, color);
}

bool Game::SpriteOverlapCheck(const Sprite& sp_a, size_t x_a, size_t y_a, const Sprite& sp_b, size_t x_b, size_t y_b) {
    return (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
            y_a < y_b + sp_b.height && y_a + sp_a.height > y_b);
}

bool Game::BulletBunkerOverlapCheck(const Bullet& bullet, const Bunker& bunker, size_t& hit_x, size_t& hit_y) {
    const Sprite& bullet_sprite = bullet.GetSprite();
    if (!(bullet.GetX() < bunker.x + bunker.width && bullet.GetX() + bullet_sprite.width > bunker.x &&
          bullet.GetY() < bunker.y + bunker.height && bullet.GetY() + bullet_sprite.height > bunker.y)) {
        return false;
    }
    for (size_t byi=0; byi<bullet_sprite.height; ++byi) for(size_t bxi=0; bxi<bullet_sprite.width; ++bxi) {
        if (bullet_sprite.data[byi * bullet_sprite.width + bxi]) {
            int bwx = bullet.GetX() + bxi, bwy = bullet.GetY() + byi;
            if (bwx>=bunker.x && bwx<bunker.x+bunker.width && bwy>=bunker.y && bwy<bunker.y+bunker.height) {
                size_t bpx = bwx - bunker.x, bpy = bwy - bunker.y;
                if (bunker.pixel_data[bpy * bunker.width + bpx] > 0) {
                    hit_x = bpx; hit_y = bpy;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Game::validate_program(unsigned int program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(program, len, nullptr, log.data());
        std::cerr << "Shader link error: " << log.data() << std::endl;
        return false;
    }
    return true;
}