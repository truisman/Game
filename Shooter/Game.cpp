#include "Game.h"
#include "Enemy.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

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
    // 1. Initialize SDL Core FIRST
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 2. Initialize SDL_image (after SDL_Init)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // 3. Initialize SDL_ttf (after SDL_Init)
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 4. Create Window
    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 5. Create Renderer (Added VSync flag for smoother rendering)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    // 6. Calculate Bullet Render Offsets (Relative to screen dimensions)
    bulletRenderOffsetX = SCREEN_WIDTH / 2;
    bulletRenderOffsetY = SCREEN_HEIGHT / 2;

    // 7. Load Textures (Check *all* textures)
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

    // Check ALL textures loaded
    if (!playerTex || !neutralObstacleTexture || !hostileObstacleTexture || !backgroundTexture || !orbTexture) {
        std::cerr << "Failed to load one or more textures: " << IMG_GetError() << std::endl;
        return false;
    }
    if (backgroundTexture) { SDL_QueryTexture(backgroundTexture, NULL, NULL, &backgroundWidth, &backgroundHeight); }

    // 8. Load Font (Check return value)
    uiFont = TTF_OpenFont("assets/arial.ttf", 24);
    if (!uiFont) {
        // Log the error, but maybe don't exit immediately unless UI is critical
        std::cerr << "Warning: Failed to load font: assets/arial.ttf Error: " << TTF_GetError() << std::endl;
    }
    textColor = { 255, 255, 255, 255 }; // White

    // 10. Seed Random Number Generator (if using rand()) - Do this ONCE
    srand(static_cast<unsigned int>(time(NULL)));

    currentState = GameState::MAIN_MENU;
    selectedMenuOption = 0;
    isRunning = true;

    return true;
}

// RenderEndCredits and RenderPlayingUI remain largely the same as previous version
void Game::RenderText(const std::string& text, int x, int y, bool centered, SDL_Color color) {
    if (!uiFont) return;

    SDL_Surface* textSurface = TTF_RenderText_Solid(uiFont, text.c_str(), color);
    if (!textSurface) { /* Error */ return; }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) { /* Error */ SDL_FreeSurface(textSurface); return; }

    SDL_Rect renderQuad = { x, y, textSurface->w, textSurface->h };
    if (centered) {
        renderQuad.x = (SCREEN_WIDTH - textSurface->w) / 2; // Center horizontally
    }

    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

