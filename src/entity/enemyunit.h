#ifndef ENEMYUNIT_H
#define ENEMYUNIT_H

#include "entity/unit.h"

class EnemyUnit : public Unit
{
public:
    EnemyUnit(const QString& name, const QString& trait);

    QString heroType() const override { return QStringLiteral("敌人"); }
    void castSkill(Board& board, const QList<Unit*>& allUnits) override;
};

#endif
