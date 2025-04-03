#include "Obstacle.h"
#include "Player.h"
#include "Bullet.h"
#include "Game.h"
#include "Globals.h"
#include <cmath>
#include <SDL.h>

Obstacle::Obstacle(float x, float y, float w, float h, SDL_Texture* tex, ObstacleType type_in, int health_in, SDL_Texture* bulletTex)
    : x(x), y(y), width(w), height(h), texture(tex), bulletTexture((type == ObstacleType::HOSTILE) ? bulletTex : nullptr), type(type_in), health(health_in), initialY(y), angle(0.0f), lastShotTime(0) {}

SDL_Rect Obstacle::GetRect() const {
    return { (int)x, (int)y, (int)width, (int)height };
}

void Obstacle::Render(SDL_Renderer* renderer, Player* player) {
    SDL_Rect rect = {static_cast<int>(x - player->x + SCREEN_WIDTH / 2), static_cast<int>(y - player->y + SCREEN_HEIGHT / 2), (int)width, (int)height};
    SDL_Point center = { (int)width / 2, (int)height / 2 };
    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, &center, SDL_FLIP_NONE);
}

void Obstacle::Update(Player* player, std::vector<Bullet*>& enemyBullets, Game* game) {
    switch (type) {
        case ObstacleType::HOSTILE:
            UpdateHostile(player, enemyBullets, game);
            break;
        case ObstacleType::NEUTRAL:
            UpdateNeutral(game);
            break;
    }
}

// --- Helper for Neutral Obstacle behavior ---
void Obstacle::UpdateNeutral(Game* game) {
    Uint32 currentTime = SDL_GetTicks();
    // Hovering motion using sine wave
    y = initialY + sin(currentTime * OBSTACLE_HOVER_SPEED / 1000.0f) * OBSTACLE_HOVER_AMPLITUDE;

    // Gentle random rotation
    float rotationChange = (rand() % 100 < 50 ? 1 : -1) * OBSTACLE_ROTATION_SPEED;
    angle += rotationChange;

    // Keep angle 0->360
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle < 0.0f) angle += 360.0f;
}

// --- Helper for Hostile Obstacle behavior ---
void Obstacle::UpdateHostile(Player* player, std::vector<Bullet*>& enemyBullets, Game* game) {
    if (!player) return;

    float dx = player->x + player->width/2.0f - (x + width/2.0f);
    float dy = player->y + player->height/2.0f - (y + height/2.0f);
    float distanceSq = dx * dx + dy * dy;
    const float shootRangeSq = 700.0f * 700.0f;

    if (distanceSq < shootRangeSq) {
        angle = atan2(dy, dx) * 180.0f / M_PI + 90.0f;
        Shoot(enemyBullets, player);
    }
}

void Obstacle::Shoot(std::vector<Bullet*>& enemyBullets, Player* player) {
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastShotTime > HOSTILE_OBSTACLE_SHOOT_COOLDOWN) {
        float dx = player->x - x;
        float dy = player->y - y;
        float distance = sqrt(dx * dx + dy * dy);

        // 2. Check Range
        if (distance > 0) {
            float normDX = dx / distance;
            float normDY = dy / distance;

            float bulletVX = normDX * BULLET_SPEED;
            float bulletVY = normDY * BULLET_SPEED;

            BulletType bulletType = BulletType::NORMAL;
            int baseDamage = 20;

            float obstacleCenterX = x + width / 2.0f;
            float obstacleCenterY = y + height / 2.0f;

            float spawnOffsetDistance = (std::max(width, height) / 2.0f) + 1.0f;

            float spawnX = obstacleCenterX + normDX * spawnOffsetDistance;
            float spawnY = obstacleCenterY + normDY * spawnOffsetDistance;

            enemyBullets.push_back(new Bullet(spawnX, spawnY, bulletVX, bulletVY, bulletTexture, baseDamage, bulletType));

            lastShotTime = currentTime;
        }
    }
}

void Obstacle::TakeDamage(int damage) {
    health -= damage;
    if (health < 0) health = 0;
}
