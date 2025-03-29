#include "Game.h"
#include "Player.h"
#include "Enemy.h"
#include "Obstacle.h"
#include "Orb.h"
#include "Bullet.h"
#include "StageManager.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <algorithm>

Game::Game() : window(nullptr), renderer(nullptr), isRunning(false), player(nullptr),
               lastEnemySpawnTime(0),
               currentState(GameState::MAIN_MENU),
               stageManager(),
               creditsScrollY(0), creditsStartTime(0),
               selectedMenuOption(0),
               uiFont(nullptr), textColor({255, 255, 255, 255}), highlightColor({255, 255, 0, 255}) {}

Game::~Game() {
    Clean();
}

bool Game::Init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    // 1. Khoi tao SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 2. Khoi tao SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // 3. Khoi tao SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 4. Khoi tao cua so
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 5. Tao Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 6. Vi tri vien dan
    bulletRenderOffsetX = SCREEN_WIDTH / 2;
    bulletRenderOffsetY = SCREEN_HEIGHT / 2;

    // 7. Load texture
    playerTex = IMG_LoadTexture(renderer, "assets/player1.png");

    bulletTexNormal = IMG_LoadTexture(renderer, "assets/bullet_normal.png");
    bulletTexPowered = IMG_LoadTexture(renderer, "assets/bullet_powered.png");
    bulletTexSuperPowered = IMG_LoadTexture(renderer, "assets/bullet_super_powered.png");
    bulletTexExtremePowered = IMG_LoadTexture(renderer, "assets/bullet_extreme_powered.png");
    bulletTexBoss = IMG_LoadTexture(renderer, "assets/bullet_boss.png");

    enemyTexNormal = IMG_LoadTexture(renderer, "assets/enemy_normal.png");
    enemyTexFast = IMG_LoadTexture(renderer, "assets/enemy_fast.png");
    enemyTexTank = IMG_LoadTexture(renderer, "assets/enemy_tank.png");
    enemyTexQuick = IMG_LoadTexture(renderer, "assets/enemy_quick.png");
    enemyTexBoss = IMG_LoadTexture(renderer, "assets/enemy_boss.png");

    neutralObstacleTexture = IMG_LoadTexture(renderer, "assets/obstacle1.png");
    hostileObstacleTexture = IMG_LoadTexture(renderer, "assets/obstacle2.png");

    backgroundTexture = IMG_LoadTexture(renderer, "assets/background.png");
    menuBackgroundTexture = IMG_LoadTexture(renderer, "assets/menu_background.png");

    orbTexture = IMG_LoadTexture(renderer, "assets/orb.png");

    // 8. Kiem tra load texture
    if (!playerTex || !neutralObstacleTexture || !hostileObstacleTexture || !backgroundTexture || !orbTexture) {
        std::cerr << "Failed to load one or more textures: " << IMG_GetError() << std::endl;
        return false;
    }
    if (backgroundTexture) { SDL_QueryTexture(backgroundTexture, NULL, NULL, &backgroundWidth, &backgroundHeight); }

    // 9. Khoi tao phong chu
    uiFont = TTF_OpenFont("assets/arial.ttf", 24);
    if (!uiFont) {
        std::cerr << "Warning: Failed to load font: assets/arial.ttf Error: " << TTF_GetError() << std::endl;
    }
    textColor = { 255, 255, 255, 255 };

    // 10. So ngau nhien
    srand(static_cast<unsigned int>(time(NULL)));

    currentState = GameState::MAIN_MENU;
    selectedMenuOption = 0;
    isRunning = true;

    return true;
}

// Ham render van ban
void Game::RenderText(const std::string& text, int x, int y, bool centered, SDL_Color color) {
    if (!uiFont || text.empty()) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(uiFont, text.c_str(), color);
    if (!textSurface) { std::cerr << "TTF_RenderText Error: " << TTF_GetError() << std::endl; return; }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) { SDL_FreeSurface(textSurface); std::cerr << "CreateTexture Error: " << SDL_GetError() << std::endl; return; }

    SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
    if (centered) {
        renderQuad.x = (SCREEN_WIDTH - textSurface->w) / 2;
    }

    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

