#include "barchart.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include <algorithm>

BarChart::BarChart(QWidget *parent)
    : QWidget(parent)
    , m_couleur("#0F766E")
{
}

void BarChart::setData(const QVector<QPair<QString, double>> &data,
                       Orientation orientation,
                       const QString &suffixeValeur,
                       const QColor &couleur)
{
    m_data = data;
    m_orientation = orientation;
    m_suffixe = suffixeValeur;
    m_couleur = couleur;
    update();
}

QSize BarChart::minimumSizeHint() const
{
    return QSize(260, 160);
}

QString BarChart::formater(double v) const
{
    const QString txt = (qAbs(v - qRound(v)) < 0.05) ? QString::number(qRound(v))
                                                      : QString::number(v, 'f', 1);
    return m_suffixe.isEmpty() ? txt : txt + " " + m_suffixe;
}

void BarChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont f = font();
    f.setPointSize(9);
    p.setFont(f);
    const QFontMetrics fm(f);

    double maxVal = 0;
    for (const auto &d : m_data)
        maxVal = std::max(maxVal, d.second);

    if (m_data.isEmpty() || maxVal <= 0) {
        p.setPen(QColor("#6B7A78"));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Aucune donnée"));
        return;
    }

    const int n = m_data.size();

    if (m_orientation == Vertical) {
        const int margeHaut = fm.height() + 10;
        const int margeBas = fm.height() + 10;
        const QRectF zone(8, margeHaut, width() - 16, height() - margeHaut - margeBas);
        const qreal pas = zone.width() / n;
        const qreal largeur = qMin<qreal>(pas * 0.6, 70);

        // ligne de base
        p.setPen(QPen(QColor("#CBD5D3"), 1));
        p.drawLine(QPointF(zone.left(), zone.bottom()), QPointF(zone.right(), zone.bottom()));

        for (int i = 0; i < n; ++i) {
            const qreal hauteur = zone.height() * m_data[i].second / maxVal;
            const qreal x = zone.left() + pas * i + (pas - largeur) / 2;
            const QRectF barre(x, zone.bottom() - hauteur, largeur, hauteur);

            p.setPen(Qt::NoPen);
            p.setBrush(m_couleur);
            if (hauteur > 0)
                p.drawRoundedRect(barre, 4, 4);

            p.setPen(QColor("#1F2A37"));
            p.drawText(QRectF(zone.left() + pas * i, barre.top() - fm.height() - 4, pas, fm.height()),
                       Qt::AlignCenter, formater(m_data[i].second));

            p.setPen(QColor("#6B7A78"));
            p.drawText(QRectF(zone.left() + pas * i, zone.bottom() + 4, pas, fm.height() + 2),
                       Qt::AlignCenter,
                       fm.elidedText(m_data[i].first, Qt::ElideRight, int(pas) - 4));
        }
    } else {
        int largeurLabels = 0;
        for (const auto &d : m_data)
            largeurLabels = std::max(largeurLabels, fm.horizontalAdvance(d.first));
        largeurLabels = qMin(largeurLabels + 10, 160);

        const int margeDroite = fm.horizontalAdvance(formater(maxVal)) + 16;
        const QRectF zone(8 + largeurLabels, 6, width() - 16 - largeurLabels - margeDroite, height() - 12);
        const qreal pas = zone.height() / n;
        const qreal epaisseur = qMin<qreal>(pas * 0.6, 28);

        p.setPen(QPen(QColor("#CBD5D3"), 1));
        p.drawLine(QPointF(zone.left(), zone.top()), QPointF(zone.left(), zone.bottom()));

        for (int i = 0; i < n; ++i) {
            const qreal longueur = zone.width() * m_data[i].second / maxVal;
            const qreal y = zone.top() + pas * i + (pas - epaisseur) / 2;
            const QRectF barre(zone.left(), y, longueur, epaisseur);

            p.setPen(QColor("#6B7A78"));
            p.drawText(QRectF(8, y, largeurLabels - 6, epaisseur),
                       Qt::AlignVCenter | Qt::AlignRight,
                       fm.elidedText(m_data[i].first, Qt::ElideRight, largeurLabels - 6));

            p.setPen(Qt::NoPen);
            p.setBrush(m_couleur);
            if (longueur > 0)
                p.drawRoundedRect(barre, 4, 4);

            p.setPen(QColor("#1F2A37"));
            p.drawText(QRectF(barre.right() + 6, y, margeDroite, epaisseur),
                       Qt::AlignVCenter | Qt::AlignLeft, formater(m_data[i].second));
        }
    }
}
