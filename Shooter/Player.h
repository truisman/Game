#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <vector>
#include "Bullet.h"
#include "Globals.h"
#include "Game.h"

class Bullet;
class Game;

class Player {
public:
    float x, y;
    float vx, vy;
    float angle;
    int width, height;
    SDL_Texture* texture;
    Game* game;
    int health;
    float speed;
    float firingRateFactor;
    Uint32 lastShotTime;
    float speedMultiplier;
    ShootingPattern shootingPattern;
    int level;
    int experience;
    int experienceToNextLevel;
    BulletType bulletType;

    Player(float x, float y, SDL_Texture* selectedTexture, int health, float speed, Game* game);

    void HandleInput(const Uint8* keystate, std::vector<Bullet*>& bullets);
    void Move(float moveSpeed);
    void Rotate(float amount);
    void Render(SDL_Renderer* renderer);
    void Shoot(std::vector<Bullet*>& bullets, ShootingPattern shootingPattern);
    void AddExperience(int amount);
    void LevelUp();
};

#endif
