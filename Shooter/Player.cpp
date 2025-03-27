// src/Player.cpp
#include "Player.h"
#include "Bullet.h" // Include Bullet.h for Bullet definition
#include <iostream>


Player::Player(float x, float y, SDL_Texture* texture, int health, float speed)
    : x(x), y(y), width(50), height(50), texture(texture), health(health), speed(speed), rotation(0.0f), lastShotTime(0),
      shootingPattern(ShootingPattern::SINGLE), level(1), experience(0), experienceToNextLevel(10), bulletType(BulletType::NORMAL), angle(0.0f),
      firingRateFactor(4.0f), speedMultiplier(1.0f) {}

void Player::HandleInput(const Uint8* keystate, std::vector<Bullet*>& bullets, SDL_Texture* bulletTexture) {
    speedMultiplier = (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) ? 2.0f : 1.0f;

    if (keystate[SDL_SCANCODE_W]) Move(speed * speedMultiplier);
    if (keystate[SDL_SCANCODE_S]) Move(-speed * speedMultiplier);
    if (keystate[SDL_SCANCODE_A]) Rotate(-PLAYER_ROTATION_SPEED);
    if (keystate[SDL_SCANCODE_D]) Rotate(PLAYER_ROTATION_SPEED);

    if (keystate[SDL_SCANCODE_SPACE]) {
        Shoot(bullets, bulletTexture, shootingPattern);
    }
}

void Player::Move(float speed) {
     float radianAngle = (angle - 90) * M_PI / 180.0f;

    float deltaX = speed * cos(radianAngle);
    float deltaY = speed * sin(radianAngle);

    // Update player position
    x += deltaX;
    y += deltaY;

    SDL_Rect newPlayerRect = { (int)x, (int)y, width, height };
}

void Player::Rotate(float amount) {
    angle += amount;
}

void Player::Render(SDL_Renderer* renderer) {
    SDL_Rect rect = { static_cast<int>(SCREEN_WIDTH / 2 - width / 2), static_cast<int>(SCREEN_HEIGHT / 2 - height / 2), width, height };
    SDL_Point center = { width / 2, height / 2 };
    SDL_RenderCopyEx(renderer, texture, nullptr, &rect, angle, &center, SDL_FLIP_NONE);
}

void Player::Shoot(std::vector<Bullet*>& bullets, SDL_Texture* bulletTexture, ShootingPattern shootingPattern) {
    std::cout << "Shoot function called" << std::endl;
    std::cout << "lastShotTime: " << lastShotTime << std::endl;
    std::cout << "BASE_SHOT_COOLDOWN: " << BASE_SHOT_COOLDOWN << std::endl;
    std::cout << "firingRateFactor: " << firingRateFactor << std::endl;

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastShotTime >= (BASE_SHOT_COOLDOWN / firingRateFactor))
    {
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
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, bulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::DOUBLE:
                bullets.push_back(new Bullet(bulletX + 5, bulletY + 5, bulletVX, bulletVY, bulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX - 5, bulletY - 5, bulletVX, bulletVY, bulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::TRIPLE:
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, bulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, bulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, bulletTexture, bulletDamage, bulletType));
                break;
            case ShootingPattern::SIDEWAYS:
                bullets.push_back(new Bullet(bulletX, bulletY, bulletVX, bulletVY, bulletTexture, bulletDamage, bulletType));

                float sidewaysAngleLeft = (angle - 90 - 90) * M_PI / 180.0f;
                float sidewaysAngleRight = (angle - 90 + 90) * M_PI / 180.0f;

                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos(sidewaysAngleLeft) * speedMultiplier * 0.5f, BULLET_SPEED * sin(sidewaysAngleLeft) * speedMultiplier * 0.5f, bulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos(sidewaysAngleRight) * speedMultiplier * 0.5f, BULLET_SPEED * sin(sidewaysAngleRight) * speedMultiplier * 0.5f, bulletTexture, bulletDamage, bulletType));

                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle - 10 - 90) * M_PI / 180.0f) * speedMultiplier, bulletTexture, bulletDamage, bulletType));
                bullets.push_back(new Bullet(bulletX, bulletY, BULLET_SPEED * cos((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, BULLET_SPEED * sin((angle + 10 - 90) * M_PI / 180.0f) * speedMultiplier, bulletTexture, bulletDamage, bulletType));
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

    if (level == 2) {
        shootingPattern = ShootingPattern::DOUBLE;
        bulletType = BulletType::NORMAL;
    } else if (level == 3) {
        shootingPattern = ShootingPattern::TRIPLE;
        bulletType = BulletType::NORMAL;
    } else if (level == 4) {
        shootingPattern = ShootingPattern::TRIPLE;
		bulletType = BulletType::POWERED;
	}
	else if (level == 5){
        shootingPattern = ShootingPattern::SIDEWAYS;
        bulletType = BulletType::POWERED;
    } else if (level >= 6) {
        shootingPattern = ShootingPattern::SIDEWAYS;
        bulletType = BulletType::SUPER_POWERED;
    }

    std::cout << "Player Leveled Up! Level: " << level << std::endl;
}

