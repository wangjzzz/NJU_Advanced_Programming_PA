#include "entity/warriorunit.h"
#include "core/board.h"
#include "entity/combattypes.h"

WarriorUnit::WarriorUnit(Controller owner)
    : Unit(QStringLiteral("战士"), owner)
{
    addTrait(QStringLiteral("战士"));
}

void WarriorUnit::castSkill(Board&, const QList<Unit*>& allUnits)
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
        const int dmg = static_cast<int>((atk() / 2) * skillDamageMultiplier());
        target->takeDamage(dmg);
        target->setStunTimer(CombatConst::kStunDuration);
    }
}
