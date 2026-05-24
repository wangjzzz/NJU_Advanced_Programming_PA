#include "core/equipbench.h"

EquipBench::EquipBench() = default;

bool EquipBench::add(EquipType type)
{
    if (type == EquipType::None || m_slots.size() >= MAX_SLOTS) {
        return false;
    }
    m_slots.append(type);
    return true;
}

EquipType EquipBench::take(int slot)
{
    if (!isSlotValid(slot)) {
        return EquipType::None;
    }
    const EquipType type = m_slots[slot];
    m_slots.removeAt(slot);
    return type;
}

EquipType EquipBench::peek(int slot) const
{
    if (!isSlotValid(slot)) {
        return EquipType::None;
    }
    return m_slots[slot];
}

bool EquipBench::isSlotValid(int slot) const
{
    return slot >= 0 && slot < m_slots.size();
}

void EquipBench::clear()
{
    m_slots.clear();
}

int EquipBench::count() const
{
    return m_slots.size();
}

QVector<EquipType> EquipBench::all() const
{
    return m_slots;
}
