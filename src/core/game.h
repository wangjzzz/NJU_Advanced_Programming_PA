#ifndef CORE_GAME_H
#define CORE_GAME_H

#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QTimer>
#include <unordered_map>
#include <vector>
#include "board.h"
#include "boardgeometry.h"
#include "combat_system.h"
#include "dragdrophandler.h"
#include "entity/combattypes.h"
#include "equipbench.h"
#include "equipment.h"
#include "player.h"
#include "shop.h"
#include "synergy.h"

class Unit;
class QGraphicsScene;
class QGraphicsRectItem;
class QGraphicsLineItem;
class GridItem;
class UnitItem;
class BenchSlotItem;

class Game : public QObject
{
    Q_OBJECT

public:
    explicit Game(QObject* parent = nullptr);
    ~Game() override;

    void initialize();
    void reset();
    void startCombat();

    QGraphicsScene* scene() const { return m_scene; }
    Player* player() { return &m_player; }
    const Player* player() const { return &m_player; }
    Board* board() { return &m_board; }
    const Board* board() const { return &m_board; }
    const Shop& shop() const { return m_shop; }
    const EquipBench& equipBench() const { return m_equipBench; }
    const BoardGeometry& geometry() const { return m_geometry; }

    GamePhase phase() const { return m_phase; }
    GameResult result() const { return m_result; }
    QString phaseMessage() const { return m_phaseMessage; }
    QString synergySummary() const;

    Unit* selectedUnit() const;
    const QList<Unit*>& allUnits() const { return m_allUnits; }
    bool canDragUnits() const { return m_phase == GamePhase::Prep && m_result == GameResult::Playing; }
    int playerUnitsOnBoard() const { return countPlayerUnitsOnBoard(); }

    bool buyFromShop(int slot);
    bool refreshShop();
    bool upgradePopulation();
    bool equipFromBench(int equipSlot, int unitId);
    int selectedEquipSlot() const { return m_selectedEquipSlot; }
    void setSelectedEquipSlot(int slot) { m_selectedEquipSlot = slot; }

    bool saveToFile(const QString& path, QString* errorMessage) const;
    bool loadFromFile(const QString& path, QString* errorMessage);

    void handleDragStarted(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void handleDragMoved(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void handleDropOnBoard(int unitId, const QPoint& source, const QPointF& scenePos);
    void handleUnitClicked(int unitId);
    void sellUnit(int unitId);

signals:
    void stateChanged();

private slots:
    void onCombatTick();

private:
    void clearEnemyUnits();
    void spawnEnemiesForRound(int round);
    void setupHeroStats(Unit* unit);
    Unit* registerUnit(Unit* unit);
    Unit* findUnitById(int unitId) const;
    UnitItem* findUnitItem(int unitId) const;

    void beginPrepPhase();
    void beginResolvePhase(bool playerWon);
    void prepareUnitsForCombat();
    void cleanupAfterCombat();
    void snapshotPlayerDeployment();
    void restorePlayerDeployment();
    QPoint findFirstEmptyPlayerCell() const;

    void tryAutoMerge(Unit* acquired);
    void tryDropEquipment(bool playerWon);
    QJsonObject toJson() const;

    int countPlayerUnitsOnBoard() const;
    bool hasEnemiesOnBoard() const;

    void buildScene();
    void ensureUnitItems();
    void syncFromBoard();
    void syncFromBench();
    void refreshUnitVisuals();
    void drawAttackLines();
    void clearAttackLines();

    Board m_board;
    Player m_player;
    Shop m_shop;
    EquipBench m_equipBench;
    SynergySystem m_synergy;
    CombatSystem m_combat;
    BoardGeometry m_geometry;
    DragDropHandler m_dragDrop;
    QList<Unit*> m_allUnits;

    QGraphicsScene* m_scene;
    QGraphicsRectItem* m_sellZoneItem;
    QTimer* m_combatTimer;
    std::vector<GridItem*> m_gridItems;
    std::vector<BenchSlotItem*> m_benchItems;
    std::vector<UnitItem*> m_unitItems;
    std::unordered_map<int, UnitItem*> m_unitItemById;
    std::vector<QGraphicsLineItem*> m_attackLineItems;

    GamePhase m_phase;
    GameResult m_result;
    QString m_phaseMessage;
    bool m_lastCombatWon;

    int m_selectedUnitId;
    int m_selectedEquipSlot;

    QHash<int, QPoint> m_preCombatBoardPositions;
    QHash<int, int> m_preCombatBenchSlots;
};

#endif // CORE_GAME_H