void Game::SpawnEnemy(int count) {
    // Man choi
    const StageData& currentStage = stageManager.GetCurrentStageData();
    Uint32 spawnInterval = currentStage.spawnInterval; // Thoi gian 1 lan spawn

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastEnemySpawnTime > spawnInterval)
    {
        int enemiesToSpawn = currentStage.baseSpawnCount; // So luong ke dich
        for (int i = 0; i < enemiesToSpawn; ++i) {
            // Vi tri spawn nguoi choi
            float x = 0.0f, y = 0.0f;
             if (!player) continue;
             float spawnDist = std::max(SCREEN_WIDTH, SCREEN_HEIGHT) * 0.8f;
             float angle = RandomFloat(0, 2 * M_PI);
             x = player->x + cos(angle) * spawnDist;
             y = player->y + sin(angle) * spawnDist;

            // Chon cac loai ke dich
            EnemyType enemyType = EnemyType::NORMAL;
            int randVal = rand() % 1000;

            // Kiem tra stage co spawn Boss khong
            bool spawnBossAttempt = false;
            if (currentStage.canSpawnBoss && randVal < currentStage.bossSpawnChance) {
                 // Chi co 1 Boss tai 1 thoi diem
                 bool bossExists = false;
                 for(const auto& existingEnemy : enemies) {
                     if(existingEnemy && existingEnemy->type == EnemyType::BOSS) {
                         bossExists = true;
                         break;
                     }
                 }
                 if (!bossExists) {
                      enemyType = EnemyType::BOSS;
                      spawnBossAttempt = true;
                 } else {
                     // Neu khong thi spawn 1 con Tank
                     enemyType = EnemyType::TANK;
                 }
            }

            // Chon cac loai ke dich (Khong co Boss)
            if (!spawnBossAttempt) {
                 int cumulativeChance = 0;
                 bool typeSet = false;
                 for (const auto& pair : currentStage.spawnProbabilities) {
                      cumulativeChance += pair.second;
                      if (randVal < cumulativeChance) {
                           enemyType = pair.first;
                           typeSet = true;
                           break;
                      }
                 }
                 if (!typeSet) {
                      enemyType = EnemyType::NORMAL;
                 }
            }

            // Chon texture
            SDL_Texture* selectedEnemyTexture = nullptr;
            SDL_Texture* selectedBulletTexture = nullptr;
            switch (enemyType) {
                case EnemyType::NORMAL: selectedEnemyTexture = enemyTexNormal; selectedBulletTexture = bulletTexNormal; break;
                case EnemyType::FAST:   selectedEnemyTexture = enemyTexFast;   selectedBulletTexture = bulletTexPowered; break;
                case EnemyType::TANK:   selectedEnemyTexture = enemyTexTank;   selectedBulletTexture = bulletTexSuperPowered; break;
                case EnemyType::QUICK:  selectedEnemyTexture = enemyTexQuick;  selectedBulletTexture = bulletTexExtremePowered; break;
                case EnemyType::BOSS:   selectedEnemyTexture = enemyTexBoss;   selectedBulletTexture = bulletTexBoss; break;
            }

            // Tao ke dich
            if (selectedEnemyTexture && selectedBulletTexture) {
                enemies.push_back(new Enemy(x, y, selectedEnemyTexture, player, 100, 1.0f, 1.0f, this, enemyType, selectedBulletTexture));
            }
        }
        lastEnemySpawnTime = currentTime;
    }
}

void Game::SpawnObstacles(int count) {
    for (int i = 0; i < count; i++) {
        // Vi tri bat ky
        float randX = player->x + RandomFloat(-SPAWN_RADIUS, SPAWN_RADIUS);
        float randY = player->y + RandomFloat(-SPAWN_RADIUS, SPAWN_RADIUS);

        float randSize = 100 + (rand() % 125);

        // Kha nang spawn
        ObstacleType type = (rand() % 100 < 80) ? ObstacleType::NEUTRAL : ObstacleType::HOSTILE;

        // Chon texture
        SDL_Texture* tex = nullptr;
        SDL_Texture* obsBulletTex = nullptr;
        int obstacleHealth = 0;
        if (type == ObstacleType::NEUTRAL) {
            tex = neutralObstacleTexture;
            obstacleHealth = 99999;
        } else {
            tex = hostileObstacleTexture;
            obstacleHealth = 200;
            obsBulletTex = bulletTexNormal;
        }

        // Tao chuong ngoai vat
        if (tex) {
            obstacles.push_back(new Obstacle(randX, randY, randSize, randSize, tex, type, obstacleHealth, obsBulletTex));
        } else {
            std::cerr << "Warning: Could not determine texture for obstacle type." << std::endl;
        }
    }
}

