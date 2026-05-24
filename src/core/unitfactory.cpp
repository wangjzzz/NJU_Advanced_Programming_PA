#include "core/unitfactory.h"
#include "entity/archerunit.h"
#include "entity/enemyunit.h"
#include "entity/healerunit.h"
#include "entity/mageunit.h"
#include "entity/warriorunit.h"

Unit* UnitFactory::createHero(const QString& name, Controller owner)
{
    if (name == QStringLiteral("战士")) {
        return new WarriorUnit(owner);
    }
    if (name == QStringLiteral("弓手")) {
        return new ArcherUnit(owner);
    }
    if (name == QStringLiteral("法师")) {
        return new MageUnit(owner);
    }
    if (name == QStringLiteral("牧师")) {
        return new HealerUnit(owner);
    }
    return new Unit(name, owner);
}

Unit* UnitFactory::createEnemy(const QString& name, const QString& trait, int round)
{
    const int scale = qMax(0, (round - 1) * 20);
    EnemyUnit* enemy = new EnemyUnit(name, trait);

    if (name == QStringLiteral("骷髅")) {
        enemy->setMaxHp(220 + scale);
        enemy->setAtk(28 + scale / 2);
        enemy->setRange(1);
    } else if (name == QStringLiteral("幽灵")) {
        enemy->setMaxHp(190 + scale);
        enemy->setAtk(32 + scale / 2);
        enemy->setRange(2);
        enemy->setMaxMana(50);
    } else if (name == QStringLiteral("恶魔")) {
        enemy->setMaxHp(380 + scale);
        enemy->setAtk(40 + scale / 2);
        enemy->setRange(1);
    } else {
        enemy->setMaxHp(200 + scale);
        enemy->setAtk(25 + scale / 2);
        enemy->setRange(1);
    }

    enemy->setHp(enemy->maxHp());
    return enemy;
}
