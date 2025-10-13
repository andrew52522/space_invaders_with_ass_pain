#ifndef ENEMY_H
#define ENEMY_H

#include "GameObject.h"

enum class AlienType : uint8_t {
    DEAD = 0,
    TYPE_A = 1,
    TYPE_B = 2,
    TYPE_C = 3
};

class Enemy : public GameObject {
public:
    Enemy(int start_x, int start_y, AlienType type, Sprite enemy_sprite);

    void Update() override;
    void Draw() override;
    void SetPosition(int start_x, int start_y);
    void Move(int dx, int dy);
    AlienType GetType() const;
    void TakeDamage(int amount);
    int GetHp() const;

private:
    AlienType type;
    int hp;
};

#endif // ENEMY_H