#include "Player.h"

Player::Player(int start_x, int start_y, Sprite player_sprite) : life(3)
{
    this->x = start_x;
    this->y = start_y;
    this->sprite = player_sprite;
}

void Player::Update()
{
    // Логика движения игрока
    int player_speed = 3;
    long new_x = this->x + player_speed * move_dir;

    // Проверка границ экрана (предположим, ширина экрана 640)
    if (new_x >= 0 && new_x <= 640 - this->sprite.width)
    {
        this->x = new_x;
    }
}

void Player::Draw()
{
    // Отрисовка обрабатывается централизованно в классе Game
}

void Player::Move(int direction)
{
    move_dir = direction;
}

void Player::SetPosition(int new_x, int new_y)
{
    this->x = new_x;
    this->y = new_y;
}

void Player::SetLife(size_t new_life)
{
    life = new_life;
}

size_t Player::GetLife() const
{
    return life;
}

void Player::DecreaseLife()
{
    if (life > 0)
    {
        life--;
    }
}
