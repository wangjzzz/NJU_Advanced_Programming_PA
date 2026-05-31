#include "core/player.h"

Player::Player()
    : m_hp(100)
    , m_maxHp(100)
    , m_gold(15)
    , m_level(1)
    , m_populationCap(4)
    , m_currentRound(1)
    , m_populationUpgradeCount(0)
    , m_winStreak(0)
    , m_lossStreak(0)
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
    m_winStreak = 0;
    m_lossStreak = 0;
    m_bench.clear();
}

int Player::interestBonus() const
{
    return qMin(m_gold / 10, 5);
}

int Player::streakBonus() const
{
    if (m_winStreak >= 6) return 3;
    if (m_winStreak >= 4) return 2;
    if (m_winStreak >= 2) return 1;
    if (m_lossStreak >= 4) return 2;
    if (m_lossStreak >= 2) return 1;
    return 0;
}

QString Player::streakText() const
{
    if (m_winStreak >= 2) return QStringLiteral("\u8FDE\u80DC%1").arg(m_winStreak);
    if (m_lossStreak >= 2) return QStringLiteral("\u8FDE\u8D25%1").arg(m_lossStreak);
    return QString();
}

void Player::onCombatWin()
{
    ++m_winStreak;
    m_lossStreak = 0;
}

void Player::onCombatLose()
{
    ++m_lossStreak;
    m_winStreak = 0;
}
