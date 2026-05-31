#ifndef CORE_BOARDGEOMETRY_H
#define CORE_BOARDGEOMETRY_H

#include <QPoint>
#include <QPointF>
#include <QPolygonF>

class BoardGeometry
{
public:
    static constexpr qreal kBenchSlotSize = 45.0;
    static constexpr qreal kSellZoneWidth = 60.0;

    BoardGeometry(int rows, int cols,
                  qreal radius, qreal rowSpacing,
                  qreal benchY, qreal benchSpacing);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    qreal benchY() const { return m_benchY; }
    qreal benchSpacing() const { return m_benchSpacing; }

    QPointF gridToWorld(int row, int col) const;
    QPoint worldToGrid(const QPointF& world) const;
    QPolygonF cellHexPolygon(int row, int col) const;
    QPointF benchSlotCenter(int slot) const;

    bool isBenchScenePos(const QPointF& scenePos) const;
    int benchSlotAt(const QPointF& scenePos) const;

    QRectF sellZoneRect() const;
    bool isSellScenePos(const QPointF& scenePos) const;

private:
    int m_rows;
    int m_cols;
    qreal m_radius;
    qreal m_rowSpacing;
    qreal m_benchY;
    qreal m_benchSpacing;
};

#endif // CORE_BOARDGEOMETRY_H
