#ifndef PLAYER_H
#define PLAYER_H

#include "GameObject.h"

class Player : public GameObject {
public:
    Player(int start_x, int start_y, Sprite player_sprite);

    void Update() override;
    void Draw() override; // Пока не реализовано, поскольку отрисовка централизована в Game
    void SetPosition(int new_x, int new_y);
    void Move(int direction);
    void SetLife(size_t new_life);
    size_t GetLife() const;
    void DecreaseLife();

private:
    size_t life;
    int move_dir = 0;
};

#endif // PLAYER_H
