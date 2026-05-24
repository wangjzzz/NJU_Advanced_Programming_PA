#include "core/equipment.h"
#include <QRandomGenerator>

EquipmentInfo Equipment::info(EquipType type)
{
    switch (type) {
    case EquipType::IronSword:
        return {type, QStringLiteral("铁剑"), QStringLiteral("攻击力 +15")};
    case EquipType::ChainMail:
        return {type, QStringLiteral("锁子甲"), QStringLiteral("生命值 +150")};
    case EquipType::SwiftGloves:
        return {type, QStringLiteral("急速手套"), QStringLiteral("攻击速度 +20%")};
    case EquipType::ManaCrystal:
        return {type, QStringLiteral("蓝水晶"), QStringLiteral("最大法力 -30")};
    default:
        return {EquipType::None, QString(), QString()};
    }
}

QString Equipment::typeToString(EquipType type)
{
    return info(type).name;
}

EquipType Equipment::randomDrop()
{
    const int roll = QRandomGenerator::global()->bounded(4);
    switch (roll) {
    case 0: return EquipType::IronSword;
    case 1: return EquipType::ChainMail;
    case 2: return EquipType::SwiftGloves;
    default: return EquipType::ManaCrystal;
    }
}
