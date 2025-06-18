#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>
#include <random>
#include <vector>
#include <iostream>
#include <algorithm> // For std::min

#define GAME_MAX_BULLETS 128
#define GAME_MAX_ALIEN_BULLETS 16
#define GAME_NUM_BUNKERS 4

// --- Data Structures ---
struct Buffer {
    size_t width, height;
    uint32_t* data;
};

struct Bullet {
    int x, y;
    int dir;
    bool is_active;
};

struct Sprite {
    size_t width, height;
    const uint8_t* data;
};

enum AlienType : uint8_t {
    ALIEN_DEAD = 0,
    ALIEN_TYPE_A = 1,
    ALIEN_TYPE_B = 2,
    ALIEN_TYPE_C = 3
};

struct Alien {
    int x, y;
    AlienType type;
    int hp;
};

struct Player {
    int x, y;
    size_t life;
};

struct Bunker {
    int x, y;
    std::vector<uint8_t> pixel_data;
    size_t width, height;
    int hp; // Added HP for bunkers
};

enum GameState {
    GAME_RUNNING,
    GAME_OVER_LOST,
    GAME_OVER_WIN
};

struct Game {
    size_t width, height;
    size_t num_aliens;
    size_t num_player_bullets;
    size_t num_alien_bullets;
    size_t score;
    Alien* aliens;
    Player player;
    Bullet player_bullets[GAME_MAX_BULLETS];
    Bullet alien_bullets[GAME_MAX_ALIEN_BULLETS];
    Bunker bunkers[GAME_NUM_BUNKERS];
    GameState state;
    int alien_move_dir;
    int alien_speed;
    size_t alien_move_timer;
    size_t alien_frames_per_move;
    size_t alien_drop_amount;
};

struct SpriteAnimation {
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    const Sprite** frames;
};

// --- Global Constants and Variables ---
const int BUFFER_WIDTH = 640;
const int BUFFER_HEIGHT = 480;
const size_t ALIENS_ROWS = 5;
const size_t ALIENS_COLS = 11;
const size_t NUM_ALIENS = ALIENS_ROWS * ALIENS_COLS;

bool game_running = true;
int move_dir = 0;
bool fire_pressed = false;

std::mt19937 rng(std::random_device{}());

// --- Functions ---
uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void buffer_clear(Buffer* buffer, uint32_t color) {
    for (size_t i = 0; i < buffer->width * buffer->height; ++i) {
        buffer->data[i] = color;
    }
}

void buffer_sprite_draw(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color) {
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

void buffer_bunker_draw(Buffer* buffer, const Bunker& bunker, uint32_t color) {
    // Only draw bunker if it has HP left
    if (bunker.hp <= 0) return;
    
    for (size_t yi = 0; yi < bunker.height; ++yi) {
        for (size_t xi = 0; xi < bunker.width; ++xi) {
            size_t sy = bunker.y + (bunker.height - 1 - yi);
            size_t sx = bunker.x + xi;

            if (bunker.pixel_data[yi * bunker.width + xi] == 0 &&
                sx < buffer->width && sy < buffer->height) {
                buffer->data[sy * buffer->width + sx] = color;
            }
        }
    }
}

bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a,
                          const Sprite& sp_b, size_t x_b, size_t y_b) {
    return (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
            y_a < y_b + sp_b.height && y_a + sp_a.height > y_b);
}

