#include "core/combat_system.h"
#include "core/board.h"
#include "entity/combattypes.h"
#include "entity/unit.h"
#include <QQueue>
#include <QtMath>
#include <QHash>
#include <QRandomGenerator>
#include <QSet>
#include <algorithm>

namespace {
const QPoint kDirs8[] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
};
}

Unit* CombatSystem::findUnitById(const QList<Unit*>& allUnits, int id) const
{
    for (Unit* unit : allUnits) {
        if (unit && unit->id() == id) {
            return unit;
        }
    }
    return nullptr;
}

QList<Unit*> CombatSystem::collectBoardUnits(const Board& board, const QList<Unit*>& allUnits) const
{
    QList<Unit*> result;
    for (Unit* unit : allUnits) {
        if (!unit || !unit->isAlive()) {
            continue;
        }
        if (board.getUnitAt(unit->position()) == unit) {
            result.append(unit);
        }
    }
    return result;
}

qreal CombatSystem::gridDistance(const QPoint& a, const QPoint& b) const
{
    const qreal dx = a.x() - b.x();
    const qreal dy = a.y() - b.y();
    return qSqrt(dx * dx + dy * dy);
}

bool CombatSystem::isInAttackRange(const Unit* a, const Unit* b) const
{
    if (!a || !b) {
        return false;
    }
    return gridDistance(a->position(), b->position()) <= a->range() + 0.01;
}

bool CombatSystem::compareTargetPriority(Unit* candidate, Unit* currentBest, const QPoint& selfPos) const
{
    if (!currentBest) {
        return true;
    }

    const qreal distC = gridDistance(selfPos, candidate->position());
    const qreal distB = gridDistance(selfPos, currentBest->position());
    if (!qFuzzyCompare(distC, distB) && distC != distB) {
        return distC < distB;
    }
    if (candidate->hp() != currentBest->hp()) {
        return candidate->hp() < currentBest->hp();
    }
    if (candidate->position().x() != currentBest->position().x()) {
        return candidate->position().x() < currentBest->position().x();
    }
    return candidate->position().y() > currentBest->position().y();
}

Unit* CombatSystem::selectTarget(Unit* self, const Board& board, const QList<Unit*>& allUnits) const
{
    Unit* best = nullptr;
    const QPoint selfPos = self->position();

    for (Unit* other : allUnits) {
        if (!other || other == self || !other->isAlive()) {
            continue;
        }
        if (other->owner() == self->owner()) {
            continue;
        }
        if (board.getUnitAt(other->position()) != other) {
            continue;
        }
        if (compareTargetPriority(other, best, selfPos)) {
            best = other;
        }
    }
    return best;
}

QVector<QPoint> CombatSystem::findPath(const Board& board, Unit* unit, const QPoint& targetPos) const
{
    const QPoint start = unit->position();
    if (!board.isValidPosition(start) || !board.isValidPosition(targetPos)) {
        return {};
    }

    QQueue<QPoint> queue;
    QHash<QPoint, QPoint> parent;
    QSet<QPoint> visited;
    queue.enqueue(start);
    visited.insert(start);

    auto isGoal = [&](const QPoint& p) {
        const qreal dx = p.x() - targetPos.x();
        const qreal dy = p.y() - targetPos.y();
        return qSqrt(dx * dx + dy * dy) <= unit->range() + 0.01;
    };

    QPoint found = start;
    bool reached = isGoal(start);

    while (!queue.isEmpty() && !reached) {
        const QPoint cur = queue.dequeue();
        for (const QPoint& dir : kDirs8) {
            const QPoint next = cur + dir;
            if (!board.isValidPosition(next) || visited.contains(next)) {
                continue;
            }
            if (next != start && board.hasUnitAt(next)) {
                continue;
            }
            visited.insert(next);
            parent.insert(next, cur);
            queue.enqueue(next);
            if (isGoal(next)) {
                found = next;
                reached = true;
                break;
            }
            if (!reached) {
                found = next;
            }
        }
    }

    QVector<QPoint> path;
    QPoint cur = found;
    while (cur != start) {
        path.prepend(cur);
        if (!parent.contains(cur)) {
            path.clear();
            break;
        }
        cur = parent.value(cur);
    }
    return path;
}

