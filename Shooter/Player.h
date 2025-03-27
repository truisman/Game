// include/Player.h
#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <vector>
#include "Bullet.h"
#include "Globals.h"

class Bullet; // Forward declaration

class Player {
public:
    float x, y;
    float vx, vy;
    float angle;
    int width, height;
    SDL_Texture* texture;
    int health;
    float speed;
    float firingRateFactor;
    Uint32 lastShotTime;
    float speedMultiplier;
    float rotation;
    ShootingPattern shootingPattern;

    int level;
    int experience;
    int experienceToNextLevel;
    BulletType bulletType;

    Player(float x, float y, SDL_Texture* tex, int health, float speed);

    void HandleInput(const Uint8* keystate, std::vector<Bullet*>& bullets, SDL_Texture* bulletTexture);
    void Move(float speed);
    void Rotate(float amount);
    void Render(SDL_Renderer* renderer);
    void Shoot(std::vector<Bullet*>& bullets, SDL_Texture* bulletTexture, ShootingPattern shootingPattern);
    void AddExperience(int amount);
    void LevelUp();
};

#endif // PLAYER_Ha
