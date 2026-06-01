#include "core/unitfactory.h"
#include "entity/archerunit.h"
#include "entity/enemyunit.h"
#include "entity/healerunit.h"
#include "entity/mageunit.h"
#include "entity/warriorunit.h"
#include <QtMath>

Unit* UnitFactory::createHero(const QString& name, Controller owner)
/*根据名称创建对应的英雄子类对象，用于商店购买和读档*/
{
    if (name == QStringLiteral("\u6218\u58EB")) {
        return new WarriorUnit(owner);
    }
    if (name == QStringLiteral("\u5F13\u624B")) {
        return new ArcherUnit(owner);
    }
    if (name == QStringLiteral("\u6CD5\u5E08")) {
        return new MageUnit(owner);
    }
    if (name == QStringLiteral("\u7267\u5E08")) {
        return new HealerUnit(owner);
    }
    return new Unit(name, owner);
}

Unit* UnitFactory::createEnemy(const QString& name, const QString& trait, int round, bool isBoss)
/*根据名称和回合数创建敌方单位，属性随轮次缩放，boss额外乘3倍HP和2倍ATK*/
{
    const int tier = (round - 1) / 3;
    const qreal multiplier = qPow(1.25, tier);
    const qreal bossMult = isBoss ? 3.0 : 1.0;
    const qreal bossAtkMult = isBoss ? 2.0 : 1.0;
    EnemyUnit* enemy = new EnemyUnit(name, trait);
    if (isBoss) {
        enemy->setIsBoss(true);
    }

    if (name == QStringLiteral("\u9AB7\u9AC5")) {
        enemy->setMaxHp(static_cast<int>(220 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(28 * multiplier * bossAtkMult));
        enemy->setRange(1);
    } else if (name == QStringLiteral("\u5E7D\u7075")) {
        enemy->setMaxHp(static_cast<int>(190 * multiplier * bossMult));
        enemy->setAtk(static_cast<int>(32 * multiplier * bossAtkMult));
        enemy->setRange(2);
        enemy->setMaxMana(50);
    } else if (name == QStringLiteral("\u6076\u9B54")) {
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
