#ifndef SYNERGY_H
#define SYNERGY_H

#include <QList>
#include <QString>
#include <QHash>

class Unit;
class Board;
class Bench;

struct SynergyState
{
    qreal skillDamageMultiplier = 1.0;
    qreal rangerDoubleAttackChance = 0.0;
};

class SynergySystem
{
public:
    QString summary(const Board& board, const Bench& bench, const QList<Unit*>& units) const;
    SynergyState compute(const Board& board, const Bench& bench, const QList<Unit*>& units) const;
    void applyToPlayerUnits(const Board& board, const Bench& bench, const QList<Unit*>& units) const;

private:
    QHash<QString, int> countTraits(const Board& board, const Bench& bench, const QList<Unit*>& units) const;
};

#endif