// Ham tao so ngau nhien
float Game::RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void Game::StartNewGame() {
    std::cout << "Starting New Game..." << std::endl;
    ResetGameData(); // Xoa du lieu game

    int startingPlayerHealth = 200;

    // Khoi tao nguoi choi
    if (!player) {
        player = new Player(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, playerTex, startingPlayerHealth, PLAYER_SPEED, this);
    } else {
        player->x = SCREEN_WIDTH / 2.0f;
        player->y = SCREEN_HEIGHT / 2.0f;
        player->maxHealth = startingPlayerHealth;
        player->health = player->maxHealth;
        player->level = 1;
        player->experience = 0;
        player->experienceToNextLevel = 50;
        player->bulletType = BulletType::NORMAL;
        player->shootingPattern = ShootingPattern::SINGLE;
        player->firingRateFactor = 4.0f;
    }
    stageManager.StartGame();
    if (stageManager.GetCurrentStageNumber() <= 0) { isRunning = false; return; }

    currentState = GameState::PLAYING;
}

void Game::ResetGameData() {
    std::cout << "Resetting game data..." << std::endl;

    for (auto b : bullets) delete b;
    bullets.clear();

    for (auto eb : enemyBullets) delete eb;
    enemyBullets.clear();

    for (auto e : enemies) delete e;
    enemies.clear();

    for (auto o : obstacles) delete o;
    obstacles.clear();

    for (auto orb : orbs) delete orb;
    orbs.clear();

    lastEnemySpawnTime = 0;
}

void Game::ReturnToMenu() {
    ResetGameData();
    currentState = GameState::MAIN_MENU;
    selectedMenuOption = 0;
    std::cout << "Returning to Main Menu." << std::endl;
}

void Game::TogglePause() {
    if (currentState == GameState::PLAYING) {
        currentState = GameState::PAUSED;
        std::cout << "Game Paused." << std::endl;
    } else if (currentState == GameState::PAUSED) {
        currentState = GameState::PLAYING;
        std::cout << "Game Resumed." << std::endl;
    }
}

void Game::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false; return;
        }

        // Dung game bang nut "P"
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p) {
             if (currentState == GameState::PLAYING || currentState == GameState::PAUSED) {
                  TogglePause();
                  continue;
             }
        }


        // Cac phim chuc nang
        switch (currentState) {
            case GameState::MAIN_MENU: HandleMenuInput(event); break;
            case GameState::PLAYING:
                 if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                      ReturnToMenu();
                      continue;
                 }
                break;
            case GameState::PAUSED:
                 HandlePausedInput(event);
                 break;
            case GameState::GAME_OVER: HandleGameOverInput(event); break;
            case GameState::CREDITS: HandleCreditsInput(event); break;
        }
    }

    if (currentState == GameState::PLAYING) {
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        HandlePlayingInput(keystate);
    }
}

void Game::HandleMenuInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         switch(event.key.keysym.sym) {
             case SDLK_UP:
             case SDLK_w:
                  selectedMenuOption = (selectedMenuOption - 1 + 2) % 2;
                  break;
             case SDLK_DOWN:
             case SDLK_s:
                  selectedMenuOption = (selectedMenuOption + 1) % 2;
                  break;
             case SDLK_RETURN:
             case SDLK_SPACE:
                  if (selectedMenuOption == 0) {
                       StartNewGame();
                  } else if (selectedMenuOption == 1) {
                       isRunning = false;
                  }
                  break;
             case SDLK_ESCAPE:
                  isRunning = false;
                  break;
         }
     }
}

void Game::HandlePlayingInput(const Uint8* keystate) {
    if (player) {
        player->HandleInput(keystate, bullets);
    }
}

void Game::HandlePausedInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         switch(event.key.keysym.sym) {
             case SDLK_p:
                  TogglePause();
                  break;
             case SDLK_ESCAPE:
                  ReturnToMenu();
                  break;
         }
     }
}

void Game::HandleGameOverInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         switch(event.key.keysym.sym) {
             case SDLK_RETURN:
             case SDLK_SPACE:
                  ReturnToMenu();
                  break;
             case SDLK_ESCAPE:
                  isRunning = false;
                  break;
         }
     }
}

void Game::HandleCreditsInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         switch(event.key.keysym.sym) {
             case SDLK_RETURN:
             case SDLK_SPACE:
             case SDLK_ESCAPE:
                  ReturnToMenu();
                  break;
         }
     }
}


void Game::Update() {
     if (!isRunning) return;

     switch (currentState) {
         case GameState::MAIN_MENU:
             break;
         case GameState::PLAYING:
             UpdatePlayingState();
             break;
         case GameState::PAUSED:
             break;
         case GameState::GAME_OVER:
             break;
         case GameState::CREDITS:
             UpdateCreditsState();
             break;
     }
}

