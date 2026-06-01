#include "entity/mageunit.h"
#include "core/board.h"
#include <QtMath>

MageUnit::MageUnit(Controller owner)
    : Unit(QStringLiteral("\u6CD5\u5E08"), owner)
/*法师：远程法术输出，高ATK短蓝条，羁绊标签为"法师"*/
{
    addTrait(QStringLiteral("\u6CD5\u5E08"));
}

void MageUnit::castSkill(Board&, const QList<Unit*>& allUnits)
/*技能【烈焰风暴】：对周围2格内所有敌方单位造成(ATK+25)×技能伤害的AOE伤害*/
{
    setMana(0);
    const QPoint center = position();
    for (Unit* unit : allUnits) {
        if (!unit || unit->owner() == owner() || !unit->isAlive()) {
            continue;
        }
        const QPoint d = unit->position() - center;
        const qreal dist = qSqrt(d.x() * d.x() + d.y() * d.y());
        if (dist <= 2.0) {
            const int dmg = static_cast<int>((atk() + 25) * skillDamageMultiplier());
            unit->takeDamage(dmg);
        }
    }
}
