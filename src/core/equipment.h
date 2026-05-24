#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QString>

enum class EquipType {
    None = 0,
    IronSword,
    ChainMail,
    SwiftGloves,
    ManaCrystal
};

struct EquipmentInfo
{
    EquipType type;
    QString name;
    QString description;
};

class Equipment
{
public:
    static EquipmentInfo info(EquipType type);
    static QString typeToString(EquipType type);
    static EquipType randomDrop();
};

#endif
