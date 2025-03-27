#include "Enemy.h"
#include "Player.h"
#include "Obstacle.h"
#include "Game.h"
#include <iostream> // For debugging output

Enemy::Enemy(float x, float y, SDL_Texture* tex, Player* target, int health, float speed, float firingRateFactor, Game* game, EnemyType type)
    : x(x), y(y), vx(0), vy(0), angle(0), width(60), height(60), texture(tex), target(target), health(health), speed(ENEMY_SPEED),
      firingRateFactor(firingRateFactor), lastShotTime(0), state(EnemyState::WANDERING), lastStateChange(0)
{
    switch (type) {
        case EnemyType::NORMAL:
            // Default values are already set in the member initialization list
            break;
        case EnemyType::FAST:
            // Modify values for fast enemies (e.g., increase speed)
            this->width = 50;
            this->height = 50;
            this->speed = 4.0f;
            break;
        case EnemyType::TANK:
            // Modify values for tanky enemies (e.g., increase health)
            this->width = 80;
            this->height = 80;
            this->health *= 3;
            break;
        case EnemyType::QUICK:
            this->firingRateFactor = firingRateFactor * 2;
            break;
    }
}

void Enemy::Update(std::vector<Enemy*>& enemies, std::vector<Obstacle*>& obstacles, std::vector<Bullet*>& enemyBullets, Player* player, Game* game) {

    // --- Existing code for calculating distanceToPlayer, etc. ---
    float targetX = player->x;
    float targetY = player->y;

    float dxToPlayer = targetX - x;
    float dyToPlayer = targetY - y;
    float distanceToPlayer = sqrt(dxToPlayer * dxToPlayer + dyToPlayer * dyToPlayer);

    // --- State Management (Modified) ---
    Uint32 currentTime = SDL_GetTicks();

    // --- State Transitions
    if (currentTime - lastStateChange > 3000) {
        if (distanceToPlayer < 200.0f && state != EnemyState::RETREATING) {
            state = EnemyState::RETREATING;
            lastStateChange = currentTime;
        } else if (distanceToPlayer > 600.0f && state != EnemyState::ENGAGING) {
            state = EnemyState::ENGAGING;
            lastStateChange = currentTime;
        } else if (distanceToPlayer >= 200.f && distanceToPlayer <= 600.f && state != EnemyState::CIRCLING) {
            state = EnemyState::CIRCLING;
            lastStateChange = currentTime;
        }
    }

    // --- Movement and Behavior based on State ---
    float targetVX = 0, targetVY = 0;
    float maxForce = 0.3f;
    float speed = 2.0f;

    switch (state) {
        case EnemyState::WANDERING: {
            if (currentTime - lastStateChange > 5000) { // Change direction every 5 seconds
                wanderingAngle = game->RandomFloat(0, 2 * M_PI);
                lastStateChange = currentTime;
            }
            targetVX = cos(wanderingAngle) * speed;
            targetVY = sin(wanderingAngle) * speed;
            break;
        }
        case EnemyState::ENGAGING: {
            float angleToPlayer = atan2(dyToPlayer, dxToPlayer);
            targetVX = cos(angleToPlayer) * speed;
            targetVY = sin(angleToPlayer) * speed;
            break;
        }
        case EnemyState::RETREATING: {
            float retreatAngle = atan2(dyToPlayer, dxToPlayer) + M_PI; // Directly away
            // Add some randomness to the retreat angle for less predictable movement
            retreatAngle += game->RandomFloat(-M_PI / 4, M_PI / 4); // +/- 45 degrees
            targetVX = cos(retreatAngle) * speed * 1.5f; // Retreat faster
            targetVY = sin(retreatAngle) * speed * 1.5f;
            break;
        }
        case EnemyState::CIRCLING: {
             float angleToPlayer = atan2(dyToPlayer, dxToPlayer);
            float strafeAngle = angleToPlayer + M_PI / 2.0f; // 90 degrees to the right

            // 2. Randomly choose to strafe left or right.
            if (currentTime - lastStateChange > 2000) // Switch strafe direction
			{
                circlingDirection = (rand() % 2 == 0) ? 1.0f : -1.0f;
                lastStateChange = currentTime;
            }

			// 3. Combine forward movement (towards optimal range) and strafing.
            float desiredDistance = 400.0f; // Optimal circling distance.
            float distanceDifference = distanceToPlayer - desiredDistance;
			float forwardSpeed = std::clamp(distanceDifference * 0.02f, -speed, speed); // Adjust 0.02 as needed

            targetVX = cos(angleToPlayer) * forwardSpeed + cos(strafeAngle) * speed * circlingDirection;
            targetVY = sin(angleToPlayer) * forwardSpeed + sin(strafeAngle) * speed * circlingDirection;
            break;
        }
    }

    // --- Apply Accumulated Forces (Smoothly) ---
    vx += std::clamp(targetVX - vx, -maxForce, maxForce);
    vy += std::clamp(targetVY - vy, -maxForce, maxForce);

    // --- Limit Speed ---
    float currentSpeed = sqrt(vx * vx + vy * vy);
    if (currentSpeed > speed) {
        vx = (vx / currentSpeed) * speed;
        vy = (vy / currentSpeed) * speed;
    }

    // --- Obstacle Avoidance (Robust Push-Back) ---
    SDL_Rect futureRect = {static_cast<int>(x + vx), static_cast<int>(y + vy), width, height};

    for (const auto& obs : obstacles) {
        SDL_Rect obsRect = obs->GetRect();
        if (SDL_HasIntersection(&futureRect, &obsRect)) {
            float dx = (obsRect.x + obsRect.w * 0.5f) - (futureRect.x + futureRect.w * 0.5f);
            float dy = (obsRect.y + obsRect.h * 0.5f) - (futureRect.y + futureRect.h * 0.5f);

            if (abs(dx) > abs(dy)) {
                if ((dx > 0 && vx > 0) || (dx < 0 && vx < 0)) {
                    vx = 0;
                }
            } else {
                if ((dy > 0 && vy > 0) || (dy < 0 && vy < 0)) {
                    vy = 0;
                }
            }
        }
    }

    // --- Update Position ---
    x += vx;
    y += vy;

    // --- Repositioning (Final Correction) ---
    futureRect = {static_cast<int>(x), static_cast<int>(y), width, height}; // Update futureRect
    for (const auto& obs : obstacles) {
        SDL_Rect obsRect = obs->GetRect();
        if (SDL_HasIntersection(&futureRect, &obsRect)) {
            int overlapX = 0;
            int overlapY = 0;

            if (futureRect.x < obsRect.x) {
                overlapX = futureRect.x + futureRect.w - obsRect.x;
            } else {
                overlapX = -(obsRect.x + obsRect.w - futureRect.x);
            }

            if (futureRect.y < obsRect.y) {
                overlapY = futureRect.y + futureRect.h - obsRect.y;
            } else {
                overlapY = -(obsRect.y + obsRect.h - futureRect.y);
            }

            if (abs(overlapX) < abs(overlapY)) {
                x -= overlapX;
            } else {
                y -= overlapY;
            }
        }
    }

    if (vx != 0 || vy != 0) {  // Only update angle if moving
        float targetAngle = atan2(vy, vx) * 180.0f / M_PI; // Calculate the target angle

        // Calculate the difference, handling wrap-around
        float angleDiff = targetAngle - angle;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        // Smoothly interpolate towards the target angle
        const float rotationSpeed = 0.2f; // Adjust this value (0.0 = no rotation, 1.0 = instant rotation)
        angle += angleDiff * rotationSpeed;

        // Keep angle within -180 to 180 range (optional, but good practice)
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;

    }

    // --- Shooting ---
    if (currentTime - lastShotTime > (BASE_SHOT_COOLDOWN / firingRateFactor) && distanceToPlayer < 700.0f) {
        Shoot(enemyBullets);
        lastShotTime = currentTime;
    }
}

