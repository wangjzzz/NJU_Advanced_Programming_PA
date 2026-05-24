#include "entity/enemyunit.h"
#include "core/board.h"

EnemyUnit::EnemyUnit(const QString& name, const QString& trait)
    : Unit(name, Controller::EnemyCtrl)
{
    addTrait(trait);
}

void EnemyUnit::castSkill(Board&, const QList<Unit*>& allUnits)
{
    setMana(0);
    Unit* target = nullptr;
    for (Unit* unit : allUnits) {
        if (unit && unit->id() == targetId()) {
            target = unit;
            break;
        }
    }
    if (target && target->isAlive()) {
        const int dmg = static_cast<int>((atk() + 10) * skillDamageMultiplier());
        target->takeDamage(dmg);
    }
}
