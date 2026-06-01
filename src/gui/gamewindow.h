#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <array>

class Game;
class QLabel;
class QGraphicsView;
class QPushButton;
class QVBoxLayout;

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);
    ~GameWindow() override;

private slots:
    void onResetClicked();
    void onStartCombatClicked();
    void onRefreshShopClicked();
    void onUpgradePopClicked();
    void onSaveClicked();
    void onLoadClicked();
    void onShopSlotClicked();
    void onEquipSlotClicked();
    void refreshPanels();

private:
    void setupUI();
    void updateButtons();
    void refreshShopButtons();

    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QGraphicsView* m_view;
    QLabel* m_hpLabel;
    QLabel* m_goldLabel;
    QLabel* m_roundLabel;
    QLabel* m_popLabel;
    QLabel* m_phaseLabel;
    QLabel* m_synergyLabel;
    QLabel* m_unitInfoLabel;
    QPushButton* m_resetButton;
    QPushButton* m_startCombatButton;
    QPushButton* m_refreshShopButton;
    QPushButton* m_upgradePopButton;
    QPushButton* m_saveButton;
    QPushButton* m_loadButton;
    std::array<QPushButton*, 5> m_shopButtons;
    std::array<QPushButton*, 6> m_equipButtons;
    QLabel* m_equipDropLabel;
    Game* m_game;
};

#endif // GAMEWINDOW_H