void Game::SpawnEnemy(int count) {
    // --- Get Stage Data ---
    const StageData& currentStage = stageManager.GetCurrentStageData();
    Uint32 spawnInterval = currentStage.spawnInterval; // Use interval from stage data

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastEnemySpawnTime > spawnInterval) // Use stage interval
    {
        int enemiesToSpawn = currentStage.baseSpawnCount; // Use base count from stage data
        for (int i = 0; i < enemiesToSpawn; ++i) {
            // --- Determine Spawn Position ---
            float x = 0.0f, y = 0.0f;
             if (!player) continue; // Skip if no player
             float spawnDist = std::max(SCREEN_WIDTH, SCREEN_HEIGHT) * 0.8f;
             float angle = RandomFloat(0, 2 * M_PI);
             x = player->x + cos(angle) * spawnDist;
             y = player->y + sin(angle) * spawnDist;

            // --- Determine Enemy Type using Stage Probabilities ---
            EnemyType enemyType = EnemyType::NORMAL;
            int randVal = rand() % 1000;

            // Check for Boss first if allowed
            bool spawnBossAttempt = false;
            if (currentStage.canSpawnBoss && randVal < currentStage.bossSpawnChance) {
                 // Check if a boss already exists
                 bool bossExists = false;
                 for(const auto& existingEnemy : enemies) {
                     if(existingEnemy && existingEnemy->type == EnemyType::BOSS) {
                         bossExists = true;
                         break;
                     }
                 }
                 if (!bossExists) {
                      enemyType = EnemyType::BOSS;
                      spawnBossAttempt = true; // Mark that we decided on Boss
                 } else {
                     // Fallback if boss exists - spawn a TANK instead
                     enemyType = EnemyType::TANK;
                 }
            }

            // If not spawning boss, determine type based on regular probabilities
            if (!spawnBossAttempt) {
                 int cumulativeChance = 0;
                 bool typeSet = false;
                 // Iterate through probabilities defined for the stage
                 for (const auto& pair : currentStage.spawnProbabilities) {
                      cumulativeChance += pair.second; // Add probability for this type
                      if (randVal < cumulativeChance) {
                           enemyType = pair.first;
                           typeSet = true;
                           break; // Found the type for this roll
                      }
                 }
                 if (!typeSet) {
                      enemyType = EnemyType::NORMAL;
                 }
            }

            // --- Select Textures based on Determined Type ---
            SDL_Texture* selectedEnemyTexture = nullptr;
            SDL_Texture* selectedBulletTexture = nullptr;
            switch (enemyType) {
                case EnemyType::NORMAL: selectedEnemyTexture = enemyTexNormal; selectedBulletTexture = bulletTexNormal; break;
                case EnemyType::FAST:   selectedEnemyTexture = enemyTexFast;   selectedBulletTexture = bulletTexPowered; break;
                case EnemyType::TANK:   selectedEnemyTexture = enemyTexTank;   selectedBulletTexture = bulletTexSuperPowered; break;
                case EnemyType::QUICK:  selectedEnemyTexture = enemyTexQuick;  selectedBulletTexture = bulletTexExtremePowered; break;
                case EnemyType::BOSS:   selectedEnemyTexture = enemyTexBoss;   selectedBulletTexture = bulletTexBoss; break;
            }

            // --- Validate Textures and Create Enemy ---
            if (selectedEnemyTexture && selectedBulletTexture) {
                enemies.push_back(new Enemy(x, y, selectedEnemyTexture, player, 100, 1.0f, 1.0f, this, enemyType, selectedBulletTexture));
            } else { /* Error message */ }

        } // End for loop (enemiesToSpawn)
        lastEnemySpawnTime = currentTime;
    } // End if time check
}

void Game::SpawnObstacles(int count) {
    for (int i = 0; i < count; i++) {
        // Calculate random spawn position
        float randX = player->x + RandomFloat(-SPAWN_RADIUS, SPAWN_RADIUS);
        float randY = player->y + RandomFloat(-SPAWN_RADIUS, SPAWN_RADIUS);

        float randSize = 75 + (rand() % 100);

        // Decide obstacle type
        ObstacleType type = (rand() % 100 < 80) ? ObstacleType::NEUTRAL : ObstacleType::HOSTILE;

        // Select texture and health based on type
        SDL_Texture* tex = nullptr;
        int obstacleHealth = 0;
        if (type == ObstacleType::NEUTRAL) {
            tex = neutralObstacleTexture;
            obstacleHealth = 1000;
        } else {
            tex = hostileObstacleTexture;
            obstacleHealth = 150;
        }

        // Create the obstacle instance
        if (tex) {
            obstacles.push_back(new Obstacle(randX, randY, randSize, randSize, tex, type, obstacleHealth));
        } else {
            std::cerr << "Warning: Could not determine texture for obstacle type." << std::endl;
        }
    }
}

float Game::RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void Game::StartNewGame() {
    std::cout << "Starting New Game..." << std::endl;
    // 1. Clear existing game objects
    ResetGameData();

    // 2. Create Player (if doesn't exist or needs reset)
    if (!player) {
        player = new Player(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, playerTex, 200, PLAYER_SPEED, this); // Reset health/pos
    }

    // 3. Start Stage Manager
    stageManager.StartGame();
    if (stageManager.GetCurrentStageNumber() <= 0) {
         std::cerr << "Error: StageManager failed to initialize correctly for new game." << std::endl;
         isRunning = false; // Stop if stages didn't load
         return;
    }

    // 4. Set Game State
    currentState = GameState::PLAYING;
}

