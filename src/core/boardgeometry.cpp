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
{
    const qreal colSpacing = m_radius * qSqrt(3.0);
    const qreal xOffset = (row % 2 == 0) ? colSpacing * 0.5 : 0.0;
    return QPointF(xOffset + col * colSpacing, row * m_rowSpacing);
}

QPoint BoardGeometry::worldToGrid(const QPointF& world) const
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
{
    return QPointF(slot * m_benchSpacing + m_benchSpacing / 2,
                   m_benchY + kBenchSlotSize / 2);
}

bool BoardGeometry::isBenchScenePos(const QPointF& scenePos) const
{
    return scenePos.y() >= m_benchY
           && scenePos.y() < m_benchY + kBenchSlotSize;
}

int BoardGeometry::benchSlotAt(const QPointF& scenePos) const
{
    if (!isBenchScenePos(scenePos)) {
        return -1;
    }
    return qBound(0, static_cast<int>(scenePos.x() / m_benchSpacing), 7);
}

QRectF BoardGeometry::sellZoneRect() const
{
    const qreal left = m_benchSpacing * 8;
    return QRectF(left, m_benchY, kSellZoneWidth, kBenchSlotSize);
}

bool BoardGeometry::isSellScenePos(const QPointF& scenePos) const
{
    const QRectF r = sellZoneRect();
    return r.contains(scenePos);
}
