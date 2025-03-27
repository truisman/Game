// include/Game.h
#ifndef GAME_H
#define GAME_H

#include "Globals.h"  // For common includes
#include "Player.h" // Include Player
#include "Bullet.h" // Include Bullet
#include "Enemy.h"  // Include Enemy
#include "Obstacle.h"  // Include Obstacle
#include "Orb.h"


class Game {
public:
    SDL_Window* window;
    SDL_Renderer* renderer;
    int bulletRenderOffsetX;
    int bulletRenderOffsetY;

    bool isRunning;
    Player* player;
    SDL_Texture* playerTex;
    SDL_Texture* bulletTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* neutralObstacleTexture;
    SDL_Texture* hostileObstacleTexture;
    SDL_Texture* backgroundTexture;
    SDL_Texture* orbTexture;

    std::vector<Orb*> orbs;
    std::vector<Bullet*> bullets;
    std::vector<Bullet*> enemyBullets;
    std::vector<Enemy*> enemies;
    std::vector<Obstacle*> obstacles;
    Uint32 lastEnemySpawnTime;


    Game();
    ~Game(); // Add destructor

    bool Init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
    void SpawnEnemy(int count);
    void SpawnObstacles(int count);
    float RandomFloat(float min, float max);
    bool CheckObstacleOverlap(float x, float y, float size);
    bool CheckNeutralSeparation(float x, float y);
    void HandleEvents();
    void Update();
    void Render();
    void Clean();
    bool Running() const;
};

#endif // GAME_H