// --- Game Flow Helper ---
void Game::ResetGameData() {
    std::cout << "Resetting game data..." << std::endl;
    // Delete bullets
    for (auto b : bullets) delete b;
    bullets.clear();
    for (auto eb : enemyBullets) delete eb;
    enemyBullets.clear();

    // Delete enemies
    for (auto e : enemies) delete e;
    enemies.clear();

    // Delete obstacles
    for (auto o : obstacles) delete o;
    obstacles.clear();

    // Delete orbs
    for (auto orb : orbs) delete orb;
    orbs.clear();

    // Reset timers/counters related to gameplay
    lastEnemySpawnTime = 0;
    // Don't delete/recreate player here unless necessary, reset state instead
}

// --- Game Flow Helper ---
void Game::ReturnToMenu() {
    ResetGameData(); // Clear game objects
    // Optional: Delete player object? Or just keep it and reset in StartNewGame?
    // if(player) { delete player; player = nullptr; }
    currentState = GameState::MAIN_MENU;
    selectedMenuOption = 0; // Reset menu selection
    std::cout << "Returning to Main Menu." << std::endl;
}

void Game::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Global Quit Event
        if (event.type == SDL_QUIT) {
            isRunning = false;
            return; // Exit event handling immediately
        }

        // State-Specific Input Handling
        switch (currentState) {
            case GameState::MAIN_MENU:
                HandleMenuInput(event);
                break;
            case GameState::PLAYING:
                // Keydown events for specific actions (pause, menu return)
                if (event.type == SDL_KEYDOWN) {
                     switch(event.key.keysym.sym) {
                         case SDLK_ESCAPE: // Escape returns to menu from playing state
                              ReturnToMenu();
                              break;
                         // Add other key presses like Pause here if needed
                     }
                }
                // Continuous state (movement/shooting) handled separately below
                break;
            case GameState::GAME_OVER:
                HandleGameOverInput(event);
                break;
            case GameState::CREDITS:
                HandleCreditsInput(event);
                break;
        }
    }

    // Handle continuous input for PLAYING state (outside event loop)
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
                  selectedMenuOption = (selectedMenuOption - 1 + 2) % 2; // Cycle up (2 options)
                  // Play menu sound effect?
                  break;
             case SDLK_DOWN:
             case SDLK_s:
                  selectedMenuOption = (selectedMenuOption + 1) % 2; // Cycle down (2 options)
                  // Play menu sound effect?
                  break;
             case SDLK_RETURN: // Enter key
             case SDLK_SPACE:
                  if (selectedMenuOption == 0) { // Start Game
                       StartNewGame();
                  } else if (selectedMenuOption == 1) { // Quit
                       isRunning = false;
                  }
                  // Play selection sound effect?
                  break;
             case SDLK_ESCAPE: // Escape also quits from menu
                  isRunning = false;
                  break;
         }
     }
}

void Game::HandlePlayingInput(const Uint8* keystate) {
    // Delegate continuous input to player
    if (player) {
        player->HandleInput(keystate, bullets);
    }
    // Note: Escape key press for returning to menu is handled in the main HandleEvents loop
}

void Game::HandleGameOverInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         switch(event.key.keysym.sym) {
             case SDLK_RETURN: // Enter to return to menu
             case SDLK_SPACE:
                  ReturnToMenu();
                  break;
             case SDLK_ESCAPE: // Escape to quit
                  isRunning = false;
                  break;
         }
     }
}

void Game::HandleCreditsInput(SDL_Event& event) {
     if (event.type == SDL_KEYDOWN) {
         // Any key press (or specific keys) returns to menu
         switch(event.key.keysym.sym) {
             case SDLK_RETURN:
             case SDLK_SPACE:
             case SDLK_ESCAPE:
                  ReturnToMenu();
                  break;
         }
     }
}


