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
{
    return isSlotValid(slot) ? m_offers[slot] : QString();
}

int Shop::costAt(int slot) const
{
    return isSlotValid(slot) ? m_costs[slot] : 0;
}

bool Shop::isSlotValid(int slot) const
{
    return slot >= 0 && slot < SLOT_COUNT;
}

QString Shop::rollHero() const
{
    const int idx = QRandomGenerator::global()->bounded(4);
    return QString::fromUtf8(kHeroPool[idx]);
}

int Shop::priceFor(const QString& hero) const
{
    if (hero == QStringLiteral("战士")) return 3;
    if (hero == QStringLiteral("弓手")) return 4;
    if (hero == QStringLiteral("法师")) return 5;
    if (hero == QStringLiteral("牧师")) return 4;
    return 3;
}

void Shop::refresh()
{
    for (int i = 0; i < SLOT_COUNT; ++i) {
        m_offers[i] = rollHero();
        m_costs[i] = priceFor(m_offers[i]);
    }
}

void Shop::clearSlot(int slot)
{
    if (!isSlotValid(slot)) {
        return;
    }
    m_offers[slot].clear();
    m_costs[slot] = 0;
}

void Shop::setSlot(int slot, const QString& name, int cost)
{
    if (!isSlotValid(slot)) {
        return;
    }
    m_offers[slot] = name;
    m_costs[slot] = cost;
}
