// src/Bullet.cpp
#include "Bullet.h"
#include "Player.h"
#include <iostream>

Bullet::Bullet(float x, float y, float vx, float vy, SDL_Texture* texture, int damage, BulletType type)
    : x(x), y(y), vx(vx), vy(vy), width(20), height(20), active(true), texture(texture), damage(damage), type(type){}


void Bullet::Update() {
    float damageMultiplier = 1.0f;
    float bulletSpeed = BULLET_SPEED;
    switch (type) {
        case BulletType::NORMAL:
            break;
        case BulletType::POWERED:
            damageMultiplier = 2.0f;
            bulletSpeed *= 1.5f;
            break;
        case BulletType::SUPER_POWERED:
            damageMultiplier = 5.0f;
            bulletSpeed *= 2.0f;
            break;
    }

    damage = static_cast<int>(damage * damageMultiplier);
    x += vx * bulletSpeed;
    y += vy * bulletSpeed;
}

void Bullet::Render(SDL_Renderer* renderer, float playerX, float playerY) {
    SDL_Rect rect = { static_cast<int>(x - playerX + SCREEN_WIDTH / 2), static_cast<int>(y - playerY + SCREEN_HEIGHT / 2), width, height };
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
}
