#include "core/synergy.h"
#include "core/bench.h"
#include "core/board.h"
#include "entity/unit.h"

QHash<QString, int> SynergySystem::countTraits(const Board& board, const Bench& bench,
                                               const QList<Unit*>& units) const
/*统计棋盘上所有己方单位的羁绊标签数量*/
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
/*根据羁绊计数计算全局状态效果（机制改变类羁绊）*/
{
    const QHash<QString, int> counts = countTraits(board, bench, units);
    SynergyState state;

    if (counts.value(QStringLiteral("\u6CD5\u5E08")) >= 3) {
        state.skillDamageMultiplier = 2.0;
    }
    if (counts.value(QStringLiteral("\u6E38\u4FA0")) >= 2) {
        state.rangerDoubleAttackChance = 0.35;
    }
    return state;
}

void SynergySystem::applyToPlayerUnits(const Board& board, const Bench& bench,
                                       const QList<Unit*>& units) const
/*应用羁绊效果到所有己方单位：重置→计算光环类→计算机制类→标记激活羁绊用于光环显示*/
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

    if (counts.value(QStringLiteral("\u6218\u58EB")) >= 4) {
        applyAuraHp(QStringLiteral("\u6218\u58EB"), 4, 70);
        applyAuraAtk(QStringLiteral("\u6218\u58EB"), 4, 10);
    } else if (counts.value(QStringLiteral("\u6218\u58EB")) >= 2) {
        applyAuraHp(QStringLiteral("\u6218\u58EB"), 2, 80);
    }

    applyAuraAtk(QStringLiteral("\u4EA1\u7075"), 2, 15);
    applyAuraAtk(QStringLiteral("\u4EA1\u7075"), 4, 15);

    applyAuraHp(QStringLiteral("\u8F85\u52A9"), 2, 50);

    const SynergyState state = compute(board, bench, units);
    for (Unit* unit : units) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        unit->setSkillDamageMultiplier(state.skillDamageMultiplier);
        if (unit->hasTrait(QStringLiteral("\u6E38\u4FA0"))) {
            unit->setDoubleAttackChance(state.rangerDoubleAttackChance);
        }
    }

    QSet<QString> activeSet;
    if (counts.value(QStringLiteral("\u6218\u58EB")) >= 2) activeSet.insert(QStringLiteral("\u6218\u58EB"));
    if (counts.value(QStringLiteral("\u4EA1\u7075")) >= 2) activeSet.insert(QStringLiteral("\u4EA1\u7075"));
    if (counts.value(QStringLiteral("\u6CD5\u5E08")) >= 3) activeSet.insert(QStringLiteral("\u6CD5\u5E08"));
    if (counts.value(QStringLiteral("\u6E38\u4FA0")) >= 2) activeSet.insert(QStringLiteral("\u6E38\u4FA0"));
    if (counts.value(QStringLiteral("\u8F85\u52A9")) >= 2) activeSet.insert(QStringLiteral("\u8F85\u52A9"));

    for (Unit* unit : units) {
        if (!unit || unit->owner() != Controller::PlayerCtrl) {
            continue;
        }
        if (board.getUnitAt(unit->position()) != unit) {
            unit->clearActiveSynergies();
            continue;
        }
        QSet<QString> unitActive;
        for (const QString& trait : unit->traits()) {
            if (activeSet.contains(trait)) {
                unitActive.insert(trait);
            }
        }
        unit->setActiveSynergies(unitActive);
    }
}

QString SynergySystem::summary(const Board& board, const Bench& bench,
                               const QList<Unit*>& units) const
/*生成羁绊状态摘要文本，用于界面显示*/
{
    const QHash<QString, int> c = countTraits(board, bench, units);
  QStringList lines;
    lines << QStringLiteral("\u6218\u58EB[%1] 2:\u5168\u4F53\u6218\u58EB+80HP 4:+70HP+10ATK").arg(c.value(QStringLiteral("\u6218\u58EB")));
    lines << QStringLiteral("\u4EA1\u7075[%1] 2:+15ATK 4:+15ATK").arg(c.value(QStringLiteral("\u4EA1\u7075")));
    lines << QStringLiteral("\u6CD5\u5E08[%1] 3:\u53CB\u519B\u6280\u80FD\u4F24\u5BB3\u00D72").arg(c.value(QStringLiteral("\u6CD5\u5E08")));
    lines << QStringLiteral("\u6E38\u4FA0[%1] 2:35%\u8FDE\u51FB").arg(c.value(QStringLiteral("\u6E38\u4FA0")));
    lines << QStringLiteral("\u8F85\u52A9[%1] 2:\u5168\u4F53+50HP").arg(c.value(QStringLiteral("\u8F85\u52A9")));
    return lines.join(QStringLiteral(" | "));
}
