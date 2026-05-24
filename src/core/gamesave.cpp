#include "core/gamesave.h"
#include "core/game.h"

bool GameSave::save(const Game& game, const QString& filePath, QString* error)
{
    return game.saveToFile(filePath, error);
}

bool GameSave::load(Game& game, const QString& filePath, QString* error)
{
    return game.loadFromFile(filePath, error);
}