// --- Update (Top Level) ---
void Game::Update() {
     if (!isRunning) return; // Skip update if not running

     // State-Specific Updates
     switch (currentState) {
         case GameState::MAIN_MENU:
             // No updates needed for simple menu usually
             break;
         case GameState::PLAYING:
             UpdatePlayingState(); // Contains the main game logic loop
             break;
         case GameState::GAME_OVER:
             // No updates needed for static game over screen
             break;
         case GameState::CREDITS:
             UpdateCreditsState(); // Handles scrolling, potentially ending credits
             break;
     }
}

void Game::UpdatePlayingState() {

    if (!player) { currentState = GameState::GAME_OVER; return; } // Game over if player disappears

    // --- Stage Advancement Check (Do this early?) ---
    if (stageManager.ShouldAdvanceStage()) {
        stageManager.AdvanceStage(player);
        if (stageManager.IsGameWon()) {
            currentState = GameState::CREDITS;
            creditsScrollY = SCREEN_HEIGHT;
            creditsStartTime = SDL_GetTicks();
            ResetGameData(); // Clear game objects for credits screen
            return; // Exit playing state update
        }
    }

    // --- 1. Despawn Out-of-Range Obstacles --- (Same as before)
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
            // 3. JITTER the cell's position. This is the key!
            float cellSize = (2.0f * SPAWN_RADIUS) / NUM_GRID_CELLS; // Width of each cell
            float cellX = player->x - SPAWN_RADIUS + gridX * cellSize;  // Top-left corner of cell
            float cellY = player->y - SPAWN_RADIUS + gridY * cellSize;

            // Add a random offset *within* the cell size.  Up to half the cell size in each direction.
            float jitterX = RandomFloat(-cellSize * 0.5f, cellSize * 0.5f);
            float jitterY = RandomFloat(-cellSize * 0.5f, cellSize * 0.5f);

            // 4. Calculate spawn position.  Apply the jitter.
            float spawnX = cellX + jitterX;
            float spawnY = cellY + jitterY;
            float randSize = 50 + (rand() % 100); // Random size

			// 5. Poisson-Disc Check (Simplified): Ensure minimum distance from *other* obstacles.
            bool tooClose = false;
            for (const auto& otherObs : obstacles) {
                float dx = spawnX - otherObs->x;
                float dy = spawnY - otherObs->y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < MIN_SEPARATION_DISTANCE) {  // Use your constant.
                    tooClose = true;
                    break; // No need to check other obstacles if one is too close.
                }
            }

			if (!tooClose)
			{
				// 6. Determine obstacle type and texture
                ObstacleType type = (rand() % 2 == 0) ? ObstacleType::NEUTRAL : ObstacleType::HOSTILE;
                SDL_Texture* tex = (type == ObstacleType::NEUTRAL) ? neutralObstacleTexture : hostileObstacleTexture;
                int obstacleHealth = 0;
                if (type == ObstacleType::NEUTRAL) {
                    tex = neutralObstacleTexture;
                    obstacleHealth = 1000;
                } else {
                    tex = hostileObstacleTexture;
                    obstacleHealth = 150;
                }

				// 7. Mark the grid cell as occupied.
                obstacleGrid[gridX][gridY] = true;

				// 8. Spawn the obstacle
                obstacles.push_back(new Obstacle(spawnX, spawnY, randSize, randSize, tex, type, obstacleHealth));
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

    // --- Player Bullets vs. Enemies and Obstacles---
    for (auto itB = bullets.begin(); itB != bullets.end();) {
        Bullet* bullet = *itB;
        bullet->Update();

        bool bulletRemoved = false;

        // --- Player Bullets vs. Enemies ---
        for (auto itE = enemies.begin(); itE != enemies.end();) {
            Enemy* enemy = *itE;
            SDL_Rect enemyRect = {static_cast<int>(enemy->x), static_cast<int>(enemy->y), enemy->width, enemy->height};
            SDL_Rect bulletRect = {static_cast<int>(bullet->x), static_cast<int>(bullet->y), bullet->width, bullet->height};

            if (SDL_HasIntersection(&bulletRect, &enemyRect)) {
                    enemy->health -= bullet->damage;
                    if (enemy->health <= 0) {
                        stageManager.RecordKill();
                        // Orb spawning logic (from previous step)
                        if ((rand() % 100) < 40 && orbTexture) {
                             int orbXp = 10; int orbSize = 15;
                             switch (enemy->type) {
                                case EnemyType::FAST: orbXp = 15; orbSize = 20; break;
                                case EnemyType::QUICK: orbXp = 20; orbSize = 25; break;
                                case EnemyType::TANK: orbXp = 50; orbSize = 30; break;
                                case EnemyType::BOSS: orbXp = 500; orbSize = 40; break;
                                default: break;
                             }
                             orbs.push_back(new Orb(enemy->x + enemy->width / 2.0f, enemy->y + enemy->height / 2.0f, orbTexture, orbSize, orbXp));
                        }
                        delete enemy;
                        itE = enemies.erase(itE);
                    } else {
                        ++itE;
                    }
                    bullet->active = false;
                    bulletRemoved = true;
                } else {
                    ++itE;
                }
            }

        // Only check obstacles if the bullet hasn't already hit an enemy
        if (!bulletRemoved) {
            // --- Player Bullets vs. Obstacles ---
            for (auto itO = obstacles.begin(); itO != obstacles.end();) {
                Obstacle* obstacle = *itO;
                SDL_Rect obstacleRect = {static_cast<int>(obstacle->x), static_cast<int>(obstacle->y), static_cast<int>(obstacle->width), static_cast<int>(obstacle->height)};
                SDL_Rect bulletRect = {static_cast<int>(bullet->x), static_cast<int>(bullet->y), bullet->width, bullet->height};

                if (SDL_HasIntersection(&bulletRect, &obstacleRect)) {
                    obstacle->TakeDamage(bullet->damage); // Use TakeDamage
                    bullet->active = false;
                    bulletRemoved = true; // Mark as removed

                    if (obstacle->health <= 0) {
                        delete obstacle;
                        itO = obstacles.erase(itO);
                    } else {
                        ++itO;
                    }
                    break; // Exit obstacle loop after collision
                } else {
                    ++itO;
                }
            }
        }

        if (bullet->x < player->x - SCREEN_WIDTH|| bullet->x > player->x + SCREEN_WIDTH || bullet->y < player->y - SCREEN_HEIGHT || bullet->y > player->y + SCREEN_HEIGHT){
           bullet->active = false;
        }

        // Remove bullet AFTER checking against both enemies and obstacles
        if (!bullet->active) {
            delete bullet;
            itB = bullets.erase(itB);
        } else {
            ++itB;
        }
    }
    // --- Enemy Bullets vs. Player ---
    for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
        Bullet* bullet = *it;
        bullet->Update();
        SDL_Rect bulletRect = {static_cast<int>(bullet->x), static_cast<int>(bullet->y), bullet->width, bullet->height};
        SDL_Rect playerRect = {static_cast<int>(player->x), static_cast<int>(player->y), player->width, player->height};
        if (SDL_HasIntersection(&bulletRect, &playerRect))
        {
            player->health -= bullet->damage;
            std::cout << "Player hit! Health: " << player->health << std::endl;

            if (player->health <= 0) {
                std::cout << "Game Over! Player health depleted." << std::endl;
                currentState = GameState::GAME_OVER;
                return;
            }
            delete bullet;
            it = enemyBullets.erase(it);
        } else {
            ++it;
        }
    }
    // --- Player vs. Obstacles (Collision) ---
    SDL_Rect playerRect = { (int)player->x, (int)player->y, player->width, player->height };
    for (auto itO = obstacles.begin(); itO != obstacles.end();) {
        Obstacle* obstacle = *itO;
        SDL_Rect obstacleRect = obstacle->GetRect();
        if (SDL_HasIntersection(&playerRect, &obstacleRect))
        {
            //Simple push-back collision resolution:
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

            //Move the player by the smallest overlap
            if(abs(overlapX) < abs(overlapY))
            {
                player->x -= overlapX;
            }
            else
            {
                player->y -= overlapY;
            }
            // --- Obstacle damage to Player ---
            if (obstacle->type == ObstacleType::HOSTILE) //Only take hit when the obstacle is hostile
            {
                 player->health -= 1; // Reduce player health (adjust damage as needed)
                if (player->health <= 0) {
                    std::cout << "Game Over! Player hit by obstacle." << std::endl;
                    currentState = GameState::GAME_OVER;
                    return;
                 }
            }
        }
         ++itO; //Always increment since there's no removal here
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
    for (auto it = orbs.begin(); it != orbs.end();) {
        Orb* orb = *it;
        orb->Update();
        SDL_Rect playerRect = {static_cast<int>(player->x), static_cast<int>(player->y), player->width, player->height};
        SDL_Rect orbRect = orb->GetRect();

        if (SDL_HasIntersection(&playerRect, &orbRect)) {
            player->AddExperience(10); // Add 10 experience to the player on collection
            delete orb;
            it = orbs.erase(it);
        } else {
            ++it;
        }
    }
    SpawnEnemy(1);

    if (stageManager.ShouldAdvanceStage()) {
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
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255); // Default background
    SDL_RenderClear(renderer);

    // State-Specific Rendering
    switch (currentState) {
        case GameState::MAIN_MENU:
            RenderMainMenu();
            break;
        case GameState::PLAYING:
            RenderPlayingState();
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

// --- State-Specific Render Helpers ---
void Game::RenderMainMenu() {
    // Optional: Render background
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL); // Stretch to screen
    } else {
        // Just clear to background color (already done)
    }

    // Render Title
    RenderText("2D SHOOTER GAME", 0, SCREEN_HEIGHT / 4, true, {200, 220, 255, 255}); // Centered, light blue

    // Render Options
    SDL_Color colorStart = (selectedMenuOption == 0) ? highlightColor : textColor;
    SDL_Color colorQuit = (selectedMenuOption == 1) ? highlightColor : textColor;

    RenderText("Start Game", 0, SCREEN_HEIGHT / 2 + 0, true, colorStart);
    RenderText("Quit", 0, SCREEN_HEIGHT / 2 + 50, true, colorQuit);

    RenderText("Use W/S or UP/DOWN to select, ENTER to confirm", 0, SCREEN_HEIGHT - 50, true, {150, 150, 150, 255});
}

