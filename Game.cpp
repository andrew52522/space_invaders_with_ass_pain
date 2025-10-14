#include "Game.h"
#include "Sprites.h"
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

// --- ШЕЙДЕРЫ ---
const char *vertex_shader_source =
    "#version 330\n"
    "noperspective out vec2 TexCoord;\n"
    "void main(void) {\n"
    "   TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0;\n"
    "   TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0;\n"
    "   gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
    "}\n";

const char *fragment_shader_source = R"(
    #version 330 core
    uniform sampler2D buffer;
    noperspective in vec2 TexCoord;
    out vec4 outColor;
    void main() {
       outColor = texture(buffer, TexCoord);
    }
)";

// --- Вспомогательная функция для проверки ошибок компиляции шейдера ---
bool validate_shader(GLuint shader_id, const std::string &type)
{
    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

        // Проверяем, что длина лога действительно больше нуля
        if (log_length > 0)
        {
            std::vector<char> log(log_length);
            glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
            std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED\n"
                      << log.data() << std::endl;
        }
        else
        {
            std::cerr << "ERROR::SHADER::" << type << "::COMPILATION_FAILED (No log available)\n";
        }
        return false;
    }
    return true;
}

// --- Реализация класса Game ---

Game::Game(int w, int h) : width(w), height(h), window(nullptr), state(GAME_RUNNING), score(0), current_level(1),
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

