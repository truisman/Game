#include "Obstacle.h"
#include "Player.h"
#include "Bullet.h" // Include Bullet.h
#include "Game.h"
#include "Globals.h"

Obstacle::Obstacle(float x, float y, float w, float h, SDL_Texture* tex, ObstacleType type, int health) // ADD health parameter
    : x(x), y(y), width(w), height(h), texture(tex), type(type), lastShotTime(0), health(health), initialY(y) {}

SDL_Rect Obstacle::GetRect() const {
    return { (int)x, (int)y, (int)width, (int)height };
}

void Obstacle::Render(SDL_Renderer* renderer, Player* player) {
    SDL_Rect rect = {static_cast<int>(x - player->x + SCREEN_WIDTH / 2), static_cast<int>(y - player->y + SCREEN_HEIGHT / 2), (int)width, (int)height};
    SDL_Point center = { (int)width / 2, (int)height / 2 }; // Center for rotation
    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle, &center, SDL_FLIP_NONE); // Use RenderCopyEx
}

void Obstacle::Update(Player* player, std::vector<Bullet*>& enemyBullets, Game* game) {
    if (type == ObstacleType::HOSTILE) {
        // Hostile obstacle: Rotate head towards the player
        float dx = player->x - x;
        float dy = player->y - y;
        float distance = sqrt(dx*dx + dy*dy);
        if(distance < 700.0f){ // Check if is player nearby
            angle = atan2(dy, dx) * 180.0f / M_PI + 90.0f;  // Calculate angle, +90 for correct orientation
            Shoot(enemyBullets, player);
        }
    } else if (type == ObstacleType::NEUTRAL) {
        // Neutral obstacle: Hover and rotate
        Uint32 currentTime = SDL_GetTicks();
        y = initialY + sin(currentTime * OBSTACLE_HOVER_SPEED / 1000.0f) * OBSTACLE_HOVER_AMPLITUDE; //Hovering

        // Random rotation
        angle += (rand() % 100 < 50 ? 1 : -1) * OBSTACLE_ROTATION_SPEED; // 50% chance to rotate left or right
    }
}

void Obstacle::Shoot(std::vector<Bullet*>& enemyBullets, Player* player) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastShotTime > HOSTILE_OBSTACLE_SHOOT_COOLDOWN) {
        // Basic shooting logic (similar to Enemy)
        float dx = player->x - x;
        float dy = player->y - y;
        float distance = sqrt(dx * dx + dy * dy);
        if(distance < 700.0f) { //Check if the player are nearby
            float bulletVX = (dx / distance) * BULLET_SPEED;  // Normalize and scale
            float bulletVY = (dy / distance) * BULLET_SPEED;

            BulletType bulletType = BulletType::NORMAL;
            enemyBullets.push_back(new Bullet(x + width / 2, y + height / 2, bulletVX, bulletVY, texture, 20, bulletType));
            lastShotTime = currentTime;
        }
    }
}

void Obstacle::TakeDamage(int damage) {
    health -= damage;
    if (health < 0) health = 0; // Prevent negative health
}
