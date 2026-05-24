#ifndef GAMESAVE_H
#define GAMESAVE_H

#include <QString>

class Game;

class GameSave
{
public:
    static bool save(const Game& game, const QString& filePath, QString* error);
    static bool load(Game& game, const QString& filePath, QString* error);
};

#endif