Game::~Game()
{
    delete[] buffer.data;
    glDeleteProgram(shader_id);
    glDeleteTextures(1, &buffer_texture_id);
    glDeleteVertexArrays(1, &vao_id);
    if (window)
    {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void Game::CheckGLError()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

void Game::Init()
{
    std::cout << "INIT CHECKPOINT 1: Start" << std::endl;

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    std::cout << "INIT CHECKPOINT 2: GLFW Initialized" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, "Space Invaders OOP", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    std::cout << "INIT CHECKPOINT 3: Window Created" << std::endl;

    glfwMakeContextCurrent(window);
    std::cout << "INIT CHECKPOINT 4: OpenGL Context Current" << std::endl;
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }
    std::cout << "INIT CHECKPOINT 5: GLEW Initialized" << std::endl;

    // прошу GLFW взять указатель на объект Game
    glfwSetWindowUserPointer(window, this);

    glfwSetKeyCallback(window, Game::KeyCallback);

    CheckGLError();
    std::cout << "INIT CHECKPOINT 6: Callbacks Set" << std::endl;

    // -- Выделение памяти для программного буфера кадра --
    buffer.data = new uint32_t[width * height];

    std::cout << "INIT CHECKPOINT 7: Screen Buffer Allocated" << std::endl;
    // умный укащатель. он автоматически управляет памятью, он сам уничтожается
    player = std::make_unique<Player>((width - player_sprite_data.width) / 2, 32, player_sprite_data);

    std::cout << "INIT CHECKPOINT 8: Player Created" << std::endl;

    const Sprite *alien_a_frames[] = {&alien_a_frame1, &alien_a_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_A)] = {true, 2, 30, 0, alien_a_frames};
    const Sprite *alien_b_frames[] = {&alien_b_frame1, &alien_b_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_B)] = {true, 2, 30, 0, alien_b_frames};
    const Sprite *alien_c_frames[] = {&alien_c_frame1, &alien_c_frame2};
    alien_animations[static_cast<int>(AlienType::TYPE_C)] = {true, 2, 30, 0, alien_c_frames};
    std::cout << "INIT CHECKPOINT 9: Animations Initialized" << std::endl;

    std::cout << "INIT CHECKPOINT 10: Compiling Shaders..." << std::endl;

    // -- компиляция шейдеров
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    if (!validate_shader(vertex_shader, "VERTEX"))
    {
        throw std::runtime_error("Vertex shader compilation failed");
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    if (!validate_shader(fragment_shader, "FRAGMENT"))
    {
        throw std::runtime_error("Fragment shader compilation failed");
    }

    shader_id = glCreateProgram();
    glAttachShader(shader_id, vertex_shader);
    glAttachShader(shader_id, fragment_shader);
    glLinkProgram(shader_id);
    if (!validate_program(shader_id))
    {
        throw std::runtime_error("Shader program validation failed");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    std::cout << "INIT CHECKPOINT 11: Shaders Linked" << std::endl;
    CheckGLError();

    glGenVertexArrays(1, &vao_id);
    glGenTextures(1, &buffer_texture_id);
    std::cout << "INIT CHECKPOINT 12: VAO and Texture Generated" << std::endl;

    glBindTexture(GL_TEXTURE_2D, buffer_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "INIT CHECKPOINT 13: Texture Configured" << std::endl;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "INIT CHECKPOINT 14: Calling ResetLevel..." << std::endl;
    ResetLevel();
    std::cout << "INIT CHECKPOINT 15: ResetLevel Finished" << std::endl;

    CheckGLError();
    std::cout << "INIT CHECKPOINT 16: Init Function Complete!" << std::endl;
}

void Game::Run()
{

    // -- Главный мой игровой цикл --
    while (!glfwWindowShouldClose(window))
    {
        // Логика обновления состояния игры
        if (state == GAME_RUNNING)
        {
            // вызываю метод апдейт который отвечает
            // за всю логику мувмента игрока, пули, алиенов
            Update();
        }

        // отрсисовоЧка
        Render();

        // Обработка событий (нажатия клавиш и т.д.)
        glfwPollEvents();
    }
}

void Game::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // очень  хитро доставаемый укзаатель, который положили в glfwSetWindowUserPointer в Init
    Game *game = static_cast<Game *>(glfwGetWindowUserPointer(window));
    if (!game)
        return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // Обработка для других состояний игры (например, перезапуск на экране Game Over)
    if (game->state != GAME_RUNNING)
    {
        if (action == GLFW_PRESS)
        {
            game->state = GAME_RUNNING;
            game->current_level = 1;
            game->score = 0;
            game->ResetLevel();
        }
        return;
    }

    // Управление в состоянии GAME_RUNNING
    int move_change = 0;
    if (key == GLFW_KEY_LEFT)
        move_change = -1;
    if (key == GLFW_KEY_RIGHT)
        move_change = 1;

    if (action == GLFW_PRESS)
    {
        if (move_change != 0)
            game->player_move_dir += move_change;
        if (key == GLFW_KEY_S)
        {
            game->switcher_super_bullet = !game->switcher_super_bullet;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        if (move_change != 0)
            game->player_move_dir -= move_change;
        if (key == GLFW_KEY_SPACE)
        {
            game->fire_pressed = true;
        }
    }
}

void Game::Update()
{
    player->Move(player_move_dir);
    player->Update();

    if (fire_pressed)
    {
        // Проверяем, может ли игрок стрелять (например, одна пуля на экране)
        if (player_bullets.empty())
        {

            player_bullets.emplace_back(
                player->GetX() + (player_sprite_data.width / 2),
                player->GetY() + player_sprite_data.height,
                4, (switcher_super_bullet == 1), bullet_sprite_data);
        }
        fire_pressed = false;
    }

    // Обновление и удаление пуль игрока
    // идиома erase-remove
    for (auto &b : player_bullets)
        b.Update();
    player_bullets.erase(std::remove_if(player_bullets.begin(), player_bullets.end(),
                                        [](const Bullet &b)
                                        { return !b.IsActive(); }),
                         player_bullets.end());

    // Обновление и удаление пуль пришельцев
    for (auto &b : alien_bullets)
        b.Update();
    alien_bullets.erase(std::remove_if(alien_bullets.begin(), alien_bullets.end(),
                                       [](const Bullet &b)
                                       { return !b.IsActive(); }),
                        alien_bullets.end());

    // Обновление анимаций пришельцев
    for (size_t type_idx = 1; type_idx <= 3; ++type_idx)
    {
        SpriteAnimation &anim = alien_animations[type_idx];
        ++anim.time;
        if (anim.time >= anim.num_frames * anim.frame_duration)
        {
            if (anim.loop)
                anim.time = 0;
        }
    }

    // Обновление счетчиков смерти
    for (size_t i = 0; i < NUM_ALIENS; ++i)
    {
        if (death_counters[i] > 0)
            --death_counters[i];
    }

    // Логика движения пришельцев
    alien_move_timer++;
    if (alien_move_timer >= alien_frames_per_move)
    {
        alien_move_timer = 0;
        bool should_drop = false;
        for (const auto &enemy : enemies)
        {
            if (!enemy.IsActive())
                continue;
            int x = enemy.GetX();
            const Sprite &s = enemy.GetSprite(); // Получаем актуальный спрайт
            if ((alien_move_dir > 0 && x + s.width + alien_speed > width) || (alien_move_dir < 0 && x - alien_speed < 0))
            {
                should_drop = true;
                break;
            }
        }

        if (should_drop)
        {
            alien_move_dir *= -1; // Меняем направление
            for (auto &enemy : enemies)
            {
                if (enemy.IsActive())
                {
                    enemy.SetPosition(enemy.GetX(), enemy.GetY() - alien_drop_amount);
                    // Проверка проигрыша
                    if (enemy.GetY() <= player->GetY() + player->GetSprite().height)
                    {
                        state = GAME_OVER_LOST;
                        return; // Выходим из Update
                    }
                }
            }
        }
        else
        {
            for (auto &enemy : enemies)
            {
                if (enemy.IsActive())
                    enemy.Move(alien_move_dir * alien_speed, 0);
            }
        }
    }

    // Логика стрельбы пришельцев
    std::uniform_int_distribution<int> fire_chance(0, 200);
    if (fire_chance(rng) < 5 && alien_bullets.size() < 3)
    {
        std::vector<int> active_aliens_indices;
        for (int i = 0; i < enemies.size(); ++i)
        {
            if (enemies[i].IsActive())
                active_aliens_indices.push_back(i);
        }
        if (!active_aliens_indices.empty())
        {
            std::uniform_int_distribution<size_t> dist(0, active_aliens_indices.size() - 1);
            const auto &shooter = enemies[active_aliens_indices[dist(rng)]];
            alien_bullets.emplace_back(shooter.GetX() + shooter.GetSprite().width / 2, shooter.GetY(), -2, false, bullet_sprite_data);
        }
    }

    CheckCollisions();

    if (player->GetLife() == 0)
        state = GAME_OVER_LOST;

    bool all_aliens_dead = true;
    for (const auto &enemy : enemies)
    {
        if (enemy.IsActive())
        {
            all_aliens_dead = false;
            break;
        }
    }
    if (all_aliens_dead)
    {
        current_level++;
        if (current_level > MAX_LEVELS)
        {
            state = GAME_OVER_WIN;
        }
        else
        {
            ResetLevel();
        }
    }
}

void Game::CheckCollisions()
{
    // Пули игрока vs (Пришельцы | Укрытия)
    for (auto &bullet : player_bullets)
    {
        if (!bullet.IsActive())
            continue;
        // Проверка столкновения с пришельцами
        for (size_t i = 0; i < enemies.size(); ++i)
        {
            if (!enemies[i].IsActive())
                continue;
            if (SpriteOverlapCheck(bullet.GetSprite(), bullet.GetX(), bullet.GetY(), enemies[i].GetSprite(), enemies[i].GetX(), enemies[i].GetY()))
            {
                enemies[i].TakeDamage(1);
                bullet.SetActive(false);
                if (!enemies[i].IsActive())
                {
                    score += 10 * (4 - static_cast<int>(enemies[i].GetType()));
                    death_counters[i] = 15; // Таймер для анимации взрыва
                }
                goto next_player_bullet; // Переходим к следующей пуле
            }
        }
        // Проверка столкновения с укрытиями
        for (auto &bunker : bunkers)
        {
            size_t hit_x, hit_y;
            if (BulletBunkerOverlapCheck(bullet, bunker, hit_x, hit_y))
            {
                // Уменьшаем "здоровье" пикселей вокруг точки попадания
                int r = 2;
                for (int yo = -r; yo <= r; ++yo)
                {
                    for (int xo = -r; xo <= r; ++xo)
                    {
                        if (xo * xo + yo * yo <= r * r)
                        {
                            int px = hit_x + xo;
                            int py = hit_y + yo;
                            if (px >= 0 && px < bunker.width && py >= 0 && py < bunker.height)
                            {
                                size_t idx = py * bunker.width + px;
                                if (bunker.pixel_data[idx] > 0)
                                    bunker.pixel_data[idx]--;
                            }
                        }
                    }
                }
                bullet.SetActive(false);
                goto next_player_bullet;
            }
        }
    next_player_bullet:;
    }

    // Пули пришельцев vs (Игрок | Укрытия)
    for (auto &bullet : alien_bullets)
    {
        if (!bullet.IsActive())
            continue;
        // Проверка столкновения с игроком
        if (SpriteOverlapCheck(bullet.GetSprite(), bullet.GetX(), bullet.GetY(), player->GetSprite(), player->GetX(), player->GetY()))
        {
            player->DecreaseLife();
            bullet.SetActive(false);
            continue; // Следующая пуля пришельцев
        }
        // Проверка столкновения с укрытиями
        for (auto &bunker : bunkers)
        {
            size_t hit_x, hit_y;
            if (BulletBunkerOverlapCheck(bullet, bunker, hit_x, hit_y))
            {
                int r = 2;
                for (int yo = -r; yo <= r; ++yo)
                {
                    for (int xo = -r; xo <= r; ++xo)
                    {
                        if (xo * xo + yo * yo <= r * r)
                        {
                            int px = hit_x + xo;
                            int py = hit_y + yo;
                            if (px >= 0 && px < bunker.width && py >= 0 && py < bunker.height)
                            {
                                size_t idx = py * bunker.width + px;
                                if (bunker.pixel_data[idx] > 0)
                                    bunker.pixel_data[idx]--;
                            }
                        }
                    }
                }
                bullet.SetActive(false);
                goto next_alien_bullet;
            }
        }
    next_alien_bullet:;
    }
}

void Game::ResetLevel()
{
    enemies.clear();
    player_bullets.clear();
    alien_bullets.clear();

    alien_frames_per_move = 60 - (current_level - 1) * 15;
    player->SetPosition((width - player->GetSprite().width) / 2, 32);
    player->SetLife(3);
    alien_drop_count = 0;

    const size_t spacing_x = 16, spacing_y = 16;
    const size_t start_y = height - 150;
    const size_t total_aliens_width = ALIENS_COLS * 12 + (ALIENS_COLS - 1) * (spacing_x - 12);
    const size_t start_x = (width - total_aliens_width) / 2;

    for (size_t yi = 0; yi < ALIENS_ROWS; ++yi)
    {
        for (size_t xi = 0; xi < ALIENS_COLS; ++xi)
        {
            AlienType type;
            const Sprite *initial_sprite;
            if (yi == 0)
            {
                type = AlienType::TYPE_C;
                initial_sprite = &alien_c_frame1;
            }
            else if (yi < 3)
            {
                type = AlienType::TYPE_B;
                initial_sprite = &alien_b_frame1;
            }
            else
            {
                type = AlienType::TYPE_A;
                initial_sprite = &alien_a_frame1;
            }

            int cur_x = start_x + xi * spacing_x;
            int cur_y = start_y - yi * spacing_y;
            enemies.emplace_back(cur_x, cur_y, type, *initial_sprite);
        }
    }

    const size_t bunker_spacing = (width - GAME_NUM_BUNKERS * bunker_base_sprite_data.width) / (GAME_NUM_BUNKERS + 1);
    const size_t bunker_start_y = player->GetY() + player_sprite_data.height + 20;
    for (size_t i = 0; i < GAME_NUM_BUNKERS; ++i)
    {
        bunkers[i].x = bunker_spacing * (i + 1) + i * bunker_base_sprite_data.width;
        bunkers[i].y = bunker_start_y;
        bunkers[i].width = bunker_base_sprite_data.width;
        bunkers[i].height = bunker_base_sprite_data.height;
        bunkers[i].pixel_data.resize(bunker_base_sprite_data.width * bunker_base_sprite_data.height);
        for (size_t p_idx = 0; p_idx < bunker_base_sprite_data.width * bunker_base_sprite_data.height; ++p_idx)
        {
            bunkers[i].pixel_data[p_idx] = (bunker_base_sprite_data.data[p_idx] == 1) ? 5 : 0; // 5 HP
        }
    }
}

void Game::Render()
{
    // --- ЭТАП 1: РИСОВАНИЕ ВСЕХ ОБЪЕКТОВ В ПРОГРАММНЫЙ БУФЕР ---

    // 1. Очищаем фон чёрным цветом
    uint32_t clear_color = RgbaToUint32(0, 0, 0, 255);
    std::fill(buffer.data, buffer.data + width * height, clear_color);

    // отрисовка пришельцев.
    // цикл анимации ломает мне программу =(
    // только статика
    for (size_t i = 0; i < enemies.size(); ++i)
    {

        const auto &enemy = enemies[i];
        AlienType type = enemy.GetType();
        if (enemy.IsActive())
        {

            // Используем только тот спрайт, который хранится в самом объекте врага
            const Sprite &s = enemy.GetSprite();

            uint32_t color;
            switch (type)
            {
            case AlienType::TYPE_A:
                color = RgbaToUint32(255, 0, 0, 255);
                break;
            case AlienType::TYPE_B:
                color = RgbaToUint32(0, 255, 255, 255);
                break;
            case AlienType::TYPE_C:
                color = RgbaToUint32(0, 0, 255, 255);
                break;
            default:
                color = RgbaToUint32(255, 255, 255, 255);
            }

            DrawSprite(s, enemy.GetX(), enemy.GetY(), color);
        }
        // Отрисовку взрывов тоже пока отключаем для чистоты теста

        else if (death_counters[i] > 0)
        {
            DrawSprite(alien_death_sprite_data, enemies[i].GetX(), enemies[i].GetY(), RgbaToUint32(255, 255, 0, 255));
        }
    }

    // 3. Рисуем пули игрока
    for (const auto &b : player_bullets)
    {
        if (b.IsActive())
        {
            uint32_t color = b.IsSuper() ? RgbaToUint32(255, 0, 255, 255) : RgbaToUint32(255, 255, 0, 255);
            DrawSprite(b.GetSprite(), b.GetX(), b.GetY(), color, b.IsSuper());
        }
    }

    // 4. Рисуем пули пришельцев
    for (const auto &b : alien_bullets)
    {
        if (b.IsActive())
        {
            DrawSprite(b.GetSprite(), b.GetX(), b.GetY(), RgbaToUint32(255, 255, 255, 255));
        }
    }

    // 5. Рисуем укрытия
    for (const auto &b : bunkers)
    {
        DrawBunker(b, RgbaToUint32(0, 255, 0, 255));
    }

    // 6. Рисуем игрока
    DrawSprite(player->GetSprite(), player->GetX(), player->GetY(), RgbaToUint32(0, 255, 0, 255));

    // 7. Рисуем интерфейс
    DrawText(font_5x7_sprite_data, "SCORE", 4, 10, RgbaToUint32(255, 255, 255, 255));
    DrawNumber(font_5x7_sprite_data, score, 60, 10, RgbaToUint32(255, 255, 255, 255));
    DrawText(font_5x7_sprite_data, "LIVES", width - 100, 10, RgbaToUint32(255, 255, 255, 255));
    for (size_t i = 0; i < player->GetLife(); ++i)
    {
        DrawSprite(heart_sprite_data, width - 40 + i * (heart_sprite_data.width + 2), 10, RgbaToUint32(255, 0, 0, 255));
    }

    // 8. Рисуем экраны победы или поражения
    if (state == GAME_OVER_LOST)
    {
        DrawText(font_5x7_sprite_data, "GAME OVER", width / 2 - 40, height / 2, RgbaToUint32(255, 0, 0, 255));
    }
    else if (state == GAME_OVER_WIN)
    {
        DrawText(font_5x7_sprite_data, "YOU WIN", width / 2 - 30, height / 2, RgbaToUint32(0, 255, 0, 255));
    }

    // --- ЭТАП 2: ВЫВОД НА ЭКРАН С ПОМОЩЬЮ GPU ---
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_id);
    glBindTexture(GL_TEXTURE_2D, buffer_texture_id); // <- Опечатка здесь исправлена
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer.data);
    glBindVertexArray(vao_id);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glfwSwapBuffers(window);
}

