#include "StageManager.h"
#include "Enemy.h"
#include "Player.h"
#include <iostream>
#include <vector>
#include <map>

StageManager::StageManager() : currentStageIndex(-1), currentKillCount(0), gameWon(false) {
    LoadStages();
}

void StageManager::LoadStages() {
    stages.clear();

    // Stage 1
    stages.push_back({ 1, 10, 2000, 1, {{EnemyType::NORMAL, 900}, {EnemyType::FAST, 100}}, false, 0 });

    // Stage 2
    stages.push_back({ 2, 20, 1800, 1, {{EnemyType::NORMAL, 600}, {EnemyType::FAST, 300}, {EnemyType::QUICK, 100}}, false, 0 });

    // Stage 3
    stages.push_back({ 3, 30, 1600, 2, {{EnemyType::NORMAL, 300}, {EnemyType::FAST, 350}, {EnemyType::QUICK, 250}, {EnemyType::TANK, 100}}, false, 0 });

    // Stage 4
    stages.push_back({ 4, 50, 1400, 2, {{EnemyType::NORMAL, 150}, {EnemyType::FAST, 250}, {EnemyType::QUICK, 340}, {EnemyType::TANK, 250}}, true, 10 });

    // Stage 5
    stages.push_back({ 5, 70, 1300, 3, {{EnemyType::NORMAL, 60}, {EnemyType::FAST, 230}, {EnemyType::QUICK, 400}, {EnemyType::TANK, 300}}, true, 10 });

    // Stage 6
    stages.push_back({ 6, 100, 1200, 3, {{EnemyType::FAST, 200}, {EnemyType::QUICK, 350}, {EnemyType::TANK, 420}}, true, 30 });

    std::cout << "Loaded " << stages.size() << " stages." << std::endl;
}

void StageManager::StartGame() {
    currentStageIndex = 0;
    currentKillCount = 0;
    gameWon = false;
    if (stages.empty()) {
        std::cerr << "Error: No stages loaded in StageManager!" << std::endl;
    } else {
         std::cout << "Starting Game - Stage " << stages[currentStageIndex].stageNumber << std::endl;
    }
}

void StageManager::AdvanceStage(Player* player) {
    if (currentStageIndex < 0 || stages.empty()) {
         std::cerr << "Error: Cannot advance stage, game not started or no stages loaded." << std::endl;
         return;
    }

    if (currentStageIndex + 1 >= stages.size()) {
        std::cout << "Final stage cleared! Game Won!" << std::endl;
        gameWon = true;
    } else {
        currentStageIndex++;
        currentKillCount = 0;

        // Apply Player Buffs
        if (player) {
             std::cout << "Advanced to Stage " << GetCurrentStageNumber()
                       << "! Player healed and health doubled to " << player->health << "." << std::endl;
        } else {
             std::cout << "Advanced to Stage " << GetCurrentStageNumber() << "." << std::endl;
        }
    }
}


void StageManager::RecordKill() {
    if (currentStageIndex >= 0 && !gameWon) {
        currentKillCount++;
    }
}

bool StageManager::ShouldAdvanceStage() const {
    if (currentStageIndex < 0 || currentStageIndex >= stages.size() || gameWon) {
        return false;
    }
    return currentKillCount >= stages[currentStageIndex].killGoal;
}

const StageData& StageManager::GetCurrentStageData() const {
    if (currentStageIndex < 0 || currentStageIndex >= stages.size()) {
        if (stages.empty()) {
             static StageData emptyStage = {0, 0, 0, 0, {}, false, 0};
             std::cerr << "Error: GetCurrentStageData called with no stages loaded!" << std::endl;
             return emptyStage;
        }
        std::cerr << "Warning: Invalid currentStageIndex (" << currentStageIndex << "). Returning stage 1 data." << std::endl;
        return stages[0];
    }
    return stages[currentStageIndex];
}

int StageManager::GetCurrentStageNumber() const {
     if (currentStageIndex < 0 || currentStageIndex >= stages.size()) {
         return 0;
     }
    return stages[currentStageIndex].stageNumber;
}

int StageManager::GetCurrentKillCount() const {
    return currentKillCount;
}

int StageManager::GetCurrentKillGoal() const {
     if (currentStageIndex < 0 || currentStageIndex >= stages.size()) {
         return 0;
     }
    return stages[currentStageIndex].killGoal;
}

bool StageManager::IsGameWon() const {
    return gameWon;
}
