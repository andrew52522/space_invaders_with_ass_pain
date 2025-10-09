#include "Enemy.h"

Enemy::Enemy(int start_x, int start_y, AlienType alien_type, Sprite enemy_sprite)
    : type(alien_type) {
    this->x = start_x;
    this->y = start_y;
    this->sprite = enemy_sprite;

    switch (type) {
        case AlienType::TYPE_A:
            hp = 1;
            break;
        case AlienType::TYPE_B:
            hp = 2;
            break;
        case AlienType::TYPE_C:
            hp = 3;
            break;
        case AlienType::DEAD:
            hp = 0;
            is_active = false;
            break;
    }
}

void Enemy::Update() {
    // Рух ворогів обробляється централізовано у класі Game
}

void Enemy::Draw() {
    // Малювання обробляється централізовано в класі Game
}

AlienType Enemy::GetType() const {
    return type;
}

int Enemy::GetHp() const {
    return hp;
}

void Enemy::TakeDamage(int amount) {
    hp -= amount;
    if (hp <= 0) {
        hp = 0;
        is_active = false;
        type = AlienType::DEAD;
    }
}

void Enemy::SetPosition(int start_x, int start_y) {
    x = start_x;
    y = start_y;
}

void Enemy::Move(int dx, int dy) {
    x += dx;
    y += dy;
}