#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <cstdint>

// Структура для спрайтів, винесена для загального доступу
struct Sprite {
    size_t width, height;
    const uint8_t* data;
};

class GameObject {
public:
    // Віртуальний деструктор важливий для базових класів
    virtual ~GameObject() = default;

    // Загальні методи для всіх об'єктів
    virtual void Update() = 0; // Чиста віртуальна функція, змушує спадкоємців реалізувати її
    virtual void Draw() = 0;   // Чиста віртуальна функція

    int GetX() const { return x; }
    int GetY() const { return y; }
    Sprite GetSprite() const { return sprite; }
    bool IsActive() const { return is_active; }
    void SetActive(bool active) { is_active = active; }

protected:
    int x, y;
    Sprite sprite;
    bool is_active = true;
};

#endif // GAMEOBJECT_H