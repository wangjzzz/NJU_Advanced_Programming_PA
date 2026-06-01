#include "entity/warriorunit.h"
#include "core/board.h"
#include "entity/combattypes.h"

WarriorUnit::WarriorUnit(Controller owner)
    : Unit(QStringLiteral("\u6218\u58EB"), owner)
/*战士：前排近战坦克，血量高射程短，羁绊标签为"战士"*/
{
    addTrait(QStringLiteral("\u6218\u58EB"));
}

void WarriorUnit::castSkill(Board&, const QList<Unit*>& allUnits)
/*技能【盾击】：对当前目标造成ATK/2的伤害并眩晕90帧*/
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
