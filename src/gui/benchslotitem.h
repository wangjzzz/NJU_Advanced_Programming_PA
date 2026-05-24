#ifndef GUI_BENCHSLOTITEM_H
#define GUI_BENCHSLOTITEM_H

#include <QGraphicsObject>
#include <QRectF>

class QGraphicsSceneHoverEvent;

class BenchSlotItem : public QGraphicsObject
{
    Q_OBJECT

public:
    BenchSlotItem(int slot, const QRectF& rect, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    int slot() const { return m_slot; }

    void setHoverActive(bool active);
    void setDropActive(bool active);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    int m_slot;
    QRectF m_rect;
    bool m_hoverActive;
    bool m_dropActive;
    bool m_pointerHover;
};

#endif // GUI_BENCHSLOTITEM_H
