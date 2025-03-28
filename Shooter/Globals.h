// include/Globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <random>

// Global Constants
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int backgroundWidth;
extern const int backgroundHeight;
extern const float BULLET_SPEED;
extern const float ENEMY_SPEED;
extern const float PLAYER_SPEED;
extern const float PLAYER_ROTATION_SPEED;
extern const float ENEMY_ROTATION_SPEED;
extern const float OBSTACLE_ROTATION_SPEED;
extern const Uint32 ENEMY_SPAWN_INTERVAL;
extern const Uint32 BASE_SHOT_COOLDOWN;
extern const Uint32 HOSTILE_OBSTACLE_SHOOT_COOLDOWN;
extern const int MAX_OBSTACLES;
extern const int MAX_ENEMIES;
extern const int OBSTACLE_GRID_SIZE;
extern const float OBSTACLE_HOVER_AMPLITUDE;
extern const float OBSTACLE_HOVER_SPEED;
extern const int SPAWN_RADIUS;
extern const int DESPAWN_RADIUS;
extern const int NUM_GRID_CELLS;
extern const float MIN_SEPARATION_DISTANCE;
extern const int ORB_SIZE;
extern const float ORB_FADE_RATE;
extern const float CREDITS_SCROLL_SPEED;
extern const int CREDITS_LINE_HEIGHT;

enum class EnemyType {
    NORMAL,
    FAST,
    TANK,
    QUICK,
    BOSS
};

enum class ObstacleType {
    NEUTRAL,
    HOSTILE
};

enum class BulletType {
    NORMAL,
    POWERED,
    SUPER_POWERED,
    EXTREME_POWERED
};

enum class EnemyState {
    WANDERING,
    ENGAGING,
    RETREATING,
    CIRCLING
};

enum class ShootingPattern {
    SINGLE,
    DOUBLE,
    TRIPLE,
    SIDEWAYS
};

#endif // GLOBALS_H