void Game::UpdatePlayingState() {

    if (!player) { currentState = GameState::GAME_OVER; return; }

    // --- Stage Advancement Check ---
    if (stageManager.ShouldAdvanceStage()) {
        stageManager.AdvanceStage(player);
        if (stageManager.IsGameWon()) {
            currentState = GameState::CREDITS;
            creditsScrollY = SCREEN_HEIGHT;
            creditsStartTime = SDL_GetTicks();
            ResetGameData();
            return;
        }
    }

    // --- 1. Despawn Out-of-Range Obstacles ---
    for (auto it = obstacles.begin(); it != obstacles.end();) {
        Obstacle* obs = *it;
        float dx = obs->x - player->x;
        float dy = obs->y - player->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > DESPAWN_RADIUS) {
            delete obs;
            it = obstacles.erase(it);
        } else {
            ++it;
        }
    }
	// --- 2. Spawn New Obstacles (Up to MAX_OBSTACLES) ---
    static std::vector<std::vector<bool>> obstacleGrid(NUM_GRID_CELLS, std::vector<bool>(NUM_GRID_CELLS, false));
    static bool gridInitialized = false;

    if (!gridInitialized) {
        // Initialize the grid to all false (empty)
        for (int i = 0; i < NUM_GRID_CELLS; ++i) {
            for (int j = 0; j < NUM_GRID_CELLS; ++j) {
                obstacleGrid[i][j] = false;
            }
        }
        gridInitialized = true;
    }

    int attempts = 0;
    while (obstacles.size() < MAX_OBSTACLES) {
        // 1. Choose a random grid cell.
        int gridX = rand() % NUM_GRID_CELLS;
        int gridY = rand() % NUM_GRID_CELLS;

        // 2. Check if the cell is occupied.
        if (!obstacleGrid[gridX][gridY]) {
            // 3. JITTER the cell's position.
            float cellSize = (2.0f * SPAWN_RADIUS) / NUM_GRID_CELLS;
            float cellX = player->x - SPAWN_RADIUS + gridX * cellSize;
            float cellY = player->y - SPAWN_RADIUS + gridY * cellSize;

            // Add a random offset *within* the cell size.
            float jitterX = RandomFloat(-cellSize * 0.5f, cellSize * 0.5f);
            float jitterY = RandomFloat(-cellSize * 0.5f, cellSize * 0.5f);

            // 4. Calculate spawn position.  Apply the jitter.
            float spawnX = cellX + jitterX;
            float spawnY = cellY + jitterY;
            float randSize = 50 + (rand() % 100); // Random size

			// 5. Ensure minimum distance from *other* obstacles.
            bool tooClose = false;
            for (const auto& otherObs : obstacles) {
                float dx = spawnX - otherObs->x;
                float dy = spawnY - otherObs->y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < MIN_SEPARATION_DISTANCE) {
                    tooClose = true;
                    break;
                }
            }

			if (!tooClose)
			{
				// 6. Determine obstacle type and texture
                ObstacleType type = (rand() % 100 < 80) ? ObstacleType::NEUTRAL : ObstacleType::HOSTILE;
                SDL_Texture* tex = (type == ObstacleType::NEUTRAL) ? neutralObstacleTexture : hostileObstacleTexture;
                SDL_Texture* obsBulletTex = nullptr;
                int obstacleHealth = 0;
                if (type == ObstacleType::NEUTRAL) {
                    tex = neutralObstacleTexture;
                    obstacleHealth = 99999;
                } else {
                    tex = hostileObstacleTexture;
                    obstacleHealth = 150;
                    obsBulletTex = bulletTexNormal;
                }

				// 7. Mark the grid cell as occupied.
                obstacleGrid[gridX][gridY] = true;

				// 8. Spawn the obstacle
                obstacles.push_back(new Obstacle(spawnX, spawnY, randSize, randSize, tex, type, obstacleHealth, obsBulletTex));
                attempts = 0;
			}
			else
			{
                attempts++;
                if (attempts > 100) {
                    return;
                }
			}
        } else {
            attempts++;
            if (attempts > 100) {
                return;
            }
        }
    }
	// --- Clear the grid for the next frame ---
    for (int i = 0; i < NUM_GRID_CELLS; ++i) {
        for (int j = 0; j < NUM_GRID_CELLS; ++j) {
            obstacleGrid[i][j] = false;
        }
    }

    SDL_Rect playerRect = { static_cast<int>(player->x), static_cast<int>(player->y), player->width, player->height };
    const float maxDist = std::max(SCREEN_WIDTH, SCREEN_HEIGHT) * 1.5f;

    // --- Player Bullets vs. Enemies and Obstacles---
     for (auto itB = bullets.begin(); itB != bullets.end();) {
        Bullet* bullet = *itB;
        if (!bullet) { itB = bullets.erase(itB); continue; } // Safety check

        bullet->Update();
        bool bulletRemoved = false;

        if (std::abs(bullet->x - player->x) > maxDist || std::abs(bullet->y - player->y) > maxDist){
           bullet->active = false;
        }

        if(bullet->active) {
            // Check vs Enemies
            for (auto itE = enemies.begin(); itE != enemies.end();) {
                Enemy* enemy = *itE; if (!enemy){ itE = enemies.erase(itE); continue; }
                SDL_Rect enemyRect = {static_cast<int>(enemy->x), static_cast<int>(enemy->y), enemy->width, enemy->height};
                SDL_Rect bulletRect = {static_cast<int>(bullet->x - bullet->width/2.0f), static_cast<int>(bullet->y - bullet->height/2.0f), bullet->width, bullet->height}; // Use centered rect

                if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    enemy->health -= bullet->damage;
                    if (enemy->health <= 0) {
                        stageManager.RecordKill();
                        if (orbTexture) {
                             int orbXp = 10; int orbSize = 15;
                             switch (enemy->type) {
                                case EnemyType::FAST: orbXp = 15; orbSize = 20; break;
                                case EnemyType::QUICK: orbXp = 20; orbSize = 25; break;
                                case EnemyType::TANK: orbXp = 50; orbSize = 30; break;
                                case EnemyType::BOSS: orbXp = 500; orbSize = 40; break;
                             }
                             orbs.push_back(new Orb(enemy->x + enemy->width / 2.0f, enemy->y + enemy->height / 2.0f, orbTexture, orbSize, orbXp));
                        }
                        delete enemy; itE = enemies.erase(itE);
                    } else { ++itE; }
                    bullet->active = false;
                    bulletRemoved = true;
                } else { ++itE; }
            }

            // Check vs Obstacles
            if (!bulletRemoved) {
                for (auto itO = obstacles.begin(); itO != obstacles.end();) {
                    Obstacle* obstacle = *itO; if (!obstacle) { itO = obstacles.erase(itO); continue; }
                    // Use GetRect
                    SDL_Rect obstacleRect = obstacle->GetRect();
                    SDL_Rect bulletRect = {static_cast<int>(bullet->x - bullet->width/2.0f), static_cast<int>(bullet->y - bullet->height/2.0f), bullet->width, bullet->height};

                    if (SDL_HasIntersection(&bulletRect, &obstacleRect)) {
                        obstacle->TakeDamage(bullet->damage);
                        bullet->active = false;
                        bulletRemoved = true;

                        if (obstacle->health <= 0 && obstacle->type != ObstacleType::NEUTRAL) {
                            delete obstacle;
                            itO = obstacles.erase(itO);
                        } else {
                            ++itO;
                        }
                    } else {
                        ++itO;
                    }
                }
            }
        }

        if (!bullet->active) {
            delete bullet;
            itB = bullets.erase(itB);
        } else {
            ++itB;
        }
    }

    // --- Enemy Bullets vs. Player AND Obstacle ---
    for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
        Bullet* bullet = *it;
        if (!bullet) { it = enemyBullets.erase(it); continue; }

        bullet->Update();
        bool bulletRemoved  = false;

        // Check Bounds
        if (std::abs(bullet->x - player->x) > maxDist || std::abs(bullet->y - player->y) > maxDist) {
            bulletRemoved  = true;
        } else {
            // Check vs Player
            SDL_Rect bulletRect = {static_cast<int>(bullet->x - bullet->width/2.0f), static_cast<int>(bullet->y - bullet->height/2.0f), bullet->width, bullet->height};
            if (SDL_HasIntersection(&bulletRect, &playerRect)) {
                player->TakeDamage(bullet->damage);
                if (player->health <= 0) {currentState = GameState::GAME_OVER; delete bullet; it = enemyBullets.erase(it); return; }
                bulletRemoved  = true;
            }

            if (!bulletRemoved ) {
                for (auto itO = obstacles.begin(); itO != obstacles.end();) {
                     Obstacle* obstacle = *itO;
                     if (!obstacle) { itO = obstacles.erase(itO); continue; }

                     SDL_Rect obstacleRect = obstacle->GetRect();
                     if (SDL_HasIntersection(&bulletRect, &obstacleRect)) {
                         obstacle->TakeDamage(bullet->damage);
                         bulletRemoved  = true;

                         if (obstacle->health <= 0 && obstacle->type != ObstacleType::NEUTRAL) {
                              delete obstacle;
                              itO = obstacles.erase(itO); // Erase obstacle
                         } else {
                              ++itO;
                         }
                         break;
                     } else {
                         ++itO;
                     }
                }
            }
        }

        // Cleanup
        if (bulletRemoved ) {
            delete bullet;
            it = enemyBullets.erase(it);
        } else {
            ++it;
        }
    }

    // --- Player vs. Obstacles (Collision) ---
    playerRect = { (int)player->x, (int)player->y, player->width, player->height };
    Uint32 currentTimeForObsDamage = SDL_GetTicks();
    static Uint32 lastObstacleDamageTime = 0;
    const Uint32 OBSTACLE_DAMAGE_COOLDOWN = 1000;

    for (auto itO = obstacles.begin(); itO != obstacles.end();) {
        Obstacle* obstacle = *itO;
        SDL_Rect obstacleRect = obstacle->GetRect();
        if (SDL_HasIntersection(&playerRect, &obstacleRect))
        {
            //Calculate overlap in x and y direction
            int overlapX = 0;
            int overlapY = 0;

            //Right side
            if (player->x < obstacle->x)
            {
                overlapX = playerRect.x + playerRect.w - obstacleRect.x;
            }
            else // Left Side
            {
                overlapX = -(obstacleRect.x + obstacleRect.w - playerRect.x);
            }

            //Down
            if(player->y < obstacle->y)
            {
                overlapY = playerRect.y + playerRect.h - obstacleRect.y;
            }
            else// Up
            {
                overlapY = -(obstacleRect.y + obstacleRect.h - playerRect.y);
            }

            // Move the player by the smallest overlap
            if(abs(overlapX) < abs(overlapY))
            {
                player->x -= overlapX;
            }
            else
            {
                player->y -= overlapY;
            }
            // Hostile Damage
            if (obstacle->type == ObstacleType::HOSTILE) {
                 if (currentTimeForObsDamage - lastObstacleDamageTime > OBSTACLE_DAMAGE_COOLDOWN) {
                      player->TakeDamage(20);
                      lastObstacleDamageTime = currentTimeForObsDamage;
                      if (player->health <= 0) { currentState = GameState::GAME_OVER; return; }
                 }
            }
        }
         ++itO;
    }

    // --- Update Enemies ---
    for (auto& enemy : enemies) {
        enemy->Update(enemies, obstacles, enemyBullets, player, this);
    }

    // --- Update Obstacles ---
    for (auto& obstacle : obstacles) {
        obstacle->Update(player, enemyBullets, this);
    }

    // --- Update Orbs ---
    playerRect = { static_cast<int>(player->x), static_cast<int>(player->y), player->width, player->height }; // Update player rect again
    const float MAGNET_RADIUS_SQ = ORB_MAGNET_RADIUS * ORB_MAGNET_RADIUS;

    for (auto it = orbs.begin(); it != orbs.end(); /* manual increment */) {
        Orb* orb = *it;
        if (!orb) { it = orbs.erase(it); continue; }

        orb->Update(); // Update fading

        if (orb->alpha <= 0) { delete orb; it = orbs.erase(it); continue; } // Remove faded

        // Magnet Logic
        float dx = (player->x + player->width / 2.0f) - orb->x;
        float dy = (player->y + player->height / 2.0f) - orb->y;
        float distSq = dx * dx + dy * dy;
        if (distSq < MAGNET_RADIUS_SQ && distSq > 1.0f) {
            float dist = std::sqrt(distSq);
            orb->x += (dx / dist) * ORB_MAGNET_SPEED; // Use constant
            orb->y += (dy / dist) * ORB_MAGNET_SPEED; // Use constant
        }

        // Collection Check
        SDL_Rect orbRect = orb->GetRect();
        if (SDL_HasIntersection(&playerRect, &orbRect)) {
            player->AddExperience(orb->xpValue); // <<< Use orb's value >>>
            delete orb;
            it = orbs.erase(it);
        } else {
            ++it;
        }
    }

    SpawnEnemy(1);

    if (stageManager.ShouldAdvanceStage()) {
        player->LevelUp();
        stageManager.AdvanceStage(player);
         if (stageManager.IsGameWon()) {
             currentState  = GameState::CREDITS;
             creditsScrollY = SCREEN_HEIGHT;
             creditsStartTime = SDL_GetTicks();
         }
    }
}

