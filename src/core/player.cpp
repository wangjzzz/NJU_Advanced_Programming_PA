#include "core/player.h"

Player::Player()
    : m_hp(100)
    , m_maxHp(100)
    , m_gold(15)
    , m_level(1)
    , m_populationCap(4)
    , m_currentRound(1)
    , m_populationUpgradeCount(0)
{
}

void Player::addGold(int amount)
{
    if (amount > 0) {
        m_gold += amount;
    }
}

bool Player::spendGold(int amount)
{
    if (amount <= 0 || m_gold < amount) {
        return false;
    }
    m_gold -= amount;
    return true;
}

int Player::populationUpgradeCost() const
{
    return 4 + m_populationUpgradeCount * 2;
}

bool Player::upgradePopulationCap()
{
    if (m_populationCap >= 9) {
        return false;
    }
    const int cost = populationUpgradeCost();
    if (!spendGold(cost)) {
        return false;
    }
    ++m_populationCap;
    ++m_populationUpgradeCount;
    return true;
}

void Player::reset()
{
    m_hp = m_maxHp;
    m_gold = 15;
    m_level = 1;
    m_populationCap = 4;
    m_currentRound = 1;
    m_populationUpgradeCount = 0;
    m_bench.clear();
}
