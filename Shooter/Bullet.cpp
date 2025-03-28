#include "Bullet.h"
#include "Player.h"
#include <iostream>

Bullet::Bullet(float x, float y, float vx, float vy, SDL_Texture* selectedTexture, int baseDamage, BulletType type)
    : x(x), y(y), vx(vx), vy(vy), width(20), height(20), active(true), texture(selectedTexture), type(type)
    {
        float damageMultiplier = 1.0f;
        switch (type) {
            case BulletType::NORMAL:
                this->width = 20;
                this->height = 20;
                break;
            case BulletType::POWERED:
                this->width = 25;
                this->height = 25;
                damageMultiplier = 2.5f;
                break;
            case BulletType::SUPER_POWERED:
                this->width = 30;
                this->height = 30;
                damageMultiplier = 6.0f;
                break;
            case BulletType::EXTREME_POWERED:
                this->width = 35;
                this->height = 35;
                damageMultiplier = 12.0f;
                break;
        }
        damage = static_cast<int>(baseDamage * damageMultiplier);
    }


void Bullet::Update() {
    float bulletSpeed = BULLET_SPEED;
    switch (type) {
        case BulletType::NORMAL:
            break;
        case BulletType::POWERED:
            bulletSpeed *= 1.5f;
            break;
        case BulletType::SUPER_POWERED:
            bulletSpeed *= 2.0f;
            break;
        case BulletType::EXTREME_POWERED:
            bulletSpeed *= 2.5f;
            break;
    }

    x += vx * bulletSpeed;
    y += vy * bulletSpeed;
}

void Bullet::Render(SDL_Renderer* renderer, float playerX, float playerY) {
    SDL_Rect rect = { static_cast<int>(x - playerX + SCREEN_WIDTH / 2), static_cast<int>(y - playerY + SCREEN_HEIGHT / 2), width, height };
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
}
