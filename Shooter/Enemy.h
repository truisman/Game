// include/Enemy.h
#ifndef ENEMY_H
#define ENEMY_H

#include "Globals.h"
#include "Bullet.h"

class Game;
class Player;
class Obstacle;

class Enemy {
public:
    // --- Member Variables ---
    float x, y;
    float vx, vy;
    float angle;
    int width, height;
    SDL_Texture* texture;
    Player* target;
    int health;
    float speed;
    float firingRateFactor;
    Uint32 lastShotTime;

    EnemyType type;
    EnemyState state;
    Uint32 lastStateChange;
    float wanderingAngle;
    float circlingDirection;

    // --- Methods ---
    Enemy(float x, float y, SDL_Texture* tex, Player* target, int health, float speed, float firingRateFactor, Game* game, EnemyType type);
    void Update(std::vector<Enemy*>& enemies, std::vector<Obstacle*>& obstacles, std::vector<Bullet*>& enemyBullets, Player* player, Game* game);
    void Shoot(std::vector<Bullet*>& enemyBullets); // Keep this taking the vector
    void Render(SDL_Renderer* renderer, Player* player);
};

#endif // ENEMY_H
