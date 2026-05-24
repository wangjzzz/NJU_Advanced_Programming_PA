#include "core/synergy.h"
#include "core/bench.h"
#include "core/board.h"
#include "entity/unit.h"

QHash<QString, int> SynergySystem::countTraits(const Board& board, const Bench& bench,
                                               const QList<Unit*>& units) const
{
    QHash<QString, int> counts;
    auto addUnit = [&](Unit* unit) {
        if (!unit || unit->owner() != Controller::PlayerCtrl || !unit->isAlive()) {
            return;
        }
        for (const QString& trait : unit->traits()) {
            counts[trait] += 1;
        }
    };

    for (Unit* unit : units) {
        if (unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        if (board.getUnitAt(unit->position()) == unit) {
            addUnit(unit);
        }
    }
    return counts;
}

SynergyState SynergySystem::compute(const Board& board, const Bench& bench,
                                    const QList<Unit*>& units) const
{
    const QHash<QString, int> counts = countTraits(board, bench, units);
    SynergyState state;

    if (counts.value(QStringLiteral("法师")) >= 3) {
        state.skillDamageMultiplier = 2.0;
    }
    if (counts.value(QStringLiteral("游侠")) >= 2) {
        state.rangerDoubleAttackChance = 0.35;
    }
    return state;
}

void SynergySystem::applyToPlayerUnits(const Board& board, const Bench& bench,
                                       const QList<Unit*>& units) const
{
    const QHash<QString, int> counts = countTraits(board, bench, units);

    for (Unit* unit : units) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        unit->clearSynergyBonuses();
        unit->recalculateStats();
    }

    auto applyAuraHp = [&](const QString& trait, int threshold, int bonusHp) {
        if (counts.value(trait) < threshold) {
            return;
        }
        for (Unit* unit : units) {
            if (unit && unit->owner() == Controller::PlayerCtrl && unit->hasTrait(trait)) {
                unit->addBonusMaxHp(bonusHp);
                unit->recalculateStats();
                unit->setHp(unit->hp() + bonusHp);
            }
        }
    };

    auto applyAuraAtk = [&](const QString& trait, int threshold, int bonusAtk) {
        if (counts.value(trait) < threshold) {
            return;
        }
        for (Unit* unit : units) {
            if (unit && unit->owner() == Controller::PlayerCtrl && unit->hasTrait(trait)) {
                unit->addBonusAtk(bonusAtk);
                unit->recalculateStats();
            }
        }
    };

    if (counts.value(QStringLiteral("战士")) >= 4) {
        applyAuraHp(QStringLiteral("战士"), 4, 70);
        applyAuraAtk(QStringLiteral("战士"), 4, 10);
    } else if (counts.value(QStringLiteral("战士")) >= 2) {
        applyAuraHp(QStringLiteral("战士"), 2, 80);
    }

    applyAuraAtk(QStringLiteral("亡灵"), 2, 15);
    applyAuraAtk(QStringLiteral("亡灵"), 4, 15);

    applyAuraHp(QStringLiteral("辅助"), 2, 50);

    const SynergyState state = compute(board, bench, units);
    for (Unit* unit : units) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        unit->setSkillDamageMultiplier(state.skillDamageMultiplier);
        if (unit->hasTrait(QStringLiteral("游侠"))) {
            unit->setDoubleAttackChance(state.rangerDoubleAttackChance);
        }
    }
}

QString SynergySystem::summary(const Board& board, const Bench& bench,
                               const QList<Unit*>& units) const
{
    const QHash<QString, int> c = countTraits(board, bench, units);
  QStringList lines;
    lines << QStringLiteral("战士[%1] 2:全体战士+80HP 4:+70HP+10ATK").arg(c.value(QStringLiteral("战士")));
    lines << QStringLiteral("亡灵[%1] 2:+15ATK 4:+15ATK").arg(c.value(QStringLiteral("亡灵")));
    lines << QStringLiteral("法师[%1] 3:友军技能伤害×2").arg(c.value(QStringLiteral("法师")));
    lines << QStringLiteral("游侠[%1] 2:35%连击").arg(c.value(QStringLiteral("游侠")));
    lines << QStringLiteral("辅助[%1] 2:全体+50HP").arg(c.value(QStringLiteral("辅助")));
    return lines.join(QStringLiteral(" | "));
}
