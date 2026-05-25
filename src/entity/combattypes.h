#ifndef COMBATTYPES_H
#define COMBATTYPES_H

enum class UnitState {
    Idle,
    Moving,
    Attacking,
    Casting,
    Dead
};

enum class GamePhase {
    Prep,
    Combat,
    Resolve
};

enum class GameResult {
    Playing,
    Victory,
    Defeat
};

namespace CombatConst {
constexpr int kFramesPerSecond = 60;
constexpr int kAttackInterval = 60;
constexpr int kMoveInterval = 20;
constexpr int kCastDuration = 30;
constexpr int kManaPerAttack = 10;
constexpr int kStunDuration = 90;
constexpr int kMaxRounds = 10;
constexpr int kPlayerDamageOnLoss = 12;
constexpr int kGoldWinBase = 6;
constexpr int kGoldLoseBase = 2;
}

#endif // COMBATTYPES_H
