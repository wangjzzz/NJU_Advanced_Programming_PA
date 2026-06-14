#include "griditem.h"
#include <QGraphicsSceneHoverEvent>
#include <QPainter>

GridItem::GridItem(int row, int col, const QPolygonF& polygon, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_row(row)
    , m_col(col)
    , m_polygon(polygon)
    , m_bounds(polygon.boundingRect())
    , m_baseColor(QColor(60, 60, 80))
    , m_hoverActive(false)
    , m_dropActive(false)
    , m_pointerHover(false)
{
    setAcceptHoverEvents(true);
}

QRectF GridItem::boundingRect() const
/*返回图元占用的矩形区域，四周留2像素余量用于边框渲染*/
{
    return m_bounds.adjusted(-2.0, -2.0, 2.0, 2.0);
}

void GridItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
/*绘制六边形格子：基色填充、可落子时绿色、悬停时亮色、画外框高亮*/
{
    QColor fill = m_baseColor;
    QColor border = QColor(40, 40, 40);

    if (m_dropActive) {
        fill = QColor(110, 170, 110);
        border = QColor(100, 255, 100);
    } else if (m_hoverActive || m_pointerHover) {
        fill = m_baseColor.lighter(120);
    }

    painter->setPen(QPen(border, 2));
    painter->setBrush(fill);
    painter->drawPolygon(m_polygon);

    if (m_hoverActive || m_pointerHover) {
        painter->setPen(QPen(QColor(220, 220, 220), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawPolygon(m_polygon);
    }
}

QPoint GridItem::gridPos() const
/*返回格子的棋盘坐标(col, row)*/
{
    return QPoint(m_col, m_row);
}

void GridItem::setBaseColor(const QColor& color)
/*设置格子基础颜色并触发重绘*/
{
    m_baseColor = color;
    update();
}

void GridItem::setHoverActive(bool active)
/*设置拖拽悬停高亮状态并触发重绘*/
{
    if (m_hoverActive == active) {
        return;
    }
    m_hoverActive = active;
    update();
}

void GridItem::setDropActive(bool active)
/*设置可落子高亮状态（绿色标记）并触发重绘*/
{
    if (m_dropActive == active) {
        return;
    }
    m_dropActive = active;
    update();
}

void GridItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
/*鼠标进入格子时记录悬停状态并重绘*/
{
    Q_UNUSED(event);
    if (!m_pointerHover) {
        m_pointerHover = true;
        update();
    }
}

void GridItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
/*鼠标离开格子时清除悬停状态并重绘*/
{
    Q_UNUSED(event);
    if (m_pointerHover) {
        m_pointerHover = false;
        update();
    }
}
