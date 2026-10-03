#include "statistiquesservices.h"

#include "barchart.h"
#include "donutchart.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QVBoxLayout>

namespace {

const char *kStyleStats = R"(
QDialog { background-color: #F4F8F7; }
QWidget { font-family: "Arial"; font-size: 11pt; color: #1F2A37; }
#carteStat { background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 12px; }
#titrePage { font-size: 20pt; font-weight: bold; color: #0B4F4A; }
#titreCarte { font-size: 13pt; font-weight: bold; color: #0B4F4A; }
#sousTitreCarte { font-size: 9pt; color: #6B7A78; }
)";

// Crée une carte blanche avec un titre, un sous-titre et le graphique donné.
QFrame *creerCarte(const QString &titre, const QString &sousTitre, QWidget *graphique, QWidget *parent)
{
    auto *carte = new QFrame(parent);
    carte->setObjectName("carteStat");
    auto *lay = new QVBoxLayout(carte);
    lay->setContentsMargins(18, 14, 18, 14);
    lay->setSpacing(4);

    auto *lblTitre = new QLabel(titre, carte);
    lblTitre->setObjectName("titreCarte");
    auto *lblSous = new QLabel(sousTitre, carte);
    lblSous->setObjectName("sousTitreCarte");

    lay->addWidget(lblTitre);
    lay->addWidget(lblSous);
    lay->addWidget(graphique, 1);
    return carte;
}

} // namespace

StatistiquesServices::StatistiquesServices(const QVector<Service> &services, QWidget *parent)
    : QDialog(parent)
    , m_services(services)
{
    setWindowTitle("Statistiques des services");
    setMinimumSize(1100, 650);
    resize(1200, 720);
    setStyleSheet(kStyleStats);

    construireInterface();
    calculer();
}

void StatistiquesServices::construireInterface()
{
    auto *racine = new QVBoxLayout(this);
    racine->setContentsMargins(28, 24, 28, 24);
    racine->setSpacing(14);

    // --- Titre ---
    auto *titre = new QLabel("Statistiques des services", this);
    titre->setObjectName("titrePage");
    racine->addWidget(titre);

    // --- Graphiques ---
    m_donut = new DonutChart(this);
    m_histoPrix = new BarChart(this);
    m_barresDuree = new BarChart(this);

    auto *grille = new QGridLayout;
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(18);
    grille->addWidget(creerCarte("Services par type",
                                 "Répartition du nombre de services selon leur type",
                                 m_donut, this), 0, 0);
    grille->addWidget(creerCarte("Histogramme des prix",
                                 "Nombre de services par tranche de prix (DT)",
                                 m_histoPrix, this), 0, 1);
    grille->addWidget(creerCarte("Durée moyenne par type de service",
                                 "Durée moyenne des services de chaque type (en minutes)",
                                 m_barresDuree, this), 1, 0, 1, 2);
    grille->setColumnStretch(0, 1);
    grille->setColumnStretch(1, 1);
    grille->setRowStretch(0, 1);
    grille->setRowStretch(1, 1);
    racine->addLayout(grille, 1);
}

void StatistiquesServices::calculer()
{
    // 1) Cercle : nombre de services par type
    QMap<QString, int> parType;

    // 2) Histogramme : tranches de prix
    struct Tranche { QString nom; double min; double max; int nb; };
    QVector<Tranche> tranches = {
        {"0 - 25",    0,    25,    0},
        {"25 - 50",   25,   50,    0},
        {"50 - 100",  50,   100,   0},
        {"100 - 200", 100,  200,   0},
        {"200 et +",  200,  1e12,  0},
    };

    // 3) Durée moyenne par type
    QMap<QString, int> sommeDuree;

    for (const Service &s : m_services) {
        parType[s.type]++;
        sommeDuree[s.type] += s.duree;
        for (Tranche &t : tranches) {
            if (s.prix >= t.min && s.prix < t.max) {
                t.nb++;
                break;
            }
        }
    }

    m_donut->setData(parType);

    QVector<QPair<QString, double>> histo;
    for (const Tranche &t : tranches)
        histo.append({t.nom, double(t.nb)});
    m_histoPrix->setData(histo, BarChart::Vertical, QString(), QColor("#F28C28"));

    QVector<QPair<QString, double>> durees;
    for (auto it = parType.constBegin(); it != parType.constEnd(); ++it)
        durees.append({it.key(), double(sommeDuree[it.key()]) / it.value()});
    m_barresDuree->setData(durees, BarChart::Horizontal, "min", QColor("#3B82F6"));
}
