#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include "entity/combattypes.h"
#include <QList>
#include <QPoint>
#include <QVector>
#include <vector>

class Board;
class Unit;

struct AttackLineRecord
{
    QPoint from;
    QPoint to;
    bool isHeal = false;
};

class CombatSystem
{
public:
    void tick(Board& board, const QList<Unit*>& allUnits);
    bool checkBattleEnd(const Board& board, bool& playerWon) const;

    const std::vector<AttackLineRecord>& lastAttackLines() const { return m_lastAttackLines; }

private:
    Unit* findUnitById(const QList<Unit*>& allUnits, int id) const;
    QList<Unit*> collectBoardUnits(const Board& board, const QList<Unit*>& allUnits) const;

    Unit* selectTarget(Unit* self, const Board& board, const QList<Unit*>& allUnits) const;
    bool isInAttackRange(const Unit* a, const Unit* b) const;
    qreal gridDistance(const QPoint& a, const QPoint& b) const;
    bool compareTargetPriority(Unit* candidate, Unit* currentBest, const QPoint& selfPos) const;

    QVector<QPoint> findPath(const Board& board, Unit* unit, const QPoint& targetPos) const;
    bool tryMoveAlongPath(Board& board, Unit* unit);
    void performBasicAttack(Unit* attacker, Unit* target);

    void updateUnit(Board& board, Unit* unit, const QList<Unit*>& allUnits);

    std::vector<AttackLineRecord> m_lastAttackLines;
};

#endif
