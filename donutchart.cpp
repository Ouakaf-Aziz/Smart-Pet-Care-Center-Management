#include "donutchart.h"

#include <QPainter>
#include <QPen>
#include <QFont>

DonutChart::DonutChart(QWidget *parent)
    : QWidget(parent)
{
    m_colors = { QColor("#0F766E"), QColor("#F28C28"), QColor("#3B82F6"),
                 QColor("#C0392B"), QColor("#8B5CF6"), QColor("#EAB308"),
                 QColor("#14B8A6") };
}

void DonutChart::setData(const QMap<QString, int> &data)
{
    m_data = data;
    update();
}

void DonutChart::setUnite(const QString &unite)
{
    m_unite = unite;
    update();
}

void DonutChart::clear()
{
    m_data.clear();
    update();
}

QSize DonutChart::minimumSizeHint() const
{
    return QSize(260, 120);
}

void DonutChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int total = 0;
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it)
        total += it.value();

    // --- Cercle ---
    const int side = qMin(height(), width());
    const qreal thickness = side * 0.16;
    const QRectF ring(thickness / 2 + 2, thickness / 2 + 2,
                      side - thickness - 4, side - thickness - 4);

    if (total == 0) {
        p.setPen(QPen(QColor("#E3EAE9"), thickness, Qt::SolidLine, Qt::FlatCap));
        p.drawEllipse(ring);
    } else {
        int start = 90 * 16;               // départ en haut, sens horaire
        int i = 0;
        for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it, ++i) {
            if (it.value() <= 0)
                continue;
            const int span = -qRound(360.0 * 16 * it.value() / total);
            p.setPen(QPen(m_colors[i % m_colors.size()], thickness,
                          Qt::SolidLine, Qt::FlatCap));
            p.drawArc(ring, start, span);
            start += span;
        }
    }

    // --- Total au centre ---
    QFont f = font();
    f.setPointSize(18);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor("#0B4F4A"));
    p.drawText(QRectF(0, side * 0.28, side, side * 0.25), Qt::AlignCenter,
               QString::number(total));

    f.setPointSize(8);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor("#6B7A78"));
    p.drawText(QRectF(0, side * 0.52, side, side * 0.15), Qt::AlignCenter,
               m_unite);

    // --- Légende ---
    f.setPointSize(9);
    p.setFont(f);

    const int legendX = side + 24;
    const int rowH = 20;
    const int rows = qMax(1, m_data.size());
    int y = (height() - rows * rowH) / 2;

    if (total == 0) {
        p.setPen(QColor("#6B7A78"));
        p.drawText(QRectF(legendX, y, width() - legendX, rowH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("Aucune donnée"));
        return;
    }

    int i = 0;
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it, ++i, y += rowH) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_colors[i % m_colors.size()]);
        p.drawRoundedRect(QRectF(legendX, y + (rowH - 10) / 2.0, 10, 10), 2, 2);

        const int pct = qRound(100.0 * it.value() / total);
        p.setPen(QColor("#1F2A37"));
        p.drawText(QRectF(legendX + 18, y, width() - legendX - 18, rowH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString("%1 : %2 (%3%)").arg(it.key()).arg(it.value()).arg(pct));
    }
}