void Game::UpdateCreditsState() {
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);

    switch (currentState) {
        case GameState::MAIN_MENU:
            RenderMainMenu();
            break;
        case GameState::PLAYING:
            RenderPlayingState();
            break;
        case GameState::PAUSED:
            RenderPausedScreen();
            break;
        case GameState::GAME_OVER:
            RenderGameOver();
            break;
        case GameState::CREDITS:
            RenderEndCredits();
            break;
    }

    SDL_RenderPresent(renderer);
}

void Game::RenderMainMenu() {
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);
    }

    // Render Title
    RenderText("SPACE SHOOTER", 0, SCREEN_HEIGHT / 5, true, {100, 180, 255, 255});

    // Menu Options
    SDL_Color colorStart = (selectedMenuOption == 0) ? highlightColor : textColor;
    SDL_Color colorQuit = (selectedMenuOption == 1) ? highlightColor : textColor;

    std::string startText = (selectedMenuOption == 0) ? "> Start Game <" : "  Start Game  ";
    std::string quitText = (selectedMenuOption == 1) ? "> Quit Game <" : "  Quit Game  ";

    RenderText(startText, 0, SCREEN_HEIGHT / 2 + 0, true, colorStart);
    RenderText(quitText, 0, SCREEN_HEIGHT / 2 + 60, true, colorQuit);

    // Instructions
    RenderText("W/S or UP/DOWN | ENTER to Select | ESC to Quit", 0, SCREEN_HEIGHT - 60, true, {180, 180, 180, 255});
}