void Game::RenderPlayingState() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);  //Xoa man hinh

        // Draw background
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int bgX = (-static_cast<int>(player->x) % backgroundWidth) + (i * backgroundWidth);
                int bgY = (-static_cast<int>(player->y) % backgroundHeight) + (j * backgroundHeight);
                SDL_Rect bgRect = { bgX, bgY, backgroundWidth, backgroundHeight };
                SDL_RenderCopy(renderer, backgroundTexture, nullptr, &bgRect);
            }
        }

        if (player) {
         // Render order matters (background -> obstacles -> orbs -> enemies -> bullets -> player)
         for (auto obs : obstacles) { if(obs) obs->Render(renderer, player); }
         for (auto orb : orbs) { if(orb) orb->Render(renderer, player); }
         for (auto enemy : enemies) { if(enemy) enemy->Render(renderer, player); }
         for (auto bullet : bullets) { if(bullet) bullet->Render(renderer, player->x, player->y); }
         for (auto enemyBullet : enemyBullets) { if(enemyBullet) enemyBullet->Render(renderer, player->x, player->y); }
         player->Render(renderer);
     }

     // --- Draw Gameplay UI ---
     RenderPlayingUI();
}


void Game::RenderPlayingUI() {
     if (uiFont && player && stageManager.GetCurrentStageNumber() > 0) {
        // Prepare Text Strings
        std::stringstream ssHealth, ssLevel, ssExp;
        ssHealth << "Health: " << player->health;
        ssLevel << "Level: " << player->level; // Assuming player has 'level' member
        ssExp << "XP: " << player->experience << " / " << player->experienceToNextLevel; // Assuming these members

        // Use the helper function
        RenderText(ssHealth.str(), 10, 10); // Position: top-left
        RenderText(ssLevel.str(), 10, 40);  // Position: below health
        RenderText(ssExp.str(), 10, 70);   // Position: below level

        // Optional: Experience Bar
        int barX = 10;
        int barY = 100;
        int barW = 200;
        int barH = 15;
        float xpPercent = static_cast<float>(player->experience) / player->experienceToNextLevel;
        if (xpPercent < 0.0f) xpPercent = 0.0f;
        if (xpPercent > 1.0f) xpPercent = 1.0f; // Clamp between 0 and 1

        // Background of the bar
        SDL_Rect bgBarRect = {barX, barY, barW, barH};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark grey
        SDL_RenderFillRect(renderer, &bgBarRect);

        // Filled portion of the bar
        SDL_Rect fillBarRect = {barX, barY, static_cast<int>(barW * xpPercent), barH};
        SDL_SetRenderDrawColor(renderer, 0, 200, 50, 255); // Green
        SDL_RenderFillRect(renderer, &fillBarRect);

        // Border of the bar
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light grey
        SDL_RenderDrawRect(renderer, &bgBarRect);

         std::stringstream ssStage, ssKills;
         ssStage << "Stage: " << stageManager.GetCurrentStageNumber();
         ssKills << "Kills: " << stageManager.GetCurrentKillCount() << " / " << stageManager.GetCurrentKillGoal();

         RenderText(ssStage.str(), SCREEN_WIDTH - 150, 10); // Top-right
         RenderText(ssKills.str(), SCREEN_WIDTH - 150, 40);
     }
}

