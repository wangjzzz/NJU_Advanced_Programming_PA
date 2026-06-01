#include "core/boardgeometry.h"
#include <QtMath>

BoardGeometry::BoardGeometry(int rows, int cols,
                             qreal radius, qreal rowSpacing,
                             qreal benchY, qreal benchSpacing)
    : m_rows(rows)
    , m_cols(cols)
    , m_radius(radius)
    , m_rowSpacing(rowSpacing)
    , m_benchY(benchY)
    , m_benchSpacing(benchSpacing)
{
}

QPointF BoardGeometry::gridToWorld(int row, int col) const
/*将棋盘逻辑坐标(row,col)转换为场景世界坐标，偶数行水平偏移半个列间距实现错列排布*/
{
    const qreal colSpacing = m_radius * qSqrt(3.0);
    const qreal xOffset = (row % 2 == 0) ? colSpacing * 0.5 : 0.0;
    return QPointF(xOffset + col * colSpacing, row * m_rowSpacing);
}

QPoint BoardGeometry::worldToGrid(const QPointF& world) const
/*将场景世界坐标反向映射为最近的棋盘格子坐标，遍历所有格子取欧氏距离最近者*/
{
    QPoint best(-1, -1);
    qreal bestDist = 1e18;

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const QPointF center = gridToWorld(row, col);
            const qreal dx = world.x() - center.x();
            const qreal dy = world.y() - center.y();
            const qreal d2 = dx * dx + dy * dy;
            if (d2 < bestDist) {
                bestDist = d2;
                best = QPoint(col, row);
            }
        }
    }
    return best;
}

QPolygonF BoardGeometry::cellHexPolygon(int row, int col) const
/*生成指定格子(row,col)的六边形多边形，用于渲染和点击检测，6个顶点均分360度*/
{
    const QPointF center = gridToWorld(row, col);
    QPolygonF poly;
    poly.reserve(6);

    for (int i = 0; i < 6; ++i) {
        const qreal angleDeg = 60.0 * i - 90.0;
        const qreal angleRad = qDegreesToRadians(angleDeg);
        poly.append(QPointF(
            center.x() + m_radius * qCos(angleRad),
            center.y() + m_radius * qSin(angleRad)));
    }
    return poly;
}

QPointF BoardGeometry::benchSlotCenter(int slot) const
/*返回备战区第slot个槽位的中心世界坐标*/
{
    return QPointF(slot * m_benchSpacing + m_benchSpacing / 2,
                   m_benchY + kBenchSlotSize / 2);
}

bool BoardGeometry::isBenchScenePos(const QPointF& scenePos) const
/*判断场景坐标是否落在备战区区域范围内*/
{
    return scenePos.y() >= m_benchY
           && scenePos.y() < m_benchY + kBenchSlotSize;
}

int BoardGeometry::benchSlotAt(const QPointF& scenePos) const
/*根据场景坐标计算对应的备战区槽位编号，不在备战区返回-1*/
{
    if (!isBenchScenePos(scenePos)) {
        return -1;
    }
    return qBound(0, static_cast<int>(scenePos.x() / m_benchSpacing), 7);
}

QRectF BoardGeometry::sellZoneRect() const
/*返回出售区的矩形区域，位于备战区右侧*/
{
    const qreal left = m_benchSpacing * 8;
    return QRectF(left, m_benchY, kSellZoneWidth, kBenchSlotSize);
}

bool BoardGeometry::isSellScenePos(const QPointF& scenePos) const
/*判断场景坐标是否落在出售区范围内*/
{
    const QRectF r = sellZoneRect();
    return r.contains(scenePos);
}
