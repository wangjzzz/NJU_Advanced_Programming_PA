#include "entity/archerunit.h"
#include "core/board.h"

ArcherUnit::ArcherUnit(Controller owner)
    : Unit(QStringLiteral("\u5F13\u624B"), owner)
/*弓手：远程游侠，射程3格，羁绊标签为"游侠"*/
{
    addTrait(QStringLiteral("\u6E38\u4FA0"));
}

void ArcherUnit::castSkill(Board& board, const QList<Unit*>& allUnits)
/*技能【穿云箭】：对同一行所有敌方单位造成(ATK+15)×技能伤害的伤害*/
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