uint32_t Game::RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

void Game::DrawSprite(const Sprite &sprite, size_t x, size_t y, uint32_t color, bool is_super)
{
    if (is_super)
    {
        // Отрисовка "лазера" вверх
        for (size_t yi = 0; yi < y + sprite.height; ++yi)
        {
            size_t sx = x;
            size_t sy = y + sprite.height - 1 - yi;
            if (sx < width && sy < height)
            {
                buffer.data[sy * width + sx] = color;
            }
        }
    }
    else
    {
        // Обычная отрисовка спрайта с правильной координатой Y
        for (size_t yi = 0; yi < sprite.height; ++yi)
        {
            for (size_t xi = 0; xi < sprite.width; ++xi)
            {
                // Y-координата инвертируется для соответствия буферу
                size_t sy = y + (sprite.height - 1 - yi);
                size_t sx = x + xi;
                if (sprite.data[yi * sprite.width + xi] && sx < width && sy < height)
                {
                    buffer.data[sy * width + sx] = color;
                }
            }
        }
    }
}

void Game::DrawBunker(const Bunker &bunker, uint32_t color)
{
    for (size_t yi = 0; yi < bunker.height; ++yi)
    {
        for (size_t xi = 0; xi < bunker.width; ++xi)
        {
            size_t sx = bunker.x + xi;
            size_t sy = bunker.y + yi;
            if (bunker.pixel_data[yi * bunker.width + xi] > 0 && sx < width && sy < height)
            {
                buffer.data[sy * width + sx] = color;
            }
        }
    }
}

