#include "statistiquesemployes.h"

#include "barchart.h"
#include "donutchart.h"
#include "linechart.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QVBoxLayout>

#include <algorithm>

namespace {

const char *kStyleStats = R"(
QDialog { background-color: #F4F8F7; }
QWidget { font-family: "Arial"; font-size: 11pt; color: #1F2A37; }
#carteStat { background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 12px; }
#titrePage { font-size: 20pt; font-weight: bold; color: #0B4F4A; }
#titreCarte { font-size: 13pt; font-weight: bold; color: #0B4F4A; }
#sousTitreCarte { font-size: 9pt; color: #6B7A78; }
)";

// Carte blanche avec un titre, un sous-titre et le graphique donné.
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

StatistiquesEmployes::StatistiquesEmployes(const QVector<Employe> &employes, QWidget *parent)
    : QDialog(parent)
    , m_employes(employes)
{
    setWindowTitle(QString::fromUtf8("Statistiques des employés"));
    setMinimumSize(1100, 650);
    resize(1200, 720);
    setStyleSheet(kStyleStats);

    construireInterface();
    calculer();
}

void StatistiquesEmployes::construireInterface()
{
    auto *racine = new QVBoxLayout(this);
    racine->setContentsMargins(28, 24, 28, 24);
    racine->setSpacing(14);

    auto *titre = new QLabel(QString::fromUtf8("Statistiques des employés"), this);
    titre->setObjectName("titrePage");
    racine->addWidget(titre);

    m_donutStatut = new DonutChart(this);
    m_donutStatut->setUnite(QString::fromUtf8("employés"));
    m_barresPoste = new BarChart(this);
    m_barresSalaire = new BarChart(this);
    m_courbeRecrutement = new LineChart(this);

    auto *grille = new QGridLayout;
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(18);
    grille->addWidget(creerCarte(QString::fromUtf8("Répartition par statut"),
                                 QString::fromUtf8("Employés actifs et inactifs"),
                                 m_donutStatut, this), 0, 0);
    grille->addWidget(creerCarte(QString::fromUtf8("Répartition par poste"),
                                 QString::fromUtf8("Nombre d'employés pour chaque poste"),
                                 m_barresPoste, this), 0, 1);
    grille->addWidget(creerCarte(QString::fromUtf8("Analyse des salaires"),
                                 QString::fromUtf8("Salaire moyen, minimum et maximum (en DT)"),
                                 m_barresSalaire, this), 1, 0);
    grille->addWidget(creerCarte(QString::fromUtf8("Évolution des recrutements"),
                                 QString::fromUtf8("Nombre d'employés recrutés par année d'embauche"),
                                 m_courbeRecrutement, this), 1, 1);
    grille->setColumnStretch(0, 1);
    grille->setColumnStretch(1, 1);
    grille->setRowStretch(0, 1);
    grille->setRowStretch(1, 1);
    racine->addLayout(grille, 1);
}

void StatistiquesEmployes::calculer()
{
    QMap<QString, int> parStatut;
    QMap<QString, int> parPoste;
    QMap<int, int> parAnnee;

    for (const Employe &e : std::as_const(m_employes)) {
        parStatut[e.statut]++;
        parPoste[e.poste]++;
        if (e.dateEmbauche.isValid())
            parAnnee[e.dateEmbauche.year()]++;
    }

    m_donutStatut->setData(parStatut);

    QVector<QPair<QString, double>> postes;
    for (auto it = parPoste.constBegin(); it != parPoste.constEnd(); ++it)
        postes.append({it.key(), double(it.value())});
    m_barresPoste->setData(postes, BarChart::Horizontal, QString(), QColor("#F28C28"));

    if (!m_employes.isEmpty()) {
        double somme = 0, mini = m_employes.first().salaire, maxi = mini;
        for (const Employe &e : std::as_const(m_employes)) {
            somme += e.salaire;
            mini = std::min(mini, e.salaire);
            maxi = std::max(maxi, e.salaire);
        }
        QVector<QPair<QString, double>> salaires = {
            {QStringLiteral("Moyen"), somme / m_employes.size()},
            {QStringLiteral("Minimum"), mini},
            {QStringLiteral("Maximum"), maxi}
        };
        m_barresSalaire->setData(salaires, BarChart::Vertical, "DT", QColor("#0F766E"));
    }

    QVector<QPair<QString, double>> annees;
    if (!parAnnee.isEmpty()) {
        for (int a = parAnnee.firstKey(); a <= parAnnee.lastKey(); ++a)
            annees.append({QString::number(a), double(parAnnee.value(a, 0))});
    }
    m_courbeRecrutement->setData(annees, QColor("#0F766E"));
}
