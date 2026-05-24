#ifndef SHOP_H
#define SHOP_H

#include <QString>
#include <array>

class Shop
{
public:
    static constexpr int SLOT_COUNT = 5;

    Shop();

    const std::array<QString, SLOT_COUNT>& offers() const { return m_offers; }
    const std::array<int, SLOT_COUNT>& costs() const { return m_costs; }

    QString offerAt(int slot) const;
    int costAt(int slot) const;
    bool isSlotValid(int slot) const;

    void refresh();
    void clearSlot(int slot);
    void setSlot(int slot, const QString& name, int cost);

private:
    QString rollHero() const;
    int priceFor(const QString& hero) const;

    std::array<QString, SLOT_COUNT> m_offers;
    std::array<int, SLOT_COUNT> m_costs;
};

#endif
