#include "core/bench.h"

Bench::Bench()
    : m_slots(BENCH_SIZE, nullptr)
{
}

bool Bench::addUnit(Unit* unit)  
{
    if (!unit) {
        return false;
    }

    for (int i = 0; i < BENCH_SIZE; ++i) {
        if (!m_slots[i]) {
            m_slots[i] = unit;
            return true;
        }
    }
    return false;
}

bool Bench::addUnitToSlot(Unit* unit, int slot)
{
    if (!unit || !isSlotValid(slot)) {
        return false;
    }
    m_slots[slot] = unit;
    return true;
}

Unit* Bench::removeUnit(int slot)
{
    if (!isSlotValid(slot)) {
        return nullptr;
    }
    Unit* unit = m_slots[slot];
    m_slots[slot] = nullptr;
    return unit;
}

Unit* Bench::getUnitAt(int slot) const
{
    if (!isSlotValid(slot)) {
        return nullptr;
    }
    return m_slots[slot];
}

bool Bench::hasUnitAt(int slot) const
{
    return getUnitAt(slot) != nullptr;
}

bool Bench::isSlotValid(int slot) const
{
    return slot >= 0 && slot < BENCH_SIZE;
}

void Bench::clear()
{
    std::fill(m_slots.begin(), m_slots.end(), nullptr);
}

void Bench::swapUnits(int slot1, int slot2)
{
    if (!isSlotValid(slot1) || !isSlotValid(slot2)) {
        return;
    }
    std::swap(m_slots[slot1], m_slots[slot2]);
}

int Bench::unitCount() const
{
    int count = 0;
    for (Unit* unit : m_slots) {
        if (unit) {
            ++count;
        }
    }
    return count;
}

bool Bench::isFull() const
{
    return unitCount() >= BENCH_SIZE;
}

QVector<Unit*> Bench::getAllUnits() const
{
    QVector<Unit*> units;
    for (Unit* unit : m_slots) {
        if (unit) {
            units.append(unit);
        }
    }
    return units;
}

int Bench::findUnitSlot(Unit* unit) const
{
    if (!unit) {
        return -1;
    }
    for (int i = 0; i < BENCH_SIZE; ++i) {
        if (m_slots[i] == unit) {
            return i;
        }
    }
    return -1;
}
