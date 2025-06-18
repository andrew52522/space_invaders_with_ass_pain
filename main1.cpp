#include <cstdio>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdint> // для uint32_t и uint8_t


#define GAME_MAX_BULLETS 128

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
    uint8_t* data;
};

struct Alien {
    size_t x, y;
    uint8_t type; 
};

// типы пришельцев
enum AlienType : uint8_t {
    ALIEN_DEAD = 0,      // Тип для мертвых пришельцев
    ALIEN_TYPE_A = 1,    // Тип A (первый ряд)
    ALIEN_TYPE_B = 2,    // Тип B (второй ряд)
    ALIEN_TYPE_C = 3     // Тип C (третий ряд и ниже)
};

struct Player {
    size_t x, y;
    size_t life;
};

struct Game {
    size_t width, height;
    size_t num_aliens;
    size_t num_bullets; // Количество пуль
    Alien* aliens;
    Player player;
    Bullet bullets[GAME_MAX_BULLETS]; // Массив пуль
};

struct SpriteAnimation {
    bool loop;
    size_t num_frames;
    size_t frame_duration; // в кадрах
    size_t time;           // текущее время анимации
    Sprite** frames;
};

// Преобразование RGB в 32-битный цвет
uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

 // Очистка буфера заданным цветом
    void buffer_clear(Buffer* buffer, uint32_t color) {
    for(size_t i = 0; i < buffer->width * buffer->height; ++i) {
        buffer->data[i] = color;
        }
    }

// Функция рисования спрайта в буфер
void buffer_sprite_draw(
    Buffer* buffer, const Sprite& sprite,
    size_t x, size_t y, uint32_t color
) {
    for(size_t xi = 0; xi < sprite.width; ++xi) {
        for(size_t yi = 0; yi < sprite.height; ++yi) {
            size_t sy = y + (sprite.height - yi - 1);
            size_t sx = x + xi;

            if(sprite.data[yi * sprite.width + xi] && 
               sy < buffer->height && sx < buffer->width) 
            {
                buffer->data[sy * buffer->width + sx] = color;
            }
        }
    }
}

void validate_shader(GLuint shader, const char* file = 0) {
    static const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    GLsizei length = 0;
    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
    if (length > 0) {
        printf("Shader %d(%s) compile error: %s", shader, (file ? file : ""), buffer);
    }
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

// проверка на то, что спрайты не пересекаются
bool sprite_overlap_check(
    const Sprite& sp_a, size_t x_a, size_t y_a,
    const Sprite& sp_b, size_t x_b, size_t y_b
) {
    return (
        x_a < x_b + sp_b.width &&
        x_a + sp_a.width > x_b &&
        y_a < y_b + sp_b.height &&
        y_a + sp_a.height > y_b
    );
}

// Инициализация буфера
const int buffer_width = 640;
const int buffer_height = 480;
uint32_t clear_color = rgba_to_uint32(255, 255, 255); // Белый фон

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}




// Вершинный шейдер
const char* vertex_shader =
    "#version 330\n"
    "noperspective out vec2 TexCoord;\n"
    "void main(void) {\n"
    "    TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0;\n"
    "    TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0;\n"
    "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
    "}\n";

// Фрагментный шейдер
const char* fragment_shader =
    "#version 330\n"
    "uniform sampler2D buffer;\n"
    "noperspective in vec2 TexCoord;\n"
    "out vec3 outColor;\n"
    "void main(void) {\n"
    "    outColor = texture(buffer, TexCoord).rgb;\n"
    "}\n";

// глобальные переменные для управления
bool game_running = true;
int move_dir = 0;
bool fire_pressed = false;

//  Функция key_callback — это обработчик событий клавиатуры
//  в GLFW. Она вызывается каждый раз,
//  когда пользователь нажимает или отпускает клавишу на клавиатуре.
//  Эта функция позволяет реагировать на действия игрока
//  (например, стрельба, движение).
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch(key) {
        case GLFW_KEY_ESCAPE:
            if(action == GLFW_PRESS) game_running = false;
            break;
        case GLFW_KEY_RIGHT:
            if(action == GLFW_PRESS) move_dir += 1;
            else if(action == GLFW_RELEASE) move_dir -= 1;
            break;
        case GLFW_KEY_LEFT:
            if(action == GLFW_PRESS) move_dir -= 1;
            else if(action == GLFW_RELEASE) move_dir += 1;
            break;
        case GLFW_KEY_SPACE:
            if(action == GLFW_RELEASE) fire_pressed = true;
            break;
    }
}

