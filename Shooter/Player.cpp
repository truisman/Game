#include "Player.h"
#include "Bullet.h"
#include <iostream>
#include "Game.h"

Player::Player(float x, float y, SDL_Texture* selectedTexture, int startingHealth, float speed, Game* gameInstance)
    : x(x), y(y), vx(0.0f), vy(0.0f), angle(0.0f), width(45), height(45), texture(selectedTexture), game(gameInstance), health(startingHealth), maxHealth(startingHealth), speed(speed), firingRateFactor(2.5f),
    lastShotTime(0), speedMultiplier(1.0f), shootingPattern(ShootingPattern::SINGLE), level(1), experience(0), experienceToNextLevel(10), bulletType(BulletType::NORMAL) {}

void Player::HandleInput(const Uint8* keystate, std::vector<Bullet*>& bullets) {
    speedMultiplier = (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) ? 2.0f : 1.0f;

    if (keystate[SDL_SCANCODE_W]) Move(speed * speedMultiplier);
    if (keystate[SDL_SCANCODE_S]) Move(-speed * speedMultiplier);
    if (keystate[SDL_SCANCODE_A]) Rotate(-PLAYER_ROTATION_SPEED);
    if (keystate[SDL_SCANCODE_D]) Rotate(PLAYER_ROTATION_SPEED);

    if (keystate[SDL_SCANCODE_SPACE]) {
        Shoot(bullets, shootingPattern);
    }
}

void Player::Move(float speed) {
     float radianAngle = (angle - 90) * M_PI / 180.0f;

    float deltaX = speed * cos(radianAngle);
    float deltaY = speed * sin(radianAngle);

    // Update player position
    x += deltaX;
    y += deltaY;
}

void Player::Rotate(float amount) {
    angle += amount;
}

void Player::Render(SDL_Renderer* renderer) {
    SDL_Rect rect = { static_cast<int>(SCREEN_WIDTH / 2 - width / 2), static_cast<int>(SCREEN_HEIGHT / 2 - height / 2), width, height };
    SDL_Point center = { width / 2, height / 2 };
    SDL_RenderCopyEx(renderer, texture, nullptr, &rect, angle, &center, SDL_FLIP_NONE);
}

void Player::Shoot(std::vector<Bullet*>& bullets, ShootingPattern shootingPattern) {

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastShotTime >= (BASE_SHOT_COOLDOWN / firingRateFactor))
    {
        SDL_Texture* selectedBulletTexture = nullptr;
        switch (this->bulletType) {
            case BulletType::NORMAL:          selectedBulletTexture = this->game->bulletTexNormal; break;
            case BulletType::POWERED:         selectedBulletTexture = this->game->bulletTexPowered; break;
            case BulletType::SUPER_POWERED:   selectedBulletTexture = this->game->bulletTexSuperPowered; break;
            case BulletType::EXTREME_POWERED: selectedBulletTexture = this->game->bulletTexExtremePowered; break;
            default:                          selectedBulletTexture = this->game->bulletTexNormal; break;
        }

        float radianAngle = (angle - 90) * M_PI / 180.0f;
        float bulletVX = BULLET_SPEED * cos(radianAngle) * speedMultiplier;
        float bulletVY = BULLET_SPEED * sin(radianAngle) * speedMultiplier;
        float bulletOffsetX = width / 2.0f * cos(radianAngle);
        float bulletOffsetY = height / 2.0f * sin(radianAngle);
        float bulletX = x + bulletOffsetX;
        float bulletY = y + bulletOffsetY;
        float bulletDamage = 50;

        switch (shootingPattern) {
            case ShootingPattern::SINGLE:
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, selectedBulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::DOUBLE:
                bullets.push_back(new Bullet(bulletX + 5, bulletY + 5, bulletVX, bulletVY, selectedBulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX - 5, bulletY - 5, bulletVX, bulletVY, selectedBulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::TRIPLE:
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, selectedBulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::SIDEWAYS:
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, selectedBulletTexture, bulletDamage, bulletType));

                float sidewaysAngleLeft = (angle - 90 - 90) * M_PI / 180.0f;
                float sidewaysAngleRight = (angle - 90 + 90) * M_PI / 180.0f;

                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos(sidewaysAngleLeft) * speedMultiplier, BULLET_SPEED * sin(sidewaysAngleLeft) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos(sidewaysAngleRight) * speedMultiplier, BULLET_SPEED * sin(sidewaysAngleRight) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));

                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, selectedBulletTexture, bulletDamage, bulletType));
                break;
        }
        lastShotTime = currentTime;
    }
}

void Player::AddExperience(int amount) {
    experience += amount;
    while (experience >= experienceToNextLevel) {
        LevelUp();
    }
}

void Player::LevelUp() {
    level++;
    experience -= experienceToNextLevel;
    experienceToNextLevel = static_cast<int>(experienceToNextLevel * 1.5f);
    std::cout << "Player Leveled Up! Level: " << level << std::endl;

    maxHealth *= 2;
    health = maxHealth;

    if (level == 2) {
        firingRateFactor *= 1.2f;
        shootingPattern  = ShootingPattern::DOUBLE;
        bulletType = BulletType::NORMAL;
    } else if (level == 3) {
        shootingPattern  = ShootingPattern::TRIPLE;
        bulletType = BulletType::NORMAL;
    } else if (level == 4) {
        firingRateFactor *= 1.2f;
        shootingPattern  = ShootingPattern::TRIPLE;
		bulletType = BulletType::POWERED;
	} else if (level == 5){
        shootingPattern  = ShootingPattern::SIDEWAYS;
        bulletType = BulletType::POWERED;
    } else if (level == 6) {
        firingRateFactor *= 1.2f;
        shootingPattern  = ShootingPattern::SIDEWAYS;
        bulletType = BulletType::SUPER_POWERED;
    }
    else if (level >= 7) {
        shootingPattern  = ShootingPattern::SIDEWAYS;
        bulletType = BulletType::EXTREME_POWERED;
    }
}

void Player::TakeDamage(int amount) {
    if (amount <= 0) return;
    health -= amount;
    if (health < 0) {
        health = 0;
    }
}
