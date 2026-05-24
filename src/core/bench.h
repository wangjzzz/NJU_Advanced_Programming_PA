#ifndef BENCH_H
#define BENCH_H

#include <QVector>
#include "entity/unit.h"

class Bench
{
public:
    static constexpr int BENCH_SIZE = 8;

    Bench();

    bool addUnit(Unit* unit);
    bool addUnitToSlot(Unit* unit, int slot);
    Unit* removeUnit(int slot);
    Unit* getUnitAt(int slot) const;
    bool hasUnitAt(int slot) const;
    bool isSlotValid(int slot) const;

    void clear();
    void swapUnits(int slot1, int slot2);

    int unitCount() const;
    bool isFull() const;

    QVector<Unit*> getAllUnits() const;
    int findUnitSlot(Unit* unit) const;

private:
    QVector<Unit*> m_slots;
};

#endif // BENCH_H
