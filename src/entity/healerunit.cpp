#include "entity/healerunit.h"
#include "core/board.h"
#include <QtMath>

HealerUnit::HealerUnit(Controller owner)
    : Unit(QStringLiteral("\u7267\u5E08"), owner)
/*牧师：辅助治疗者，普攻改为治疗友军，羁绊标签为"辅助"*/
{
    addTrait(QStringLiteral("\u8F85\u52A9"));
}

void HealerUnit::castSkill(Board&, const QList<Unit*>& allUnits)
/*技能【圣光】：对周围2.5格内所有友方单位回复80点生命值*/
{
    setMana(0);
    const QPoint center = position();
    for (Unit* unit : allUnits) {
        if (!unit || unit->owner() != owner() || !unit->isAlive()) {
            continue;
        }
        const QPoint d = unit->position() - center;
        const qreal dist = qSqrt(d.x() * d.x() + d.y() * d.y());
        if (dist <= 2.5) {
            unit->heal(80);
        }
    }
}
