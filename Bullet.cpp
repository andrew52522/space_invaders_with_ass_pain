#include "Bullet.h"

// 1. Обновите конструктор
Bullet::Bullet(int start_x, int start_y, int direction, bool super, Sprite bullet_sprite)
    : dir(direction), is_super(super) { 
    this->x = start_x;
    this->y = start_y;
    this->sprite = bullet_sprite;
}

// 2. В вашей функции Update() все уже почти правильно.
void Bullet::Update() {
    y += dir;
    // Логика выхода за экран уже есть, но переменной не было. Теперь будет работать.
    if (y >= 480 || y < 0) {
        is_active = false;
    }
}

void Bullet::Draw() {
    // Малювання обробляється централізовано в класі Game
}

bool Bullet::IsSuper() const {
    return is_super;
}

