#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <string> // Для использования std::string, хотя в данном случае не обязательно

#define GAME_MAX_BULLETS 128

// --- Структуры данных ---

struct Buffer {
    size_t width, height;
    uint32_t* data;
};

struct Bullet {
    size_t x, y;
    int dir;
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
    size_t x, y;
    AlienType type;
};

struct Player {
    size_t x, y;
    size_t life;
};

struct Game {
    size_t width, height;
    size_t num_aliens;
    size_t num_bullets;
    size_t score; // Переменная для хранения счета
    Alien* aliens;
    Player player;
    Bullet bullets[GAME_MAX_BULLETS];
};

struct SpriteAnimation {
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    const Sprite** frames; // Указатель на массив указателей на спрайты
};

// --- Глобальные переменные и константы ---

const int BUFFER_WIDTH = 640;
const int BUFFER_HEIGHT = 480;
const size_t ALIENS_ROWS = 5;
const size_t ALIENS_COLS = 11;
const size_t NUM_ALIENS = ALIENS_ROWS * ALIENS_COLS;

// Глобальные переменные для управления
bool game_running = true;
int move_dir = 0;
bool fire_pressed = false;

// --- Функции ---

// Преобразование RGBA в 32-битный цвет (исправлено для little-endian систем)
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
            // Переворачиваем спрайт по Y, чтобы он рисовался правильно (y - это нижняя граница спрайта)
            size_t sy = y + (sprite.height - 1 - yi);
            size_t sx = x + xi;

            if (sprite.data[yi * sprite.width + xi] &&
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

// Функция для отрисовки текста по спрайт-листу шрифта
// @buffer: буфер, куда будет производиться отрисовка
// @font_spritesheet: спрайт-лист шрифта, где каждый спрайт - это символ
// @text: строка для отрисовки
// @x, @y: координаты начала отрисовки текста
// @color: цвет текста
void buffer_draw_text(
    Buffer* buffer,
    const Sprite& font_spritesheet, // Теперь более общее название для спрайт-листа шрифта
    const char* text,
    size_t x, size_t y,
    uint32_t color
) {
    size_t xp = x; // Текущая позиция X для отрисовки символа
    // Вычисляем "шаг" (stride) - размер данных одного символа в спрайт-листе.
    // Это необходимо, чтобы "перепрыгивать" между символами в одном большом массиве data.
    size_t stride = font_spritesheet.width * font_spritesheet.height; 
    
    // Создаем временный спрайт, который будет "указывать" на текущий символ в font_spritesheet
    Sprite current_char_sprite = font_spritesheet; 

    // Итерируемся по каждому символу в строке 'text'
    for (const char* charp = text; *charp != '\0'; ++charp) // Пока не встретим нулевой символ ('\0'), который обозначает конец строки
    {
        // Вычисляем индекс символа в спрайт-листе.
        // ASCII-значение 'пробела' (первого символа в нашем спрайт-листе) равно 32.
        // Вычитая 32, мы получаем 0 для пробела, 1 для '!', и так далее.
        int character_index = *charp - 32; 

        // Проверяем, находится ли индекс в допустимом диапазоне (0 до 64, так как у нас 65 символов)
        if (character_index < 0 || character_index >= 65) continue; // Пропускаем неизвестные символы

        // Обновляем указатель на данные для текущего символа в спрайт-листе.
        // Мы смещаемся на character_index * stride байт от начала данных спрайт-листа.
        current_char_sprite.data = font_spritesheet.data + character_index * stride;
        
        // Отрисовываем текущий символ
        buffer_sprite_draw(buffer, current_char_sprite, xp, y, color);
        
        // Смещаем позицию X для следующего символа.
        // Добавляем 1 пиксель для небольшого зазора между символами.
        xp += font_spritesheet.width + 1;
    }
}

// Функция для отрисовки чисел
// @buffer: буфер, куда будет производиться отрисовка
// @font_spritesheet: спрайт-лист шрифта (он же содержит цифры)
// @number: число для отрисовки
// @x, @y: координаты начала отрисовки числа
// @color: цвет числа
void buffer_draw_number(
    Buffer* buffer,
    const Sprite& font_spritesheet, 
    size_t number,
    size_t x, size_t y,
    uint32_t color
) {
    // Временный массив для хранения цифр. 64 достаточно для очень больших чисел.
    uint8_t digits[64]; 
    size_t num_digits = 0; // Количество цифр в числе
    size_t current_number = number; // Копия числа для манипуляций

    // Извлекаем цифры из числа, начиная с последней, и сохраняем их в массив.
    do {
        digits[num_digits++] = current_number % 10;
        current_number = current_number / 10;
    } while (current_number > 0 || num_digits == 0); // num_digits == 0 для случая, когда number = 0

    size_t xp = x; // Текущая позиция X для отрисовки цифры
    // "Шаг" для цифр. Символ '0' (ASCII 48) соответствует индексу 16 в нашем спрайт-листе.
    // (48 - 32 = 16)
    size_t stride = font_spritesheet.width * font_spritesheet.height; 
    Sprite current_digit_sprite = font_spritesheet; // Временный спрайт для текущей цифры

    // Отрисовываем цифры в обратном порядке, чтобы число читалось слева направо.
    for (size_t i = 0; i < num_digits; ++i) {
        uint8_t digit_value = digits[num_digits - i - 1]; // Получаем цифру
        // Смещаемся к нужной цифре в спрайт-листе.
        // Символ '0' (ASCII 48) соответствует индексу 16 в нашем спрайт-листе.
        // Поэтому для цифры `digit_value` (0-9) мы используем индекс `16 + digit_value`.
        current_digit_sprite.data = font_spritesheet.data + (16 + digit_value) * stride;
        
        buffer_sprite_draw(buffer, current_digit_sprite, xp, y, color);
        xp += font_spritesheet.width + 1; // Смещаем позицию X
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
}

const char* vertex_shader =
    "#version 330\n"
    "noperspective out vec2 TexCoord;\n"
    "void main(void) {\n"
    "    TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0;\n"
    "    TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0;\n"
    "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
    "}\n";

const char* fragment_shader =
    "#version 330\n"
    "uniform sampler2D buffer;\n"
    "noperspective in vec2 TexCoord;\n"
    "out vec4 outColor;\n"
    "void main(void) {\n"
    "    outColor = texture(buffer, TexCoord);\n"
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
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }

    // --- Инициализация ресурсов ---

    Buffer buffer;
    buffer.width = BUFFER_WIDTH;
    buffer.height = BUFFER_HEIGHT;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    uint32_t clear_color = rgba_to_uint32(0, 0, 0, 255);

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

    const uint8_t death_data[] = {
        0,1,0,0,1,0,0,0,1,0,0,1,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        1,1,0,0,0,0,0,0,0,0,0,1,1,
        0,0,0,1,0,0,0,0,0,1,0,0,0,
        0,0,1,0,0,1,0,1,0,0,1,0,0,
        0,1,0,0,1,0,0,0,1,0,0,1,0
    };
    Sprite alien_death_sprite = {13, 7, death_data};

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

    
    // ПОСЛЕ ФУНКЦИЙ НА АНИМАЦИЮ РАЗНЫХ ТИПОВ ИНОПЛАНЕТЯН

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
    // перед ИНИЦИАЛИЗАЦИЕЙ ИГРЫ


    // --- Инициализация Игры ---
    Game game;
    game.width = BUFFER_WIDTH;
    game.height = BUFFER_HEIGHT;
    game.num_aliens = NUM_ALIENS;
    game.aliens = new Alien[game.num_aliens];
    game.num_bullets = 0;
    game.score = 0; // Инициализация счета

    game.player.x = (game.width - player_sprite.width) / 2;
    game.player.y = 32;
    game.player.life = 3;
    
    // Инициализация роя пришельцев
    const size_t spacing_x = 16;
    const size_t spacing_y = 16;
    const size_t start_y = 150;
    // Используем ширину спрайта ALIEN_TYPE_C для расчета общей ширины,
    // так как это самый широкий спрайт из новых (12px)
    const size_t total_width = (ALIENS_COLS - 1) * spacing_x + alien_c_frame1.width; 
    const size_t start_x = (game.width - total_width) / 2;

    for (size_t yi = 0; yi < ALIENS_ROWS; ++yi) {
        for (size_t xi = 0; xi < ALIENS_COLS; ++xi) {
            size_t index = yi * ALIENS_COLS + xi;
            game.aliens[index].x = start_x + xi * spacing_x;
            game.aliens[index].y = start_y + yi * spacing_y;

            // Назначение типов пришельцев по рядам
            if (yi == 0) game.aliens[index].type = ALIEN_TYPE_C; // Верхний ряд (даёт 10 очков)
            else if (yi < 3) game.aliens[index].type = ALIEN_TYPE_B; // Средние ряды (дают 20 очков)
            else game.aliens[index].type = ALIEN_TYPE_A; // Нижние ряды (дают 30 очков)
        }
    }
    
    // Инициализация таймеров смерти
    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; ++i) {
        death_counters[i] = 0; 
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
    
    // --- Главный игровой цикл --- 
    while (!glfwWindowShouldClose(window) && game_running) {
        buffer_clear(&buffer, clear_color);

        // --- Обновление состояния игры ---

        // Движение игрока
        int player_speed = 3;
        if (move_dir != 0) {
            long new_x = game.player.x + (long)(player_speed * move_dir);
            if (new_x >= 0 && new_x <= game.width - player_sprite.width) {
                game.player.x = new_x;
            }
        }
        
        // Стрельба
        if (fire_pressed && game.num_bullets < GAME_MAX_BULLETS) {
            game.bullets[game.num_bullets].x = game.player.x + (player_sprite.width / 2);
            game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
            game.bullets[game.num_bullets].dir = 4; // Скорость пули (положительное значение, т.к. Y увеличивается вверх)
            ++game.num_bullets;
            fire_pressed = false;
        }

        // Обновление пуль
        for (size_t i = 0; i < game.num_bullets;) {
            game.bullets[i].y += game.bullets[i].dir;
            if (game.bullets[i].y >= game.height) { 
                game.bullets[i] = game.bullets[game.num_bullets - 1];
                --game.num_bullets;
            } else {
                ++i; 
            }
        }
        
        // Обновление анимации пришельцев
        // Теперь обновляем время для каждой активной анимации (для каждого типа пришельца)
        for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
            SpriteAnimation& current_anim = alien_animations[type_idx]; 
            ++current_anim.time;
            if (current_anim.time >= current_anim.num_frames * current_anim.frame_duration) {
                if (current_anim.loop) current_anim.time = 0;
            }
        }

        // --- Обновление таймеров смерти пришельцев ---
        for (size_t i = 0; i < game.num_aliens; ++i) {
            if (game.aliens[i].type == ALIEN_DEAD && death_counters[i] > 0) {
                --death_counters[i];
            }
        }

        // --- Обработка столкновений и логика ---
        // Получаем текущий кадр анимации для каждого типа пришельца, чтобы использовать его для коллизии
        // Это делается здесь, чтобы избежать повторных вычислений в цикле по пришельцам
        const Sprite* current_anim_frames_for_collision[4]; // Массив указателей на спрайты
        for (size_t type_idx = ALIEN_TYPE_A; type_idx <= ALIEN_TYPE_C; ++type_idx) {
            const SpriteAnimation& anim = alien_animations[type_idx];
            current_anim_frames_for_collision[type_idx] = anim.frames[anim.time / anim.frame_duration];
        }

        for (size_t bi = 0; bi < game.num_bullets;) { 
            bool bullet_removed = false;
            for (size_t ai = 0; ai < game.num_aliens; ++ai) { 
                if (game.aliens[ai].type == ALIEN_DEAD) continue; 

                const Alien& alien = game.aliens[ai];
                // Используем правильный спрайт анимации для коллизии в зависимости от типа пришельца
                const Sprite& alien_sprite_for_collision = *current_anim_frames_for_collision[alien.type];

                if (sprite_overlap_check(bullet_sprite, game.bullets[bi].x, game.bullets[bi].y, 
                                        alien_sprite_for_collision, alien.x, alien.y)) {
                    game.aliens[ai].type = ALIEN_DEAD; 
                    // Обновленная логика начисления очков
                    // Пришельцы типа A дают 30, B - 20, C - 10
                    game.score += 10 * (4 - alien.type); 
                    death_counters[ai] = 15; 

                    // Корректировка позиции для спрайта смерти, чтобы он центрировался по X
                    game.aliens[ai].x -= (alien_death_sprite.width - alien_sprite_for_collision.width) / 2;
                    
                    game.bullets[bi] = game.bullets[game.num_bullets - 1];
                    --game.num_bullets;
                    bullet_removed = true;
                    break; 
                }
            }
            if (!bullet_removed) ++bi; 
        }
        
        // --- Отрисовка ---
        
        // Рисуем пришельцев
        for (size_t i = 0; i < game.num_aliens; ++i) {
            const Alien& alien = game.aliens[i];
            if (alien.type != ALIEN_DEAD) {
                // Получаем текущий кадр анимации для данного типа пришельца
                const SpriteAnimation& anim = alien_animations[alien.type];
                size_t current_frame_index_for_draw = anim.time / anim.frame_duration;
                const Sprite& current_alien_sprite_for_draw = *anim.frames[current_frame_index_for_draw];

                buffer_sprite_draw(&buffer, current_alien_sprite_for_draw, alien.x, alien.y, rgba_to_uint32(255, 0, 0, 255));
            } else if (death_counters[i] > 0) {
                buffer_sprite_draw(&buffer, alien_death_sprite, alien.x, alien.y, rgba_to_uint32(255, 255, 0, 255));
            }
        }
        
        // Рисуем пули
        for (size_t i = 0; i < game.num_bullets; ++i) {
            buffer_sprite_draw(&buffer, bullet_sprite, game.bullets[i].x, game.bullets[i].y, rgba_to_uint32(255, 255, 0, 255));
        }
        
        // Рисуем игрока
        buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgba_to_uint32(0, 255, 0, 255));

        // Рисуем счет
        buffer_draw_text(
            &buffer,
            font_5x7_sprite, "SCORE",
            4, game.height - font_5x7_sprite.height - 7, // Расположение в верхнем левом углу
            rgba_to_uint32(128, 0, 0, 255) // Темно-красный цвет
        );

        buffer_draw_number(
            &buffer,
            font_5x7_sprite, game.score,
            4 + (font_5x7_sprite.width + 1) * 6, game.height - font_5x7_sprite.height - 7, // Справа от "SCORE"
            rgba_to_uint32(128, 0, 0, 255)
        );

        // Рисуем "CREDIT 00" внизу по центру
        const char* credit_text = "CREDIT 00";
        size_t credit_text_width = (font_5x7_sprite.width + 1) * 9 - 1; // 9 символов, минус 1 для последнего пробела
        buffer_draw_text(
            &buffer,
            font_5x7_sprite, credit_text,
            (game.width - credit_text_width) / 2, 7, // По центру внизу
            rgba_to_uint32(128, 0, 0, 255)
        );

        // Рисуем горизонтальную линию над текстом кредита
        uint32_t line_color = rgba_to_uint32(128, 0, 0, 255);
        size_t line_y_from_bottom = 16; // Высота линии от низа экрана
        for (size_t i = 0; i < game.width; ++i){    
            // Корректное преобразование Y-координаты для отрисовки линии в буфере
            // Поскольку buffer_sprite_draw рисует снизу вверх, то buffer.data[y * width + x]
            // где y=0 это нижняя строка. Если line_y_from_bottom = 16 (16 пикселей от низа),
            // то фактическая строка в буфере будет (buffer.height - 1 - line_y_from_bottom).
            size_t actual_buffer_y = buffer.height - 1 - line_y_from_bottom;
            if (actual_buffer_y < buffer.height) { // Дополнительная проверка на выход за пределы буфера
                 buffer.data[actual_buffer_y * game.width + i] = line_color; 
            }
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