bool bullet_bunker_overlap_check(const Bullet& bullet, const Sprite& bullet_sprite,
                                 const Bunker& bunker, size_t& hit_x_in_bunker, size_t& hit_y_in_bunker) {
    if (!(bullet.x < bunker.x + bunker.width && bullet.x + bullet_sprite.width > bunker.x &&
          bullet.y < bunker.y + bunker.height && bullet.y + bullet_sprite.height > bunker.y)) {
        return false;
    }

    for (size_t byi = 0; byi < bullet_sprite.height; ++byi) {
        for (size_t bxi = 0; bxi < bullet_sprite.width; ++bxi) {
            if (bullet_sprite.data[byi * bullet_sprite.width + bxi]) {
                size_t bullet_world_x = bullet.x + bxi;
                size_t bullet_world_y = bullet.y + byi;

                if (bullet_world_x >= bunker.x && bullet_world_x < bunker.x + bunker.width &&
                    bullet_world_y >= bunker.y && bullet_world_y < bunker.y + bunker.height) {
                    size_t bunker_pixel_x = bullet_world_x - bunker.x;
                    size_t bunker_pixel_y = bunker.height - 1 - (bullet_world_y - bunker.y);

                    if (bunker.pixel_data[bunker_pixel_y * bunker.width + bunker_pixel_x] == 0) {
                        hit_x_in_bunker = bunker_pixel_x;
                        hit_y_in_bunker = bunker_pixel_y;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void buffer_draw_text(Buffer* buffer, const Sprite& font_spritesheet,
                      const char* text, size_t x, size_t y, uint32_t color) {
    size_t xp = x;
    size_t stride = font_spritesheet.width * font_spritesheet.height;
    Sprite current_char_sprite = font_spritesheet;

    for (const char* charp = text; *charp != '\0'; ++charp) {
        int character_index = *charp - 32;
        if (character_index < 0 || character_index >= 65) continue;

        current_char_sprite.data = font_spritesheet.data + character_index * stride;
        buffer_sprite_draw(buffer, current_char_sprite, xp, y, color);
        xp += font_spritesheet.width + 1;
    }
}

void buffer_draw_number(Buffer* buffer, const Sprite& font_spritesheet,
                        size_t number, size_t x, size_t y, uint32_t color) {
    uint8_t digits[64];
    size_t num_digits = 0;
    size_t current_number = number;

    do {
        digits[num_digits++] = current_number % 10;
        current_number = current_number / 10;
    } while (current_number > 0 || num_digits == 0);

    size_t xp = x;
    size_t stride = font_spritesheet.width * font_spritesheet.height;
    Sprite current_digit_sprite = font_spritesheet;

    for (size_t i = 0; i < num_digits; ++i) {
        uint8_t digit_value = digits[num_digits - i - 1];
        current_digit_sprite.data = font_spritesheet.data + (16 + digit_value) * stride;
        buffer_sprite_draw(buffer, current_digit_sprite, xp, y, color);
        xp += font_spritesheet.width + 1;
    }
}

// Fixed font data with complete English character set
const uint8_t fixed_font_data[] = {
    // Space (32)
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,

    // ! (33)
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,

    // " (34)
    0,1,0,1,0,
    0,1,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,

    // # (35)
    0,1,0,1,0,
    1,1,1,1,1,
    0,1,0,1,0,
    1,1,1,1,1,
    0,1,0,1,0,
    0,0,0,0,0,
    0,0,0,0,0,

    // $ (36)
    0,0,1,0,0,
    0,1,1,1,0,
    1,0,1,0,0,
    0,1,1,1,0,
    0,0,1,0,1,
    0,1,1,1,0,
    0,0,1,0,0,

    // % (37)
    1,1,0,0,0,
    1,1,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,1,1,
    0,0,0,1,1,

    // & (38)
    0,1,1,0,0,
    1,0,0,1,0,
    0,1,1,0,0,
    1,0,0,1,0,
    1,0,0,1,0,
    0,1,1,0,1,
    0,0,0,0,0,

    // ' (39)
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,

    // ( (40)
    0,0,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,1,0,

    // ) (41)
    0,1,0,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,0,0,0,

    // * (42)
    0,0,1,0,0,
    1,0,1,0,1,
    0,1,1,1,0,
    0,0,1,0,0,
    0,1,1,1,0,
    1,0,1,0,1,
    0,0,1,0,0,

    // + (43)
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,0,0,0,

    // , (44)
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,0,0,0,

    // - (45)
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,

    // . (46)
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,

    // / (47)
    0,0,0,0,0,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
    0,0,0,0,0,

    // 0 (48)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,1,1,
    1,0,1,0,1,
    1,1,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // 1 (49)
    0,0,1,0,0,
    0,1,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,1,1,0,

    // 2 (50)
    0,1,1,1,0,
    1,0,0,0,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,1,1,1,1,

    // 3 (51)
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,0,0,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // 4 (52)
    0,0,0,1,0,
    0,0,1,1,0,
    0,1,0,1,0,
    1,0,0,1,0,
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,0,1,0,

    // 5 (53)
    1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,0,
    0,0,0,0,1,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // 6 (54)
    0,0,1,1,0,
    0,1,0,0,0,
    1,0,0,0,0,
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // 7 (55)
    1,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    0,1,0,0,0,
    0,1,0,0,0,

    // 8 (56)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // 9 (57)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,1,1,0,0,

    // : (58)
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,

    // ; (59)
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,0,0,0,

    // < (60)
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
    0,1,0,0,0,
    0,0,1,0,0,
    0,0,0,1,0,

    // = (61)
    0,0,0,0,0,
    0,0,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    1,1,1,1,1,
    0,0,0,0,0,
    0,0,0,0,0,

    // > (62)
    0,1,0,0,0,
    0,0,1,0,0,
    0,0,0,1,0,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,

    // ? (63)
    0,1,1,1,0,
    1,0,0,0,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,0,0,0,0,
    0,0,1,0,0,

    // @ (64)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,1,1,1,
    1,0,1,0,1,
    1,0,1,1,0,
    1,0,0,0,0,
    0,1,1,1,0,

    // A (65)
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,

    // B (66)
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,

    // C (67)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,1,
    0,1,1,1,0,

    // D (68)
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,

    // E (69)
    1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,

    // F (70)
    1,1,1,1,1,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,

    // G (71)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,0,
    1,0,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // H (72)
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,

    // I (73)
    0,1,1,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,1,1,1,0,

    // J (74)
    0,0,0,0,1,
    0,0,0,0,1,
    0,0,0,0,1,
    0,0,0,0,1,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // K (75)
    1,0,0,0,1,
    1,0,0,1,0,
    1,0,1,0,0,
    1,1,0,0,0,
    1,0,1,0,0,
    1,0,0,1,0,
    1,0,0,0,1,

    // L (76)
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1,

    // M (77)
    1,0,0,0,1,
    1,1,0,1,1,
    1,0,1,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,

    // N (78)
    1,0,0,0,1,
    1,1,0,0,1,
    1,0,1,0,1,
    1,0,0,1,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,

    // O (79)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // P (80)
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,0,0,0,
    1,0,0,0,0,
    1,0,0,0,0,

    // Q (81)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,1,0,1,
    1,0,0,1,0,
    0,1,1,0,1,

    // R (82)
    1,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,1,1,1,0,
    1,0,1,0,0,
    1,0,0,1,0,
    1,0,0,0,1,

    // S (83)
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,0,
    0,1,1,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // T (84)
    1,1,1,1,1,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,

    // U (85)
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,

    // V (86)
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,1,0,1,0,
    0,0,1,0,0,

    // W (87)
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,1,0,1,
    1,0,1,0,1,
    1,1,0,1,1,
    1,0,0,0,1,

    // X (88)
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,1,0,1,0,
    1,0,0,0,1,
    1,0,0,0,1,

    // Y (89)
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,0,1,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,

    // Z (90)
    1,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
    1,1,1,1,1
};

Sprite fixed_font_sprite = {5, 7, fixed_font_data};

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

bool validate_program(GLuint program) {
    static const GLsizei BUFFER_SIZE = 512;
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;
    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
    if (length > 0) {
        printf("Program %d link error: %s\n", program, buffer);
        return false;
    }
    return true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Game* game_ptr = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game_ptr->state == GAME_RUNNING) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                if (action == GLFW_PRESS) game_running = false;
                break;
            case GLFW_KEY_RIGHT:
                if (action == GLFW_PRESS) move_dir += 1;
                else if (action == GLFW_RELEASE) move_dir -= 1;
                break;
            case GLFW_KEY_LEFT:
                if (action == GLFW_PRESS) move_dir -= 1;
                else if (action == GLFW_RELEASE) move_dir += 1;
                break;
            case GLFW_KEY_SPACE:
                if (action == GLFW_RELEASE) fire_pressed = true;
                break;
        }
    } else {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            game_running = false;
        }
    }
}

const char* vertex_shader =
    "#version 330\n"
    "noperspective out vec2 TexCoord;\n"
    "void main(void) {\n"
    "   TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0;\n"
    "   TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0;\n"
    "   gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
    "}\n";

const char* fragment_shader =
    "#version 330\n"
    "uniform sampler2D buffer;\n"
    "noperspective in vec2 TexCoord;\n"
    "out vec4 outColor;\n"
    "void main(void) {\n"
    "   outColor = texture(buffer, TexCoord);\n"
    "}\n";

int main() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(BUFFER_WIDTH, BUFFER_HEIGHT, "Space Invaders", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }

    // --- Resource Initialization ---
    Buffer buffer;
    buffer.width = BUFFER_WIDTH;
    buffer.height = BUFFER_HEIGHT;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    uint32_t clear_color = rgba_to_uint32(0, 0, 0, 255);

    // Alien sprites
    const uint8_t alien_a_frame1_data[] = {
        0,0,0,1,1,0,0,0,
        0,0,1,1,1,1,0,0,
        0,1,1,1,1,1,1,0,
        1,1,0,1,1,0,1,1,
        1,1,1,1,1,1,1,1,
        0,1,0,1,1,0,1,0,
        1,0,0,0,0,0,0,1,
        0,1,0,0,0,0,1,0
    };
    Sprite alien_a_frame1 = {8, 8, alien_a_frame1_data};

    const uint8_t alien_a_frame2_data[] = {
        0,0,0,1,1,0,0,0,
        0,0,1,1,1,1,0,0,
        0,1,1,1,1,1,1,0,
        1,1,0,1,1,0,1,1,
        1,1,1,1,1,1,1,1,
        0,0,1,0,0,1,0,0,
        0,1,0,1,1,0,1,0,
        1,0,1,0,0,1,0,1
    };
    Sprite alien_a_frame2 = {8, 8, alien_a_frame2_data};

    const uint8_t alien_b_frame1_data[] = {
        0,0,1,0,0,0,0,0,1,0,0,
        0,0,0,1,0,0,0,1,0,0,0,
        0,0,1,1,1,1,1,1,1,0,0,
        0,1,1,0,1,1,1,0,1,1,0,
        1,1,1,1,1,1,1,1,1,1,1,
        1,0,1,1,1,1,1,1,1,0,1,
        1,0,1,0,0,0,0,0,1,0,1,
        0,0,0,1,1,0,1,1,0,0,0
    };
    Sprite alien_b_frame1 = {11, 8, alien_b_frame1_data};

    const uint8_t alien_b_frame2_data[] = {
        0,0,1,0,0,0,0,0,1,0,0,
        1,0,0,1,0,0,0,1,0,0,1,
        1,0,1,1,1,1,1,1,1,0,1,
        1,1,1,0,1,1,1,0,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,
        0,1,1,1,1,1,1,1,1,1,0,
        0,0,1,0,0,0,0,0,1,0,0,
        0,1,0,0,0,0,0,0,0,1,0
    };
    Sprite alien_b_frame2 = {11, 8, alien_b_frame2_data};

    const uint8_t alien_c_frame1_data[] = {
        0,0,0,0,1,1,1,1,0,0,0,0,
        0,1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,0,0,1,1,0,0,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,
        0,0,0,1,1,0,0,1,1,0,0,0,
        0,0,1,1,0,1,1,0,1,1,0,0,
        1,1,0,0,0,0,0,0,0,0,1,1
    };
    Sprite alien_c_frame1 = {12, 8, alien_c_frame1_data};

    const uint8_t alien_c_frame2_data[] = {
        0,0,0,0,1,1,1,1,0,0,0,0,
        0,1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,0,0,1,1,0,0,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,
        0,0,1,1,1,0,0,1,1,1,0,0,
        0,1,1,0,0,1,1,0,0,1,1,0,
        0,0,1,1,0,0,0,0,1,1,0,0
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
    Sprite player_sprite = {11, 7, player_data};

    const uint8_t bullet_data[] = {1, 1, 1};
    Sprite bullet_sprite = {1, 3, bullet_data};

    const uint8_t alien_death_data[] = {
        0,1,0,0,1,0,0,0,1,0,0,1,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,1,1,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,1,0,0,1,0,0,0,1,0,0,1,0
    };
    Sprite alien_death_sprite = {13, 7, alien_death_data};

    const uint8_t heart_data[] = {
        0,1,1,0,1,1,0,
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,
        0,1,1,1,1,1,0,
        0,0,1,1,1,0,0,
        0,0,0,1,0,0,0
    };
    Sprite heart_sprite = {7, 6, heart_data};

    const uint8_t bunker_base_data[] = {
        0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
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
        1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
        1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1
    };
    Sprite bunker_base_sprite = {22, 16, bunker_base_data};

    // Alien animations
    SpriteAnimation alien_animations[4];
    const Sprite* alien_a_frames[] = {&alien_a_frame1, &alien_a_frame2};
    alien_animations[ALIEN_TYPE_A] = {true, 2, 30, 0, alien_a_frames};
    const Sprite* alien_b_frames[] = {&alien_b_frame1, &alien_b_frame2};
    alien_animations[ALIEN_TYPE_B] = {true, 2, 30, 0, alien_b_frames};
    const Sprite* alien_c_frames[] = {&alien_c_frame1, &alien_c_frame2};
    alien_animations[ALIEN_TYPE_C] = {true, 2, 30, 0, alien_c_frames};

    // --- Game Initialization ---
    Game game;
    game.width = BUFFER_WIDTH;
    game.height = BUFFER_HEIGHT;
    game.num_aliens = NUM_ALIENS;
    game.aliens = new Alien[game.num_aliens];
    game.num_player_bullets = 0;
    game.num_alien_bullets = 0;
    game.score = 0;
    game.player.x = (game.width - player_sprite.width) / 2;
    game.player.y = 32;
    game.player.life = 3;
    game.state = GAME_RUNNING;
    game.alien_move_dir = 1;
    game.alien_speed = 1;
    game.alien_move_timer = 0;
    game.alien_frames_per_move = 1;
    game.alien_drop_amount = 8;

    for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
        game.alien_bullets[i].is_active = false;
    }
    game.num_alien_bullets = 0;

    glfwSetWindowUserPointer(window, &game);
    glfwSetKeyCallback(window, key_callback);

    // Initialize aliens
    const size_t spacing_x = 16;
    const size_t spacing_y = 16;
    const size_t start_y = 150;
    const size_t total_width = (ALIENS_COLS - 1) * spacing_x + alien_c_frame1.width;
    const size_t start_x = (game.width - total_width) / 2;

    for (size_t yi = 0; yi < ALIENS_ROWS; ++yi) {
        for (size_t xi = 0; xi < ALIENS_COLS; ++xi) {
            size_t index = yi * ALIENS_COLS + xi;
            game.aliens[index].x = start_x + xi * spacing_x;
            game.aliens[index].y = start_y + yi * spacing_y;

            if (yi == 0) {
                game.aliens[index].type = ALIEN_TYPE_C;
                game.aliens[index].hp = 3;
            } else if (yi < 3) {
                game.aliens[index].type = ALIEN_TYPE_B;
                game.aliens[index].hp = 2;
            } else {
                game.aliens[index].type = ALIEN_TYPE_A;
                game.aliens[index].hp = 1;
            }
        }
    }

    // Initialize death counters
    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; ++i) {
        death_counters[i] = 0;
    }

    // Initialize bunkers with HP
    const size_t bunker_spacing = (game.width - GAME_NUM_BUNKERS * bunker_base_sprite.width) / (GAME_NUM_BUNKERS + 1);
    const size_t bunker_start_y = game.player.y + player_sprite.height + 40;
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
        game.bunkers[i].x = bunker_spacing * (i + 1) + i * bunker_base_sprite.width;
        game.bunkers[i].y = bunker_start_y;
        game.bunkers[i].width = bunker_base_sprite.width;
        game.bunkers[i].height = bunker_base_sprite.height;
        game.bunkers[i].pixel_data.assign(bunker_base_sprite.data, 
            bunker_base_sprite.data + bunker_base_sprite.width * bunker_base_sprite.height);
        game.bunkers[i].hp = 5; // 5 HP for each bunker
    }

    // --- OpenGL Setup ---
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer.width, buffer.height, 0, 
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    glUseProgram(shader_id);
    glBindVertexArray(vao);
    
    // --- Main Game Loop ---
    while (!glfwWindowShouldClose(window) && game_running) {
        buffer_clear(&buffer, clear_color);

        if (game.state == GAME_RUNNING) {
            // Player movement
            int player_speed = 3;
            if (move_dir != 0) {
                long new_x = game.player.x + (long)(player_speed * move_dir);
                if (new_x >= 0 && new_x <= game.width - player_sprite.width) {
                    game.player.x = new_x;
                }
            }
            
            // Player shooting
            if (fire_pressed && game.num_player_bullets < GAME_MAX_BULLETS) {
                bool can_fire = true;
                for (size_t i = 0; i < game.num_player_bullets; ++i) {
                    if (game.player_bullets[i].is_active) {
                        can_fire = false;
                        break;
                    }
                }

                if (can_fire) {
                    game.player_bullets[game.num_player_bullets].x = game.player.x + (player_sprite.width / 2);
                    game.player_bullets[game.num_player_bullets].y = game.player.y + player_sprite.height;
                    game.player_bullets[game.num_player_bullets].dir = 4;
                    game.player_bullets[game.num_player_bullets].is_active = true;
                    ++game.num_player_bullets;
                }
                fire_pressed = false;
            }

            // Update player bullets
            for (size_t i = 0; i < game.num_player_bullets;) {
                if (game.player_bullets[i].is_active) {
                    game.player_bullets[i].y += game.player_bullets[i].dir;
                    if (game.player_bullets[i].y >= game.height) {
                        game.player_bullets[i].is_active = false;
                    }
                }

                if (!game.player_bullets[i].is_active) {
                    game.player_bullets[i] = game.player_bullets[game.num_player_bullets - 1];
                    game.player_bullets[game.num_player_bullets - 1].is_active = false;
                    --game.num_player_bullets;
                } else {
                    ++i;
                }
            }
            
            // Alien shooting
            int active_alien_bullets = 0;
            for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                if (game.alien_bullets[i].is_active) {
                    active_alien_bullets++;
                }
            }

            std::uniform_int_distribution<size_t> alien_dist(0, game.num_aliens - 1);
            std::uniform_int_distribution<int> fire_chance(0, 100);

            for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
                if (fire_chance(rng) < 5 && game.num_alien_bullets < GAME_MAX_ALIEN_BULLETS) {
                    size_t shooter_idx = alien_dist(rng);
                    if (game.aliens[shooter_idx].type != ALIEN_DEAD) {
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

            if (active_alien_bullets < 3) {
                for (int count = 0; count < (3 - active_alien_bullets); ++count) {
                    if (fire_chance(rng) < 1) {
                        size_t shooter_idx = alien_dist(rng);
                        if (game.aliens[shooter_idx].type != ALIEN_DEAD) {
                            size_t bullet_idx_to_use = -1;
                            for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                                if (!game.alien_bullets[i].is_active) {
                                    bullet_idx_to_use = i;
                                    break;
                                }
                            }
                            if (bullet_idx_to_use != (size_t)-1) {
                                game.alien_bullets[bullet_idx_to_use].x = game.aliens[shooter_idx].x + (alien_a_frame1.width / 2);
                                game.alien_bullets[bullet_idx_to_use].y = game.aliens[shooter_idx].y;
                                game.alien_bullets[bullet_idx_to_use].dir = -2;
                                game.alien_bullets[bullet_idx_to_use].is_active = true;
                                game.num_alien_bullets++;
                            }
                        }
                    }
                }
            }
            
            // Update alien bullets
            for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                if (game.alien_bullets[i].is_active) {
                    game.alien_bullets[i].y += game.alien_bullets[i].dir;

                    if (game.alien_bullets[i].y < 0) {
                        game.alien_bullets[i].is_active = false;
                        --game.num_alien_bullets;
                    }
                }
            }

            // Update animations
            for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
                SpriteAnimation& current_anim = alien_animations[type_idx];
                ++current_anim.time;
                if (current_anim.time >= current_anim.num_frames * current_anim.frame_duration) {
                    if (current_anim.loop) current_anim.time = 0;
                }
            }

            // Update death counters
            for (size_t i = 0; i < game.num_aliens; ++i) {
                if (game.aliens[i].type == ALIEN_DEAD && death_counters[i] > 0) {
                    --death_counters[i];
                }
            }

            // Alien movement
            game.alien_move_timer++;
            if (game.alien_move_timer >= game.alien_frames_per_move) {
                game.alien_move_timer = 0;
                bool should_drop = false;

                // Boundary checking
                for (size_t i = 0; i < game.num_aliens; ++i) {
                    if (game.aliens[i].type == ALIEN_DEAD) continue;

                    const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
                    int alien_right = game.aliens[i].x + sprite.width;
                    int alien_left = game.aliens[i].x;

                    if (game.alien_move_dir == 1 && alien_right + game.alien_speed > game.width) {
                        should_drop = true;
                        break;
                    }

                    if (game.alien_move_dir == -1 && alien_left - game.alien_speed < 0) {
                        should_drop = true;
                        break;
                    }
                }

                if (should_drop) {
                    game.alien_move_dir *= -1;
                    
                    // Move aliens down
                    for (size_t i = 0; i < game.num_aliens; ++i) {
                        if (game.aliens[i].type != ALIEN_DEAD) {
                            game.aliens[i].y += game.alien_drop_amount; // Move down
                            
                            // Check if aliens reached player
                            const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
                            if (game.aliens[i].y + sprite.height >= game.player.y) {
                                game.state = GAME_OVER_LOST;
                                break;
                            }
                            
                            // Check if aliens reached bunkers
                            if (game.aliens[i].y + sprite.height >= game.bunkers[0].y) {
                                game.state = GAME_OVER_LOST;
                                break;
                            }
                        }
                    }
                }

                // Move aliens horizontally
                for (size_t i = 0; i < game.num_aliens; ++i) {
                    if (game.aliens[i].type != ALIEN_DEAD) {
                        game.aliens[i].x += game.alien_move_dir * game.alien_speed;
                        
                        // Keep within screen bounds
                        const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
                        if (game.aliens[i].x < 0) game.aliens[i].x = 0;
                        if (game.aliens[i].x + sprite.width > game.width) {
                            game.aliens[i].x = game.width - sprite.width;
                        }
                    }
                }
            }

            // Player bullet collisions with aliens
            const Sprite* current_anim_frames_for_collision[4];
            for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
                const SpriteAnimation& anim = alien_animations[type_idx];
                current_anim_frames_for_collision[type_idx] = anim.frames[anim.time / anim.frame_duration];
            }

            for (size_t bi = 0; bi < game.num_player_bullets;) {
                if (!game.player_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                for (size_t ai = 0; ai < game.num_aliens; ++ai) {
                    if (game.aliens[ai].type == ALIEN_DEAD) continue;

                    const Alien& alien = game.aliens[ai];
                    const Sprite& alien_sprite_for_collision = *current_anim_frames_for_collision[alien.type];

                    if (sprite_overlap_check(bullet_sprite, game.player_bullets[bi].x, game.player_bullets[bi].y,
                                             alien_sprite_for_collision, alien.x, alien.y)) {
                        game.aliens[ai].hp--;
                        if (game.aliens[ai].hp <= 0) {
                            game.aliens[ai].type = ALIEN_DEAD;
                            game.score += 10 * (4 - alien.type);
                            death_counters[ai] = 15;
                            game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite_for_collision.width) / 2;
                        }
                        game.player_bullets[bi].is_active = false;
                        bullet_removed = true;
                        break;
                    }
                }
                if (!bullet_removed) ++bi;
            }

            // Player bullet collisions with bunkers
            for (size_t bi = 0; bi < game.num_player_bullets;) {
                if (!game.player_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
                    if (game.bunkers[bui].hp <= 0) continue;
                    
                    size_t hit_x, hit_y;
                    if (bullet_bunker_overlap_check(game.player_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                        game.bunkers[bui].hp--;
                        game.player_bullets[bi].is_active = false;
                        bullet_removed = true;
                        break;
                    }
                }
                if (!bullet_removed) ++bi;
            }

            // Alien bullet collisions with player
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
            
            // Alien bullet collisions with bunkers
            for (size_t bi = 0; bi < GAME_MAX_ALIEN_BULLETS;) {
                if (!game.alien_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
                    if (game.bunkers[bui].hp <= 0) continue;
                    
                    size_t hit_x, hit_y;
                    if (bullet_bunker_overlap_check(game.alien_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                        game.bunkers[bui].hp--;
                        game.alien_bullets[bi].is_active = false;
                        bullet_removed = true;
                        break;
                    }
                }
                if (!bullet_removed) ++bi;
            }

            // Check for win condition
            size_t alive_aliens = 0;
            for (size_t i = 0; i < game.num_aliens; ++i) {
                if (game.aliens[i].type != ALIEN_DEAD) {
                    alive_aliens++;
                }
            }
            if (alive_aliens == 0) {
                game.state = GAME_OVER_WIN;
            }
        } // End of GAME_RUNNING block

        // --- Rendering ---
        
        // Draw aliens
        for (size_t i = 0; i < game.num_aliens; ++i) {
            const Alien& alien = game.aliens[i];
            if (alien.type != ALIEN_DEAD) {
                const SpriteAnimation& anim = alien_animations[alien.type];
                size_t current_frame_index_for_draw = anim.time / anim.frame_duration;
                const Sprite& current_alien_sprite_for_draw = *anim.frames[current_frame_index_for_draw];
                
                uint32_t color;
                switch (alien.type) {
                    case ALIEN_TYPE_A: color = rgba_to_uint32(255, 0, 0, 255); break;
                    case ALIEN_TYPE_B: color = rgba_to_uint32(0, 130, 125, 255); break;
                    case ALIEN_TYPE_C: color = rgba_to_uint32(0, 0, 255, 255); break;
                    default: color = rgba_to_uint32(255, 0, 0, 255); break;
                }
                
                buffer_sprite_draw(&buffer, current_alien_sprite_for_draw, alien.x, alien.y, color);
            } else if (death_counters[i] > 0) {
                buffer_sprite_draw(&buffer, alien_death_sprite, alien.x, alien.y, rgba_to_uint32(255, 255, 0, 255));
            }
        }
        
        // Draw bullets
        for (size_t i = 0; i < game.num_player_bullets; ++i) {
            if (game.player_bullets[i].is_active) {
                 buffer_sprite_draw(&buffer, bullet_sprite, game.player_bullets[i].x, game.player_bullets[i].y, rgba_to_uint32(255, 255, 0, 255));
            }
        }

        for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
            if (game.alien_bullets[i].is_active) {
                buffer_sprite_draw(&buffer, bullet_sprite, game.alien_bullets[i].x, game.alien_bullets[i].y, rgba_to_uint32(255, 255, 255, 255));
            }
        }
        
        // Draw player
        buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgba_to_uint32(0, 255, 0, 255));

        // Draw bunkers
        for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
            if (game.bunkers[i].hp > 0) {
                buffer_bunker_draw(&buffer, game.bunkers[i], rgba_to_uint32(0, 255, 0, 255));
            }
        }

        // Draw score
        buffer_draw_text(&buffer, fixed_font_sprite, "SCORE", 4, 
                         game.height - fixed_font_sprite.height - 7,
                         rgba_to_uint32(128, 0, 0, 255));
        buffer_draw_number(&buffer, fixed_font_sprite, game.score,
                           4 + (fixed_font_sprite.width + 1) * 6, 
                           game.height - fixed_font_sprite.height - 7,
                           rgba_to_uint32(128, 0, 0, 255));

        // Draw lives
        buffer_draw_text(&buffer, fixed_font_sprite, "LIVES",
                         game.width - (fixed_font_sprite.width + 1) * 6 - 4, 
                         game.height - fixed_font_sprite.height - 7,
                         rgba_to_uint32(128, 0, 0, 255));
        for (size_t i = 0; i < game.player.life; ++i) {
            buffer_sprite_draw(&buffer, heart_sprite,
                               game.width - 4 - (i + 1) * (heart_sprite.width + 2),
                               game.height - fixed_font_sprite.height - 7,
                               rgba_to_uint32(255, 0, 0, 255));
        }

        // Draw credit
        const char* credit_text = "CREDIT 00";
        size_t credit_text_width = (fixed_font_sprite.width + 1) * 9 - 1;
        buffer_draw_text(&buffer, fixed_font_sprite, credit_text,
                         (game.width - credit_text_width) / 2, 7,
                         rgba_to_uint32(128, 0, 0, 255));

        // Draw line above credit
        uint32_t line_color = rgba_to_uint32(128, 0, 0, 255);
        size_t line_y_from_bottom = 16;
        for (size_t i = 0; i < game.width; ++i){    
            size_t actual_buffer_y = buffer.height - 1 - line_y_from_bottom;
            if (actual_buffer_y < buffer.height) {
                buffer.data[actual_buffer_y * game.width + i] = line_color;
            }
        }

        // Game over/win screens
        if (game.state == GAME_OVER_LOST) {
            const char* game_over_text = "GAME OVER";
            size_t text_width = (fixed_font_sprite.width + 1) * 9 - 1;
            buffer_draw_text(&buffer, fixed_font_sprite, game_over_text,
                             (game.width - text_width) / 2, game.height / 2,
                             rgba_to_uint32(255, 255, 255, 255));
            
            const char* score_text = "SCORE: ";
            size_t score_text_width = (fixed_font_sprite.width + 1) * 7 - 1;
            buffer_draw_text(&buffer, fixed_font_sprite, score_text,
                             (game.width - score_text_width) / 2, game.height / 2 - 20,
                             rgba_to_uint32(255, 255, 255, 255));
            buffer_draw_number(&buffer, fixed_font_sprite, game.score,
                               (game.width - score_text_width) / 2 + score_text_width, 
                               game.height / 2 - 20,
                               rgba_to_uint32(255, 255, 255, 255));
        } else if (game.state == GAME_OVER_WIN) {
            const char* win_text = "YOU WIN!";
            size_t text_width = (fixed_font_sprite.width + 1) * 8 - 1;
            buffer_draw_text(&buffer, fixed_font_sprite, win_text,
                             (game.width - text_width) / 2, game.height / 2,
                             rgba_to_uint32(0, 255, 0, 255));
            
            const char* score_text = "SCORE: ";
            size_t score_text_width = (fixed_font_sprite.width + 1) * 7 - 1;
            buffer_draw_text(&buffer, fixed_font_sprite, score_text,
                             (game.width - score_text_width) / 2, game.height / 2 - 20,
                             rgba_to_uint32(0, 255, 0, 255));
            buffer_draw_number(&buffer, fixed_font_sprite, game.score,
                               (game.width - score_text_width) / 2 + score_text_width, 
                               game.height / 2 - 20,
                               rgba_to_uint32(0, 255, 0, 255));
        }

        // --- Display ---
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, 
                        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
    delete[] buffer.data;
    delete[] game.aliens;
    delete[] death_counters;

    glDeleteTextures(1, &buffer_texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_id);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
// тут важные изменения !!! тут исправлены слова