#include "entity/mageunit.h"
#include "core/board.h"
#include <QtMath>

MageUnit::MageUnit(Controller owner)
    : Unit(QStringLiteral("法师"), owner)
{
    addTrait(QStringLiteral("法师"));
}

void MageUnit::castSkill(Board&, const QList<Unit*>& allUnits)
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
