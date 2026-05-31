#ifndef PLAYER_H
#define PLAYER_H

#include "core/bench.h"

class Player
{
public:
    Player();

    int hp() const { return m_hp; }
    int maxHp() const { return m_maxHp; }
    int gold() const { return m_gold; }
    int level() const { return m_level; }
    int populationCap() const { return m_populationCap; }
    int currentRound() const { return m_currentRound; }
    int populationUpgradeCount() const { return m_populationUpgradeCount; }
    int winStreak() const { return m_winStreak; }
    int lossStreak() const { return m_lossStreak; }
    int interestBonus() const;
    int streakBonus() const;
    QString streakText() const;

    Bench* bench() { return &m_bench; }
    const Bench* bench() const { return &m_bench; }

    void setHp(int hp) { m_hp = qMax(0, qMin(hp, m_maxHp)); }
    void setGold(int gold) { m_gold = qMax(0, gold); }
    void setPopulationCap(int cap) { m_populationCap = qBound(1, cap, 9); }
    void setCurrentRound(int round) { m_currentRound = qMax(1, round); }
    void setPopulationUpgradeCount(int count) { m_populationUpgradeCount = qMax(0, count); }
    void takeDamage(int damage) { m_hp = qMax(0, m_hp - damage); }
    void addGold(int amount);
    bool spendGold(int amount);

    int populationUpgradeCost() const;
    bool upgradePopulationCap();

    void advanceRound() { ++m_currentRound; }
    void onCombatWin();
    void onCombatLose();
    void reset();

    bool isAlive() const { return m_hp > 0; }

private:
    int m_hp;
    int m_maxHp;
    int m_gold;
    int m_level;
    int m_populationCap;
    int m_currentRound;
    int m_populationUpgradeCount;
    int m_winStreak;
    int m_lossStreak;

    Bench m_bench;
};

#endif // PLAYER_H
