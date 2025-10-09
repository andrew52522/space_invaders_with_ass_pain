#include "Player.h"

Player::Player(int start_x, int start_y, Sprite player_sprite) : life(3) {
    this->x = start_x;
    this->y = start_y;
    this->sprite = player_sprite;
}

void Player::Update() {
    // Логіка руху гравця
    int player_speed = 3;
    long new_x = this->x + (long)(player_speed * move_dir);

    // Перевірка меж екрану (припустимо, ширина екрану 640)
    if (new_x >= 0 && new_x <= 640 - this->sprite.width) {
        this->x = new_x;
    }
}

void Player::Draw() {
    // Малювання обробляється централізовано в класі Game
}

void Player::Move(int direction) {
    move_dir = direction;
}

void Player::SetPosition(int new_x, int new_y) {
    this->x = new_x;
    this->y = new_y;
}

void Player::SetLife(size_t new_life) {
    life = new_life;
}

size_t Player::GetLife() const {
    return life;
}

void Player::DecreaseLife() {
    if (life > 0) {
        life--;
    }
}