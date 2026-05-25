#ifndef UNITFACTORY_H
#define UNITFACTORY_H

#include "entity/unit.h"
#include <QString>

class UnitFactory
{
public:
    static Unit* createHero(const QString& name, Controller owner);
    static Unit* createEnemy(const QString& name, const QString& trait, int round, bool isBoss = false);
};

#endif
