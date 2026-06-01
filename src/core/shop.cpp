#include "core/shop.h"
#include <QRandomGenerator>

namespace {
const char* kHeroPool[] = {
    "战士", "弓手", "法师", "牧师"
};
}

Shop::Shop()
{
    refresh();
}

QString Shop::offerAt(int slot) const
/*返回指定槽位的英雄名称，槽位非法返回空字符串*/
{
    return isSlotValid(slot) ? m_offers[slot] : QString();
}

int Shop::costAt(int slot) const
/*返回指定槽位英雄的价格，槽位非法返回0*/
{
    return isSlotValid(slot) ? m_costs[slot] : 0;
}

bool Shop::isSlotValid(int slot) const
/*判断商店槽位编号是否有效*/
{
    return slot >= 0 && slot < SLOT_COUNT;
}

QString Shop::rollHero() const
/*从英雄池中随机抽取一个英雄名称*/
{
    const int idx = QRandomGenerator::global()->bounded(4);
    return QString::fromUtf8(kHeroPool[idx]);
}

int Shop::priceFor(const QString& hero) const
/*根据英雄名称返回购买价格*/
{
    if (hero == QStringLiteral("\u6218\u58EB")) return 3;
    if (hero == QStringLiteral("\u5F13\u624B")) return 4;
    if (hero == QStringLiteral("\u6CD5\u5E08")) return 5;
    if (hero == QStringLiteral("\u7267\u5E08")) return 4;
    return 3;
}

void Shop::refresh()
/*刷新商店：五个槽位全部重新随机*/
{
    for (int i = 0; i < SLOT_COUNT; ++i) {
        m_offers[i] = rollHero();
        m_costs[i] = priceFor(m_offers[i]);
    }
}

void Shop::clearSlot(int slot)
/*清空指定槽位（购买后调用）*/
{
    if (!isSlotValid(slot)) {
        return;
    }
    m_offers[slot].clear();
    m_costs[slot] = 0;
}

void Shop::setSlot(int slot, const QString& name, int cost)
/*设置指定槽位的英雄和价格（用于读档恢复）*/
{
    if (!isSlotValid(slot)) {
        return;
    }
    m_offers[slot] = name;
    m_costs[slot] = cost;
}
