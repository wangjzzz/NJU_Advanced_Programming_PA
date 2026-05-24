#ifndef WARRIORUNIT_H
#define WARRIORUNIT_H

#include "entity/unit.h"

class WarriorUnit : public Unit
{
public:
    explicit WarriorUnit(Controller owner = Controller::PlayerCtrl);

    QString heroType() const override { return QStringLiteral("战士"); }
    void castSkill(Board& board, const QList<Unit*>& allUnits) override;
};

#endif
