#include "gui/overlayitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

OverlayItem::OverlayItem(const QRectF& rect, const QString& title,
                         const QString& subtext, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_rect(rect)
    , m_title(title)
    , m_subtext(subtext)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setZValue(100.0);
}

QRectF OverlayItem::boundingRect() const
/*返回浮层占用的矩形区域*/
{
    return m_rect;
}

void OverlayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
                        QWidget*)
/*绘制战斗结果浮层：半透明黑底→深色卡片→标题→副文本→底部"点击继续"提示*/
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 180));
    painter->drawRoundedRect(m_rect, 12, 12);

    painter->setPen(QPen(QColor(255, 220, 100), 3));
    painter->setBrush(QColor(40, 40, 45, 220));
    const QRectF inner = m_rect.adjusted(20, 20, -20, -20);
    painter->drawRoundedRect(inner, 8, 8);

    QFont titleFont = painter->font();
    titleFont.setBold(true);
    titleFont.setPointSize(28);
    painter->setFont(titleFont);
    painter->setPen(QColor(255, 220, 100));
    painter->drawText(QRectF(inner.x(), inner.y() + 20,
                              inner.width(), 50),
                      Qt::AlignHCenter, m_title);

    QFont subFont = painter->font();
    subFont.setBold(false);
    subFont.setPointSize(14);
    painter->setFont(subFont);
    painter->setPen(QColor(220, 220, 220));
    painter->drawText(QRectF(inner.x(), inner.y() + 75,
                              inner.width(), 40),
                      Qt::AlignHCenter, m_subtext);

    QFont hintFont = painter->font();
    hintFont.setPointSize(11);
    painter->setFont(hintFont);
    painter->setPen(QColor(160, 160, 160));
    painter->drawText(QRectF(inner.x(), inner.y() + inner.height() - 35,
                              inner.width(), 25),
                      Qt::AlignHCenter,
                          QStringLiteral("\u70B9\u51FB\u6B64\u5904\u7EE7\u7EED"));
}

void OverlayItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
/*点击任意位置发射dismissed信号，由Game移除浮层*/
{
    if (event->button() == Qt::LeftButton) {
        event->accept();
        emit dismissed();
    }
}