int main() {
    // 1. Инициализация GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    // 2. Настройка окна и OpenGL 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1); // Включаем V-Sync 
    // для стабильности фепесов (1 = синхронизация с обновлением монитора)

    // 3. Инициализация GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) { glfwTerminate(); return -1; }

    // 4. Проверка версии OpenGL
    int glVersion[2] = {-1, -1};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
    printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);

    // 5. Буфер - выделение памяти и спрайт 
    Buffer buffer;
    buffer.width = buffer_width;
    buffer.height = buffer_height;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    buffer_clear(&buffer, clear_color);

    // Спрайт для ALIEN_TYPE_A (ширина 11x8)
    Sprite alien_sprite_a;
    alien_sprite_a.width = 11;
    alien_sprite_a.height = 8;
    alien_sprite_a.data = new uint8_t[88] {
        0,0,1,0,0,0,0,0,1,0,0, // ..@...@..
        1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@...@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.....@.
    };

    // Спрайт для ALIEN_TYPE_B (ширина 11x8)
    Sprite alien_sprite_b;
    alien_sprite_b.width = 11;
    alien_sprite_b.height = 8;
    alien_sprite_b.data = new uint8_t[88] {
        0,0,0,1,1,0,1,1,0,0,0, // ...@@.@@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@...@.@
        0,0,0,1,1,0,1,1,0,0,0, // ...@@.@@...
        0,1,0,0,0,0,0,0,0,1,0  // .@.....@.
    };

    // Спрайт для ALIEN_TYPE_C (ширина 11x8)
    Sprite alien_sprite_c;
    alien_sprite_c.width = 11;
    alien_sprite_c.height = 8;
    alien_sprite_c.data = new uint8_t[88] {
        0,0,1,0,0,0,0,0,1,0,0, // ..@...@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@...@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };


    // определение спрайта игрока
    Sprite player_sprite;
    player_sprite.width = 11;
    player_sprite.height = 7;
    player_sprite.data = new uint8_t[77] {
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1  // @@@@@@@@@@@
    };

    // спрайт на пульку
    Sprite bullet_sprite;
    bullet_sprite.width = 1;
    bullet_sprite.height = 3;
    bullet_sprite.data = new uint8_t[3] {
        1, // @
        1, // @
        1  // @
    };
    
    // спрайт на СМЭРТЬ от пульки
    Sprite alien_death_sprite;
    alien_death_sprite.width = 13;
    alien_death_sprite.height = 7;
    alien_death_sprite.data = new uint8_t[91] {
        0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    // Массив спрайтов для всех типов пришельцев
    Sprite alien_sprites[4]; // 0 - DEAD, 1 - TYPE_A, 2 - TYPE_B, 3 - TYPE_C
    alien_sprites[0] = alien_sprite_a; // Пример: TYPE_A как спрайт смерти
    alien_sprites[1] = alien_sprite_a;
    alien_sprites[2] = alien_sprite_b;
    alien_sprites[3] = alien_sprite_c;
    // Теперь вы можете обращаться к спрайтам по индексу: 
    // alien_sprites[ALIEN_TYPE_A], alien_sprites[ALIEN_TYPE_B]
    //  и т.д.

    // Создание анимации
    SpriteAnimation alien_animation;
    alien_animation.loop = true;
    alien_animation.num_frames = 2;
    alien_animation.frame_duration = 30; // 30 кадров на 1 спрайт (примерно полсекунды при 60 FPS)
    alien_animation.time = 0;

    alien_animation.frames = new Sprite*[2];
    alien_animation.frames[0] = &alien_sprite_a; // спрайт ручки вверх
    alien_animation.frames[1] = &alien_sprite_c; // ручки вниз

    // 'экземпляр игра
    Game game;
    game.width = buffer_width;
    game.height = buffer_height;

    // Инициализация игрока
    game.player.x = (buffer.width - player_sprite.width) / 2; // Примерно по центру
    game.player.y = 32;
    game.player.life = 3;

    //переменные для позиционирования пришельцев
    const size_t spacing = 16; // Расстояние между пришельцами
    const size_t total_width = (11 - 1) * spacing + 11; // Ширина всей группы пришельцев
    const size_t start_x = (game.width - total_width) / 2; // Центрирование
    // Инициализация роя пришельцев
    for(size_t yi = 0; yi < 5; ++yi) {
    for(size_t xi = 0; xi < 11; ++xi) {
        size_t index = yi * 11 + xi;
        game.aliens[index].x = spacing * xi + start_x;
        game.aliens[index].y = 17 * yi + 128;

        // Назначаем типы по рядам
        if (yi == 0 || yi == 1) {
            game.aliens[index].type = ALIEN_TYPE_A; // Первые два ряда — TYPE_A
        } else if (yi == 2 || yi == 3) {
            game.aliens[index].type = ALIEN_TYPE_B; // Следующие два ряда — TYPE_B
        } else {
            game.aliens[index].type = ALIEN_TYPE_C; // Последний ряд — TYPE_C
        }
    }
}    // Теперь первый и второй ряды — пришельцы типа A,
//  третий и четвертый — типа B, пятый — типа C.

    
    //инициализация пулек
    game.score = 0;
    game.num_bullets = 0;

    // Рассчитываем ширину всей группы пришельцев
    const size_t alien_width = 11;      // Ширина одного пришельца
    const size_t spacing = 16;          // Расстояние между пришельцами
    size_t total_width = (11 - 1) * spacing + alien_width; // Общая ширина группы
    size_t start_x = (game.width - total_width) / 2;      // Начальная позиция X для центрирования

    for(size_t yi = 0; yi < 5; ++yi) {
        for(size_t xi = 0; xi < 11; ++xi) {
            size_t index = yi * 11 + xi;
            game.aliens[index].x = spacing * xi + start_x; // Центрированная позиция
            game.aliens[index].y = 17 * yi + 128;
        }
    }

    // death_counters отслеживает время анимации смерти
    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; ++i) {
        death_counters[i] = 10; // Анимация смерти длится 10 кадров
    }
    // Массив death_counters хранит таймер для каждого пришельца.
    //  Когда значение таймера достигает 0, пришелец "исчезает".

    // 6. Шейдеры
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_id, 1, &vertex_shader, NULL);
    glCompileShader(vertex_shader_id);

    // ✅ Проверка вершинного шейдера
    GLint success;
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
    if (!success) { 
    char infoLog[512];
    glGetShaderInfoLog(vertex_shader_id, 512, NULL, infoLog);
    printf("Ошибка компиляции вершинного шейдера: %s\n", infoLog);
    }

    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader_id, 1, &fragment_shader, NULL);
    glCompileShader(fragment_shader_id);

    // ✅ Проверка фрагментного шейдера
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &success);
    if (!success) { 
    char infoLog[512];
    glGetShaderInfoLog(fragment_shader_id, 512, NULL, infoLog);
    printf("Ошибка фрагментарного вершинного* шейдера: %s\n", infoLog);
}

    GLuint shader_id = glCreateProgram();
    glAttachShader(shader_id, vertex_shader_id);
    glAttachShader(shader_id, fragment_shader_id);
    glLinkProgram(shader_id);

    // проверка линковки программы
    if (!validate_program(shader_id)) {
        glfwTerminate();
        return -1;
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    

    // 7. Текстура и VAO
    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, 
        buffer.width, buffer.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, buffer.data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint fullscreen_triangle_vao;
    glGenVertexArrays(1, &fullscreen_triangle_vao);
    glBindVertexArray(fullscreen_triangle_vao);

    // Перед игровым циклом
    glDisable(GL_DEPTH_TEST); // Отключаем тест глубины
    glBindVertexArray(fullscreen_triangle_vao);


    // Привязка текстуры к шейдеру
    GLint location = glGetUniformLocation(shader_id, "buffer");
    glUniform1i(location, 0); // Указываем, что используем текстурный юнит 0

    // направления игрока
    int player_move_dir = 1; // 1 - вправо, -1 - влево 

    // Главный игровой цикл
    while (!glfwWindowShouldClose(window)) {

        // Очищаем буфер и рисуем спрайт
        buffer_clear(&buffer, clear_color);

        // Рисуем всех пришельцев
        for (size_t ai = 0; ai < game.num_aliens; ++ai) {
        if (!death_counters[ai]) continue; // Пропустить мертвых пришельцев

        for (size_t ai = 0; ai < game.num_aliens; ++ai) {
    if (!death_counters[ai]) continue; // Пропустить мертвых пришельцев

        const Alien& alien = game.aliens[ai]; // Объявляем переменную
        if (alien.type == ALIEN_DEAD) {
            buffer_sprite_draw(&buffer, alien_death_sprite, alien.x, alien.y, rgba_to_uint32(255, 0, 0));
        } else {
            size_t current_frame = alien_animation.time / alien_animation.frame_duration;
            const Sprite& sprite = *alien_animation.frames[current_frame];
            buffer_sprite_draw(&buffer, sprite, alien.x, alien.y, rgba_to_uint32(255, 0, 0));
        }
    }  // Если пришелец мертв (ALIEN_DEAD), рисуем спрайт смерти. Иначе рисуем его текущий анимационный кадр.
                
            // Вычисляем текущий кадр анимации
            size_t current_frame = alien_animation.time / alien_animation.frame_duration;
            const Sprite& sprite = *alien_animation.frames[current_frame];

            buffer_sprite_draw(&buffer, sprite, alien.x, alien.y, rgba_to_uint32(255, 0, 0));
        
        // Рисуем игрока
        buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y, rgba_to_uint32(0, 255, 0)); // Сделаем его зеленым

        
        // Проверка столкновений пули и пришельцев
        for (size_t bi = 0; bi < game.num_bullets;) {
            bool hit = false;
            for (size_t ai = 0; ai < game.num_aliens; ++ai) {
                if (game.aliens[ai].type == ALIEN_DEAD) continue;

                const Alien& alien = game.aliens[ai];
                const Sprite& sprite = *alien_animation.frames[alien.type == ALIEN_DEAD ? 0 : 1];
                bool overlap = sprite_overlap_check(
                    bullet_sprite, game.bullets[bi].x, game.bullets[bi].y,
                    sprite, alien.x, alien.y
                );

                if (overlap) {
                    game.aliens[ai].type = ALIEN_DEAD;
                    game.aliens[ai].x -= (alien_death_sprite.width - sprite.width) / 2;
                    game.bullets[bi] = game.bullets[game.num_bullets - 1];
                    --game.num_bullets;
                    hit = true;
                    break;
                }
            }
            if (!hit) ++bi;
        }
        // При попадании пули в пришельца его тип меняется на ALIEN_DEAD, и он отображается как спрайт смерти.
        
        // Обновление таймеров смерти
        for (size_t ai = 0; ai < game.num_aliens; ++ai) {
            if (game.aliens[ai].type == ALIEN_DEAD && death_counters[ai]) {
                --death_counters[ai];
            }
        }
        
        // Отрисовка пуль
        for(size_t bi = 0; bi < game.num_bullets; ++bi) {
            const Bullet& bullet = game.bullets[bi];
            buffer_sprite_draw(&buffer, bullet_sprite, bullet.x, bullet.y, rgba_to_uint32(255, 255, 0)); // Желтый
        }

        // Обновление позиции пуль
        for(size_t bi = 0; bi < game.num_bullets;) {
            game.bullets[bi].y += game.bullets[bi].dir;
            if(game.bullets[bi].y >= game.height || game.bullets[bi].y < bullet_sprite.height) {
                game.bullets[bi] = game.bullets[game.num_bullets - 1];
                --game.num_bullets;
                continue;
            }
            ++bi;
        }
    
        // Обновляем текстуру в GPU
        glActiveTexture(GL_TEXTURE0); // ✅ Активируем юнит 0
        glBindTexture(GL_TEXTURE_2D, buffer_texture);
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, // xoffset, yoffset
            buffer.width, buffer.height,
            GL_RGBA, GL_UNSIGNED_BYTE, buffer.data // ✅ Используем GL_UNSIGNED_BYTE
        );

        // Отрисовка треугольника с текстурой
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader_id);
        glBindVertexArray(fullscreen_triangle_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ++alien_animation.time;
        if (alien_animation.time == alien_animation.num_frames * alien_animation.frame_duration) {
            if (alien_animation.loop) {
                alien_animation.time = 0;
            }
        }
        
        // логика на движение игрока
        int player_move_dir = 2 * move_dir;
        if(player_move_dir != 0) {
            if(game.player.x + player_sprite.width + player_move_dir >= game.width - 1) {
                game.player.x = game.width - player_sprite.width - 1;
            } else if((int)game.player.x + player_move_dir <= 0) {
                game.player.x = 0;
            } else {
                game.player.x += player_move_dir;
            }
        }

        // 
        // Обновляем позицию игрока движения
        if (game.player.x + player_sprite.width + player_move_dir >= game.width - 1) {
            // Достигли правого края
            game.player.x = game.width - player_sprite.width - 1;
            player_move_dir *= -1; // Меняем направление
        } else if ((int)game.player.x + player_move_dir <= 0) {
            // Достигли левого края
            game.player.x = 0;
            player_move_dir *= -1; // Меняем направление
        } else {
            // Просто двигаемся
            game.player.x += player_move_dir;
        }

        // реализация стрельбы
        if(fire_pressed && game.num_bullets < GAME_MAX_BULLETS) {
            game.bullets[game.num_bullets].x = game.player.x + player_sprite.width / 2;
            game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
            game.bullets[game.num_bullets].dir = 2; // Вверх
            ++game.num_bullets;
        }
        fire_pressed = false;

        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
   

    // 9. Очисткаf
    delete[] buffer.data;
    delete[] death_counters;
    delete[] alien_sprite_a.data;
    delete[] alien_sprite_b.data;
    delete[] alien_sprite_c.data;
    delete[] alien_death_sprite.data;
    delete[] player_sprite.data; // чистим память от спрайта игрока
    delete[] game.aliens; // чистим память от метода aliens стру
    delete[] alien_animation.frames; // удаляем анимацию
    delete[] bullet_sprite.data; // удаляем пулю


    glDeleteTextures(1, &buffer_texture);
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);
    glDeleteProgram(shader_id);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ебаный death_counters
