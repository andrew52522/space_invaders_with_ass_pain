#ifndef BULLET_H
#define BULLET_H

#include "GameObject.h"

class Bullet : public GameObject {
public:
    Bullet(int start_x, int start_y, int direction, bool super, Sprite bullet_sprite);

    void Update() override;
    void Draw() override;

    bool IsSuper() const;

    //bool IsActive() const;
    //void SetActive(bool status);
    //int GetY() const { return y; }

private:
    int dir;
    bool is_super;
    //bool is_active;
};

#endif // BULLET_H