void Enemy::Shoot(std::vector<Bullet*>& enemyBullets) {
    float dx = target->x - x;
    float dy = target->y - y;
    float length = sqrt(dx * dx + dy * dy);
    float bulletVX = (dx / length) * BULLET_SPEED;
    float bulletVY = (dy / length) * BULLET_SPEED;

    BulletType bulletType = BulletType::NORMAL;
    if (type == EnemyType::TANK) {
        bulletType = BulletType::POWERED;
    }
    if (type == EnemyType::QUICK) {
        bulletType = BulletType::POWERED;
    }

    switch (type) {
    case EnemyType::NORMAL:
        enemyBullets.push_back(new Bullet(x + width / 2, y + height / 2, bulletVX, bulletVY, texture, 10, bulletType));
        break;
    case EnemyType::FAST:
        enemyBullets.push_back(new Bullet(x + width / 2, y + height / 2, bulletVX, bulletVY, texture, 10, bulletType));
        break;
    case EnemyType::TANK:
         enemyBullets.push_back(new Bullet(x + width / 2, y + height / 2, bulletVX, bulletVY, texture, 30, bulletType));
        break;
    case EnemyType::QUICK:
        enemyBullets.push_back(new Bullet(x + width / 2 - 10, y + height / 2, bulletVX, bulletVY, texture, 10, bulletType));
        enemyBullets.push_back(new Bullet(x + width / 2 + 10, y + height / 2, bulletVX, bulletVY, texture, 10, bulletType));
        break;
    }
}

void Enemy::Render(SDL_Renderer* renderer, Player* player) {
    SDL_Rect rect = {static_cast<int>(x - player->x + SCREEN_WIDTH / 2), static_cast<int>(y - player->y + SCREEN_HEIGHT / 2), width, height};
    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle + 90, NULL, SDL_FLIP_NONE);
}
