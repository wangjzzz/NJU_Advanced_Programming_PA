#ifndef MAGEUNIT_H
#define MAGEUNIT_H

#include "entity/unit.h"

class MageUnit : public Unit
{
public:
    explicit MageUnit(Controller owner = Controller::PlayerCtrl);

    QString heroType() const override { return QStringLiteral("法师"); }
    void castSkill(Board& board, const QList<Unit*>& allUnits) override;
};

#endif
