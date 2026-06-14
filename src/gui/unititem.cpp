#include "gui/unititem.h"
#include "entity/unit.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

UnitItem::UnitItem(Unit* unit, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_unit(unit)
    , m_gridPos(-1, -1)
    , m_benchSlot(-1)
    , m_dragging(false)
    , m_pressed(false)
    , m_spriteTried(false)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}

QRectF UnitItem::boundingRect() const
/*返回单位图元的占用矩形区域*/
{
    return QRectF(-44, -44, 88, 88);
}

void UnitItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
/*绘制单位：精灵图/占位圆→羁绊光环→星级→BOSS标签→受击闪白→SKILL弹字→血条蓝条*/
{
    painter->setRenderHint(QPainter::Antialiasing);
    ensureSpriteLoaded();

    if (!m_sprite.isNull()) {
        painter->drawPixmap(QRectF(-36, -36, 72, 72), m_sprite, m_sprite.rect());
    } else {
        QColor fill(100, 150, 200);
        if (m_unit) {
            if (m_unit->owner() == Controller::EnemyCtrl) {
                fill = QColor(180, 80, 80);
            } else if (m_unit->hasTrait(QStringLiteral("\u6218\u58EB"))) {
                fill = QColor(200, 110, 90);
            } else if (m_unit->hasTrait(QStringLiteral("\u6CD5\u5E08"))) {
                fill = QColor(100, 120, 210);
            } else if (m_unit->hasTrait(QStringLiteral("\u6E38\u4FA0"))) {
                fill = QColor(100, 190, 120);
            } else if (m_unit->hasTrait(QStringLiteral("\u8F85\u52A9"))) {
                fill = QColor(210, 210, 130);
            }
        }

        painter->setPen(QPen(QColor(25, 25, 25), 2));
        painter->setBrush(fill);
        painter->drawEllipse(QRectF(-28, -28, 56, 56));

        if (m_unit) {
            painter->setPen(Qt::white);
            QFont font = painter->font();
            font.setBold(true);
            font.setPointSize(11);
            painter->setFont(font);
            painter->drawText(QRectF(-28, -12, 56, 24), Qt::AlignCenter, m_unit->name().left(1));
        }
    }

    if (m_unit && m_unit->owner() != Controller::EnemyCtrl
        && !m_unit->activeSynergies().isEmpty()) {
        const QColor colors[] = {
            QColor(255, 120, 60, 140),   // 战士-橙
            QColor(150, 80, 180, 140),   // 亡灵-紫
            QColor(80, 140, 240, 140),   // 法师-蓝
            QColor(100, 210, 120, 140),  // 游侠-绿
            QColor(255, 210, 80, 140),   // 辅助-金
        };
        int idx = 0;
        const auto& syns = m_unit->activeSynergies();
        if (syns.contains(QStringLiteral("\u6218\u58EB"))) idx = 0;
        else if (syns.contains(QStringLiteral("\u4EA1\u7075"))) idx = 1;
        else if (syns.contains(QStringLiteral("\u6CD5\u5E08"))) idx = 2;
        else if (syns.contains(QStringLiteral("\u6E38\u4FA0"))) idx = 3;
        else if (syns.contains(QStringLiteral("\u8F85\u52A9"))) idx = 4;

        painter->setPen(QPen(colors[idx], 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QRectF(-24, 12, 48, 16));
    }

    if (m_unit && m_unit->starLevel() > 1) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 215, 0));
        for (int i = 0; i < m_unit->starLevel(); ++i) {
            const qreal x = -16 + i * 12;
            static const QPointF starPts[10] = {
                {0, -4}, {1, -1}, {4, -1}, {2, 1}, {3, 4},
                {0, 2}, {-3, 4}, {-2, 1}, {-4, -1}, {-1, -1}
            };
            QPolygonF star;
            for (const QPointF& pt : starPts) {
                star << QPointF(x + pt.x(), -30 + pt.y());
            }
            painter->drawPolygon(star);
        }
    }

    if (m_unit && m_unit->isBoss()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(220, 40, 40));
        QFont bossFont = painter->font();
        bossFont.setBold(true);
        bossFont.setPointSize(10);
        painter->setFont(bossFont);
        painter->setPen(QColor(255, 255, 200));
        painter->drawText(QRectF(-22, -48, 44, 14), Qt::AlignCenter, QStringLiteral("BOSS"));
    }

    if (m_unit && m_unit->hitFlashTimer() > 0) {
        const qreal alpha = 0.55 * m_unit->hitFlashTimer() / 8.0;
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 255, static_cast<int>(alpha * 255)));
        painter->drawRoundedRect(QRectF(-28, -28, 56, 56), 8, 8);
    }

    if (m_unit && m_unit->skillPopupTimer() > 0
        && m_unit->owner() != Controller::EnemyCtrl) {
        const qreal alpha = qBound(0.0, m_unit->skillPopupTimer() / 50.0, 1.0);
        QFont skillFont = painter->font();
        skillFont.setBold(true);
        skillFont.setPointSize(13);
        painter->setFont(skillFont);
        painter->setPen(QColor(255, 220, 50, static_cast<int>(alpha * 255)));
        painter->drawText(QRectF(-40, -60, 80, 22), Qt::AlignCenter,
                          QStringLiteral("SKILL!"));
    }

    drawStatBars(painter);
}

