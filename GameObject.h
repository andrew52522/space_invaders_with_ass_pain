#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <cstdint>

// Структура для спрайтов, вынесенная для общего доступа
struct Sprite {
    size_t width, height;
    const uint8_t* data;
};

class GameObject {
public:
    // Виртуальный деструктор важен для базовых классов
    virtual ~GameObject() = default;

    // Общие методы для всех объектов
    virtual void Update() = 0; // Чистая виртуальная функция, заставляет наследников реализовать её
    virtual void Draw() = 0;   // Чистая виртуальная функция

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
