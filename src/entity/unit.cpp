#include "entity/unit.h"
#include "core/board.h"
#include <QtMath>

int Unit::s_nextId = 0;

Unit::Unit(const QString& name, Controller owner)
    : m_id(s_nextId++)
    , m_name(name)
    , m_owner(owner)
    , m_hp(300)
    , m_maxHp(300)
    , m_atk(35)
    , m_range(1)
    , m_mana(0)
    , m_maxMana(60)
    , m_starLevel(1)
    , m_baseMaxHp(300)
    , m_baseAtk(35)
    , m_baseMaxMana(60)
    , m_baseRange(1)
    , m_bonusMaxHp(0)
    , m_bonusAtk(0)
    , m_attackSpeedMultiplier(1.0)
    , m_skillDamageMultiplier(1.0)
    , m_doubleAttackChance(0.0)
    , m_equipment(EquipType::None)
    , m_position(0, 0)
    , m_state(UnitState::Idle)
    , m_targetId(-1)
    , m_attackTimer(0)
    , m_moveTimer(0)
    , m_castTimer(0)
    , m_stunTimer(0)
{
}

void Unit::setHp(int hp)
{
    m_hp = qMax(0, qMin(hp, m_maxHp));
    if (m_hp <= 0) {
        m_state = UnitState::Dead;
    }
}

void Unit::setMaxHp(int maxHp)
{
    m_maxHp = maxHp;
    m_hp = qMin(m_hp, m_maxHp);
}

void Unit::setMana(int mana)
{
    m_mana = qMax(0, qMin(mana, m_maxMana));
}

void Unit::setMaxMana(int maxMana)
{
    m_maxMana = maxMana;
    m_mana = qMin(m_mana, m_maxMana);
}

void Unit::setStarLevel(int level)
{
    m_starLevel = qBound(1, level, 3);
}

int Unit::attackIntervalFrames() const
{
    const int interval = qMax(12, static_cast<int>(CombatConst::kAttackInterval / m_attackSpeedMultiplier));
    return interval;
}

QString Unit::displayName() const
{
    return QStringLiteral("%1 ★%2").arg(m_name).arg(m_starLevel);
}

QString Unit::stateText() const
{
    switch (m_state) {
    case UnitState::Idle: return QStringLiteral("空闲");
    case UnitState::Moving: return QStringLiteral("移动");
    case UnitState::Attacking: return QStringLiteral("攻击");
    case UnitState::Casting: return QStringLiteral("施法");
    case UnitState::Dead: return QStringLiteral("死亡");
    }
    return QString();
}

QString Unit::equipmentText() const
{
    if (m_equipment == EquipType::None) {
        return QStringLiteral("无");
    }
    return Equipment::info(m_equipment).name;
}

void Unit::takeDamage(int damage)
{
    if (damage <= 0 || isDead()) {
        return;
    }
    setHp(m_hp - damage);
}

void Unit::heal(int amount)
{
    if (amount <= 0 || isDead()) {
        return;
    }
    setHp(m_hp + amount);
}

void Unit::gainMana(int amount)
{
    if (amount <= 0 || isDead()) {
        return;
    }
    setMana(m_mana + amount);
}

void Unit::resetCombatState()
{
    if (isDead()) {
        return;
    }
    m_state = UnitState::Idle;
    m_targetId = -1;
    m_attackTimer = 0;
    m_moveTimer = 0;
    m_castTimer = 0;
    m_stunTimer = 0;
    m_path.clear();
}

void Unit::captureBaseStats()
{
    m_baseMaxHp = m_maxHp;
    m_baseAtk = m_atk;
    m_baseMaxMana = m_maxMana;
    m_baseRange = m_range;
}

void Unit::recalculateStats()
{
    const qreal starMult = m_starLevel == 1 ? 1.0 : (m_starLevel == 2 ? 1.8 : 2.5);

    int equipHp = 0;
    int equipAtk = 0;
    m_attackSpeedMultiplier = 1.0;
    int equipManaDelta = 0;

    switch (m_equipment) {
    case EquipType::IronSword:
        equipAtk += 15;
        break;
    case EquipType::ChainMail:
        equipHp += 150;
        break;
    case EquipType::SwiftGloves:
        m_attackSpeedMultiplier = 1.2;
        break;
    case EquipType::ManaCrystal:
        equipManaDelta = -30;
        break;
    default:
        break;
    }

    const int newMaxHp = qMax(1, static_cast<int>(m_baseMaxHp * starMult) + equipHp + m_bonusMaxHp);
    const int newAtk = qMax(1, static_cast<int>(m_baseAtk * starMult) + equipAtk + m_bonusAtk);
    const int newMaxMana = qMax(10, static_cast<int>(m_baseMaxMana * starMult) + equipManaDelta);

    setMaxHp(newMaxHp);
    setAtk(newAtk);
    setMaxMana(newMaxMana);
    setRange(m_baseRange);
}

void Unit::upgradeToTwoStar()
{
    if (m_starLevel >= 2) {
        return;
    }
    m_starLevel = 2;
    recalculateStats();
    setHp(maxHp());
}

bool Unit::canEquipMore() const
{
    return m_equipment == EquipType::None;
}

bool Unit::equipItem(EquipType type)
{
    if (type == EquipType::None || !canEquipMore()) {
        return false;
    }
    m_equipment = type;
    recalculateStats();
    return true;
}

void Unit::clearEquipment()
{
    m_equipment = EquipType::None;
    recalculateStats();
}

void Unit::clearSynergyBonuses()
{
    m_bonusMaxHp = 0;
    m_bonusAtk = 0;
    m_skillDamageMultiplier = 1.0;
    m_doubleAttackChance = 0.0;
}

void Unit::castSkill(Board&, const QList<Unit*>&)
{
    setMana(0);
}
