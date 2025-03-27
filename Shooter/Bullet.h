#ifndef BULLET_H
#define BULLET_H

#include <SDL.h>
#include "Globals.h"
#include "Player.h"

class Bullet {
public:
    float x, y;
    float vx, vy;
    int width, height;
    bool active;
    SDL_Texture* texture;
    int damage;
	BulletType type;

    Bullet(float x, float y, float vx, float vy, SDL_Texture* texture, int damage, BulletType type);
    void Update();
    void Render(SDL_Renderer* renderer, float playerX, float playerY);
};

#endif
