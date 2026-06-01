#include "entity/enemyunit.h"
#include "core/board.h"

EnemyUnit::EnemyUnit(const QString& name, const QString& trait)
    : Unit(name, Controller::EnemyCtrl)
/*敌方单位：由关卡配置生成，属性随回合数缩放，可持有单一羁绊标签*/
{
    addTrait(trait);
}

void EnemyUnit::castSkill(Board&, const QList<Unit*>& allUnits)
/*技能【强化打击】：对当前目标造成(ATK+10)×技能伤害的额外伤害*/
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
