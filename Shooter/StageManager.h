#ifndef STAGEMANAGER_H
#define STAGEMANAGER_H

#include <vector>
#include <map>
#include <SDL_stdinc.h>

enum class EnemyType;

struct StageData {
    int stageNumber;
    int killGoal;
    Uint32 spawnInterval;
    int baseSpawnCount;
    std::map<EnemyType, int> spawnProbabilities;
    bool canSpawnBoss;
    int bossSpawnChance;
};

class Player;

class StageManager {
public:
    StageManager();

    void LoadStages();

    void StartGame();
    void AdvanceStage(Player* player);

    void RecordKill();

    bool ShouldAdvanceStage() const;

    const StageData& GetCurrentStageData() const;
    int GetCurrentStageNumber() const;
    int GetCurrentKillCount() const;
    int GetCurrentKillGoal() const;
    bool IsGameWon() const;

private:
    std::vector<StageData> stages;
    int currentStageIndex;
    int currentKillCount;
    bool gameWon;
};

#endif
