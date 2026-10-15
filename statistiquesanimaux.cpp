#include "statistiquesanimaux.h"

#include "barchart.h"
#include "donutchart.h"
#include "linechart.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPair>
#include <QVBoxLayout>

namespace {

const char *kStyleStats = R"(
QDialog { background-color: #F4F8F7; }
QWidget { font-family: "Arial"; font-size: 11pt; color: #1F2A37; }
#carteStat { background-color: #FFFFFF; border: 1px solid #CBD5D3; border-radius: 12px; }
#titrePage { font-size: 20pt; font-weight: bold; color: #0B4F4A; }
#titreCarte { font-size: 13pt; font-weight: bold; color: #0B4F4A; }
#sousTitreCarte { font-size: 9pt; color: #6B7A78; }
#kpiTitre { font-size: 9pt; color: #6B7A78; }
#kpiValeur { font-size: 20pt; font-weight: bold; color: #0F766E; }
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

// Petite carte d'indicateur : un libellé et une grande valeur.
QFrame *creerKpi(const QString &titre, const QString &valeur, QWidget *parent)
{
    auto *carte = new QFrame(parent);
    carte->setObjectName("carteStat");
    auto *lay = new QVBoxLayout(carte);
    lay->setContentsMargins(18, 10, 18, 10);
    lay->setSpacing(0);

    auto *lblTitre = new QLabel(titre, carte);
    lblTitre->setObjectName("kpiTitre");
    auto *lblValeur = new QLabel(valeur, carte);
    lblValeur->setObjectName("kpiValeur");

    lay->addWidget(lblTitre);
    lay->addWidget(lblValeur);
    return carte;
}

} // namespace

StatistiquesAnimaux::StatistiquesAnimaux(const QVector<FicheAnimal> &animaux, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Statistiques des animaux");
    setMinimumSize(1100, 700);
    resize(1200, 760);
    setStyleSheet(kStyleStats);

    // ----- Calculs -----
    QMap<QString, int> parEspece;
    QMap<QString, int> parSexe;
    QVector<double> parMois(12, 0.0);
    int tranches[4] = { 0, 0, 0, 0 };                 // < 1 an, 1-3 ans, 4-7 ans, 8 ans et +
    int suivisAFaire = 0;
    const QDate aujourdhui = QDate::currentDate();
    const QString aJour = QString::fromUtf8("À jour");

    for (const FicheAnimal &a : animaux) {
        if (!a.espece.isEmpty()) parEspece[a.espece]++;
        if (!a.sexe.isEmpty())   parSexe[a.sexe]++;
        if (a.suiviSante.compare(aJour, Qt::CaseInsensitive) != 0)
            suivisAFaire++;
        if (a.naissance.isValid()) {
            parMois[a.naissance.month() - 1] += 1.0;
            const int age = int(a.naissance.daysTo(aujourdhui) / 365);
            if (age < 1)       tranches[0]++;
            else if (age <= 3) tranches[1]++;
            else if (age <= 7) tranches[2]++;
            else               tranches[3]++;
        }
    }

    QString especeDominante = "-";
    int max = 0;
    for (auto it = parEspece.constBegin(); it != parEspece.constEnd(); ++it) {
        if (it.value() > max) {
            max = it.value();
            especeDominante = it.key();
        }
    }

    // ----- Interface -----
    auto *racine = new QVBoxLayout(this);
    racine->setContentsMargins(28, 24, 28, 24);
    racine->setSpacing(14);

    auto *titre = new QLabel("Statistiques des animaux", this);
    titre->setObjectName("titrePage");
    racine->addWidget(titre);

    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);
    kpis->addWidget(creerKpi("Total animaux", QString::number(animaux.size()), this));
    kpis->addWidget(creerKpi(QString::fromUtf8("Espèces"), QString::number(parEspece.size()), this));
    kpis->addWidget(creerKpi(QString::fromUtf8("Espèce dominante"), especeDominante, this));
    kpis->addWidget(creerKpi(QString::fromUtf8("Suivis santé à faire"), QString::number(suivisAFaire), this));
    racine->addLayout(kpis);

    auto *donutEspece = new DonutChart(this);
    donutEspece->setUnite("animaux");
    donutEspece->setData(parEspece);

    auto *donutSexe = new DonutChart(this);
    donutSexe->setUnite("animaux");
    donutSexe->setData(parSexe);

    auto *histoAge = new BarChart(this);
    QVector<QPair<QString, double>> ages;
    ages.append({ QString::fromUtf8("< 1 an"),  double(tranches[0]) });
    ages.append({ QString::fromUtf8("1-3 ans"), double(tranches[1]) });
    ages.append({ QString::fromUtf8("4-7 ans"), double(tranches[2]) });
    ages.append({ QString::fromUtf8("8 ans et +"), double(tranches[3]) });
    histoAge->setData(ages, BarChart::Vertical, QString(), QColor("#F28C28"));

    auto *courbe = new LineChart(this);
    const QStringList mois = { "Jan", QString::fromUtf8("Fév"), "Mar", "Avr", "Mai", "Juin",
                               "Juil", QString::fromUtf8("Août"), "Sep", "Oct", "Nov", QString::fromUtf8("Déc") };
    QVector<QPair<QString, double>> naissances;
    for (int i = 0; i < 12; ++i)
        naissances.append({ mois[i], parMois[i] });
    courbe->setData(naissances);

    auto *grille = new QGridLayout;
    grille->setHorizontalSpacing(18);
    grille->setVerticalSpacing(18);
    grille->addWidget(creerCarte(QString::fromUtf8("Animaux par espèce"),
                                 QString::fromUtf8("Répartition du nombre d'animaux selon leur espèce"),
                                 donutEspece, this), 0, 0);
    grille->addWidget(creerCarte(QString::fromUtf8("Répartition par sexe"),
                                 QString::fromUtf8("Mâles et femelles suivis par la clinique"),
                                 donutSexe, this), 0, 1);
    grille->addWidget(creerCarte(QString::fromUtf8("Tranches d'âge"),
                                 QString::fromUtf8("Nombre d'animaux par tranche d'âge"),
                                 histoAge, this), 1, 0);
    grille->addWidget(creerCarte("Naissances par mois",
                                 QString::fromUtf8("Mois de naissance des animaux enregistrés"),
                                 courbe, this), 1, 1);
    grille->setColumnStretch(0, 1);
    grille->setColumnStretch(1, 1);
    grille->setRowStretch(0, 1);
    grille->setRowStretch(1, 1);
    racine->addLayout(grille, 1);
}
