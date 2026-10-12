#include "statistiquesemployes.h"

#include "ui_gsmartpetcare.h"

#include "barchart.h"
#include "donutchart.h"
#include "linechart.h"

#include <QMap>

#include <algorithm>

StatistiquesEmployes::StatistiquesEmployes(Ui::GSmartPetCare *ui, QObject *parent)
    : QObject(parent)
    , ui(ui)
    , m_voirSalaires(false)
{
    ui->donutStatut->setUnite("employés");
    connect(ui->btnRetourStats, &QPushButton::clicked, this, &StatistiquesEmployes::retour);
}

void StatistiquesEmployes::afficher(const QVector<Employe> &employes, bool voirSalaires)
{
    m_employes = employes;
    m_voirSalaires = voirSalaires;

    // Disposition : 2 x 2 cartes ; sans le privilège « voir les salaires », la carte des salaires
    // disparaît et la courbe des recrutements occupe toute la 2e ligne.
    ui->carteSalaire->setVisible(m_voirSalaires);
    ui->gridCartes->removeWidget(ui->carteRecrutement);
    if (m_voirSalaires)
        ui->gridCartes->addWidget(ui->carteRecrutement, 1, 1);
    else
        ui->gridCartes->addWidget(ui->carteRecrutement, 1, 0, 1, 2);
    calculer();
}

void StatistiquesEmployes::calculer()
{
    QMap<QString, int> parStatut;
    QMap<QString, int> parPoste;
    QMap<int, int> parAnnee;

    for (const Employe &e : m_employes) {
        parStatut[e.statut]++;
        parPoste[e.poste]++;
        if (e.dateEmbauche.isValid())
            parAnnee[e.dateEmbauche.year()]++;
    }

    // 1. Répartition par statut
    ui->donutStatut->setData(parStatut);

    // 2. Répartition par poste
    QVector<QPair<QString, double>> postes;
    for (auto it = parPoste.constBegin(); it != parPoste.constEnd(); ++it)
        postes.append({it.key(), double(it.value())});
    ui->barresPoste->setData(postes, BarChart::Horizontal, QString(), QColor("#F28C28"));

    // 3. Analyse des salaires : moyen, minimum, maximum
    if (m_voirSalaires && !m_employes.isEmpty()) {
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
        ui->barresSalaire->setData(salaires, BarChart::Vertical, "DT", QColor("#0F766E"));
    }

    // 4. Évolution des recrutements : toutes les années entre la première et la dernière
    //    embauche, y compris celles sans recrutement (0).
    QVector<QPair<QString, double>> annees;
    if (!parAnnee.isEmpty()) {
        for (int a = parAnnee.firstKey(); a <= parAnnee.lastKey(); ++a)
            annees.append({QString::number(a), double(parAnnee.value(a, 0))});
    }
    ui->courbeRecrutement->setData(annees, QColor("#0F766E"));
}
