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
/*返回图元占用的矩形区域*/
{
    return m_rect;
}

void BenchSlotItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
/*绘制备战区槽位：圆角矩形 + 槽位编号文字，可落子时绿色、悬停时亮色*/
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
    painter->drawText(m_rect, Qt::AlignCenter, QStringLiteral("\u5907%1").arg(m_slot + 1));
}

void BenchSlotItem::setHoverActive(bool active)
/*设置拖拽悬停高亮状态并触发重绘*/
{
    if (m_hoverActive != active) {
        m_hoverActive = active;
        update();
    }
}

void BenchSlotItem::setDropActive(bool active)
/*设置可落子高亮状态（绿色标记）并触发重绘*/
{
    if (m_dropActive != active) {
        m_dropActive = active;
        update();
    }
}

void BenchSlotItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
/*鼠标进入槽位时记录悬停状态并重绘*/
{
    Q_UNUSED(event);
    m_pointerHover = true;
    update();
}

void BenchSlotItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
/*鼠标离开槽位时清除悬停状态并重绘*/
{
    Q_UNUSED(event);
    m_pointerHover = false;
    update();
}
