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
    , m_hitFlashTimer(0)
    , m_skillPopupTimer(0)
{
}

void Unit::setHp(int hp)
/*设置生命值，钳位在[0, maxHp]，归零则标记死亡*/
{
    m_hp = qMax(0, qMin(hp, m_maxHp));
    if (m_hp <= 0) {
        m_state = UnitState::Dead;
    }
}

void Unit::setMaxHp(int maxHp)
/*设置最大生命值并修正当前HP不超上限*/
{
    m_maxHp = maxHp;
    m_hp = qMin(m_hp, m_maxHp);
}

void Unit::setMana(int mana)
/*设置法力值，钳位在[0, maxMana]*/
{
    m_mana = qMax(0, qMin(mana, m_maxMana));
}

void Unit::setMaxMana(int maxMana)
/*设置最大法力值并修正当前法力不超上限*/
{
    m_maxMana = maxMana;
    m_mana = qMin(m_mana, m_maxMana);
}

void Unit::setStarLevel(int level)
/*设置星阶，钳位在[1,3]*/
{
    m_starLevel = qBound(1, level, 3);
}

int Unit::attackIntervalFrames() const
/*根据攻速倍率计算实际攻击间隔帧数，最低12帧*/
{
    const int interval = qMax(12, static_cast<int>(CombatConst::kAttackInterval / m_attackSpeedMultiplier));
    return interval;
}

QString Unit::displayName() const
/*返回带星级的显示名称，如"战士 ★2"*/
{
    return QStringLiteral("%1 ★%2").arg(m_name).arg(m_starLevel);
}

QString Unit::stateText() const
/*返回当前状态的中文描述*/
{
    switch (m_state) {
    case UnitState::Idle: return QStringLiteral("\u7A7A\u95F2");
    case UnitState::Moving: return QStringLiteral("\u79FB\u52A8");
    case UnitState::Attacking: return QStringLiteral("\u653B\u51FB");
    case UnitState::Casting: return QStringLiteral("\u65BD\u6CD5");
    case UnitState::Dead: return QStringLiteral("\u6B7B\u4EA1");
    }
    return QString();
}

QString Unit::equipmentText() const
/*返回穿戴装备的名称，未装备返回"无"*/
{
    if (m_equipment == EquipType::None) {
        return QStringLiteral("\u65E0");
    }
    return Equipment::info(m_equipment).name;
}

void Unit::takeDamage(int damage)
/*受到伤害：扣血并触发受击闪白计时器*/
{
    if (damage <= 0 || isDead()) {
        return;
    }
    setHp(m_hp - damage);
    m_hitFlashTimer = 8;
}

void Unit::heal(int amount)
/*回复生命值，不超过最大HP*/
{
    if (amount <= 0 || isDead()) {
        return;
    }
    setHp(m_hp + amount);
}

void Unit::gainMana(int amount)
/*获取法力值，通常由普攻触发*/
{
    if (amount <= 0 || isDead()) {
        return;
    }
    setMana(m_mana + amount);
}

void Unit::resetCombatState()
/*重置战斗相关状态：空闲、清除目标、清除路径、重置计时器*/
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
    m_hitFlashTimer = 0;
    m_skillPopupTimer = 0;
    m_path.clear();
}

void Unit::captureBaseStats()
/*记录当前属性为基础属性，用于后续星阶/装备/羁绊重算*/
{
    m_baseMaxHp = m_maxHp;
    m_baseAtk = m_atk;
    m_baseMaxMana = m_maxMana;
    m_baseRange = m_range;
}

void Unit::recalculateStats()
/*根据基础属性、星阶倍率、装备加成、羁绊加成重新计算最终属性*/
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
/*升星到2星：设置星阶、重算属性、回满血量*/
{
    if (m_starLevel >= 2) {
        return;
    }
    m_starLevel = 2;
    recalculateStats();
    setHp(maxHp());
}

bool Unit::canEquipMore() const
/*判断是否还能穿戴装备（当前未穿戴）*/
{
    return m_equipment == EquipType::None;
}

bool Unit::equipItem(EquipType type)
/*穿戴装备：更新装备类型并重算属性*/
{
    if (type == EquipType::None || !canEquipMore()) {
        return false;
    }
    m_equipment = type;
    recalculateStats();
    return true;
}

void Unit::clearEquipment()
/*卸下装备并重算属性*/
{
    m_equipment = EquipType::None;
    recalculateStats();
}

void Unit::clearSynergyBonuses()
/*清除所有羁绊加成效果，恢复基础属性*/
{
    m_bonusMaxHp = 0;
    m_bonusAtk = 0;
    m_skillDamageMultiplier = 1.0;
    m_doubleAttackChance = 0.0;
    m_activeSynergies.clear();
}

void Unit::castSkill(Board&, const QList<Unit*>&)
/*技能基类默认实现：仅清空法力值，派生类重写具体效果*/
{
    setMana(0);
}
