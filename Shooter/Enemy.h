#ifndef ENEMY_H
#define ENEMY_H

#include "Bullet.h"

class Game;
class Player;
class Obstacle;
class Bullet;

class Enemy {
public:
    float x, y;
    float vx, vy;
    float angle;
    int width, height;

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

    Enemy(float x, float y, SDL_Texture* selectedTexture, Player* target, int health, float speedFactor, float firingRateFactor, Game* game, EnemyType type, SDL_Texture* selectedBulletTexture);
    void Update(std::vector<Enemy*>& enemies, std::vector<Obstacle*>& obstacles, std::vector<Bullet*>& enemyBullets, Player* player, Game* game);
    void Shoot(std::vector<Bullet*>& enemyBullets);
    void Render(SDL_Renderer* renderer, Player* player);
    SDL_Texture* texture;
    SDL_Texture* bulletTexture;
};

#endif // ENEMY_H