void Game::RenderPlayingState() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);  // Clear window

        // Background
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int bgX = (-static_cast<int>(player->x) % backgroundWidth) + (i * backgroundWidth);
                int bgY = (-static_cast<int>(player->y) % backgroundHeight) + (j * backgroundHeight);
                SDL_Rect bgRect = { bgX, bgY, backgroundWidth, backgroundHeight };
                SDL_RenderCopy(renderer, backgroundTexture, nullptr, &bgRect);
            }
        }

        if (player) {
         for (auto obs : obstacles) { if(obs) obs->Render(renderer, player); }
         for (auto orb : orbs) { if(orb) orb->Render(renderer, player); }
         for (auto enemy : enemies) { if(enemy) enemy->Render(renderer, player); }
         for (auto bullet : bullets) { if(bullet) bullet->Render(renderer, player->x, player->y); }
         for (auto enemyBullet : enemyBullets) { if(enemyBullet) enemyBullet->Render(renderer, player->x, player->y); }
         player->Render(renderer);
     }

     // Game UI
     RenderPlayingUI();
}


void Game::RenderPlayingUI() {
     if (!uiFont || !player) return;

     int barX = 10;
     int barW = 220;
     int barH = 18;
     int spacing = 5;

     // --- Health Bar ---
     int healthBarY = 10;
     float healthPercent = (player->maxHealth > 0) ? static_cast<float>(player->health) / player->maxHealth : 0.0f;
     healthPercent = std::clamp(healthPercent, 0.0f, 1.0f);
     SDL_Color healthColor = {static_cast<Uint8>(200 * (1.0f - healthPercent)), static_cast<Uint8>(200 * healthPercent), 50, 255};

     // Background
     SDL_Rect bgHealthBarRect = {barX, healthBarY, barW, barH};
     SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
     SDL_RenderFillRect(renderer, &bgHealthBarRect);
     // Fill
     SDL_Rect fillHealthBarRect = {barX, healthBarY, static_cast<int>(barW * healthPercent), barH};
     SDL_SetRenderDrawColor(renderer, healthColor.r, healthColor.g, healthColor.b, healthColor.a);
     SDL_RenderFillRect(renderer, &fillHealthBarRect);
     // Border
     SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
     SDL_RenderDrawRect(renderer, &bgHealthBarRect);
     // Health Value
     std::stringstream ssHealth;
     ssHealth << player->health << " / " << player->maxHealth;
     RenderText(ssHealth.str(), barX + barW + spacing, healthBarY, false, textColor);


     // --- Experience Bar ---
     int xpBarY = healthBarY + barH + spacing;
     float xpPercent = (player->experienceToNextLevel > 0) ? static_cast<float>(player->experience) / player->experienceToNextLevel : 0.0f;
     xpPercent = std::clamp(xpPercent, 0.0f, 1.0f);
     // Background
     SDL_Rect bgXpBarRect = {barX, xpBarY, barW, barH};
     SDL_SetRenderDrawColor(renderer, 40, 40, 40, 200);
     SDL_RenderFillRect(renderer, &bgXpBarRect);
     // Fill (XP Color - e.g., Blue)
     SDL_Rect fillXpBarRect = {barX, xpBarY, static_cast<int>(barW * xpPercent), barH};
     SDL_SetRenderDrawColor(renderer, 50, 150, 255, 255);
     SDL_RenderFillRect(renderer, &fillXpBarRect);
     // Border
     SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
     SDL_RenderDrawRect(renderer, &bgXpBarRect);
     // Text (Level)
     std::stringstream ssLevel;
     ssLevel << "Lvl: " << player->level;
     RenderText(ssLevel.str(), barX + barW + spacing, xpBarY, false, textColor);


     // --- Stage Info (Top Right) ---
     if (stageManager.GetCurrentStageNumber() > 0) {
         int stageInfoX = SCREEN_WIDTH - 180;
         int stageInfoY = 10;
         std::stringstream ssStage, ssKills;
         ssStage << "Stage: " << stageManager.GetCurrentStageNumber();
         ssKills << "Kills: " << stageManager.GetCurrentKillCount() << " / " << stageManager.GetCurrentKillGoal();

         RenderText(ssStage.str(), stageInfoX, stageInfoY, false, textColor);
         RenderText(ssKills.str(), stageInfoX, stageInfoY + 25, false, textColor);
     }
}

