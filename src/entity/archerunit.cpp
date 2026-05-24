#include "entity/archerunit.h"
#include "core/board.h"

ArcherUnit::ArcherUnit(Controller owner)
    : Unit(QStringLiteral("弓手"), owner)
{
    addTrait(QStringLiteral("游侠"));
}

void ArcherUnit::castSkill(Board& board, const QList<Unit*>& allUnits)
{
    setMana(0);
    const int row = position().y();
    for (Unit* unit : allUnits) {
        if (!unit || unit->owner() == owner() || !unit->isAlive()) {
            continue;
        }
        if (unit->position().y() == row) {
            const int dmg = static_cast<int>((atk() + 15) * skillDamageMultiplier());
            unit->takeDamage(dmg);
        }
    }
    Q_UNUSED(board);
}
