#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>
#include <random> // Для генерации случайных чисел
#include <vector> // Для разрушаемых укрытий (хранение пикселей)
#include <iostream>


#define GAME_MAX_BULLETS 128
#define GAME_MAX_ALIEN_BULLETS 16 // Максимальное количество пуль от пришельцев
#define GAME_NUM_BUNKERS 4 // Количество разрушаемых укрытий

// --- Структуры данных ---

struct Buffer {
    size_t width, height;
    uint32_t* data;
};

struct Bullet {
    int x, y;
    int dir;
    bool is_active; // Флаг активности пули
};

struct Sprite {
    size_t width, height;
    const uint8_t* data;
};

// типы пришельцев
enum AlienType : uint8_t {
    ALIEN_DEAD = 0,
    ALIEN_TYPE_A = 1,
    ALIEN_TYPE_B = 2,
    ALIEN_TYPE_C = 3
};

struct Alien {
    int x, y;
    AlienType type;
    int hp; // Добавляем очки здоровья для пришельцев
};

struct Player {
    int x, y;
    size_t life;
};

struct Bunker {
    int x, y;
    // Используем динамический массив для хранения состояния пикселей бункера
    // 0 - пиксель цел, 1 - пиксель разрушен
    std::vector<uint8_t> pixel_data;
    size_t width, height;
};

enum GameState {
    GAME_RUNNING,
    GAME_OVER_LOST,
    GAME_OVER_WIN
};

struct Game {
    size_t width, height;
    size_t num_aliens;
    size_t num_player_bullets; // Переименовано для ясности
    size_t num_alien_bullets;  // Новое поле для пуль пришельцев
    size_t score;
    Alien* aliens;
    Player player;
    Bullet player_bullets[GAME_MAX_BULLETS];
    Bullet alien_bullets[GAME_MAX_ALIEN_BULLETS]; // Массив для пуль пришельцев
    Bunker bunkers[GAME_NUM_BUNKERS]; // Массив укрытий
    GameState state; // Состояние игры
    int alien_move_dir; // 1 для вправо, -1 для влево
    int alien_speed;
    size_t alien_move_timer;
    size_t alien_frames_per_move; // Сколько кадров до следующего движения
    size_t alien_drop_amount; // Насколько пришельцы опускаются вниз
};

struct SpriteAnimation {
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    const Sprite** frames;
};

// --- Глобальные переменные и константы ---

const int BUFFER_WIDTH = 640;
const int BUFFER_HEIGHT = 480;
const size_t ALIENS_ROWS = 5;
const size_t ALIENS_COLS = 11;
const size_t NUM_ALIENS = ALIENS_ROWS * ALIENS_COLS;

// Глобальные переменные для управления
bool game_running = true; // Теперь управляется через game.state
int move_dir = 0;
bool fire_pressed = false;

// Генератор случайных чисел
std::mt19937 rng(std::random_device{}());

// --- Функции ---

uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void buffer_clear(Buffer* buffer, uint32_t color) {
    for (size_t i = 0; i < buffer->width * buffer->height; ++i) {
        buffer->data[i] = color;
    }
}