void Game::RenderPausedScreen() {
    // Paused game
    RenderPlayingState();

    // Dimming Overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect screenRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &screenRect);

    // Render "PAUSED" Text and Instructions
    RenderText("PAUSED", 0, SCREEN_HEIGHT / 3, true, {255, 255, 100, 255});
    RenderText("Press P to Resume", 0, SCREEN_HEIGHT / 2, true, textColor);
    RenderText("Press ESC to Return to Menu", 0, SCREEN_HEIGHT / 2 + 40, true, textColor);
}

void Game::RenderGameOver() {
    // Dimming Render
    RenderPlayingState();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, NULL);

    RenderText("GAME OVER", 0, SCREEN_HEIGHT / 3, true, {255, 50, 50, 255});
    RenderText("Press ENTER or SPACE to return to Menu", 0, SCREEN_HEIGHT / 2, true, textColor);
    RenderText("Press ESC to Quit", 0, SCREEN_HEIGHT / 2 + 40, true, textColor);
}

void Game::RenderEndCredits() {
    if (!uiFont) return;

    Uint32 timeElapsed = SDL_GetTicks() - creditsStartTime;
    // Scrolling from bottom
    float currentScrollPos = SCREEN_HEIGHT - (timeElapsed / 1000.0f * CREDITS_SCROLL_SPEED);

    std::vector<std::string> creditsText = {
        "GAME COMPLETE!",
        "",
        "A 2D Shooter Game",
        "",
        "--- CREDITS ---",
        "Created By: [Ngo Minh Triet]",
        "",
        "Special Thanks:",
        "SDL Libraries",
        "You (The Player!)",
        "",
        "",
        "Thanks for Playing!"
    };

    int currentY = static_cast<int>(currentScrollPos);
    for (const std::string& line : creditsText) {
        // RenderText helper
        RenderText(line, 0, currentY, true, textColor);
        currentY += CREDITS_LINE_HEIGHT;
    }

    if (currentY < -CREDITS_LINE_HEIGHT) {
        ReturnToMenu();
    }
}

