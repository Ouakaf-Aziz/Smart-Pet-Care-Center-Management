#include "statistiquesdialog.h"

#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QPair>
#include <QVBoxLayout>
#include <QtMath>

namespace {

// ---------- Utilitaires ----------

QColor couleurSerie(int i)
{
    static const char *codes[] = { "#1F4E43", "#E07B20", "#1E9BE9", "#B52A2A",
                                   "#6F7FE0", "#E5A022", "#7A8B99", "#8E44AD" };
    return QColor(codes[i % 8]);
}

QString majuscule(const QString &s)
{
    return s.isEmpty() ? s : s.left(1).toUpper() + s.mid(1);
}

QString formaterNombre(double v)
{
    return QString::number(v, 'g', 4);
}

// Carte blanche arrondie avec titre + sous-titre
QFrame *creerCarte(const QString &titre, const QString &sousTitre, QWidget *contenu)
{
    auto *carte = new QFrame;
    carte->setObjectName("carte");
    carte->setStyleSheet("QFrame#carte { background: white; border: 1px solid #E3E8E6;"
                         "               border-radius: 12px; }");

    auto *lay = new QVBoxLayout(carte);
    lay->setContentsMargins(16, 14, 16, 12);
    lay->setSpacing(2);

    auto *t = new QLabel(titre);
    t->setStyleSheet("font-weight: bold; font-size: 11pt; color: #1F2A37;");
    auto *s = new QLabel(sousTitre);
    s->setStyleSheet("font-size: 8pt; color: #6B7280;");

    lay->addWidget(t);
    lay->addWidget(s);
    lay->addSpacing(6);
    lay->addWidget(contenu, 1);
    return carte;
}

// ---------- Anneau + legende ----------

class DonutWidget : public QWidget
{
public:
    DonutWidget(const QVector<QPair<QString, int>> &data, const QVector<QColor> &couleurs,
                const QString &texteCentre, QWidget *parent = nullptr)
        : QWidget(parent), m_data(data), m_couleurs(couleurs), m_centre(texteCentre)
    {
        setMinimumHeight(170);
    }

protected:
    QColor couleur(int i) const
    {
        return i < m_couleurs.size() ? m_couleurs[i] : couleurSerie(i);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        int total = 0;
        for (const auto &d : m_data) total += d.second;

        const int side = qMax(80, qMin(height() - 6, width() / 2));
        const QRectF r(4, (height() - side) / 2.0, side, side);
        const double ep = side * 0.22;
        const QRectF arc = r.adjusted(ep / 2, ep / 2, -ep / 2, -ep / 2);

        if (total == 0) {
            p.setPen(QPen(QColor("#E5E7EB"), ep, Qt::SolidLine, Qt::FlatCap));
            p.drawEllipse(arc);
            p.setPen(QColor("#6B7280"));
            p.drawText(r, Qt::AlignCenter, QString::fromUtf8("Aucune donnée"));
            return;
        }

        double debut = 90 * 16;
        for (int i = 0; i < m_data.size(); ++i) {
            const double span = -360.0 * 16 * m_data[i].second / total;
            p.setPen(QPen(couleur(i), ep, Qt::SolidLine, Qt::FlatCap));
            p.drawArc(arc, int(debut), int(span));
            debut += span;
        }

        p.setPen(QColor("#1F2A37"));
        QFont grand = font();
        grand.setPointSize(22);
        grand.setBold(true);
        p.setFont(grand);
        p.drawText(QRectF(r.left(), r.center().y() - 26, r.width(), 34), Qt::AlignCenter,
                   QString::number(total));

        QFont petit = font();
        petit.setPointSize(8);
        p.setFont(petit);
        p.setPen(QColor("#6B7280"));
        p.drawText(QRectF(r.left(), r.center().y() + 8, r.width(), 16), Qt::AlignCenter, m_centre);

        QFont fl = font();
        fl.setPointSize(9);
        p.setFont(fl);
        const int ligneH = 24;
        const int x0 = int(r.right()) + 24;
        const int y0 = (height() - ligneH * int(m_data.size())) / 2;
        for (int i = 0; i < m_data.size(); ++i) {
            const int y = y0 + i * ligneH;
            p.setPen(Qt::NoPen);
            p.setBrush(couleur(i));
            p.drawRoundedRect(QRectF(x0, y + 7, 10, 10), 2, 2);

            const int pct = qRound(100.0 * m_data[i].second / total);
            p.setPen(QColor("#374151"));
            p.drawText(QRectF(x0 + 18, y, width() - x0 - 18, ligneH), Qt::AlignVCenter | Qt::AlignLeft,
                       QString("%1 : %2 (%3%)").arg(m_data[i].first).arg(m_data[i].second).arg(pct));
        }
    }

private:
    QVector<QPair<QString, int>> m_data;
    QVector<QColor> m_couleurs;
    QString m_centre;
};

// ---------- Courbe (evolution dans le temps) ----------

class CourbeWidget : public QWidget
{
public:
    explicit CourbeWidget(const QVector<QPair<QString, int>> &data, QWidget *parent = nullptr)
        : QWidget(parent), m_data(data)
    {
        setMinimumHeight(170);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int n = m_data.size();
        if (n == 0) {
            p.setPen(QColor("#6B7280"));
            p.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("Aucune donnée"));
            return;
        }

