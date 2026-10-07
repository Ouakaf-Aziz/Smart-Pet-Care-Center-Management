#include "linechart.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

LineChart::LineChart(QWidget *parent)
    : QWidget(parent)
    , m_couleur("#0F766E")
{
}

void LineChart::setData(const QVector<QPair<QString, double>> &data, const QColor &couleur)
{
    m_data = data;
    m_couleur = couleur;
    update();
}

QSize LineChart::minimumSizeHint() const
{
    return QSize(260, 160);
}

void LineChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont f = font();
    f.setFamily("Arial");
    f.setPointSize(11);
    p.setFont(f);
    const QFontMetrics fm(f);

    double maxVal = 0;
    for (const auto &d : m_data)
        maxVal = std::max(maxVal, d.second);

    if (m_data.isEmpty() || maxVal <= 0) {
        p.setPen(QColor("#1F2A37"));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Aucune donnée"));
        return;
    }

    const int n = m_data.size();
    const int margeHaut = fm.height() + 12;
    const int margeBas = fm.height() + 10;
    const QRectF zone(24, margeHaut, width() - 48, height() - margeHaut - margeBas);
    const qreal pas = zone.width() / n;

    p.setPen(QPen(QColor("#CBD5D3"), 1, Qt::DashLine));
    for (int k = 0; k <= 2; ++k) {
        const qreal y = zone.bottom() - zone.height() * k / 2.0;
        p.drawLine(QPointF(zone.left(), y), QPointF(zone.right(), y));
    }

    QVector<QPointF> pts;
    for (int i = 0; i < n; ++i)
        pts.append(QPointF(zone.left() + pas * (i + 0.5),
                           zone.bottom() - zone.height() * m_data[i].second / maxVal));

    if (n > 1) {
        QPainterPath aire;
        aire.moveTo(pts.first().x(), zone.bottom());
        for (const QPointF &pt : pts)
            aire.lineTo(pt);
        aire.lineTo(pts.last().x(), zone.bottom());
        aire.closeSubpath();
        QColor c = m_couleur;
        c.setAlpha(40);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawPath(aire);
    }

    p.setPen(QPen(m_couleur, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    for (int i = 1; i < n; ++i)
        p.drawLine(pts[i - 1], pts[i]);

    for (int i = 0; i < n; ++i) {
        p.setPen(QPen(m_couleur, 2));
        p.setBrush(QColor("#FFFFFF"));
        p.drawEllipse(pts[i], 5, 5);

        p.setPen(QColor("#1F2A37"));
        p.drawText(QRectF(pts[i].x() - pas / 2, pts[i].y() - fm.height() - 8, pas, fm.height()),
                   Qt::AlignCenter, QString::number(qRound(m_data[i].second)));
        p.drawText(QRectF(zone.left() + pas * i, zone.bottom() + 6, pas, fm.height() + 2),
                   Qt::AlignCenter, m_data[i].first);
    }
}
