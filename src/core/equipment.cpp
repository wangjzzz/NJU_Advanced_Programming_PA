#include "core/equipment.h"
#include <QRandomGenerator>

EquipmentInfo Equipment::info(EquipType type)
/*返回装备类型对应的名字和效果描述*/
{
    switch (type) {
    case EquipType::IronSword:
        return {type, QStringLiteral("\u94C1\u5251"), QStringLiteral("\u653B\u51FB\u529B +15")};
    case EquipType::ChainMail:
        return {type, QStringLiteral("\u9501\u5B50\u7532"), QStringLiteral("\u751F\u547D\u503C +150")};
    case EquipType::SwiftGloves:
        return {type, QStringLiteral("\u6025\u901F\u624B\u5957"), QStringLiteral("\u653B\u51FB\u901F\u5EA6 +20%")};
    case EquipType::ManaCrystal:
        return {type, QStringLiteral("\u84DD\u6C34\u6676"), QStringLiteral("\u6700\u5927\u6CD5\u529B -30")};
    default:
        return {EquipType::None, QString(), QString()};
    }
}

QString Equipment::typeToString(EquipType type)
/*装备类型枚举转字符串名称*/
{
    return info(type).name;
}

EquipType Equipment::randomDrop()
/*随机掉落一件基础装备，四种等概率*/
{
    const int roll = QRandomGenerator::global()->bounded(4);
    switch (roll) {
    case 0: return EquipType::IronSword;
    case 1: return EquipType::ChainMail;
    case 2: return EquipType::SwiftGloves;
    default: return EquipType::ManaCrystal;
    }
}
