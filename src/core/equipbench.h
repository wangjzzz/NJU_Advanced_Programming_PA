#ifndef EQUIPBENCH_H
#define EQUIPBENCH_H

#include "core/equipment.h"
#include <QVector>

class EquipBench
{
public:
    static constexpr int MAX_SLOTS = 6;

    EquipBench();

    bool add(EquipType type);
    EquipType take(int slot);
    EquipType peek(int slot) const;
    bool isSlotValid(int slot) const;
    void clear();

    int count() const;
    QVector<EquipType> all() const;

private:
    QVector<EquipType> m_slots;
};

#endif
