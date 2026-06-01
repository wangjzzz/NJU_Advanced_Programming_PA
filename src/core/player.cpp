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
/*增加金币，负数忽略*/
{
    if (amount > 0) {
        m_gold += amount;
    }
}

bool Player::spendGold(int amount)
/*消费金币，不足或非正数返回false*/
{
    if (amount <= 0 || m_gold < amount) {
        return false;
    }
    m_gold -= amount;
    return true;
}

int Player::populationUpgradeCost() const
/*计算下次升级人口所需的金币，随升级次数递增*/
{
    return 4 + m_populationUpgradeCount * 2;
}

bool Player::upgradePopulationCap()
/*升级人口上限，最多9，扣除金币并+1上限*/
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
/*重置玩家所有状态到初始值*/
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
/*计算利息：每10金额外+1金，封顶5金*/
{
    return qMin(m_gold / 10, 5);
}

int Player::streakBonus() const
/*计算连胜/连败额外金币奖励*/
{
    if (m_winStreak >= 6) return 3;
    if (m_winStreak >= 4) return 2;
    if (m_winStreak >= 2) return 1;
    if (m_lossStreak >= 4) return 2;
    if (m_lossStreak >= 2) return 1;
    return 0;
}

QString Player::streakText() const
/*返回连胜/连败的显示文本，如"连胜3"或"连败2"*/
{
    if (m_winStreak >= 2) return QStringLiteral("\u8FDE\u80DC%1").arg(m_winStreak);
    if (m_lossStreak >= 2) return QStringLiteral("\u8FDE\u8D25%1").arg(m_lossStreak);
    return QString();
}

void Player::onCombatWin()
/*处理战斗胜利：连胜+1，连败清零*/
{
    ++m_winStreak;
    m_lossStreak = 0;
}

void Player::onCombatLose()
/*处理战斗失败：连败+1，连胜清零*/
{
    ++m_lossStreak;
    m_winStreak = 0;
}
