#include "entity/healerunit.h"
#include "core/board.h"
#include <QtMath>

HealerUnit::HealerUnit(Controller owner)
    : Unit(QStringLiteral("牧师"), owner)
{
    addTrait(QStringLiteral("辅助"));
}

void HealerUnit::castSkill(Board&, const QList<Unit*>& allUnits)
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
