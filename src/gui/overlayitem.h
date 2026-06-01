#ifndef GUI_OVERLAYITEM_H
#define GUI_OVERLAYITEM_H

#include <QGraphicsObject>
#include <QRectF>
#include <QString>
#include <QColor>

class OverlayItem : public QGraphicsObject
{
    Q_OBJECT

public:
    OverlayItem(const QRectF& rect, const QString& title, const QString& subtext,
                QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

signals:
    void dismissed();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QRectF m_rect;
    QString m_title;
    QString m_subtext;
};

#endif