void Game::RenderGameOver() {
     // Render a faded version of the playing state underneath?
     RenderPlayingState();
     SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); // Semi-transparent black overlay
     SDL_RenderFillRect(renderer, NULL); // Cover screen

     // Render centered "GAME OVER" text
     RenderText("GAME OVER", 0, SCREEN_HEIGHT / 2 - 40, true, {0, 255, 0, 255}); // Red
     RenderText("Press ENTER or SPACE to return to Menu", 0, SCREEN_HEIGHT / 2 + 20, true);
}

void Game::RenderEndCredits() {
    if (!uiFont) return;

    Uint32 timeElapsed = SDL_GetTicks() - creditsStartTime;
    // Correct calculation for scrolling UP from bottom
    float currentScrollPos = SCREEN_HEIGHT - (timeElapsed / 1000.0f * CREDITS_SCROLL_SPEED);

    std::vector<std::string> creditsText = {
        "GAME COMPLETE!",
        "",
        "A 2D Shooter Game",
        "",
        "--- LORE ---",
        "In the vast expanse...",
        "a lone pilot...",
        "...faced the endless swarm.",
        "Their journey ends, but the echo remains.",
        "",
        "--- CREDITS ---",
        "Created By: [Your Name / Alias Here]", // <<< CHANGE THIS
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
        // Use the enhanced RenderText helper
        RenderText(line, 0, currentY, true, textColor); // Centered, default color
        currentY += CREDITS_LINE_HEIGHT;
    }

    // Optional: Logic to return to menu after credits scroll off screen
    if (currentY < -CREDITS_LINE_HEIGHT) {
        ReturnToMenu();
    }
}

void Game::Clean() {
    // Cleanup in reverse order of creation
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
