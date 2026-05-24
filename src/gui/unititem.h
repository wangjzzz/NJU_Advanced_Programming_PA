#ifndef GUI_UNITITEM_H
#define GUI_UNITITEM_H

#include <QGraphicsObject>
#include <QPixmap>
#include <QPoint>

class Unit;

class UnitItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit UnitItem(Unit* unit, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    Unit* unit() const { return m_unit; }
    int unitId() const;

    void setGridPos(const QPoint& gridPos);
    QPoint gridPos() const { return m_gridPos; }

    void setBenchSlot(int slot);
    int benchSlot() const { return m_benchSlot; }

signals:
    void dragStarted(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void dragMoved(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void dragDropped(int unitId, const QPoint& sourceGrid, const QPointF& scenePos);
    void clicked(int unitId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void ensureSpriteLoaded() const;
    QString spriteRelativePathForUnit() const;
    bool tryLoadFallbackSprite(const QString& root) const;
    void drawStatBars(QPainter* painter) const;

    Unit* m_unit;
    QPoint m_gridPos;
    int m_benchSlot;
    bool m_dragging;
    bool m_pressed;
    QPointF m_pressScenePos;
    mutable QPixmap m_sprite;
    mutable bool m_spriteTried;
};

#endif // GUI_UNITITEM_H