bool CombatSystem::tryMoveAlongPath(Board& board, Unit* unit)
{
    if (unit->path().isEmpty()) {
        return false;
    }

    unit->setMoveTimer(unit->moveTimer() + 1);
    if (unit->moveTimer() < CombatConst::kMoveInterval) {
        return false;
    }
    unit->setMoveTimer(0);

    const QPoint next = unit->path().first();
    if (!board.isValidPosition(next) || (board.hasUnitAt(next) && board.getUnitAt(next) != unit)) {
        unit->clearPath();
        unit->setState(UnitState::Idle);
        return false;
    }

    board.removeUnit(unit);
    board.addUnit(unit, next);
    QVector<QPoint> rest = unit->path();
    rest.removeFirst();
    unit->setPath(rest);
    if (rest.isEmpty()) {
        unit->setState(UnitState::Idle);
    }
    return true;
}

void CombatSystem::performBasicAttack(Unit* attacker, Unit* target)
{
    if (!attacker || !target || !target->isAlive()) {
        return;
    }

    auto strike = [&]() {
        target->takeDamage(attacker->atk());
        attacker->gainMana(CombatConst::kManaPerAttack);
    };

    strike();
    if (attacker->doubleAttackChance() > 0.0
        && QRandomGenerator::global()->generateDouble() < attacker->doubleAttackChance()) {
        strike();
    }
}

void CombatSystem::updateUnit(Board& board, Unit* unit, const QList<Unit*>& allUnits)
{
    if (!unit || unit->isDead()) {
        return;
    }
    if (board.getUnitAt(unit->position()) != unit) {
        return;
    }

    if (unit->stunTimer() > 0) {
        unit->setStunTimer(unit->stunTimer() - 1);
        return;
    }

    if (unit->state() == UnitState::Casting) {
        unit->setCastTimer(unit->castTimer() + 1);
        if (unit->castTimer() >= CombatConst::kCastDuration) {
            unit->castSkill(board, allUnits);
            unit->setCastTimer(0);
            unit->setState(UnitState::Idle);
        }
        return;
    }

    Unit* target = findUnitById(allUnits, unit->targetId());
    if (!target || !target->isAlive() || target->owner() == unit->owner()
        || board.getUnitAt(target->position()) != target) {
        target = selectTarget(unit, board, allUnits);
        unit->setTargetId(target ? target->id() : -1);
        unit->clearPath();
        unit->setState(UnitState::Idle);
    }

    if (!target) {
        unit->setState(UnitState::Idle);
        return;
    }

    if (isInAttackRange(unit, target)) {
        unit->clearPath();
        unit->setState(UnitState::Attacking);
        unit->setAttackTimer(unit->attackTimer() + 1);
        if (unit->attackTimer() >= unit->attackIntervalFrames()) {
            unit->setAttackTimer(0);
            performBasicAttack(unit, target);
            if (unit->mana() >= unit->maxMana()) {
                unit->setState(UnitState::Casting);
                unit->setCastTimer(0);
            }
        }
        return;
    }

    unit->setState(UnitState::Moving);
    if (unit->path().isEmpty()) {
        unit->setPath(findPath(board, unit, target->position()));
        unit->setMoveTimer(0);
    }
    tryMoveAlongPath(board, unit);
}

void CombatSystem::tick(Board& board, const QList<Unit*>& allUnits)
{
    QList<Unit*> fighters = collectBoardUnits(board, allUnits);
    std::sort(fighters.begin(), fighters.end(), [](Unit* a, Unit* b) {
        return a->id() < b->id();
    });

    for (Unit* unit : fighters) {
        if (unit->isDead()) {
            continue;
        }
        updateUnit(board, unit, allUnits);
    }

    for (Unit* unit : allUnits) {
        if (!unit || !unit->isDead()) {
            continue;
        }
        if (board.getUnitAt(unit->position()) == unit) {
            board.removeUnit(unit);
        }
    }
}

bool CombatSystem::checkBattleEnd(const Board& board, bool& playerWon) const
{
    bool hasPlayer = false;
    bool hasEnemy = false;

    for (int row = 0; row < Board::ROWS; ++row) {
        for (int col = 0; col < Board::COLS; ++col) {
            Unit* unit = board.getUnitAt(QPoint(col, row));
            if (!unit || !unit->isAlive()) {
                continue;
            }
            if (unit->owner() == Controller::PlayerCtrl) {
                hasPlayer = true;
            } else {
                hasEnemy = true;
            }
        }
    }

    if (hasPlayer && hasEnemy) {
        return false;
    }
    if (!hasPlayer && !hasEnemy) {
        playerWon = true;
        return true;
    }
    playerWon = hasPlayer && !hasEnemy;
    return true;
}
