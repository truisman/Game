#include "Enemy.h"
#include "Player.h"
#include "Obstacle.h"
#include "Game.h"
#include <iostream>

Enemy::Enemy(float x, float y, SDL_Texture* selectedTexture, Player* target, int health, float speedFactor, float firingRateFactor, Game* game, EnemyType type, SDL_Texture* selectedBulletTexture)
    : x(x), y(y), vx(0), vy(0), angle(0), width(110), height(110), texture(selectedTexture),bulletTexture(selectedBulletTexture), target(target), health(health), speed(ENEMY_SPEED),
      firingRateFactor(firingRateFactor), lastShotTime(0), state(EnemyState::WANDERING), lastStateChange(0)
{
    switch (type) {
        case EnemyType::NORMAL:
            this->health = 100;
            break;
        case EnemyType::FAST:
            this->width = 110;
            this->height = 110;
            this->speed = ENEMY_SPEED * 1.5f;
            this->health = 70;
            break;
        case EnemyType::TANK:
            this->width = 200;
            this->height = 200;
            this->speed = ENEMY_SPEED * 0.6f;
            this->health *= 5;
            this->firingRateFactor *= 0.5f;
            break;
        case EnemyType::QUICK:
             this->width = 130;
             this->height = 130;
             this->speed = ENEMY_SPEED * 1.2f;
             this->firingRateFactor *= 2.0f;
             this->health = 120;
            break;
        case EnemyType::BOSS:
             this->width = 300;
             this->height = 300;
             this->speed *= 0.5f;
             this->health = 5000;
             this->firingRateFactor *= 1.5f;
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
    float targetCenterX = target->x + target->width / 2.0f;
    float targetCenterY = target->y + target->height / 2.0f;
    float startX = x + width / 2.0f;
    float startY = y + height / 2.0f;

    float dx = targetCenterX - startX;
    float dy = targetCenterY - startY;
    float length = sqrt(dx * dx + dy * dy);

    float baseVX = 0, baseVY = 0;
    float angleToTargetRad = 0;
    if (length > 0.01f) {
        baseVX = dx / length;
        baseVY = dy / length;
        angleToTargetRad = atan2(baseVY, baseVX);
    } else {
        angleToTargetRad = (this->angle + 90.0f) * M_PI / 180.0f;
        baseVX = cos(angleToTargetRad);
        baseVY = sin(angleToTargetRad);
    }

    BulletType bulletType = BulletType::NORMAL;
    int baseDamage = 20;

    switch (type) {
        case EnemyType::NORMAL:
            bulletType = BulletType::NORMAL;
            break;
        case EnemyType::FAST:
            bulletType = BulletType::POWERED;
            break;
        case EnemyType::TANK:
            bulletType = BulletType::EXTREME_POWERED;
            break;
        case EnemyType::QUICK:
            bulletType = BulletType::SUPER_POWERED;
            break;
        case EnemyType::BOSS:
            bulletType = BulletType::EXTREME_POWERED;
            break;
    }

    switch (type) {
        case EnemyType::NORMAL:
            break;
        case EnemyType::FAST:
            break;
        case EnemyType::TANK:
            {
                enemyBullets.push_back(new Bullet(startX, startY, baseVX, baseVY, this->bulletTexture, baseDamage, bulletType));
            }
            break;

        case EnemyType::QUICK:
            {
            float perpAngle = angleToTargetRad + M_PI / 2.0f;
            float offsetDist = 10.0f;
            float offsetX = offsetDist * cos(perpAngle);
            float offsetY = offsetDist * sin(perpAngle);

            enemyBullets.push_back(new Bullet(startX + offsetX, startY + offsetY, baseVX, baseVY, this->bulletTexture, baseDamage, bulletType));
            enemyBullets.push_back(new Bullet(startX - offsetX, startY - offsetY, baseVX, baseVY, this->bulletTexture, baseDamage, bulletType));
            }
            break;

        case EnemyType::BOSS:
            {
            enemyBullets.push_back(new Bullet(startX, startY, baseVX, baseVY, this->bulletTexture, baseDamage, bulletType));

            float angleLeft90 = angleToTargetRad - M_PI / 2.0f;
            float angleRight90 = angleToTargetRad + M_PI / 2.0f;
            float sideSpeedFactor = 0.6f;

            enemyBullets.push_back(new Bullet(startX, startY, cos(angleLeft90) * sideSpeedFactor, sin(angleLeft90) * sideSpeedFactor, this->bulletTexture, baseDamage, bulletType));
            enemyBullets.push_back(new Bullet(startX, startY, cos(angleRight90) * sideSpeedFactor, sin(angleRight90) * sideSpeedFactor, this->bulletTexture, baseDamage, bulletType));

            float spreadRad = 15.0f * M_PI / 180.0f;
            float angleLeftSpread = angleToTargetRad - spreadRad;
            float angleRightSpread = angleToTargetRad + spreadRad;

            enemyBullets.push_back(new Bullet(startX, startY, cos(angleLeftSpread), sin(angleLeftSpread), this->bulletTexture, baseDamage, bulletType));
            enemyBullets.push_back(new Bullet(startX, startY, cos(angleRightSpread), sin(angleRightSpread), this->bulletTexture, baseDamage, bulletType));
            }
            break;
    }
}

void Enemy::Render(SDL_Renderer* renderer, Player* player) {
    SDL_Rect rect = {static_cast<int>(x - player->x + SCREEN_WIDTH / 2), static_cast<int>(y - player->y + SCREEN_HEIGHT / 2), width, height};
    SDL_RenderCopyEx(renderer, texture, NULL, &rect, angle + 90, NULL, SDL_FLIP_NONE);
}
