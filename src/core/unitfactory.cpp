#include "core/unitfactory.h"
#include "entity/archerunit.h"
#include "entity/enemyunit.h"
#include "entity/healerunit.h"
#include "entity/mageunit.h"
#include "entity/warriorunit.h"
#include <QtMath>

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

Unit* UnitFactory::createEnemy(const QString& name, const QString& trait, int round, bool isBoss)
{
    const int tier = (round - 1) / 3;
    const qreal multiplier = qPow(1.25, tier);
    const qreal bossMult = isBoss ? 3.0 : 1.0;
    const qreal bossAtkMult = isBoss ? 2.0 : 1.0;
    EnemyUnit* enemy = new EnemyUnit(name, trait);
    if (isBoss) {
        enemy->setIsBoss(true);
    }

    if (name == QStringLiteral("骷髅")) {
        enemy->setMaxHp(static_cast<int>(220 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(28 * multiplier * bossAtkMult));
        enemy->setRange(1);
    } else if (name == QStringLiteral("幽灵")) {
        enemy->setMaxHp(static_cast<int>(190 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(32 * multiplier * bossAtkMult));
        enemy->setRange(2);
        enemy->setMaxMana(50);
    } else if (name == QStringLiteral("恶魔")) {
        enemy->setMaxHp(static_cast<int>(380 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(40 * multiplier * bossAtkMult));
        enemy->setRange(1);
    } else {
        enemy->setMaxHp(static_cast<int>(200 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(25 * multiplier * bossAtkMult));
        enemy->setRange(1);
    }

    enemy->setHp(enemy->maxHp());
    return enemy;
}