void buffer_sprite_draw(
    Buffer* buffer, const Sprite& sprite,
    size_t x, size_t y, uint32_t color
) {
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

// Новая функция для отрисовки разрушаемого бункера
void buffer_bunker_draw(Buffer* buffer, const Bunker& bunker, uint32_t color) {
    for (size_t yi = 0; yi < bunker.height; ++yi) {
        for (size_t xi = 0; xi < bunker.width; ++xi) {
            size_t sy = bunker.y + (bunker.height - 1 - yi);
            size_t sx = bunker.x + xi;

            // Рисуем пиксель, только если он не разрушен (0 в pixel_data)
            if (bunker.pixel_data[yi * bunker.width + xi] == 0 &&
                sx < buffer->width && sy < buffer->height) {
                buffer->data[sy * buffer->width + sx] = color;
            }
        }
    }
}

bool sprite_overlap_check(
    const Sprite& sp_a, size_t x_a, size_t y_a,
    const Sprite& sp_b, size_t x_b, size_t y_b
) {
    return (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
            y_a < y_b + sp_b.height && y_a + sp_a.height > y_b);
}

// Перегруженная функция для проверки пересечения пули с бункером
bool bullet_bunker_overlap_check(
    const Bullet& bullet, const Sprite& bullet_sprite,
    const Bunker& bunker, size_t& hit_x_in_bunker, size_t& hit_y_in_bunker
) {
    // Проверяем общее прямоугольное пересечение сначала
    if (!(bullet.x < bunker.x + bunker.width && bullet.x + bullet_sprite.width > bunker.x &&
          bullet.y < bunker.y + bunker.height && bullet.y + bullet_sprite.height > bunker.y)) {
        return false;
    }

    // Если есть пересечение прямоугольников, проверяем пиксели
    // Итерируемся по пикселям пули
    for (size_t byi = 0; byi < bullet_sprite.height; ++byi) {
        for (size_t bxi = 0; bxi < bullet_sprite.width; ++bxi) {
            if (bullet_sprite.data[byi * bullet_sprite.width + bxi]) { // Если пиксель пули активен
                size_t bullet_world_x = bullet.x + bxi;
                size_t bullet_world_y = bullet.y + byi;

                // Проверяем, находится ли пиксель пули внутри бункера
                if (bullet_world_x >= bunker.x && bullet_world_x < bunker.x + bunker.width &&
                    bullet_world_y >= bunker.y && bullet_world_y < bunker.y + bunker.height) {

                    // Переводим мировые координаты в координаты пикселей бункера
                    size_t bunker_pixel_x = bullet_world_x - bunker.x;
                    size_t bunker_pixel_y = bunker.height - 1 - (bullet_world_y - bunker.y); // Переворачиваем Y

                    // Проверяем, цел ли пиксель бункера в этой точке
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

void buffer_draw_text(
    Buffer* buffer,
    const Sprite& font_spritesheet,
    const char* text,
    size_t x, size_t y,
    uint32_t color
) {
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

void buffer_draw_number(
    Buffer* buffer,
    const Sprite& font_spritesheet,
    size_t number,
    size_t x, size_t y,
    uint32_t color
) {
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
    // Теперь состояние игры проверяется здесь для обработки ввода
    Game* game_ptr = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game_ptr->state == GAME_RUNNING) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                if (action == GLFW_PRESS) game_running = false; // Позволяет выйти из игры
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
    } else { // Если игра не запущена (GAME_OVER)
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            game_running = false; // Позволяет выйти из игры
        }
        // Можно добавить логику для перезапуска игры по другой кнопке
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

    // --- Инициализация ресурсов ---

    Buffer buffer;
    buffer.width = BUFFER_WIDTH;
    buffer.height = BUFFER_HEIGHT;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    uint32_t clear_color = rgba_to_uint32(0, 0, 0, 255);


    std::cout << "initiialization\n";
  //  std::cin.get(); // Ждет нажатия клавиши
    
    

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

    // Спрайт сердца для индикации здоровья
    const uint8_t heart_data[] = {
        0,1,1,0,1,1,0,
        1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,
        0,1,1,1,1,1,0,
        0,0,1,1,1,0,0,
        0,0,0,1,0,0,0
    };
    Sprite heart_sprite = {7, 6, heart_data}; // Размеры сердца

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
    Sprite bunker_base_sprite = {22, 16, bunker_base_data}; // Размеры базового спрайта бункера

    // --- Анимации для каждого типа пришельцев ---
    // Массив анимаций, индексируемый по AlienType
    SpriteAnimation alien_animations[4];

    // Анимация для ALIEN_TYPE_A
    const Sprite* alien_a_frames[] = {&alien_a_frame1, &alien_a_frame2};
    alien_animations[ALIEN_TYPE_A] = {true, 2, 30, 0, alien_a_frames}; // loop, num_frames, frame_duration, time, frames

    // Анимация для ALIEN_TYPE_B
    const Sprite* alien_b_frames[] = {&alien_b_frame1, &alien_b_frame2};
    alien_animations[ALIEN_TYPE_B] = {true, 2, 30, 0, alien_b_frames};

    // Анимация для ALIEN_TYPE_C
    const Sprite* alien_c_frames[] = {&alien_c_frame1, &alien_c_frame2};
    alien_animations[ALIEN_TYPE_C] = {true, 2, 30, 0, alien_c_frames};

    // --- Спрайт-лист для шрифта (5x7 пикселей на символ) ---
    // Начинается с ASCII 32 (' ') и заканчивается ASCII 96 ('`'). Всего 65 символов.
    // Важно: данные должны соответствовать порядку ASCII символов от 32 до 96.
    const uint8_t font_5x7_data[] = { // здесь находится все рабочие спрайты ASCII от 32 до 96
        0,0,0,0,0, // Space (32)
        0,0,1,0,0, // ! (33)
        0,1,0,1,0, // " (34)
        0,1,1,1,0, // # (35)
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,0,0,0,
        0,1,0,1,0, // $ (36)
        1,1,1,1,1,
        1,1,1,1,1,
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,0,0,0,
        0,0,1,0,0, // % (37)
        0,1,0,1,0,
        0,1,0,0,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        1,0,0,0,1, // & (38)
        1,0,1,0,0,
        0,1,0,1,0,
        1,0,1,0,1,
        0,1,0,1,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0, // ' (39)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,1,0, // ( (40)
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,0,0, // ) (41)
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,0,0,0,0, // * (42)
        0,1,0,1,0,
        0,0,1,0,0,
        0,1,1,1,0,
        0,0,1,0,0,
        0,1,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0, // + (43)
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,1,1,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // , (44)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0, // - (45)
        0,0,0,0,0,
        0,0,0,0,0,
        0,1,1,1,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // . (46)
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // / (47)
        0,0,0,0,1,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        1,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,1,1,1,0, // 0 (48)
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,1,0,0, // 1 (49)
        0,1,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,1,1,1,0,
        0,1,1,1,0, // 2 (50)
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,0,1,1,0,
        0,1,1,1,0,
        0,1,1,1,0, // 3 (51)
        0,0,0,1,0,
        0,0,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,1,0,
        0,1,0,1,0, // 4 (52)
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,1,0, // 5 (53)
        0,1,0,0,0,
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,1,1,1,0, // 6 (54)
        0,1,0,0,0,
        0,1,1,1,0,
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,1,1,1,0, // 7 (55)
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,0,0,0,
        0,1,1,1,0, // 8 (56)
        0,1,0,1,0,
        0,1,1,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,1,1,1,0, // 9 (57)
        0,1,0,1,0,
        0,1,0,1,0,
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,0,1,0,
        0,1,1,1,0,
        0,0,0,0,0, // : (58)
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // ; (59)
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0, // < (60)
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,0,0,0,
        0,0,0,0,0, // = (61)
        0,0,0,0,0,
        0,1,1,1,0,
        0,0,0,0,0,
        0,1,1,1,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // > (62)
        0,1,0,0,0,
        0,0,1,0,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // ? (63)
        0,1,1,1,0,
        0,0,0,1,0,
        0,0,1,0,0,
        0,1,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0,
        0,0,0,0,0, // @ (64)
        0,1,1,1,0,
        1,0,0,1,0,
        1,1,1,1,0,
        1,0,1,0,0,
        1,1,1,1,0,
        0,0,0,0,0,
    };
    Sprite font_5x7_sprite = {5, 7, font_5x7_data};

     std::cout << "sprites\n";

    // --- Инициализация Игры ---
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
    game.player.life = 3; // 3 жизни
    game.state = GAME_RUNNING;
    game.alien_move_dir = 1; // Начинают двигаться вправо
    game.alien_speed = 1; // Скорость движения пришельцев
    game.alien_move_timer = 0;
    game.alien_frames_per_move = 1; // Движение раз в 20 кадров (1 секунда при 60 FPS)
    game.alien_drop_amount = 8; // Опускаются на 8 пикселей

     std::cout << "initiialization Game\n";

    // --- Инициализация пуль пришельцев в начале main() ---
    for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
        game.alien_bullets[i].is_active = false;
    }
    game.num_alien_bullets = 0;

    // Привязываем указатель на объект game к окну GLFW
    glfwSetWindowUserPointer(window, &game);
    glfwSetKeyCallback(window, key_callback);


    // Инициализация роя пришельцев
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
                game.aliens[index].hp = 3; // Type C - 3 HP
            } else if (yi < 3) {
                game.aliens[index].type = ALIEN_TYPE_B;
                game.aliens[index].hp = 2; // Type B - 2 HP
            } else {
                game.aliens[index].type = ALIEN_TYPE_A;
                game.aliens[index].hp = 1; // Type A - 1 HP
            }
        }
    }
    
     std::cout << "initiialization group of aliens\n";

    // Инициализация таймеров смерти
    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; ++i) {
        death_counters[i] = 0;
    }

    // Инициализация укрытий
    const size_t bunker_spacing = (game.width - GAME_NUM_BUNKERS * bunker_base_sprite.width) / (GAME_NUM_BUNKERS + 1);
    const size_t bunker_start_y = game.player.y + player_sprite.height + 40; // Чуть выше игрока
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
        game.bunkers[i].x = bunker_spacing * (i + 1) + i * bunker_base_sprite.width;
        game.bunkers[i].y = bunker_start_y;
        game.bunkers[i].width = bunker_base_sprite.width;
        game.bunkers[i].height = bunker_base_sprite.height;
        // Копируем данные базового спрайта в pixel_data каждого бункера
        game.bunkers[i].pixel_data.assign(bunker_base_sprite.data, bunker_base_sprite.data + bunker_base_sprite.width * bunker_base_sprite.height);
    }

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

     std::cout << "Customization Open GL\n";
    
    // --- Главный игровой цикл ---
    while (!glfwWindowShouldClose(window) && game_running) {
        buffer_clear(&buffer, clear_color);

        // --- Обновление состояния игры ---
        if (game.state == GAME_RUNNING) {
            // Движение игрока
            int player_speed = 3;
            if (move_dir != 0) {
                long new_x = game.player.x + (long)(player_speed * move_dir);
                if (new_x >= 0 && new_x <= game.width - player_sprite.width) {
                    game.player.x = new_x;
                }
            }
            
            // Стрельба игрока
            if (fire_pressed && game.num_player_bullets < GAME_MAX_BULLETS) {
                // Проверяем, есть ли уже активная пуля (для ограничения "одна пуля на экране")
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
                    game.player_bullets[game.num_player_bullets].dir = 4; // Скорость пули (вверх)
                    game.player_bullets[game.num_player_bullets].is_active = true;
                    ++game.num_player_bullets;
                }
                fire_pressed = false;
            }

            // Обновление пуль игрока
            for (size_t i = 0; i < game.num_player_bullets;) {
                if (game.player_bullets[i].is_active) {
                    game.player_bullets[i].y += game.player_bullets[i].dir;
                    if (game.player_bullets[i].y >= game.height) {
                        game.player_bullets[i].is_active = false;
                    }
                }

                if (!game.player_bullets[i].is_active) {
                    game.player_bullets[i] = game.player_bullets[game.num_player_bullets - 1];
                    game.player_bullets[game.num_player_bullets - 1].is_active = false; // Убедиться, что последняя пуля неактивна после перемещения
                    --game.num_player_bullets;
                } else {
                    ++i;
                }
            }
             std::cout << "main game loop cycle bullets of gamer\n";
            
            // --- Подсчёт активных пуль пришельцев ---
            int active_alien_bullets = 0;
            for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
                if (game.alien_bullets[i].is_active) {
                    active_alien_bullets++;
                }
            }
             // Стрельба пришельцев
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
            
            // обновление пуль пришельцев
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
            // --- Движение пришельцев и проверка границ ---

            game.alien_move_timer++;
            if (game.alien_move_timer >= game.alien_frames_per_move) {
                game.alien_move_timer = 0;
                bool should_drop = false;

                // Проверка границ с новыми условиями
                for (size_t i = 0; i < game.num_aliens; ++i) {
                    if (game.aliens[i].type == ALIEN_DEAD) continue;

                    const Sprite& sprite = *alien_animations[game.aliens[i].type].frames[0];
                    int alien_right = game.aliens[i].x + sprite.width;
                    int alien_left = game.aliens[i].x;

                    // Проверка правой границы
                    if (game.alien_move_dir == 1 && alien_right + game.alien_speed > game.width) {
                        should_drop = true;
                        break;
                    }
                    // Проверка левой границы
                    if (game.alien_move_dir == -1 && alien_left - game.alien_speed < 0) {
                        should_drop = true;
                        break;
                    }
                }

                if (should_drop) {
                    game.alien_move_dir *= -1; // Меняем направление
                    
                    // Опускаем всех пришельцев вниз
                    for (size_t i = 0; i < game.num_aliens; ++i) {
                        if (game.aliens[i].type != ALIEN_DEAD) {
                            game.aliens[i].y -= game.alien_drop_amount;
                            
                            // Проверка поражения (пришельцы достигли игрока)
                            if (game.aliens[i].y <= game.player.y + player_sprite.height) {
                                game.state = GAME_OVER_LOST;
                                break;
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

            // Обработка столкновений пули игрока с пришельцами
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
                        game.aliens[ai].hp--; // Уменьшаем HP пришельца
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
            std::cout << "main game loop cycle bullet of gamer into alien\n";


            // Обработка столкновений пуль игрока с укрытиями
            for (size_t bi = 0; bi < game.num_player_bullets;) {
                if (!game.player_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
                    size_t hit_x, hit_y;
                    if (bullet_bunker_overlap_check(game.player_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                        // Разрушаем пиксели в области столкновения
                        int radius = 20; // 20 пикселей в области столкновения
                        for (int y_offset = -radius / 2; y_offset < radius / 2; ++y_offset) {
                            for (int x_offset = -radius / 2; x_offset < radius / 2; ++x_offset) {
                                int current_x = hit_x + x_offset;
                                int current_y = hit_y + y_offset;

                                if (current_x >= 0 && current_x < game.bunkers[bui].width &&
                                    current_y >= 0 && current_y < game.bunkers[bui].height) {
                                    game.bunkers[bui].pixel_data[current_y * game.bunkers[bui].width + current_x] = 1; // 1 - разрушено
                                }
                            }
                        }
                        game.player_bullets[bi].is_active = false;
                        bullet_removed = true;
                        break;
                    }
                }
                if (!bullet_removed) ++bi;
            }
            std::cout << "main game loop cycle update bullet of gamer into bunker\n";

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
            
            // Обработка столкновений пуль пришельцев с укрытиями
            for (size_t bi = 0; bi < GAME_MAX_ALIEN_BULLETS;) {
                if (!game.alien_bullets[bi].is_active) {
                    ++bi;
                    continue;
                }

                bool bullet_removed = false;
                for (size_t bui = 0; bui < GAME_NUM_BUNKERS; ++bui) {
                    size_t hit_x, hit_y;
                    if (bullet_bunker_overlap_check(game.alien_bullets[bi], bullet_sprite, game.bunkers[bui], hit_x, hit_y)) {
                        int radius = 20;
                        for (int y_offset = -radius / 2; y_offset < radius / 2; ++y_offset) {
                            for (int x_offset = -radius / 2; x_offset < radius / 2; ++x_offset) {
                                int current_x = hit_x + x_offset;
                                int current_y = hit_y + y_offset;

                                if (current_x >= 0 && current_x < game.bunkers[bui].width &&
                                    current_y >= 0 && current_y < game.bunkers[bui].height) {
                                    game.bunkers[bui].pixel_data[current_y * game.bunkers[bui].width + current_x] = 1;
                                }
                            }
                        }
                        game.alien_bullets[bi].is_active = false;
                        bullet_removed = true;
                        break;
                    }
                }
                if (!bullet_removed) ++bi;
            }

            // Проверка на победу (все пришельцы уничтожены)
            size_t alive_aliens = 0;
            for (size_t i = 0; i < game.num_aliens; ++i) {
                if (game.aliens[i].type != ALIEN_DEAD) {
                    alive_aliens++;
                }
            }
            if (alive_aliens == 0) {
                game.state = GAME_OVER_WIN;
            }
        } // Конец блока game.state == GAME_RUNNING

        // --- Отрисовка ---
        
        // Рисуем пришельцев
        // В функции отрисовки пришельцев
        for (size_t i = 0; i < game.num_aliens; ++i) {
            const Alien& alien = game.aliens[i];
            if (alien.type != ALIEN_DEAD) {
                const SpriteAnimation& anim = alien_animations[alien.type];
                size_t current_frame_index_for_draw = anim.time / anim.frame_duration;
                const Sprite& current_alien_sprite_for_draw = *anim.frames[current_frame_index_for_draw];
                
                // Цвет зависит от типа пришельца
                uint32_t color;
                switch (alien.type) {
                    case ALIEN_TYPE_A: color = rgba_to_uint32(255, 0, 0, 255); break; // Красный
                    case ALIEN_TYPE_B: color = rgba_to_uint32(0, 130, 125, 255); break; // фиол? 
                    case ALIEN_TYPE_C: color = rgba_to_uint32(0, 0, 255, 255); break; // Синий
                    default: color = rgba_to_uint32(255, 0, 0, 255); break;
                }
                
                buffer_sprite_draw(&buffer, current_alien_sprite_for_draw, alien.x, alien.y, color);
            } else if (death_counters[i] > 0) {
                buffer_sprite_draw(&buffer, alien_death_sprite, alien.x, alien.y, rgba_to_uint32(255, 255, 0, 255));
            }
        }
        
        // Рисуем пули игрока
        for (size_t i = 0; i < game.num_player_bullets; ++i) {
            if (game.player_bullets[i].is_active) {
                 buffer_sprite_draw(&buffer, bullet_sprite, game.player_bullets[i].x, game.player_bullets[i].y, rgba_to_uint32(255, 255, 0, 255));
            }
        }

        // Рисуем пули пришельцев
        for (size_t i = 0; i < GAME_MAX_ALIEN_BULLETS; ++i) {
            if (game.alien_bullets[i].is_active) {
                buffer_sprite_draw(&buffer, bullet_sprite, game.alien_bullets[i].x, game.alien_bullets[i].y, rgba_to_uint32(255, 255, 255, 255)); // Белые пули
            }
        }
        
        // Рисуем игрока
        buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgba_to_uint32(0, 255, 0, 255));

        // Рисуем укрытия
        for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i) {
            buffer_bunker_draw(&buffer, game.bunkers[i], rgba_to_uint32(0, 255, 0, 255)); // Зеленые укрытия
        }

        // Рисуем счет
        buffer_draw_text(
            &buffer,
            font_5x7_sprite, "SCORE",
            4, game.height - font_5x7_sprite.height - 7,
            rgba_to_uint32(128, 0, 0, 255)
        );

        buffer_draw_number(
            &buffer,
            font_5x7_sprite, game.score,
            4 + (font_5x7_sprite.width + 1) * 6, game.height - font_5x7_sprite.height - 7,
            rgba_to_uint32(128, 0, 0, 255)
        );

        // Рисуем жизни игрока в виде сердец
        buffer_draw_text(
            &buffer,
            font_5x7_sprite, "LIVES",
            game.width - (font_5x7_sprite.width + 1) * 6 - 4, game.height - font_5x7_sprite.height - 7,
            rgba_to_uint32(128, 0, 0, 255)
        );
        for (size_t i = 0; i < game.player.life; ++i) {
            buffer_sprite_draw(
                &buffer, heart_sprite,
                game.width - 4 - (i + 1) * (heart_sprite.width + 2), // Располагаем сердца справа налево
                game.height - font_5x7_sprite.height - 7,
                rgba_to_uint32(255, 0, 0, 255) // Красные сердца
            );
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

        // Экран Game Over / Win
        if (game.state == GAME_OVER_LOST) {
            const char* game_over_text = "GAME OVER";
            size_t text_width = (font_5x7_sprite.width + 1) * (std::string(game_over_text).length()) - 1;
            buffer_draw_text(
                &buffer, font_5x7_sprite, game_over_text,
                (game.width - text_width) / 2, game.height / 2,
                rgba_to_uint32(255, 255, 255, 255) // Белый цвет
            );
        } else if (game.state == GAME_OVER_WIN) {
            const char* win_text = "YOU WIN!";
            size_t text_width = (font_5x7_sprite.width + 1) * (std::string(win_text).length()) - 1;
            buffer_draw_text(
                &buffer, font_5x7_sprite, win_text,
                (game.width - text_width) / 2, game.height / 2,
                rgba_to_uint32(0, 255, 0, 255) // Зеленый цвет
            );
        }

        // --- Вывод на экран ---
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Очистка ресурсов ---
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