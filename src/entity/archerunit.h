#ifndef ARCHERUNIT_H
#define ARCHERUNIT_H

#include "entity/unit.h"

class ArcherUnit : public Unit
{
public:
    explicit ArcherUnit(Controller owner = Controller::PlayerCtrl);

    QString heroType() const override { return QStringLiteral("弓手"); }
    void castSkill(Board& board, const QList<Unit*>& allUnits) override;
};

#endif