void Game::DrawText(const Sprite &font, const char *text, size_t x, size_t y, uint32_t color)
{
    size_t xp = x;
    size_t stride = font.width * font.height;

    for (const char *cp = text; *cp != '\0'; ++cp)
    {
        char c = *cp;
        if (c < 32 || c > 126)
            c = '?';

        const uint8_t *char_data = font_5x7_data + (c - 32) * stride;

        for (size_t yi = 0; yi < font.height; ++yi)
        {
            for (size_t xi = 0; xi < font.width; ++xi)
            {
                // Правильная Y-координата для текста
                size_t sy = y + (font.height - 1 - yi);
                size_t sx = xp + xi;
                if (char_data[yi * font.width + xi] && sx < width && sy < height)
                {
                    buffer.data[sy * width + sx] = color;
                }
            }
        }
        xp += font.width + 1;
    }
}

void Game::DrawNumber(const Sprite &font, size_t number, size_t x, size_t y, uint32_t color)
{
    char text_buffer[11];
    snprintf(text_buffer, sizeof(text_buffer), "%zu", number);
    DrawText(font, text_buffer, x, y, color);
}

bool Game::SpriteOverlapCheck(const Sprite &sp_a, size_t x_a, size_t y_a, const Sprite &sp_b, size_t x_b, size_t y_b)
{
    return (x_a < x_b + sp_b.width && x_a + sp_a.width > x_b &&
            y_a < y_b + sp_b.height && y_a + sp_a.height > y_b);
}

