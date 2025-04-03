#ifndef GAME_H
#define GAME_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include "Globals.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Obstacle.h"
#include "Orb.h"
#include "StageManager.h"

class Player;
class Bullet;
class Enemy;
class Obstacle;
class Orb;

// --- GAME STATES ---
enum class GameState {
    MAIN_MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    CREDITS
};

class Game {
public:
    SDL_Texture* bulletTexNormal;
    SDL_Texture* bulletTexPowered;
    SDL_Texture* bulletTexSuperPowered;
    SDL_Texture* bulletTexExtremePowered;
    SDL_Texture* bulletTexBoss;


    Game();
    ~Game();

    bool Init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
    void HandleEvents();
    void Update();
    void Render();
    void Clean();
    bool Running() const;

    float RandomFloat(float min, float max);
    bool LoadSounds();
    SDL_Renderer* GetRenderer();

    void PlaySoundEffect(Mix_Chunk* sound);

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool isRunning;
    Uint32 lastEnemySpawnTime;
    int bulletRenderOffsetX;
    int bulletRenderOffsetY;
    int backgroundWidth;
    int backgroundHeight;

    // --- Game State ---
    GameState currentState;

    // --- Game Objects ---
    Player* player;
    std::vector<Bullet*> bullets;
    std::vector<Enemy*> enemies;
    std::vector<Bullet*> enemyBullets;
    std::vector<Obstacle*> obstacles;
    std::vector<Orb*> orbs;

    // --- Managers ---
    StageManager stageManager;

    // --- Textures ---
    SDL_Texture* playerTex;
    SDL_Texture* enemyTexNormal;
    SDL_Texture* enemyTexFast;
    SDL_Texture* enemyTexTank;
    SDL_Texture* enemyTexQuick;
    SDL_Texture* enemyTexBoss;
    SDL_Texture* neutralObstacleTexture;
    SDL_Texture* hostileObstacleTexture;
    SDL_Texture* backgroundTexture;
    SDL_Texture* orbTexture;
    SDL_Texture* menuBackgroundTexture;

    // ---Sound---
    Mix_Music* backgroundMusic;
    Mix_Chunk* playerShootSound;
    Mix_Chunk* enemyShootSound;
    Mix_Chunk* enemyDeathSound;
    Mix_Chunk* playerDeathSound;

    // --- UI / Font ---
    TTF_Font* uiFont;
    SDL_Color textColor;
    SDL_Color highlightColor;

    // --- Credits State ---
    float creditsScrollY;
    Uint32 creditsStartTime;

    // --- Menu State ---
    int selectedMenuOption;

    // Other Helpers
    void SpawnEnemy(int count);
    void SpawnObstacles(int count);
    void RenderText(const std::string& text, int x, int y, bool centered = false, SDL_Color color = {255, 255, 255, 255});

    // Game Flow Helpers
    void StartNewGame();
    void ResetGameData();
    void ReturnToMenu();
    void TogglePause();

    // --- Private Methods ---
    void HandleMenuInput(SDL_Event& event);
    void HandlePlayingInput(const Uint8* keystate);
    void HandlePausedInput(SDL_Event& event);
    void HandleGameOverInput(SDL_Event& event);
    void HandleCreditsInput(SDL_Event& event);

    void UpdatePlayingState();
    void UpdateCreditsState();

    void RenderMainMenu();
    void RenderPlayingState();
    void RenderPlayingUI();
    void RenderPausedScreen();
    void RenderGameOver();
    void RenderEndCredits();
};

#endif
