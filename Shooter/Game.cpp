// src/Game.cpp
#include "Game.h"
#include "Enemy.h"
#include <limits>

Game::Game() : window(nullptr), renderer(nullptr), isRunning(false), player(nullptr), lastEnemySpawnTime(0) {}

Game::~Game() {
    Clean(); // Ensure proper cleanup in destructor
}

bool Game::Init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;

    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) return false;

    bulletRenderOffsetX = SCREEN_WIDTH / 2;
    bulletRenderOffsetY = SCREEN_HEIGHT / 2;

    playerTex = IMG_LoadTexture(renderer, "assets/player1.png");
    bulletTexture = IMG_LoadTexture(renderer, "assets/bullet1.png");
    enemyTexture = IMG_LoadTexture(renderer, "assets/enemy1.png");
    neutralObstacleTexture = IMG_LoadTexture(renderer, "assets/obstacle1.png");
    hostileObstacleTexture = IMG_LoadTexture(renderer, "assets/obstacle2.png");
    backgroundTexture = IMG_LoadTexture(renderer, "assets/background1.png");
    orbTexture = IMG_LoadTexture(renderer, "assets/orb.png");
    if (!playerTex || !bulletTexture || !enemyTexture || !neutralObstacleTexture || !hostileObstacleTexture || !backgroundTexture) {
        std::cerr << "Failed to load textures: " << IMG_GetError() << std::endl;
        return false;
    }

    player = new Player(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, playerTex, 99999, PLAYER_SPEED);

    isRunning = true;
    return true;
}

void Game::SpawnEnemy(int count) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastEnemySpawnTime > ENEMY_SPAWN_INTERVAL)
    {
        for (int i = 0; i < count; ++i) { // Loop to spawn 'count' enemies
            int side = rand() % 4;
            float x, y;
            switch (side) {
                case 0: x = rand() % SCREEN_WIDTH; y = -100; break;
                case 1: x = rand() % SCREEN_WIDTH; y = SCREEN_HEIGHT + 100; break;
                case 2: x = -100; y = rand() % SCREEN_HEIGHT; break;
                case 3: x = SCREEN_WIDTH + 100; y = rand() % SCREEN_HEIGHT; break;
            }

            EnemyType enemyType;
            int randVal = rand() % 100;
            if (randVal < 60) {
                enemyType = EnemyType::NORMAL;
            } else if (randVal < 90) {
                enemyType = EnemyType::FAST;
            } else if (randVal < 95) {
                enemyType = EnemyType::QUICK;
            } else {
                enemyType = EnemyType::TANK;
            }

            enemies.push_back(new Enemy(x, y, enemyTexture, player, 100, 1.0f, 1.0f, this, enemyType));
        }
        lastEnemySpawnTime = currentTime;
    }
}

void Game::SpawnObstacles(int count) {
    for (int i = 0; i < count; i++) {
        float randX = rand() % SCREEN_WIDTH;
        float randY = rand() % SCREEN_HEIGHT;
        float randSize = 75 + (rand() % 100);
        ObstacleType type = (rand() % 2 == 0) ? ObstacleType::NEUTRAL : ObstacleType::HOSTILE;
        const int obstacleHealth = (type == ObstacleType::NEUTRAL) ? 200 : 50; // Different health values

        SDL_Texture* tex = (type == ObstacleType::NEUTRAL) ? neutralObstacleTexture : hostileObstacleTexture;

        obstacles.push_back(new Obstacle(randX, randY, randSize, randSize, tex, type, obstacleHealth));
    }
}

float Game::RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

bool Game::CheckNeutralSeparation(float x, float y) {
    for (const auto& obs : obstacles) {
        if (obs->type == ObstacleType::NEUTRAL) { // Only check against other NEUTRAL obstacles
            float dx = x - obs->x;
            float dy = y - obs->y;
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < MIN_SEPARATION_DISTANCE) {
                return false; // Too close to another neutral obstacle
            }
        }
    }
    return true; // Sufficiently far from all *neutral* obstacles.
}

void Game::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
        }
    }

    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    // Now, just call Player::HandleInput, passing the bullets vector.
    player->HandleInput(keystate, bullets, bulletTexture);
}

void Game::Update() {

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
                int obstacleHealth = (type == ObstacleType::NEUTRAL) ? 200 : 50;

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
                    //std::cout << "Failed to spawn obstacle after multiple attempts." << std::endl; // Debugging
                    return;  // Give up for this frame.
                }
			}
        } else {
            // Cell is occupied.  Try again.
            attempts++;
            if (attempts > 100) {
                return;  // Give up for this frame.
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
                    if (rand() % 100 < 35) {
                        orbs.push_back(new Orb(enemy->x, enemy->y, orbTexture));
                    }
                    delete enemy;
                    itE = enemies.erase(itE);
                } else {
                    ++itE;
                }
                bullet->active = false;
                bulletRemoved = true;
                break; // Exit enemy loop after hitting an enemy
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
                std::cout << "Game Over!" << std::endl;
                isRunning = false;
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
                    std::cout << "Game Over! (Hit by obstacle)" << std::endl;
                    isRunning = false;
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
        orb->Update(player);
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
}

void Game::Render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);  // Clear screen first

        // Draw background
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int bgX = (-static_cast<int>(player->x) % backgroundWidth) + (i * backgroundWidth);
                int bgY = (-static_cast<int>(player->y) % backgroundHeight) + (j * backgroundHeight);
                SDL_Rect bgRect = { bgX, bgY, backgroundWidth, backgroundHeight };
                SDL_RenderCopy(renderer, backgroundTexture, nullptr, &bgRect);
            }
        }

        // Draw everything else
        player->Render(renderer);
        for (auto bullet : bullets) bullet->Render(renderer, player->x, player->y);
        for (auto enemy : enemies) enemy->Render(renderer, player);
        for (auto enemyBullet : enemyBullets) enemyBullet->Render(renderer, player->x, player->y);
        for (auto obs : obstacles) obs->Render(renderer, player);
        for (auto orb : orbs) orb->Render(renderer, player);

        SDL_RenderPresent(renderer);
}

void Game::Clean() {
    // Cleanup in reverse order of creation
    if (player) {
        delete player;
        player = nullptr;
    }

    for (auto bullet : bullets) {
        delete bullet;
    }
    bullets.clear();

    for (auto enemyBullet : enemyBullets) {
        delete enemyBullet;
    }
    enemyBullets.clear();

    for (auto enemy : enemies) {
        delete enemy;
    }
    enemies.clear();

    for (auto obs : obstacles) {
        delete obs;
    }
    obstacles.clear();

    for (auto orb : orbs) {
        delete orb;
    }
    orbs.clear();

    // Destroy textures
    if (playerTex) SDL_DestroyTexture(playerTex);
    if (bulletTexture) SDL_DestroyTexture(bulletTexture);
    if (enemyTexture) SDL_DestroyTexture(enemyTexture);
    if (neutralObstacleTexture) SDL_DestroyTexture(neutralObstacleTexture);
    if (hostileObstacleTexture) SDL_DestroyTexture(hostileObstacleTexture);
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (orbTexture) SDL_DestroyTexture(orbTexture);

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    IMG_Quit();
    SDL_Quit();
}

bool Game::Running() const {
    return isRunning;
}