void UnitItem::drawStatBars(QPainter* painter) const
/*绘制单位底部的血条（绿/红）和蓝条（蓝色）*/
{
    if (!m_unit || m_unit->maxHp() <= 0) {
        return;
    }

    const qreal barW = 52.0;
    const qreal barH = 5.0;
    const qreal hpRatio = qBound(0.0, static_cast<qreal>(m_unit->hp()) / m_unit->maxHp(), 1.0);
    const qreal manaRatio = m_unit->maxMana() > 0
        ? qBound(0.0, static_cast<qreal>(m_unit->mana()) / m_unit->maxMana(), 1.0)
        : 0.0;

    const QRectF hpBg(-barW / 2, 30, barW, barH);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(50, 50, 50));
    painter->drawRoundedRect(hpBg, 2, 2);

    const QColor hpColor = m_unit->owner() == Controller::EnemyCtrl
        ? QColor(220, 70, 70)
        : QColor(80, 200, 100);
    painter->setBrush(hpColor);
    painter->drawRoundedRect(QRectF(-barW / 2, 30, barW * hpRatio, barH), 2, 2);

    const QRectF manaBg(-barW / 2, 37, barW, barH - 1);
    painter->setBrush(QColor(40, 40, 55));
    painter->drawRoundedRect(manaBg, 2, 2);
    painter->setBrush(QColor(80, 140, 240));
    painter->drawRoundedRect(QRectF(-barW / 2, 37, barW * manaRatio, barH - 1), 2, 2);
}

void UnitItem::ensureSpriteLoaded() const
/*懒加载精灵图：按名称映射路径→兜底用craftpix素材→都失败则用占位圆*/
{
    if (m_spriteTried) {
        return;
    }
    m_spriteTried = true;

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString roots[] = {
        QFileInfo(appDir + QStringLiteral("/..")).canonicalFilePath(),
        QFileInfo(appDir + QStringLiteral("/../..")).canonicalFilePath(),
    };

    for (const QString& root : roots) {
        if (root.isEmpty()) {
            continue;
        }
        QPixmap pix;
        if (pix.load(root + QLatin1Char('/') + spriteRelativePathForUnit())) {
            m_sprite = pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return;
        }
        if (tryLoadFallbackSprite(root)) {
            return;
        }
    }
}