        int maxV = 1;
        for (const auto &d : m_data) maxV = qMax(maxV, d.second);

        // Graduation de l'axe Y (entiers)
        const int pas = maxV <= 5 ? 1 : qCeil(maxV / 5.0);
        const int ymax = qMax(pas, qCeil(double(maxV) / pas) * pas);

        const QColor vert("#0F766E");
        const QRectF zone(34, 24, width() - 34 - 16, height() - 24 - 28);

        QFont f = font();
        f.setPointSize(8);
        p.setFont(f);

        // Grille horizontale + valeurs de l'axe Y
        for (int v = 0; v <= ymax; v += pas) {
            const double y = zone.bottom() - zone.height() * v / ymax;
            p.setPen(QPen(QColor(v == 0 ? "#D1D5DB" : "#EEF0F2"), 1));
            p.drawLine(QPointF(zone.left(), y), QPointF(zone.right(), y));
            p.setPen(QColor("#6B7280"));
            p.drawText(QRectF(0, y - 8, 28, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
        }

        // Points de la courbe
        const double marge = 24;
        QVector<QPointF> pts;
        for (int i = 0; i < n; ++i) {
            const double x = (n == 1) ? zone.center().x()
                                      : zone.left() + marge + (zone.width() - 2 * marge) * i / (n - 1);
            const double y = zone.bottom() - zone.height() * m_data[i].second / ymax;
            pts.append(QPointF(x, y));
        }

        // Trace lisse (sans depassement) + zone coloree sous la courbe
        QPainterPath chemin(pts[0]);
        for (int i = 1; i < n; ++i) {
            const double mx = (pts[i - 1].x() + pts[i].x()) / 2;
            chemin.cubicTo(QPointF(mx, pts[i - 1].y()), QPointF(mx, pts[i].y()), pts[i]);
        }

        if (n > 1) {
            QPainterPath aire = chemin;
            aire.lineTo(pts.last().x(), zone.bottom());
            aire.lineTo(pts.first().x(), zone.bottom());
            aire.closeSubpath();

            QLinearGradient degrade(0, zone.top(), 0, zone.bottom());
            degrade.setColorAt(0, QColor(15, 118, 110, 90));
            degrade.setColorAt(1, QColor(15, 118, 110, 5));
            p.setPen(Qt::NoPen);
            p.setBrush(degrade);
            p.drawPath(aire);

            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(vert, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPath(chemin);
        }

        // Etiquettes X : on en saute si les points sont trop serres
        const int saut = qMax(1, qCeil(n / qMax(1.0, zone.width() / 60.0)));

        for (int i = 0; i < n; ++i) {
            // point
            p.setPen(QPen(vert, 2.5));
            p.setBrush(Qt::white);
            p.drawEllipse(pts[i], 5, 5);

            // valeur au-dessus
            p.setPen(QColor("#374151"));
            p.drawText(QRectF(pts[i].x() - 20, pts[i].y() - 24, 40, 14), Qt::AlignCenter,
                       QString::number(m_data[i].second));

            // date en dessous
            if (i % saut == 0) {
                p.setPen(QColor("#6B7280"));
                p.drawText(QRectF(pts[i].x() - 30, zone.bottom() + 6, 60, 16), Qt::AlignCenter,
                           m_data[i].first);
            }
        }
    }

private:
    QVector<QPair<QString, int>> m_data;
};

// ---------- Barres horizontales ----------

class BarresHorizontalesWidget : public QWidget
{
public:
    BarresHorizontalesWidget(const QVector<QPair<QString, double>> &data, const QString &unite,
                             const QColor &couleur, QWidget *parent = nullptr)
        : QWidget(parent), m_data(data), m_unite(unite), m_couleur(couleur)
    {
        setMinimumHeight(130);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int n = m_data.size();
        if (n == 0) {
            p.setPen(QColor("#6B7280"));
            p.drawText(rect(), Qt::AlignCenter, QString::fromUtf8("Aucune donnée"));
            return;
        }

        QFont f = font();
        f.setPointSize(8);
        p.setFont(f);
        const QFontMetrics fm(f);

        double maxV = 1;
        int labelW = 0;
        for (const auto &d : m_data) {
            maxV = qMax(maxV, d.second);
            labelW = qMax(labelW, fm.horizontalAdvance(d.first));
        }
        labelW += 14;

        const int valeurW = 60;
        const double zoneBar = qMax(10, width() - labelW - valeurW - 10);
        const double ligneH = double(height()) / n;
        const double barH = qMin(ligneH * 0.55, 18.0);

        for (int i = 0; i < n; ++i) {
            const double cy = ligneH * i + ligneH / 2;

            p.setPen(QColor("#6B7280"));
            p.drawText(QRectF(0, cy - 10, labelW - 8, 20), Qt::AlignRight | Qt::AlignVCenter, m_data[i].first);

            const QRectF bar(labelW, cy - barH / 2, zoneBar * m_data[i].second / maxV, barH);
            p.setPen(Qt::NoPen);
            p.setBrush(m_couleur);
            p.drawRoundedRect(bar, 3, 3);

            p.setPen(QColor("#374151"));
            p.drawText(QRectF(bar.right() + 6, cy - 10, valeurW, 20), Qt::AlignLeft | Qt::AlignVCenter,
                       formaterNombre(m_data[i].second) + m_unite);
        }
    }

private:
    QVector<QPair<QString, double>> m_data;
    QString m_unite;
    QColor m_couleur;
};

} // namespace

// ---------- Fenetre ----------

StatistiquesDialog::StatistiquesDialog(const QVector<RdvStat> &donnees, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Statistiques des rendez-vous"));
    resize(1000, 700);
    setStyleSheet("QDialog { background: #FFFFFF; }");

    // --- Calculs a partir des donnees du tableau ---
    QMap<QString, int> nbParStatut;
    QMap<QDate, int> nbParDate;
    QMap<QString, int> nbParPriorite;   // cle = "rang|libelle" pour trier
    QMap<QString, int> nbParSalle;      // cle = "numero|libelle"
    int annules = 0;

    for (const RdvStat &r : donnees) {
        // statut
        const QString statut = majuscule(r.statut.trimmed().isEmpty() ? QString("Autre") : r.statut.trimmed());
        nbParStatut[statut] += 1;
        if (statut.startsWith("Annul", Qt::CaseInsensitive)) ++annules;

        // date
        if (r.date.isValid())
            nbParDate[r.date] += 1;

        // priorite (vide = normal)
        const QString prio = r.priorite.trimmed();
        const QString libPrio = prio.isEmpty() ? QString("Normal") : majuscule(prio);
        int rang = 0;
        if (!prio.isEmpty())
            rang = prio.contains(QString::fromUtf8("rés"), Qt::CaseInsensitive)
                   || prio.contains(QString::fromUtf8("res"), Qt::CaseInsensitive) ? 2 : 1;
        nbParPriorite[QString("%1|%2").arg(rang).arg(libPrio)] += 1;

        // salle
        const QString salle = r.salle.trimmed().isEmpty() ? QString("?") : r.salle.trimmed();
        nbParSalle[QString("%1|Salle %2").arg(salle.toInt(), 4, 10, QChar('0')).arg(salle)] += 1;
    }

    QVector<QPair<QString, int>> statuts;
    QVector<QColor> couleursStatut;
    int idx = 0;
    for (auto it = nbParStatut.cbegin(); it != nbParStatut.cend(); ++it, ++idx) {
        statuts.append(qMakePair(it.key(), it.value()));
        if (it.key().startsWith("Annul", Qt::CaseInsensitive))
            couleursStatut.append(QColor("#C62828"));
        else if (it.key().startsWith("Confirm", Qt::CaseInsensitive))
            couleursStatut.append(QColor("#0F766E"));
        else
            couleursStatut.append(couleurSerie(idx + 2));
    }

    QVector<QPair<QString, int>> courbe;
    for (auto it = nbParDate.cbegin(); it != nbParDate.cend(); ++it)
        courbe.append(qMakePair(it.key().toString("dd/MM"), it.value()));

    QVector<QPair<QString, double>> priorites;
    for (auto it = nbParPriorite.cbegin(); it != nbParPriorite.cend(); ++it)
        priorites.append(qMakePair(it.key().section('|', 1), double(it.value())));

    QVector<QPair<QString, double>> salles;
    for (auto it = nbParSalle.cbegin(); it != nbParSalle.cend(); ++it)
        salles.append(qMakePair(it.key().section('|', 1), double(it.value())));

    const int total = donnees.size();
    const int tauxAnnulation = total > 0 ? qRound(100.0 * annules / total) : 0;

    // --- Interface ---
    auto *titre = new QLabel(QString::fromUtf8("Statistiques des rendez-vous"));
    titre->setStyleSheet("font-size: 20pt; font-weight: bold; color: #1F2A37;");

    auto *carteStatut = creerCarte(
        QString::fromUtf8("Rendez-vous par statut"),
        QString::fromUtf8("Confirmés et annulés – taux d'annulation : %1 %").arg(tauxAnnulation),
        new DonutWidget(statuts, couleursStatut, QString::fromUtf8("rendez-vous")));

    auto *carteCourbe = creerCarte(
        QString::fromUtf8("Évolution des rendez-vous"),
        QString::fromUtf8("Nombre de rendez-vous par date"),
        new CourbeWidget(courbe));

    auto *cartePriorite = creerCarte(
        QString::fromUtf8("Rendez-vous par priorité"),
        QString::fromUtf8("Répartition selon le niveau d'urgence"),
        new BarresHorizontalesWidget(priorites, "", QColor("#E07B20")));

    auto *carteSalle = creerCarte(
        QString::fromUtf8("Occupation des salles"),
        QString::fromUtf8("Nombre de rendez-vous par salle"),
        new BarresHorizontalesWidget(salles, "", QColor("#1E9BE9")));

    auto *grille = new QGridLayout(this);
    grille->setContentsMargins(24, 18, 24, 24);
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(16);
    grille->addWidget(titre, 0, 0, 1, 2);
    grille->addWidget(carteStatut, 1, 0);
    grille->addWidget(carteCourbe, 1, 1);
    grille->addWidget(cartePriorite, 2, 0);
    grille->addWidget(carteSalle, 2, 1);
    grille->setRowStretch(1, 3);
    grille->setRowStretch(2, 2);
    grille->setColumnStretch(0, 1);
    grille->setColumnStretch(1, 1);
}