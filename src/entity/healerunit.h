#ifndef HEALERUNIT_H
#define HEALERUNIT_H

#include "entity/unit.h"

class HealerUnit : public Unit
{
public:
    explicit HealerUnit(Controller owner = Controller::PlayerCtrl);

    QString heroType() const override { return QStringLiteral("牧师"); }
    void castSkill(Board& board, const QList<Unit*>& allUnits) override;
};

#endif