bool UnitItem::tryLoadFallbackSprite(const QString& root) const
/*尝试从craftpix素材目录加载精灵图作为兜底*/
{
    if (!m_unit) {
        return false;
    }

    static const QString kReaperBase =
        QStringLiteral("assets/craftpix-reaper-man-chibi-2d-game-sprites/Reaper_Man_%1/PNG/PNG Sequences/Idle/0_Reaper_Man_Idle_000.png");
    static const QString kSatyrBase =
        QStringLiteral("assets/craftpix-satyr-tiny-style-2d-sprites/PNG/Satyr_%1/PNG Sequences/Idle/Satyr_%1_Idle_000.png");

    QString path;
    if (m_unit->owner() == Controller::EnemyCtrl) {
        path = kReaperBase.arg(3);
    } else {
        const QString name = m_unit->name();
        if (name == QStringLiteral("\u6218\u58EB"))       path = kReaperBase.arg(1);
        else if (name == QStringLiteral("\u7267\u5E08"))   path = kReaperBase.arg(2);
        else if (name == QStringLiteral("\u5F13\u624B"))   path = kSatyrBase.arg(QStringLiteral("01")).arg(QStringLiteral("01"));
        else if (name == QStringLiteral("\u6CD5\u5E08"))   path = kSatyrBase.arg(QStringLiteral("02")).arg(QStringLiteral("02"));
        else                                       path = kReaperBase.arg(1);
    }

    QPixmap pix;
    if (pix.load(root + QLatin1Char('/') + path)) {
        m_sprite = pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return true;
    }
    return false;
}

QString UnitItem::spriteRelativePathForUnit() const
/*根据单位名称返回assets/textures下的PNG路径*/
{
    if (!m_unit) {
        return QString();
    }

    QString filename;
    if (m_unit->owner() == Controller::EnemyCtrl) {
        const QString name = m_unit->name();
        if (m_unit->isBoss()) filename = "boss.png";
        else if (name == "\u9AB7\u9AC5") filename = "skeleton.png";
        else if (name == "\u5E7D\u7075") filename = "ghost.png";
        else if (name == "\u6076\u9B54") filename = "demon.png";
    } else {
        const QString name = m_unit->name();
        if (name == QStringLiteral("\u6218\u58EB"))       filename = QStringLiteral("warrior.png");
        else if (name == QStringLiteral("\u7267\u5E08"))   filename = QStringLiteral("healer.png");
        else if (name == QStringLiteral("\u5F13\u624B"))   filename = QStringLiteral("archer.png");
        else if (name == QStringLiteral("\u6CD5\u5E08"))   filename = QStringLiteral("mage.png");
        else                                       filename = QStringLiteral("warrior.png");
    }

    return QStringLiteral("assets/textures/") + filename;
}

int UnitItem::unitId() const
/*返回关联Unit的id，不存在返回-1*/
{
    return m_unit ? m_unit->id() : -1;
}

void UnitItem::setGridPos(const QPoint& gridPos)
/*设置棋盘位置并清除备战区标记*/
{
    m_gridPos = gridPos;
    m_benchSlot = -1;
}

void UnitItem::setBenchSlot(int slot)
/*设置备战区槽位并清除棋盘标记*/
{
    m_benchSlot = slot;
    if (slot >= 0) {
        m_gridPos = QPoint(-1, -1);
    }
}

void UnitItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
/*鼠标按下：右键发射rightClicked出售，左键记录按下状态准备拖拽或点击*/
{
    if (event->button() == Qt::RightButton) {
        event->accept();
        emit rightClicked(unitId());
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    m_pressed = true;
    m_dragging = false;
    m_pressScenePos = event->scenePos();
    event->accept();
}

void UnitItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
/*鼠标移动：超过6像素阈值后开始拖拽并发射dragStarted信号，持续发射dragMoved*/
{
    if (!m_pressed) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    if (!m_dragging) {
        const QPointF delta = event->scenePos() - m_pressScenePos;
        if (delta.manhattanLength() < 6) {
            return;
        }
        m_dragging = true;
        emit dragStarted(unitId(), m_gridPos, event->scenePos());
    }

    emit dragMoved(unitId(), m_gridPos, event->scenePos());
    event->accept();
}

void UnitItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
/*鼠标释放：正在拖拽则发射dragDropped，短按则发射clicked*/
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    if (m_dragging) {
        emit dragDropped(unitId(), m_gridPos, event->scenePos());
    } else if (m_pressed) {
        emit clicked(unitId());
    }

    m_pressed = false;
    m_dragging = false;
    event->accept();
}
