// include/Obstacle.h
#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "Globals.h" // For common includes
#include "Game.h"
class Player;  // Forward declaration
class Bullet; // Forward declare Bullet

class Obstacle {
public:
    float x, y, width, height;
    SDL_Texture* texture;
    ObstacleType type;
    Uint32 lastShotTime;
    int health;
    float initialY;
    float angle;

    Obstacle(float x, float y, float w, float h, SDL_Texture* tex, ObstacleType typeint, int health = 100); // Modified constructor
    SDL_Rect GetRect() const;
    void Render(SDL_Renderer* renderer, Player* player);
    void Update(Player* player, std::vector<Bullet*>& enemyBullets, Game* game); // Add Update method for shooting
    void Shoot(std::vector<Bullet*>& enemyBullets, Player* player);  // Add Shoot method
    void TakeDamage(int damage);
private:
    Game* game;
};

#endif // OBSTACLE_H
