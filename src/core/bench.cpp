#include "core/bench.h"

Bench::Bench()
    : m_slots(BENCH_SIZE, nullptr)
{
}

bool Bench::addUnit(Unit* unit)  
/*在棋盘上添加Unit，确保Unit非空，然后遍历检查一维数组备战区序号找到合适的位置*/
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
/*在指定的一维数组备战区上放置Unit*/
{
    if (!unit || !isSlotValid(slot)) {
        return false;
    }
    m_slots[slot] = unit;
    return true;
}

Unit* Bench::removeUnit(int slot)
/*移除在一维数组备战区slot位置上的Unit*/
{
    if (!isSlotValid(slot)) {
        return nullptr;
    }
    Unit* unit = m_slots[slot];
    m_slots[slot] = nullptr;
    return unit;
}

Unit* Bench::getUnitAt(int slot) const
/*返回在slot位置的Unit指针*/
{
    if (!isSlotValid(slot)) {
        return nullptr;
    }
    return m_slots[slot];
}

bool Bench::hasUnitAt(int slot) const
/*判断slot处是否有Unit，返回bool值*/
{
    return getUnitAt(slot) != nullptr;
}

bool Bench::isSlotValid(int slot) const
/*判断slot是否有效（越界）*/
{
    return slot >= 0 && slot < BENCH_SIZE;
}

void Bench::clear()
/*清除Bench上的单位*/
{
    std::fill(m_slots.begin(), m_slots.end(), nullptr);
}

void Bench::swapUnits(int slot1, int slot2)
/*交换两个slot位置的Unit*/
{
    if (!isSlotValid(slot1) || !isSlotValid(slot2)) {
        return;
    }
    std::swap(m_slots[slot1], m_slots[slot2]);
}

int Bench::unitCount() const
/*返回现有的Unit数量*/
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
/*判断备战区是不是满了*/
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
/*找到Unit的slot序号*/
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
