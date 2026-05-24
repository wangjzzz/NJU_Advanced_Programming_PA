#include "gui/benchslotitem.h"
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

BenchSlotItem::BenchSlotItem(int slot, const QRectF& rect, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_slot(slot)
    , m_rect(rect)
    , m_hoverActive(false)
    , m_dropActive(false)
    , m_pointerHover(false)
{
    setAcceptHoverEvents(true);
}

QRectF BenchSlotItem::boundingRect() const
{
    return m_rect;
}

void BenchSlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QColor fill(45, 48, 58);
    QColor border(75, 80, 95);

    if (m_dropActive) {
        fill = QColor(90, 150, 100);
        border = QColor(120, 220, 130);
    } else if (m_hoverActive || m_pointerHover) {
        fill = QColor(60, 65, 78);
    }

    painter->setPen(QPen(border, 2));
    painter->setBrush(fill);
    painter->drawRoundedRect(m_rect, 6, 6);

    painter->setPen(QColor(180, 185, 200));
    QFont font = painter->font();
    font.setPointSize(9);
    painter->setFont(font);
    painter->drawText(m_rect, Qt::AlignCenter, QStringLiteral("备%1").arg(m_slot + 1));
}

void BenchSlotItem::setHoverActive(bool active)
{
    if (m_hoverActive != active) {
        m_hoverActive = active;
        update();
    }
}

void BenchSlotItem::setDropActive(bool active)
{
    if (m_dropActive != active) {
        m_dropActive = active;
        update();
    }
}

void BenchSlotItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    m_pointerHover = true;
    update();
}

void BenchSlotItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    m_pointerHover = false;
    update();
}
