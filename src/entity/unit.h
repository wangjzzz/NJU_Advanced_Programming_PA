#ifndef UNIT_H
#define UNIT_H

#include "core/equipment.h"
#include "entity/combattypes.h"
#include <QList>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QVector>

class Board;

enum class Controller {
    PlayerCtrl,
    EnemyCtrl
};

class Unit
{
public:
    explicit Unit(const QString& name = QStringLiteral("Unit"),
                  Controller owner = Controller::PlayerCtrl);
    virtual ~Unit() = default;

    int id() const { return m_id; }
    QString name() const { return m_name; }
    Controller owner() const { return m_owner; }

    int hp() const { return m_hp; }
    int maxHp() const { return m_maxHp; }
    int atk() const { return m_atk; }
    int range() const { return m_range; }
    int mana() const { return m_mana; }
    int maxMana() const { return m_maxMana; }
    int starLevel() const { return m_starLevel; }

    int baseMaxHp() const { return m_baseMaxHp; }
    int baseAtk() const { return m_baseAtk; }
    EquipType equipment() const { return m_equipment; }

    QPoint position() const { return m_position; }
    QSet<QString> traits() const { return m_traits; }

    bool isBoss() const { return m_isBoss; }
    void setIsBoss(bool v) { m_isBoss = v; }

    UnitState state() const { return m_state; }
    int targetId() const { return m_targetId; }
    int stunTimer() const { return m_stunTimer; }
    int attackTimer() const { return m_attackTimer; }
    int moveTimer() const { return m_moveTimer; }
    int castTimer() const { return m_castTimer; }
    int hitFlashTimer() const { return m_hitFlashTimer; }
    void setHitFlashTimer(int frames) { m_hitFlashTimer = frames; }
    int skillPopupTimer() const { return m_skillPopupTimer; }
    void setSkillPopupTimer(int frames) { m_skillPopupTimer = frames; }

    qreal skillDamageMultiplier() const { return m_skillDamageMultiplier; }
    qreal doubleAttackChance() const { return m_doubleAttackChance; }
    int attackIntervalFrames() const;

    void setName(const QString& name) { m_name = name; }
    void setOwner(Controller owner) { m_owner = owner; }
    void setPosition(const QPoint& pos) { m_position = pos; }

    void setHp(int hp);
    void setMaxHp(int maxHp);
    void setAtk(int atk) { m_atk = atk; }
    void setRange(int range) { m_range = range; m_baseRange = range; }
    void setMana(int mana);
    void setMaxMana(int maxMana);
    void setStarLevel(int level);

    void setState(UnitState state) { m_state = state; }
    void setTargetId(int id) { m_targetId = id; }
    void setStunTimer(int frames) { m_stunTimer = frames; }
    void setAttackTimer(int frames) { m_attackTimer = frames; }
    void setMoveTimer(int frames) { m_moveTimer = frames; }
    void setCastTimer(int frames) { m_castTimer = frames; }

    void addTrait(const QString& trait) { m_traits.insert(trait); }
    bool hasTrait(const QString& trait) const { return m_traits.contains(trait); }

    bool isDead() const { return m_hp <= 0 || m_state == UnitState::Dead; }
    bool isAlive() const { return !isDead(); }

    QString displayName() const;
    QString stateText() const;
    QString equipmentText() const;

    void takeDamage(int damage);
    void heal(int amount);
    void gainMana(int amount);

    void resetCombatState();
    void clearPath() { m_path.clear(); }
    const QVector<QPoint>& path() const { return m_path; }
    void setPath(const QVector<QPoint>& path) { m_path = path; }

    void captureBaseStats();
    void recalculateStats();
    void upgradeToTwoStar();
    bool canEquipMore() const;
    bool equipItem(EquipType type);
    void clearEquipment();

    void clearSynergyBonuses();
    void addBonusMaxHp(int v) { m_bonusMaxHp += v; }
    void addBonusAtk(int v) { m_bonusAtk += v; }
    void setSkillDamageMultiplier(qreal m) { m_skillDamageMultiplier = m; }
    void setDoubleAttackChance(qreal c) { m_doubleAttackChance = c; }
    const QSet<QString>& activeSynergies() const { return m_activeSynergies; }
    void setActiveSynergies(const QSet<QString>& s) { m_activeSynergies = s; }
    void clearActiveSynergies() { m_activeSynergies.clear(); }

    virtual QString heroType() const { return QStringLiteral("基础"); }
    virtual void castSkill(Board& board, const QList<Unit*>& allUnits);

protected:
    static int s_nextId;

    int m_id;
    QString m_name;
    Controller m_owner;

    int m_hp;
    int m_maxHp;
    int m_atk;
    int m_range;
    int m_mana;
    int m_maxMana;
    int m_starLevel;

    int m_baseMaxHp;
    int m_baseAtk;
    int m_baseMaxMana;
    int m_baseRange;

    int m_bonusMaxHp;
    int m_bonusAtk;
    qreal m_attackSpeedMultiplier;
    qreal m_skillDamageMultiplier;
    qreal m_doubleAttackChance;

    EquipType m_equipment;

    QPoint m_position;
    QSet<QString> m_traits;
    QSet<QString> m_activeSynergies;

    UnitState m_state;
    int m_targetId;
    int m_attackTimer;
    int m_moveTimer;
    int m_castTimer;
    int m_stunTimer;
    int m_hitFlashTimer;
    int m_skillPopupTimer;
    bool m_isBoss = false;
    QVector<QPoint> m_path;
};

#endif // UNIT_H