void Game::Clean() {
    // Cleanup
    if (player) {
        delete player;
        player = nullptr;
    }

    for (auto bullet : bullets) delete bullet;
    bullets.clear();

    for (auto enemyBullet : enemyBullets) delete enemyBullet;
    enemyBullets.clear();

    for (auto enemy : enemies) delete enemy;
    enemies.clear();

    for (auto obs : obstacles) delete obs;
    obstacles.clear();

    for (auto orb : orbs) delete orb;
    orbs.clear();

    // Destroy textures
    if (playerTex) SDL_DestroyTexture(playerTex);

    if (bulletTexNormal) SDL_DestroyTexture(bulletTexNormal);
    if (bulletTexPowered) SDL_DestroyTexture(bulletTexPowered);
    if (bulletTexSuperPowered) SDL_DestroyTexture(bulletTexSuperPowered);

    if (enemyTexNormal) SDL_DestroyTexture(enemyTexNormal);
    if (enemyTexFast) SDL_DestroyTexture(enemyTexFast);
    if (enemyTexTank) SDL_DestroyTexture(enemyTexTank);
    if (enemyTexQuick) SDL_DestroyTexture(enemyTexQuick);
    if (enemyTexBoss) SDL_DestroyTexture(enemyTexBoss);

    if (neutralObstacleTexture) SDL_DestroyTexture(neutralObstacleTexture);
    if (hostileObstacleTexture) SDL_DestroyTexture(hostileObstacleTexture);

    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (menuBackgroundTexture) SDL_DestroyTexture(menuBackgroundTexture);

    if (orbTexture) SDL_DestroyTexture(orbTexture);

    if (uiFont) {
        TTF_CloseFont(uiFont);
        uiFont = nullptr;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    isRunning = false;
}

bool Game::Running() const {
    return isRunning;
}