bool Game::BulletBunkerOverlapCheck(const Bullet &bullet, const Bunker &bunker, size_t &hit_x, size_t &hit_y)
{
    const Sprite &bullet_sprite = bullet.GetSprite();
    if (!(bullet.GetX() < bunker.x + bunker.width && bullet.GetX() + bullet_sprite.width > bunker.x &&
          bullet.GetY() < bunker.y + bunker.height && bullet.GetY() + bullet_sprite.height > bunker.y))
    {
        return false;
    }
    for (size_t byi = 0; byi < bullet_sprite.height; ++byi)
    {
        for (size_t bxi = 0; bxi < bullet_sprite.width; ++bxi)
        {
            if (bullet_sprite.data[byi * bullet_sprite.width + bxi])
            {
                int world_x = bullet.GetX() + bxi;
                int world_y = bullet.GetY() + byi;
                if (world_x >= bunker.x && world_x < bunker.x + bunker.width &&
                    world_y >= bunker.y && world_y < bunker.y + bunker.height)
                {

                    size_t bunker_px = world_x - bunker.x;
                    size_t bunker_py = world_y - bunker.y;
                    if (bunker.pixel_data[bunker_py * bunker.width + bunker_px] > 0)
                    {
                        hit_x = bunker_px;
                        hit_y = bunker_py;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool Game::validate_program(unsigned int program)
{
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

        // Проверяем, что длина лога действительно больше нуля
        if (len > 0)
        {
            std::vector<char> log(len);
            glGetProgramInfoLog(program, len, nullptr, log.data());
            std::cerr << "Shader link error: " << log.data() << std::endl;
        }
        else
        {
            std::cerr << "Shader link error: (No log available)\n";
        }
        return false;
    }
    return true;
}