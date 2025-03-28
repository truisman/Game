#include "Obstacle.h"
#include "Player.h"
#include "Bullet.h"
#include "Game.h"
#include "Globals.h"
#include <cmath>
#include <SDL.h>

Obstacle::Obstacle(float x, float y, float w, float h, SDL_Texture* tex, ObstacleType type, int health)
    : x(x), y(y), width(w), height(h), texture(tex), type(type), health(health), initialY(y), angle(0.0f), lastShotTime(0) {}

SDL_Rect Obstacle::GetRect() const {
    return { (int)x, (int)y, (int)width, (int)height };
}

void Obstacle::Render(SDL_Renderer* renderer, Player* player) {
    SDL_Rect rect = {static_cast<int>(x - player->x + SCREEN_WIDTH / 2), static_cast<int>(y - player->y + SCREEN_HEIGHT / 2), (int)width, (int)height};
    SDL_Point center = { (int)width / 2, (int)height / 2 }; // Center for rotation
    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, &center, SDL_FLIP_NONE); // Use RenderCopyEx
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

    // Keep angle within reasonable bounds if desired (e.g., 0-360)
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle < 0.0f) angle += 360.0f;
}

// --- Helper for Hostile Obstacle behavior ---
void Obstacle::UpdateHostile(Player* player, std::vector<Bullet*>& enemyBullets, Game* game) {
    float dx = player->x - x;
    float dy = player->y - y;
    float distanceSq = dx * dx + dy * dy; // Use squared distance for comparison (faster)
    float shootRangeSq = 700.0f * 700.0f;

    if (distanceSq < shootRangeSq) { // Check if player is within range
        // Rotate towards the player
        angle = atan2(dy, dx) * 180.0f / M_PI + 90.0f; // +90 for sprite orientation

        // Attempt to shoot
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
        if (distance < 700.0f) {
            if (distance > 0) {
                float bulletVX = (dx / distance) * BULLET_SPEED;
                float bulletVY = (dy / distance) * BULLET_SPEED;

                BulletType bulletType = BulletType::NORMAL;

                float spawnX = x + width / 2;
                float spawnY = y + height / 2;

                enemyBullets.push_back(new Bullet(spawnX, spawnY, bulletVX, bulletVY, texture, 20, bulletType));

                lastShotTime = currentTime;
            }
        }
    }
}

void Obstacle::TakeDamage(int damage) {
    if (type == ObstacleType::NEUTRAL) {
        std::cout << "NEUTRAL Obstacle hit! Health before: " << health << ", Damage: " << damage << std::endl;
    }
    health -= damage;
    if (health < 0) health = 0;
    if (type == ObstacleType::NEUTRAL) {
        std::cout << "NEUTRAL Obstacle health after: " << health << std::endl;
    }
}
