#include "Bullet.h"

// 1. Конструктор пули
Bullet::Bullet(int start_x, int start_y, int direction, bool super, Sprite bullet_sprite)
    : dir(direction), is_super(super) { 
    this->x = start_x;
    this->y = start_y;
    this->sprite = bullet_sprite;
}

// 2. Функция обновления состояния пули
void Bullet::Update() {
    y += dir;
    // Логика для деактивации пули при выходе за пределы экрана
    if (y >= 480 || y < 0) {
        is_active = false;
    }
}

void Bullet::Draw() {
    // Отрисовка обрабатывается централизованно в классе Game
}

bool Bullet::IsSuper() const {
    return is_super;
}
