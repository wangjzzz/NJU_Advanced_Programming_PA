#include "board.h"
#include "entity/unit.h"

Board::Board()
    : m_cells(ROWS * COLS, nullptr)
{}

void Board::addUnit(Unit* unit, const QPoint& pos)
/*在棋盘指定位置添加单位，若该位置已有单位或坐标非法则忽略*/
{
    const int idx = indexOf(pos);
    if (!unit || idx < 0 || m_cells[idx]) {
        return;
    }

    m_cells[idx] = unit;
    m_unitToPosition[unit] = pos;
    unit->setPosition(pos);
}

void Board::removeUnit(Unit* unit)
/*从棋盘上移除指定单位*/
{
    if (!unit || !m_unitToPosition.contains(unit)) {
        return;
    }

    const int idx = indexOf(m_unitToPosition.value(unit));
    if (idx >= 0) {
        m_cells[idx] = nullptr;
    }
    m_unitToPosition.remove(unit);
}

Unit* Board::getUnitAt(const QPoint& pos) const
/*返回棋盘指定位置上的单位指针，无单位则返回空指针*/
{
    const int idx = indexOf(pos);
    return idx < 0 ? nullptr : m_cells[idx];
}

bool Board::hasUnitAt(const QPoint& pos) const
/*判断棋盘指定位置是否有单位占据*/
{
    return getUnitAt(pos) != nullptr;
}

bool Board::isValidPosition(const QPoint& pos) const
/*判断坐标是否在棋盘范围内*/
{
    return pos.x() >= 0 && pos.x() < COLS && pos.y() >= 0 && pos.y() < ROWS;
}

bool Board::isPlayerHalf(const QPoint& pos) const
/*判断坐标是否在玩家半场（棋盘下半区）*/
{
    return pos.y() >= ROWS / 2;
}

void Board::clear()
/*清空棋盘上所有单位*/
{
    std::fill(m_cells.begin(), m_cells.end(), nullptr);
    m_unitToPosition.clear();
}

int Board::indexOf(const QPoint& pos) const
/*将二维棋盘坐标转换为一维数组索引*/
{
    if (!isValidPosition(pos)) {
        return -1;
    }
    return pos.y() * COLS + pos.x();
}
