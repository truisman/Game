#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <SDL.h>
#include "Globals.h"
#include "Game.h"

class Player;
class Bullet;
class Game;

class Obstacle {
public:
    float x, y;
    int width, height;
    int health;
    ObstacleType type;

    SDL_Texture* texture;
    SDL_Texture* bulletTexture;
    Uint32 lastShotTime;
    float initialY;
    float angle;

    Obstacle(float x, float y, float w, float h, SDL_Texture* tex, ObstacleType type, int health, SDL_Texture* bulletTex);
    void Update(Player* player, std::vector<Bullet*>& enemyBullets, Game* game);
    void UpdateNeutral(Game* game);
    void UpdateHostile(Player* player, std::vector<Bullet*>& enemyBullets, Game* game);
    void Shoot(std::vector<Bullet*>& enemyBullets, Player* player);
    void Render(SDL_Renderer* renderer, Player* player);
    void TakeDamage(int damage);
    SDL_Rect GetRect() const;
};

#endif
